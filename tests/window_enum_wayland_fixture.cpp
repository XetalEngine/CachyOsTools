// A private protocol server, not a desktop compositor. It never opens a host
// display or input device; it exposes one synthetic window to each test client.
#include "wlr-foreign-toplevel-management-unstable-v1-server.h"
#include "ext-foreign-toplevel-list-v1-server.h"
#include <wayland-server.h>
#include <cstdio>
#include <cstring>

struct Window {
    bool minimized = false, maximized = false, fullscreen = false;
};
void state(wl_resource *resource) {
    auto *window = static_cast<Window *>(wl_resource_get_user_data(resource));
    wl_array values;
    wl_array_init(&values);
    auto add = [&](uint32_t value) { *static_cast<uint32_t *>(wl_array_add(&values, sizeof(uint32_t))) = value; };
    if (window->minimized) add(ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED);
    if (window->maximized) add(ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MAXIMIZED);
    if (window->fullscreen) add(ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_FULLSCREEN);
    zwlr_foreign_toplevel_handle_v1_send_state(resource, &values);
    zwlr_foreign_toplevel_handle_v1_send_done(resource);
    wl_array_release(&values);
}
const struct zwlr_foreign_toplevel_handle_v1_interface handleImplementation{
    [](wl_client *, wl_resource *r) { static_cast<Window *>(wl_resource_get_user_data(r))->maximized = true; state(r); },
    [](wl_client *, wl_resource *r) { static_cast<Window *>(wl_resource_get_user_data(r))->maximized = false; state(r); },
    [](wl_client *, wl_resource *r) { static_cast<Window *>(wl_resource_get_user_data(r))->minimized = true; state(r); },
    [](wl_client *, wl_resource *r) { static_cast<Window *>(wl_resource_get_user_data(r))->minimized = false; state(r); },
    [](wl_client *, wl_resource *, wl_resource *) {},
    [](wl_client *, wl_resource *r) { zwlr_foreign_toplevel_handle_v1_send_closed(r); },
    [](wl_client *, wl_resource *, wl_resource *, int32_t, int32_t, int32_t, int32_t) {},
    [](wl_client *, wl_resource *r) { wl_resource_destroy(r); },
    [](wl_client *, wl_resource *r, wl_resource *) { static_cast<Window *>(wl_resource_get_user_data(r))->fullscreen = true; state(r); },
    [](wl_client *, wl_resource *r) { static_cast<Window *>(wl_resource_get_user_data(r))->fullscreen = false; state(r); },
};
const struct zwlr_foreign_toplevel_manager_v1_interface managerImplementation{
    [](wl_client *, wl_resource *r) { zwlr_foreign_toplevel_manager_v1_send_finished(r); }
};
const struct ext_foreign_toplevel_handle_v1_interface extHandleImplementation{
    [](wl_client *, wl_resource *r) { wl_resource_destroy(r); }
};
const struct ext_foreign_toplevel_list_v1_interface extImplementation{
    [](wl_client *, wl_resource *r) { ext_foreign_toplevel_list_v1_send_finished(r); },
    [](wl_client *, wl_resource *r) { wl_resource_destroy(r); }
};

int main(int argc, char **argv)
{
    wl_display *display = wl_display_create();
    if (argc > 1 && std::strcmp(argv[1], "ext") == 0) {
        wl_global_create(display, &ext_foreign_toplevel_list_v1_interface, 1, nullptr,
            [](wl_client *client, void *, uint32_t, uint32_t id) {
            auto *manager = wl_resource_create(client, &ext_foreign_toplevel_list_v1_interface, 1, id);
            wl_resource_set_implementation(manager, &extImplementation, nullptr, nullptr);
            auto *window = wl_resource_create(client, &ext_foreign_toplevel_handle_v1_interface, 1, 0);
            wl_resource_set_implementation(window, &extHandleImplementation, nullptr, nullptr);
            ext_foreign_toplevel_list_v1_send_toplevel(manager, window);
            ext_foreign_toplevel_handle_v1_send_title(window, "Wayland fixture Ω");
            ext_foreign_toplevel_handle_v1_send_app_id(window, "org.xetal.Fixture");
            ext_foreign_toplevel_handle_v1_send_identifier(window, "test-window-1");
            ext_foreign_toplevel_handle_v1_send_done(window);
        });
    } else {
        wl_global_create(display, &zwlr_foreign_toplevel_manager_v1_interface, 3, nullptr,
            [](wl_client *client, void *, uint32_t version, uint32_t id) {
            auto *manager = wl_resource_create(client, &zwlr_foreign_toplevel_manager_v1_interface, version, id);
            wl_resource_set_implementation(manager, &managerImplementation, nullptr, nullptr);
            auto *window = wl_resource_create(client, &zwlr_foreign_toplevel_handle_v1_interface, version, 0);
            wl_resource_set_implementation(window, &handleImplementation, new Window,
                [](wl_resource *r) { delete static_cast<Window *>(wl_resource_get_user_data(r)); });
            zwlr_foreign_toplevel_manager_v1_send_toplevel(manager, window);
            zwlr_foreign_toplevel_handle_v1_send_title(window, "Wayland fixture Ω");
            zwlr_foreign_toplevel_handle_v1_send_app_id(window, "org.xetal.Fixture");
            state(window);
        });
    }
    const char *socket = wl_display_add_socket_auto(display);
    if (!socket) return 1;
    std::puts(socket);
    std::fflush(stdout);
    wl_display_run(display);
    wl_display_destroy_clients(display);
    wl_display_destroy(display);
}
