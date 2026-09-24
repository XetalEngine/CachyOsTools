#!/usr/bin/env bash
set -u

firstboot_ssh() {
    command -v ssh-keygen >/dev/null || { echo '[ssh] Install openssh to regenerate host keys.'; return 1; }
    mkdir -p /etc/xetal-source/ssh || return 1
    local file
    for file in /etc/ssh/ssh_host_*; do
        [[ -f $file ]] || continue
        cp -a "$file" /etc/xetal-source/ssh/ || return 1
        rm -- "$file" || return 1
    done
    ssh-keygen -A
}

firstboot_network() {
    command -v nmcli >/dev/null && nmcli general status >/dev/null 2>&1 || {
        echo "[net] NetworkManager is not active. Configure the source distribution's existing network service manually."
        return 1
    }
    local uuid type ifname uuids
    uuids=$(nmcli -t -f UUID connection show) || return 1
    while IFS= read -r uuid; do
        [[ -n $uuid ]] || continue
        type=$(nmcli -g connection.type connection show uuid "$uuid") || return 1
        [[ $type == 802-3-ethernet || $type == 802-11-wireless ]] || continue
        ifname=$(nmcli -g connection.interface-name connection show uuid "$uuid") || return 1
        [[ -n $ifname ]] || continue
        ip link show "$ifname" >/dev/null 2>&1 && continue
        # Retain bridges, VLANs, VPNs and addresses. Only remove a stale physical
        # interface binding; do not convert another distribution's network to NAT.
        nmcli connection modify uuid "$uuid" connection.interface-name '' || return 1
    done <<< "$uuids"
}

firstboot_gpu() {
    command -v lspci >/dev/null || { echo '[gpu] pciutils is needed for GPU detection.'; return 1; }
    local info module file
    info=$(lspci -nn | grep -Ei 'vga|3d controller|display') || return 1
    echo "[gpu] $info"
    if grep -qi nvidia <<< "$info"; then
        if ! modinfo nvidia >/dev/null 2>&1 && ! modinfo nouveau >/dev/null 2>&1; then
            echo "[gpu] Install a compatible NVIDIA driver with your distribution's supported tools."
            return 1
        fi
        return 0
    fi
    if grep -Eqi 'amd|ati|radeon' <<< "$info"; then module=amdgpu
    elif grep -qi intel <<< "$info"; then module=i915
    else echo '[gpu] No supported GPU adaptation was identified.'; return 1; fi
    modinfo "$module" >/dev/null 2>&1 || {
        [[ $module == i915 ]] && modinfo xe >/dev/null 2>&1 || return 1
    }
    pacman -Q mesa >/dev/null 2>&1 || {
        echo '[gpu] Mesa is missing. Perform a full distribution update and install its graphics packages, then retry.'
        return 1
    }
    mkdir -p /etc/xetal-source/xorg || return 1
    for file in /etc/X11/xorg.conf /etc/X11/xorg.conf.d/*.conf; do
        [[ -f $file ]] || continue
        if grep -Eqi 'Driver[[:space:]]+"nvidia"' "$file"; then
            cp -a "$file" "/etc/xetal-source/xorg/${file##*/}" || return 1
            mv "$file" "$file.xetal-disabled" || return 1
        fi
    done
    if [[ -f /etc/environment ]]; then
        cp -an /etc/environment /etc/xetal-source/environment || return 1
        sed -i -E '/^(GBM_BACKEND=nvidia|__GLX_VENDOR_LIBRARY_NAME=nvidia)/s/^/# xetal: /' /etc/environment || return 1
    fi
    echo '[gpu] Existing drivers configured. No repositories or package versions were changed.'
}

firstboot_user() {
    local user newuser group
    user=$(awk -F: '$3>=1000 && $3<60000 && $1!="nobody" {print $1; exit}' /etc/passwd)
    [[ -n $user ]] || return 1
    read -rp "New username (Enter keeps $user): " newuser || return 1
    if [[ -n $newuser && $newuser != "$user" ]]; then
        [[ $newuser =~ ^[a-z_][a-z0-9_-]*$ ]] || { echo '[user] Invalid username.'; return 1; }
        ! id "$newuser" >/dev/null 2>&1 || { echo '[user] That account already exists.'; return 1; }
        group=$(id -gn "$user") || return 1
        usermod --login "$newuser" --home "/home/$newuser" --move-home "$user" || return 1
        if [[ $group == "$user" ]]; then groupmod --new-name "$newuser" "$group" || return 1; fi
        user=$newuser
        echo '[user] Review any application paths or autologin settings that referred to the old username.'
    fi
    passwd "$user"
}

firstboot_main() {
    [[ -f /etc/xetal-firstboot.conf ]] || return 0
    source /etc/xetal-firstboot.conf
    exec > >(tee -a /var/log/xetal-firstboot.log) 2>&1
    local step flag failed=0
    mkdir -p /var/lib/xetal-firstboot/done || return 1
    for step in ssh network gpu user; do
        case "$step" in
            ssh) flag=${REGEN_SSH:-0} ;; network) flag=${FIX_NETWORK:-0} ;;
            gpu) flag=${FIX_GPU:-0} ;; user) flag=${CHANGE_USER:-0} ;;
        esac
        [[ $flag == 1 && ! -f /var/lib/xetal-firstboot/done/$step ]] || continue
        if "firstboot_$step"; then
            touch "/var/lib/xetal-firstboot/done/$step" || failed=1
        else
            failed=1
        fi
    done
    if (( failed )); then
        echo '[ERROR] Some adaptation steps need attention. See /var/log/xetal-firstboot.log; completed steps will not repeat.'
        return 1
    fi
    mv /etc/xetal-firstboot.conf /var/lib/xetal-firstboot/completed.conf || return 1
    systemctl disable xetal-firstboot.service
}

if [[ ${BASH_SOURCE[0]} == "$0" ]]; then firstboot_main "$@"; fi
