// Executed as a temporary KWin script for one request, then unloaded by the app.
// Window titles/classes are data, never source text.
function windowEnumKWin(request) {
    function read(window, name, fallback) {
        try {
            var value = window[name];
            return value === undefined || value === null ? fallback : value;
        } catch (_) { return fallback; }
    }
    function windows() {
        var result = [], seen = {};
        var lists = [typeof workspace.windowList === "function" ? workspace.windowList() :
                     typeof workspace.clientList === "function" ? workspace.clientList() : [],
                     workspace.stackingOrder || []];
        lists.forEach(function(list) {
            for (var i = 0; i < list.length; ++i) {
                var w = list[i], id = String(w.internalId);
                if (!seen[id]) { seen[id] = true; result.push(w); }
            }
        });
        return result;
    }
    function describe(w) {
        var g = w.frameGeometry;
        var row = {
            id: String(w.internalId), backend: "KWin", title: read(w, "caption", ""),
            class: String(read(w, "resourceClass", "")), instance: String(read(w, "resourceName", "")),
            appId: read(w, "desktopFileName", ""), pid: read(w, "pid", 0), pidSource: "KWin",
            x: g.x, y: g.y, width: g.width, height: g.height,
            frameX: g.x, frameY: g.y, frameWidth: g.width, frameHeight: g.height,
            geometryUnits: "Compositor logical coordinates; window frame",
            type: read(w, "windowType", null), role: read(w, "windowRole", ""),
            active: read(w, "active", null), above: read(w, "keepAbove", null),
            below: read(w, "keepBelow", null), minimized: read(w, "minimized", null),
            fullscreen: read(w, "fullScreen", null), opacity: Math.round(read(w, "opacity", 1) * 100),
            skipTaskbar: read(w, "skipTaskbar", null), skipPager: read(w, "skipPager", null),
            skipSwitcher: read(w, "skipSwitcher", null), modal: read(w, "modal", null),
            attention: read(w, "demandsAttention", null), decorated: !read(w, "noBorder", false),
            managed: read(w, "managed", null), deleted: read(w, "deleted", false),
            popup: read(w, "popupWindow", null), hidden: read(w, "hidden", null),
            stack: read(w, "stackingOrder", null), activities: read(w, "activities", []),
            unresponsive: read(w, "unresponsive", null), capabilities: []
        };
        var mode = read(w, "maximizeMode", null);
        if (mode !== null) row.maximized = Number(mode) === 3;
        else if (read(w, "maximizedHorizontally", null) !== null)
            row.maximized = w.maximizedHorizontally && w.maximizedVertically;
        var output = read(w, "output", null);
        row.monitor = output ? read(output, "name", "") : read(w, "screen", null);
        var desktops = read(w, "desktops", null);
        if (desktops !== null) {
            row.desktop = desktops.length ? Number(desktops[0].x11DesktopNumber) - 1 : -1;
            row.workspaces = Array.prototype.map.call(desktops, function(d) { return d.name; });
        } else {
            var desktop = read(w, "desktop", null);
            if (desktop !== null) row.desktop = desktop < 0 ? -1 : desktop - 1;
        }
        var parent = read(w, "transientFor", null);
        if (parent) row.transientFor = String(parent.internalId);
        var types = [["desktopWindow", "Desktop"], ["dock", "Dock"], ["toolbar", "Toolbar"],
            ["menu", "Menu"], ["normalWindow", "Normal"], ["dialog", "Dialog"],
            ["splash", "Splash"], ["utility", "Utility"], ["dropdownMenu", "Dropdown menu"],
            ["popupMenu", "Popup menu"], ["tooltip", "Tooltip"], ["notification", "Notification"],
            ["criticalNotification", "Critical notification"], ["appletPopup", "Applet popup"],
            ["osd", "On-screen display"], ["dndIcon", "Drag-and-drop"], ["popupWindow", "Popup"]];
        var labels = [];
        types.forEach(function(pair) { if (read(w, pair[0], false)) labels.push(pair[1]); });
        if (labels.length) row.type = labels;
        if (!row.deleted && row.managed !== false) {
            row.capabilities = ["activate", "above", "below", "opacity", "desktop", "decorated"];
            if (read(w, "moveable", false)) row.capabilities.push("move");
            if (read(w, "resizeable", false)) row.capabilities.push("resize");
            if (read(w, "minimizable", read(w, "minimizeable", false))) row.capabilities.push("minimized");
            if (read(w, "maximizable", read(w, "maximizeable", false))) row.capabilities.push("maximized");
            if (read(w, "fullScreenable", false)) row.capabilities.push("fullscreen");
            if (read(w, "closeable", false)) row.capabilities.push("close");
            if (typeof workspace.raiseWindow === "function") row.capabilities.push("raise");
        }
        return row;
    }
    function action(w) {
        var caps = describe(w).capabilities;
        if (request.action === "geometry" ?
            ((request.move !== false && caps.indexOf("move") < 0) ||
             (request.resize !== false && caps.indexOf("resize") < 0)) : caps.indexOf(request.action) < 0)
            throw new Error("KWin does not support that operation for this window.");
        switch (request.action) {
        case "geometry":
            // Copy into a plain object: newer KWin exposes a RectF value
            // wrapper whose individual fields may not mutate as a JS object.
            var current = w.frameGeometry;
            var g = {x: current.x, y: current.y, width: current.width, height: current.height};
            if (request.move !== false) { g.x = request.x; g.y = request.y; }
            if (request.resize !== false) { g.width = request.width; g.height = request.height; }
            w.frameGeometry = g; break;
        case "activate":
            if (typeof workspace.activateWindow === "function") workspace.activateWindow(w);
            else if ("activeWindow" in workspace) workspace.activeWindow = w;
            else workspace.activeClient = w;
            break;
        case "raise": workspace.raiseWindow(w); break;
        case "above": w.keepAbove = request.value; break;
        case "below": w.keepBelow = request.value; break;
        case "minimized": w.minimized = request.value; break;
        case "maximized": w.setMaximize(request.value, request.value); break;
        case "fullscreen": w.fullScreen = request.value; break;
        case "opacity": w.opacity = request.value / 100; break;
        case "decorated": w.noBorder = !request.value; break;
        case "desktop":
            if (read(w, "desktops", null) !== null) {
                var desktops = workspace.desktops;
                if (request.value >= desktops.length) throw new Error("That workspace does not exist.");
                w.desktops = request.value < 0 ? [] : [desktops[request.value]];
            } else w.desktop = request.value < 0 ? -1 : request.value + 1;
            break;
        case "close": w.closeWindow(); break;
        default: throw new Error("Unsupported action.");
        }
    }
    var result;
    try {
        var list = windows();
        if (request.action === "list") {
            result = {windows: list.map(describe),
                coverage: "KWin workspace and stacking lists, including exposed popups, panels and overlays. Internal compositor surfaces and application-drawn overlays are not independent windows. X11 tree also exposes XWayland child windows."};
        } else {
            var target = list.filter(function(w) { return String(w.internalId) === request.id; })[0];
            if (!target) throw new Error("The window no longer exists.");
            action(target);
            result = {ok: true, message: "Request sent to KWin. Window rules and application size constraints may affect the result."};
        }
    } catch (error) { result = {error: String(error)}; }
    callDBus(request.service, request.path, "org.xetal.CachyOsTools.WindowEnum",
             "Publish", request.token, JSON.stringify(result));
}
