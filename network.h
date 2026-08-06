// Network management functions

#include <QNetworkInterface>
#include <QHostInfo>
#include <QNetworkAddressEntry>
#include <QRegularExpression>
#include <QFile>
#include <QTextStream>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QGroupBox>
#include <QRadioButton>
#include <QGridLayout>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QCoreApplication>

void MainWindow::refreshNetworkInfo() {
    // Clear and setup IP address table
    ui->ipAddressTable->setRowCount(0);
    ui->ipAddressTable->setColumnCount(4);
    QStringList headers;
    headers << "Interface" << "IP Address" << "Netmask" << "Status";
    ui->ipAddressTable->setHorizontalHeaderLabels(headers);
    ui->ipAddressTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->ipAddressTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->ipAddressTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->ipAddressTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    
    // Get all network interfaces
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    QString localIp;
    QStringList dnsServers;
    
    for (const QNetworkInterface &interface : interfaces) {
        // Skip loopback and inactive interfaces
        if (interface.flags().testFlag(QNetworkInterface::IsLoopBack) || 
            !interface.flags().testFlag(QNetworkInterface::IsUp)) {
            continue;
        }
        
        QString interfaceName = interface.humanReadableName();
        if (interfaceName.isEmpty()) {
            interfaceName = interface.name();
        }
        
        QList<QNetworkAddressEntry> entries = interface.addressEntries();
        for (const QNetworkAddressEntry &entry : entries) {
            QHostAddress ip = entry.ip();
            
            // Only show IPv4 addresses
            if (ip.protocol() == QAbstractSocket::IPv4Protocol) {
                int row = ui->ipAddressTable->rowCount();
                ui->ipAddressTable->insertRow(row);
                
                ui->ipAddressTable->setItem(row, 0, new QTableWidgetItem(interfaceName));
                ui->ipAddressTable->setItem(row, 1, new QTableWidgetItem(ip.toString()));
                ui->ipAddressTable->setItem(row, 2, new QTableWidgetItem(entry.netmask().toString()));
                
                QString status = interface.flags().testFlag(QNetworkInterface::IsRunning) ? "Up" : "Down";
                ui->ipAddressTable->setItem(row, 3, new QTableWidgetItem(status));
                
                // Get first non-loopback IPv4 as local IP
                if (localIp.isEmpty() && !ip.isLoopback()) {
                    localIp = ip.toString();
                }
            }
        }
    }
    
    // Update local IP label
    if (!localIp.isEmpty()) {
        ui->localIpValueLabel->setText(localIp);
    } else {
        ui->localIpValueLabel->setText("Not available");
    }
    
    // Get hostname
    QString hostname = QHostInfo::localHostName();
    ui->hostnameValueLabel->setText(hostname);
    
    // Get DNS servers from /etc/resolv.conf
    QFile resolvFile("/etc/resolv.conf");
    if (resolvFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&resolvFile);
        QStringList dnsList;
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("nameserver")) {
                QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                if (parts.size() >= 2) {
                    dnsList << parts[1];
                }
            }
        }
        if (!dnsList.isEmpty()) {
            ui->dnsValueLabel->setText(dnsList.join(", "));
        } else {
            ui->dnsValueLabel->setText("Not configured");
        }
        resolvFile.close();
    } else {
        ui->dnsValueLabel->setText("Cannot read");
    }
    
    // Get public IP (async)
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl("https://api.ipify.org"));
    QNetworkReply *reply = manager->get(request);
    
    connect(reply, &QNetworkReply::finished, [this, reply, manager]() {
        if (reply->error() == QNetworkReply::NoError) {
            QString publicIp = QString::fromUtf8(reply->readAll()).trimmed();
            ui->publicIpValueLabel->setText(publicIp);
        } else {
            ui->publicIpValueLabel->setText("Unable to fetch");
        }
        reply->deleteLater();
        manager->deleteLater();
    });
}

void MainWindow::on_refreshIpButton_clicked() {
    refreshNetworkInfo();
    refreshInterfaceStats();
}

void MainWindow::refreshInterfaceStats() {
    ui->interfaceStatsTable->setRowCount(0);
    ui->interfaceStatsTable->setColumnCount(8);
    QStringList headers;
    headers << "Interface" << "RX Bytes" << "TX Bytes" << "RX Packets" << "TX Packets" << "RX Errors" << "TX Errors" << "Speed";
    ui->interfaceStatsTable->setHorizontalHeaderLabels(headers);
    for (int i = 0; i < 8; i++) {
        ui->interfaceStatsTable->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    }
    
    // First, read all statistics from /proc/net/dev into a map
    QMap<QString, QStringList> statsMap;
    QFile procFile("/proc/net/dev");
    if (procFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&procFile);
        QString line;
        while (!in.atEnd()) {
            line = in.readLine();
            QString trimmed = line.trimmed();
            // Skip header lines
            if (trimmed.startsWith("Inter-") || trimmed.startsWith("face") || trimmed.isEmpty()) {
                continue;
            }
            
            // Parse line: interface: rx_bytes rx_packets rx_errs rx_drop ... tx_bytes tx_packets tx_errs tx_drop ...
            if (trimmed.contains(":")) {
                QStringList parts = trimmed.split(":", Qt::SkipEmptyParts);
                if (parts.size() >= 2) {
                    QString interfaceName = parts[0].trimmed();
                    QString statsData = parts[1].trimmed();
                    QStringList statsParts = statsData.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                    if (statsParts.size() >= 16) {
                        statsMap[interfaceName] = statsParts;
                    }
                }
            }
        }
        procFile.close();
    }
    
    // Iterate over every interface that has stats in /proc/net/dev (don't rely on Qt's list,
    // which can omit bridge slaves or differ by platform and leave the table empty)
    for (auto it = statsMap.begin(); it != statsMap.end(); ++it) {
        QString interfaceName = it.key();
        QStringList statsParts = it.value();
        if (statsParts.size() < 16) {
            continue;
        }
        // Skip loopback by name
        if (interfaceName == "lo") {
            continue;
        }
        QString displayName = interfaceName;
        QNetworkInterface iface = QNetworkInterface::interfaceFromName(interfaceName);
        if (iface.isValid() && !iface.humanReadableName().isEmpty()) {
            displayName = iface.humanReadableName();
        }
        // Format: rx_bytes rx_packets rx_errs rx_drop rx_fifo rx_frame rx_compressed rx_multicast
        //         tx_bytes tx_packets tx_errs tx_drop tx_fifo tx_colls tx_carrier tx_compressed
        qint64 rxBytes = statsParts[0].toLongLong();
        qint64 rxPackets = statsParts[1].toLongLong();
        qint64 rxErrors = statsParts[2].toLongLong();
        qint64 txBytes = statsParts[8].toLongLong();
        qint64 txPackets = statsParts[9].toLongLong();
        qint64 txErrors = statsParts[10].toLongLong();
        QString speed = "Unknown";
        QProcess speedProc;
        speedProc.start("ethtool", QStringList() << interfaceName);
        speedProc.waitForFinished();
        if (speedProc.exitCode() == 0) {
            QString speedOutput = QString::fromUtf8(speedProc.readAllStandardOutput());
            QRegularExpression speedRe("Speed:\\s+(\\S+)");
            QRegularExpressionMatch speedMatch = speedRe.match(speedOutput);
            if (speedMatch.hasMatch()) {
                speed = speedMatch.captured(1);
            }
        }
        int row = ui->interfaceStatsTable->rowCount();
        ui->interfaceStatsTable->insertRow(row);
        ui->interfaceStatsTable->setItem(row, 0, new QTableWidgetItem(displayName));
        ui->interfaceStatsTable->setItem(row, 1, new QTableWidgetItem(formatBytes(rxBytes)));
        ui->interfaceStatsTable->setItem(row, 2, new QTableWidgetItem(formatBytes(txBytes)));
        ui->interfaceStatsTable->setItem(row, 3, new QTableWidgetItem(QString::number(rxPackets)));
        ui->interfaceStatsTable->setItem(row, 4, new QTableWidgetItem(QString::number(txPackets)));
        ui->interfaceStatsTable->setItem(row, 5, new QTableWidgetItem(QString::number(rxErrors)));
        ui->interfaceStatsTable->setItem(row, 6, new QTableWidgetItem(QString::number(txErrors)));
        ui->interfaceStatsTable->setItem(row, 7, new QTableWidgetItem(speed));
    }
}

QString MainWindow::formatBytes(qint64 bytes) {
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;
    const qint64 TB = GB * 1024;
    
    if (bytes >= TB) {
        return QString::number(bytes / (double)TB, 'f', 2) + " TB";
    } else if (bytes >= GB) {
        return QString::number(bytes / (double)GB, 'f', 2) + " GB";
    } else if (bytes >= MB) {
        return QString::number(bytes / (double)MB, 'f', 2) + " MB";
    } else if (bytes >= KB) {
        return QString::number(bytes / (double)KB, 'f', 2) + " KB";
    } else {
        return QString::number(bytes) + " B";
    }
}

void MainWindow::refreshBridges() {
    ui->bridgeTable->setRowCount(0);
    ui->bridgeTable->setColumnCount(3);
    QStringList headers;
    headers << "Bridge Name" << "Interfaces" << "Status";
    ui->bridgeTable->setHorizontalHeaderLabels(headers);
    ui->bridgeTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->bridgeTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->bridgeTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    
    // Map: bridge name -> list of slave interface names
    QMap<QString, QStringList> bridgeMap;
    
    // Get list of bridges from kernel (ip link type bridge)
    QProcess ipProc;
    ipProc.start("ip", QStringList() << "link" << "show" << "type" << "bridge");
    ipProc.waitForFinished();
    if (ipProc.exitCode() != 0) {
        return;
    }
    QString ipOutput = QString::fromUtf8(ipProc.readAllStandardOutput());
    QRegularExpression bridgeNameRe("^\\d+:\\s+(\\S+):");
    for (const QString &line : ipOutput.split('\n', Qt::SkipEmptyParts)) {
        QRegularExpressionMatch m = bridgeNameRe.match(line.trimmed());
        if (m.hasMatch()) {
            QString br = m.captured(1);
            if (!bridgeMap.contains(br)) {
                bridgeMap[br] = QStringList();
            }
        }
    }
    
    // Get slave interfaces from "bridge link show" (iproute2; works without deprecated brctl)
    QProcess bridgeProc;
    bridgeProc.start("bridge", QStringList() << "link" << "show");
    bridgeProc.waitForFinished();
    if (bridgeProc.exitCode() == 0) {
        // Lines like " 2: enp7s0: <...> ... master br0 ..."
        QString blOutput = QString::fromUtf8(bridgeProc.readAllStandardOutput());
        QRegularExpression masterRe("master\\s+(\\S+)");
        for (const QString &line : blOutput.split('\n', Qt::SkipEmptyParts)) {
            QString trimmed = line.trimmed();
            if (trimmed.isEmpty()) continue;
            QRegularExpressionMatch masterMatch = masterRe.match(trimmed);
            if (masterMatch.hasMatch()) {
                QString masterBr = masterMatch.captured(1);
                // First token is "N: ifname:" - get ifname
                QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                if (parts.size() >= 2) {
                    QString iface = parts[1].trimmed();
                    if (iface.endsWith(':')) {
                        iface.chop(1);
                    }
                    if (bridgeMap.contains(masterBr) && !iface.isEmpty()) {
                        bridgeMap[masterBr] << iface;
                    }
                }
            }
        }
    }
    
    // Populate table
    for (auto it = bridgeMap.begin(); it != bridgeMap.end(); ++it) {
        QString bridgeName = it.key();
        QStringList interfaces = it.value();
        
        int row = ui->bridgeTable->rowCount();
        ui->bridgeTable->insertRow(row);
        ui->bridgeTable->setItem(row, 0, new QTableWidgetItem(bridgeName));
        ui->bridgeTable->setItem(row, 1, new QTableWidgetItem(interfaces.join(", ")));
        
        QProcess statusProc;
        statusProc.start("ip", QStringList() << "link" << "show" << bridgeName);
        statusProc.waitForFinished();
        QString statusOutput = QString::fromUtf8(statusProc.readAllStandardOutput());
        QString status = statusOutput.contains("state UP") ? "Up" : "Down";
        ui->bridgeTable->setItem(row, 2, new QTableWidgetItem(status));
    }
}

void MainWindow::on_refreshBridgeButton_clicked() {
    refreshBridges();
}

void MainWindow::on_createBridgeButton_clicked() {
    // Require NetworkManager for a bridge that gets DHCP and works for VMs
    QProcess nmCheck;
    nmCheck.start("which", QStringList() << "nmcli");
    nmCheck.waitForFinished();
    if (nmCheck.exitCode() != 0) {
        QMessageBox::warning(this, "Create Bridge",
                             "NetworkManager (nmcli) is required to create a managed bridge.\nInstall it (e.g. pacman -S networkmanager).");
        return;
    }
    
    // Get ethernet interfaces (exclude virtual/bridge)
    QStringList ethernetIfaces;
    QProcess ipProc;
    ipProc.start("ip", QStringList() << "-o" << "link" << "show" << "type" << "ethernet");
    ipProc.waitForFinished();
    if (ipProc.exitCode() == 0) {
        QString out = QString::fromUtf8(ipProc.readAllStandardOutput());
        for (const QString &line : out.split('\n', Qt::SkipEmptyParts)) {
            // "2: enp7s0: <...>" -> enp7s0
            QRegularExpression re("^\\d+:\\s+(\\S+):");
            QRegularExpressionMatch m = re.match(line.trimmed());
            if (m.hasMatch()) {
                QString iface = m.captured(1);
                if (!iface.startsWith("br") && !iface.startsWith("vir")) {
                    ethernetIfaces << iface;
                }
            }
        }
    }
    if (ethernetIfaces.isEmpty()) {
        QMessageBox::warning(this, "Create Bridge", "No Ethernet interface found. Connect a cable or check your hardware.");
        return;
    }
    
    // Default route interface for pre-selection
    QString defaultIface;
    QProcess routeProc;
    routeProc.start("ip", QStringList() << "route" << "show" << "default");
    routeProc.waitForFinished();
    if (routeProc.exitCode() == 0) {
        QString out = QString::fromUtf8(routeProc.readAllStandardOutput()).trimmed();
        QStringList parts = out.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        for (int i = 0; i < parts.size() - 1; i++) {
            if (parts[i] == "dev" && i + 1 < parts.size()) {
                defaultIface = parts[i + 1];
                break;
            }
        }
    }
    
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Create Bridge (NetworkManager)");
    dialog->setMinimumWidth(400);
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    
    QLabel *infoLabel = new QLabel(
        "Creates a bridge managed by NetworkManager with DHCP. "
        "The selected Ethernet interface will be attached so the bridge gets an IP. "
        "Use this for VM networking (e.g. libvirt).", dialog);
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);
    
    QHBoxLayout *brLayout = new QHBoxLayout();
    brLayout->addWidget(new QLabel("Bridge name:", dialog));
    QLineEdit *brEdit = new QLineEdit("br0", dialog);
    brEdit->setPlaceholderText("br0");
    brLayout->addWidget(brEdit);
    layout->addLayout(brLayout);
    
    QHBoxLayout *ifLayout = new QHBoxLayout();
    ifLayout->addWidget(new QLabel("Interface to attach:", dialog));
    QComboBox *ifaceCombo = new QComboBox(dialog);
    ifaceCombo->addItems(ethernetIfaces);
    int defaultIdx = ethernetIfaces.indexOf(defaultIface);
    if (defaultIdx >= 0) {
        ifaceCombo->setCurrentIndex(defaultIdx);
    }
    ifLayout->addWidget(ifaceCombo);
    layout->addLayout(ifLayout);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *okBtn = new QPushButton("Create", dialog);
    QPushButton *cancelBtn = new QPushButton("Cancel", dialog);
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);
    
    connect(cancelBtn, &QPushButton::clicked, dialog, &QDialog::reject);
    connect(okBtn, &QPushButton::clicked, [dialog, brEdit, ifaceCombo, this]() {
        QString bridgeName = brEdit->text().trimmed();
        QString iface = ifaceCombo->currentText().trimmed();
        if (bridgeName.isEmpty()) {
            bridgeName = "br0";
        }
        // Allow only safe chars (alphanumeric + underscore)
        if (!QRegularExpression("^[a-zA-Z0-9_]+$").match(bridgeName).hasMatch()) {
            QMessageBox::warning(dialog, "Invalid name", "Bridge name must contain only letters, numbers, and underscores.");
            return;
        }
        if (iface.isEmpty()) {
            QMessageBox::warning(dialog, "Invalid selection", "Please select an interface to attach.");
            return;
        }
        
        QString script = QString(
            "conn=$(nmcli -t -g GENERAL.CONNECTION device show \"%1\" 2>/dev/null | head -1); "
            "[ -n \"$conn\" ] && [ \"$conn\" != -- ] && nmcli connection delete \"$conn\" 2>/dev/null; "
            "nmcli connection delete \"%2\" 2>/dev/null; "
            "nmcli connection add type bridge ifname \"%2\" con-name \"%2\"; "
            "nmcli connection add type bridge-slave ifname \"%1\" master \"%2\"; "
            "nmcli connection modify \"%2\" ipv4.method auto ipv6.method auto connection.autoconnect yes; "
            "nmcli connection up \"%2\"; "
            "echo ''; echo 'Bridge %2 is up.'; echo 'Press Enter to close.'; read -r"
        ).arg(iface, bridgeName);
        runSudoCommandInTerminal("sudo bash -c '" + script + "'");
        dialog->accept();
        QTimer::singleShot(3000, this, [this]() { refreshBridges(); });
    });
    
    dialog->exec();
    dialog->deleteLater();
}

void MainWindow::on_deleteBridgeButton_clicked() {
    QList<QTableWidgetItem*> selected = ui->bridgeTable->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a bridge to delete.");
        return;
    }
    
    int row = selected[0]->row();
    QString bridgeName = ui->bridgeTable->item(row, 0)->text().trimmed();
    if (bridgeName.isEmpty()) {
        return;
    }
    
    int ret = QMessageBox::question(this, "Delete Bridge",
                                     QString("Are you sure you want to delete bridge '%1'?\n\n"
                                             "If it is managed by NetworkManager it will be removed cleanly. "
                                             "Otherwise slaves will be detached first.").arg(bridgeName),
                                     QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (ret != QMessageBox::Yes) {
        return;
    }
    
    // Try NM first (clean teardown); then fallback: detach slaves and ip link delete
    // Use temp script to avoid fragile quoting; script removes itself when done
    QString scriptPath = QDir::tempPath() + "/cachyos_delete_bridge_" + QString::number(QCoreApplication::applicationPid()) + ".sh";
    QString scriptContent = QString(
        "#!/bin/bash\n"
        "br=\"%1\"\n"
        "nmcli connection delete \"$br\" 2>/dev/null\n"
        "if ip link show \"$br\" &>/dev/null; then\n"
        "  for s in $(bridge link show 2>/dev/null | grep \" master $br \" | awk '{gsub(/:$/,\"\",$2); print $2}'); do\n"
        "    ip link set \"$s\" nomaster 2>/dev/null\n"
        "  done\n"
        "  ip link set \"$br\" down 2>/dev/null\n"
        "  ip link delete \"$br\" 2>/dev/null\n"
        "fi\n"
        "echo ''; echo 'Done.'; echo 'Press Enter to close.'; read -r\n"
        "rm -f \"$0\"\n"
    ).arg(bridgeName);
    QFile scriptFile(scriptPath);
    if (!scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Could not create temporary script.");
        return;
    }
    scriptFile.write(scriptContent.toUtf8());
    scriptFile.close();
    scriptFile.setPermissions(scriptFile.permissions() | QFile::ExeOwner);
    runSudoCommandInTerminal("sudo " + scriptPath);
    
    QTimer::singleShot(3000, this, [this]() {
        refreshBridges();
    });
}

void MainWindow::refreshLibvirtNetworks() {
    ui->libvirtNetworkTable->setRowCount(0);
    ui->libvirtNetworkTable->setColumnCount(4);
    QStringList headers;
    headers << "Network Name" << "Type" << "State" << "Autostart";
    ui->libvirtNetworkTable->setHorizontalHeaderLabels(headers);
    ui->libvirtNetworkTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->libvirtNetworkTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->libvirtNetworkTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->libvirtNetworkTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    
    // Check if libvirt is available
    QProcess checkProc;
    checkProc.start("virsh", QStringList() << "--version");
    checkProc.waitForFinished();
    
    if (checkProc.exitCode() != 0) {
        int row = ui->libvirtNetworkTable->rowCount();
        ui->libvirtNetworkTable->insertRow(row);
        ui->libvirtNetworkTable->setItem(row, 0, new QTableWidgetItem("Libvirt not installed"));
        ui->libvirtNetworkTable->setItem(row, 1, new QTableWidgetItem(""));
        ui->libvirtNetworkTable->setItem(row, 2, new QTableWidgetItem(""));
        ui->libvirtNetworkTable->setItem(row, 3, new QTableWidgetItem(""));
        return;
    }
    
    // List libvirt networks
    QProcess proc;
    proc.start("virsh", QStringList() << "net-list" << "--all");
    proc.waitForFinished();
    
    if (proc.exitCode() == 0) {
        QString output = QString::fromUtf8(proc.readAllStandardOutput());
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        
        // Skip header lines
        for (int i = 2; i < lines.size(); i++) {
            QStringList parts = lines[i].split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 3) {
                QString name = parts[0];
                QString state = parts[1];
                QString autostart = parts.size() >= 3 ? parts[2] : "no";
                
                // Get network type
                QProcess infoProc;
                infoProc.start("virsh", QStringList() << "net-info" << name);
                infoProc.waitForFinished();
                QString infoOutput = QString::fromUtf8(infoProc.readAllStandardOutput());
                QString type = "NAT";
                if (infoOutput.contains("bridge")) {
                    type = "Bridge";
                } else if (infoOutput.contains("routed")) {
                    type = "Routed";
                }
                
                int row = ui->libvirtNetworkTable->rowCount();
                ui->libvirtNetworkTable->insertRow(row);
                ui->libvirtNetworkTable->setItem(row, 0, new QTableWidgetItem(name));
                ui->libvirtNetworkTable->setItem(row, 1, new QTableWidgetItem(type));
                ui->libvirtNetworkTable->setItem(row, 2, new QTableWidgetItem(state));
                ui->libvirtNetworkTable->setItem(row, 3, new QTableWidgetItem(autostart));
            }
        }
    }
}

void MainWindow::on_refreshLibvirtNetworkButton_clicked() {
    refreshLibvirtNetworks();
}

void MainWindow::on_startLibvirtNetworkButton_clicked() {
    QList<QTableWidgetItem*> selected = ui->libvirtNetworkTable->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a network to start.");
        return;
    }
    
    int row = selected[0]->row();
    QString networkName = ui->libvirtNetworkTable->item(row, 0)->text();
    
    QProcess proc;
    proc.start("virsh", QStringList() << "net-start" << networkName);
    proc.waitForFinished();
    
    if (proc.exitCode() == 0) {
        QMessageBox::information(this, "Success", QString("Network '%1' started successfully.").arg(networkName));
        refreshLibvirtNetworks();
    } else {
        QString error = QString::fromUtf8(proc.readAllStandardError());
        QMessageBox::warning(this, "Error", QString("Failed to start network:\n%1").arg(error));
    }
}

void MainWindow::on_stopLibvirtNetworkButton_clicked() {
    QList<QTableWidgetItem*> selected = ui->libvirtNetworkTable->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a network to stop.");
        return;
    }
    
    int row = selected[0]->row();
    QString networkName = ui->libvirtNetworkTable->item(row, 0)->text();
    
    int ret = QMessageBox::question(this, "Stop Network", 
                                     QString("Are you sure you want to stop network '%1'?").arg(networkName),
                                     QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        QProcess proc;
        proc.start("virsh", QStringList() << "net-destroy" << networkName);
        proc.waitForFinished();
        
        if (proc.exitCode() == 0) {
            QMessageBox::information(this, "Success", QString("Network '%1' stopped successfully.").arg(networkName));
            refreshLibvirtNetworks();
        } else {
            QString error = QString::fromUtf8(proc.readAllStandardError());
            QMessageBox::warning(this, "Error", QString("Failed to stop network:\n%1").arg(error));
        }
    }
}

void MainWindow::on_restartLibvirtNetworkButton_clicked() {
    QList<QTableWidgetItem*> selected = ui->libvirtNetworkTable->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a network to restart.");
        return;
    }
    
    int row = selected[0]->row();
    QString networkName = ui->libvirtNetworkTable->item(row, 0)->text();
    
    QProcess proc;
    proc.start("virsh", QStringList() << "net-destroy" << networkName);
    proc.waitForFinished();
    
    QTimer::singleShot(1000, this, [this, networkName]() {
        QProcess proc2;
        proc2.start("virsh", QStringList() << "net-start" << networkName);
        proc2.waitForFinished();
        
        if (proc2.exitCode() == 0) {
            QMessageBox::information(this, "Success", QString("Network '%1' restarted successfully.").arg(networkName));
            refreshLibvirtNetworks();
        } else {
            QString error = QString::fromUtf8(proc2.readAllStandardError());
            QMessageBox::warning(this, "Error", QString("Failed to restart network:\n%1").arg(error));
        }
    });
}

void MainWindow::refreshInterfaceConfig() {
    ui->interfaceConfigTable->setRowCount(0);
    ui->interfaceConfigTable->setColumnCount(5);
    QStringList headers;
    headers << "Interface" << "Configuration" << "IP Address" << "Gateway" << "Status";
    ui->interfaceConfigTable->setHorizontalHeaderLabels(headers);
    for (int i = 0; i < 5; i++) {
        ui->interfaceConfigTable->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    }
    
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    
    for (const QNetworkInterface &interface : interfaces) {
        // Skip loopback
        if (interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            continue;
        }
        
        QString interfaceName = interface.humanReadableName();
        if (interfaceName.isEmpty()) {
            interfaceName = interface.name();
        }
        
        QNetworkInterface::InterfaceFlags flags = interface.flags();
        QString status = (flags.testFlag(QNetworkInterface::IsUp) && flags.testFlag(QNetworkInterface::IsRunning)) ? "Up" : "Down";
        
        // Get IP configuration
        QString configType = "Unknown";
        QString ipAddress = "N/A";
        QString gateway = "N/A";
        
        QList<QNetworkAddressEntry> entries = interface.addressEntries();
        for (const QNetworkAddressEntry &entry : entries) {
            QHostAddress ip = entry.ip();
            if (ip.protocol() == QAbstractSocket::IPv4Protocol) {
                ipAddress = ip.toString();
                break;
            }
        }
        
        // Check if using NetworkManager or systemd-networkd
        QProcess nmProc;
        nmProc.start("nmcli", QStringList() << "device" << "show" << interfaceName);
        nmProc.waitForFinished();
        
        if (nmProc.exitCode() == 0) {
            QString nmOutput = QString::fromUtf8(nmProc.readAllStandardOutput());
            if (nmOutput.contains("IP4.ADDRESS")) {
                // Check if DHCP or manual
                if (nmOutput.contains("IP4.METHOD:auto") || nmOutput.contains("IP4.METHOD:dhcp")) {
                    configType = "AUTOMATIC (DHCP)";
                } else {
                    configType = "MANUAL (Static)";
                }
                
                // Extract gateway
                QRegularExpression gwRe("IP4\\.GATEWAY:\\s+(\\S+)");
                QRegularExpressionMatch gwMatch = gwRe.match(nmOutput);
                if (gwMatch.hasMatch()) {
                    gateway = gwMatch.captured(1);
                }
            }
        } else {
            // Check systemd-networkd
            QString networkdFile = QString("/etc/systemd/network/10-%1.network").arg(interfaceName);
            QFile file(networkdFile);
            if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                bool foundDhcp = false;
                while (!in.atEnd()) {
                    QString line = in.readLine().trimmed();
                    if (line.startsWith("DHCP=")) {
                        if (line.contains("yes") || line.contains("ipv4")) {
                            configType = "AUTOMATIC (DHCP)";
                            foundDhcp = true;
                        } else {
                            configType = "MANUAL (Static)";
                        }
                    } else if (line.startsWith("Address=") && !foundDhcp) {
                        configType = "MANUAL (Static)";
                    } else if (line.startsWith("Gateway=")) {
                        QStringList parts = line.split("=");
                        if (parts.size() >= 2) {
                            gateway = parts[1].trimmed();
                        }
                    }
                }
                file.close();
            } else {
                // Check if using dhcpcd
                QProcess dhcpcdProc;
                dhcpcdProc.start("dhcpcd", QStringList() << "-U" << interfaceName);
                dhcpcdProc.waitForFinished();
                if (dhcpcdProc.exitCode() == 0) {
                    configType = "AUTOMATIC (DHCP)";
                } else {
                    configType = "Not configured";
                }
            }
        }
        
        int row = ui->interfaceConfigTable->rowCount();
        ui->interfaceConfigTable->insertRow(row);
        ui->interfaceConfigTable->setItem(row, 0, new QTableWidgetItem(interfaceName));
        ui->interfaceConfigTable->setItem(row, 1, new QTableWidgetItem(configType));
        ui->interfaceConfigTable->setItem(row, 2, new QTableWidgetItem(ipAddress));
        ui->interfaceConfigTable->setItem(row, 3, new QTableWidgetItem(gateway));
        ui->interfaceConfigTable->setItem(row, 4, new QTableWidgetItem(status));
    }
}

void MainWindow::on_refreshInterfaceConfigButton_clicked() {
    refreshInterfaceConfig();
}

void MainWindow::on_configureInterfaceButton_clicked() {
    QList<QTableWidgetItem*> selected = ui->interfaceConfigTable->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select an interface to configure.");
        return;
    }
    
    int row = selected[0]->row();
    QString interfaceName = ui->interfaceConfigTable->item(row, 0)->text();
    configureInterface(interfaceName);
}

void MainWindow::configureInterface(const QString &interfaceName) {
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(QString("Configure Interface: %1").arg(interfaceName));
    dialog->setMinimumWidth(500);
    
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    
    // Configuration type (AUTOMATIC/MANUAL)
    QGroupBox *configTypeGroup = new QGroupBox("Configuration Type", dialog);
    QVBoxLayout *configTypeLayout = new QVBoxLayout(configTypeGroup);
    QRadioButton *automaticRadio = new QRadioButton("AUTOMATIC (DHCP)", configTypeGroup);
    QRadioButton *manualRadio = new QRadioButton("MANUAL (Static IP)", configTypeGroup);
    configTypeLayout->addWidget(automaticRadio);
    configTypeLayout->addWidget(manualRadio);
    automaticRadio->setChecked(true);
    layout->addWidget(configTypeGroup);
    
    // Manual configuration fields
    QGroupBox *manualGroup = new QGroupBox("Manual Configuration", dialog);
    QGridLayout *manualLayout = new QGridLayout(manualGroup);
    
    QLabel *ipLabel = new QLabel("IP Address:", manualGroup);
    QLineEdit *ipEdit = new QLineEdit(manualGroup);
    ipEdit->setPlaceholderText("192.168.1.100");
    
    QLabel *netmaskLabel = new QLabel("Netmask:", manualGroup);
    QLineEdit *netmaskEdit = new QLineEdit(manualGroup);
    netmaskEdit->setPlaceholderText("255.255.255.0");
    
    QLabel *gatewayLabel = new QLabel("Gateway:", manualGroup);
    QLineEdit *gatewayEdit = new QLineEdit(manualGroup);
    gatewayEdit->setPlaceholderText("192.168.1.1");
    
    QLabel *dnsLabel = new QLabel("DNS Servers:", manualGroup);
    QLineEdit *dnsEdit = new QLineEdit(manualGroup);
    dnsEdit->setPlaceholderText("8.8.8.8, 8.8.4.4");
    
    manualLayout->addWidget(ipLabel, 0, 0);
    manualLayout->addWidget(ipEdit, 0, 1);
    manualLayout->addWidget(netmaskLabel, 1, 0);
    manualLayout->addWidget(netmaskEdit, 1, 1);
    manualLayout->addWidget(gatewayLabel, 2, 0);
    manualLayout->addWidget(gatewayEdit, 2, 1);
    manualLayout->addWidget(dnsLabel, 3, 0);
    manualLayout->addWidget(dnsEdit, 3, 1);
    
    manualGroup->setEnabled(false);
    layout->addWidget(manualGroup);
    
    // Enable/disable manual fields based on radio selection
    connect(manualRadio, &QRadioButton::toggled, manualGroup, &QGroupBox::setEnabled);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton("Apply", dialog);
    QPushButton *cancelButton = new QPushButton("Cancel", dialog);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);
    
    connect(okButton, &QPushButton::clicked, [this, dialog, interfaceName, automaticRadio, ipEdit, netmaskEdit, gatewayEdit, dnsEdit]() {
        // Build the nmcli modify arguments
        QString modifyArgs;
        if (automaticRadio->isChecked()) {
            modifyArgs = "ipv4.method auto ipv4.addresses '' ipv4.gateway '' ipv4.dns ''";
        } else {
            QString ip = ipEdit->text().trimmed();
            QString netmask = netmaskEdit->text().trimmed();
            QString gateway = gatewayEdit->text().trimmed();
            QString dns = dnsEdit->text().trimmed().replace(" ", "");

            if (ip.isEmpty() || netmask.isEmpty()) {
                QMessageBox::warning(dialog, "Invalid Input", "IP Address and Netmask are required for manual configuration.");
                return;
            }

            QString cidr = convertNetmaskToCIDR(netmask);
            modifyArgs = QString("ipv4.method manual ipv4.addresses %1/%2").arg(ip, cidr);
            if (!gateway.isEmpty()) {
                modifyArgs += QString(" ipv4.gateway %1").arg(gateway);
            }
            if (!dns.isEmpty()) {
                modifyArgs += QString(" ipv4.dns %1").arg(dns);
            }
        }

        // The NM connection name usually differs from the interface name
        // ("Wired connection 1" etc.), so resolve it from the device and
        // apply everything in one terminal run.
        QString script = QString(
            "iface=\"%1\"\n"
            "conn=$(nmcli -t -g GENERAL.CONNECTION device show \"$iface\" 2>/dev/null | head -1)\n"
            "if [ -z \"$conn\" ] || [ \"$conn\" = \"--\" ]; then\n"
            "  echo \"No connection profile for $iface - creating one.\"\n"
            "  nmcli connection add type ethernet con-name \"$iface\" ifname \"$iface\"\n"
            "  conn=\"$iface\"\n"
            "fi\n"
            "echo \"Applying configuration to connection '$conn'...\"\n"
            "nmcli connection modify \"$conn\" %2 && nmcli connection up \"$conn\"\n"
        ).arg(interfaceName, modifyArgs);
        runScriptInTerminal(script, "configure_interface");

        dialog->accept();
        QTimer::singleShot(3000, this, [this]() {
            refreshInterfaceConfig();
            refreshNetworkInfo();
        });
    });
    
    connect(cancelButton, &QPushButton::clicked, dialog, &QDialog::reject);
    
    dialog->exec();
    dialog->deleteLater();
}

QString MainWindow::convertNetmaskToCIDR(const QString &netmask) {
    QStringList parts = netmask.split(".");
    if (parts.size() != 4) return "24"; // Default
    
    int cidr = 0;
    for (const QString &part : parts) {
        int octet = part.toInt();
        // Count set bits in octet
        for (int i = 7; i >= 0; i--) {
            if (octet & (1 << i)) {
                cidr++;
            } else {
                break; // Stop at first zero bit
            }
        }
    }
    return QString::number(cidr);
}

void MainWindow::on_upInterfaceButton_clicked() {
    QList<QTableWidgetItem*> selected = ui->interfaceConfigTable->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select an interface to bring up.");
        return;
    }
    
    int row = selected[0]->row();
    QString interfaceName = ui->interfaceConfigTable->item(row, 0)->text();
    
    QString command = QString("sudo ip link set %1 up").arg(interfaceName);
    runSudoCommandInTerminal(command);
    
    QTimer::singleShot(2000, this, [this]() {
        refreshInterfaceConfig();
    });
}

void MainWindow::on_downInterfaceButton_clicked() {
    QList<QTableWidgetItem*> selected = ui->interfaceConfigTable->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select an interface to bring down.");
        return;
    }

    int row = selected[0]->row();
    QString interfaceName = ui->interfaceConfigTable->item(row, 0)->text();

    int ret = QMessageBox::question(this, "Bring Interface Down",
                                     QString("Are you sure you want to bring interface '%1' down?").arg(interfaceName),
                                     QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        QString command = QString("sudo ip link set %1 down").arg(interfaceName);
        runSudoCommandInTerminal(command);

        QTimer::singleShot(2000, this, [this]() {
            refreshInterfaceConfig();
        });
    }
}

// Writes a bash script to a temp file and runs it with sudo in a terminal.
// Avoids fragile nested quoting; the script pauses at the end and deletes itself.
void MainWindow::runScriptInTerminal(const QString &scriptContent, const QString &namePrefix) {
    static int scriptRunId = 0;
    QString scriptPath = QDir::tempPath() + QString("/cachyos_%1_%2_%3.sh")
                         .arg(namePrefix).arg(QCoreApplication::applicationPid()).arg(++scriptRunId);
    QFile scriptFile(scriptPath);
    if (!scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Could not create temporary script.");
        return;
    }
    scriptFile.write("#!/bin/bash\n");
    scriptFile.write(scriptContent.toUtf8());
    scriptFile.write("\necho ''; echo 'Done. Press Enter to close.'; read -r\nrm -f \"$0\"\n");
    scriptFile.close();
    scriptFile.setPermissions(scriptFile.permissions() | QFile::ExeOwner);
    runSudoCommandInTerminal("sudo " + scriptPath);
}

// Splits one line of `nmcli -t` output on unescaped colons ("\:" inside
// values like SSIDs is unescaped, "\\" becomes "\").
QStringList MainWindow::splitNmcliLine(const QString &line) {
    QStringList fields;
    QString current;
    for (int i = 0; i < line.size(); ++i) {
        QChar c = line[i];
        if (c == '\\' && i + 1 < line.size()) {
            current += line[i + 1];
            ++i;
        } else if (c == ':') {
            fields << current;
            current.clear();
        } else {
            current += c;
        }
    }
    fields << current;
    return fields;
}

QString MainWindow::getWifiDevice() {
    QProcess proc;
    proc.start("nmcli", QStringList() << "-t" << "-f" << "DEVICE,TYPE" << "device");
    proc.waitForFinished(10000);
    if (proc.exitCode() != 0) {
        return QString();
    }
    const QStringList lines = QString::fromUtf8(proc.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        QStringList parts = splitNmcliLine(line);
        if (parts.size() >= 2 && parts[1] == "wifi") {
            return parts[0];
        }
    }
    return QString();
}

void MainWindow::refreshWifiNetworks(bool rescan) {
    ui->wifiTable->setRowCount(0);
    ui->wifiTable->setColumnCount(6);
    QStringList headers;
    headers << "In Use" << "SSID" << "Signal" << "Security" << "Channel" << "Rate";
    ui->wifiTable->setHorizontalHeaderLabels(headers);
    for (int i = 0; i < 5; i++) {
        ui->wifiTable->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    }
    ui->wifiTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);

    QString wifiDev = getWifiDevice();
    if (wifiDev.isEmpty()) {
        ui->wifiStatusLabel->setText("No Wi-Fi adapter detected.");
        return;
    }

    // Radio state drives the toggle button text
    QProcess radioProc;
    radioProc.start("nmcli", QStringList() << "radio" << "wifi");
    radioProc.waitForFinished(10000);
    QString radioState = QString::fromUtf8(radioProc.readAllStandardOutput()).trimmed();
    ui->wifiRadioToggleButton->setText(radioState == "enabled" ? "📶 Turn Wi-Fi Off" : "📶 Turn Wi-Fi On");
    if (radioState != "enabled") {
        ui->wifiStatusLabel->setText(QString("Wi-Fi is turned off (device %1).").arg(wifiDev));
        return;
    }

    if (rescan) {
        ui->wifiStatusLabel->setText("Scanning...");
        QApplication::processEvents();
    }

    QStringList args;
    args << "-t" << "-f" << "IN-USE,SSID,SIGNAL,SECURITY,CHAN,RATE"
         << "device" << "wifi" << "list" << "ifname" << wifiDev;
    if (rescan) {
        args << "--rescan" << "yes";
    }
    QProcess proc;
    proc.start("nmcli", args);
    proc.waitForFinished(rescan ? 30000 : 10000);
    if (proc.exitCode() != 0) {
        ui->wifiStatusLabel->setText("❌ Failed to list Wi-Fi networks.");
        return;
    }

    const QStringList lines = QString::fromUtf8(proc.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        QStringList parts = splitNmcliLine(line);
        if (parts.size() < 6 || parts[1].isEmpty()) continue; // skip hidden SSIDs

        int row = ui->wifiTable->rowCount();
        ui->wifiTable->insertRow(row);
        ui->wifiTable->setItem(row, 0, new QTableWidgetItem(parts[0].contains('*') ? "✓" : ""));
        ui->wifiTable->setItem(row, 1, new QTableWidgetItem(parts[1]));
        ui->wifiTable->setItem(row, 2, new QTableWidgetItem(parts[2] + "%"));
        ui->wifiTable->setItem(row, 3, new QTableWidgetItem(parts[3].isEmpty() ? "Open" : parts[3]));
        ui->wifiTable->setItem(row, 4, new QTableWidgetItem(parts[4]));
        ui->wifiTable->setItem(row, 5, new QTableWidgetItem(parts[5]));
    }

    ui->wifiStatusLabel->setText(QString("%1 networks found (device %2).")
                                 .arg(ui->wifiTable->rowCount()).arg(wifiDev));
}

void MainWindow::on_wifiScanButton_clicked() {
    refreshWifiNetworks(true);
}

void MainWindow::on_wifiConnectButton_clicked() {
    int row = ui->wifiTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "No Selection", "Please select a Wi-Fi network to connect to.");
        return;
    }

    QString ssid = ui->wifiTable->item(row, 1)->text();
    QString security = ui->wifiTable->item(row, 3)->text();
    QString wifiDev = getWifiDevice();

    QString password;
    if (security != "Open") {
        bool ok;
        password = QInputDialog::getText(this, "Wi-Fi Password",
                                         QString("Password for '%1'\n(leave empty if it is already saved):").arg(ssid),
                                         QLineEdit::Password, "", &ok);
        if (!ok) {
            return;
        }
    }

    // nmcli talks to NetworkManager over D-Bus; no sudo or terminal needed,
    // and passing args as a list keeps the password out of any shell.
    QStringList args;
    args << "device" << "wifi" << "connect" << ssid;
    if (!password.isEmpty()) {
        args << "password" << password;
    }
    if (!wifiDev.isEmpty()) {
        args << "ifname" << wifiDev;
    }

    ui->wifiStatusLabel->setText(QString("Connecting to '%1'...").arg(ssid));

    QProcess *process = new QProcess(this);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, process, ssid](int exitCode, QProcess::ExitStatus) {
                QString output = QString::fromUtf8(process->readAllStandardOutput());
                QString error = QString::fromUtf8(process->readAllStandardError());
                if (exitCode == 0) {
                    ui->wifiStatusLabel->setText(QString("✅ Connected to '%1'.").arg(ssid));
                    refreshWifiNetworks(false);
                    refreshNetworkInfo();
                    refreshConnectionsList();
                } else {
                    ui->wifiStatusLabel->setText(QString("❌ Failed to connect to '%1'.").arg(ssid));
                    QMessageBox::warning(this, "Connection Failed",
                                         QString("Could not connect to '%1':\n%2").arg(ssid, (output + "\n" + error).trimmed()));
                }
                process->deleteLater();
            });
    process->start("nmcli", args);
}

void MainWindow::on_wifiDisconnectButton_clicked() {
    QString wifiDev = getWifiDevice();
    if (wifiDev.isEmpty()) {
        QMessageBox::warning(this, "No Wi-Fi", "No Wi-Fi adapter detected.");
        return;
    }

    QProcess proc;
    proc.start("nmcli", QStringList() << "device" << "disconnect" << wifiDev);
    proc.waitForFinished(15000);
    if (proc.exitCode() == 0) {
        ui->wifiStatusLabel->setText(QString("Disconnected %1.").arg(wifiDev));
    } else {
        ui->wifiStatusLabel->setText(QString("❌ Failed to disconnect %1.").arg(wifiDev));
    }
    refreshWifiNetworks(false);
    refreshNetworkInfo();
}

void MainWindow::on_wifiRadioToggleButton_clicked() {
    QProcess radioProc;
    radioProc.start("nmcli", QStringList() << "radio" << "wifi");
    radioProc.waitForFinished(10000);
    QString radioState = QString::fromUtf8(radioProc.readAllStandardOutput()).trimmed();

    QProcess proc;
    proc.start("nmcli", QStringList() << "radio" << "wifi" << (radioState == "enabled" ? "off" : "on"));
    proc.waitForFinished(10000);

    QTimer::singleShot(1500, this, [this]() { refreshWifiNetworks(false); });
}

void MainWindow::refreshConnectionsList() {
    ui->connectionsTable->setRowCount(0);
    ui->connectionsTable->setColumnCount(5);
    QStringList headers;
    headers << "Name" << "Type" << "Device" << "Active" << "Autoconnect";
    ui->connectionsTable->setHorizontalHeaderLabels(headers);
    ui->connectionsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int i = 1; i < 5; i++) {
        ui->connectionsTable->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    }

    QProcess proc;
    proc.start("nmcli", QStringList() << "-t" << "-f" << "NAME,UUID,TYPE,DEVICE,ACTIVE,AUTOCONNECT" << "connection" << "show");
    proc.waitForFinished(10000);
    if (proc.exitCode() != 0) {
        return;
    }

    const QStringList lines = QString::fromUtf8(proc.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        QStringList parts = splitNmcliLine(line);
        if (parts.size() < 6 || parts[2] == "loopback") continue;

        QString type = parts[2];
        if (type == "802-3-ethernet") type = "Ethernet";
        else if (type == "802-11-wireless") type = "Wi-Fi";
        else if (type == "bridge") type = "Bridge";

        int row = ui->connectionsTable->rowCount();
        ui->connectionsTable->insertRow(row);
        QTableWidgetItem *nameItem = new QTableWidgetItem(parts[0]);
        nameItem->setData(Qt::UserRole, parts[1]); // UUID for unambiguous nmcli targeting
        ui->connectionsTable->setItem(row, 0, nameItem);
        ui->connectionsTable->setItem(row, 1, new QTableWidgetItem(type));
        ui->connectionsTable->setItem(row, 2, new QTableWidgetItem(parts[3].isEmpty() ? "--" : parts[3]));
        ui->connectionsTable->setItem(row, 3, new QTableWidgetItem(parts[4]));
        ui->connectionsTable->setItem(row, 4, new QTableWidgetItem(parts[5]));
    }
}

void MainWindow::on_connRefreshButton_clicked() {
    refreshConnectionsList();
}

void MainWindow::on_connUpButton_clicked() {
    int row = ui->connectionsTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "No Selection", "Please select a connection to activate.");
        return;
    }
    QString name = ui->connectionsTable->item(row, 0)->text();
    QString uuid = ui->connectionsTable->item(row, 0)->data(Qt::UserRole).toString();

    QProcess *process = new QProcess(this);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, process, name](int exitCode, QProcess::ExitStatus) {
                if (exitCode != 0) {
                    QString error = QString::fromUtf8(process->readAllStandardError());
                    QMessageBox::warning(this, "Error", QString("Failed to activate '%1':\n%2").arg(name, error.trimmed()));
                }
                refreshConnectionsList();
                refreshNetworkInfo();
                process->deleteLater();
            });
    process->start("nmcli", QStringList() << "connection" << "up" << "uuid" << uuid);
}

void MainWindow::on_connDownButton_clicked() {
    int row = ui->connectionsTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "No Selection", "Please select a connection to deactivate.");
        return;
    }
    QString name = ui->connectionsTable->item(row, 0)->text();
    QString uuid = ui->connectionsTable->item(row, 0)->data(Qt::UserRole).toString();

    QProcess proc;
    proc.start("nmcli", QStringList() << "connection" << "down" << "uuid" << uuid);
    proc.waitForFinished(20000);
    if (proc.exitCode() != 0) {
        QString error = QString::fromUtf8(proc.readAllStandardError());
        QMessageBox::warning(this, "Error", QString("Failed to deactivate '%1':\n%2").arg(name, error.trimmed()));
    }
    refreshConnectionsList();
    refreshNetworkInfo();
}

void MainWindow::on_connDeleteButton_clicked() {
    int row = ui->connectionsTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "No Selection", "Please select a connection to delete.");
        return;
    }
    QString name = ui->connectionsTable->item(row, 0)->text();
    QString uuid = ui->connectionsTable->item(row, 0)->data(Qt::UserRole).toString();

    int ret = QMessageBox::question(this, "Delete Connection",
                                    QString("Delete the saved connection '%1'?\n\n"
                                            "If it is currently active it will be disconnected.").arg(name),
                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes) {
        return;
    }

    QProcess proc;
    proc.start("nmcli", QStringList() << "connection" << "delete" << "uuid" << uuid);
    proc.waitForFinished(20000);
    if (proc.exitCode() != 0) {
        QString error = QString::fromUtf8(proc.readAllStandardError());
        QMessageBox::warning(this, "Error", QString("Failed to delete '%1':\n%2").arg(name, error.trimmed()));
    }
    refreshConnectionsList();
}

void MainWindow::on_connAutoconnectButton_clicked() {
    int row = ui->connectionsTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "No Selection", "Please select a connection.");
        return;
    }
    QString name = ui->connectionsTable->item(row, 0)->text();
    QString uuid = ui->connectionsTable->item(row, 0)->data(Qt::UserRole).toString();
    QString current = ui->connectionsTable->item(row, 4)->text();
    QString newValue = (current == "yes") ? "no" : "yes";

    QProcess proc;
    proc.start("nmcli", QStringList() << "connection" << "modify" << "uuid" << uuid
                                      << "connection.autoconnect" << newValue);
    proc.waitForFinished(15000);
    if (proc.exitCode() != 0) {
        QString error = QString::fromUtf8(proc.readAllStandardError());
        QMessageBox::warning(this, "Error", QString("Failed to change autoconnect for '%1':\n%2").arg(name, error.trimmed()));
    }
    refreshConnectionsList();
}

void MainWindow::on_restartNetworkManagerButton_clicked() {
    int ret = QMessageBox::question(this, "Restart NetworkManager",
                                    "Restart the NetworkManager service?\n\n"
                                    "All connections will briefly drop and reconnect.",
                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes) {
        return;
    }
    runScriptInTerminal("systemctl restart NetworkManager\n"
                        "sleep 2\n"
                        "systemctl status NetworkManager --no-pager | head -5\n",
                        "restart_nm");
    QTimer::singleShot(5000, this, [this]() {
        refreshNetworkInfo();
        refreshConnectionsList();
    });
}

void MainWindow::on_fixBridgeButton_clicked() {
    // Bridge name: selected row if any, otherwise br0
    QString bridgeName = "br0";
    QList<QTableWidgetItem*> selected = ui->bridgeTable->selectedItems();
    if (!selected.isEmpty()) {
        bridgeName = ui->bridgeTable->item(selected[0]->row(), 0)->text().trimmed();
    }

    int ret = QMessageBox::question(this, "Fix Bridge",
        QString("Repair bridge '%1' for this machine's hardware?\n\n"
                "This is for when a system image was installed on a different PC and "
                "the bridge is still bound to the old machine's network device.\n\n"
                "• Stale slave profiles pointing at missing devices are removed\n"
                "• If this machine has Ethernet, the bridge is re-attached to it\n"
                "• If only Wi-Fi is available, the bridge switches to shared (NAT) mode, "
                "since Wi-Fi adapters cannot join a bridge directly — VMs keep using "
                "'%1' and reach the network through Wi-Fi").arg(bridgeName),
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) {
        return;
    }

    QString script = QString(
        "br=\"%1\"\n"
        "echo \"=== Fixing bridge '$br' for this hardware ===\"\n"
        "\n"
        "# Find a usable wired interface (prefer one with a cable plugged in)\n"
        "eth=\"\"; first_eth=\"\"\n"
        "while IFS=: read -r dev type state; do\n"
        "  if [ \"$type\" = \"ethernet\" ]; then\n"
        "    [ -n \"$first_eth\" ] || first_eth=\"$dev\"\n"
        "    if [ \"$(cat /sys/class/net/$dev/carrier 2>/dev/null)\" = \"1\" ]; then eth=\"$dev\"; break; fi\n"
        "  fi\n"
        "done < <(nmcli -t -f DEVICE,TYPE,STATE device 2>/dev/null)\n"
        "[ -n \"$eth\" ] || eth=\"$first_eth\"\n"
        "wifi=$(nmcli -t -f DEVICE,TYPE device 2>/dev/null | awk -F: '$2==\"wifi\"{print $1; exit}')\n"
        "\n"
        "# Make sure the bridge profile itself exists\n"
        "if ! nmcli connection show \"$br\" >/dev/null 2>&1; then\n"
        "  echo \"Bridge profile '$br' not found - creating it.\"\n"
        "  nmcli connection add type bridge ifname \"$br\" con-name \"$br\"\n"
        "fi\n"
        "bruuid=$(nmcli -t -g connection.uuid connection show \"$br\" 2>/dev/null | head -1)\n"
        "\n"
        "# Remove slave profiles that point at devices this machine does not have\n"
        "for uuid in $(nmcli -t -f UUID connection show); do\n"
        "  master=$(nmcli -g connection.master connection show uuid \"$uuid\" 2>/dev/null)\n"
        "  if [ \"$master\" = \"$br\" ] || { [ -n \"$bruuid\" ] && [ \"$master\" = \"$bruuid\" ]; }; then\n"
        "    ifname=$(nmcli -g connection.interface-name connection show uuid \"$uuid\" 2>/dev/null)\n"
        "    if [ -n \"$ifname\" ] && ! ip link show \"$ifname\" >/dev/null 2>&1; then\n"
        "      echo \"Removing stale slave profile for missing device '$ifname'.\"\n"
        "      nmcli connection delete uuid \"$uuid\"\n"
        "    fi\n"
        "  fi\n"
        "done\n"
        "\n"
        "if [ -n \"$eth\" ]; then\n"
        "  echo \"Using wired interface: $eth\"\n"
        "  # Free the interface from any competing profile (but keep it if the\n"
        "  # active profile is already this bridge's own slave)\n"
        "  conn=$(nmcli -t -g GENERAL.CONNECTION device show \"$eth\" 2>/dev/null | head -1)\n"
        "  if [ -n \"$conn\" ] && [ \"$conn\" != \"--\" ] && [ \"$conn\" != \"$br\" ]; then\n"
        "    cmaster=$(nmcli -g connection.master connection show \"$conn\" 2>/dev/null)\n"
        "    if [ \"$cmaster\" != \"$br\" ] && { [ -z \"$bruuid\" ] || [ \"$cmaster\" != \"$bruuid\" ]; }; then\n"
        "      echo \"Removing competing profile '$conn' from $eth.\"\n"
        "      nmcli connection delete \"$conn\" 2>/dev/null\n"
        "    fi\n"
        "  fi\n"
        "  # Enslave it unless a slave profile for it already exists\n"
        "  have=\"\"\n"
        "  for uuid in $(nmcli -t -f UUID connection show); do\n"
        "    master=$(nmcli -g connection.master connection show uuid \"$uuid\" 2>/dev/null)\n"
        "    if [ \"$master\" = \"$br\" ] || { [ -n \"$bruuid\" ] && [ \"$master\" = \"$bruuid\" ]; }; then\n"
        "      [ \"$(nmcli -g connection.interface-name connection show uuid \"$uuid\" 2>/dev/null)\" = \"$eth\" ] && have=1\n"
        "    fi\n"
        "  done\n"
        "  [ -n \"$have\" ] || nmcli connection add type bridge-slave ifname \"$eth\" master \"$br\"\n"
        "  nmcli connection modify \"$br\" ipv4.method auto ipv6.method auto connection.autoconnect yes\n"
        "  nmcli connection up \"$br\"\n"
        "  echo \"Bridge '$br' is now attached to $eth.\"\n"
        "elif [ -n \"$wifi\" ]; then\n"
        "  echo \"No wired interface found - only Wi-Fi ($wifi) is available.\"\n"
        "  echo \"Wi-Fi cannot join a bridge directly, so '$br' is switched to\"\n"
        "  echo \"shared (NAT) mode: VMs keep using '$br' and reach the network\"\n"
        "  echo \"through the Wi-Fi connection.\"\n"
        "  nmcli connection modify \"$br\" ipv4.method shared ipv6.method ignore connection.autoconnect yes\n"
        "  nmcli connection up \"$br\"\n"
        "  echo \"Bridge '$br' is up in shared/NAT mode over Wi-Fi.\"\n"
        "else\n"
        "  echo \"ERROR: No Ethernet or Wi-Fi device found - nothing to attach.\"\n"
        "fi\n"
    ).arg(bridgeName);

    runScriptInTerminal(script, "fix_bridge");

    QTimer::singleShot(5000, this, [this]() {
        refreshBridges();
        refreshConnectionsList();
        refreshNetworkInfo();
    });
}
