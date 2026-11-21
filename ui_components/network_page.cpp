#include "network_page.h"
#include "panel_widgets.h"
#include <QtWidgets>
#include <algorithm>

namespace {
template<class T> T *take(QWidget *root, const char *name) {
    auto *widget = root->findChild<T *>(name);
    if (!widget) qFatal("Network page is missing its backend widget: %s", name);
    return widget;
}
QLabel *label(const QString &text, const char *name = "networkDescription") {
    auto *result = new QLabel(text); result->setObjectName(name);
    result->setTextFormat(Qt::PlainText); result->setWordWrap(true);
    return result;
}
QString cell(QTableWidget *table, int row, int column) {
    auto *item = table->item(row, column); return item ? item->text() : QString();
}
class NetworkDelegate : public QStyledItemDelegate {
    QTableWidget *table;
public:
    explicit NetworkDelegate(QTableWidget *table) : QStyledItemDelegate(table), table(table) {}
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override {
        QStyledItemDelegate::initStyleOption(option, index);
        const auto name = table->objectName(); const auto value = index.data().toString();
        if (index.column() == (name == "wifiTable" ? 1 : 0)) option->font.setBold(true);
        const bool link = name == "interfaceConfigTable" && index.column() == 4;
        const bool active = name == "connectionsTable" && index.column() == 3;
        const bool automatic = name == "connectionsTable" && index.column() == 4;
        if (link || active || automatic) {
            const bool on = value == "Up" || value == "yes";
            option->text = link ? (on ? "Up" : "Down") : active ? (on ? "Active" : "Saved") : (on ? "On" : "Off");
            option->font.setBold(true);
            option->palette.setColor(QPalette::Text, table->palette().color(on ? QPalette::Link : QPalette::PlaceholderText));
        }
    }
};
}

NetworkPage::NetworkPage(QWidget *legacy, QWidget *parent) : QWidget(parent), legacy(legacy) {
    setObjectName("networkPage");
    setStyleSheet(
        "QWidget#networkPage {background:transparent;}"
        "QFrame#networkCard {background:palette(alternate-base);border:1px solid palette(mid);border-radius:10px;}"
        "QFrame#networkCard QLabel {border:0;background:transparent;}"
        "QLabel#networkHeading {font-size:26px;font-weight:650;}"
        "QLabel#networkCardTitle {font-size:16px;font-weight:600;}"
        "QLabel#networkDescription,QLabel#networkEmpty {font-size:12px;}"
        "QLabel#networkEmpty {padding:14px 0;}"
        "QLabel[networkMetric=\"true\"] {font-size:15px;font-weight:600;}"
        "QLabel#networkSummary {color:palette(link);}"
        "QPushButton[networkPrimary=\"true\"]:enabled {background:palette(highlight);color:palette(highlighted-text);border:1px solid palette(highlight);font-weight:600;}"
        "QTableWidget[networkList=\"true\"] {background:transparent;alternate-background-color:transparent;border:0;}"
        "QTableWidget[networkList=\"true\"]::item {padding:4px 6px;border:0;}"
        "QTableWidget[networkList=\"true\"] QHeaderView::section {background:transparent;border:0;border-bottom:1px solid palette(mid);padding:6px;text-align:left;}"
        "QScrollArea#networkScroll {background:transparent;border:0;}"
        "QWidget#networkCanvas {background:transparent;}"
        "QToolButton#networkFoldout {background:transparent;border:0;text-align:left;padding:4px 0;font-size:15px;font-weight:600;}");
    auto *outer = new QVBoxLayout(this); outer->setContentsMargins(0, 0, 0, 0); outer->setSpacing(12);
    auto *heading = new QHBoxLayout;
    auto *titles = new QVBoxLayout; titles->setSpacing(4);
    titles->addWidget(label("Network", "networkHeading"));
    summary = label("Connections, Wi-Fi and network tools in one place.", "networkSummary");
    titles->addWidget(summary); heading->addLayout(titles, 1);
    auto *refresh = new QPushButton(QIcon::fromTheme("view-refresh"), "Refresh"); refresh->setObjectName("networkRefresh");
    connect(refresh, &QPushButton::clicked, this, &NetworkPage::requestRefresh); heading->addWidget(refresh);
    outer->addLayout(heading);

    sections = new PanelTabs(this); sections->setObjectName("networkSections");
    sections->setDocumentMode(true); outer->addWidget(sections, 1);
    QMap<QString, QVBoxLayout *> pages;
    for (const auto &name : {"Connections", "Virtual networks", "Diagnostics"}) {
        auto *scroll = new QScrollArea; scroll->setObjectName("networkScroll"); scroll->setWidgetResizable(true);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        auto *canvas = new QWidget; canvas->setObjectName("networkCanvas");
        auto *layout = new QVBoxLayout(canvas); layout->setContentsMargins(0, 14, 4, 8); layout->setSpacing(12);
        pages[name] = layout; scroll->setWidget(canvas); sections->addTab(scroll, name);
    }

    auto *connections = pages["Connections"];
    metrics = new QGridLayout; metrics->setSpacing(12);
    for (const auto &entry : {QPair<const char *, const char *>{"hostnameValueLabel", "This computer"}, {"localIpValueLabel", "Local IPv4 address"}, {"dnsValueLabel", "DNS servers"}}) {
        auto *box = new QFrame; box->setObjectName("networkCard");
        auto *layout = new QVBoxLayout(box); layout->setContentsMargins(14, 10, 14, 10); layout->setSpacing(4);
        layout->addWidget(label(entry.second));
        auto *value = take<QLabel>(legacy, entry.first); value->setProperty("networkMetric", true);
        // Keep the original Ui pointer alive; only the presentation changes.
        value->setTextFormat(Qt::PlainText); value->setWordWrap(true); value->setTextInteractionFlags(Qt::TextSelectableByMouse);
        value->setMinimumWidth(0); value->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        layout->addWidget(value); metricCards.append(box);
    }
    for (int i = 0; i < metricCards.size(); ++i) { metrics->addWidget(metricCards[i], 0, i); metrics->setColumnStretch(i, 1); }
    connections->addLayout(metrics);
    auto *connectionGrid = new QGridLayout; connectionGrid->setSpacing(12); connections->addLayout(connectionGrid);
    auto *adapters = card("Network adapters", "Select an adapter to view or change its IP and DNS settings.", "network-wired");
    list(adapters, "interfaceConfigTable", "No network adapters are available.", 5);
    content(adapters)->addStretch();
    adapterDetail = label("Select an adapter to see its gateway."); content(adapters)->addWidget(adapterDetail);
    auto *configure = button("configureInterfaceButton", "IP && DNS…", "configure", true);
    auto *up = button("upInterfaceButton", "Enable adapter", "network-connect");
    auto *down = button("downInterfaceButton", "Disable adapter", "network-disconnect");
    actions(adapters, {configure}); menu(adapters, {up, down});
    lists["interfaceConfigTable"].selectionButtons = {configure, up, down};
    auto *wifi = card("Wi-Fi", "Choose a nearby network to connect. Saved passwords can be reused.", "network-wireless");
    list(wifi, "wifiTable", "No nearby networks listed. Scan to look again.", 5);
    content(wifi)->addStretch();
    auto *wifiStatus = take<QLabel>(legacy, "wifiStatusLabel"); wifiStatus->setTextFormat(Qt::PlainText); wifiStatus->setWordWrap(true);
    content(wifi)->addWidget(wifiStatus);
    auto *wifiConnect = button("wifiConnectButton", "Connect", "network-connect", true);
    auto *scan = button("wifiScanButton", "Scan", "view-refresh");
    actions(wifi, {wifiConnect, scan});
    menu(wifi, {button("wifiRadioToggleButton", "Turn Wi-Fi off", "network-wireless"), button("wifiDisconnectButton", "Disconnect Wi-Fi", "network-disconnect")});
    lists["wifiTable"].selectionButtons = {wifiConnect};
    responsive.append({connectionGrid, {adapters, wifi}});

    auto *saved = card({}, {}, "");
    auto *savedHeader = new QHBoxLayout;
    auto *fold = new QToolButton; fold->setObjectName("networkFoldout"); fold->setCheckable(true);
    fold->setToolButtonStyle(Qt::ToolButtonTextBesideIcon); fold->setArrowType(Qt::RightArrow); savedToggle = fold;
    savedHeader->addWidget(fold, 0, Qt::AlignLeft); savedHeader->addStretch(); content(saved)->addLayout(savedHeader);
    savedHint = label("Reuse saved Wi-Fi and Ethernet profiles, or choose which ones connect automatically."); content(saved)->addWidget(savedHint);
    auto *savedBody = new QWidget; savedBody->setObjectName("networkSavedBody"); auto *savedLayout = new QVBoxLayout(savedBody);
    savedLayout->setContentsMargins(0, 4, 0, 0); savedLayout->setSpacing(10); content(saved)->addWidget(savedBody);
    list(savedBody, "connectionsTable", "No saved connections. Connect to a network to create a profile.", 6);
    auto *activate = button("connUpButton", "Connect", "network-connect", true);
    auto *deactivate = button("connDownButton", "Disconnect", "network-disconnect");
    auto *autoConnect = button("connAutoconnectButton", "Connect automatically", "view-refresh");
    auto *forget = button("connDeleteButton", "Forget connection…", "edit-delete");
    actions(savedBody, {activate, deactivate}); menu(savedBody, {autoConnect, forget});
    lists["connectionsTable"].selectionButtons = {activate, deactivate, autoConnect, forget};
    savedBody->hide();
    connect(fold, &QToolButton::toggled, this, [fold, savedBody](bool expanded) {
        fold->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow); savedBody->setVisible(expanded);
    });
    connections->addWidget(saved); connections->addStretch();

    auto *virtuals = pages["Virtual networks"];
    virtuals->addWidget(label("Connect virtual machines to your local network, or manage their private networks."));
    auto *virtualGrid = new QGridLayout; virtualGrid->setSpacing(12); virtuals->addLayout(virtualGrid);
    auto *bridges = card("Network bridges", "A bridge lets virtual machines share a physical network connection.", "network-workgroup");
    list(bridges, "bridgeTable", "No bridges configured. Create one when a virtual machine needs access to your LAN.", 7);
    auto *create = button("createBridgeButton", "Create bridge…", "list-add", true);
    auto *remove = button("deleteBridgeButton", "Delete bridge…", "edit-delete");
    auto *repair = button("fixBridgeButton", "Update hardware assignment…", "configure");
    actions(bridges, {create}); menu(bridges, {repair, remove});
    lists["bridgeTable"].selectionButtons = {remove, repair};
    auto *libvirt = card("Virtual machine networks", "Start or stop networks managed by libvirt.", "network-server");
    list(libvirt, "libvirtNetworkTable", "No libvirt networks are available. Refresh after configuring your virtual machine network.", 7);
    auto *start = button("startLibvirtNetworkButton", "Start", "media-playback-start", true);
    auto *stop = button("stopLibvirtNetworkButton", "Stop…", "media-playback-stop");
    auto *restart = button("restartLibvirtNetworkButton", "Restart…", "view-refresh");
    actions(libvirt, {start, stop, restart}); lists["libvirtNetworkTable"].selectionButtons = {start, stop, restart};
    responsive.append({virtualGrid, {bridges, libvirt}}); virtuals->addStretch();

    auto *diagnostics = pages["Diagnostics"];
    auto *addresses = card("Addresses", "Local addresses belong to your adapters. The public address is how your connection appears online.", "dialog-information");
    auto *publicRow = new QHBoxLayout; publicRow->addWidget(label("Public IP"));
    auto *publicIp = take<QLabel>(legacy, "publicIpValueLabel"); publicIp->setTextFormat(Qt::PlainText);
    publicIp->setTextInteractionFlags(Qt::TextSelectableByMouse); publicRow->addWidget(publicIp, 1); content(addresses)->addLayout(publicRow);
    list(addresses, "ipAddressTable", "No local IPv4 addresses are assigned.", 4);
    diagnostics->addWidget(addresses);
    auto *ports = card("Listening ports", "Programs accepting connections. A listening port is not necessarily reachable through your firewall.", "network-transmit-receive");
    list(ports, "portsTable", "No listening sockets reported.", 7);
    auto *portsStatus = take<QLabel>(legacy, "portsInfoLabel"); portsStatus->setTextFormat(Qt::PlainText); portsStatus->setWordWrap(true); content(ports)->addWidget(portsStatus);
    diagnostics->addWidget(ports);
    auto *traffic = card("Adapter traffic", "Total data and packet counters since each adapter was started.", "utilities-system-monitor");
    list(traffic, "interfaceStatsTable", "No interface statistics are available.", 5); diagnostics->addWidget(traffic);
    auto *repairBox = card("Connection troubleshooting", "Restarting NetworkManager briefly disconnects all managed connections.", "preferences-system-network");
    actions(repairBox, {button("restartNetworkManagerButton", "Restart network service…", "view-refresh")});
    diagnostics->addWidget(repairBox); diagnostics->addStretch();

    updateTimer = new QTimer(this); updateTimer->setSingleShot(true);
    connect(updateTimer, &QTimer::timeout, this, &NetworkPage::sync);
    for (auto *action : findChildren<QPushButton *>())
        connect(action, &QPushButton::clicked, this, [this] { updateTimer->start(0); });
    for (auto &entry : lists) {
        auto *table = entry.table;
        connect(table->model(), &QAbstractItemModel::dataChanged, this, [this] { if (!syncing) updateTimer->start(0); });
        connect(table->model(), &QAbstractItemModel::rowsInserted, this, [this] { if (!syncing) updateTimer->start(0); });
        connect(table->model(), &QAbstractItemModel::rowsRemoved, this, [this] { if (!syncing) updateTimer->start(0); });
        connect(table, &QTableWidget::itemSelectionChanged, this, [this, table] {
            if (!syncing) {
                const auto key = selectionKey(table, table->currentRow());
                if (!key.isEmpty()) lists[table->objectName()].selectedKey = key;
                updateTimer->start(0);
            }
        });
    }
    // Move the existing pages intact, preserving their controls and backend connections.
    for (const auto &entry : {QPair<const char *, const char *>{"firewallSubTab", "Firewall"}, {"networkSharesSubTab", "Shared folders"}}) {
        auto *page = take<QWidget>(legacy, entry.first);
        auto *parent = page->parentWidget();
        while (parent && !qobject_cast<QTabWidget *>(parent)) parent = parent->parentWidget();
        if (auto *tabs = qobject_cast<QTabWidget *>(parent)) tabs->removeTab(tabs->indexOf(page));
        sections->addTab(page, entry.second);
    }
    connect(sections, &QTabWidget::currentChanged, this, [this, refresh] {
        // Firewall rules require an explicit Load Rules action, never an automatic sudo prompt.
        refresh->setVisible(sections->currentWidget()->objectName() != "firewallSubTab");
        if (isVisible()) requestRefresh();
    });
    sync(); arrange();
}

QVBoxLayout *NetworkPage::content(QWidget *card) const { return qobject_cast<QVBoxLayout *>(card->layout()); }
QWidget *NetworkPage::card(const QString &title, const QString &description, const QString &icon) {
    auto *box = new QFrame(this); box->setObjectName("networkCard");
    box->setAccessibleName(title);
    auto *layout = new QVBoxLayout(box); layout->setContentsMargins(16, 14, 16, 14); layout->setSpacing(10);
    if (!title.isEmpty()) {
        auto *row = new QHBoxLayout;
        auto *symbol = new QLabel; symbol->setPixmap(QIcon::fromTheme(icon).pixmap(20, 20)); symbol->setFixedSize(22, 22);
        row->addWidget(symbol); row->addWidget(label(title, "networkCardTitle"), 1); layout->addLayout(row);
    }
    if (!description.isEmpty()) layout->addWidget(label(description));
    return box;
}
QPushButton *NetworkPage::button(const char *name, const QString &text, const QString &icon, bool primary) {
    auto *result = take<QPushButton>(legacy, name);
    result->setText(text); result->setIcon(QIcon::fromTheme(icon)); result->setProperty("networkPrimary", primary);
    result->setMinimumSize(0, 30); result->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    result->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed); return result;
}
QTableWidget *NetworkPage::list(QWidget *box, const char *name, const QString &empty, int maxRows) {
    auto *table = take<QTableWidget>(legacy, name);
    table->setProperty("networkList", true); table->setShowGrid(false); table->setAlternatingRowColors(false);
    table->setSelectionBehavior(QAbstractItemView::SelectRows); table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers); table->setWordWrap(false);
    table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel); table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    table->setTextElideMode(Qt::ElideRight); table->verticalHeader()->hide(); table->verticalHeader()->setDefaultSectionSize(36);
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    table->setItemDelegate(new NetworkDelegate(table));
    table->setMinimumSize(0, 0); content(box)->addWidget(table);
    auto *emptyLabel = label(empty, "networkEmpty"); content(box)->addWidget(emptyLabel);
    lists[name] = {table, emptyLabel, {}, {}, empty, maxRows}; return table;
}
void NetworkPage::actions(QWidget *box, const QList<QPushButton *> &buttons) {
    auto *row = new QHBoxLayout; row->setSpacing(8);
    for (auto *button : buttons) { row->addWidget(button); button->show(); }
    row->addStretch(); content(box)->addLayout(row);
}
void NetworkPage::menu(QWidget *box, const QList<QPushButton *> &buttons) {
    auto *more = new QToolButton(box); more->setText("More"); more->setIcon(QIcon::fromTheme("view-more-symbolic"));
    more->setToolButtonStyle(Qt::ToolButtonTextBesideIcon); more->setAccessibleName("More network actions");
    more->setPopupMode(QToolButton::InstantPopup); auto *choices = new QMenu(more);
    for (auto *button : buttons) {
        button->setParent(box); button->hide();
        auto *action = choices->addAction(button->icon(), button->text()); action->setObjectName(button->objectName()+"MenuAction");
        connect(action, &QAction::triggered, button, &QPushButton::click); menuActions[button] = action;
    }
    more->setMenu(choices);
    auto *row = qobject_cast<QHBoxLayout *>(content(box)->itemAt(content(box)->count()-1)->layout());
    if (row) row->insertWidget(row->count()-1, more);
}
QString NetworkPage::selectionKey(QTableWidget *table, int row) const {
    if (row < 0 || !table->item(row, 0)) return {};
    if (table->objectName() == "connectionsTable") return table->item(row, 0)->data(Qt::UserRole).toString();
    if (table->objectName() == "wifiTable") return cell(table, row, 1) + '\n' + cell(table, row, 3) + '\n' + cell(table, row, 4);
    if (table->objectName() == "portsTable") return cell(table, row, 0) + '\n' + cell(table, row, 2) + '\n' + cell(table, row, 3);
    if (table->objectName() == "ipAddressTable") return cell(table, row, 0) + '\n' + cell(table, row, 1);
    return cell(table, row, 0);
}
void NetworkPage::sync() {
    syncing = true;
    for (auto it = lists.begin(); it != lists.end(); ++it) {
        auto &entry = it.value(); auto *table = entry.table; auto *header = table->horizontalHeader();
        const auto name = it.key();
        const bool missingLibvirt = name == "libvirtNetworkTable" && table->rowCount() == 1 && cell(table, 0, 2).isEmpty();
        const int count = missingLibvirt ? 0 : table->rowCount();
        entry.empty->setText(missingLibvirt ? "Libvirt is not installed or no networks could be read." : entry.emptyText);
        entry.empty->setVisible(!count); table->setVisible(count > 0);
        table->setFixedHeight(header->sizeHint().height() + std::min(std::max(1, count), entry.maxRows)*36 + 8);
        header->setStretchLastSection(false);
        for (int c = 0; c < table->columnCount(); ++c) header->setSectionResizeMode(c, QHeaderView::ResizeToContents);
        if (table->columnCount()) header->setSectionResizeMode(0, QHeaderView::Stretch);
        if (name == "interfaceConfigTable" && table->columnCount() == 5) {
            table->setHorizontalHeaderLabels({"Adapter", "Addressing", "IPv4 address", "Gateway", "Link"});
            header->setSectionResizeMode(0, QHeaderView::ResizeToContents); header->setSectionResizeMode(2, QHeaderView::Stretch);
            table->setColumnHidden(3, true);
        } else if (name == "wifiTable" && table->columnCount() == 6) {
            table->setHorizontalHeaderLabels({"", "Network", "Signal", "Security", "Channel", "Link rate"});
            header->setSectionResizeMode(0, QHeaderView::Fixed); table->setColumnWidth(0, 28);
            table->horizontalHeaderItem(0)->setText(""); header->setSectionResizeMode(1, QHeaderView::Stretch);
            table->setColumnHidden(4, true); table->setColumnHidden(5, true);
        } else if (name == "connectionsTable" && table->columnCount() == 5) {
            table->setHorizontalHeaderLabels({"Connection", "Type", "Adapter", "Active", "Auto-connect"});
        } else if (name == "interfaceStatsTable" && table->columnCount() == 8) {
            table->setHorizontalHeaderLabels({"Adapter", "Received", "Sent", "Received packets", "Sent packets", "Receive errors", "Send errors", "Link speed"});
        } else if (name == "bridgeTable" && table->columnCount() == 3) {
            table->setHorizontalHeaderLabels({"Bridge", "Attached adapters", "Link"});
        } else if (name == "libvirtNetworkTable" && table->columnCount() == 4) {
            table->setHorizontalHeaderLabels({"Network", "Mode", "State", "Start at boot"});
        }
        for (int row = 0; row < table->rowCount(); ++row) {
            for (int c = 0; c < table->columnCount(); ++c) if (auto *item = table->item(row, c)) {
                auto tooltip = item->text();
                if (name == "wifiTable" && c == 1) tooltip += "\nChannel " + cell(table, row, 4) + " · " + cell(table, row, 5);
                if (item->toolTip() != tooltip) item->setToolTip(tooltip);
            }
            if (!entry.selectedKey.isEmpty() && selectionKey(table, row) == entry.selectedKey &&
                (selectionKey(table, table->currentRow()) != entry.selectedKey || table->selectedItems().isEmpty()))
                table->selectRow(row);
        }
        const bool selected = count > 0 && table->currentRow() >= 0 && !table->selectedItems().isEmpty() &&
            (name != "connectionsTable" || !selectionKey(table, table->currentRow()).isEmpty());
        for (auto *action : entry.selectionButtons) action->setEnabled(selected);
    }
    auto *adapters = lists["interfaceConfigTable"].table;
    const int row = adapters->currentRow();
    adapterDetail->setText(row >= 0 ? "Gateway · " + cell(adapters, row, 3) : "Select an adapter to see its gateway.");
    auto *saved = lists["connectionsTable"].table;
    savedToggle->setText(QString("Saved connections · %1").arg(saved->rowCount()));
    auto *autoConnect = take<QPushButton>(legacy, "connAutoconnectButton");
    autoConnect->setText(cell(saved, saved->currentRow(), 4) == "yes" ? "Turn auto-connect off" : "Turn auto-connect on");
    const auto wifiStatus = take<QLabel>(legacy, "wifiStatusLabel")->text();
    const bool noWifi = wifiStatus.startsWith("No Wi-Fi adapter") || wifiStatus.contains("NetworkManager is unavailable");
    take<QPushButton>(legacy, "wifiScanButton")->setEnabled(!noWifi);
    take<QPushButton>(legacy, "wifiRadioToggleButton")->setEnabled(!noWifi);
    bool wifiConnected = false;
    for (int r = 0; r < lists["wifiTable"].table->rowCount(); ++r)
        wifiConnected |= !cell(lists["wifiTable"].table, r, 0).isEmpty();
    take<QPushButton>(legacy, "wifiDisconnectButton")->setEnabled(!noWifi && wifiConnected);
    if (noWifi) { lists["wifiTable"].empty->hide(); }
    for (auto it = menuActions.begin(); it != menuActions.end(); ++it) {
        it.value()->setEnabled(it.key()->isEnabled()); it.value()->setText(it.key()->text());
    }
    syncing = false;
}
void NetworkPage::arrange() {
    const int metricColumns = width() >= 1000 ? 3 : width() >= 650 ? 2 : 1;
    while (metrics->count()) delete metrics->takeAt(0);
    for (int i = 0; i < 3; ++i) metrics->setColumnStretch(i, i < metricColumns ? 1 : 0);
    for (int i = 0; i < metricCards.size(); ++i)
        metrics->addWidget(metricCards[i], i / metricColumns, i % metricColumns, 1,
                           metricColumns == 2 && i == 2 ? 2 : 1);
    const int columns = width() >= 1120 ? 2 : 1;
    for (const auto &entry : responsive) {
        auto *grid = entry.first;
        while (grid->count()) delete grid->takeAt(0);
        grid->setColumnStretch(0, 1); grid->setColumnStretch(1, columns == 2 ? 1 : 0);
        for (int i = 0; i < entry.second.size(); ++i)
            grid->addWidget(entry.second[i], i / columns, i % columns,
                            columns == 2 ? Qt::Alignment{} : Qt::AlignTop);
    }
}
void NetworkPage::requestRefresh() {
    emit refreshRequested(sections->tabText(sections->currentIndex()));
    sync();
}
void NetworkPage::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (refreshScheduled) return;
    refreshScheduled = true;
    QTimer::singleShot(0, this, [this] { refreshScheduled = false; if (isVisible()) requestRefresh(); });
}
void NetworkPage::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event); arrange();
    if (updateTimer) updateTimer->start(0);
}
