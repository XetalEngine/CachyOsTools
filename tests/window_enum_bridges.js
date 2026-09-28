const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const root = path.resolve(__dirname, '..');

const window = {
    internalId: 'uuid-1', caption: 'Game "Ω" $(touch NEVER)', resourceClass: 'GameClass',
    resourceName: 'game', desktopFileName: 'test.game', pid: 42,
    frameGeometry: {x: -50, y: 20, width: 640, height: 480}, managed: true,
    keepAbove: false, keepBelow: false, opacity: 1, minimized: false, fullScreen: false,
    moveable: true, resizeable: true, minimizable: true, maximizable: true, closeable: true,
    fullScreenable: true, normalWindow: true, desktops: [{x11DesktopNumber: 1, name: 'One'}],
    setMaximize(x, y) { this.maximizeMode = x && y ? 3 : 0; },
    closeWindow() { this.closed = true; },
};
let response;
const kde = vm.createContext({
    workspace: {windowList: () => [window], stackingOrder: [window],
                desktops: [{x11DesktopNumber: 1, name: 'One'}]},
    callDBus: (...args) => { response = JSON.parse(args[5]); },
});
vm.runInContext(fs.readFileSync(path.join(root, 'window_enum/kwin.js'), 'utf8'), kde);
function kwin(request) { kde.windowEnumKWin(request); return response; }
const snapshot = kwin({action: 'list'});
assert.equal(snapshot.windows.length, 1, 'window and stacking lists must not duplicate rows');
assert.equal(snapshot.windows[0].title, window.caption);
assert.equal(snapshot.windows[0].x, -50);
assert.equal(snapshot.windows[0].pid, 42);
assert(kwin({action: 'above', id: 'uuid-1', value: true}).ok);
assert(window.keepAbove);
assert(kwin({action: 'geometry', id: 'uuid-1', x: 10, y: 30, width: 400, height: 300}).ok);
assert.equal(window.frameGeometry.width, 400);
window.resizeable = false;
assert(kwin({action: 'geometry', id: 'uuid-1', x: 55, y: 40, width: 1, height: 1, resize: false}).ok);
assert.equal(window.frameGeometry.x, 55);
assert.equal(window.frameGeometry.width, 400);
assert(kwin({action: 'geometry', id: 'uuid-1', x: 0, y: 0, width: 1, height: 1}).error);
assert(kwin({action: 'close', id: 'gone'}).error);
assert(kwin({action: 'desktop', id: 'uuid-1', value: 8}).error);
assert(kwin({action: 'title', id: 'uuid-1', value: 'unsupported'}).error);

const actor = {opacity: 255};
const native = {
    get_stable_sequence: () => 101, get_frame_rect: () => ({x: 0, y: 0, width: 640, height: 480}),
    get_compositor_private: () => actor, get_workspace: () => ({index: () => 0}), get_transient_for: () => null,
    allows_move: () => true, allows_resize: () => true, can_minimize: () => true,
    can_maximize: () => true, can_close: () => true, get_window_type: () => 0,
    get_title: () => 'GNOME game', get_wm_class: () => 'Game', get_wm_class_instance: () => 'game',
    get_gtk_application_id: () => 'test.game', get_pid: () => 99, is_remote: () => false,
    get_role: () => 'normal', is_above: () => false, is_fullscreen: () => false,
    has_focus: () => true, is_hidden: () => false, is_override_redirect: () => false,
    is_skip_taskbar: () => false, get_monitor: () => 0, is_on_all_workspaces: () => false,
    make_above() { this.above = true; }, move_resize_frame(...args) { this.geometry = args; },
    move_frame(...args) { this.position = args; }, delete() { this.closed = true; },
};
const gnome = vm.createContext({
    Extension: class {}, Gio: {}, GLib: {uuid_string_random: () => 'session-1'},
    Meta: {WindowType: {NORMAL: 0}, MaximizeFlags: {BOTH: 3}},
    global: {display: {list_all_windows: () => [native]}, workspace_manager: {n_workspaces: 2},
             get_current_time: () => 1},
});
const source = fs.readFileSync(path.join(root, 'window_enum/gnome/extension.js'), 'utf8')
    .replace(/^import .+;$/gm, '').replace('export default class', 'globalThis.Bridge = class');
vm.runInContext(source, gnome);
const bridge = new gnome.Bridge();
assert.equal(JSON.parse(bridge.List()).windows[0].title, 'GNOME game');
const act = request => JSON.parse(bridge.Act(JSON.stringify({id: 'session-1:101', ...request})));
assert(act({action: 'above', value: true}).ok);
assert(native.above);
assert(act({action: 'opacity', value: 30}).ok);
assert.equal(actor.opacity, 77);
assert(act({action: 'geometry', x: 20, y: 30, width: 600, height: 450}).ok);
assert.equal(native.geometry[3], 600);
assert(act({action: 'geometry', x: 80, y: 90, width: 600, height: 450, resize: false}).ok);
assert.equal(native.position[1], 80);
assert(act({action: 'geometry', x: 0, y: 0, width: -1, height: 2}).error);
assert(act({action: 'above', value: 'true'}).error);
assert(act({action: 'opacity', value: 999}).error);
assert(act({action: 'desktop', value: 999}).error);
assert(act({action: 'close', id: 'closed-window'}).error);
assert(act({action: 'close', id: 'previous-session:101'}).error);
assert(act({action: 'kill'}).error);
console.log('KWin and GNOME bridge behavior checks passed.');
