#!/usr/bin/env bash
# Installed in the restored system and called after subsequent kernel updates.
set -Eeuo pipefail

restore_boot_main() {
    source /usr/local/lib/xetal-iso/common.sh
    [[ $EUID == 0 ]] || { iso_die 'Boot regeneration requires root.'; return 1; }
    source /etc/xetal-boot.conf
    [[ $ROOT_UUID =~ ^[a-fA-F0-9-]+$ ]] || { iso_die 'Invalid root UUID.'; return 1; }
    [[ $BOOTLOADER == grub || $BOOTLOADER == systemd-boot ]] || return 1
    local tool kernels version pkgbase kernel image microcode entry entry_tmp config_tmp
    local -a post_options=()
    tool=$(iso_initramfs_tool /) || return 1
    kernels=$(iso_kernels /) || return 1
    mkdir -p /boot/xetal
    if [[ $BOOTLOADER == systemd-boot ]]; then
        mountpoint -q /efi || { iso_die 'Mount the EFI system partition at /efi before updating boot files.'; return 1; }
        mkdir -p /efi/loader/entries /efi/Xetal
    fi

    # The restored root is a new, unencrypted ext4 filesystem. Do not embed the
    # source disk's encryption, LVM, resume UUIDs or Btrfs subvolume parameters.
    cat > /etc/mkinitcpio-xetal.conf <<'EOF'
MODULES=(ext4)
BINARIES=()
FILES=()
HOOKS=(base udev modconf keyboard block filesystems fsck)
COMPRESSION="gzip"
EOF
    mkdir -p /etc/dracut-xetal.conf.d
    printf 'hostonly="no"\nhostonly_cmdline="no"\n' > /etc/dracut-xetal.conf
    if [[ $tool == mkinitcpio ]] && mkinitcpio --help | grep -q -- --nopost; then post_options+=(--nopost); fi
    config_tmp=$(mktemp /boot/xetal/grub.cfg.XXXXXX)
    printf 'set default=0\nset timeout=5\n' > "$config_tmp"

    while IFS=$'\t' read -r version pkgbase kernel; do
        image="/boot/xetal/initramfs-$version.img"
        echo "[*] Generating $tool image for $pkgbase ($version)..."
        if [[ $tool == mkinitcpio ]]; then
            mkinitcpio -c /etc/mkinitcpio-xetal.conf -k "$version" -g "$image.new" "${post_options[@]}" || return 1
        else
            dracut --force --no-hostonly --no-hostonly-cmdline --conf /etc/dracut-xetal.conf \
                --confdir /etc/dracut-xetal.conf.d --add-drivers ext4 \
                --kernel-cmdline "root=UUID=$ROOT_UUID rw" "$image.new" "$version" || return 1
        fi
        [[ -s $image.new ]] || { iso_die "No initramfs was produced for $version."; return 1; }
        mv "$image.new" "$image"
        cp "$kernel" "/boot/xetal/vmlinuz-$version"
        {
            printf "menuentry 'Cloned system - %s' {\n" "$pkgbase"
            printf "  search --no-floppy --fs-uuid --set=root %s\n" "$ROOT_UUID"
            printf '  linux /boot/xetal/vmlinuz-%s root=UUID=%s rw\n' "$version" "$ROOT_UUID"
            printf '  initrd'
            for microcode in /boot/intel-ucode.img /boot/amd-ucode.img; do
                [[ ! -s $microcode ]] || printf ' %s' "$microcode"
            done
            printf ' /boot/xetal/initramfs-%s.img\n}\n' "$version"
        } >> "$config_tmp"
        if [[ $BOOTLOADER == systemd-boot ]]; then
            cp "/boot/xetal/vmlinuz-$version" "/efi/Xetal/vmlinuz-$version"
            cp "$image" "/efi/Xetal/initramfs-$version.img"
            entry="/efi/loader/entries/xetal-$version.conf"
            entry_tmp="$entry.new"
            {
                printf 'title Cloned system - %s\nlinux /Xetal/vmlinuz-%s\n' "$pkgbase" "$version"
                for microcode in /boot/intel-ucode.img /boot/amd-ucode.img; do
                    if [[ -s $microcode ]]; then
                        cp "$microcode" "/efi/Xetal/${microcode##*/}"
                        printf 'initrd /Xetal/%s\n' "${microcode##*/}"
                    fi
                done
                printf 'initrd /Xetal/initramfs-%s.img\noptions root=UUID=%s rw\n' "$version" "$ROOT_UUID"
            } > "$entry_tmp"
            mv "$entry_tmp" "$entry"
        fi
    done <<< "$kernels"
    if [[ $BOOTLOADER == grub ]]; then
        mkdir -p /boot/grub
        grub-script-check "$config_tmp" || return 1
        mv "$config_tmp" /boot/grub/grub.cfg
    else
        rm "$config_tmp"
        printf 'default xetal-*\ntimeout 5\n' > /efi/loader/loader.conf
        # Remove only our entries for kernels that no longer exist.
        for entry in /efi/loader/entries/xetal-*.conf; do
            [[ -f $entry ]] || continue
            version=${entry##*/xetal-}; version=${version%.conf}
            if [[ ! -d /usr/lib/modules/$version ]]; then
                rm -- "$entry" "/efi/Xetal/vmlinuz-$version" "/efi/Xetal/initramfs-$version.img"
            fi
        done
    fi
    echo '[*] Boot files regenerated successfully.'
}

if [[ ${BASH_SOURCE[0]} == "$0" ]]; then restore_boot_main "$@"; fi
