#include "window_enum_backend.h"

#include <QHash>
#include <QSet>
#include <QSysInfo>
#include <xcb/xcb.h>
#include <xcb/res.h>
#include <memory>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace {
template<class T> using Reply = std::unique_ptr<T, decltype(&std::free)>;
template<class T> Reply<T> reply(T *p) { return Reply<T>(p, &std::free); }
QString xid(uint32_t id) { return QString("0x%1").arg(id, 8, 16, QLatin1Char('0')); }

class XWindows {
public:
    xcb_connection_t *connection = nullptr;
    QHash<QString, xcb_atom_t> atoms;
    QHash<xcb_atom_t, QString> names;
    QSet<xcb_window_t> clients;
    QHash<xcb_window_t, int> stacking;
    QHash<xcb_window_t, int> siblingStacking;
    QHash<xcb_window_t, xcb_window_t> parents;
    QSet<xcb_window_t> roots;
    bool hasRes = false;

    XWindows() {
        connection = xcb_connect(nullptr, nullptr);
        if (xcb_connection_has_error(connection)) return;
        const auto *extension = xcb_get_extension_data(connection, &xcb_res_id);
        if (extension && extension->present) {
            auto version = reply(xcb_res_query_version_reply(connection,
                                 xcb_res_query_version(connection, 1, 2), nullptr));
            hasRes = version && (version->server_major > 1 ||
                (version->server_major == 1 && version->server_minor >= 2));
        }
        for (auto it = xcb_setup_roots_iterator(xcb_get_setup(connection)); it.rem; xcb_screen_next(&it)) {
            roots.insert(it.data->root);
            for (uint32_t window : integers(it.data->root, "_NET_CLIENT_LIST")) clients.insert(window);
            auto order = integers(it.data->root, "_NET_CLIENT_LIST_STACKING");
            for (int i = 0; i < order.size(); ++i) stacking.insert(order[i], i);
        }
    }
    ~XWindows() { if (connection) xcb_disconnect(connection); }
    xcb_atom_t atom(const QString &name) {
        if (atoms.contains(name)) return atoms[name];
        const QByteArray bytes = name.toLatin1();
        auto result = reply(xcb_intern_atom_reply(connection,
                            xcb_intern_atom(connection, false, bytes.size(), bytes.constData()), nullptr));
        const xcb_atom_t id = result ? result->atom : xcb_atom_t(XCB_ATOM_NONE);
        atoms.insert(name, id);
        names.insert(id, name);
        return id;
    }
    QString name(xcb_atom_t id) {
        if (names.contains(id)) return names[id];
        auto result = reply(xcb_get_atom_name_reply(connection, xcb_get_atom_name(connection, id), nullptr));
        const QString text = result ? QString::fromLatin1(xcb_get_atom_name_name(result.get()),
                        xcb_get_atom_name_name_length(result.get())) : xid(id);
        names.insert(id, text);
        return text;
    }
    Reply<xcb_get_property_reply_t> property(xcb_window_t window, xcb_atom_t key) {
        return reply(xcb_get_property_reply(connection,
                     xcb_get_property(connection, false, window, key, XCB_GET_PROPERTY_TYPE_ANY, 0, 16384), nullptr));
    }
    QByteArray bytes(xcb_window_t window, const QString &key) {
        auto value = property(window, atom(key));
        if (!value || value->format != 8) return {};
        return QByteArray(static_cast<const char *>(xcb_get_property_value(value.get())),
                          xcb_get_property_value_length(value.get()));
    }
    QVector<uint32_t> integers(xcb_window_t window, const QString &key) {
        auto value = property(window, atom(key));
        QVector<uint32_t> result;
        if (!value || value->format != 32) return result;
        const auto *data = static_cast<const uint32_t *>(xcb_get_property_value(value.get()));
        for (int i = 0; i < xcb_get_property_value_length(value.get()) / 4; ++i) result << data[i];
        return result;
    }
    QJsonArray atomNames(const QVector<uint32_t> &values) {
        QJsonArray result;
        for (auto value : values) result.append(name(value));
        return result;
    }
    int localPid(xcb_window_t window) {
        if (!hasRes) return 0;
        const xcb_res_client_id_spec_t spec{window, XCB_RES_CLIENT_ID_MASK_LOCAL_CLIENT_PID};
        auto ids = reply(xcb_res_query_client_ids_reply(connection,
                         xcb_res_query_client_ids(connection, 1, &spec), nullptr));
        if (!ids) return 0;
        for (auto it = xcb_res_query_client_ids_ids_iterator(ids.get()); it.rem; xcb_res_client_id_value_next(&it)) {
            if ((it.data->spec.mask & XCB_RES_CLIENT_ID_MASK_LOCAL_CLIENT_PID) &&
                xcb_res_client_id_value_value_length(it.data) > 0) {
                const uint32_t pid = *xcb_res_client_id_value_value(it.data);
                if (pid && pid <= uint32_t(std::numeric_limits<int>::max())) return int(pid);
            }
        }
        return 0;
    }
    QJsonObject window(xcb_window_t id, bool raw = false) {
        auto ac = xcb_get_window_attributes(connection, id);
        auto gc = xcb_get_geometry(connection, id);
        auto attributes = reply(xcb_get_window_attributes_reply(connection, ac, nullptr));
        auto geometry = reply(xcb_get_geometry_reply(connection, gc, nullptr));
        if (!attributes || !geometry) return {}; // Window closed during the scan.
        auto translated = reply(xcb_translate_coordinates_reply(connection,
            xcb_translate_coordinates(connection, id, geometry->root, 0, 0), nullptr));
        if (!translated) return {};
        if (!parents.contains(id)) {
            auto tree = reply(xcb_query_tree_reply(connection, xcb_query_tree(connection, id), nullptr));
            if (tree) parents[id] = tree->parent;
        }
        const bool managed = clients.contains(id);
        QJsonObject row{{"id", xid(id)}, {"backend", "X11"}, {"parent", xid(parents.value(id))},
            {"x", translated->dst_x}, {"y", translated->dst_y}, {"width", geometry->width},
            {"height", geometry->height}, {"border", geometry->border_width}, {"depth", geometry->depth},
            {"overrideRedirect", bool(attributes->override_redirect)}, {"managed", managed},
            {"inputOnly", attributes->_class == XCB_WINDOW_CLASS_INPUT_ONLY},
            {"mapped", attributes->map_state != XCB_MAP_STATE_UNMAPPED},
            {"visibility", attributes->map_state == XCB_MAP_STATE_VIEWABLE ? "Viewable" :
                attributes->map_state == XCB_MAP_STATE_UNVIEWABLE ? "Unviewable" : "Unmapped"},
            {"geometryUnits", "X11 pixels; client area; root-relative position"}};
        QByteArray title = bytes(id, "_NET_WM_NAME");
        row["title"] = title.isEmpty() ? QString::fromLocal8Bit(bytes(id, "WM_NAME")) : QString::fromUtf8(title);
        auto classes = bytes(id, "WM_CLASS").split('\0');
        if (!classes.isEmpty()) row["instance"] = QString::fromLocal8Bit(classes[0]);
        if (classes.size() > 1) row["class"] = QString::fromLocal8Bit(classes[1]);
        row["appId"] = QString::fromUtf8(bytes(id, "_GTK_APPLICATION_ID"));
        row["role"] = QString::fromUtf8(bytes(id, "WM_WINDOW_ROLE"));
        const QString machine = QString::fromLocal8Bit(bytes(id, "WM_CLIENT_MACHINE")).remove(QChar('\0'));
        row["machine"] = machine;
        int pid = localPid(id);
        if (pid) row["pidSource"] = "XRes (local client)";
        else {
            auto pids = integers(id, "_NET_WM_PID");
            if (!pids.isEmpty() && pids[0] <= uint32_t(std::numeric_limits<int>::max())) {
                pid = int(pids[0]);
                row["pidSource"] = "_NET_WM_PID (client reported)";
                // A remote or unspecified host cannot reliably name a local /proc process.
                row["remote"] = machine.isEmpty() || (machine != QSysInfo::machineHostName()
                    && machine != "localhost" && machine != QSysInfo::machineHostName().section('.', 0, 0));
            }
        }
        if (pid) row["pid"] = pid;
        const auto state = integers(id, "_NET_WM_STATE");
        const auto types = integers(id, "_NET_WM_WINDOW_TYPE");
        const auto allowed = integers(id, "_NET_WM_ALLOWED_ACTIONS");
        const auto protocols = integers(id, "WM_PROTOCOLS");
        const auto supported = integers(geometry->root, "_NET_SUPPORTED");
        row["states"] = atomNames(state);
        row["type"] = roots.contains(id) ? QJsonValue("Root") : types.isEmpty() ?
            QJsonValue(managed ? "Normal" : attributes->override_redirect ? "Unmanaged / overlay" : "Child / unmapped / internal")
            : QJsonValue(atomNames(types));
        row["protocols"] = atomNames(protocols);
        row["allowedActions"] = atomNames(allowed);
        for (const auto &flag : QList<QPair<QString, QString>>{
            {"above", "_NET_WM_STATE_ABOVE"}, {"below", "_NET_WM_STATE_BELOW"},
            {"minimized", "_NET_WM_STATE_HIDDEN"}, {"fullscreen", "_NET_WM_STATE_FULLSCREEN"},
            {"skipTaskbar", "_NET_WM_STATE_SKIP_TASKBAR"}, {"skipPager", "_NET_WM_STATE_SKIP_PAGER"},
            {"modal", "_NET_WM_STATE_MODAL"}, {"attention", "_NET_WM_STATE_DEMANDS_ATTENTION"}}) {
            if (managed || !state.isEmpty()) row[flag.first] = state.contains(atom(flag.second));
        }
        if (managed) row["maximized"] = state.contains(atom("_NET_WM_STATE_MAXIMIZED_HORZ")) &&
            state.contains(atom("_NET_WM_STATE_MAXIMIZED_VERT"));
        if (stacking.contains(id)) row["stack"] = stacking[id];
        if (siblingStacking.contains(id)) row["siblingStack"] = siblingStacking[id];
        const auto active = integers(geometry->root, "_NET_ACTIVE_WINDOW");
        if (!active.isEmpty()) row["active"] = active[0] == id;
        const auto desktop = integers(id, "_NET_WM_DESKTOP");
        if (!desktop.isEmpty()) row["desktop"] = desktop[0] == UINT32_MAX ? -1 : int(desktop[0]);
        const auto transient = integers(id, "WM_TRANSIENT_FOR");
        if (!transient.isEmpty()) row["transientFor"] = xid(transient[0]);
        const auto opacity = integers(id, "_NET_WM_WINDOW_OPACITY");
        row["opacity"] = opacity.isEmpty() ? 100 : int(qRound(double(opacity[0]) * 100 / UINT32_MAX));
        const auto extents = integers(id, "_NET_FRAME_EXTENTS");
        if (extents.size() == 4) {
            row["frameX"] = translated->dst_x - int(extents[0]);
            row["frameY"] = translated->dst_y - int(extents[2]);
            row["frameWidth"] = int(geometry->width + extents[0] + extents[1]);
            row["frameHeight"] = int(geometry->height + extents[2] + extents[3]);
        }
        QJsonArray capabilities;
        auto supports = [&](const char *key) { return supported.contains(atom(key)); };
        auto permits = [&](const char *key) { return allowed.isEmpty() || allowed.contains(atom(key)); };
        if (!roots.contains(id)) {
            capabilities = {"title", "raise", "lower"};
            if (attributes->_class != XCB_WINDOW_CLASS_INPUT_ONLY) capabilities.append("opacity");
            if (!managed || supports("_NET_MOVERESIZE_WINDOW")) {
                if (!managed || permits("_NET_WM_ACTION_MOVE")) capabilities.append("move");
                if (!managed || permits("_NET_WM_ACTION_RESIZE")) capabilities.append("resize");
            }
            if (managed && supports("_NET_ACTIVE_WINDOW")) capabilities.append("activate");
            if (managed && supports("_NET_WM_STATE")) {
                for (const auto &flag : QList<QPair<QString, QString>>{
                    {"above", "_NET_WM_STATE_ABOVE"}, {"below", "_NET_WM_STATE_BELOW"},
                    {"fullscreen", "_NET_WM_STATE_FULLSCREEN"}})
                    // KWin advertises ABOVE/BELOW states but omits their
                    // optional per-window action hints. Honor state support.
                    if (supports(flag.second.toLatin1().constData()) &&
                        (flag.first != "fullscreen" || permits("_NET_WM_ACTION_FULLSCREEN")))
                        capabilities.append(flag.first);
                if (supports("_NET_WM_STATE_MAXIMIZED_HORZ") && supports("_NET_WM_STATE_MAXIMIZED_VERT") &&
                    permits("_NET_WM_ACTION_MAXIMIZE_HORZ") && permits("_NET_WM_ACTION_MAXIMIZE_VERT"))
                    capabilities.append("maximized");
                if (permits("_NET_WM_ACTION_MINIMIZE")) capabilities.append("minimized");
                if (supports("_NET_WM_DESKTOP")) capabilities.append("desktop");
            }
            // Graceful close only; never XKillClient (which can kill unrelated windows).
            if (protocols.contains(atom("WM_DELETE_WINDOW"))) capabilities.append("close");
        }
        row["capabilities"] = capabilities;
        windowEnumProcessInfo(row);
        if (raw) {
            QJsonObject properties;
            auto list = reply(xcb_list_properties_reply(connection, xcb_list_properties(connection, id), nullptr));
            if (list) {
                const auto *keys = xcb_list_properties_atoms(list.get());
                for (int i = 0; i < xcb_list_properties_atoms_length(list.get()); ++i) {
                    auto value = property(id, keys[i]);
                    if (!value) continue;
                    QJsonObject p{{"type", name(value->type)}, {"format", value->format},
                                  {"remainingBytes", double(value->bytes_after)}};
                    const int length = xcb_get_property_value_length(value.get());
                    const auto *data = static_cast<const char *>(xcb_get_property_value(value.get()));
                    if (value->format == 8) p["value"] = QString::fromUtf8(data, length);
                    else if (value->format == 32) {
                        QJsonArray items;
                        const auto *values = reinterpret_cast<const uint32_t *>(data);
                        for (int n = 0; n < length / 4; ++n)
                            items.append(value->type == XCB_ATOM_ATOM ? QJsonValue(name(values[n])) : QJsonValue(double(values[n])));
                        p["value"] = items;
                    } else p["hex"] = QString::fromLatin1(QByteArray(data, length).toHex());
                    properties[name(keys[i])] = p;
                }
            }
            row["properties"] = properties;
        }
        return row;
    }
    QJsonObject snapshot() {
        QList<xcb_window_t> queue = roots.values();
        QSet<xcb_window_t> visited;
        QJsonArray rows;
        for (qsizetype i = 0; i < queue.size(); ++i) {
            const auto id = queue[i];
            if (visited.contains(id)) continue;
            visited.insert(id);
            auto tree = reply(xcb_query_tree_reply(connection, xcb_query_tree(connection, id), nullptr));
            if (!tree) continue;
            parents[id] = tree->parent;
            const auto *children = xcb_query_tree_children(tree.get());
            for (int n = 0; n < xcb_query_tree_children_length(tree.get()); ++n) {
                queue.append(children[n]);
                siblingStacking[children[n]] = n;
            }
            const auto row = window(id);
            if (!row.isEmpty()) rows.append(row);
        }
        return {{"windows", rows}, {"coverage", "Full accessible X11 tree: roots, managed, child, hidden, input-only and override-redirect windows. On Wayland this tree covers XWayland only."}};
    }
    QString checked(xcb_void_cookie_t cookie) {
        auto error = reply(xcb_request_check(connection, cookie));
        return error ? QString("X11 error %1; the window may have closed or rejected the operation.").arg(error->error_code) : QString();
    }
    QString message(xcb_window_t root, xcb_window_t id, const QString &type, std::initializer_list<uint32_t> data,
                    bool toRoot = true) {
        xcb_client_message_event_t event {};
        event.response_type = XCB_CLIENT_MESSAGE;
        event.window = id;
        event.type = atom(type);
        event.format = 32;
        int i = 0;
        for (auto value : data) event.data.data32[i++] = value;
        return checked(xcb_send_event_checked(connection, false, toRoot ? root : id,
            toRoot ? XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY : XCB_EVENT_MASK_NO_EVENT,
            reinterpret_cast<const char *>(&event)));
    }
    QJsonObject act(const QJsonObject &request) {
        QString error;
        if (!windowEnumValidateAction(request, &error)) return {{"error", error}};
        bool ok = false;
        const auto id = request["id"].toString().toUInt(&ok, 16);
        if (!ok) return {{"error", "Invalid X11 window ID."}};
        const auto row = window(id);
        if (row.isEmpty()) return {{"error", "The window no longer exists."}};
        if (request.value("pid").toInt() > 0 && request["pid"] != row["pid"])
            return {{"error", "Window ownership changed. Refresh and select it again."}};
        const QString action = request["action"].toString();
        const QJsonArray caps = row["capabilities"].toArray();
        const bool move = request.value("move").toBool(true), resize = request.value("resize").toBool(true);
        if (action == "geometry" ? ((move && !caps.contains("move")) || (resize && !caps.contains("resize"))) : !caps.contains(action))
            return {{"error", "This operation is not supported for the selected window."}};
        auto geometry = reply(xcb_get_geometry_reply(connection, xcb_get_geometry(connection, id), nullptr));
        if (!geometry) return {{"error", "The window closed."}};
        const auto root = geometry->root;
        const bool managed = row["managed"].toBool();
        if (action == "geometry") {
            const int x = move ? request["x"].toInt() : row["x"].toInt();
            const int y = move ? request["y"].toInt() : row["y"].toInt();
            const uint32_t width = resize ? request["width"].toInt() : row["width"].toInt();
            const uint32_t height = resize ? request["height"].toInt() : row["height"].toInt();
            if (managed) error = message(root, id, "_NET_MOVERESIZE_WINDOW",
                {uint32_t(10 | ((move ? 3 : 0) << 8) | ((resize ? 3 : 0) << 10) | (2 << 12)),
                 uint32_t(x), uint32_t(y), width, height});
            else {
                // ConfigureWindow positions children relative to their parent.
                auto origin = reply(xcb_translate_coordinates_reply(connection,
                    xcb_translate_coordinates(connection, parents.value(id), root, 0, 0), nullptr));
                if (!origin) return {{"error", "The parent window closed."}};
                const uint32_t values[]{uint32_t(x - origin->dst_x), uint32_t(y - origin->dst_y), width, height};
                error = checked(xcb_configure_window_checked(connection, id, XCB_CONFIG_WINDOW_X |
                    XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values));
            }
        } else if (action == "activate") error = message(root, id, "_NET_ACTIVE_WINDOW", {2, XCB_CURRENT_TIME, 0});
        else if (action == "raise" || action == "lower") {
            const uint32_t mode = action == "raise" ? XCB_STACK_MODE_ABOVE : XCB_STACK_MODE_BELOW;
            if (managed && integers(root, "_NET_SUPPORTED").contains(atom("_NET_RESTACK_WINDOW")))
                error = message(root, id, "_NET_RESTACK_WINDOW", {2, 0, mode});
            else error = checked(xcb_configure_window_checked(connection, id, XCB_CONFIG_WINDOW_STACK_MODE, &mode));
        } else if (action == "minimized") {
            if (request["value"].toBool()) error = message(root, id, "WM_CHANGE_STATE", {3});
            else error = checked(xcb_map_window_checked(connection, id));
        } else if (action == "desktop") {
            const int desktop = request["value"].toInt();
            const auto count = integers(root, "_NET_NUMBER_OF_DESKTOPS");
            if (desktop >= 0 && (count.isEmpty() || uint32_t(desktop) >= count[0]))
                return {{"error", "That workspace does not exist."}};
            error = message(root, id, "_NET_WM_DESKTOP", {uint32_t(desktop), 2});
        } else if (action == "close") error = message(root, id, "WM_PROTOCOLS", {atom("WM_DELETE_WINDOW"), XCB_CURRENT_TIME}, false);
        else if (action == "opacity") {
            const uint32_t opacity = uint32_t(double(request["value"].toInt()) * UINT32_MAX / 100);
            error = checked(xcb_change_property_checked(connection, XCB_PROP_MODE_REPLACE, id,
                atom("_NET_WM_WINDOW_OPACITY"), XCB_ATOM_CARDINAL, 32, 1, &opacity));
        } else if (action == "title") {
            const QByteArray text = request["value"].toString().toUtf8();
            error = checked(xcb_change_property_checked(connection, XCB_PROP_MODE_REPLACE, id, atom("_NET_WM_NAME"),
                atom("UTF8_STRING"), 8, text.size(), text.constData()));
        } else {
            const QHash<QString, QString> states{{"above", "_NET_WM_STATE_ABOVE"}, {"below", "_NET_WM_STATE_BELOW"},
                {"fullscreen", "_NET_WM_STATE_FULLSCREEN"}, {"maximized", "_NET_WM_STATE_MAXIMIZED_HORZ"}};
            error = message(root, id, "_NET_WM_STATE", {request["value"].toBool() ? 1u : 0u, atom(states[action]),
                action == "maximized" ? atom("_NET_WM_STATE_MAXIMIZED_VERT") : 0u, 2});
        }
        xcb_flush(connection);
        return error.isEmpty() ? QJsonObject{{"ok", true}, {"message", "Request sent. Refresh to see the window manager's resulting state."}}
                               : QJsonObject{{"error", error}};
    }
};
}

QJsonObject windowEnumX11(const QJsonObject &request)
{
    XWindows windows;
    if (xcb_connection_has_error(windows.connection))
        return {{"error", "Cannot connect to the X11 display. Run in your desktop session; native Wayland windows need a compositor backend."}};
    if (request["action"] == "list") return windows.snapshot();
    if (request["action"] == "details") {
        bool ok;
        const auto id = request["id"].toString().toUInt(&ok, 16);
        const auto row = ok ? windows.window(id, true) : QJsonObject();
        return row.isEmpty() ? QJsonObject{{"error", "The window no longer exists."}} : QJsonObject{{"window", row}};
    }
    return windows.act(request);
}
