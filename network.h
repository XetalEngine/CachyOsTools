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
    
    // Now iterate through network interfaces and match with stats
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    
    for (const QNetworkInterface &interface : interfaces) {
        // Skip loopback
        if (interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            continue;
        }
        
        QString interfaceName = interface.name(); // Use raw name for /proc/net/dev matching
        QString displayName = interface.humanReadableName();
        if (displayName.isEmpty()) {
            displayName = interfaceName;
        }
        
        QNetworkInterface::InterfaceFlags flags = interface.flags();
        bool isUp = flags.testFlag(QNetworkInterface::IsUp) && flags.testFlag(QNetworkInterface::IsRunning);
        
        // Check if we have stats for this interface
        if (!statsMap.contains(interfaceName)) {
            continue;
        }
        
        QStringList statsParts = statsMap[interfaceName];
        if (statsParts.size() >= 16) {
            // Format: rx_bytes rx_packets rx_errs rx_drop rx_fifo rx_frame rx_compressed rx_multicast
            //         tx_bytes tx_packets tx_errs tx_drop tx_fifo tx_colls tx_carrier tx_compressed
            qint64 rxBytes = statsParts[0].toLongLong();
            qint64 rxPackets = statsParts[1].toLongLong();
            qint64 rxErrors = statsParts[2].toLongLong();
            qint64 txBytes = statsParts[8].toLongLong();
            qint64 txPackets = statsParts[9].toLongLong();
            qint64 txErrors = statsParts[10].toLongLong();
            
            // Get speed from ethtool if available
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
    
    // Use a map to track bridges and avoid duplicates
    QMap<QString, QStringList> bridgeMap;
    
    // First, try using brctl if available (more reliable for interface list)
    QProcess brctlProc;
    brctlProc.start("brctl", QStringList() << "show");
    brctlProc.waitForFinished();
    
    if (brctlProc.exitCode() == 0) {
        QString output = QString::fromUtf8(brctlProc.readAllStandardOutput());
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        
        // Skip header line
        for (int i = 1; i < lines.size(); i++) {
            QStringList parts = lines[i].split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 1) {
                QString bridgeName = parts[0];
                QStringList interfaces;
                for (int j = 1; j < parts.size(); j++) {
                    // Skip STP and other flags
                    if (parts[j] != "yes" && parts[j] != "no" && !parts[j].isEmpty()) {
                        interfaces << parts[j];
                    }
                }
                bridgeMap[bridgeName] = interfaces;
            }
        }
    } else {
        // Fallback to ip command if brctl is not available
        QProcess proc;
        proc.start("ip", QStringList() << "link" << "show" << "type" << "bridge");
        proc.waitForFinished();
        
        if (proc.exitCode() == 0) {
            QString output = QString::fromUtf8(proc.readAllStandardOutput());
            QStringList lines = output.split('\n', Qt::SkipEmptyParts);
            
            QString currentBridge;
            for (const QString &line : lines) {
                QString trimmed = line.trimmed();
                // Extract bridge name from lines like "2: br0: <BROADCAST,MULTICAST,UP,LOWER_UP>"
                QRegularExpression re("^\\d+:\\s+(\\S+):");
                QRegularExpressionMatch match = re.match(trimmed);
                if (match.hasMatch()) {
                    currentBridge = match.captured(1);
                    if (!bridgeMap.contains(currentBridge)) {
                        bridgeMap[currentBridge] = QStringList();
                    }
                }
            }
        }
    }
    
    // Now populate the table with unique bridges
    for (auto it = bridgeMap.begin(); it != bridgeMap.end(); ++it) {
        QString bridgeName = it.key();
        QStringList interfaces = it.value();
        
        int row = ui->bridgeTable->rowCount();
        ui->bridgeTable->insertRow(row);
        ui->bridgeTable->setItem(row, 0, new QTableWidgetItem(bridgeName));
        ui->bridgeTable->setItem(row, 1, new QTableWidgetItem(interfaces.join(", ")));
        
        // Check if bridge is up
        QProcess statusProc;
        statusProc.start("ip", QStringList() << "link" << "show" << bridgeName);
        statusProc.waitForFinished();
        QString statusOutput = QString::fromUtf8(statusProc.readAllStandardOutput());
        QString status = statusOutput.contains("UP") ? "Up" : "Down";
        ui->bridgeTable->setItem(row, 2, new QTableWidgetItem(status));
    }
}

void MainWindow::on_refreshBridgeButton_clicked() {
    refreshBridges();
}

void MainWindow::on_createBridgeButton_clicked() {
    bool ok;
    QString bridgeName = QInputDialog::getText(this, "Create Bridge", "Enter bridge name:", QLineEdit::Normal, "br0", &ok);
    
    if (ok && !bridgeName.isEmpty()) {
        QString command = QString("sudo ip link add name %1 type bridge").arg(bridgeName);
        runSudoCommandInTerminal(command);
        
        QTimer::singleShot(2000, this, [this]() {
            refreshBridges();
        });
    }
}

void MainWindow::on_deleteBridgeButton_clicked() {
    QList<QTableWidgetItem*> selected = ui->bridgeTable->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a bridge to delete.");
        return;
    }
    
    int row = selected[0]->row();
    QString bridgeName = ui->bridgeTable->item(row, 0)->text();
    
    int ret = QMessageBox::question(this, "Delete Bridge", 
                                     QString("Are you sure you want to delete bridge '%1'?").arg(bridgeName),
                                     QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        QString command = QString("sudo ip link delete %1").arg(bridgeName);
        runSudoCommandInTerminal(command);
        
        QTimer::singleShot(2000, this, [this]() {
            refreshBridges();
        });
    }
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
        QString command;
        
        if (automaticRadio->isChecked()) {
            // Configure for DHCP
            command = QString("sudo nmcli connection modify %1 ipv4.method auto").arg(interfaceName);
            // If connection doesn't exist, create it
            QProcess checkProc;
            checkProc.start("nmcli", QStringList() << "connection" << "show" << interfaceName);
            checkProc.waitForFinished();
            if (checkProc.exitCode() != 0) {
                command = QString("sudo nmcli connection add type ethernet con-name %1 ifname %2 ipv4.method auto").arg(interfaceName).arg(interfaceName);
            }
        } else {
            // Configure for static IP
            QString ip = ipEdit->text();
            QString netmask = netmaskEdit->text();
            QString gateway = gatewayEdit->text();
            QString dns = dnsEdit->text();
            
            if (ip.isEmpty() || netmask.isEmpty()) {
                QMessageBox::warning(dialog, "Invalid Input", "IP Address and Netmask are required for manual configuration.");
                return;
            }
            
            QString cidr = convertNetmaskToCIDR(netmask);
            command = QString("sudo nmcli connection modify %1 ipv4.method manual ipv4.addresses %2/%3").arg(interfaceName).arg(ip).arg(cidr);
            
            if (!gateway.isEmpty()) {
                command += QString(" ipv4.gateway %1").arg(gateway);
            }
            if (!dns.isEmpty()) {
                command += QString(" ipv4.dns %1").arg(dns.replace(" ", ""));
            }
            
            // If connection doesn't exist, create it
            QProcess checkProc;
            checkProc.start("nmcli", QStringList() << "connection" << "show" << interfaceName);
            checkProc.waitForFinished();
            if (checkProc.exitCode() != 0) {
                QString createCmd = QString("sudo nmcli connection add type ethernet con-name %1 ifname %2").arg(interfaceName).arg(interfaceName);
                runSudoCommandInTerminal(createCmd + " && " + command);
            } else {
                runSudoCommandInTerminal(command);
            }
            
            // Bring interface up
            runSudoCommandInTerminal(QString("sudo nmcli connection up %1").arg(interfaceName));
        }
        
        if (!command.isEmpty()) {
            runSudoCommandInTerminal(command);
            QMessageBox::information(dialog, "Configuration Applied", 
                                     QString("Configuration for %1 has been applied. The interface will be restarted.").arg(interfaceName));
        }
        
        dialog->accept();
        QTimer::singleShot(2000, this, [this]() {
            refreshInterfaceConfig();
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
