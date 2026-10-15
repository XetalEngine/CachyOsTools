#!/usr/bin/env bash
set -Eeuo pipefail
# Partition modes adapted from devvz: https://github.com/XetalEngine/CachyOsTools/pull/7

installer_show_logo() {
    local title='XETAL ENGINE - System Installer'
    if [[ ! -t 1 || ${TERM:-dumb} == dumb ]]; then
        printf '%s\n\n' "$title"
        return 0
    fi

    local cols rows width height colors=16 available_colors art='' line pad index=0
    cols=$(tput cols 2>/dev/null) || cols=80
    rows=$(tput lines 2>/dev/null) || rows=24
    [[ $cols =~ ^[1-9][0-9]{0,3}$ ]] || cols=80
    [[ $rows =~ ^[1-9][0-9]{0,3}$ ]] || rows=24
    # Leave the last column unused to avoid wrapping at the terminal's edge.
    (( cols > 1 )) && cols=$((cols - 1))
    width=$cols
    (( width <= 110 )) || width=110
    height=$((rows / 3))
    (( height <= 9 )) || height=9
    (( height >= 1 )) || height=1
    available_colors=$(tput colors 2>/dev/null) || available_colors=16
    if [[ $available_colors =~ ^[0-9]{1,4}$ ]] && (( available_colors >= 256 )); then colors=240; fi

    printf '\033[0m\033[2J\033[H\n'
    if (( cols >= 40 && rows >= 12 )) && [[ -r /opt/clone/logo.png ]] && command -v chafa >/dev/null 2>&1; then
        # Use ordinary character cells: graphics overlays and terminal probes can
        # leave artifacts or stray input behind when dialog takes over the TTY.
        # Buffer the result so a renderer failure cannot leave half a logo onscreen.
        art=$(chafa --format symbols --symbols space,block --fg-only --colors "$colors" \
            --probe off --polite on --animate off --relative off --optimize 0 \
            --align center --view-size "${cols}x${rows}" --size "${width}x${height}" \
            /opt/clone/logo.png </dev/null 2>/dev/null) || art=''
    fi
    if [[ -n $art ]]; then
        printf '%s\033[0m\n' "$art"
    elif (( cols >= 65 && rows >= 12 )); then
        local -a shades=(31 31 33 32 32)
        pad=$(((cols - 65) / 2))
        while IFS= read -r line; do
            printf '%*s\033[1;%sm%s\033[0m\n' "$pad" '' "${shades[index]}" "$line"
            index=$((index + 1))
        done <<'XETAL_TEXT'
X   X EEEEE TTTTT  AAA  L      EEEEE N   N  GGGG  III N   N EEEEE
 X X  E       T   A   A L      E     NN  N G       I  NN  N E
  X   EEEE    T   AAAAA L      EEEE  N N N G  GG   I  N N N EEEE
 X X  E       T   A   A L      E     N  NN G   G   I  N  NN E
X   X EEEEE   T   A   A LLLLL  EEEEE N   N  GGGG  III N   N EEEEE
XETAL_TEXT
    else
        line='XETAL ENGINE'
        line=${line:0:cols}
        printf '%*s\033[1;32m%s\033[0m\n' "$(((cols - ${#line}) / 2))" '' "$line"
    fi
    if (( cols < ${#title} )); then title='System Installer'; fi
    title=${title:0:cols}
    printf '\n%*s%s\n\n' "$(((cols - ${#title}) / 2))" '' "$title"
}

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

installer_confirm_disk() {
    local disk=$1 back='XETAL ENGINE - System Installer' dinfo
    if command -v dialog >/dev/null 2>&1 && [[ -t 0 ]]; then
        dinfo=$(installer_device_description "$disk") || return 1
        dialog --backtitle "$back" --colors --defaultno --title " Confirm Target " \
            --yesno "\nInstall the cloned system to:\n\n    $disk  ($dinfo)\n\n\Z1This PERMANENTLY ERASES everything on that disk.\Zn\n\nContinue?" 14 66 \
            || { clear; echo 'Installation cancelled.'; return 1; }
        dialog --backtitle "$back" --colors --defaultno --title " FINAL WARNING " \
            --yesno "\n\Z1LAST CHANCE:\Zn wipe $disk and install the cloned system?" 9 56 \
            || { clear; echo 'Installation cancelled.'; return 1; }
        clear
    else
        printf '[ERROR] Open the installer on a terminal with dialog available. No disks were changed.\n' >&2
        return 1
    fi
}

# Pick one tag using buttons/arrow keys. Never accept a typed device or an
# unattended pipe as authorization to erase a disk.
installer_menu() {
    local title=$1 prompt=$2 choice='' i; shift 2
    local -a items=("$@")
    if command -v dialog >/dev/null 2>&1 && [[ -t 0 ]]; then
        choice=$(dialog --stdout --backtitle 'XETAL ENGINE - System Installer' --colors \
            --default-item "${items[0]}" --cancel-label 'Back' --title "$title" \
            --menu "$prompt" 20 78 8 "${items[@]}") || return 1
    else
        printf '[ERROR] The installer needs dialog and an interactive terminal.\n' >&2
        return 1
    fi
    for ((i = 0; i < ${#items[@]}; i += 2)); do
        if [[ ${items[i]} == "$choice" ]]; then printf '%s\n' "$choice"; return 0; fi
    done
    printf '[ERROR] Invalid selection.\n' >&2; return 1
}

installer_choose_mode() {
    local -a items=(wipe 'Restore entire hard drive (erase selected drive)'
        verify 'Check snapshot integrity (optional)')
    if [[ -d /sys/firmware/efi ]]; then
        items+=(free 'Use unused space - devvz (PR #7)'
            parts 'Use prepared partitions (advanced) - devvz (PR #7)')
    fi
    installer_menu ' XETAL ENGINE ' \
        "Choose an option. Snapshot check: ${SNAPSHOT_CHECK:-not checked}.\nThe integrity check is optional; restoring will not run it automatically." "${items[@]}"
}

# Disks have partition labels, rather than a single filesystem label. Display
# all their labels, and partition names where no filesystem label exists.
installer_device_description() {
    local device=$1 info labels
    info=$(lsblk -dno SIZE,MODEL "$device") || return 1
    labels=$(lsblk -no LABEL,PARTLABEL "$device" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//' |
        awk 'NF && !seen[$0]++ {if (n++) printf ", "; printf "%s", $0}') || return 1
    # Device metadata must not inject terminal control codes or dialog colors.
    printf 'Labels: %s | %s' "${labels:-none}" "$info" | LC_ALL=C tr -d '\000-\037\177' | sed 's/\\/ /g'
}

installer_check_snapshot() {
    local log status=0
    log=$(mktemp) || return 1
    dialog --backtitle 'XETAL ENGINE - System Installer' --title ' Snapshot integrity ' \
        --infobox 'Checking the complete snapshot...\n\nThis may take several minutes. No disks are being changed.' 9 70 || { rm -f "$log"; return 1; }
    if [[ ! -r /opt/clone/snapshot.sha256 || ! -r /opt/clone/rootfs-snapshot.tar.zst ]]; then
        printf 'The snapshot or checksum file is missing.\n' > "$log"
        status=1
    else
        (cd /opt/clone && sha256sum --check --strict snapshot.sha256) > "$log" 2>&1 || status=$?
    fi
    if (( status == 0 )); then
        SNAPSHOT_CHECK=passed
        dialog --backtitle 'XETAL ENGINE - System Installer' --title ' Snapshot integrity ' \
            --msgbox 'Snapshot integrity check passed.\n\nReturning to the installer menu.' 9 66 || true
    else
        SNAPSHOT_CHECK=failed
        dialog --backtitle 'XETAL ENGINE - System Installer' --title ' Snapshot integrity FAILED ' \
            --msgbox 'The snapshot could not be verified.\n\nInstallation is blocked until a check passes. Recreate the ISO or installation media if the problem persists.' 11 70 || true
    fi
    rm -f "$log"
}

installer_home() {
    local choice
    SNAPSHOT_CHECK='not checked'
    while true; do
        choice=$(installer_choose_mode) || return 1
        if [[ $choice == verify ]]; then
            installer_check_snapshot || return 1
        elif [[ ${SNAPSHOT_CHECK:-} == failed ]]; then
            dialog --backtitle 'XETAL ENGINE - System Installer' --msgbox \
                'The snapshot check failed. Run a successful integrity check before installing.' 8 68 || true
        else
            case $choice in wipe|free|parts) ;; *) printf '[ERROR] Invalid selection.\n' >&2; return 1 ;; esac
            INSTALL_MODE=$choice
            return 0
        fi
    done
}

# Free-space mode. Sets SEL_DISK, SEL_START and SEL_END (MiB, END exclusive).
installer_select_free() {
    local protected=$1 bytes=$2 disk start end n=0 idx
    local -a d_disk=() d_start=() d_end=() items=()
    SEL_DISK=''; SEL_START=''; SEL_END=''
    while read -r disk; do
        [[ -n $disk ]] || continue
        iso_validate_disk "$disk" "$protected" >/dev/null 2>&1 || continue
        [[ $(iso_gpt_table "$disk") == gpt ]] || continue
        while IFS=$'\t' read -r start end; do
            [[ -n $start ]] || continue
            d_disk+=("$disk"); d_start+=("$start"); d_end+=("$end")
            items+=("$n" "$disk: $(((end - start) / 1024)) GiB free; $(installer_device_description "$disk")")
            n=$((n + 1))
        done < <(iso_free_regions "$disk" "$bytes")
    done < <(lsblk -dpnro NAME,TYPE | awk '$2 == "disk" {print $1}')
    (( n )) || {
        iso_die "No GPT disk has a free region of at least $((bytes / 1024 / 1024 / 1024 + 1)) GiB. Shrink a partition first (for example in Windows Disk Management). No disk has been changed."
        return 1
    }
    if (( n == 1 )); then
        idx=0
    else
        idx=$(installer_menu 'Select free space' 'Install into which free region? Existing partitions are not modified.' "${items[@]}") || return 1
    fi
    SEL_DISK=${d_disk[idx]}; SEL_START=${d_start[idx]}; SEL_END=${d_end[idx]}
}

# Existing-partition mode. Sets SEL_ROOT, SEL_ESP and SEL_DISK (the root's disk).
installer_select_parts() {
    local protected=$1 root_need=$2 esp_need=$3 part size
    local -a roots=() esps=()
    SEL_DISK=''; SEL_ROOT=''; SEL_ESP=''
    while read -r part; do
        [[ -n $part ]] || continue
        iso_validate_partition "$part" "$protected" >/dev/null 2>&1 || continue
        size=$(lsblk -bdnro SIZE "$part")
        [[ $size =~ ^[0-9]+$ ]] || continue
        if (( size >= root_need )) && [[ $(lsblk -dnro PARTTYPE "$part") != c12a7328-f81f-11d2-ba4b-00a0c93ec93b ]]; then
            roots+=("$part" "$(installer_device_description "$part")")
        fi
        if (( size >= esp_need )) && iso_validate_esp_target "$part" >/dev/null 2>&1; then
            esps+=("$part" "$(installer_device_description "$part")")
        fi
    done < <(lsblk -pnro NAME,TYPE | awk '$2 == "part" {print $1}')
    (( ${#roots[@]} )) || {
        iso_die "No unused partition is large enough for the root filesystem (needs $((root_need / 1024 / 1024 / 1024 + 1)) GiB). No disk has been changed."
        return 1
    }
    SEL_ROOT=$(installer_menu 'Select ROOT partition (will be ERASED)' \
        'The cloned system is restored into this partition. Everything on it is lost.' "${roots[@]}") || return 1
    local -a esps_left=()
    local i
    for ((i = 0; i < ${#esps[@]}; i += 2)); do
        [[ ${esps[i]} == "$SEL_ROOT" ]] || esps_left+=("${esps[i]}" "${esps[i + 1]}")
    done
    (( ${#esps_left[@]} )) || {
        iso_die "No empty dedicated EFI system partition of at least $((esp_need / 1024 / 1024)) MiB. Existing boot partitions are protected. No disk has been changed."
        return 1
    }
    SEL_ESP=$(installer_menu 'Select EFI partition (will be FORMATTED)' \
        'Use an empty dedicated EFI system partition. It will be formatted as FAT32.' "${esps_left[@]}") || return 1
    SEL_ROOT=$(readlink -f -- "$SEL_ROOT"); SEL_ESP=$(readlink -f -- "$SEL_ESP")
    iso_validate_partition "$SEL_ROOT" "$protected" || return 1
    iso_validate_partition "$SEL_ESP" "$protected" || return 1
    iso_validate_esp_target "$SEL_ESP" || return 1
    SEL_DISK=$(iso_disk_ancestors "$SEL_ROOT" | head -n 1)
}

# What must stay identical between selection and the first write.
installer_target_identity() {
    local part parent
    case $1 in
        parts)
            for part in "$SEL_ROOT" "$SEL_ESP"; do
                lsblk -bdnro MAJ:MIN,PARTUUID,SIZE "$part" || return 1
                parent=$(iso_disk_ancestors "$part") || return 1
                lsblk -dnro MAJ:MIN,SERIAL,WWN "$parent" || return 1
            done ;;
        *) lsblk -dnro MAJ:MIN,SERIAL,WWN "$2" ;;
    esac
}

# Same two default-No warnings as the original full-drive installation.
installer_confirm_plan() {
    local summary=$1 danger=$2 back='XETAL ENGINE - System Installer'
    if command -v dialog >/dev/null 2>&1 && [[ -t 0 ]]; then
        dialog --backtitle "$back" --colors --cr-wrap --no-collapse --defaultno --title ' Confirm Plan ' \
            --yesno "$summary" 22 76 || { clear; echo 'Installation cancelled.'; return 1; }
        dialog --backtitle "$back" --colors --cr-wrap --no-collapse --defaultno --title ' FINAL WARNING ' \
            --yesno "$danger" 9 66 || { clear; echo 'Installation cancelled.'; return 1; }
        clear
    else
        printf '[ERROR] Open the installer on a terminal with dialog available. No disks were changed.\n' >&2
        return 1
    fi
}

installer_register_efi() {
    local esp=$1 parent number order index
    EFI_REGISTERED=0
    parent=$(iso_disk_ancestors "$esp") || return 1
    [[ -n $parent && $parent != *$'\n'* ]] || return 1
    read -r number < "/sys/class/block/${esp##*/}/partition" || return 1
    [[ $number =~ ^[1-9][0-9]*$ ]] || return 1
    order=$(efibootmgr) || return 1
    index=$(awk '/^BootOrder:/ {n=split($2, a, ",")} END {print n+0}' <<< "$order")
    # Append: retain all existing entries and their current boot priority.
    efibootmgr --create --index "$index" --disk "$parent" --part "$number" \
        --label 'Xetal Engine' --loader '\EFI\BOOT\BOOTX64.EFI' || return 1
    EFI_REGISTERED=1
}

installer_main() {
    installer_show_logo
    # Show the menu before hashing, firmware checks, or disk discovery.
    installer_home || return 1
    source /opt/clone/common.sh
    [[ $EUID == 0 ]] || { iso_die 'Run the installer as root from the live ISO.'; return 1; }
    local tool mode uefi=0 disk='' protected identity bytes capacity esp_mib root_uuid loader
    local key value source_bytes='' supports_uefi=0 supports_bios=0 kernel_count=1 format='' arch=''
    local install_mode=$INSTALL_MODE dry_run=0 plan='' esp_num root_num summary='' danger=''
    # --dry-run: choose and validate a target, print the plan, change nothing.
    [[ ${1:-} != --dry-run ]] || dry_run=1
    for tool in lsblk findmnt losetup swapon mountpoint parted partprobe udevadm mkfs.fat mkfs.ext4 \
        mount umount genfstab arch-chroot tar zstd sha256sum blkid file wipefs; do iso_need "$tool" 'live ISO installer' || return 1; done
    [[ -r /opt/clone/snapshot.meta && -r /opt/clone/rootfs-snapshot.tar.zst ]] || {
        iso_die 'This ISO has no readable snapshot and manifest. Rebuild it with a current creator.'; return 1;
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
                iso_die 'Disable Secure Boot before restoring from this live installer.'; return 1;
            }
        done
    else
        mode=i386-pc
        [[ $supports_bios == 1 ]] || { iso_die 'This snapshot requires UEFI. Reboot the USB in UEFI mode; no disk has been changed.'; return 1; }
    fi
    protected=$(installer_protected_disks) || return 1
    esp_mib=$((kernel_count * 512))
    (( esp_mib >= 2048 )) || esp_mib=2048
    # Optional override: XETAL_ESP_MIB=4096 bash installer.sh   (EFI partition size in MiB)
    if [[ -n ${XETAL_ESP_MIB:-} ]]; then
        [[ $XETAL_ESP_MIB =~ ^[0-9]{3,5}$ ]] && (( 10#$XETAL_ESP_MIB >= 512 && 10#$XETAL_ESP_MIB <= 16384 )) || {
            iso_die 'XETAL_ESP_MIB must be a whole number of MiB between 512 and 16384.'; return 1; }
        esp_mib=$((10#$XETAL_ESP_MIB))
    fi
    bytes=$((source_bytes + source_bytes / 5 + (esp_mib + 1024) * 1024 * 1024))
    [[ ${XETAL_SIGN_BOOT:-0} == 0 || ${XETAL_SIGN_BOOT:-0} == 1 ]] || {
        iso_die 'XETAL_SIGN_BOOT must be 0 or 1.'; return 1;
    }
    if iso_signing_enabled && (( ! uefi )); then
        iso_die 'Secure Boot signing requires a UEFI installation.'; return 1
    fi

    if [[ $install_mode != wipe ]] && (( ! uefi )); then
        iso_die 'Free-space and existing-partition installs need the USB booted in UEFI mode. No disk has been changed.'; return 1
    fi
    case $install_mode in
    wipe)
        local -a items=()
        while read -r value; do
            [[ -n $value ]] || continue
            if iso_validate_disk "$value" "$protected" >/dev/null 2>&1; then
                items+=("$value" "$(installer_device_description "$value")")
            fi
        done < <(lsblk -dpnro NAME,TYPE | awk '$2 == "disk" {print $1}')
        (( ${#items[@]} )) || { iso_die 'No unused writable target disks were found.'; return 1; }
        echo 'The target disk will be erased and recreated as unencrypted ext4 plus an EFI partition.'
        echo 'Source encryption, Btrfs snapshots, partition layout and bootloader configuration are not preserved.'
        disk=$(installer_menu 'Select disk to ERASE' \
            'Restore to a new unencrypted ext4 filesystem. All target data will be lost.' "${items[@]}") || return 1
        disk=$(readlink -f -- "$disk")
        iso_validate_disk "$disk" "$protected" || return 1
        capacity=$(lsblk -bdnro SIZE "$disk")
        [[ $capacity =~ ^[0-9]+$ ]] && (( capacity >= bytes )) || {
            iso_die "The target needs at least $((bytes / 1024 / 1024 / 1024 + 1)) GiB for this snapshot."; return 1;
        }
        summary="Whole-disk install to $disk. EVERYTHING on that disk will be erased."
        ;;
    free)
        echo 'The cloned system will be installed into free space; existing partitions are not modified.'
        installer_select_free "$protected" "$bytes" || return 1
        disk=$SEL_DISK
        summary="Install the cloned system into FREE SPACE:"$'\n\n'
        summary+="  Disk: $disk ($(installer_device_description "$disk"))"$'\n'
        summary+="  Free region:        $(((SEL_END - SEL_START) / 1024)) GiB, starting at $SEL_START MiB"$'\n'
        summary+="  New EFI partition:  $esp_mib MiB (FAT32)"$'\n'
        summary+="  New root partition: $(((SEL_END - SEL_START - esp_mib) / 1024)) GiB (ext4)"$'\n\n'
        summary+='Existing partitions (Windows etc.) are NOT modified. Only two new'$'\n'
        summary+='partition table entries are written, inside the free region above.'$'\n'
        summary+='Keep Secure Boot off while restoring.'
        danger="Create the two new partitions on $disk now and install the cloned system into them?"
        ;;
    parts)
        echo 'The two partitions you pick will be formatted; no other partition is touched.'
        installer_select_parts "$protected" "$((bytes - esp_mib * 1024 * 1024))" "$((esp_mib * 1024 * 1024))" || return 1
        disk=$SEL_DISK
        summary="Install the cloned system into EXISTING partitions:"$'\n\n'
        summary+="  ROOT (ext4):  $SEL_ROOT  ($(installer_device_description "$SEL_ROOT"))"$'\n'
        summary+="  EFI  (FAT32): $SEL_ESP  ($(installer_device_description "$SEL_ESP"))"$'\n\n'
        summary+='Both partitions will be FORMATTED: their contents are permanently erased.'$'\n'
        summary+='No other partition is modified. Keep Secure Boot off while restoring.'
        danger="LAST CHANCE: erase $SEL_ROOT and $SEL_ESP and install the cloned system?"
        ;;
    esac
    if (( dry_run )); then
        printf '\n[dry-run] Plan:\n%s\n\n[dry-run] No changes were made.\n' "$summary"
        return 0
    fi
    identity=$(installer_target_identity "$install_mode" "$disk") || return 1
    [[ -n $identity ]] || { iso_die 'Cannot identify the selected devices.'; return 1; }
    if [[ $install_mode == wipe ]]; then
        installer_confirm_disk "$disk" || return 1
    else
        installer_confirm_plan "$summary" "$danger" || return 1
    fi
    installer_show_logo
    # Revalidate after the interactive pause, immediately before destructive work.
    [[ $identity == "$(installer_target_identity "$install_mode" "$disk")" ]] || { iso_die 'The selected device changed.'; return 1; }
    if [[ $install_mode == parts ]]; then
        iso_validate_partition "$SEL_ROOT" "$protected" || return 1
        iso_validate_partition "$SEL_ESP" "$protected" || return 1
        iso_validate_esp_target "$SEL_ESP" || return 1
    else
        iso_validate_disk "$disk" "$protected" || return 1
    fi
    if [[ $install_mode == free ]]; then
        # The chosen region must still be free and large enough.
        local region_ok=0 rs re
        while IFS=$'\t' read -r rs re; do
            [[ -n $rs ]] || continue
            if (( rs <= SEL_START && re >= SEL_END )); then region_ok=1; fi
        done < <(iso_free_regions "$disk" "$bytes")
        (( region_ok )) || { iso_die 'The free region changed since it was selected. Nothing was written.'; return 1; }
    fi
    TARGET=$(mktemp -d /mnt/xetal-install-XXXXXX)
    trap installer_cleanup EXIT
    trap 'exit 130' INT
    trap 'exit 143' TERM
    exec > >(tee -a /var/log/xetal-installer.log) 2>&1

    local esp root_device
    case $install_mode in
    wipe)
        echo '[1/6] Partitioning the confirmed disk...'
        parted -s "$disk" mklabel gpt
        parted -s "$disk" mkpart BIOS 1MiB 3MiB
        parted -s "$disk" set 1 bios_grub on
        parted -s "$disk" mkpart ESP fat32 3MiB "$((esp_mib + 3))MiB"
        parted -s "$disk" set 2 esp on
        parted -s "$disk" mkpart ROOT ext4 "$((esp_mib + 3))MiB" 100%
        partprobe "$disk"
        udevadm settle --timeout=30
        esp=$(iso_partition_path "$disk" 2); root_device=$(iso_partition_path "$disk" 3)
        ;;
    free)
        echo '[1/6] Creating partitions in the selected free space...'
        plan=$(iso_create_efi_root "$disk" "$SEL_START" "$esp_mib" "$SEL_END") || return 1
        read -r esp_num root_num <<< "$plan"
        partprobe "$disk"
        udevadm settle --timeout=30
        esp=$(iso_partition_path "$disk" "$esp_num"); root_device=$(iso_partition_path "$disk" "$root_num")
        [[ -b $esp && -b $root_device ]] || { iso_die 'New partitions did not appear.'; return 1; }
        # Free space may hold leftovers of an old filesystem; clear stale signatures.
        wipefs -a -- "$esp" "$root_device"
        ;;
    parts)
        echo '[1/6] Using the selected existing partitions...'
        esp=$SEL_ESP; root_device=$SEL_ROOT
        wipefs -a -- "$esp" "$root_device"
        ;;
    esac
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
    local sign_boot=0 limine_graphics=''
    if iso_signing_enabled; then
        iso_sbctl_ready "$TARGET" || { iso_die 'Signing was requested but the snapshot has no usable sbctl keys/certificate.'; return 1; }
        sign_boot=1
    fi
    if [[ -f $TARGET/etc/xetal-boot.conf ]] && grep -qx 'LIMINE_GRAPHICS=no' "$TARGET/etc/xetal-boot.conf"; then
        limine_graphics='LIMINE_GRAPHICS=no'
    fi
    printf 'ROOT_UUID=%s\nBOOTLOADER=%s\nSIGN_BOOT=%s\n%s\n' "$root_uuid" "$loader" "$sign_boot" "$limine_graphics" > "$TARGET/etc/xetal-boot.conf"
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
Target = boot/*-ucode.img
Target = usr/share/limine/BOOTX64.EFI
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
    local sb_sign=$sign_boot sb_file
    if [[ $loader == grub ]]; then
        if (( uefi )); then
            local -a grub_sb=()
            # A shim-less GRUB needs these to boot signed kernels under Secure Boot.
            if (( sb_sign )); then grub_sb=(--modules=tpm --disable-shim-lock); fi
            # The removable path works without writable EFI NVRAM variables.
            arch-chroot "$TARGET" grub-install --target=x86_64-efi --efi-directory=/efi --bootloader-id=XetalClone --removable --no-nvram "${grub_sb[@]}"
            if (( sb_sign )); then
                arch-chroot "$TARGET" sbctl sign -s /efi/EFI/BOOT/BOOTX64.EFI
                arch-chroot "$TARGET" bash -c 'source /usr/local/lib/xetal-iso/common.sh; iso_verify_boot_file "$1"' xetal /efi/EFI/BOOT/BOOTX64.EFI
            fi
        else
            arch-chroot "$TARGET" grub-install --target=i386-pc "$disk"
        fi
    elif [[ $loader == limine ]]; then
        # xetal-update-boot above already deployed limine.conf, the kernels and the Limine
        # binary (config hash enrolled and signed when sbctl keys exist) on the EFI partition.
        echo '[i] Limine was deployed to the EFI partition (/EFI/BOOT/BOOTX64.EFI and limine.conf).'
    else
        if (( sb_sign )); then
            # bootctl prefers the .signed variant, so later bootctl updates stay signed too.
            arch-chroot "$TARGET" sbctl sign -s -o /usr/lib/systemd/boot/efi/systemd-bootx64.efi.signed \
                /usr/lib/systemd/boot/efi/systemd-bootx64.efi
            arch-chroot "$TARGET" bash -c 'source /usr/local/lib/xetal-iso/common.sh; iso_verify_boot_file "$1"' xetal /usr/lib/systemd/boot/efi/systemd-bootx64.efi.signed
        fi
        arch-chroot "$TARGET" bootctl --esp-path=/efi --no-variables install
        if (( sb_sign )); then
            # Belt and braces: make sure the copies on the EFI partition are signed.
            for sb_file in /efi/EFI/systemd/systemd-bootx64.efi /efi/EFI/BOOT/BOOTX64.EFI; do
                arch-chroot "$TARGET" sbctl sign -s "$sb_file"
                arch-chroot "$TARGET" bash -c 'source /usr/local/lib/xetal-iso/common.sh; iso_verify_boot_file "$1"' xetal "$sb_file"
            done
        fi
    fi
    if (( sb_sign )); then
        echo '[i] Secure Boot: boot-file signatures were verified using the requested sbctl keys.'
        echo '[i] Enable Secure Boot in firmware only after confirming the keys are enrolled (sbctl status).'
    fi
    if [[ $install_mode != wipe ]]; then
        installer_register_efi "$esp" || echo '[WARN] Firmware registration failed. The fallback bootloader is installed; select EFI/BOOT/BOOTX64.EFI on the new EFI partition in firmware.'
    fi
    sync
    umount -R "$TARGET"
    if [[ $install_mode != wipe ]]; then
        echo "[i] The restored system boots from its own EFI partition ($esp)."
        if [[ ${EFI_REGISTERED:-0} == 1 ]]; then
            echo '[i] Choose Xetal Engine in the firmware boot menu. Existing boot entries and their priority were preserved.'
        fi
        if (( ! sb_sign )); then echo '[i] Secure Boot has to remain disabled for this unsigned bootloader.'; fi
    fi
    echo '[6/6] Installation complete. Remove the USB and reboot when ready.'
}

if [[ ${BASH_SOURCE[0]} == "$0" ]]; then installer_main "$@"; fi
