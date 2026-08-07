
// Dashboard -> Devices sub-tab: Windows-style Device Manager.
// Tree of PCI + USB devices with driver info; module actions with guardrails.

#include <QTreeWidget>
#include <QPlainTextEdit>

void MainWindow::setupDeviceManagerTab() {
    QVBoxLayout *layout = new QVBoxLayout(ui->deviceManagerSubTab);

    QHBoxLayout *bar = new QHBoxLayout();
    QLabel *hint = new QLabel(tr("Every PCI and USB device, the kernel driver it uses, and its modules."), ui->deviceManagerSubTab);
    hint->setStyleSheet("color:#888;");
    QPushButton *refreshBtn = new QPushButton(tr("🔄 Refresh"), ui->deviceManagerSubTab);
    QPushButton *infoBtn = new QPushButton(tr("ℹ️ Module Info"), ui->deviceManagerSubTab);
    QPushButton *loadBtn = new QPushButton(tr("▶️ Load Module"), ui->deviceManagerSubTab);
    QPushButton *unloadBtn = new QPushButton(tr("⏏️ Unload Module"), ui->deviceManagerSubTab);
    QPushButton *blacklistBtn = new QPushButton(tr("🚫 Blacklist Module"), ui->deviceManagerSubTab);
    bar->addWidget(hint);
    bar->addStretch();
    bar->addWidget(refreshBtn);
    bar->addWidget(infoBtn);
    bar->addWidget(loadBtn);
    bar->addWidget(unloadBtn);
    bar->addWidget(blacklistBtn);
    layout->addLayout(bar);

    devMgrTree = new QTreeWidget(ui->deviceManagerSubTab);
    devMgrTree->setColumnCount(3);
    devMgrTree->setHeaderLabels(QStringList() << tr("Device") << tr("Driver in use") << tr("Kernel modules"));
    devMgrTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    devMgrTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    devMgrTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    devMgrTree->setAlternatingRowColors(true);
    layout->addWidget(devMgrTree);

    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshDeviceManager);

    // The module a selected row refers to: driver in use, else first kernel module
    auto selectedModule = [this]() -> QString {
        QTreeWidgetItem *item = devMgrTree->currentItem();
        if (!item || !item->parent()) return QString();
        QString mod = item->text(1);
        if (mod.isEmpty() || mod == "-") mod = item->text(2).section(',', 0, 0).trimmed();
        return (mod == "-") ? QString() : mod.trimmed();
    };

    connect(infoBtn, &QPushButton::clicked, this, [this, selectedModule]() {
        QString mod = selectedModule();
        if (mod.isEmpty()) {
            QMessageBox::information(this, tr("No Module"), tr("Select a device that has a kernel module."));
            return;
        }
        QProcess proc;
        proc.start("modinfo", QStringList() << mod);
        proc.waitForFinished(3000);
        QString out = QString::fromUtf8(proc.readAllStandardOutput());
        if (out.isEmpty()) out = tr("No modinfo available for '%1' (may be built into the kernel).").arg(mod);
        QDialog dlg(this);
        dlg.setWindowTitle(tr("Module Info — %1").arg(mod));
        dlg.resize(700, 500);
        QVBoxLayout *dl = new QVBoxLayout(&dlg);
        QPlainTextEdit *text = new QPlainTextEdit(out, &dlg);
        text->setReadOnly(true);
        text->setFont(QFont("monospace"));
        dl->addWidget(text);
        QPushButton *closeBtn = new QPushButton(tr("Close"), &dlg);
        connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
        dl->addWidget(closeBtn);
        dlg.exec();
    });

    connect(loadBtn, &QPushButton::clicked, this, [this, selectedModule]() {
        QString mod = selectedModule();
        if (mod.isEmpty()) {
            QMessageBox::information(this, tr("No Module"), tr("Select a device that has a kernel module."));
            return;
        }
        runSudoCommandInTerminal(QString("sudo modprobe %1 && echo 'Loaded %1' || echo 'Failed to load %1'; read -p 'Press Enter...'").arg(mod));
    });

    connect(unloadBtn, &QPushButton::clicked, this, [this, selectedModule]() {
        QString mod = selectedModule();
        if (mod.isEmpty()) {
            QMessageBox::information(this, tr("No Module"), tr("Select a device that has a kernel module."));
            return;
        }
        if (QMessageBox::warning(this, tr("Unload Module"),
                tr("Unloading '%1' can take away your display, network or input if it is in use.\n\nContinue?").arg(mod),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
        runSudoCommandInTerminal(QString("sudo modprobe -r %1 && echo 'Unloaded %1' || echo 'Failed (module in use?)'; read -p 'Press Enter...'").arg(mod));
    });

    connect(blacklistBtn, &QPushButton::clicked, this, [this, selectedModule]() {
        QString mod = selectedModule();
        if (mod.isEmpty()) {
            QMessageBox::information(this, tr("No Module"), tr("Select a device that has a kernel module."));
            return;
        }
        if (QMessageBox::warning(this, tr("Blacklist Module"),
                tr("This writes 'blacklist %1' to /etc/modprobe.d/cachyostools-blacklist.conf so it never loads at boot.\n\n"
                   "Blacklisting your GPU, disk or network driver will break the system. Continue?").arg(mod),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
        runSudoCommandInTerminal(QString(
            "echo 'blacklist %1' | sudo tee -a /etc/modprobe.d/cachyostools-blacklist.conf && "
            "echo 'Blacklisted %1. If this driver loads from the initramfs, also run: sudo mkinitcpio -P'; read -p 'Press Enter...'").arg(mod));
    });
}

void MainWindow::refreshDeviceManager() {
    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, proc](int, QProcess::ExitStatus) {
                devMgrTree->clear();
                QHash<QString, QTreeWidgetItem*> groups;
                auto groupItem = [&](const QString &name) {
                    if (!groups.contains(name)) {
                        QTreeWidgetItem *g = new QTreeWidgetItem(devMgrTree);
                        g->setText(0, name);
                        g->setFirstColumnSpanned(true);
                        QFont f = g->font(0); f.setBold(true); g->setFont(0, f);
                        groups.insert(name, g);
                    }
                    return groups.value(name);
                };

                const QString all = QString::fromUtf8(proc->readAllStandardOutput());
                const QStringList sections = all.split("===USB===");
                // --- PCI: a device starts at a non-indented "01:00.0 Class [xxxx]: Name [id]"
                // line; its indented lines carry Subsystem/driver/modules (no blank separators!)
                QString cls, devName, driver, modules;
                auto flushDevice = [&]() {
                    if (devName.isEmpty()) return;
                    QTreeWidgetItem *item = new QTreeWidgetItem(groupItem(cls));
                    item->setText(0, devName);
                    item->setText(1, driver);
                    item->setText(2, modules);
                    if (driver == "-") item->setForeground(1, QColor("#e67e22"));
                    if (driver == "vfio-pci") item->setForeground(1, QColor("#3498db"));
                    devName.clear();
                };
                static const QRegularExpression devStart("^[0-9a-f]{2,4}:[0-9a-f]{2}\\.[0-9a-f] ");
                for (const QString &l : sections.value(0).split('\n', Qt::SkipEmptyParts)) {
                    if (devStart.match(l).hasMatch()) {
                        flushDevice();
                        QString rest = l.mid(l.indexOf(' ') + 1);
                        cls = rest.section(':', 0, 0);              // "VGA compatible controller [0300]"
                        cls.remove(QRegularExpression("\\s*\\[[0-9a-f]{4}\\]"));
                        devName = rest.section(':', 1).trimmed();
                        driver = "-";
                        modules = "-";
                    } else if (l.trimmed().startsWith("Kernel driver in use:")) {
                        driver = l.section(':', 1).trimmed();
                    } else if (l.trimmed().startsWith("Kernel modules:")) {
                        modules = l.section(':', 1).trimmed();
                    }
                }
                flushDevice();
                // --- USB: "Bus 003 Device 002: ID 046d:c52b Logitech ..."
                QTreeWidgetItem *usbGroup = nullptr;
                const QStringList usbLines = sections.value(1).split('\n', Qt::SkipEmptyParts);
                for (const QString &l : usbLines) {
                    QString name = l.section(':', 2).trimmed();     // after "ID xxxx"
                    name = name.mid(name.indexOf(' ') + 1);
                    if (name.isEmpty() || name.contains("root hub", Qt::CaseInsensitive)) continue;
                    if (!usbGroup) {
                        usbGroup = new QTreeWidgetItem(devMgrTree);
                        usbGroup->setText(0, tr("USB Devices"));
                        usbGroup->setFirstColumnSpanned(true);
                        QFont f = usbGroup->font(0); f.setBold(true); usbGroup->setFont(0, f);
                    }
                    QTreeWidgetItem *item = new QTreeWidgetItem(usbGroup);
                    item->setText(0, name);
                    item->setText(1, "usb");
                    item->setText(2, "-");
                }
                devMgrTree->expandAll();
                proc->deleteLater();
            });
    proc->start("bash", QStringList() << "-c" << "lspci -nnk; echo; echo ===USB===; lsusb");
}
