#!/usr/bin/env bash
# Shared by the builder, installer and regression tests. Sourcing has no effects.

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

iso_bootloader() {
    local root=$1 mode=$2
    if [[ -x $root/usr/bin/grub-install && -d $root/usr/lib/grub/$mode ]]; then
        printf 'grub\n'
    elif [[ $mode == x86_64-efi && -x $root/usr/bin/bootctl &&
            -s $root/usr/lib/systemd/boot/efi/systemd-bootx64.efi ]]; then
        printf 'systemd-boot\n'
    else
        iso_die "This snapshot has no supported $mode bootloader. Install GRUB in the source, or use UEFI with systemd-boot available."
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
