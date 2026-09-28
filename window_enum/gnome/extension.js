import Gio from 'gi://Gio';
import GLib from 'gi://GLib';
import Meta from 'gi://Meta';
import {Extension} from 'resource:///org/gnome/shell/extensions/extension.js';

const XML = '<node><interface name="org.xetal.CachyOsTools.WindowEnum.Gnome">' +
    '<method name="List"><arg type="s" direction="out"/></method>' +
    '<method name="Act"><arg type="s" direction="in"/><arg type="s" direction="out"/></method>' +
    '</interface></node>';
const SESSION = GLib.uuid_string_random();
const windowId = window => SESSION + ':' + window.get_stable_sequence();

function allWindows() {
    if (typeof global.display.list_all_windows === 'function')
        return global.display.list_all_windows();
    const windows = global.display.get_tab_list(Meta.TabList.NORMAL_ALL, null);
    for (const actor of global.get_window_actors()) {
        const window = actor.meta_window;
        if (window && !windows.includes(window)) windows.push(window);
    }
    return windows;
}

function describe(window) {
    const frame = window.get_frame_rect();
    const actor = window.get_compositor_private();
    const workspace = window.get_workspace();
    const parent = window.get_transient_for();
    const caps = ['activate', 'raise', 'lower', 'above', 'desktop'];
    if (window.allows_move()) caps.push('move');
    if (window.allows_resize()) caps.push('resize');
    if (window.can_minimize()) caps.push('minimized');
    if (window.can_maximize()) caps.push('maximized', 'fullscreen');
    if (window.can_close()) caps.push('close');
    if (actor) caps.push('opacity');
    const type = window.get_window_type();
    return {
        id: windowId(window), backend: 'GNOME',
        title: window.get_title(), class: window.get_wm_class(), instance: window.get_wm_class_instance(),
        appId: window.get_gtk_application_id() || window.get_sandboxed_app_id(),
        pid: window.get_pid(), pidSource: 'Mutter', remote: window.is_remote(),
        x: frame.x, y: frame.y, width: frame.width, height: frame.height,
        frameX: frame.x, frameY: frame.y, frameWidth: frame.width, frameHeight: frame.height,
        geometryUnits: 'Compositor logical coordinates; window frame',
        type: Object.keys(Meta.WindowType).find(key => Meta.WindowType[key] === type) || type,
        role: window.get_role(), above: window.is_above(), minimized: window.minimized,
        maximized: typeof window.is_maximized === 'function' ? window.is_maximized() :
            window.maximized_horizontally && window.maximized_vertically,
        fullscreen: window.is_fullscreen(), active: window.has_focus(), hidden: window.is_hidden(),
        overrideRedirect: window.is_override_redirect(), managed: !window.is_override_redirect(),
        skipTaskbar: window.is_skip_taskbar(), monitor: window.get_monitor(),
        desktop: window.is_on_all_workspaces() ? -1 : workspace ? workspace.index() : null,
        transientFor: parent ? windowId(parent) : null,
        opacity: actor ? Math.round(actor.opacity * 100 / 255) : null, capabilities: caps,
    };
}

export default class WindowEnumExtension extends Extension {
    enable() {
        this._object = Gio.DBusExportedObject.wrapJSObject(XML, this);
        this._object.export(Gio.DBus.session, '/org/xetal/CachyOsTools/WindowEnum');
    }

    disable() {
        this._object?.unexport();
        this._object = null;
    }

    List() {
        try {
            return JSON.stringify({
                windows: allWindows().map(describe),
                coverage: 'GNOME/Mutter windows, including exposed override-redirect windows. Shell UI actors, subsurfaces and overlays drawn inside applications are not independent windows. X11 tree exposes additional XWayland child windows.',
            });
        } catch (error) { return JSON.stringify({error: String(error)}); }
    }

    Act(json) {
        try {
            const request = JSON.parse(json);
            const window = allWindows().find(w => windowId(w) === request.id);
            if (!window) throw new Error('The window no longer exists.');
            const caps = describe(window).capabilities;
            if (request.action === 'geometry' ?
                (request.move !== false && !caps.includes('move')) ||
                (request.resize !== false && !caps.includes('resize')) : !caps.includes(request.action))
                throw new Error('This operation is not supported for the selected window.');
            const numeric = (value, min, max) => Number.isInteger(value) && value >= min && value <= max;
            if (request.action === 'geometry' &&
                (!numeric(request.x, -32768, 32767) || !numeric(request.y, -32768, 32767) ||
                 !numeric(request.width, 1, 65535) || !numeric(request.height, 1, 65535)))
                throw new Error('Invalid geometry.');
            if (['above', 'minimized', 'maximized', 'fullscreen'].includes(request.action)
                && typeof request.value !== 'boolean') throw new Error('A boolean value is required.');
            switch (request.action) {
            case 'geometry': {
                const frame = window.get_frame_rect();
                if (request.resize === false) window.move_frame(true, request.x, request.y);
                else window.move_resize_frame(true, request.move === false ? frame.x : request.x,
                    request.move === false ? frame.y : request.y, request.width, request.height);
                break;
            }
            case 'activate': window.activate(global.get_current_time()); break;
            case 'raise': window.raise(); break;
            case 'lower': window.lower(); break;
            case 'above': request.value ? window.make_above() : window.unmake_above(); break;
            case 'minimized': request.value ? window.minimize() : window.unminimize(); break;
            case 'maximized':
                if (Meta.MaximizeFlags)
                    request.value ? window.maximize(Meta.MaximizeFlags.BOTH) : window.unmaximize(Meta.MaximizeFlags.BOTH);
                else request.value ? window.maximize() : window.unmaximize();
                break;
            case 'fullscreen': request.value ? window.make_fullscreen() : window.unmake_fullscreen(); break;
            case 'opacity':
                if (!numeric(request.value, 0, 100)) throw new Error('Invalid opacity.');
                window.get_compositor_private().opacity = Math.round(request.value * 255 / 100);
                break;
            case 'desktop':
                if (!numeric(request.value, -1, global.workspace_manager.n_workspaces - 1))
                    throw new Error('That workspace does not exist.');
                if (request.value < 0) window.stick();
                else { window.unstick(); window.change_workspace_by_index(request.value, false); }
                break;
            case 'close': window.delete(global.get_current_time()); break;
            default: throw new Error('Unsupported action.');
            }
            return JSON.stringify({ok: true, message: 'Request sent to Mutter. The compositor may constrain the resulting geometry or state.'});
        } catch (error) { return JSON.stringify({error: String(error)}); }
    }
}
