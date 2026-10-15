#!/usr/bin/env bash
# Installed in the restored system and called after subsequent kernel updates.
# Limine support adapted from devvz: https://github.com/XetalEngine/CachyOsTools/pull/7
set -Eeuo pipefail

restore_boot_main() {
    source /usr/local/lib/xetal-iso/common.sh
    [[ $EUID == 0 ]] || { iso_die 'Boot regeneration requires root.'; return 1; }
    source /etc/xetal-boot.conf
    [[ $ROOT_UUID =~ ^[a-fA-F0-9-]+$ ]] || { iso_die 'Invalid root UUID.'; return 1; }
    [[ $BOOTLOADER == grub || $BOOTLOADER == systemd-boot || $BOOTLOADER == limine ]] || return 1
    local tool kernels version pkgbase kernel image microcode entry entry_tmp config_tmp
    tool=$(iso_initramfs_tool /) || return 1
    kernels=$(iso_kernels /) || return 1
    if iso_signing_enabled; then
        iso_sbctl_ready || { iso_die 'Requested signing keys/certificate are unavailable.'; return 1; }
    fi
    mkdir -p /boot/xetal
    exec 9>"/boot/xetal/.update.lock"
    flock -n 9 || { iso_die 'Another boot-file update is running.'; return 1; }
    if [[ $BOOTLOADER == systemd-boot || $BOOTLOADER == limine ]]; then
        mountpoint -q /efi || { iso_die 'Mount the EFI system partition at /efi before updating boot files.'; return 1; }
        mkdir -p /efi/Xetal
        if [[ $BOOTLOADER == systemd-boot ]]; then mkdir -p /efi/loader/entries; fi
    fi

    if [[ $BOOTLOADER == limine ]]; then
        restore_boot_limine_main "$tool" "$kernels"
        return $?
    fi

    config_tmp=$(mktemp /boot/xetal/grub.cfg.XXXXXX)
    printf 'set default=0\nset timeout=5\n' > "$config_tmp"

    while IFS=$'\t' read -r version pkgbase kernel; do
        image="/boot/xetal/initramfs-$version.img"
        echo "[*] Generating $tool image for $pkgbase ($version)..."
        restore_boot_make_image "$tool" "$version" "$image.new" || return 1
        [[ -s $image.new ]] || { iso_die "No initramfs was produced for $version."; return 1; }
        mv "$image.new" "$image"
        cp "$kernel" "/boot/xetal/vmlinuz-$version.new"
        iso_sign_boot_file "/boot/xetal/vmlinuz-$version.new" || return 1
        mv "/boot/xetal/vmlinuz-$version.new" "/boot/xetal/vmlinuz-$version"
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

restore_boot_gpu_modules() {
    local version=$1 m out=''
    while read -r m; do
        m=${m%\?}
        case $m in
            nvidia|nvidia_modeset|nvidia_uvm|nvidia_drm|amdgpu|radeon|i915|xe|nouveau|virtio_gpu|vmwgfx|qxl|bochs)
                [[ " $out " != *" $m "* ]] || continue
                if modinfo -k "$version" "$m" >/dev/null 2>&1; then
                    out+="${out:+ }$m"
                else
                    printf '[WARN] Skipping unavailable optional GPU module %s for %s.\n' "$m" "$version" >&2
                fi ;;
        esac
    done < <(bash -c 'MODULES=(); for f in "$@"; do [[ -r $f ]] && . "$f"; done; printf "%s\n" "${MODULES[@]}"' \
        _ /etc/mkinitcpio.conf /etc/mkinitcpio.conf.d/*.conf 2>/dev/null)
    printf '%s' "$out"
}

restore_boot_make_image() {
    local tool=$1 version=$2 image=$3 early_modules
    local -a post_options=()
    early_modules=$(restore_boot_gpu_modules "$version") || return 1
    # The clone is ext4. Source encryption, LVM, resume and Btrfs hooks must not
    # leak into its initramfs. Optional GPU drivers are checked for EACH kernel.
    {
        printf 'MODULES=(ext4%s)\n' "${early_modules:+ $early_modules}"
        printf 'BINARIES=()\nFILES=()\nHOOKS=(base udev modconf keyboard block filesystems fsck)\nCOMPRESSION="gzip"\n'
    } > /etc/mkinitcpio-xetal.conf || return 1
    mkdir -p /etc/dracut-xetal.conf.d || return 1
    printf 'hostonly="no"\nhostonly_cmdline="no"\n' > /etc/dracut-xetal.conf || return 1
    if [[ $tool == mkinitcpio ]]; then
        if mkinitcpio --help | grep -q -- --nopost; then post_options+=(--nopost); fi
        mkinitcpio -c /etc/mkinitcpio-xetal.conf -k "$version" -g "$image" "${post_options[@]}"
    else
        dracut --force --no-hostonly --no-hostonly-cmdline --conf /etc/dracut-xetal.conf \
            --confdir /etc/dracut-xetal.conf.d --add-drivers "ext4${early_modules:+ $early_modules}" \
            --kernel-cmdline "root=UUID=$ROOT_UUID rw" "$image" "$version"
    fi
}

restore_boot_hash() {
    local digest
    (( ${limine_hash:-0} )) || return 0
    digest=$(b2sum -- "$1") || return 1
    printf '#%s' "${digest%% *}"
}

restore_boot_limine_path() {
    local key=$1 file=$2 prefix=$3 hash
    hash=$(restore_boot_hash "$file") || return 1
    printf '    %s: boot():%s/%s%s\n' "$key" "$prefix" "${file##*/}" "$hash"
}

# Runs in the Limine updater's subshell. Until BOTH live files are installed,
# any failure/signal restores the old pair; files referenced by it stay intact.
restore_boot_limine_cleanup() {
    local status=$? rollback_ok=1
    trap - EXIT
    if (( activating && ! committed )); then
        if (( had_conf )); then cp -f "$stage/old.conf" "$conf" || rollback_ok=0; else rm -f "$conf" || rollback_ok=0; fi
        if (( had_efi )); then cp -f "$stage/old.efi" "$efi" || rollback_ok=0; else rm -f "$efi" || rollback_ok=0; fi
        if (( ! rollback_ok )); then
            printf '[ERROR] Boot rollback needs recovery. Backups retained in %s.\n' "$stage" >&2
            exit 1
        fi
    fi
    rm -f "$conf.new" "$efi.new"
    if (( ! committed )) && [[ -n $generation ]]; then rm -rf -- "$generation"; fi
    [[ -z $stage ]] || rm -rf -- "$stage"
    exit "$status"
}

restore_boot_limine_main() (
    local tool=$1 kernels=$2 version pkgbase kernel image microcode windows_guid hash prefix
    local stage='' generation='' old_generation old_conf='' entry_n=0 default_n=0 limine_hash=0
    local conf=/efi/EFI/BOOT/limine.conf efi=/efi/EFI/BOOT/BOOTX64.EFI
    local activating=0 committed=0 had_conf=0 had_efi=0
    mkdir -p /efi/EFI/BOOT /efi/Xetal || return 1
    trap restore_boot_limine_cleanup EXIT
    trap 'exit 130' INT
    trap 'exit 143' TERM
    stage=$(mktemp -d /boot/xetal/.limine-XXXXXXXX) || return 1
    generation=$(mktemp -d /efi/Xetal/generation-XXXXXXXX) || return 1
    printf 'Xetal boot generation\n' > "$generation/.xetal-generation" || return 1
    prefix="/Xetal/${generation##*/}"
    if iso_signing_enabled; then limine_hash=1; fi
    printf 'timeout: 5\n' > "$generation/limine.conf" || return 1
    if [[ ${LIMINE_GRAPHICS:-} == no ]]; then printf 'graphics: no\n' >> "$generation/limine.conf" || return 1; fi

    while IFS=$'\t' read -r version pkgbase kernel; do
        image="$stage/initramfs-$version.img"
        echo "[*] Generating $tool image for $pkgbase ($version)..."
        restore_boot_make_image "$tool" "$version" "$image" || return 1
        [[ -s $image ]] || { iso_die "No initramfs was produced for $version."; return 1; }
        mv "$image" "$generation/initramfs-$version.img" || return 1
        cp "$kernel" "$generation/vmlinuz-$version" || return 1
        iso_sign_boot_file "$generation/vmlinuz-$version" || return 1
        entry_n=$((entry_n + 1))
        if (( default_n == 0 )) && [[ $pkgbase != *lts* ]]; then default_n=$entry_n; fi
        {
            printf '\n/Cloned system - %s\n    protocol: linux\n' "$pkgbase" || return 1
            restore_boot_limine_path path "$generation/vmlinuz-$version" "$prefix" || return 1
            for microcode in /boot/intel-ucode.img /boot/amd-ucode.img; do
                [[ -s $microcode ]] || continue
                cp "$microcode" "$generation/${microcode##*/}" || return 1
                restore_boot_limine_path module_path "$generation/${microcode##*/}" "$prefix" || return 1
            done
            restore_boot_limine_path module_path "$generation/initramfs-$version.img" "$prefix" || return 1
            printf '    cmdline: root=UUID=%s rw\n' "$ROOT_UUID" || return 1
        } >> "$generation/limine.conf" || return 1
    done <<< "$kernels"
    windows_guid=$(iso_windows_esp_guid) || return 1
    if [[ -n $windows_guid ]]; then
        printf '\n/Windows\n    protocol: efi\n    path: guid(%s):/EFI/Microsoft/Boot/bootmgfw.efi\n' \
            "$windows_guid" >> "$generation/limine.conf" || return 1
    fi
    if (( default_n > 1 )); then sed -i "1a default_entry: $default_n" "$generation/limine.conf" || return 1; fi
    [[ -s /usr/share/limine/BOOTX64.EFI ]] || { iso_die 'The limine package files are missing.'; return 1; }
    install -m644 /usr/share/limine/BOOTX64.EFI "$generation/BOOTX64.EFI" || return 1
    if (( limine_hash )); then
        hash=$(b2sum -- "$generation/limine.conf") || return 1
        limine enroll-config "$generation/BOOTX64.EFI" "${hash%% *}" || {
            iso_die 'Could not enroll the Limine config hash. Previous boot files were preserved.'; return 1;
        }
        iso_sign_boot_file "$generation/BOOTX64.EFI" || return 1
    fi
    if [[ -f $conf ]]; then
        cp "$conf" "$stage/old.conf" || return 1
        old_conf=$(cat "$conf") || return 1
        had_conf=1
    fi
    if [[ -f $efi ]]; then cp "$efi" "$stage/old.efi" || return 1; had_efi=1; fi
    # A separately bootable previous pair also provides recovery after a power
    # interruption between the two FAT renames. Never remove its generation.
    if (( had_conf && had_efi )); then
        mkdir -p /efi/EFI/XetalPrevious || return 1
        cp "$stage/old.conf" /efi/EFI/XetalPrevious/limine.conf || return 1
        cp "$stage/old.efi" /efi/EFI/XetalPrevious/BOOTX64.EFI || return 1
    fi
    cp "$generation/limine.conf" "$conf.new" || return 1
    cp "$generation/BOOTX64.EFI" "$efi.new" || return 1
    activating=1
    mv "$conf.new" "$conf" || return 1
    mv "$efi.new" "$efi" || return 1
    sync -f "$efi" || return 1
    committed=1
    # Keep current + previous generations. Only remove directories owned by us.
    for old_generation in /efi/Xetal/generation-*; do
        [[ -d $old_generation && ! -L $old_generation && -f $old_generation/.xetal-generation ]] || continue
        [[ $old_generation != "$generation" && $old_conf != *"/Xetal/${old_generation##*/}/"* ]] || continue
        rm -rf -- "$old_generation" || printf '[WARN] Could not remove old boot generation %s.\n' "$old_generation" >&2
    done
    echo '[*] Boot files regenerated successfully.'
)

if [[ ${BASH_SOURCE[0]} == "$0" ]]; then restore_boot_main "$@"; fi
