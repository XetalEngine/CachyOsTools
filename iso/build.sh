#!/usr/bin/env bash
# Embedded after common.sh and the application-generated configuration.

run_sudo() { printf '%s\n' "$SUDO_PASS" | sudo -S -p '' -- "$@"; }

iso_build_cleanup() {
    local status=$?
    trap - EXIT
    if [[ -n ${WORK:-} ]]; then iso_unmount_tree "$WORK" || status=1; fi
    if (( status != 0 )); then
        printf '[ERROR] ISO creation stopped. Build files and log are preserved in: %s\n' "${RUN:-not started}" >&2
    fi
    unset SUDO_PASS
    exit "$status"
}

iso_check_source_mounts() {
    local mount
    while IFS= read -r mount; do
        printf -v mount '%b' "$mount"
        case "$mount" in
            /boot|/boot/*|/efi|/home|/usr|/var)
                mountpoint -q "$mount" || { iso_die "Source filesystem $mount is listed in fstab but is not mounted."; return 1; } ;;
        esac
    done < <(findmnt --fstab --noheadings --raw --output TARGET)
}

iso_build_main() {
    set -Eeuo pipefail
    umask 077
    local tool pair initial_packages final_packages attempt rc archive file snapshot_bytes kernel_count
    for pair in mkarchiso:archiso pacman:pacman pacman-key:pacman \
        rsync:rsync tar:tar zstd:zstd bsdtar:libarchive \
        findmnt:util-linux flock:util-linux xorriso:libisoburn \
        mksquashfs:squashfs-tools mkfs.fat:dosfstools mcopy:mtools \
        sha256sum:coreutils file:file sudo:sudo; do
        iso_need "${pair%%:*}" "${pair##*:}" || return 1
    done
    iso_host_supported / || return 1
    iso_check_source_mounts || return 1
    iso_initramfs_tool / >/dev/null || return 1
    iso_kernels / >/dev/null || return 1
    if ! iso_bootloader / x86_64-efi >/dev/null 2>&1 && ! iso_bootloader / i386-pc >/dev/null 2>&1; then
        iso_die 'Install GRUB or systemd-boot in the source before cloning.'; return 1
    fi
    [[ -s /usr/share/pacman/keyrings/archlinux.gpg ]] || {
        iso_die 'Install archlinux-keyring from your distribution. An Arch trust store is required for the live ISO.'; return 1;
    }
    [[ -f $RELENG/profiledef.sh && -f $RELENG/packages.x86_64 ]] || {
        iso_die 'The installed archiso package has no releng profile. Install a complete upstream-compatible archiso package.'; return 1;
    }
    # mkarchiso/pacman serialize cache paths as whitespace-separated values.
    [[ $BASE != *[[:space:]]* && $OUTPUT_DIR != *[[:space:]]* ]] || {
        iso_die 'archiso requires build and output paths without whitespace.'; return 1;
    }
    [[ $ISO_NAME =~ ^[a-zA-Z0-9_-]+$ ]] || { iso_die 'Invalid ISO name.'; return 1; }
    mkdir -p "$BASE/xiso/builds" "$OUTPUT_DIR"
    exec 9> "$BASE/xiso/build.lock"
    flock -n 9 || { iso_die 'An ISO build is already running.'; return 1; }
    echo '[*] Preparing HOME-only build tree...'
    RUN=$(mktemp -d "$BASE/xiso/builds/build-$(date +%Y%m%d-%H%M%S)-XXXXXX")
    PROFILE="$RUN/profile"; WORK="$RUN/work"
    SNAPDIR="$RUN/snapshot"
    trap iso_build_cleanup EXIT
    trap 'exit 130' INT
    trap 'exit 143' TERM
    exec > >(tee -a "$RUN/build.log") 2>&1
    printf '%s\n' "$RUN" > "$BASE/xiso/last-build.txt"
    cp -r "$RELENG" "$PROFILE"
    # Authenticate without storing the password in generated files or the environment.
    IFS= read -r -s SUDO_PASS || { iso_die 'No sudo credentials received.'; return 1; }
    run_sudo true
    mkdir -p "$RUN/gnupg" "$RUN/pkg"
    run_sudo pacman-key --gpgdir "$RUN/gnupg" --init
    run_sudo pacman-key --gpgdir "$RUN/gnupg" --populate archlinux
    iso_write_pacman_conf "$PROFILE/pacman.conf" "$RUN/pkg" "$RUN/gnupg" 'https://geo.mirror.pkgbuild.com/$repo/os/$arch'
    # Keep the boot modes shipped with this archiso release; do not rewrite its
    # bootmode array with a parser that breaks single-line or newer profiles.
    cat >> "$PROFILE/profiledef.sh" <<EOF
iso_name='$ISO_NAME'
iso_version='$(date +%Y.%m.%d-%H%M%S)'
airootfs_image_type='squashfs'
airootfs_image_tool_options=('-comp' 'zstd' '-Xcompression-level' '6' '-b' '1M')
file_permissions+=(['/xetal.sh']='0:0:755' ['/usr/local/bin/installer.sh']='0:0:755')
EOF
    for tool in arch-install-scripts bash util-linux coreutils rsync tar zstd grub \
        efibootmgr parted e2fsprogs dosfstools dialog chafa; do
        printf '%s\n' "$tool" >> "$PROFILE/packages.x86_64"
    done
    if (( ISO_OFFLINE )); then
        [[ -r $OFFLINE_PACKAGE ]] || { iso_die "Offline archive not readable: $OFFLINE_PACKAGE"; return 1; }
        OFFLINE_CACHE_DIR="$RUN/offline-cache"
        mkdir -p "$OFFLINE_CACHE_DIR"
        # Extract unprivileged; GNU tar rejects traversal through absolute/.. names.
        tar --no-same-owner -xf "$OFFLINE_PACKAGE" -C "$OFFLINE_CACHE_DIR"
    fi
    iso_prepare_packages

    [[ ! -e /var/lib/pacman/db.lck ]] || { iso_die 'A package transaction is running. Retry after it finishes.'; return 1; }
    initial_packages=$(pacman -Q)
    mkdir -p "$SNAPDIR"
    local -a excludes=(--exclude=/proc/* --exclude=/sys/* --exclude=/dev/*
        --exclude=/run/* --exclude=/tmp/* --exclude=/mnt/* --exclude=/media/*
        --exclude=/lost+found --exclude=/.snapshots/* --exclude=/home/.snapshots/*
        "--exclude=$BASE/*" "--exclude=$OUTPUT_DIR/*")
    for file in "${USER_EXCLUDES[@]}"; do excludes+=("--exclude=$file"); done
    echo '[*] Creating full-system snapshot...'
    for attempt in 1 2; do
        rc=0
        run_sudo rsync -aHAXS --numeric-ids --info=progress2,stats2 "${excludes[@]}" / "$SNAPDIR/" || rc=$?
        (( rc != 0 )) || break
        if (( rc != 24 || attempt == 2 )); then
            iso_die "Snapshot incomplete (rsync exit $rc). Close changing applications and fix any unreadable files before retrying."; return 1
        fi
        echo '[*] Files changed during the snapshot; retrying once...'
    done
    final_packages=$(pacman -Q)
    [[ ! -e /var/lib/pacman/db.lck && ! -e $SNAPDIR/var/lib/pacman/db.lck && $initial_packages == "$final_packages" ]] || {
        iso_die 'Packages changed while copying. The snapshot will not be used; retry after updates finish.'; return 1;
    }
    iso_host_supported "$SNAPDIR" || return 1
    run_sudo chroot "$SNAPDIR" /usr/bin/bash -c \
        'test -s /etc/passwd && test -s /etc/group && test -x /usr/lib/systemd/systemd && test -x /usr/bin/pacman' || {
        iso_die 'The snapshot is missing essential system files. Review the exclusions and retry.'; return 1;
    }
    if ! iso_bootloader "$SNAPDIR" x86_64-efi >/dev/null 2>&1 && ! iso_bootloader "$SNAPDIR" i386-pc >/dev/null 2>&1; then
        iso_die 'The snapshot exclusions removed the supported bootloader. Review the exclusions and retry.'; return 1
    fi
    iso_kernels "$SNAPDIR" > "$RUN/kernels.txt"
    kernel_count=$(wc -l < "$RUN/kernels.txt")
    iso_initramfs_tool "$SNAPDIR" >/dev/null
    snapshot_bytes=$(run_sudo du -sx --apparent-size --block-size=1 "$SNAPDIR" | cut -f1)
    mkdir -p "$PROFILE/airootfs/opt/clone"
    archive="$PROFILE/airootfs/opt/clone/rootfs-snapshot.tar.zst"
    echo '[*] Packing snapshot...'
    run_sudo tar --xattrs --acls --numeric-owner --sparse -C "$SNAPDIR" -I 'zstd -6 -T0' -cpf "$archive" .
    run_sudo chown "$(id -u):$(id -g)" "$archive"
    zstd -t "$archive"
    (cd "${archive%/*}" && sha256sum rootfs-snapshot.tar.zst > snapshot.sha256)
    {
        printf 'FORMAT=1\nARCH=x86_64\nBYTES=%s\nKERNELS=%s\n' "$snapshot_bytes" "$kernel_count"
        if iso_bootloader "$SNAPDIR" x86_64-efi >/dev/null 2>&1; then echo 'UEFI=1'; else echo 'UEFI=0'; fi
        if iso_bootloader "$SNAPDIR" i386-pc >/dev/null 2>&1; then echo 'BIOS=1'; else echo 'BIOS=0'; fi
    } > "$PROFILE/airootfs/opt/clone/snapshot.meta"
    echo '[*] Cleaning up temporary snapshot directory...'
    run_sudo rm -rf --one-file-system -- "$SNAPDIR"
    echo '[*] Embedding snapshot and installer...'
    stage_iso_payload
    echo '[*] Building ISO...'
    run_sudo mkarchiso -v -w "$WORK" -o "$OUTPUT_DIR" "$PROFILE"
    compgen -G "$OUTPUT_DIR/$ISO_NAME-*.iso" >/dev/null || { iso_die 'mkarchiso produced no ISO.'; return 1; }
    echo '[*] Final cleanup...'
    iso_unmount_tree "$WORK"
    run_sudo rm -rf --one-file-system -- "$WORK"
    printf '[✓] ISO ready in: %s\n' "$OUTPUT_DIR"
    printf '[*] Build log and offline-package inputs: %s\n' "$RUN"
}

iso_build_main "$@"
