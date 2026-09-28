#include "window_enum.h"
#include "window_enum_backend.h"
#include <QApplication>
#include <QComboBox>
#include <QFileInfo>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QSignalSpy>
#include <QSpinBox>
#include <QTableView>
#include <QtTest>
#include <xcb/xcb.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

class WindowEnumTests : public QObject {
    Q_OBJECT
    xcb_connection_t *connection = nullptr;
    xcb_screen_t *screen = nullptr;
    xcb_window_t parent = 0, child = 0, hidden = 0, overlay = 0, input = 0;
    QString id(xcb_window_t value) const { return QString("0x%1").arg(value, 8, 16, QLatin1Char('0')); }
    xcb_atom_t atom(const char *name) {
        auto *reply = xcb_intern_atom_reply(connection, xcb_intern_atom(connection, false, std::strlen(name), name), nullptr);
        const auto result = reply->atom;
        std::free(reply);
        return result;
    }
    void text(xcb_window_t window, const char *property, const QByteArray &value, xcb_atom_t type = XCB_ATOM_STRING) {
        xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, atom(property), type, 8, value.size(), value.constData());
    }
    void sync() { std::free(xcb_get_input_focus_reply(connection, xcb_get_input_focus(connection), nullptr)); }
    xcb_window_t create(xcb_window_t owner, bool mapped, bool overrideRedirect = false, bool inputOnly = false) {
        const auto window = xcb_generate_id(connection);
        const uint32_t value = overrideRedirect;
        xcb_create_window(connection, XCB_COPY_FROM_PARENT, window, owner, 30, 40, 200, 120, 0,
            inputOnly ? XCB_WINDOW_CLASS_INPUT_ONLY : XCB_WINDOW_CLASS_INPUT_OUTPUT,
            XCB_COPY_FROM_PARENT, XCB_CW_OVERRIDE_REDIRECT, &value);
        if (mapped) xcb_map_window(connection, window);
        return window;
    }
    QJsonObject find(const QJsonObject &snapshot, const QString &key) const {
        for (const auto &row : snapshot["windows"].toArray())
            if (row.toObject()["id"] == key) return row.toObject();
        return {};
    }
    QJsonObject action(xcb_window_t target, const QString &name, QJsonObject args = {}) {
        args["id"] = id(target); args["action"] = name; args["pid"] = int(getpid());
        return windowEnumX11(args);
    }
private slots:
    void initTestCase() {
        QVERIFY2(qEnvironmentVariable("WINDOW_ENUM_ISOLATED") == "1", "Run with tests/run_window_enum_tests.py, never against a personal desktop.");
        connection = xcb_connect(nullptr, nullptr);
        QVERIFY(!xcb_connection_has_error(connection));
        screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
        parent = create(screen->root, true);
        child = create(parent, true);
        hidden = create(screen->root, false);
        overlay = create(screen->root, true, true);
        input = create(screen->root, true, true, true);
        text(parent, "_NET_WM_NAME", QString::fromUtf8("Game — Vulkan / 窗口 $(touch NEVER)").toUtf8(), atom("UTF8_STRING"));
        text(parent, "WM_CLASS", QByteArray("game\0GameClass\0", 15));
        text(hidden, "_NET_WM_NAME", "Hidden tool", atom("UTF8_STRING"));
        text(overlay, "_NET_WM_NAME", "Overlay", atom("UTF8_STRING"));
        const uint32_t falsePid = 999999;
        xcb_change_property(connection, XCB_PROP_MODE_REPLACE, parent, atom("_NET_WM_PID"), XCB_ATOM_CARDINAL, 32, 1, &falsePid);
        const uint32_t protocol = atom("WM_DELETE_WINDOW");
        xcb_change_property(connection, XCB_PROP_MODE_REPLACE, parent, atom("WM_PROTOCOLS"), XCB_ATOM_ATOM, 32, 1, &protocol);
        sync();
    }
    void validatesRequests() {
        QString error;
        QVERIFY(!windowEnumValidateAction({{"id", "1"}, {"action", "destroy"}}, &error));
        QVERIFY(!windowEnumValidateAction({{"id", "1"}, {"action", "above"}, {"value", "true"}}, &error));
        QVERIFY(!windowEnumValidateAction({{"id", "1"}, {"action", "opacity"}, {"value", 101}}, &error));
        QVERIFY(!windowEnumValidateAction({{"id", "1"}, {"action", "geometry"}, {"x", 0}, {"y", 0}, {"width", -1}, {"height", 50}}, &error));
        QVERIFY(windowEnumValidateAction({{"id", "1"}, {"action", "geometry"}, {"x", -100}, {"y", 0}, {"width", 500}, {"height", 400}}, &error));
    }
    void enumeratesEveryX11KindAndRealOwnership() {
        const auto snapshot = windowEnumX11({{"action", "list"}});
        QVERIFY2(!snapshot.contains("error"), qPrintable(windowEnumCell(snapshot["error"])));
        for (auto window : {screen->root, parent, child, hidden, overlay, input})
            QVERIFY2(!find(snapshot, id(window)).isEmpty(), qPrintable(id(window)));
        const auto game = find(snapshot, id(parent));
        QCOMPARE(game["title"].toString(), QString::fromUtf8("Game — Vulkan / 窗口 $(touch NEVER)"));
        QCOMPARE(game["class"].toString(), QString("GameClass"));
        QCOMPARE(game["pid"].toInt(), int(getpid())); // XRes overrides a false client-supplied PID.
        QCOMPARE(game["executable"].toString(), QFileInfo("/proc/self/exe").symLinkTarget());
        QCOMPARE(find(snapshot, id(child))["parent"].toString(), id(parent));
        QVERIFY(!find(snapshot, id(hidden))["mapped"].toBool());
        QVERIFY(find(snapshot, id(overlay))["overrideRedirect"].toBool());
        QVERIFY(find(snapshot, id(input))["inputOnly"].toBool());
    }
    void movesAndResizesChildrenInRootCoordinates() {
        auto result = action(child, "geometry", {{"x", 190}, {"y", 180}, {"width", 320}, {"height", 240}});
        QVERIFY2(result["ok"].toBool(), qPrintable(windowEnumCell(result)));
        auto row = find(windowEnumX11({{"action", "list"}}), id(child));
        QCOMPARE(row["x"].toInt(), 190); QCOMPARE(row["y"].toInt(), 180);
        QCOMPARE(row["width"].toInt(), 320); QCOMPARE(row["height"].toInt(), 240);
        result = action(child, "geometry", {{"x", 220}, {"y", 210}, {"width", 1}, {"height", 1}, {"resize", false}});
        QVERIFY(result["ok"].toBool());
        row = find(windowEnumX11({{"action", "list"}}), id(child));
        QCOMPARE(row["x"].toInt(), 220); QCOMPARE(row["width"].toInt(), 320);
    }
    void editsTitleOpacityAndReadsRawProperties() {
        const QString title = QString::fromUtf8("Literal \"title\"; $(false) — Ω");
        QVERIFY(action(overlay, "title", {{"value", title}})["ok"].toBool());
        QVERIFY(action(overlay, "opacity", {{"value", 42}})["ok"].toBool());
        auto result = windowEnumX11({{"action", "details"}, {"id", id(overlay)}});
        auto row = result["window"].toObject();
        QCOMPARE(row["title"].toString(), title);
        QCOMPARE(row["opacity"].toInt(), 42);
        QCOMPARE(row["properties"].toObject()["_NET_WM_NAME"].toObject()["value"].toString(), title);
    }
    void rejectsInvalidTargetsAndOwnershipChanges() {
        QVERIFY(action(screen->root, "close").contains("error"));
        QVERIFY(action(parent, "opacity", {{"value", -1}}).contains("error"));
        QVERIFY(windowEnumX11({{"id", id(parent)}, {"pid", 999999}, {"action", "title"}, {"value", "wrong"}}).contains("error"));
        const auto gone = create(screen->root, false);
        xcb_destroy_window(connection, gone); sync();
        QVERIFY(action(gone, "geometry", {{"x", 0}, {"y", 0}, {"width", 50}, {"height", 50}}).contains("error"));
    }
    void closeUsesGracefulWindowProtocol() {
        while (auto *event = xcb_poll_for_event(connection)) std::free(event);
        QVERIFY(action(parent, "close")["ok"].toBool());
        sync();
        auto *event = xcb_poll_for_event(connection);
        QVERIFY(event);
        QCOMPARE(event->response_type & 0x7f, XCB_CLIENT_MESSAGE);
        const auto *message = reinterpret_cast<xcb_client_message_event_t *>(event);
        QCOMPARE(message->data.data32[0], atom("WM_DELETE_WINDOW"));
        std::free(event);
        QVERIFY(!find(windowEnumX11({{"action", "list"}}), id(child)).isEmpty());
    }
    void tableDisplaysAndFiltersRealRows() {
        WindowEnumTab tab;
        tab.resize(1560, 760);
        tab.findChild<QComboBox *>()->setCurrentIndex(1);
        tab.show();
        auto *table = tab.findChild<QTableView *>();
        QTRY_VERIFY_WITH_TIMEOUT(table->model()->rowCount() >= 6, 10000);
        auto *filter = tab.findChild<QLineEdit *>();
        filter->setText("GameClass");
        QTRY_COMPARE(table->model()->rowCount(), 1);
        QCOMPARE(table->model()->index(0, 3).data().toString(), QString::number(getpid()));
        table->selectRow(0);
        const QString selectedWindow = table->model()->index(0, 15).data().toString();
        auto *position = tab.findChildren<QSpinBox *>().first();
        position->setValue(99);
        auto *controller = tab.findChild<WindowEnumController *>();
        QSignalSpy refreshed(controller, &WindowEnumController::result);
        controller->request({{"action", "list"}});
        QTRY_VERIFY_WITH_TIMEOUT(!refreshed.isEmpty(), 10000);
        QCOMPARE(position->value(), 99); // Live updates must not discard a pending edit.
        QCOMPARE(table->model()->index(table->currentIndex().row(), 15).data().toString(), selectedWindow);
        filter->clear();
        if (!qEnvironmentVariableIsEmpty("WINDOW_ENUM_SCREENSHOT"))
            QVERIFY(tab.grab().save(qEnvironmentVariable("WINDOW_ENUM_SCREENSHOT")));
    }
    void kwinWaylandIntegration() {
        if (qEnvironmentVariable("WINDOW_ENUM_KWIN_TEST") != "1") QSKIP("Optional nested KWin session.");
        QWidget window;
        window.setWindowTitle("Window Enum native Wayland fixture");
        window.resize(440, 300);
        window.show();
        QTest::qWait(400);
        WindowEnumController controller;
        controller.setBackend("kwin");
        QSignalSpy spy(&controller, &WindowEnumController::result);
        controller.request({{"action", "list"}});
        QTRY_VERIFY_WITH_TIMEOUT(!spy.isEmpty(), 10000);
        auto snapshot = spy.takeFirst()[0].toJsonObject();
        QVERIFY2(!snapshot.contains("error"), qPrintable(windowEnumCell(snapshot)));
        QJsonObject row;
        for (auto value : snapshot["windows"].toArray())
            if (value.toObject()["title"] == window.windowTitle()) row = value.toObject();
        QVERIFY2(!row.isEmpty(), qPrintable(windowEnumCell(snapshot)));
        QCOMPARE(row["pid"].toInt(), int(getpid()));
        QCOMPARE(row["executable"].toString(), QFileInfo("/proc/self/exe").symLinkTarget());
        auto request = row; request["action"] = "above"; request["value"] = true;
        controller.request(request);
        QTRY_VERIFY_WITH_TIMEOUT(!spy.isEmpty(), 10000);
        QVERIFY2(spy.first()[0].toJsonObject()["ok"].toBool(), qPrintable(windowEnumCell(spy.first()[0].toJsonObject())));
        spy.clear();
        controller.request({{"action", "list"}});
        QTRY_VERIFY_WITH_TIMEOUT(!spy.isEmpty(), 10000);
        snapshot = spy.takeFirst()[0].toJsonObject();
        QVERIFY(find(snapshot, row["id"].toString())["above"].toBool());
        request = row; request["action"] = "geometry"; request["x"] = 110; request["y"] = 120;
        request["width"] = 550; request["height"] = 370;
        controller.request(request);
        QTRY_VERIFY_WITH_TIMEOUT(!spy.isEmpty(), 10000);
        QVERIFY2(spy.first()[0].toJsonObject()["ok"].toBool(), qPrintable(windowEnumCell(spy.first()[0].toJsonObject())));
        spy.clear(); QTest::qWait(200);
        controller.request({{"action", "list"}});
        QTRY_VERIFY_WITH_TIMEOUT(!spy.isEmpty(), 10000);
        row = find(spy.takeFirst()[0].toJsonObject(), row["id"].toString());
        QCOMPARE(row["x"].toInt(), 110); QCOMPARE(row["y"].toInt(), 120);
        QCOMPARE(row["width"].toInt(), 550); QCOMPARE(row["height"].toInt(), 370);
    }
    void x11WindowManagerRequests() {
        if (qEnvironmentVariable("WINDOW_ENUM_WM_TEST") != "1") QSKIP("Optional nested X11 window manager.");
        QWidget window;
        window.setWindowTitle("Window Enum managed X11 fixture");
        window.resize(440, 300);
        window.show();
        QTest::qWait(500);
        QJsonObject row;
        const auto snapshot = windowEnumX11({{"action", "list"}});
        for (auto value : snapshot["windows"].toArray())
            if (value.toObject()["title"] == window.windowTitle()) row = value.toObject();
        QVERIFY(!row.isEmpty());
        QVERIFY(row["managed"].toBool());
        QVERIFY2(row["capabilities"].toArray().contains("above"), qPrintable(windowEnumCell(row)));
        auto request = row; request["action"] = "above"; request["value"] = true;
        QVERIFY(windowEnumX11(request)["ok"].toBool());
        QTest::qWait(150);
        QVERIFY(find(windowEnumX11({{"action", "list"}}), row["id"].toString())["above"].toBool());
        request = row; request["action"] = "geometry";
        request["x"] = 120; request["y"] = 140; request["width"] = 600; request["height"] = 350;
        QVERIFY(windowEnumX11(request)["ok"].toBool());
        QTest::qWait(250);
        row = find(windowEnumX11({{"action", "list"}}), row["id"].toString());
        QCOMPARE(row["x"].toInt(), 120); QCOMPARE(row["y"].toInt(), 140);
        QCOMPARE(row["width"].toInt(), 600); QCOMPARE(row["height"].toInt(), 350);
        request = row; request["action"] = "minimized"; request["value"] = true;
        QVERIFY(windowEnumX11(request)["ok"].toBool());
        QTest::qWait(150);
        QVERIFY(find(windowEnumX11({{"action", "list"}}), row["id"].toString())["minimized"].toBool());
    }
    void cleanupTestCase() { xcb_disconnect(connection); }
};

int main(int argc, char **argv)
{
    if (argc > 1 && std::strcmp(argv[1], "--window-enum-x11") == 0) {
        QCoreApplication app(argc, argv);
        const auto result = windowEnumX11(QJsonDocument::fromJson(argv[2]).object());
        const auto bytes = QJsonDocument(result).toJson(QJsonDocument::Compact);
        std::fwrite(bytes.constData(), 1, size_t(bytes.size()), stdout);
        return result.contains("error") ? 1 : 0;
    }
    QApplication app(argc, argv);
    WindowEnumTests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "window_enum_tests.moc"
