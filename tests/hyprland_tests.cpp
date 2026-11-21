#include "window_enum_hyprland.h"
#include "window_enum.h"

#include <QtTest>
#include <QtWidgets>
#include <unistd.h>

class HyprlandTests final : public QObject {
    Q_OBJECT
    QJsonArray clients;
    QJsonObject metadata, layers, surfaces;
    QList<QStringList> commands;
    bool unavailable = false, malformed = false, oldAdapter = false, lua = false, legacyClients = false;

    QJsonObject snapshot(QJsonObject request = {{"action","list"}}) {
        return windowEnumHyprland(request, [this](const QStringList &args) -> HyprlandReply {
            commands.append(args);
            if (unavailable) return {{},"Disconnected"};
            if (args.first() == "eval") return lua ? HyprlandReply{"ok", {}} : HyprlandReply{{}, "unknown request"};
            if (args.first() == "dispatch") return {"ok", {}};
            QJsonDocument reply;
            if (args.last() == "clients") {
                if (legacyClients && args.contains("-a")) return {{}, "unknown flag -a"};
                if (malformed) return {"not json",{}};
                reply = QJsonDocument(clients);
            } else if (args.last() == "layers") reply = QJsonDocument(layers);
            else if (args.last() == "monitors") reply = QJsonDocument(QJsonArray{QJsonObject{{"id",2},{"name","DP-3"}}});
            else if (args.last() == "activewindow") reply = QJsonDocument(QJsonObject{{"address","0x12"}});
            else if (args.last() == "xetal-windows") reply = QJsonDocument(metadata);
            else if (args.last() == "xetal-inspector-surfaces") {
                if (oldAdapter) return {"unknown request",{}};
                reply = QJsonDocument(surfaces);
            } else return {{},"Unexpected test request"};
            return {reply.toJson(QJsonDocument::Compact), {}};
        });
    }
private slots:
    void init() {
        commands.clear(); unavailable = malformed = oldAdapter = lua = legacyClients = false;
        clients = {QJsonObject{{"address","0x12"},{"stableId","a1"},{"pid",int(getpid())},
            {"title","Files $(literal)"},{"class","files"},{"mapped",true},{"hidden",false},
            {"visible",true},{"floating",true},{"xwayland",false},{"at",QJsonArray{-1500,40}},
            {"size",QJsonArray{900,650}},{"workspace",QJsonObject{{"id",3},{"name","3"}}},{"monitor",2}}};
        metadata = {{"0x12",QJsonObject{{"geometry_managed",true}}}};
        const QJsonObject layer{{"address","0x20"},{"namespace","waybar"},{"pid",int(getpid())},
            {"x",-1920},{"y",0},{"w",1920},{"h",30},{"alpha",1}};
        const QJsonObject levels{{"3",QJsonArray{layer}}};
        layers = {{"DP-3",QJsonObject{{"levels",levels}}}};
        surfaces = {{"version",1},{"surfaces",QJsonArray{
            QJsonObject{{"address","0x30"},{"role","popup"},{"pid",int(getpid())},{"mapped",true},{"visible",true},
                {"x",-1300},{"y",60},{"width",180},{"height",200},{"ownerType","window"},{"ownerAddress","0x12"}},
            QJsonObject{{"address","0x31"},{"role","subsurface"},{"pid",int(getpid())},{"mapped",true},{"visible",true},
                {"x",-1400},{"y",75},{"width",400},{"height",300}}}}};
    }
    void automaticBackendUsesTheRunningCompositor() {
        QCOMPARE(windowEnumAutomaticBackend("wayland","wayland-1","Hyprland","session"),QString("hyprland"));
        QCOMPARE(windowEnumAutomaticBackend("wayland","wayland-1","KDE","session"),QString("hyprland"));
        QCOMPARE(windowEnumAutomaticBackend("wayland","wayland-1","KDE",""),QString("kwin"));
        QCOMPARE(windowEnumAutomaticBackend("x11","","KDE",""),QString("x11"));
        QCOMPARE(windowEnumAutomaticBackend("x11","","Hyprland","inherited-parent"),QString("x11"));
        QCOMPARE(windowEnumAutomaticBackend("wayland","wayland-1","GNOME",""),QString("gnome"));
        QCOMPARE(windowEnumAutomaticBackend("wayland","wayland-1","sway",""),QString("wayland"));
    }
    void includesAllWorkspacesLayersAndUnnamedChildSurfaces() {
        auto result = snapshot(); QVERIFY(!result.contains("error"));
        const auto rows = result["windows"].toArray(); QCOMPARE(rows.size(),4);
        const auto window = rows[0].toObject();
        QCOMPARE(window["x"].toInt(),-1500); QCOMPARE(window["desktop"].toInt(),2);
        QCOMPARE(window["monitor"].toString(),QString("DP-3"));
        QCOMPARE(window["executable"].toString(),QFileInfo("/proc/self/exe").symLinkTarget());
        QVERIFY(window["capabilities"].toArray().contains("move")); QVERIFY(window["active"].toBool());
        QVERIFY(!rows[1].toObject().contains("title")); QVERIFY(rows[1].toObject()["above"].toBool());
        QCOMPARE(rows[2].toObject()["parent"],window["id"]);
        QVERIFY(rows[2].toObject()["capabilities"].toArray().isEmpty());
        QVERIFY(rows[3].toObject()["class"].toString().isEmpty());
        QVERIFY(commands.contains(QStringList{"-j","-a","clients"}));
        for (const auto &command : commands) QVERIFY(command.first() != "dispatch");
    }
    void currentTableShowsUnnamedLayersAndStillOffersExplicitFilters() {
        const auto result = snapshot();
        WindowEnumTab tab;
        auto *controller = tab.findChild<WindowEnumController *>();
        QVERIFY(QMetaObject::invokeMethod(controller,"result",Qt::DirectConnection,Q_ARG(QJsonObject,result)));
        auto *table = tab.findChild<QTableView *>(); QCOMPARE(table->model()->rowCount(),4);
        auto *names = tab.findChild<QCheckBox *>("windowEnumHideEmptyWindowName");
        auto *classes = tab.findChild<QCheckBox *>("windowEnumHideEmptyClass");
        QVERIFY(!names->isChecked()); QVERIFY(!classes->isChecked());
        names->setChecked(true); QCOMPARE(table->model()->rowCount(),1);
        names->setChecked(false); classes->setChecked(true); QCOMPARE(table->model()->rowCount(),3);
        classes->setChecked(false); QCOMPARE(table->model()->rowCount(),4);
        const QJsonObject failure{{"error","Disconnected"}};
        QVERIFY(QMetaObject::invokeMethod(controller,"result",Qt::DirectConnection,Q_ARG(QJsonObject,failure)));
        QCOMPARE(table->model()->rowCount(),0);
    }
    void stockAndOlderHyprlandDoNotRequireExtensions() {
        legacyClients = oldAdapter = true; metadata = {};
        auto raw = clients[0].toObject(); raw["xwayland"] = true; raw.remove("stableId"); clients[0] = raw;
        const auto result = snapshot(); QVERIFY(!result.contains("error"));
        QCOMPARE(result["windows"].toArray().size(), 2);
        const auto window = result["windows"].toArray()[0].toObject();
        QCOMPARE(window["type"].toString(), QString("XWayland window"));
        QVERIFY(!window.contains("managed")); QVERIFY(!window.contains("above"));
        QVERIFY(window["capabilities"].toArray().isEmpty());
        QVERIFY(commands.contains(QStringList{"-j", "clients"}));
    }
    void targetedGeometryUsesAddressesAndRejectsStaleWindows() {
        const QString id = snapshot()["windows"].toArray()[0].toObject()["id"].toString();
        commands.clear();
        const QJsonObject move{{"action","geometry"},{"id",id},{"pid",int(getpid())},
            {"x",-1200},{"y",80},{"width",700},{"height",500}};
        QVERIFY(snapshot(move)["ok"].toBool());
        QCOMPARE(commands[commands.size()-2],(QStringList{"dispatch","resizewindowpixel","exact 700 500,address:0x12"}));
        QCOMPARE(commands.last(),(QStringList{"dispatch","movewindowpixel","exact -1200 80,address:0x12"}));
        auto replaced = clients[0].toObject(); replaced["stableId"] = "a2"; clients[0] = replaced;
        commands.clear(); QVERIFY(snapshot(move).contains("error"));
        for (const auto &command : commands) QVERIFY(command.first() != "dispatch");
    }
    void popupsTiledWindowsAndChangedOwnersCannotBeMoved() {
        auto rows = snapshot()["windows"].toArray();
        QJsonObject request{{"action","close"},{"id",rows[2].toObject()["id"]}};
        QVERIFY(snapshot(request).contains("error"));
        request = {{"action","activate"},{"id",rows[0].toObject()["id"]},{"pid",-123}};
        QVERIFY(snapshot(request).contains("error"));
        auto tiled = clients[0].toObject(); tiled["floating"] = false; clients[0] = tiled;
        request = {{"action","geometry"},{"id",rows[0].toObject()["id"]},{"x",0},{"y",0},{"width",500},{"height",300}};
        QVERIFY(snapshot(request).contains("error"));
        request["action"] = "activate";
        QVERIFY(snapshot(request)["ok"].toBool());
        QCOMPARE(commands.last(),(QStringList{"dispatch","focuswindow","address:0x12"}));
    }
    void newerLuaDispatchersRemainAddressedAndLiteral() {
        lua = true;
        const auto id = snapshot()["windows"].toArray()[0].toObject()["id"].toString();
        QVERIFY(snapshot({{"action","geometry"},{"id",id},{"x",-720},{"y",15},{"width",640},{"height",480}})["ok"].toBool());
        QCOMPARE(commands.last(), (QStringList{"dispatch", "hl.dsp.window.move({window='address:0x12',x=-720,y=15,relative=false})"}));
        QVERIFY(snapshot({{"action","raise"},{"id",id}})["ok"].toBool());
        QCOMPARE(commands.last(), (QStringList{"dispatch", "hl.dsp.window.alter_zorder({window='address:0x12',mode='top'})"}));
        QVERIFY(snapshot({{"action","close"},{"id",id}})["ok"].toBool());
        QCOMPARE(commands.last(), (QStringList{"dispatch", "hl.dsp.window.close({window='address:0x12'})"}));
    }
    void failedAndOlderCollectorsAreReportedWithoutInventingData() {
        unavailable = true; QVERIFY(snapshot().contains("error"));
        unavailable = false; malformed = true; QVERIFY(snapshot().contains("error"));
        malformed = false; oldAdapter = true;
        auto result = snapshot(); QCOMPARE(result["windows"].toArray().size(),2);
        QVERIFY(result["coverage"].toString().contains("inspection extension"));
        QVERIFY(snapshot({{"action","close"},{"id","hyprland:window:0x12; exec anything:a1"}}).contains("error"));
    }

};
QTEST_MAIN(HyprlandTests)
#include "hyprland_tests.moc"
