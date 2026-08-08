
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
    bar->addWidget(devMgrFilter);
    bar->addWidget(devMgrCountLabel);
    bar->addStretch();
    bar->addWidget(propsBtn);
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
                const QStringList blocks = QString::fromUtf8(proc->readAllStandardOutput()).split("@DEV\n", Qt::SkipEmptyParts);
                for (const QString &block : blocks) {
                    QVariantMap p;
                    QString resources;
                    for (const QString &line : block.split('\n')) {
                        const int eq = line.indexOf('=');
                        if (eq <= 0) continue;
                        const QString key = line.left(eq), val = line.mid(eq + 1);
                        if (key == "res") resources += (resources.isEmpty() ? "" : "\n") + val;
                        else p.insert(key, val);
                    }
                    if (p.value("device").toString().isEmpty()) continue;
                    p.insert("resources", resources);

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
