#!/usr/bin/env bash
# A reusable Arch live environment plus a small, replaceable installer payload.
# Bump dummy-cache-v1 in the Qt generator when this bootstrap contract changes.

iso_dummy_snapshot() {
    local clone="$PROFILE/airootfs/opt/clone"
    mkdir -p "$clone"
    # A real, valid archive with no members, not a snapshot of any host directory.
    tar -I 'zstd -1' -cf "$clone/rootfs-snapshot.tar.zst" --files-from /dev/null
    (cd "$clone" && sha256sum rootfs-snapshot.tar.zst > snapshot.sha256)
    printf 'FORMAT=1\nARCH=x86_64\nBYTES=0\nKERNELS=1\nUEFI=1\nBIOS=1\nDUMMY=1\n' > "$clone/snapshot.meta"
    printf 'Xetal Engine TUI preview. Disk changes are simulated.\n' > "$clone/dummy-iso"
}

iso_dummy_launcher() {
    # This stays in the cached live filesystem. The current application supplies
    # the installer, logo and empty archive outside SquashFS on every build.
    cat > "$PROFILE/airootfs/xetal.sh" <<'XETAL_DUMMY_LAUNCHER'
#!/usr/bin/env bash
set -Eeuo pipefail
payload=/run/archiso/bootmnt/xetal-preview.tar
if [[ ! -r $payload ]]; then
    echo 'Dummy ISO payload is missing. Recreate this ISO with Create Dummy ISO.' >&2
    exit 1
fi
tar --no-same-owner -xf "$payload" -C /
exec bash /usr/local/bin/installer.sh "$@"
XETAL_DUMMY_LAUNCHER
}

iso_dummy_live_base() {
    local pair tool
    for pair in mkarchiso:archiso pacman:pacman pacman-key:pacman \
        bsdtar:libarchive findmnt:util-linux mksquashfs:squashfs-tools \
        mkfs.fat:dosfstools mcopy:mtools sudo:sudo; do
        iso_need "${pair%%:*}" "${pair##*:}" || return 1
    done
    [[ $(uname -m) == x86_64 ]] || { iso_die 'The dummy live ISO requires an x86_64 build host.'; return 1; }
    [[ -s /usr/share/pacman/keyrings/archlinux.gpg && -f $RELENG/profiledef.sh && -f $RELENG/packages.x86_64 ]] || {
        iso_die 'Install archiso and archlinux-keyring to prepare the dummy live environment.'; return 1;
    }
    echo '[*] Building the dummy Arch live environment for the first time (no system snapshot)...'
    # SUDO_PASS is consumed by run_sudo in iso/build.sh.
    # shellcheck disable=SC2034
    IFS= read -r -s SUDO_PASS || { iso_die 'No sudo credentials received.'; return 1; }
    run_sudo true
    PROFILE="$RUN/profile"; WORK="$RUN/work"
    cp -r "$RELENG" "$PROFILE"
    mkdir -p "$RUN/gnupg" "$RUN/pkg" "$RUN/base-output"
    run_sudo pacman-key --gpgdir "$RUN/gnupg" --init
    run_sudo pacman-key --gpgdir "$RUN/gnupg" --populate archlinux
    iso_write_pacman_conf "$PROFILE/pacman.conf" "$RUN/pkg" "$RUN/gnupg" 'https://geo.mirror.pkgbuild.com/$repo/os/$arch'
    cat >> "$PROFILE/profiledef.sh" <<EOF
iso_name='xetal-dummy-base'
iso_version='$(date +%Y.%m.%d-%H%M%S)'
airootfs_image_type='squashfs'
airootfs_image_tool_options=('-comp' 'zstd' '-Xcompression-level' '1' '-b' '1M')
file_permissions+=(['/xetal.sh']='0:0:755' ['/usr/local/bin/installer.sh']='0:0:755')
EOF
    # Use the same releng profile and installer dependencies as a full clone.
    for tool in arch-install-scripts bash util-linux coreutils rsync tar zstd grub \
        efibootmgr parted e2fsprogs dosfstools dialog chafa python; do
        printf '%s\n' "$tool" >> "$PROFILE/packages.x86_64"
    done
    if (( ISO_OFFLINE )); then
        [[ -r $OFFLINE_PACKAGE ]] || { iso_die "Offline archive not readable: $OFFLINE_PACKAGE"; return 1; }
        OFFLINE_CACHE_DIR="$RUN/offline-cache"
        mkdir -p "$OFFLINE_CACHE_DIR"
        tar --no-same-owner -xf "$OFFLINE_PACKAGE" -C "$OFFLINE_CACHE_DIR"
    fi
    iso_prepare_packages
    stage_iso_payload
    iso_dummy_snapshot
    iso_dummy_launcher
    echo '[*] Building ISO live environment...'
    run_sudo mkarchiso -v -w "$WORK" -o "$RUN/base-output" "$PROFILE"
    local -a images=()
    mapfile -t images < <(compgen -G "$RUN/base-output/xetal-dummy-base-*.iso")
    (( ${#images[@]} == 1 )) && [[ -s ${images[0]} ]] || {
        iso_die 'mkarchiso did not produce a single dummy base ISO.'; return 1;
    }
    run_sudo chown "$(id -u):$(id -g)" "${images[0]}"
    mkdir -p "${DUMMY_CACHE%/*}"
    mv -- "${images[0]}" "$DUMMY_CACHE"
    iso_unmount_tree "$WORK"
    run_sudo rm -rf --one-file-system -- "$WORK"
    WORK=''
}

iso_dummy_refresh() {
    local output=$1
    PROFILE="$RUN/payload"
    stage_iso_payload
    iso_dummy_snapshot
    tar -cf "$RUN/xetal-preview.tar" -C "$PROFILE/airootfs" opt/clone usr/local/bin/installer.sh
    echo '[*] Refreshing dummy installer payload (system snapshot skipped)...'
    # Replay the cached BIOS/UEFI boot entries and hybrid disk layout after edits.
    # https://www.gnu.org/software/xorriso/man_1_xorriso.html
    xorriso -abort_on FAILURE -return_with SORRY 32 \
        -indev "$DUMMY_CACHE" -outdev "$output" \
        -overwrite on -map "$RUN/xetal-preview.tar" /xetal-preview.tar \
        -boot_image any replay -compliance no_emul_toc -padding included -commit -end
    [[ -s $output ]] || { iso_die 'No dummy ISO was produced.'; return 1; }
}

iso_dummy_main() {
    set -Eeuo pipefail
    umask 077
    local pair image destination
    for pair in tar:tar zstd:zstd sha256sum:coreutils xorriso:libisoburn flock:util-linux; do
        iso_need "${pair%%:*}" "${pair##*:}" || return 1
    done
    [[ $BASE != *[[:space:]]* && $OUTPUT_DIR != *[[:space:]]* ]] || {
        iso_die 'archiso requires build and output paths without whitespace.'; return 1;
    }
    [[ $ISO_NAME =~ ^[a-zA-Z0-9_-]+$ ]] || { iso_die 'Invalid ISO name.'; return 1; }
    mkdir -p "$BASE/xiso/builds" "$OUTPUT_DIR"
    exec 9> "$BASE/xiso/build.lock"
    flock -n 9 || { iso_die 'An ISO build is already running.'; return 1; }
    echo '[*] Preparing dummy ISO (no host files or accounts are copied)...'
    RUN=$(mktemp -d "$BASE/xiso/builds/dummy-$(date +%Y%m%d-%H%M%S)-XXXXXX")
    WORK=''
    trap iso_build_cleanup EXIT
    trap 'exit 130' INT
    trap 'exit 143' TERM
    exec > >(tee -a "$RUN/build.log") 2>&1
    # Never replace the real clone's offline-export pointer with this preview.
    printf '%s\n' "$RUN" > "$BASE/xiso/last-dummy-build.txt"
    if [[ -f $DUMMY_CACHE && -r $DUMMY_CACHE && -s $DUMMY_CACHE ]]; then
        echo '[*] Reusing cached Arch live environment; no downloads or sudo needed.'
    else
        iso_dummy_live_base
    fi
    destination="$OUTPUT_DIR/$ISO_NAME-${RUN##*/}-x86_64.iso"
    image="$OUTPUT_DIR/.${destination##*/}.part"
    iso_dummy_refresh "$image"
    mv -- "$image" "$destination"
    printf '[✓] ISO ready: %s\n' "$destination"
    printf '[*] Dummy preview only: empty snapshot; all target disk changes are simulated.\n'
    printf '[*] Build log: %s/build.log\n' "$RUN"
}
