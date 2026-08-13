<div align="center">

<img src="images/XetalEngine.png" alt="XETAL ENGINE" width="480">

# CachyOsTools

### The control panel Arch never had.

**Clone your entire system to a bootable ISO. Manage kernels, users, devices, sensors and firewalls like it's one app — because it is. Fix pacman when it breaks. Set up GPU passthrough without praying. In three languages.**

<br>

![Qt](https://img.shields.io/badge/Qt_6-41CD52?style=for-the-badge&logo=qt&logoColor=white)
![C++17](https://img.shields.io/badge/C%2B%2B17-1d1f24?style=for-the-badge&logo=cplusplus&logoColor=27ae60)
![Arch](https://img.shields.io/badge/Arch--based-1793D1?style=for-the-badge&logo=archlinux&logoColor=white)
![CachyOS](https://img.shields.io/badge/CachyOS-tested-27ae60?style=for-the-badge)
![GPLv3](https://img.shields.io/badge/GPLv3-bd0000?style=for-the-badge&logo=gnu&logoColor=white)

**🇺🇸 English &nbsp;·&nbsp; 🇧🇷 Português &nbsp;·&nbsp; 🇩🇪 Deutsch**

<br>

[⚡ Quick Start](#-quick-start) &nbsp;•&nbsp;
[🎯 Why This Exists](#-why-this-exists) &nbsp;•&nbsp;
[🧰 The Arsenal](#-the-arsenal) &nbsp;•&nbsp;
[📸 Gallery](#-gallery) &nbsp;•&nbsp;
[🤝 Contributing](#-contributing)

<br>

<img src="images/dash-welcome.png" alt="CachyOsTools Dashboard" width="920">

<sub>*Your system at a glance — drive health, services, updates, and your latest safety ISO. Flag dropdown included.*</sub>

</div>

<br>

---

## 🆕 Fresh in this build

|   |   |   |
|---|---|---|
| 🌡️ | **Sensors & Power** | Every hwmon reading, live — coloured against each chip's *own* limits, NVIDIA included. Plus the knobs you reach for when one goes red |
| 🩺 | **Pacman Doctor** | Diagnoses stale mirrors, a broken keyring, a stuck `db.lck`, pending `.pacnew`s — and fixes each one. Downgrade included |
| 📁 | **Network Shares** | Finds the NAS on your LAN, mounts it, and shares your folders back — without a password ever touching a dialog |
| 🚀 | **Autostart** | msconfig for Arch. XDG entries *and* systemd user units in one list; add anything with a file picker |

---

## 🎯 Why This Exists

Windows users get a Control Panel. Mac users get System Settings. Arch users get... 47 terminal commands and a wiki tab that never closes.

**CachyOsTools closes that gap** — one native Qt app that does the things you actually do to your system, with the safety rails you wish the terminal had:

|   |   |
|---|---|
| 💿 | **Clone your running system into a bootable installer ISO** — your insurance policy against dead drives, with an auto-launching TUI installer on the other end |
| 🧬 | **See every kernel that can boot your machine** — including your TKG source builds and that stale one from 2020 — install, remove, and pick the GRUB default without touching a config |
| 🔌 | **A real Device Manager** — every PCI/USB device, its driver, its modules. Load, unload, blacklist |
| 🖥️ | **GPU passthrough for humans** — complete Single-GPU and Dual-GPU guides plus step-by-step VFIO setup with per-step backups |
| 📦 | **Packages without borders** — official repos, AUR, and yes, **.deb files** converted into native pacman packages |
| 🩺 | **When pacman breaks, it tells you why** — stale mirrors, a dead keyring, a leftover lock, unmerged `.pacnew`s. One click each to fix, and a downgrade button for the update that shouldn't have shipped |
| 🛡️ | **Guardrails everywhere** — it refuses to delete your running kernel, your own account, or format a mounted drive. It knows better, because we made those mistakes for you |

---

## ⚡ Quick Start

```bash
git clone https://github.com/XetalEngine/CachyOsTools.git
cd CachyOsTools
chmod +x build.sh
./build.sh          # picks compiler, build system, and offers ISO-creator deps
./CachyOsTools
```

That's it. The interactive build script handles the choices; if the **System ISO Creator** needs `archiso`, `rsync`, `tar` or `zstd`, it offers to install them for you.

<details>
<summary><b>Manual build &amp; requirements</b></summary>

<br>

**Build requirements:** Qt6 (or Qt5), CMake 3.16+, GCC or Clang with C++17

```bash
sudo pacman -S qt6-base qt6-tools cmake gcc make    # build
sudo pacman -S udisks2                              # runtime (drive tools)
sudo pacman -S archiso rsync tar zstd               # ISO creator
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

**Non-interactive ISO deps (CI):** `./build.sh --install-iso-deps`

**OS:** CachyOS (tested), Manjaro, EndeavourOS, and other Arch-based distributions.

</details>

---

## 🧰 The Arsenal

### 💿 The Crown Jewel — System ISO Creator

<table>
<tr>
<td width="55%">

Your system, **exactly as it is right now**, burned into a bootable ISO with a guided installer.

- **Exclusion panels** — drop huge folders and keep app logins (Discord, browsers) *out* of shareable ISOs
- **Exact clone** or **first-boot hardware adaptation** — network, GPU drivers, user rename, fresh SSH identity
- **XETAL ENGINE TUI installer** launches automatically on boot: pick a disk, confirm, done
- Multithreaded compression tuned for speed — a full system in one coffee

*The Dashboard tracks your latest safety ISO and nags you when it gets old. Listen to it.*

</td>
<td width="45%">
<img src="images/tab-system-iso.png" alt="System ISO Creator">
</td>
</tr>
</table>

### 🎛️ The Control Panel — Windows nostalgia, Arch power

<table>
<tr>
<td width="45%">
<img src="images/dash-kernels.png" alt="Kernel Manager">
</td>
<td width="55%">

Eight tools that used to be eight terminal sessions, living as Dashboard sub-tabs:

- **🔌 Devices** — PCI/USB tree, drivers in use, module info / load / unload / blacklist
- **🌡️ Sensors & Power** — every hwmon reading live, coloured against each chip's *own* limits (NVIDIA included), plus the switches you reach for when one goes red: power profile, CPU governor, energy preference, battery charge limit that survives a reboot, and the fancontrol service
- **👥 Users & Groups** — accounts with avatars, group membership editor (wheel, libvirt, docker...), password & shell changes
- **🧬 Kernels** — scans `/boot` itself so *custom and source-built kernels show up too*; one-click install of 9 popular kernels including **linux-tkg built from source**; GRUB default picker
- **🪪 System Properties** — hostname, machine-id, hardware identity — the sysdm.cpl you missed
- **⏪ Restore Points** — snapper/btrfs snapshots: list, create, delete, and honest rollback guidance
- **🌱 Environment** — the two-pane variables dialog, for `/etc/environment` and your user session

</td>
</tr>
</table>

### 🌡️ Sensors & Power — the panel that watches back

<table>
<tr>
<td width="55%">

`lm_sensors` in a terminal tells you a number. This tells you whether that number is **a problem** — every reading is coloured against the limits *that chip reports for itself*, not a hard-coded guess.

- **Live, and honest about it** — refreshes every 2s, but only while you're looking at it. No background polling, no `du` storms
- **NVIDIA too** — lm_sensors can't see the proprietary stack, so the GPU is read straight from the driver
- **No lm_sensors? Still works** — falls back to raw `/sys/class/hwmon`, and offers to run `sensors-detect`
- **The knobs, right there** — power profile, CPU governor, energy preference, and a battery charge limit that installs a systemd unit so it survives reboots
- **Fan control without the footgun** — it launches `pwmconfig` and manages the service, and warns you *why* your fans stop during the probe

</td>
<td width="45%">
<img src="images/dash-sensors.png" alt="Sensors and Power">
</td>
</tr>
</table>

### 🖥️ KVM & GPU Passthrough — the full cockpit

<table>
<tr>
<td width="55%">

Three sub-tabs, from "read this first" to "click and done":

- **🖥️ Dual GPU** — illustrated guide (iGPU + dGPU setups, Looking Glass, the identical-GPU trap) sitting on top of the classic 7-step VFIO workflow — every step with its own **Backup** button
- **🎯 Single GPU** — the complete single-GPU passthrough manual: BIOS, IOMMU for Intel *and* AMD, hook scripts, vBIOS patching, CPU pinning, troubleshooting table
- **⚡ 1-Click KVM** — full automation (QEMU/EDK2 source builds, auto vBIOS dump, generated hooks) — *under construction*

</td>
<td width="45%">
<img src="images/kvm-single-gpu.png" alt="Single GPU Passthrough Guide">
</td>
</tr>
</table>

### 📦 Packages Without Borders

<table>
<tr>
<td width="45%">
<img src="images/tab-pkg-uninstall.png" alt="PKG Uninstall">
</td>
<td width="55%">

- **PKG Install** — official repos + AUR search, Yay/Paru management, update checker with **Arch news feed** (read it before big upgrades!)
- **🩺 Repair** — the pacman doctor. One click diagnoses stale mirrors, a broken keyring, a leftover `db.lck`, pending `.pacnew` files and a cache that ate your disk — then fixes each one. Includes **downgrade a package** from your cache (with optional `IgnorePkg` hold), because the fastest way out of a bad update is backwards
- **📦 Install .deb** — Debian packages converted via debtap into *native, pacman-tracked* packages. Full conversion visible in the terminal
- **PKG Uninstall** — everything sorted by size with install dates, orphan cleanup, cache cleaner. Find the 1.2 GB you forgot about

</td>
</tr>
</table>

<div align="center">
<img src="images/tab-pkg-repair.png" alt="Pacman Doctor" width="920">
<br>
<sub><i>Ten checks, ten dots. Each one turns green the moment the fix actually lands — even if you ran it in your own terminal.</i></sub>
</div>

<br>

### 📁 Shares & 🚀 Autostart — the two applets Arch forgot

<table>
<tr>
<td width="50%">
<img src="images/net-shares.png" alt="Network Shares">
<sub><b>Network Shares</b> — mDNS + NetBIOS discovery finds the NAS, double-click a share to mount it. Boot-time mounts get <code>x-systemd.automount</code> and a root-only credentials file; the password is typed into the terminal, never into a dialog. It also shares <i>your</i> folders over Samba — and flatly refuses to export <code>/</code>, <code>/home</code>, <code>/etc</code> or your <code>.ssh</code>.</sub>
</td>
<td width="50%">
<img src="images/tab-autostart.png" alt="Autostart">
<sub><b>Autostart</b> — your <code>~/.config/autostart</code> entries, the ones packages dropped in <code>/etc/xdg/autostart</code>, and your enabled systemd user units, in <b>one list</b>. Add anything with a file picker: it sets the execute bit, warns about a missing shebang, and can delay the start. Disabling a package's entry writes a personal <code>Hidden=true</code> override instead of touching <code>/etc</code>.</sub>
</td>
</tr>
</table>

### 🚀 And the daily drivers

| | |
|---|---|
| **💾 Drives** | Mount, format, SMART health checks, take-ownership, **burn ISO → USB** with triple-checked device confirmation |
| **🌐 Network** | Interfaces, bridges (with one-click *fix bridge on new hardware*), libvirt networks, Wi-Fi, saved connections, open ports — and the **🛡️ Firewall manager** (ufw/firewalld rules) right next to them |
| **📁 Shares** | Mount SMB/NFS from anything on your LAN, and export your own folders over Samba — *detailed above* |
| **🔧 Tweaks** | ZRAM, CPU governor, gaming optimizations (`vm.max_map_count`), mitigations, TRIM, swappiness, I/O schedulers, pacman candy — each with Info, Apply, Backup, and Edit Config |
| **⚙️ Services** | Start/stop/enable, failed-service banner, per-service journal, boot-time analysis — with **🚀 Autostart** as its second sub-tab |
| **📋 Logs** | journald with boot selector, live follow mode, hardware reports |
| **📄 System Config** | Curated config file list with a **built-in sudo-capable editor** — because Kate can't and nano won't hold your hand |
| **🎨 Theme** | XETAL presets or your own accent/background — the whole app reskins live |
| **🌍 Languages** | Full GUI in English, Português and Deutsch — flag selector on the Welcome page |

---

## 📸 Gallery

<div align="center">
<img src="images/dash-devices.png" alt="Device Manager" width="920">
<sub><i>The Device Manager Arch never shipped — every device, every driver, every module.</i></sub>
</div>

<br>

<details>
<summary><b>🎛️ Control Panel</b> — Sensors &amp; Power · Users &amp; Groups · Kernels · System Properties · Restore Points · Environment</summary>
<br>
<img src="images/dash-sensors.png" alt="Sensors and Power" width="900">
<img src="images/dash-users.png" alt="Users and Groups" width="900">
<img src="images/dash-kernels.png" alt="Kernel Manager" width="900">
<img src="images/dash-sysprops.png" alt="System Properties" width="900">
<img src="images/dash-restore.png" alt="Restore Points" width="900">
<img src="images/dash-environment.png" alt="Environment Variables" width="900">
</details>

<details>
<summary><b>💿 System ISO &amp; Storage</b> — ISO Creator · Drives</summary>
<br>
<img src="images/tab-system-iso.png" alt="System ISO" width="900">
<img src="images/tab-drives.png" alt="Drives" width="900">
</details>

<details>
<summary><b>🖥️ KVM</b> — Dual GPU · Single GPU · 1-Click</summary>
<br>
<img src="images/kvm-dual-gpu.png" alt="KVM Dual GPU" width="900">
<img src="images/kvm-single-gpu.png" alt="KVM Single GPU" width="900">
<img src="images/kvm-1click.png" alt="KVM 1-Click" width="900">
</details>

<details>
<summary><b>📦 Packages</b> — Install · Repair · Uninstall</summary>
<br>
<img src="images/tab-pkg-install.png" alt="PKG Install" width="900">
<img src="images/tab-pkg-repair.png" alt="Pacman Doctor" width="900">
<img src="images/tab-pkg-uninstall.png" alt="PKG Uninstall" width="900">
</details>

<details>
<summary><b>🌐 Network</b> — Interfaces · Firewall · Shares</summary>
<br>
<img src="images/tab-network.png" alt="Network" width="900">
<img src="images/net-firewall.png" alt="Firewall" width="900">
<img src="images/net-shares.png" alt="Network Shares" width="900">
</details>

<details>
<summary><b>🔧 System</b> — Tweaks · Services · Autostart · Logs · Config · Shell · Theme</summary>
<br>
<img src="images/tab-tweaks.png" alt="Tweaks" width="900">
<img src="images/tab-services.png" alt="Services" width="900">
<img src="images/tab-autostart.png" alt="Autostart" width="900">
<img src="images/tab-logs.png" alt="Logs" width="900">
<img src="images/tab-system-config.png" alt="System Config" width="900">
<img src="images/tab-shell.png" alt="Shell" width="900">
<img src="images/tab-theme.png" alt="Theme" width="900">
</details>

---

## 🛟 Design Philosophy

1. **Add, don't refactor.** Working features never get compromised for new ones.
2. **Show, don't hide.** Every privileged operation runs in a visible terminal — you see exactly what's executed, always.
3. **Backup before touch.** Config-changing features create timestamped backups first.
4. **Refuse the footgun.** Delete your running kernel? Your own account? Burn an ISO over your system disk? The app says no so you don't have to say oops.
5. **Never lie about the current state.** A panel that shows a stale reading is worse than one that shows nothing. The diagnostics watch the files they report on, so a fix turns the dot green the moment it lands — even when you ran that fix yourself, in your own terminal, with this app just sitting there.

---

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch — `git checkout -b feature/amazing`
3. Commit — `git commit -m 'Add amazing feature'`
4. Push and open a Pull Request

Found a bug? Want a tool added to the control panel? **Issues are welcome.**

---

## 📄 License

**GPLv3** — see [LICENSE](LICENSE). Use it, fork it, improve it; keep it open.

<br>

<div align="center">

**If CachyOsTools saved your system (or just your afternoon) — ⭐ star the repo. It helps more than you think.**

<br>

Made with ❤️ for the CachyOS and Arch community &nbsp;·&nbsp; from the maker of [LINUX STUDIO 2026](https://www.xetal.net)

<img src="images/XetalEngine.png" alt="XETAL ENGINE" width="200">

</div>
