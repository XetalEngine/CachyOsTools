# CachyOsTools interface upgrade

CachyOsTools remains a separate application. It keeps its original tabs, system
actions, KDE tools, KVM guides, preferences and Qt implementation. The new widgets
and helpers are bundled here; running Hyper Fusion or installing its profile is
not required.

## Existing tools

- Every top-level page has a heading, concise guidance, consistent controls and
  palette-aware cards. Long pages scroll inside the window instead of forcing it
  beyond the screen. Tab labels remain readable with scroll arrows.
- Tweaks keeps its existing controls and connections, adds category search and
  arranges simple action groups into wrapping cards. Complex forms remain intact.
- Network presents adapters, Wi-Fi and saved connections as cards, with separate
  virtual-network, diagnostics, firewall and sharing sections. Firewall loading
  still requires the existing explicit action. Wi-Fi background refresh uses
  cached results; Scan requests a new scan.
- Drives groups disks and partitions, keeps selection by device across refreshes,
  and checks mounted descendants even when a filter hides them. Existing filters
  are preserved. New profiles show disks and partitions together.
- Cleaner retains its cleanup catalogue and size tree. The additional Disk
  Visualizer scans on request, supports navigation and a treemap, and confirms
  permanent deletion. It checks identities to reject files replaced after a scan.
- Theme offers six visual palette cards alongside the existing system-default and
  custom color controls. Themes also supply palette colors to custom-painted
  widgets and dialogs.
- Failed or replaced drive commands retire their process objects after callbacks
  finish, avoiding a crash when a required utility is unavailable.

## GRUB Editor

The new tab contains **Current GRUB**, **Add Entry** and **Appearance**. It uses an
existing GRUB installation; it does not install GRUB or switch bootloaders.

Read the current menu, inspect a drive, detect another installed system, and
review an entry before adding it. Appearance includes six CachyOsTools color
presets, a live preview, custom pictures, text size, placement and menu shading.
The application's theme and the boot screen design can be chosen independently.

Changes requiring administrator access use PolicyKit. The helper validates the
menu, detects stale state, saves backups and recovery instructions, and supports
undo. Appearance presents the backup review before final confirmation. Existing
entries, default selection and countdown are preserved by appearance changes.

The editor owns `/var/lib/cachyostools/grub-editor` and
`/var/lib/cachyostools/grub-appearance`, with generators under `/etc/grub.d` named
`42_cachyostools` and `99_cachyostools_appearance`. These are separate from Hyper
Fusion's files. Backups belong to the machine on which a change was made.

Runtime requirements for these tools on Arch/CachyOS:

```bash
sudo pacman -S python python-pillow grub polkit
```

The Inter fonts and their OFL license are included in `assets/fonts/`. Helpers
find resources beside an installed executable under `share/cachyostools`, or in
the source checkout when running a development build.

## Wayland

[Window Enum](WINDOW_ENUM.md) documents the backend-specific coverage. Hyprland
now has its own collector for windows and desktop layers, including untitled
surfaces. Optional inspection extensions add information when available.
Unsupported fields remain unknown; controls are enabled only for supported
operations. Both legacy and Lua dispatcher syntax are handled.

The X11, KWin, GNOME and generic Wayland collectors remain available. KDE's
restart-compositor action also detects Wayland when the session-type variable is
missing, avoiding an X11 restart attempt in that session.

## Build and verification

```bash
cmake -S . -B build/modern -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_MODERN_UI_TESTS=ON -DBUILD_PREFERENCES_TESTS=ON \
  -DBUILD_DRIVE_LABEL_TESTS=ON
cmake --build build/modern -j4
ctest --test-dir build/modern --output-on-failure
python3 -m unittest discover -s tests -p 'test_window_enum_bridges.py'
./build/modern/CachyOsTools
```

Release builds passed with Qt 6 and Qt 5. The interface tests use Qt 6.
Qt 5 compatibility includes treemap mouse coordinates, typed network-card
metadata and the existing VBIOS byte reader.

The tests cover drive hierarchy/filters and action targets, preferences,
network controls, disk scanning/deletion, GRUB validation/backup/undo and preview
workflows, and Hyprland collection/action fixtures. The full-app smoke test uses
isolated settings and an empty system-command search path: it switches all 17
top-level tabs across three palettes, checks a 1100×720 window, searches each
Tweaks category, and checks missing-command handling. Set
`CACHY_UI_SCREENSHOTS=/tmp/cachy-ui` when running that test to save renderings.

Nested X11 and Wayland protocol-server tests can be enabled with
`-DBUILD_WINDOW_ENUM_TESTS=ON`. This workspace blocks display-server socket
creation, so those two integration tests could not run here. The KWin/GNOME
bridge fixtures and native Hyprland fixtures run without a display server.
Actual GRUB boot/reboot and live compositor interaction still need checks in
appropriate disposable machines or desktop sessions; no real boot configuration
was changed during this work.

For installation, CMake includes the binary, Python helpers and font resources:

```bash
cmake --install build/modern --prefix "$HOME/.local"
```

Copying only the executable to another machine will omit the GRUB and disk
helper resources. The repository's `build.sh` can still copy the binary to the
checkout root, where its `tools/` and `assets/` directories are available.
