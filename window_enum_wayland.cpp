#include "window_enum_backend.h"
#include "wlr-foreign-toplevel-management-unstable-v1-client.h"
#include "ext-foreign-toplevel-list-v1-client.h"

#include <QCoreApplication>
#include <QHash>
#include <QJsonDocument>
#include <QSocketNotifier>
#include <QUuid>
#include <wayland-client.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstring>

namespace {
void output(const QJsonObject &value) {
    const auto bytes = QJsonDocument(value).toJson(QJsonDocument::Compact) + '\n';
    std::fwrite(bytes.constData(), 1, size_t(bytes.size()), stdout);
    std::fflush(stdout);
}
struct WaylandWindows;
struct Entry {
    WaylandWindows *owner;
    QJsonObject row;
    zwlr_foreign_toplevel_handle_v1 *wlr = nullptr;
    ext_foreign_toplevel_handle_v1 *ext = nullptr;
};
struct Global { uint32_t name = 0, version = 0; };
struct WaylandWindows {
    wl_display *display = nullptr;
    wl_seat *seat = nullptr;
    QHash<QString, Global> globals;
    QHash<QString, Entry *> entries;
    QString session = QUuid::createUuid().toString(QUuid::WithoutBraces);
    int next = 0;
    bool management = false;
    QByteArray input;
    ~WaylandWindows() {
        qDeleteAll(entries);
        if (display) wl_display_disconnect(display);
    }
    Entry *add() {
        auto *entry = new Entry{this, {{"id", session + ":" + QString::number(++next)},
            {"backend", "Wayland"}, {"type", "Toplevel"}, {"managed", true}, {"capabilities", QJsonArray()}}};
        entries.insert(entry->row["id"].toString(), entry);
        return entry;
    }
    void closed(Entry *entry) {
        entries.remove(entry->row["id"].toString());
        if (entry->wlr) zwlr_foreign_toplevel_handle_v1_destroy(entry->wlr);
        if (entry->ext) ext_foreign_toplevel_handle_v1_destroy(entry->ext);
        delete entry;
    }
    void snapshot() {
        QJsonArray rows;
        for (const auto *entry : entries) rows.append(entry->row);
        output({{"windows", rows}, {"coverage", management ?
            "Wayland foreign toplevels: application windows and advertised state controls. This compositor protocol does not expose position, size, PID, executable, popups or layer-shell overlays. Use KDE/GNOME access or the X11 tree for additional information." :
            "Wayland foreign toplevel list: title and app ID only. This compositor does not expose geometry, process ownership, overlays or window controls through this protocol."}});
    }
    void request(const QJsonObject &request) {
        if (request["action"] == "list") { snapshot(); return; }
        QString error;
        if (!windowEnumValidateAction(request, &error)) { output({{"error", error}}); return; }
        auto *entry = entries.value(request["id"].toString());
        if (!entry || !entry->wlr) { output({{"error", "The window closed or this backend is read-only."}}); return; }
        const QString action = request["action"].toString();
        if (!entry->row["capabilities"].toArray().contains(action)) {
            output({{"error", "The compositor does not expose that operation."}}); return;
        }
        const bool value = request["value"].toBool();
        if (action == "activate") zwlr_foreign_toplevel_handle_v1_activate(entry->wlr, seat);
        else if (action == "close") zwlr_foreign_toplevel_handle_v1_close(entry->wlr);
        else if (action == "maximized") {
            if (value) zwlr_foreign_toplevel_handle_v1_set_maximized(entry->wlr);
            else zwlr_foreign_toplevel_handle_v1_unset_maximized(entry->wlr);
        } else if (action == "minimized") {
            if (value) zwlr_foreign_toplevel_handle_v1_set_minimized(entry->wlr);
            else zwlr_foreign_toplevel_handle_v1_unset_minimized(entry->wlr);
        } else if (action == "fullscreen") {
            if (value) zwlr_foreign_toplevel_handle_v1_set_fullscreen(entry->wlr, nullptr);
            else zwlr_foreign_toplevel_handle_v1_unset_fullscreen(entry->wlr);
        }
        wl_display_flush(display);
        output({{"ok", true}, {"message", "Request sent to the compositor; its policy determines the resulting state."}});
    }
};
Entry *entry(void *data) { return static_cast<Entry *>(data); }
const zwlr_foreign_toplevel_handle_v1_listener handleListener = {
    [](void *d, auto *, const char *s) { entry(d)->row["title"] = QString::fromUtf8(s); },
    [](void *d, auto *, const char *s) { entry(d)->row["appId"] = QString::fromUtf8(s); },
    [](void *, auto *, wl_output *) {},
    [](void *, auto *, wl_output *) {},
    [](void *d, auto *, wl_array *states) {
        auto &row = entry(d)->row;
        row["maximized"] = false; row["minimized"] = false;
        row["active"] = false; row["fullscreen"] = false;
        const auto *values = static_cast<uint32_t *>(states->data);
        for (size_t i = 0; i < states->size / sizeof(uint32_t); ++i) {
            switch (values[i]) {
            case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MAXIMIZED: row["maximized"] = true; break;
            case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED: row["minimized"] = true; break;
            case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED: row["active"] = true; break;
            case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_FULLSCREEN: row["fullscreen"] = true; break;
            }
        }
    },
    [](void *, auto *) {},
    [](void *d, auto *) { auto *e = entry(d); e->owner->closed(e); },
    [](void *d, auto *, zwlr_foreign_toplevel_handle_v1 *parent) {
        if (!parent) { entry(d)->row.remove("transientFor"); return; }
        for (const auto *other : entry(d)->owner->entries)
            if (other->wlr == parent) entry(d)->row["transientFor"] = other->row["id"];
    }
};
const zwlr_foreign_toplevel_manager_v1_listener managerListener = {
    [](void *d, auto *, zwlr_foreign_toplevel_handle_v1 *handle) {
        auto *owner = static_cast<WaylandWindows *>(d);
        auto *e = owner->add(); e->wlr = handle;
        QJsonArray caps{"close", "maximized", "minimized"};
        if (owner->seat) caps.append("activate");
        if (zwlr_foreign_toplevel_handle_v1_get_version(handle) >= 2) caps.append("fullscreen");
        e->row["capabilities"] = caps;
        zwlr_foreign_toplevel_handle_v1_add_listener(handle, &handleListener, e);
    },
    [](void *, auto *) {
        output({{"error", "The compositor stopped window enumeration."}});
        QCoreApplication::exit(1);
    }
};
const ext_foreign_toplevel_handle_v1_listener extHandleListener = {
    [](void *d, auto *) { auto *e = entry(d); e->owner->closed(e); },
    [](void *, auto *) {},
    [](void *d, auto *, const char *s) { entry(d)->row["title"] = QString::fromUtf8(s); },
    [](void *d, auto *, const char *s) { entry(d)->row["appId"] = QString::fromUtf8(s); },
    [](void *d, auto *, const char *s) { entry(d)->row["nativeId"] = QString::fromUtf8(s); }
};
const ext_foreign_toplevel_list_v1_listener extManagerListener = {
    [](void *d, auto *, ext_foreign_toplevel_handle_v1 *handle) {
        auto *e = static_cast<WaylandWindows *>(d)->add(); e->ext = handle;
        ext_foreign_toplevel_handle_v1_add_listener(handle, &extHandleListener, e);
    },
    [](void *, auto *) {
        output({{"error", "The compositor stopped window enumeration."}});
        QCoreApplication::exit(1);
    }
};
}

int windowEnumWayland()
{
    WaylandWindows windows;
    windows.display = wl_display_connect(nullptr);
    if (!windows.display) { output({{"error", "Cannot connect to the Wayland compositor."}}); return 1; }
    auto *registry = wl_display_get_registry(windows.display);
    static const wl_registry_listener registryListener = {
        [](void *d, wl_registry *, uint32_t id, const char *name, uint32_t version) {
            static_cast<WaylandWindows *>(d)->globals.insert(QString::fromLatin1(name), {id, version});
        },
        [](void *, wl_registry *, uint32_t) {}
    };
    wl_registry_add_listener(registry, &registryListener, &windows);
    if (wl_display_roundtrip(windows.display) < 0) return 1;
    const auto seat = windows.globals.value("wl_seat");
    if (seat.name) {
        windows.seat = static_cast<wl_seat *>(wl_registry_bind(registry, seat.name, &wl_seat_interface, 1));
        static const wl_seat_listener listener{
            [](void *, wl_seat *, uint32_t) {}, [](void *, wl_seat *, const char *) {}};
        wl_seat_add_listener(windows.seat, &listener, nullptr);
    }
    const auto wlr = windows.globals.value("zwlr_foreign_toplevel_manager_v1");
    const auto ext = windows.globals.value("ext_foreign_toplevel_list_v1");
    if (wlr.name) {
        windows.management = true;
        auto *manager = static_cast<zwlr_foreign_toplevel_manager_v1 *>(
            wl_registry_bind(registry, wlr.name, &zwlr_foreign_toplevel_manager_v1_interface, qMin(wlr.version, 3u)));
        zwlr_foreign_toplevel_manager_v1_add_listener(manager, &managerListener, &windows);
    } else if (ext.name) {
        auto *manager = static_cast<ext_foreign_toplevel_list_v1 *>(
            wl_registry_bind(registry, ext.name, &ext_foreign_toplevel_list_v1_interface, 1));
        ext_foreign_toplevel_list_v1_add_listener(manager, &extManagerListener, &windows);
    } else {
        output({{"error", "This compositor exposes no foreign-window protocol. Select KDE or enable the GNOME bridge; X11 tree can still inspect XWayland windows."}});
        return 1;
    }
    if (wl_display_roundtrip(windows.display) < 0) return 1;
    windows.snapshot();
    QSocketNotifier display(wl_display_get_fd(windows.display), QSocketNotifier::Read);
    QObject::connect(&display, &QSocketNotifier::activated, [&]() {
        if (wl_display_dispatch(windows.display) < 0) {
            output({{"error", "The Wayland connection closed."}});
            QCoreApplication::exit(1);
        }
    });
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
    QSocketNotifier input(STDIN_FILENO, QSocketNotifier::Read);
    QObject::connect(&input, &QSocketNotifier::activated, [&]() {
        char buffer[8192];
        const auto size = ::read(STDIN_FILENO, buffer, sizeof(buffer));
        if (size == 0) { QCoreApplication::quit(); return; }
        if (size < 0) return;
        windows.input.append(buffer, size);
        int newline;
        while ((newline = windows.input.indexOf('\n')) >= 0) {
            const auto line = windows.input.left(newline);
            windows.input.remove(0, newline + 1);
            QJsonParseError error;
            const auto document = QJsonDocument::fromJson(line, &error);
            if (error.error != QJsonParseError::NoError || !document.isObject())
                output({{"error", "Invalid JSON request."}});
            else windows.request(document.object());
        }
    });
    return QCoreApplication::exec();
}
