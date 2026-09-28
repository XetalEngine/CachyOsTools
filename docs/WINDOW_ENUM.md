# Window Enum

Open **Dashboard → Window Enum**, immediately after **Sensors & Power**.
Each row represents one window exposed by the selected backend. Every statistic
has its own sortable column. Filter searches all columns; right-click a column
header to show/hide columns. Export writes the matching rows to JSON or CSV.
Live refresh runs every two seconds while the tab is visible. Selection and
pending geometry edits survive refreshes. A failed collector clears stale rows
and pauses Live; Refresh retries.

The table includes title, class/instance, application ID, executable path, PID
and its source, owner, command line, position, dimensions, frame geometry,
workspace, monitor, topmost/below, minimized/maximized/fullscreen, opacity,
parent/transient relationships, type, visibility, override-redirect/input-only,
state flags, protocols and available controls. A dash means unavailable; it
does not mean false or zero.

## Desktop coverage

| Session / backend | Discovery | Controls |
| --- | --- | --- |
| X11 on KDE, Xfce, MATE, GNOME and other EWMH window managers | Recursively walks every screen's full X window tree, including root, child, unmapped, input-only and override-redirect windows | Move, resize, raise/lower, focus, topmost/below, minimize/maximize, fullscreen, workspace, opacity, title and graceful close as supported |
| KDE Wayland, or explicit KDE backend | KWin workspace and stacking lists, including the popups, panels and overlays those APIs expose | Native move/resize, focus/raise, topmost/below, minimize/maximize, fullscreen, workspace, opacity, decorations and graceful close |
| GNOME Wayland, or explicit GNOME backend | Mutter window list through the included GNOME Shell extension; includes override-redirect windows where exposed | Native geometry, focus/raise/lower, topmost, minimize/maximize, fullscreen, workspace, actor opacity and graceful close as supported |
| Other Wayland compositors, including Xfce/MATE used with a compatible compositor | wlr-foreign-toplevel-management; falls back to ext-foreign-toplevel-list | wlr supports advertised state controls, activation and close; ext is read-only |
| XWayland under any Wayland desktop | Select **X11 / XWayland — full tree** | X11 controls for XWayland windows; this view does not include native Wayland windows |

Automatic uses the session type and desktop to select a backend. The backend
selector can override it for sessions using a different compositor.

There is no universal Wayland interface for inspecting and controlling every
surface. Generic foreign-toplevel protocols do not expose global geometry,
PID/executable, child surfaces, menus or layer-shell overlays. KWin/Mutter expose
more through their compositor APIs, but still do not expose every internal
surface. Separate backend views avoid duplicating one window under unrelated
compositor and X11 identifiers.

Toolkit or renderer does not filter enumeration: GTK, Qt, games, ImGui, OpenGL
and Vulkan windows are included when represented by discoverable windows.
An ImGui panel, in-game overlay or HUD rendered inside an existing window is
part of that window, with no independent window ID or geometry to change.
The window system also cannot reliably identify the graphics API used inside
an arbitrary window; no renderer is guessed from its title.

## Ownership and controls

X11 uses XRes to obtain the local client PID, including windows that omit
WM_CLASS or _NET_WM_PID. If XRes cannot provide a PID, the client-reported
_NET_WM_PID is labelled as such. Executable paths come from /proc/PID/exe.
Remote, terminated, inaccessible or differently namespaced processes can leave
the path unavailable. Application IDs and desktop-file launch commands are
not substituted for verified executable paths.

X11 X/Y and dimensions refer to the client area in X-server pixels, with
root-relative positions even for child windows. Separate frame columns show
decorations when provided. KDE/GNOME geometry uses compositor logical
coordinates for the window frame. Negative positions are supported.

**Topmost** is the window manager's keep-above flag, not a promise that nothing
can cover the window. Popups, fullscreen layers and compositor surfaces can
have separate stacking rules. Applications and compositor rules can constrain
geometry or reverse a requested state. The next refresh shows the actual result.
Changing a title or opacity is temporary and the application may overwrite it.
GNOME actor opacity can also change during Shell animations.

**Window actions** enables controls according to the selected window's
capabilities. Closing uses the normal window-close protocol, not process
termination. **All properties** includes raw X11 properties; individual large
properties are limited to 64 KiB and report remainingBytes when truncated.
Collectors have a timeout and run outside the UI process. No root access,
shell interpolation of window metadata, or changes to GNOME unsafe-mode
settings are needed.

## GNOME setup

On GNOME Shell 45–51, click **Enable GNOME Bridge**. This installs the bundled
extension for the current user and asks gnome-extensions to enable it.
If Shell has not discovered the new extension, log out and back in and enable
**CachyOsTools Window Enum** in GNOME Extensions. Then refresh.

The extension is installed under the user data directory at
gnome-shell/extensions/window-enum@cachyostools.xetal.net. It exports only
window listing and a fixed set of window actions. Disable or remove it in
GNOME Extensions when it is no longer needed. KDE uses temporary scripts
which are unloaded after each request and requires no persistent installation.

## Build and validation

Additional build packages:

- Arch/CachyOS/Manjaro: pkgconf, libxcb, wayland
- Debian/Ubuntu: pkg-config, libxcb1-dev, libxcb-res0-dev, libwayland-dev, libwayland-bin
- Fedora: pkgconf-pkg-config, libxcb-devel, wayland-devel

The two protocol XML definitions are vendored with their original licenses;
wayland-scanner generates the bindings during the build.

Build the optional tests with:

    cmake -S . -B build/window-enum -G Ninja -DBUILD_WINDOW_ENUM_TESTS=ON
    cmake --build build/window-enum --parallel 2
    ctest --test-dir build/window-enum --output-on-failure
    python3 -m unittest discover -s tests -v

The X11 test requires Xvfb and dbus-run-session. It creates disposable windows,
checks hidden/child/overlay/input-only enumeration and XRes ownership, performs
geometry/title/opacity changes, validates graceful close and checks the table.
Protocol-only Wayland fixtures exercise real client bindings for both foreign
toplevel protocols, including state changes, close, missing fields and stale
IDs. Node.js tests exercise the KWin and GNOME bridges against API fixtures.

An optional real nested KWin test checks native Wayland enumeration,
executable lookup, keep-above and geometry:

    python3 tests/run_window_enum_tests.py build/window-enum/window_enum_tests --kwin

Use --kwin-x11 to exercise EWMH geometry, topmost and minimize requests against
a real KWin instance on the disposable X server.

The nested test uses its own display, session bus, runtime and configuration
directories. It does not operate on the user's desktop.

Validated during development: full Qt application build; 52 Python regression
tests; isolated X11 tests and real nested KWin X11 window controls;
native nested KWin Wayland discovery and controls; both Wayland protocol
fixtures; JavaScript bridge tests. GNOME behavior is covered by API fixtures,
not a live GNOME session. Xfce/MATE window-manager policy and actual
game/overlay applications still require desktop-specific checks.

## API references

- [KWin scripting API](https://develop.kde.org/docs/plasma/kwin/api/)
- [Mutter window API](https://gnome.pages.gitlab.gnome.org/mutter/meta/class.Window.html)
- [EWMH window management](https://specifications.freedesktop.org/wm/latest/)
- [wlr foreign toplevel protocol](https://gitlab.freedesktop.org/wlroots/wlr-protocols/-/blob/master/unstable/wlr-foreign-toplevel-management-unstable-v1.xml)
- [ext foreign toplevel protocol](https://gitlab.freedesktop.org/wayland/wayland-protocols/-/blob/main/staging/ext-foreign-toplevel-list/ext-foreign-toplevel-list-v1.xml)
