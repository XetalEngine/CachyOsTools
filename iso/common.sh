#!/usr/bin/env bash
# Shared by the builder, installer and regression tests. Sourcing has no effects.
# Partition/Limine helpers adapted from devvz's CachyOsTools PR #7.

iso_die() { printf '[ERROR] %s\n' "$*" >&2; return 1; }
iso_need() { command -v "$1" >/dev/null 2>&1 || iso_die "Missing tool: $1 ($2)"; }
iso_is_block() { [[ -b $1 ]]; }

iso_host_supported() {
    local root=${1:-/} arch=${2:-$(uname -m)}
    [[ $arch == x86_64 ]] || { iso_die "ISO creation supports x86_64 only (found $arch)."; return 1; }
    [[ -d $root/var/lib/pacman/local && -f $root/usr/lib/systemd/systemd ]] || {
        iso_die 'The source must be a pacman-based distribution using systemd.'; return 1;
    }
}

# Kernel package names differ (linux, linux-lts, linux-cachyos, linux612, ...).
# Use each installed module tree's pkgbase instead of the running kernel name.
iso_kernels() {
    local root=${1:-/} tree version pkgbase kernel candidate description count=0
    for tree in "$root"/usr/lib/modules/*; do
        [[ -d $tree && -s $tree/modules.dep ]] || continue
        version=${tree##*/}
        pkgbase=$version
        if [[ -f $tree/pkgbase ]]; then read -r pkgbase < "$tree/pkgbase" || [[ -n $pkgbase ]] || return 1; fi
        [[ $version =~ ^[a-zA-Z0-9._+-]+$ && $pkgbase =~ ^[a-zA-Z0-9._+-]+$ ]] || {
            iso_die 'Invalid kernel metadata.'; return 1;
        }
        kernel="/usr/lib/modules/$version/vmlinuz"
        if [[ ! -s $root$kernel ]]; then kernel="/boot/vmlinuz-$pkgbase"; fi
        if [[ ! -s $root$kernel ]]; then
            # Manjaro and custom kernels may use versioned /boot filenames,
            # unrelated to pkgbase. Match the image's embedded kernel release.
            kernel=''
            for candidate in "$root"/boot/vmlinuz-*; do
                [[ -s $candidate ]] || continue
                description=$(file --brief -- "$candidate") || return 1
                if [[ $description == *"version $version "* || $description == *"version $version,"* ]]; then
                    kernel=${candidate#"$root"}
                    [[ $kernel == /* ]] || kernel="/$kernel"
                    break
                fi
            done
        fi
        [[ -n $kernel && -s $root$kernel ]] || continue
        printf '%s\t%s\t%s\n' "$version" "$pkgbase" "$kernel"
        count=$((count + 1))
    done
    (( count > 0 )) || iso_die 'No complete installed kernel was found. Mount /boot and reinstall the source kernel before cloning.'
}

iso_initramfs_tool() {
    local root=${1:-/}
    if [[ -x $root/usr/bin/mkinitcpio ]]; then printf 'mkinitcpio\n'
    elif [[ -x $root/usr/bin/dracut ]]; then printf 'dracut\n'
    else iso_die 'The source needs mkinitcpio or dracut to regenerate its boot image.'; fi
}

# True when the source is really set up for Limine: the package files plus a sign it
# is configured (CachyOS's /etc/default/limine and limine-entry-tool config, or a
# limine.conf). Having the package installed alone must not change the bootloader
# chosen for a GRUB or systemd-boot source.
iso_uses_limine() {
    local root=$1 marker
    [[ -x $root/usr/bin/limine && -s $root/usr/share/limine/BOOTX64.EFI ]] || return 1
    for marker in etc/default/limine etc/limine-entry-tool.conf boot/limine.conf boot/limine/limine.conf \
                  efi/limine.conf efi/EFI/BOOT/limine.conf boot/EFI/BOOT/limine.conf boot/EFI/limine/limine.conf; do
        [[ -e $root/$marker ]] && return 0
    done
    return 1
}

iso_bootloader() {
    local root=$1 mode=$2
    if [[ -x $root/usr/bin/grub-install && -d $root/usr/lib/grub/$mode ]]; then
        printf 'grub\n'
    elif [[ $mode == x86_64-efi ]] && iso_uses_limine "$root"; then
        printf 'limine\n'
    elif [[ $mode == x86_64-efi && -x $root/usr/bin/bootctl &&
            -s $root/usr/lib/systemd/boot/efi/systemd-bootx64.efi ]]; then
        printf 'systemd-boot\n'
    else
        iso_die "This snapshot has no supported $mode bootloader. Install GRUB in the source, or use UEFI with Limine or systemd-boot available."
    fi
}

iso_partition_path() {
    [[ $1 =~ [0-9]$ ]] && printf '%sp%s\n' "$1" "$2" || printf '%s%s\n' "$1" "$2"
}

# Match whole device ancestry, including mapper devices, partitions and loop ISOs.
iso_disk_ancestors() {
    local source=$1 backing
    source=${source%%\[*}
    iso_is_block "$source" || return 0
    if [[ $source == /dev/loop* ]]; then
        backing=$(losetup --noheadings --raw --output BACK-FILE "$source") || return 1
        [[ -n $backing ]] || return 1
        source=$(findmnt -nro SOURCE -T "$backing") || return 1
        source=${source%%\[*}
    fi
    lsblk --inverse --list --noheadings --paths --output NAME,TYPE "$source" |
        awk '$2 == "disk" {print $1}'
}

iso_validate_disk() {
    local disk=$1 protected=$2 line _name type mounted swap ancestors children swaps
    if ! iso_is_block "$disk" || [[ $(lsblk -dnro TYPE "$disk") != disk || $(lsblk -dnro RO "$disk") != 0 ]]; then
        iso_die 'Select a writable whole disk.'; return 1;
    fi
    while IFS= read -r line; do
        [[ $disk != "$line" ]] || { iso_die 'The selected disk contains the running system or installation medium.'; return 1; }
    done <<< "$protected"
    mounted=$(lsblk -nrpo MOUNTPOINTS "$disk") || return 1
    [[ -z ${mounted//[[:space:]]/} ]] || {
        iso_die 'The selected disk has mounted filesystems or active swap. Unmount them before installing.'; return 1;
    }
    # Do not erase a disk used by a mapper/RAID/LVM device, even if unmounted.
    children=$(lsblk -nrpo NAME,TYPE "$disk") || return 1
    [[ -n $children ]] || { iso_die 'Cannot inspect disk consumers.'; return 1; }
    while read -r _name type; do
        [[ $type == disk || $type == part ]] || {
            iso_die 'The disk is in use by a mapper, RAID or LVM device.'; return 1;
        }
    done <<< "$children"
    swaps=$(swapon --noheadings --raw --show=NAME) || return 1
    while read -r swap; do
        [[ -n $swap ]] || continue
        ancestors=$(iso_disk_ancestors "$swap") || return 1
        [[ $'\n'"$ancestors"$'\n' != *$'\n'"$disk"$'\n'* ]] || {
            iso_die 'The selected disk contains active swap.'; return 1;
        }
    done <<< "$swaps"
}

# ---------------------------------------------------------------------------
# Partition-aware install helpers (free space / existing partitions).
# All of these only READ the disk except iso_create_efi_root, which creates two
# new partitions strictly inside a free region and verifies that every
# pre-existing partition is byte-for-byte unchanged afterwards.
# ---------------------------------------------------------------------------

# Prints the partition table type of a disk (gpt, msdos, loop, or nothing).
iso_gpt_table() {
    parted -m -s "$1" unit B print 2>/dev/null | awk -F: 'NR == 2 {print $6}'
}

# One line per existing partition: NUMBER<TAB>START<TAB>END<TAB>SIZE (bytes).
iso_partition_rows() {
    local out
    out=$(parted -m -s "$1" unit B print 2>/dev/null) || return 1
    awk -F: 'NR > 2 && $1 ~ /^[0-9]+$/ && $5 !~ /^free;?$/ {
        gsub(/B/, "", $2); gsub(/B/, "", $3); gsub(/B/, "", $4)
        print $1 "\t" $2 "\t" $3 "\t" $4 }' <<< "$out"
}

# Partition number of the partition starting at the given byte offset.
iso_partition_at() {
    iso_partition_rows "$1" | awk -F'\t' -v s="$2" '$2 == s {print $1; found = 1} END {exit !found}'
}

# Free regions of at least MIN_BYTES, as START_MIB<TAB>END_MIB (END exclusive).
# Start is rounded up and end rounded down to whole MiB so new partitions are
# always 1 MiB aligned and can never spill into a neighbouring partition.
iso_free_regions() {
    local disk=$1 min_bytes=$2 mib=1048576 out start end s e
    out=$(parted -m -s "$disk" unit B print free 2>/dev/null) || return 1
    while IFS=$'\t' read -r start end; do
        [[ $start =~ ^[0-9]+$ && $end =~ ^[0-9]+$ ]] || continue
        s=$(((start + mib - 1) / mib)); e=$(((end + 1) / mib))
        (( s > 0 && (e - s) * mib >= min_bytes )) || continue
        printf '%s\t%s\n' "$s" "$e"
    done < <(awk -F: 'NR > 2 && $5 ~ /^free;?$/ {
        gsub(/B/, "", $2); gsub(/B/, "", $3); print $2 "\t" $3 }' <<< "$out")
}

# Create an EFI partition (ESP_MIB) followed by a root partition filling the rest
# of [START_MIB, END_MIB). Prints "ESP_NUMBER ROOT_NUMBER" on success.
iso_create_efi_root() {
    local disk=$1 start=$2 esp_mib=$3 end=$4 mib=1048576
    local mid before after esp_num root_num n s e z
    [[ $start =~ ^[0-9]+$ && $esp_mib =~ ^[0-9]+$ && $end =~ ^[0-9]+$ ]] &&
        (( start > 0 && esp_mib > 0 && end > start + esp_mib )) || {
        iso_die 'Invalid partition plan.'; return 1;
    }
    mid=$((start + esp_mib))
    before=$(iso_partition_rows "$disk") || { iso_die 'Cannot read the partition table.'; return 1; }
    parted -s "$disk" mkpart ESP fat32 "${start}MiB" "${mid}MiB" >/dev/null || return 1
    esp_num=$(iso_partition_at "$disk" $((start * mib))) || { iso_die 'The new EFI partition was not found.'; return 1; }
    parted -s "$disk" set "$esp_num" esp on >/dev/null || return 1
    parted -s "$disk" mkpart ROOT ext4 "${mid}MiB" "${end}MiB" >/dev/null || return 1
    root_num=$(iso_partition_at "$disk" $((mid * mib))) || { iso_die 'The new root partition was not found.'; return 1; }
    after=$(iso_partition_rows "$disk") || return 1
    # Post-condition: nothing that existed before may have moved or resized.
    while IFS=$'\t' read -r n s e z; do
        [[ -n $n ]] || continue
        grep -qxF -- "$n"$'\t'"$s"$'\t'"$e"$'\t'"$z" <<< "$after" || {
            iso_die "Existing partition $n was modified. Aborting before formatting anything."; return 1;
        }
    done <<< "$before"
    printf '%s %s\n' "$esp_num" "$root_num"
}

# Like iso_validate_disk, but for a single partition that will be formatted.
iso_validate_partition() {
    local part=$1 protected=$2 line _name type pdisk swap swaps children mounted
    if ! iso_is_block "$part" || [[ $(lsblk -dnro TYPE "$part") != part || $(lsblk -dnro RO "$part") != 0 ]]; then
        iso_die 'Select a writable partition.'; return 1
    fi
    pdisk=$(iso_disk_ancestors "$part") || return 1
    [[ -n $pdisk && $pdisk != *$'\n'* ]] || { iso_die 'Cannot identify the disk of that partition.'; return 1; }
    [[ $(iso_gpt_table "$pdisk") == gpt ]] || { iso_die 'Prepared partitions must be on a GPT disk.'; return 1; }
    while IFS= read -r line; do
        [[ -z $line ]] || [[ $pdisk != "$line" ]] || {
            iso_die 'That partition is on the running system or installation medium.'; return 1;
        }
    done <<< "$protected"
    mounted=$(lsblk -nrpo MOUNTPOINTS "$part") || return 1
    [[ -z ${mounted//[[:space:]]/} ]] || { iso_die "$part is mounted or in use as swap."; return 1; }
    children=$(lsblk -nrpo NAME,TYPE "$part") || return 1
    [[ -n $children ]] || { iso_die 'Cannot inspect partition consumers.'; return 1; }
    while read -r _name type; do
        [[ $_name == "$part" && $type == part ]] || { iso_die "$part is in use by another device."; return 1; }
    done <<< "$children"
    swaps=$(swapon --noheadings --raw --show=NAME) || return 1
    while read -r swap; do
        [[ -n $swap ]] || continue
        [[ $swap != "$part" ]] || { iso_die "$part is active swap."; return 1; }
    done <<< "$swaps"
}

# Only an empty, dedicated GPT ESP may be formatted. Protect existing bootloaders
# (Windows, Linux, recovery, or shared ESPs) without relying on a typed warning.
iso_validate_esp_target() (
    local part=$1 type fs mnt='' entry status
    type=$(lsblk -dnro PARTTYPE "$part") || return 1
    [[ ${type,,} == c12a7328-f81f-11d2-ba4b-00a0c93ec93b ]] || {
        iso_die 'Choose a partition marked as an EFI system partition.'; return 1;
    }
    fs=$(blkid -p -o value -s TYPE "$part" 2>/dev/null) || {
        status=$?
        # blkid uses status 2 for no recognizable signature. Other failures
        # must not be interpreted as an empty partition.
        [[ $status == 2 ]] || return 1
        fs=''
    }
    [[ -n $fs ]] || return 0
    [[ $fs == vfat ]] || { iso_die 'The EFI target contains another filesystem.'; return 1; }
    mnt=$(mktemp -d) || return 1
    trap 'if mountpoint -q "$mnt"; then umount "$mnt" || exit 1; fi; rmdir "$mnt"' EXIT
    mount -o ro,noexec,nosuid,nodev "$part" "$mnt" || return 1
    entry=$(find "$mnt" -mindepth 1 -maxdepth 1 -print -quit) || return 1
    [[ -z $entry ]] || { iso_die 'This EFI partition contains files. Existing boot partitions are protected.'; return 1; }
)

iso_unmount_tree() {
    local root=$1 encoded path failed=0
    # /proc encodes spaces and backslashes. Decode before comparing or unmounting.
    while IFS= read -r encoded; do
        printf -v path '%b' "$encoded"
        [[ $path == "$root" || $path == "$root/"* ]] || continue
        run_sudo umount -- "$path" || failed=1
    done < <(awk '{print $2}' /proc/self/mounts | LC_ALL=C sort -r)
    (( failed == 0 )) || iso_die "Could not unmount $root; it has been preserved."
}

# ---------------------------------------------------------------------------
# Secure Boot signing is opt-in. A requested signature must succeed and verify;
# a failed signature must never be reported as a signed, bootable installation.
# ---------------------------------------------------------------------------

# iso_sbctl_ready [ROOT] - true if ROOT (default /) has sbctl and a db signing key.
# ROOT is optional; omitting it checks the current system.
# shellcheck disable=SC2120
iso_sbctl_ready() {
    local root=${1:-}
    # XETAL_NO_SIGN=1 turns off all Secure Boot signing and Limine hash pinning.
    [[ ${XETAL_NO_SIGN:-0} != 1 ]] || return 1
    [[ -x $root/usr/bin/sbctl ]] &&
        { [[ -r $root/var/lib/sbctl/keys/db/db.key && -r $root/var/lib/sbctl/keys/db/db.pem ]] ||
          [[ -r $root/usr/share/secureboot/keys/db/db.key && -r $root/usr/share/secureboot/keys/db/db.pem ]]; }
}

iso_signing_enabled() {
    [[ ${XETAL_NO_SIGN:-0} != 1 && ${SIGN_BOOT:-${XETAL_SIGN_BOOT:-0}} == 1 ]]
}

iso_verify_boot_file() {
    local result
    result=$(sbctl --json verify "$1") || return 1
    # sbctl returns zero even for unsigned/missing/invalid files. Its JSON
    # result, not its process status, must affirm the requested file is signed.
    grep -Eq '"is_signed"[[:space:]]*:[[:space:]]*(1|true)([[:space:],}]|$)' <<< "$result" || {
        iso_die "Secure Boot signature verification failed for $1."; return 1;
    }
}

# Staged filenames must not be saved in sbctl's persistent file database.
iso_sign_boot_file() {
    iso_signing_enabled || return 0
    iso_sbctl_ready || { iso_die 'Signing was requested but sbctl keys/certificate are unavailable.'; return 1; }
    sbctl sign "$1" >/dev/null && iso_verify_boot_file "$1" || {
        iso_die "Could not sign and verify $1 for Secure Boot."; return 1;
    }
}

# iso_windows_esp_guid - print the PARTUUID of an EFI system partition that holds the
# Windows boot manager, if exactly one is found. Strictly read-only: other ESPs are only
# mounted read-only/noexec for a file check, and the one mounted at /efi is skipped.
iso_windows_esp_guid() {
    local esp_type=c12a7328-f81f-11d2-ba4b-00a0c93ec93b own path type guid mnt found=
    own=$(findmnt -no SOURCE /efi 2>/dev/null || true)
    mnt=$(mktemp -d) || return 0
    while read -r path type guid; do
        [[ ${type,,} == "$esp_type" && -n $guid && $path != "$own" ]] || continue
        if mount -o ro,noexec,nosuid,nodev "$path" "$mnt" 2>/dev/null; then
            [[ -f $mnt/EFI/Microsoft/Boot/bootmgfw.efi ]] && found+="$guid"$'\n'
            umount "$mnt" 2>/dev/null || true
        fi
    done < <(lsblk -rno PATH,PARTTYPE,PARTUUID 2>/dev/null)
    rmdir "$mnt" 2>/dev/null || true
    # Several Windows ESPs would be ambiguous, so add nothing rather than guess.
    [[ $(printf '%s' "$found" | grep -c .) == 1 ]] && printf '%s' "$found"
    return 0
}

iso_write_pacman_conf() {
    local destination=$1 cache=$2 keyring=$3 server=$4
    # Explicit repositories prevent a Manjaro mirrorlist or CachyOS overrides
    # from supplying packages to the Arch live environment.
    cat > "$destination" <<EOF
[options]
Architecture = x86_64
SigLevel = Required DatabaseOptional
LocalFileSigLevel = Required
GPGDir = $keyring
CacheDir = $cache
ParallelDownloads = 5
[core]
Server = $server
[extra]
Server = $server
EOF
    if [[ $server == https://geo.mirror.pkgbuild.com/* ]]; then
        sed -i '/^Server = /a Server = https://mirrors.kernel.org/archlinux/$repo/os/$arch' "$destination"
    fi
}

iso_normalize_packages() {
    local package i have_dkms=0
    mapfile -t iso_packages < <(sed 's/#.*//;s/^[[:space:]]*//;s/[[:space:]]*$//;/^$/d' "$PROFILE/packages.x86_64")
    (( ${#iso_packages[@]} )) || { iso_die 'The ISO package list is empty.'; return 1; }
    for i in "${!iso_packages[@]}"; do
        package=${iso_packages[i]}
        if [[ $package == broadcom-wl ]]; then
            if (( ISO_OFFLINE )) && run_sudo pacman "${pacman_args[@]}" -Si broadcom-wl >/dev/null 2>&1; then
                continue
            fi
            echo '[*] Updating live ISO driver: broadcom-wl -> broadcom-wl-dkms'
            iso_packages[i]=broadcom-wl-dkms
        fi
        [[ ${iso_packages[i]} != broadcom-wl-dkms ]] || have_dkms=1
    done
    (( ! have_dkms )) || iso_packages+=(linux-headers)
    printf '%s\n' "${iso_packages[@]}" | awk '!seen[$0]++' > "$PROFILE/packages.x86_64"
    mapfile -t iso_packages < "$PROFILE/packages.x86_64"
}

iso_local_mirror() {
    local db=$1 cache=$2 mirror=$3 repo file
    mkdir -p "$mirror/core/os/x86_64" "$mirror/extra/os/x86_64"
    for repo in core extra; do
        [[ -s $db/sync/$repo.db ]] || { iso_die "Missing offline database: $repo.db"; return 1; }
        cp "$db/sync/$repo.db" "$mirror/$repo/os/x86_64/"
        if [[ -f $db/sync/$repo.db.sig ]]; then cp "$db/sync/$repo.db.sig" "$mirror/$repo/os/x86_64/"; fi
        for file in "$cache"/*.pkg.tar.*; do
            [[ -f $file && $file != *.part ]] || continue
            ln -sf "$file" "$mirror/$repo/os/x86_64/${file##*/}"
        done
    done
}

iso_prepare_packages() {
    echo '[*] Checking live ISO packages before creating the system snapshot...'
    mkdir -p "$RUN/db/local" "$RUN/pkg"
    pacman_args=(--config "$PROFILE/pacman.conf" --dbpath "$RUN/db"
                 --logfile "$RUN/pacman.log" --noconfirm)
    if (( ISO_OFFLINE )); then
        [[ -d $OFFLINE_CACHE_DIR/offline-packages/sync && -d $OFFLINE_CACHE_DIR/offline-packages/pkg ]] || {
            iso_die 'Offline archive needs both sync/ and pkg/. Create a new archive with create_offline_package.sh.'; return 1;
        }
        cp -r "$OFFLINE_CACHE_DIR/offline-packages/sync" "$RUN/db/"
        cp -a --reflink=auto "$OFFLINE_CACHE_DIR/offline-packages/pkg/." "$RUN/pkg/"
        iso_local_mirror "$RUN/db" "$RUN/pkg" "$RUN/mirror" || return 1
        iso_write_pacman_conf "$PROFILE/pacman.conf" "$RUN/pkg" "$RUN/gnupg" "file://$RUN/mirror/\$repo/os/\$arch"
    else
        run_sudo pacman "${pacman_args[@]}" -Sy || {
            iso_die 'Cannot refresh the Arch ISO repositories. Check the network, mirrors and archlinux-keyring.'; return 1;
        }
    fi
    iso_normalize_packages || return 1
    # Resolve and download every dependency against an empty installed-package DB.
    # Pin these databases and archives before the long snapshot; mkarchiso later
    # reads only this local mirror, even if the upstream repositories have changed.
    run_sudo pacman "${pacman_args[@]}" -Sp --print-format '%f' -- "${iso_packages[@]}" > "$RUN/package-files.txt" || {
        iso_die 'ISO package resolution failed. Update archiso (or rebuild the offline archive). No snapshot was taken.'; return 1;
    }
    if (( ISO_OFFLINE )); then
        local file missing=0
        while IFS= read -r file; do
            [[ $file != */* && $file == *.pkg.tar.* ]] || { iso_die 'Invalid package filename from database.'; return 1; }
            if [[ ! -s $RUN/pkg/$file ]]; then printf '[ERROR] Missing offline package: %s\n' "$file" >&2; missing=1; fi
        done < "$RUN/package-files.txt"
        (( ! missing )) || return 1
    fi
    run_sudo pacman "${pacman_args[@]}" -Sw -- "${iso_packages[@]}" || {
        iso_die 'ISO package download or signature verification failed. No snapshot was taken.'; return 1;
    }
    iso_local_mirror "$RUN/db" "$RUN/pkg" "$RUN/mirror" || return 1
    iso_write_pacman_conf "$PROFILE/pacman.conf" "$RUN/pkg" "$RUN/gnupg" "file://$RUN/mirror/\$repo/os/\$arch"
    echo '[*] Live ISO packages and dependencies are available locally.'
}
