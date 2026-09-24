#!/usr/bin/env bash
set -Eeuo pipefail

installer_cleanup() {
    local status=$?
    trap - EXIT
    if [[ -n ${TARGET:-} ]] && mountpoint -q "$TARGET"; then
        if ! umount -R -- "$TARGET"; then
            printf '[ERROR] Could not unmount %s; do not remove the disk yet.\n' "$TARGET" >&2
            status=1
        fi
    fi
    if (( status != 0 )); then
        printf '[ERROR] Installation did not complete. See /var/log/xetal-installer.log.\n' >&2
    fi
    exit "$status"
}

installer_protected_disks() {
    local media source disks
    media=$(findmnt -nro SOURCE -M /run/archiso/bootmnt) || {
        iso_die 'Cannot identify the installation medium. Boot from a directly attached ISO/USB without copy-to-RAM.'; return 1;
    }
    disks=$(iso_disk_ancestors "$media") || return 1
    # Optical media have no disk ancestor and are safe because only whole disks
    # are offered. All other media must have identifiable backing devices.
    if [[ -z $disks && $(lsblk -dnro TYPE "$media") != rom ]]; then
        iso_die 'Cannot identify the physical disk containing the ISO; refusing to erase a disk.'; return 1
    fi
    printf '%s\n' "$disks"
    source=$(findmnt -nro SOURCE -T /) || return 1
    iso_disk_ancestors "$source"
}

installer_reset_firstboot() {
    local target=$1
    # A clone of a previously restored system may contain old adaptation state.
    # Each installation must honor this ISO's selections and start fresh.
    if [[ -f $target/etc/xetal-firstboot.conf ]]; then
        cp -a "$target/etc/xetal-firstboot.conf" "$target/etc/xetal-source/firstboot.conf" || return 1
        rm -- "$target/etc/xetal-firstboot.conf" || return 1
    fi
    rm -f -- "$target/etc/systemd/system/multi-user.target.wants/xetal-firstboot.service" || return 1
    rm -rf -- "$target/var/lib/xetal-firstboot/done"
}

installer_main() {
    source /opt/clone/common.sh
    [[ $EUID == 0 ]] || { iso_die 'Run the installer as root from the live ISO.'; return 1; }
    local tool mode uefi=0 disk='' protected identity confirm bytes capacity esp_mib root_uuid loader
    local key value source_bytes='' supports_uefi=0 supports_bios=0 kernel_count=1 format='' arch=''
    for tool in lsblk findmnt losetup swapon mountpoint parted partprobe udevadm mkfs.fat mkfs.ext4 \
        mount umount genfstab arch-chroot tar zstd sha256sum blkid file; do iso_need "$tool" 'live ISO installer' || return 1; done
    [[ -r /opt/clone/snapshot.meta && -r /opt/clone/snapshot.sha256 ]] || {
        iso_die 'This ISO has no verified snapshot manifest. Rebuild it with a current creator.'; return 1;
    }
    while IFS='=' read -r key value; do
        case "$key" in
            FORMAT) format=$value ;; ARCH) arch=$value ;; BYTES) source_bytes=$value ;;
            UEFI) supports_uefi=$value ;; BIOS) supports_bios=$value ;; KERNELS) kernel_count=$value ;;
        esac
    done < /opt/clone/snapshot.meta
    [[ $format == 1 && $arch == x86_64 && $(uname -m) == x86_64 &&
       $source_bytes =~ ^[0-9]{1,16}$ && $kernel_count =~ ^[1-9][0-9]?$ ]] || {
        iso_die 'Unsupported or invalid snapshot manifest.'; return 1;
    }
    if [[ -d /sys/firmware/efi ]]; then
        uefi=1; mode=x86_64-efi
        [[ $supports_uefi == 1 ]] || { iso_die 'The source lacks a UEFI bootloader. Rebuild after installing GRUB or systemd-boot.'; return 1; }
        for value in /sys/firmware/efi/efivars/SecureBoot-*; do
            [[ -f $value ]] || continue
            [[ $(od -An -j4 -N1 -tu1 "$value" | tr -d ' ') != 1 ]] || {
                iso_die 'Disable Secure Boot before restoring: this installer creates unsigned boot files.'; return 1;
            }
        done
    else
        mode=i386-pc
        [[ $supports_bios == 1 ]] || { iso_die 'This snapshot requires UEFI. Reboot the USB in UEFI mode; no disk has been changed.'; return 1; }
    fi
    protected=$(installer_protected_disks) || return 1
    echo '[*] Verifying the complete snapshot before selecting a target...'
    (cd /opt/clone && sha256sum --check --strict snapshot.sha256) || return 1
    esp_mib=$((kernel_count * 512))
    (( esp_mib >= 2048 )) || esp_mib=2048
    bytes=$((source_bytes + source_bytes / 5 + (esp_mib + 1024) * 1024 * 1024))

    local -a items=()
    while read -r value; do
        [[ -n $value ]] || continue
        if iso_validate_disk "$value" "$protected" >/dev/null 2>&1; then
            items+=("$value" "$(lsblk -dnro SIZE,MODEL "$value")")
        fi
    done < <(lsblk -dpnro NAME,TYPE | awk '$2 == "disk" {print $1}')
    (( ${#items[@]} )) || { iso_die 'No unused writable target disks were found.'; return 1; }
    echo 'XETAL ENGINE — restore a system clone'
    echo 'The target disk will be erased and recreated as unencrypted ext4 plus an EFI partition.'
    echo 'Source encryption, Btrfs snapshots, partition layout and bootloader configuration are not preserved.'
    if command -v dialog >/dev/null 2>&1 && [[ -t 0 ]]; then
        disk=$(dialog --stdout --title 'Select disk to ERASE' --menu \
            'Restore to a new unencrypted ext4 filesystem. All target data will be lost.' 18 78 8 "${items[@]}") || return 1
    else
        printf '%s\n' "${items[@]}"
        read -rp 'Whole target disk (for example /dev/sda): ' disk
    fi
    disk=$(readlink -f -- "$disk")
    iso_validate_disk "$disk" "$protected" || return 1
    capacity=$(lsblk -bdnro SIZE "$disk")
    [[ $capacity =~ ^[0-9]+$ ]] && (( capacity >= bytes )) || {
        iso_die "The target needs at least $((bytes / 1024 / 1024 / 1024 + 1)) GiB for this snapshot."; return 1;
    }
    identity=$(lsblk -dnro MAJ:MIN,SERIAL,WWN "$disk")
    printf '\nERASE %s (%s) and install the clone?\n' "$disk" "$(lsblk -dnro SIZE,MODEL "$disk")"
    read -rp "Type exactly 'ERASE $disk' to confirm: " confirm
    [[ $confirm == "ERASE $disk" ]] || { echo 'Installation cancelled.'; return 1; }
    # Revalidate after the interactive pause, immediately before destructive work.
    [[ $identity == "$(lsblk -dnro MAJ:MIN,SERIAL,WWN "$disk")" ]] || { iso_die 'The selected device changed.'; return 1; }
    iso_validate_disk "$disk" "$protected" || return 1
    TARGET=$(mktemp -d /mnt/xetal-install-XXXXXX)
    trap installer_cleanup EXIT
    trap 'exit 130' INT
    trap 'exit 143' TERM
    exec > >(tee -a /var/log/xetal-installer.log) 2>&1

    echo '[1/6] Partitioning the confirmed disk...'
    parted -s "$disk" mklabel gpt
    parted -s "$disk" mkpart BIOS 1MiB 3MiB
    parted -s "$disk" set 1 bios_grub on
    parted -s "$disk" mkpart ESP fat32 3MiB "$((esp_mib + 3))MiB"
    parted -s "$disk" set 2 esp on
    parted -s "$disk" mkpart ROOT ext4 "$((esp_mib + 3))MiB" 100%
    partprobe "$disk"
    udevadm settle --timeout=30
    local esp root_device
    esp=$(iso_partition_path "$disk" 2); root_device=$(iso_partition_path "$disk" 3)
    [[ -b $esp && -b $root_device ]] || { iso_die 'New partitions did not appear.'; return 1; }
    echo '[2/6] Formatting...'
    mkfs.fat -F32 "$esp"
    mkfs.ext4 -F "$root_device"
    echo '[3/6] Mounting the target...'
    mount "$root_device" "$TARGET"
    echo '[4/6] Restoring snapshot...'
    tar --xattrs --xattrs-include='*' --acls --numeric-owner --sparse -I zstd \
        -xpf /opt/clone/rootfs-snapshot.tar.zst -C "$TARGET"
    loader=$(iso_bootloader "$TARGET" "$mode") || return 1
    iso_kernels "$TARGET" >/dev/null || return 1
    iso_initramfs_tool "$TARGET" >/dev/null || return 1
    mkdir -p "$TARGET/etc/xetal-source"
    for value in fstab crypttab crypttab.initramfs; do
        if [[ -e $TARGET/etc/$value ]]; then cp -a "$TARGET/etc/$value" "$TARGET/etc/xetal-source/"; fi
    done
    # These mappings reference the source's disks and would block the new boot.
    printf '# Source mappings saved in /etc/xetal-source/crypttab\n' > "$TARGET/etc/crypttab"
    printf '# Source mappings saved in /etc/xetal-source/crypttab.initramfs\n' > "$TARGET/etc/crypttab.initramfs"
    if [[ -L $TARGET/efi ]]; then mv "$TARGET/efi" "$TARGET/etc/xetal-source/efi-link"; fi
    mkdir -p "$TARGET/efi"
    mount "$esp" "$TARGET/efi"
    genfstab -U "$TARGET" > "$TARGET/etc/fstab"
    root_uuid=$(blkid -s UUID -o value "$root_device")
    [[ $root_uuid =~ ^[a-fA-F0-9-]+$ ]] || return 1
    printf 'ROOT_UUID=%s\nBOOTLOADER=%s\n' "$root_uuid" "$loader" > "$TARGET/etc/xetal-boot.conf"
    install -Dm644 /opt/clone/common.sh "$TARGET/usr/local/lib/xetal-iso/common.sh"
    install -Dm755 /opt/clone/restore-boot.sh "$TARGET/usr/local/sbin/xetal-update-boot"
    mkdir -p "$TARGET/etc/pacman.d/hooks"
    cat > "$TARGET/etc/pacman.d/hooks/zz-xetal-boot.hook" <<'HOOK'
[Trigger]
Operation = Install
Operation = Upgrade
Operation = Remove
Type = Path
Target = usr/lib/modules/*/vmlinuz
Target = usr/lib/modules/*/pkgbase
Target = boot/vmlinuz-*
[Action]
Description = Updating the restored system's boot files...
When = PostTransaction
Exec = /usr/local/sbin/xetal-update-boot
HOOK
    installer_reset_firstboot "$TARGET"
    if [[ -f /opt/clone/firstboot.conf ]]; then
        install -Dm755 /opt/clone/firstboot.sh "$TARGET/usr/local/bin/xetal-firstboot.sh"
        install -Dm644 /opt/clone/firstboot.conf "$TARGET/etc/xetal-firstboot.conf"
        install -Dm644 /opt/clone/firstboot.service "$TARGET/etc/systemd/system/xetal-firstboot.service"
        if grep -qx 'REGEN_MACHINE_ID=1' /opt/clone/firstboot.conf; then
            rm -f "$TARGET/etc/machine-id" "$TARGET/var/lib/dbus/machine-id"
            arch-chroot "$TARGET" systemd-machine-id-setup
            rm -f "$TARGET/var/lib/libvirt/secrets/secrets-encryption-key"
        fi
        arch-chroot "$TARGET" systemctl enable xetal-firstboot.service
    fi
    rm -f "$TARGET/var/lib/systemd/random-seed"
    echo '[5/6] Regenerating boot files for the restored system...'
    arch-chroot "$TARGET" /usr/local/sbin/xetal-update-boot
    if [[ $loader == grub ]]; then
        if (( uefi )); then
            # The removable path works without writable EFI NVRAM variables.
            arch-chroot "$TARGET" grub-install --target=x86_64-efi --efi-directory=/efi --bootloader-id=XetalClone --removable --no-nvram
        else
            arch-chroot "$TARGET" grub-install --target=i386-pc "$disk"
        fi
    else
        arch-chroot "$TARGET" bootctl --esp-path=/efi --no-variables install
    fi
    sync
    umount -R "$TARGET"
    echo '[6/6] Installation complete. Remove the USB and reboot when ready.'
}

if [[ ${BASH_SOURCE[0]} == "$0" ]]; then installer_main "$@"; fi
