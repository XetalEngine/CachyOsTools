#include "network_page.h"
#include "network_status.h"
#include <QtTest>
#include <QtWidgets>

namespace {
void fixture(QWidget &root) {
    for (const auto &name : {"hostnameValueLabel", "localIpValueLabel", "dnsValueLabel", "publicIpValueLabel", "wifiStatusLabel", "portsInfoLabel"}) {
        auto *label = new QLabel(&root); label->setObjectName(name);
    }
    root.findChild<QLabel *>("hostnameValueLabel")->setText("CachyOS");
    root.findChild<QLabel *>("localIpValueLabel")->setText("192.168.1.30");
    root.findChild<QLabel *>("dnsValueLabel")->setText("192.168.1.1");
    root.findChild<QLabel *>("publicIpValueLabel")->setText("203.0.113.12");
    root.findChild<QLabel *>("wifiStatusLabel")->setText("3 networks available.");
    for (const auto &name : {"configureInterfaceButton", "upInterfaceButton", "downInterfaceButton", "wifiConnectButton", "wifiScanButton", "wifiRadioToggleButton", "wifiDisconnectButton", "connUpButton", "connDownButton", "connAutoconnectButton", "connDeleteButton", "createBridgeButton", "deleteBridgeButton", "fixBridgeButton", "startLibvirtNetworkButton", "stopLibvirtNetworkButton", "restartLibvirtNetworkButton", "restartNetworkManagerButton"}) {
        auto *button = new QPushButton(name, &root); button->setObjectName(name);
    }
    const QMap<QString, QStringList> columns{
        {"interfaceConfigTable", {"Interface", "Configuration", "IP Address", "Gateway", "Status"}},
        {"wifiTable", {"In Use", "SSID", "Signal", "Security", "Channel", "Rate"}},
        {"connectionsTable", {"Name", "Type", "Device", "Active", "Autoconnect"}},
        {"bridgeTable", {"Bridge Name", "Interfaces", "Status"}},
        {"libvirtNetworkTable", {"Network Name", "Type", "State", "Autostart"}},
        {"ipAddressTable", {"Interface", "IP Address", "Netmask", "Status"}},
        {"portsTable", {"Protocol", "State", "Address", "Port", "PID", "Process"}},
        {"interfaceStatsTable", {"Interface", "RX Bytes", "TX Bytes", "RX Packets", "TX Packets", "RX Errors", "TX Errors", "Speed"}}
    };
    for (auto it = columns.begin(); it != columns.end(); ++it) {
        auto *table = new QTableWidget(0, it.value().size(), &root); table->setObjectName(it.key()); table->setHorizontalHeaderLabels(it.value());
    }
    for (const auto &name : {"firewallSubTab", "networkSharesSubTab"}) {
        auto *page = new QWidget(&root); page->setObjectName(name);
        auto *layout = new QVBoxLayout(page); layout->addWidget(new QLabel(name));
    }
}
void row(QWidget &root, const char *name, const QStringList &values, const QString &uuid = {}) {
    auto *table = root.findChild<QTableWidget *>(name); const int at = table->rowCount(); table->insertRow(at);
    for (int c = 0; c < values.size(); ++c) table->setItem(at, c, new QTableWidgetItem(values[c]));
    if (!uuid.isEmpty()) table->item(at, 0)->setData(Qt::UserRole, uuid);
}
void populated(QWidget &root) {
    row(root, "interfaceConfigTable", {"enp7s0", "Automatic", "192.168.1.30", "192.168.1.1", "Up"});
    row(root, "interfaceConfigTable", {"wlan0", "Automatic", "N/A", "N/A", "Down"});
    row(root, "wifiTable", {"", "Home network", "94%", "WPA2", "36", "540 Mbit/s"});
    row(root, "wifiTable", {"", "Workshop", "71%", "WPA3", "44", "540 Mbit/s"});
    row(root, "wifiTable", {"", "Guest Wi-Fi", "42%", "Open", "6", "270 Mbit/s"});
    row(root, "connectionsTable", {"Wired connection", "Ethernet", "enp7s0", "yes", "yes"}, "wired-uuid");
    row(root, "connectionsTable", {"Home network", "Wi-Fi", "--", "no", "yes"}, "home-uuid");
    row(root, "bridgeTable", {"br0", "enp8s0", "Up"});
    row(root, "libvirtNetworkTable", {"default", "Private (NAT)", "active", "yes"});
    row(root, "ipAddressTable", {"enp7s0", "192.168.1.30", "255.255.255.0", "Up"});
    row(root, "portsTable", {"tcp", "LISTEN", "127.0.0.1", "631", "123", "cupsd"});
    row(root, "interfaceStatsTable", {"enp7s0", "12.4 GB", "1.2 GB", "203241", "112452", "0", "0", "1000 Mb/s"});
}
}

class NetworkPageTests : public QObject {
    Q_OBJECT
private slots:
    void fiveAreasAndLazyRefresh() {
        QWidget owner; fixture(owner); NetworkPage page(&owner, &owner);
        QSignalSpy requests(&page, &NetworkPage::refreshRequested);
        auto *tabs = page.findChild<QTabWidget *>("networkSections"); QCOMPARE(tabs->count(), 5);
        owner.resize(1400, 900); page.resize(1380, 850); owner.show(); page.show();
        QTRY_COMPARE(requests.count(), 1); QCOMPARE(requests.first().first().toString(), QString("Connections"));
        tabs->setCurrentIndex(1); QCOMPARE(requests.last().first().toString(), QString("Virtual networks"));
        tabs->setCurrentIndex(2); QCOMPARE(requests.last().first().toString(), QString("Diagnostics"));
        tabs->setCurrentIndex(3); QCOMPARE(tabs->currentWidget(), owner.findChild<QWidget *>("firewallSubTab"));
        QVERIFY(page.findChild<QPushButton *>("networkRefresh")->isHidden());
        tabs->setCurrentIndex(4); QCOMPARE(tabs->currentWidget(), owner.findChild<QWidget *>("networkSharesSubTab"));
        QCOMPARE(requests.last().first().toString(), QString("Shared folders"));
        QVERIFY(!page.findChild<QPushButton *>("networkRefresh")->isHidden());
    }
    void originalActionsKeepTheirTargetsAndRunOnce() {
        QWidget owner; fixture(owner); populated(owner);
        auto *forget = owner.findChild<QPushButton *>("connDeleteButton"); QSignalSpy clicks(forget, &QPushButton::clicked);
        NetworkPage page(&owner, &owner);
        auto *table = owner.findChild<QTableWidget *>("connectionsTable"); table->selectRow(1); page.sync();
        page.findChild<QAction *>("connDeleteButtonMenuAction")->trigger();
        QCOMPARE(clicks.count(), 1); QCOMPARE(table->item(table->currentRow(), 0)->data(Qt::UserRole).toString(), QString("home-uuid"));
        QCOMPARE(table->item(0, 4)->text(), QString("yes")); // Raw backend values stay intact.
    }
    void profileSelectionSurvivesRefreshAndReorder() {
        QWidget owner; fixture(owner); populated(owner); NetworkPage page(&owner, &owner);
        auto *table = owner.findChild<QTableWidget *>("connectionsTable"); table->selectRow(1); page.sync();
        table->setRowCount(0);
        row(owner, "connectionsTable", {"Home network", "Wi-Fi", "--", "no", "yes"}, "home-uuid");
        row(owner, "connectionsTable", {"Wired connection", "Ethernet", "enp7s0", "yes", "yes"}, "wired-uuid");
        page.sync(); QCOMPARE(table->currentRow(), 0);
        QCOMPARE(table->item(0, 0)->data(Qt::UserRole).toString(), QString("home-uuid"));
    }
    void unavailableAndUnselectedActionsAreDisabled() {
        QWidget owner; fixture(owner);
        owner.findChild<QLabel *>("wifiStatusLabel")->setText("No Wi-Fi adapter detected.");
        row(owner, "libvirtNetworkTable", {"Libvirt not installed", "", "", ""});
        NetworkPage page(&owner, &owner);
        for (const auto &name : {"wifiConnectButton", "wifiScanButton", "wifiRadioToggleButton", "connDeleteButton", "deleteBridgeButton", "configureInterfaceButton", "startLibvirtNetworkButton"})
            QVERIFY2(!owner.findChild<QPushButton *>(name)->isEnabled(), name);
        owner.findChild<QTableWidget *>("libvirtNetworkTable")->selectRow(0); page.sync();
        QVERIFY(!owner.findChild<QPushButton *>("startLibvirtNetworkButton")->isEnabled());
    }
    void savedConnectionsExpandWithoutChangingSettings() {
        QWidget owner; fixture(owner); populated(owner); NetworkPage page(&owner, &owner);
        auto *fold = page.findChild<QToolButton *>("networkFoldout"); auto *body = page.findChild<QWidget *>("networkSavedBody");
        QVERIFY(body->isHidden()); QSignalSpy actions(owner.findChild<QPushButton *>("connUpButton"), &QPushButton::clicked);
        fold->click(); QVERIFY(!body->isHidden()); QCOMPARE(actions.count(), 0);
    }
    void narrowLayoutAndLongNames() {
        QWidget owner; fixture(owner); populated(owner); NetworkPage page(&owner, &owner);
        owner.findChild<QLabel *>("dnsValueLabel")->setText("2001:4860:4860::8888, 2606:4700:4700::1111");
        owner.findChild<QTableWidget *>("wifiTable")->item(0, 1)->setText(QString(32, 'W'));
        auto *layout = new QVBoxLayout(&owner); layout->addWidget(&page); owner.resize(780, 850); owner.show(); page.sync(); QTest::qWait(50);
        QCOMPARE(owner.width(), 780);
        auto *adapters = owner.findChild<QTableWidget *>("interfaceConfigTable")->parentWidget();
        auto *wifi = owner.findChild<QTableWidget *>("wifiTable")->parentWidget();
        QCOMPARE(adapters->x(), wifi->x()); QVERIFY(wifi->y() > adapters->y());
        for (auto *scroll : page.findChildren<QScrollArea *>("networkScroll"))
            if (scroll->isVisible()) QCOMPARE(scroll->horizontalScrollBar()->maximum(), 0);
    }
    void addressingUsesActualProfileMethod() {
        QCOMPARE(NetworkStatus::addressingLabel("auto\n"), QString("Automatic"));
        QCOMPARE(NetworkStatus::addressingLabel("manual"), QString("Manual"));
        QCOMPARE(NetworkStatus::addressingLabel("shared"), QString("Shared"));
        QCOMPARE(NetworkStatus::addressingLabel("disabled"), QString("IPv4 off"));
        QCOMPARE(NetworkStatus::addressingLabel(""), QString("Not reported"));
    }
    void virtualNetworkModeUsesForwardingDefinition() {
        QCOMPARE(NetworkStatus::virtualNetworkMode("<network><bridge name='virbr0'/></network>"), QString("Isolated"));
        QCOMPARE(NetworkStatus::virtualNetworkMode("<network><forward mode='nat'/><bridge name='virbr0'/></network>"), QString("Private (NAT)"));
        QCOMPARE(NetworkStatus::virtualNetworkMode("<network><forward mode='bridge'/></network>"), QString("Bridge"));
        QCOMPARE(NetworkStatus::virtualNetworkMode("<network><forward mode='route'/></network>"), QString("Routed"));
        QCOMPARE(NetworkStatus::virtualNetworkMode("<network><forward/></network>"), QString("Private (NAT)"));
        QCOMPARE(NetworkStatus::virtualNetworkMode("<network>"), QString("Not reported"));
    }
    void preview() {
        const auto directory = qEnvironmentVariable("CACHY_NETWORK_PREVIEW");
        if (directory.isEmpty()) QSKIP("Optional visual preview");
        QFile input(qEnvironmentVariable("CACHY_NETWORK_THEME"));
        if (input.open(QIODevice::ReadOnly)) {
            const auto data = QJsonDocument::fromJson(input.readAll()).object(); QPalette palette;
            const auto roles = QMetaEnum::fromType<QPalette::ColorRole>(); const auto colors = data["colors"].toObject();
            for (auto i = colors.begin(); i != colors.end(); ++i) {
                const int role = roles.keyToValue(i.key().toUtf8());
                if (role >= 0) palette.setColor(QPalette::All, QPalette::ColorRole(role), QColor(i.value().toString()));
            }
            qApp->setPalette(palette); qApp->setFont(QFont(data["font"].toString(), 10)); qApp->setStyleSheet(data["stylesheet"].toString());
            QStringList paths; for (const auto &path : data["iconSearchPaths"].toArray()) paths << path.toString();
            QIcon::setThemeSearchPaths(paths); QIcon::setThemeName(data["iconTheme"].toString()); QIcon::setFallbackThemeName("hicolor");
        }
        QWidget owner; fixture(owner); populated(owner); auto *layout = new QVBoxLayout(&owner);
        NetworkPage page(&owner, &owner); layout->addWidget(&page); owner.resize(1430, 930); owner.show(); QTest::qWait(80);
        QDir().mkpath(directory); QVERIFY(owner.grab().save(directory+"/connections.png"));
        page.findChild<QToolButton *>("networkFoldout")->click(); QTest::qWait(30); QVERIFY(owner.grab().save(directory+"/saved.png"));
        auto *tabs = page.findChild<QTabWidget *>("networkSections"); tabs->setCurrentIndex(1); QTest::qWait(30); QVERIFY(owner.grab().save(directory+"/virtual.png"));
        tabs->setCurrentIndex(2); QTest::qWait(30); QVERIFY(owner.grab().save(directory+"/diagnostics.png"));
        tabs->setCurrentIndex(0); owner.resize(780, 850); QTest::qWait(30); QVERIFY(owner.grab().save(directory+"/narrow.png"));
    }
};
QTEST_MAIN(NetworkPageTests)
#include "network_page_tests.moc"
