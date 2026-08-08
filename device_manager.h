
// Dashboard -> Devices sub-tab: Windows-style Device Manager, Linux-deep.
// Tree of PCI + USB devices with driver info, IRQ, PCIe link speed and IOMMU
// group; right-click for a full Properties dialog and module actions.

#include <QTreeWidget>
#include <QPlainTextEdit>
#include <QMenu>
#include <QLineEdit>
#include <QTextBrowser>

// Every field the collector emits lands in this map, stored on the tree item.
static QVariantMap devMgrProps(QTreeWidgetItem *item) {
    return item ? item->data(0, Qt::UserRole).toMap() : QVariantMap();
}

// All hardware dumps (vBIOS ROMs, firmware) land in a "dumps" folder beside
// the application, created on demand and kept out of git.
static QString xetalDumpsDir() {
    // Prefer the project/install root (where CMakeLists.txt lives) so dumps land
    // in one place whether the app was started from the root or a build dir.
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 4; ++i) {
        if (dir.exists("CMakeLists.txt")) break;
        if (!dir.cdUp()) { dir.setPath(QCoreApplication::applicationDirPath()); break; }
    }
    const QString path = dir.absolutePath() + "/dumps";
    QDir().mkpath(path);
    return path;
}

// "16.0 GT/s PCIe" -> 16.0 ; used to spot links running below their capacity
static double devMgrLinkGTs(const QString &s) {
    return QRegularExpression("([0-9.]+)\\s*GT/s").match(s).captured(1).toDouble();
}

void MainWindow::setupDeviceManagerTab() {
    QVBoxLayout *layout = new QVBoxLayout(ui->deviceManagerSubTab);

    QHBoxLayout *bar = new QHBoxLayout();
    devMgrFilter = new QLineEdit(ui->deviceManagerSubTab);
    devMgrFilter->setPlaceholderText(tr("🔍 Filter devices, vendors or drivers..."));
    devMgrFilter->setClearButtonEnabled(true);
    devMgrFilter->setMaximumWidth(320);
    devMgrCountLabel = new QLabel(ui->deviceManagerSubTab);
    devMgrCountLabel->setStyleSheet("color:#888;");
    QPushButton *refreshBtn = new QPushButton(tr("🔄 Refresh"), ui->deviceManagerSubTab);
    QPushButton *propsBtn = new QPushButton(tr("🔎 Properties"), ui->deviceManagerSubTab);
    QPushButton *exportBtn = new QPushButton(tr("📄 Export Report"), ui->deviceManagerSubTab);
    QPushButton *dumpsBtn = new QPushButton(tr("📂 Dumps"), ui->deviceManagerSubTab);
    dumpsBtn->setToolTip(tr("Open the folder holding vBIOS/firmware dumps"));
    QPushButton *fwDumpBtn = new QPushButton(tr("💾 Dump Firmware ▾"), ui->deviceManagerSubTab);
    fwDumpBtn->setToolTip(tr("Dump system firmware tables (ACPI, SMBIOS, monitor EDID)"));
    bar->addWidget(devMgrFilter);
    bar->addWidget(devMgrCountLabel);
    bar->addStretch();
    bar->addWidget(propsBtn);
    bar->addWidget(fwDumpBtn);
    bar->addWidget(dumpsBtn);
    bar->addWidget(exportBtn);
    bar->addWidget(refreshBtn);
    layout->addLayout(bar);

    QLabel *hint = new QLabel(tr("Double-click or right-click a device for full properties, driver details and module actions."), ui->deviceManagerSubTab);
    hint->setStyleSheet("color:#888;");
    layout->addWidget(hint);

    devMgrTree = new QTreeWidget(ui->deviceManagerSubTab);
    devMgrTree->setObjectName("devMgrTree");
    devMgrTree->setColumnCount(6);
    devMgrTree->setHeaderLabels(QStringList()
        << tr("Device") << tr("Vendor") << tr("Driver") << tr("Address") << tr("IRQ") << tr("Link / Speed"));
    devMgrTree->header()->setSectionResizeMode(QHeaderView::Interactive);
    devMgrTree->header()->setStretchLastSection(true);
    devMgrTree->setAlternatingRowColors(true);
    devMgrTree->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(devMgrTree);

    connect(fwDumpBtn, &QPushButton::clicked, this, [this, fwDumpBtn]() {
        QMenu menu(this);
        menu.addAction(tr("🧬 ACPI tables (DSDT, IVRS/DMAR, SSDTs)"), this, [this]() { dumpFirmware("acpi"); });
        menu.addAction(tr("🪪 SMBIOS / DMI tables"), this, [this]() { dumpFirmware("dmi"); });
        menu.addAction(tr("🖥️ Monitor EDIDs"), this, [this]() { dumpFirmware("edid"); });
        menu.addSeparator();
        menu.addAction(tr("🔩 Motherboard BIOS (SPI flash)..."), this, [this]() { dumpFirmware("bios"); });
        menu.exec(fwDumpBtn->mapToGlobal(QPoint(0, fwDumpBtn->height())));
    });
    connect(dumpsBtn, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(xetalDumpsDir()));
    });
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshDeviceManager);
    connect(propsBtn, &QPushButton::clicked, this, [this]() { showDeviceProperties(devMgrTree->currentItem()); });
    connect(devMgrTree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int) { showDeviceProperties(item); });

    // Live filter: hides non-matching devices, and any group left empty
    connect(devMgrFilter, &QLineEdit::textChanged, this, [this](const QString &text) {
        for (int g = 0; g < devMgrTree->topLevelItemCount(); ++g) {
            QTreeWidgetItem *group = devMgrTree->topLevelItem(g);
            int visible = 0;
            for (int c = 0; c < group->childCount(); ++c) {
                QTreeWidgetItem *child = group->child(c);
                bool match = text.isEmpty();
                for (int col = 0; col < devMgrTree->columnCount() && !match; ++col)
                    match = child->text(col).contains(text, Qt::CaseInsensitive);
                child->setHidden(!match);
                if (match) ++visible;
            }
            group->setHidden(visible == 0);
        }
    });

    connect(devMgrTree, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QTreeWidgetItem *item = devMgrTree->itemAt(pos);
        if (!item || !item->parent()) return;
        devMgrTree->setCurrentItem(item);
        const QVariantMap p = devMgrProps(item);
        const QString mod = p.value("driver").toString() != "-" ? p.value("driver").toString()
                                                                : p.value("modules").toString().section(',', 0, 0).trimmed();

        QMenu menu(this);
        menu.addAction(tr("🔎 Properties"), this, [this, item]() { showDeviceProperties(item); });
        menu.addSeparator();
        menu.addAction(tr("📋 Copy device ID"), this, [p]() {
            QGuiApplication::clipboard()->setText(p.value("ids").toString());
        });
        menu.addAction(tr("📋 Copy full device line"), this, [item]() {
            QStringList cols;
            for (int c = 0; c < 6; ++c) cols << item->text(c);
            QGuiApplication::clipboard()->setText(cols.join("  |  "));
        });
        if (p.value("hasrom").toString() == "1") {
            menu.addSeparator();
            menu.addAction(tr("💾 Dump vBIOS (for GPU passthrough)"), this, [this, p]() { dumpDeviceVbios(p); });
        }
        menu.addAction(tr("📜 Kernel messages for this device"), this, [this, p]() {
            QProcess proc;
            proc.start("bash", QStringList() << "-c" << QString(
                "journalctl -k -b 2>/dev/null | grep -i '%1' | tail -60").arg(p.value("addr").toString()));
            proc.waitForFinished(5000);
            QString out = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
            if (out.isEmpty()) out = tr("No kernel messages mention this device in the current boot.");
            QDialog dlg(this);
            dlg.setWindowTitle(tr("Kernel messages — %1").arg(p.value("device").toString()));
            dlg.resize(900, 500);
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
        menu.addSeparator();
        menu.addAction(tr("🌐 Search this device online"), this, [p]() {
            QDesktopServices::openUrl(QUrl("https://duckduckgo.com/?q=" +
                QUrl::toPercentEncoding(p.value("vendor").toString() + " " + p.value("device").toString() + " linux driver")));
        });
        if (!mod.isEmpty() && mod != "-") {
            menu.addSeparator();
            menu.addAction(tr("ℹ️ Module info (%1)").arg(mod), this, [this, mod]() { showModuleInfo(mod); });
            menu.addAction(tr("▶️ Load module"), this, [this, mod]() {
                runSudoCommandInTerminal(QString("sudo modprobe %1 && echo 'Loaded %1' || echo 'Failed to load %1'; read -p 'Press Enter...'").arg(mod));
            });
            menu.addAction(tr("⏏️ Unload module"), this, [this, mod]() {
                if (QMessageBox::warning(this, tr("Unload Module"),
                        tr("Unloading '%1' can take away your display, network or input if it is in use.\n\nContinue?").arg(mod),
                        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
                runSudoCommandInTerminal(QString("sudo modprobe -r %1 && echo 'Unloaded %1' || echo 'Failed (module in use?)'; read -p 'Press Enter...'").arg(mod));
            });
            menu.addAction(tr("🚫 Blacklist module"), this, [this, mod]() {
                if (QMessageBox::warning(this, tr("Blacklist Module"),
                        tr("This writes 'blacklist %1' to /etc/modprobe.d/cachyostools-blacklist.conf so it never loads at boot.\n\n"
                           "Blacklisting your GPU, disk or network driver will break the system. Continue?").arg(mod),
                        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
                runSudoCommandInTerminal(QString(
                    "echo 'blacklist %1' | sudo tee -a /etc/modprobe.d/cachyostools-blacklist.conf && "
                    "echo 'Blacklisted %1. If this driver loads from the initramfs, also run: sudo mkinitcpio -P'; read -p 'Press Enter...'").arg(mod));
            });
        }
        menu.exec(devMgrTree->viewport()->mapToGlobal(pos));
    });

    connect(exportBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getSaveFileName(this, tr("Export Device Report"),
                           QDir::homePath() + "/device-report.txt", tr("Text files (*.txt)"));
        if (path.isEmpty()) return;
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, tr("Export Failed"), tr("Could not write %1").arg(path));
            return;
        }
        QTextStream out(&f);
        out << "CachyOsTools device report — " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n\n";
        for (int g = 0; g < devMgrTree->topLevelItemCount(); ++g) {
            QTreeWidgetItem *group = devMgrTree->topLevelItem(g);
            out << "=== " << group->text(0) << " ===\n";
            for (int c = 0; c < group->childCount(); ++c) {
                QTreeWidgetItem *child = group->child(c);
                out << "  " << child->text(3) << "  " << child->text(0) << "\n"
                    << "      vendor: " << child->text(1) << " | driver: " << child->text(2)
                    << " | irq: " << child->text(4) << " | link: " << child->text(5) << "\n";
            }
            out << "\n";
        }
        f.close();
        QMessageBox::information(this, tr("Exported"), tr("Device report written to:\n%1").arg(path));
    });
}

void MainWindow::showModuleInfo(const QString &mod) {
    QProcess proc;
    proc.start("modinfo", QStringList() << mod);
    proc.waitForFinished(3000);
    QString out = QString::fromUtf8(proc.readAllStandardOutput());
    if (out.isEmpty()) out = tr("No modinfo available for '%1' (may be built into the kernel).").arg(mod);
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Module Info — %1").arg(mod));
    dlg.resize(760, 560);
    QVBoxLayout *dl = new QVBoxLayout(&dlg);
    QPlainTextEdit *text = new QPlainTextEdit(out, &dlg);
    text->setReadOnly(true);
    text->setFont(QFont("monospace"));
    dl->addWidget(text);
    QPushButton *closeBtn = new QPushButton(tr("Close"), &dlg);
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    dl->addWidget(closeBtn);
    dlg.exec();
}

// Windows-style "Properties" — everything Linux knows about one device.
void MainWindow::showDeviceProperties(QTreeWidgetItem *item) {
    if (!item || !item->parent()) {
        QMessageBox::information(this, tr("No Selection"), tr("Select a device first."));
        return;
    }
    const QVariantMap p = devMgrProps(item);
    const bool isUsb = p.value("type").toString() == "usb";

    auto row = [](const QString &k, const QString &v, const QString &color = QString()) {
        if (v.isEmpty() || v == "-") return QString();
        return QString("<tr><td style='color:#888; padding:3px 18px 3px 0; white-space:nowrap;'>%1</td>"
                       "<td style='padding:3px 0; %2'>%3</td></tr>")
               .arg(k, color.isEmpty() ? "" : "color:" + color + "; font-weight:bold;", v.toHtmlEscaped());
    };
    auto section = [](const QString &title) {
        return QString("<tr><td colspan='2' style='padding:14px 0 4px 0;'><b style='color:#27ae60;'>%1</b><hr></td></tr>").arg(title);
    };

    QString html = QString("<h2 style='color:#27ae60;'>%1</h2><table>").arg(p.value("device").toString().toHtmlEscaped());

    html += section(tr("General"));
    html += row(tr("Device"), p.value("device").toString());
    html += row(tr("Vendor"), p.value("vendor").toString());
    html += row(tr("Class"), p.value("class").toString());
    html += row(isUsb ? tr("Bus / Port") : tr("PCI address"), p.value("addr").toString());
    html += row(tr("Hardware IDs"), p.value("ids").toString());
    html += row(tr("Subsystem"), p.value("subsystem").toString());
    html += row(tr("Revision"), p.value("rev").toString());
    if (isUsb) {
        html += row(tr("Serial number"), p.value("serial").toString());
        html += row(tr("USB version"), p.value("usbver").toString());
        html += row(tr("Max power draw"), p.value("maxpower").toString());
    }

    html += section(tr("Driver"));
    const QString driver = p.value("driver").toString();
    html += row(tr("Driver in use"), driver == "-" ? tr("(none — device is unclaimed)") : driver,
                driver == "-" ? "#e67e22" : (driver == "vfio-pci" ? "#3498db" : QString()));
    html += row(tr("Kernel modules"), p.value("modules").toString());
    if (driver == "vfio-pci")
        html += row(tr("Note"), tr("Bound to VFIO — reserved for GPU/device passthrough to a VM."), "#3498db");
    // modinfo details for the active driver (cheap, cached by the kernel)
    if (driver != "-" && !driver.isEmpty()) {
        QProcess mi;
        mi.start("modinfo", QStringList() << driver);
        mi.waitForFinished(2500);
        for (const QString &line : QString::fromUtf8(mi.readAllStandardOutput()).split('\n')) {
            const QString key = line.section(':', 0, 0).trimmed();
            const QString val = line.section(':', 1).trimmed();
            if (val.isEmpty()) continue;
            if (key == "version")     html += row(tr("Module version"), val);
            else if (key == "license")     html += row(tr("License"), val);
            else if (key == "description") html += row(tr("Description"), val);
            else if (key == "author")      html += row(tr("Author"), val);
            else if (key == "signer")      html += row(tr("Signed by"), val, "#27ae60");
            else if (key == "sig_hashalgo") html += row(tr("Signature algorithm"), val);
        }
    }

    // What this controller actually drives (NIC, NVMe) and how hot it runs
    if (!p.value("netif").toString().isEmpty() || !p.value("diskmodel").toString().isEmpty()
        || !p.value("temp").toString().isEmpty()) {
        html += section(tr("Attached hardware"));
        html += row(tr("Network interface"), p.value("netif").toString());
        html += row(tr("MAC address"), p.value("mac").toString());
        html += row(tr("Link speed"), p.value("netspeed").toString().isEmpty() ? QString()
                    : tr("%1 Mb/s").arg(p.value("netspeed").toString()));
        html += row(tr("Disk model"), p.value("diskmodel").toString());
        html += row(tr("Disk firmware"), p.value("diskfw").toString());
        html += row(tr("Disk serial"), p.value("diskserial").toString());
        if (!p.value("temp").toString().isEmpty()) {
            const int t = p.value("temp").toInt();
            html += row(tr("Temperature"), tr("%1 °C").arg(t),
                        t >= 80 ? "#c0392b" : (t >= 65 ? "#e67e22" : "#27ae60"));
        }
    }

    html += section(tr("Resources"));
    const QString irq = p.value("irq").toString();
    if (!irq.isEmpty() && irq != "0" && irq != "-") {
        QProcess ic;
        ic.start("bash", QStringList() << "-c" << QString(
            "awk -F: '$1+0==%1 {s=0; for(i=2;i<=NF;i++){n=split($i,a,\" \"); for(j=1;j<=n;j++) if(a[j]~/^[0-9]+$/) s+=a[j]} print s}' /proc/interrupts | head -1").arg(irq));
        ic.waitForFinished(2000);
        const QString count = QString::fromUtf8(ic.readAllStandardOutput()).trimmed();
        html += row(tr("IRQ"), count.isEmpty() ? irq : tr("%1  (%2 interrupts since boot)").arg(irq, count));
    }
    html += row(tr("IOMMU group"), p.value("iommu").toString().isEmpty() ? QString()
                : tr("%1 — used for VFIO/GPU passthrough").arg(p.value("iommu").toString()));
    html += row(tr("NUMA node"), p.value("numa").toString() == "-1" ? tr("none (single-node system)") : p.value("numa").toString());
    if (!p.value("resources").toString().isEmpty())
        html += QString("<tr><td style='color:#888; padding:3px 18px 3px 0; vertical-align:top;'>%1</td>"
                        "<td style='padding:3px 0;'><pre style='margin:0; color:#9cdcfe;'>%2</pre></td></tr>")
                .arg(tr("Memory / I/O"), p.value("resources").toString().toHtmlEscaped());

    if (!isUsb) {
        html += row(tr("Interrupt mode"), p.value("msi").toString().isEmpty() ? tr("legacy IRQ")
                    : tr("MSI/MSI-X (%1 vector(s))").arg(p.value("msi").toString()));
        if (p.value("bootvga").toString() == "1")
            html += row(tr("Primary display"), tr("yes — this is the GPU the system booted with"));

        // Everything a passthrough attempt depends on, in one place
        html += section(tr("VM passthrough readiness"));
        const QString reset = p.value("reset").toString();
        html += row(tr("Reset support"), reset.isEmpty() ? tr("⚠ none advertised — the card may not survive a VM restart")
                    : tr("%1%2").arg(reset, reset.contains("flr") ? tr("  (FLR — clean reset between VM boots)") : QString()),
                    reset.isEmpty() ? "#e67e22" : "#27ae60");
        html += row(tr("IOMMU group"), p.value("iommu").toString());
        html += row(tr("vBIOS ROM"), p.value("hasrom").toString() == "1"
                    ? tr("available — right-click the device to dump it") : tr("not exposed by this device"));
        if (p.value("bootvga").toString() == "1")
            html += row(tr("Note"), tr("Boot GPU: single-GPU passthrough needs hook scripts (see the KVM tab)."), "#888");
    }

    if (!isUsb && !p.value("linkcur").toString().isEmpty()) {
        html += section(tr("PCI Express link"));
        const double cur = devMgrLinkGTs(p.value("linkcur").toString());
        const double max = devMgrLinkGTs(p.value("linkmax").toString());
        const int curW = p.value("widthcur").toInt(), maxW = p.value("widthmax").toInt();
        const bool degraded = (max > 0 && cur > 0 && cur < max) || (maxW > 0 && curW < maxW);
        html += row(tr("Current"), tr("%1  ×%2").arg(p.value("linkcur").toString()).arg(curW));
        html += row(tr("Maximum"), tr("%1  ×%2").arg(p.value("linkmax").toString()).arg(maxW));
        if (degraded)
            html += row(tr("Status"), (curW < maxW && maxW > 0)
                        ? tr("⚠ Running at reduced width — check the slot or riser")
                        : tr("Link speed is below maximum (normal at idle — GPUs downclock the link to save power)"),
                        (curW < maxW && maxW > 0) ? "#e67e22" : "#888");
        else
            html += row(tr("Status"), tr("✅ Running at full speed and width"), "#27ae60");
    }

    if (!isUsb) {
        html += section(tr("Power"));
        html += row(tr("Power state"), p.value("power").toString());
        html += row(tr("D3cold allowed"), p.value("d3cold").toString() == "1" ? tr("yes") : tr("no"));
    } else {
        html += section(tr("Connection"));
        html += row(tr("Speed"), p.value("link").toString());
    }
    html += "</table>";

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Properties — %1").arg(p.value("device").toString()));
    dlg.resize(720, 640);
    QVBoxLayout *dl = new QVBoxLayout(&dlg);
    QTextBrowser *view = new QTextBrowser(&dlg);
    view->setHtml(html);
    view->setOpenExternalLinks(true);
    dl->addWidget(view);
    QHBoxLayout *btns = new QHBoxLayout();
    QPushButton *copyBtn = new QPushButton(tr("📋 Copy"), &dlg);
    QPushButton *closeBtn = new QPushButton(tr("Close"), &dlg);
    connect(copyBtn, &QPushButton::clicked, this, [view]() {
        QGuiApplication::clipboard()->setText(view->toPlainText());
    });
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    btns->addStretch(); btns->addWidget(copyBtn); btns->addWidget(closeBtn);
    dl->addLayout(btns);
    dlg.exec();
}

void MainWindow::refreshDeviceManager() {
    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, proc](int, QProcess::ExitStatus) {
                const QString saved = devMgrFilter->text();
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

                int pciCount = 0, usbCount = 0, unclaimed = 0;
                const QString all = QString::fromUtf8(proc->readAllStandardOutput());

                // Pass 1: the @EXTRA blocks, keyed by PCI address
                QHash<QString, QVariantMap> extras;
                for (const QString &block : all.split("@EXTRA\n", Qt::SkipEmptyParts)) {
                    QVariantMap e;
                    for (const QString &line : block.split('\n')) {
                        if (line.startsWith("@DEV")) break;          // extras section ended
                        const int eq = line.indexOf('=');
                        if (eq > 0) e.insert(line.left(eq), line.mid(eq + 1));
                    }
                    const QString addr = e.value("addr").toString();
                    if (!addr.isEmpty() && e.size() > 1) extras.insert(addr, e);
                }

                // Pass 2: the devices themselves
                const QStringList blocks = all.split("@DEV\n", Qt::SkipEmptyParts);
                for (const QString &block : blocks) {
                    QVariantMap p;
                    QString resources;
                    for (const QString &line : block.split('\n')) {
                        if (line.startsWith("@EXTRA")) break;
                        const int eq = line.indexOf('=');
                        if (eq <= 0) continue;
                        const QString key = line.left(eq), val = line.mid(eq + 1);
                        if (key == "res") resources += (resources.isEmpty() ? "" : "\n") + val;
                        else p.insert(key, val);
                    }
                    if (p.value("device").toString().isEmpty()) continue;
                    p.insert("resources", resources);
                    // merge the sysfs extras for this address
                    const QVariantMap e = extras.value(p.value("addr").toString());
                    for (auto it = e.constBegin(); it != e.constEnd(); ++it)
                        if (it.key() != "addr") p.insert(it.key(), it.value());

                    const bool isUsb = p.value("type").toString() == "usb";
                    QTreeWidgetItem *item = new QTreeWidgetItem(groupItem(isUsb ? tr("USB Devices") : p.value("class").toString()));
                    item->setText(0, p.value("device").toString());
                    item->setText(1, p.value("vendor").toString());
                    item->setText(2, p.value("driver").toString());
                    item->setText(3, p.value("addr").toString());
                    item->setText(4, p.value("irq").toString() == "0" ? "" : p.value("irq").toString());
                    item->setText(5, isUsb ? p.value("link").toString()
                                           : (p.value("linkcur").toString().isEmpty() ? QString()
                                              : QString("%1 ×%2").arg(p.value("linkcur").toString().remove(" PCIe"),
                                                                      p.value("widthcur").toString())));
                    item->setData(0, Qt::UserRole, p);
                    item->setToolTip(0, tr("%1\n%2\nDouble-click for full properties").arg(
                        p.value("device").toString(), p.value("ids").toString()));

                    const QString driver = p.value("driver").toString();
                    if (driver == "-") { item->setForeground(2, QColor("#e67e22")); ++unclaimed; }
                    if (driver == "vfio-pci") item->setForeground(2, QColor("#3498db"));
                    // degraded PCIe width is a real hardware problem worth flagging
                    if (!isUsb && p.value("widthmax").toInt() > 0
                        && p.value("widthcur").toInt() < p.value("widthmax").toInt())
                        item->setForeground(5, QColor("#e67e22"));
                    isUsb ? ++usbCount : ++pciCount;
                }

                devMgrTree->expandAll();
                for (int c = 0; c < 5; ++c) devMgrTree->resizeColumnToContents(c);
                devMgrCountLabel->setText(tr("%1 PCI · %2 USB · %3 unclaimed")
                                          .arg(pciCount).arg(usbCount).arg(unclaimed));
                if (!saved.isEmpty()) emit devMgrFilter->textChanged(saved);   // re-apply live filter
                proc->deleteLater();
            });

    // One pass: lspci machine-readable + sysfs enrichment (awk reads sysfs directly),
    // then the USB tree from /sys/bus/usb.
    proc->start("bash", QStringList() << "-c" << R"SCRIPT(
RES=$(mktemp) ; trap 'rm -f "$RES"' EXIT
lspci -Dv 2>/dev/null | awk '
/^[0-9a-f]{4}:/ { slot=$1; next }
/^\t(Memory at|I\/O ports at|Expansion ROM)/ { gsub(/^\t/, ""); print slot "\t" $0 }
' > "$RES"

lspci -Dvmmnnk | awk -v resfile="$RES" '
function rd(p,  v) { v=""; if ((getline v < p) <= 0) v=""; close(p); return v }
BEGIN {
  while ((getline line < resfile) > 0) {
    t = index(line, "\t")
    s = substr(line, 1, t-1); r = substr(line, t+1)
    res[s] = (s in res) ? res[s] "\n" r : r
  }
  close(resfile)
}
function flush_dev(  n, i, parts) {
  if (slot == "") return
  d = "/sys/bus/pci/devices/" slot "/"
  printf "@DEV\ntype=pci\naddr=%s\nclass=%s\nvendor=%s\ndevice=%s\nsubsystem=%s\nrev=%s\ndriver=%s\nmodules=%s\nids=%s\n",
    slot, cls, vend, dev, subsys, (rev==""?"-":rev), (drv==""?"-":drv), (mods==""?"-":mods), ids
  printf "irq=%s\niommu=%s\nnuma=%s\npower=%s\nd3cold=%s\nlinkcur=%s\nlinkmax=%s\nwidthcur=%s\nwidthmax=%s\n",
    rd(d "irq"), iommu, rd(d "numa_node"), rd(d "power_state"), rd(d "d3cold_allowed"),
    rd(d "current_link_speed"), rd(d "max_link_speed"), rd(d "current_link_width"), rd(d "max_link_width")
  if (slot in res) {
    n = split(res[slot], parts, "\n")
    for (i = 1; i <= n; i++) printf "res=%s\n", parts[i]
  }
  slot=""; cls=""; vend=""; dev=""; subsys=""; svend=""; rev=""; drv=""; mods=""; ids=""; iommu=""
}
/^Slot:/       { flush_dev(); slot=$2 }
/^Class:/      { sub(/^Class:[ \t]+/, "", $0); cls=$0; gsub(/ \[[0-9a-f]{4}\]$/, "", cls) }
/^Vendor:/     { sub(/^Vendor:[ \t]+/, "", $0); vend=$0; vid=$0; gsub(/^.*\[/, "", vid); gsub(/\]$/, "", vid); gsub(/ \[[0-9a-f]{4}\]$/, "", vend) }
/^Device:/     { sub(/^Device:[ \t]+/, "", $0); dev=$0; did=$0; gsub(/^.*\[/, "", did); gsub(/\]$/, "", did); gsub(/ \[[0-9a-f]{4}\]$/, "", dev); ids=vid ":" did }
/^SVendor:/    { sub(/^SVendor:[ \t]+/, "", $0); svend=$0 }
/^SDevice:/    { sub(/^SDevice:[ \t]+/, "", $0); subsys = svend " " $0 }
/^Rev:/        { rev=$2 }
/^Driver:/     { drv=$2 }
/^Module:/     { mods = (mods=="" ? $2 : mods ", " $2) }
/^IOMMUGroup:/ { iommu=$2 }
END { flush_dev() }
'
# Per-device extras that need directory listings (merged by address in C++):
# vBIOS ROM presence, reset support, boot GPU flag, MSI count, temperature,
# and any attached network interface or NVMe disk.
for d in /sys/bus/pci/devices/*/; do
  a=$(basename "$d")
  printf "@EXTRA\naddr=%s\n" "$a"
  [ -e "$d/rom" ] && echo "hasrom=1"
  r=$(cat "$d/reset_method" 2>/dev/null); [ -n "$r" ] && echo "reset=$r"
  [ "$(cat "$d/boot_vga" 2>/dev/null)" = "1" ] && echo "bootvga=1"
  n=$(ls "$d/msi_irqs" 2>/dev/null | wc -l); [ "$n" -gt 0 ] && echo "msi=$n"
  for h in "$d"hwmon/hwmon*/temp1_input "$d"nvme/*/hwmon*/temp1_input; do
    [ -f "$h" ] && { t=$(cat "$h" 2>/dev/null); [ -n "$t" ] && echo "temp=$((t/1000))"; break; }
  done
  for ni in "$d"net/*/; do
    [ -d "$ni" ] || continue
    echo "netif=$(basename "$ni")"
    echo "mac=$(cat "$ni/address" 2>/dev/null)"
    s=$(cat "$ni/speed" 2>/dev/null); [ -n "$s" ] && [ "$s" != "-1" ] && echo "netspeed=$s"
  done
  for nv in "$d"nvme/*/; do
    [ -d "$nv" ] || continue
    echo "diskmodel=$(cat "$nv/model" 2>/dev/null | xargs)"
    echo "diskfw=$(cat "$nv/firmware_rev" 2>/dev/null | xargs)"
    echo "diskserial=$(cat "$nv/serial" 2>/dev/null | xargs)"
  done
done

# USB devices: real hardware only (skip hubs/interfaces)
for u in /sys/bus/usb/devices/*/; do
  [ -f "$u/idVendor" ] || continue
  cls=$(cat "$u/bDeviceClass" 2>/dev/null)
  [ "$cls" = "09" ] && continue                       # hubs
  vend=$(cat "$u/manufacturer" 2>/dev/null); prod=$(cat "$u/product" 2>/dev/null)
  [ -n "$prod" ] || continue
  spd=$(cat "$u/speed" 2>/dev/null)
  case "$spd" in
    1.5) spdtxt="1.5 Mb/s (USB 1.0 low)";; 12) spdtxt="12 Mb/s (USB 1.1)";;
    480) spdtxt="480 Mb/s (USB 2.0)";; 5000) spdtxt="5 Gb/s (USB 3.0)";;
    10000) spdtxt="10 Gb/s (USB 3.1)";; 20000) spdtxt="20 Gb/s (USB 3.2)";; *) spdtxt="$spd Mb/s";;
  esac
  drv="-"
  for i in "$u"*:*/; do
    [ -L "$i/driver" ] && { d=$(basename "$(readlink "$i/driver")"); drv=$([ "$drv" = "-" ] && echo "$d" || echo "$drv, $d"); }
  done
  printf "@DEV\ntype=usb\naddr=%s\nclass=USB\nvendor=%s\ndevice=%s\ndriver=%s\nmodules=-\nids=%s:%s\nserial=%s\nusbver=%s\nmaxpower=%s\nlink=%s\nirq=\n" \
    "$(basename "$u")" "${vend:-Unknown}" "$prod" "$drv" \
    "$(cat "$u/idVendor" 2>/dev/null)" "$(cat "$u/idProduct" 2>/dev/null)" \
    "$(cat "$u/serial" 2>/dev/null)" "$(cat "$u/version" 2>/dev/null | tr -d ' ')" \
    "$(cat "$u/bMaxPower" 2>/dev/null)" "$spdtxt"
done
)SCRIPT");
}

// vBIOS dump, GPU-Z style but native: the kernel exposes the card's ROM at
// /sys/bus/pci/devices/<addr>/rom once it is enabled for reading.
// For passthrough, NVIDIA ROMs additionally need their pre-header stripped —
// we offer to do that here so no hex editor is required.
void MainWindow::dumpDeviceVbios(const QVariantMap &p) {
    const QString addr = p.value("addr").toString();
    const QString romPath = "/sys/bus/pci/devices/" + addr + "/rom";
    if (!QFile::exists(romPath)) {
        QMessageBox::information(this, tr("No ROM"),
            tr("This device does not expose a readable ROM."));
        return;
    }

    QString notice = tr("The card's ROM will be read directly from the kernel (needs sudo).");
    if (p.value("bootvga").toString() == "1")
        notice += tr("\n\n⚠ This is the GPU your system booted with, so the dump may be the "
                     "BIOS-shadowed copy rather than a pristine image. If the VM later refuses "
                     "to boot with it, grab the exact ROM for your card from TechPowerUp instead.");
    if (QMessageBox::question(this, tr("Dump vBIOS"),
            tr("%1\n\nContinue?").arg(notice)) != QMessageBox::Yes) return;

    // Timestamped so repeat dumps never silently overwrite an earlier one
    const QString name = QString(p.value("device").toString())
                             .remove(QRegularExpression("[^A-Za-z0-9._-]+")).left(40);
    const QString suggested = QString("%1/vbios-%2-%3.rom")
        .arg(xetalDumpsDir(), name, QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss"));
    const QString outPath = QFileDialog::getSaveFileName(this, tr("Save vBIOS ROM"), suggested, tr("ROM images (*.rom)"));
    if (outPath.isEmpty()) return;

    // Enable ROM reading, copy it out, disable again — all as root, visibly.
    // Run the terminal as a child process so we know when the dump finished.
    const QString script = QString(
        "echo '== Reading vBIOS of %1'; "
        "echo 1 | sudo tee %2 >/dev/null && "
        "sudo cat %2 > '%3' && sudo chown \"$USER\" '%3'; "
        "echo 0 | sudo tee %2 >/dev/null; "
        "echo; ls -la '%3'; echo; echo '== Done.'; read -p 'Press Enter to close...'")
        .arg(addr, romPath, outPath);

    QString term;
    for (const QString &t : {QString("konsole"), QString("gnome-terminal"), QString("xterm"), QString("alacritty"), QString("kitty")}) {
        if (!QStandardPaths::findExecutable(t).isEmpty()) { term = t; break; }
    }
    if (term.isEmpty()) {
        QMessageBox::warning(this, tr("Terminal Not Found"),
            tr("Could not find a suitable terminal emulator. Please install one of: konsole, gnome-terminal, xterm, alacritty, or kitty"));
        return;
    }
    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, proc, outPath](int, QProcess::ExitStatus) {
                proc->deleteLater();
                QFileInfo fi(outPath);
                if (!fi.exists() || fi.size() == 0) {
                    QMessageBox::warning(this, tr("Dump Failed"),
                        tr("No ROM data was produced.\n\nThis usually means the card refuses to expose its ROM "
                           "while it is in use. Options: dump it from a live USB session, or download the ROM "
                           "for your exact card from TechPowerUp."));
                    return;
                }
                patchVbiosIfNeeded(outPath);
            });
    proc->start(term, QStringList() << "-e" << "bash" << "-c" << script);
}

// NVIDIA ROMs carry a pre-header before the real image; OVMF chokes on it.
// The keeper image starts at the 0x55 0xAA signature that is followed by "VIDEO".
void MainWindow::patchVbiosIfNeeded(const QString &romPath) {
    QFile f(romPath);
    if (!f.open(QIODevice::ReadOnly)) return;
    const QByteArray data = f.readAll();
    f.close();
    if (data.size() < 4) return;

    int keepFrom = -1;
    for (int i = 0; i + 1 < data.size(); ++i) {
        if (static_cast<unsigned char>(data[i]) != 0x55 || static_cast<unsigned char>(data[i + 1]) != 0xAA) continue;
        const int window = qMin(512, data.size() - i);
        if (data.mid(i, window).contains("VIDEO")) keepFrom = i;   // last such signature wins
    }

    QString info = tr("vBIOS saved to:\n%1\n\nSize: %2 KB").arg(romPath).arg(data.size() / 1024);
    if (keepFrom <= 0) {
        info += tr("\n\nNo pre-header found — the file is ready to use as-is\n"
                   "(AMD cards normally need no patching).");
        QMessageBox::information(this, tr("vBIOS Dumped"), info);
        return;
    }

    if (QMessageBox::question(this, tr("Patch for Passthrough?"),
            info + tr("\n\nThis looks like an NVIDIA ROM with a %1-byte pre-header, which stops OVMF "
                      "from initialising the card in a VM.\n\nWrite a patched copy with the header removed?")
                   .arg(keepFrom)) != QMessageBox::Yes) return;

    QString patchedPath = romPath;
    patchedPath.replace(QRegularExpression("\\.rom$"), "");
    patchedPath += "-patched.rom";
    QFile out(patchedPath);
    if (!out.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Patch Failed"), tr("Could not write %1").arg(patchedPath));
        return;
    }
    out.write(data.mid(keepFrom));
    out.close();
    QMessageBox::information(this, tr("Patched vBIOS Ready"),
        tr("Patched ROM written to:\n%1\n\nUse it in the VM's XML:\n"
           "  <rom file=\"%1\"/>\n\n"
           "Tip: copy it to /var/lib/libvirt/vbios/ so libvirt can read it under its own AppArmor/SELinux rules.")
        .arg(patchedPath));
}

// System-wide firmware dumps. Everything here is a plain read of what the
// kernel already exposes — except the motherboard BIOS, which needs flashrom
// and is often blocked by the chipset (explained rather than attempted blindly).
void MainWindow::dumpFirmware(const QString &kind) {
    const QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
    const QString dir = xetalDumpsDir();

    if (kind == "acpi") {
        const QString outDir = QString("%1/acpi-%2").arg(dir, stamp);
        if (QMessageBox::question(this, tr("Dump ACPI Tables"),
                tr("Copies every ACPI table the firmware published (DSDT, SSDTs, and the "
                   "IOMMU tables IVRS/DMAR) into:\n\n%1\n\n"
                   "Handy for passthrough debugging and for decompiling with iasl.\n\nContinue?").arg(outDir))
            != QMessageBox::Yes) return;
        runSudoCommandInTerminal(QString(
            "mkdir -p '%1' && sudo cp /sys/firmware/acpi/tables/* '%1'/ 2>/dev/null; "
            "sudo chown -R \"$USER\" '%1'; "
            "echo; ls -la '%1'; echo; echo '== ACPI tables dumped. Decompile with: iasl -d %1/DSDT'; "
            "read -p 'Press Enter to close...'").arg(outDir));
        return;
    }

    if (kind == "dmi") {
        const QString outFile = QString("%1/smbios-dmi-%2.bin").arg(dir, stamp);
        if (QMessageBox::question(this, tr("Dump SMBIOS / DMI"),
                tr("Saves the raw SMBIOS tables (board, BIOS version, memory layout) to:\n\n%1\n\n"
                   "A readable text copy is written alongside it.\n\nContinue?").arg(outFile))
            != QMessageBox::Yes) return;
        runSudoCommandInTerminal(QString(
            "sudo cp /sys/firmware/dmi/tables/DMI '%1' && sudo chown \"$USER\" '%1'; "
            "command -v dmidecode >/dev/null && sudo dmidecode > '%1.txt' && sudo chown \"$USER\" '%1.txt'; "
            "echo; ls -la '%1'*; echo; echo '== SMBIOS/DMI dumped.'; read -p 'Press Enter to close...'").arg(outFile));
        return;
    }

    if (kind == "edid") {
        QStringList found;
        const QDir drm("/sys/class/drm");
        for (const QString &out : drm.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QFile e("/sys/class/drm/" + out + "/edid");
            if (e.exists() && e.size() > 0) found << out;
        }
        if (found.isEmpty()) {
            QMessageBox::information(this, tr("No EDIDs Available"),
                tr("No monitor EDID is exposed by the kernel right now.\n\n"
                   "The proprietary NVIDIA driver does not publish EDIDs through /sys/class/drm — "
                   "they are available on nouveau, AMD and Intel graphics, or via a live USB session."));
            return;
        }
        const QString outDir = QString("%1/edid-%2").arg(dir, stamp);
        QDir().mkpath(outDir);
        int saved = 0;
        for (const QString &out : found) {
            QFile in("/sys/class/drm/" + out + "/edid");
            if (!in.open(QIODevice::ReadOnly)) continue;
            QFile dst(QString("%1/%2.edid").arg(outDir, out));
            if (dst.open(QIODevice::WriteOnly)) { dst.write(in.readAll()); dst.close(); ++saved; }
            in.close();
        }
        QMessageBox::information(this, tr("EDIDs Dumped"),
            tr("%1 monitor EDID(s) written to:\n%2\n\nUseful for virtual-display spoofing in a VM "
               "(Looking Glass) and for forcing modes with drm.edid_firmware.").arg(saved).arg(outDir));
        return;
    }

    if (kind == "bios") {
        // Honest handling: reading the SPI flash is a different class of operation
        const bool haveFlashrom = !QStandardPaths::findExecutable("flashrom").isEmpty();
        QString msg = tr(
            "Unlike a graphics card's vBIOS, the motherboard's UEFI firmware is NOT exposed by the "
            "kernel — it lives on an SPI flash chip and can only be read with flashrom.\n\n"
            "Two honest warnings:\n"
            "• On most modern AMD/Intel boards the chipset blocks reads (protected ranges), so the "
            "attempt usually returns an error or a garbage image.\n"
            "• flashrom writes are how boards get bricked. This app only ever offers the READ (-r) "
            "operation, never a write.\n\n"
            "For a usable BIOS image, download the update file for your exact board from the vendor.\n\n");
        msg += haveFlashrom ? tr("flashrom is installed. Attempt a read-only dump now?")
                            : tr("flashrom is not installed. Install it and attempt a read-only dump?");
        if (QMessageBox::question(this, tr("Motherboard BIOS"), msg) != QMessageBox::Yes) return;

        const QString outFile = QString("%1/motherboard-bios-%2.bin").arg(dir, stamp);
        runSudoCommandInTerminal(QString(
            "%1"
            "echo '== Attempting a READ-ONLY dump of the SPI flash...'; "
            "echo '== (no write operation is performed)'; echo; "
            "sudo flashrom -p internal -r '%2' ; "
            "if [ -s '%2' ]; then sudo chown \"$USER\" '%2'; echo; echo '== Saved: %2'; "
            "else echo; echo '== No image produced - the chipset most likely blocks reads. Use the vendor BIOS file instead.'; fi; "
            "read -p 'Press Enter to close...'")
            .arg(haveFlashrom ? "" : "sudo pacman -S --needed flashrom || exit 1; ", outFile));
        return;
    }
}
