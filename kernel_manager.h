
#include <QListWidget>
#include <QComboBox>
// Dashboard -> Kernels sub-tab: every kernel that can actually boot this
// machine (scanned from /boot, not a package whitelist — catches TKG builds
// and manually installed kernels too), install/remove, popular-kernel
// installer (repo / AUR / build-from-source), and the default GRUB entry.

void MainWindow::setupKernelManagerTab() {
    QVBoxLayout *layout = new QVBoxLayout(ui->kernelManagerSubTab);

    QHBoxLayout *bar = new QHBoxLayout();
    QLabel *hint = new QLabel(tr("Multiple kernels = safety. Keep an LTS fallback next to your daily driver."), ui->kernelManagerSubTab);
    hint->setStyleSheet("color:#888;");
    QPushButton *refreshBtn = new QPushButton(tr("🔄 Refresh"), ui->kernelManagerSubTab);
    QPushButton *popularBtn = new QPushButton(tr("🌟 Install Popular Kernel..."), ui->kernelManagerSubTab);
    QPushButton *removeBtn = new QPushButton(tr("🗑️ Remove Selected"), ui->kernelManagerSubTab);
    bar->addWidget(hint);
    bar->addStretch();
    bar->addWidget(refreshBtn);
    bar->addWidget(popularBtn);
    bar->addWidget(removeBtn);
    layout->addLayout(bar);

    kmTable = new QTableWidget(ui->kernelManagerSubTab);
    kmTable->setColumnCount(5);
    kmTable->setHorizontalHeaderLabels(QStringList() << tr("Boot Image") << tr("Package") << tr("Version") << tr("Running") << tr("Headers"));
    kmTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    kmTable->horizontalHeader()->setStretchLastSection(true);
    kmTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    kmTable->setSelectionMode(QAbstractItemView::SingleSelection);
    kmTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    kmTable->setAlternatingRowColors(true);
    kmTable->verticalHeader()->setVisible(false);
    layout->addWidget(kmTable);

    QLabel *grubHeader = new QLabel(tr("🥾 Boot entry (GRUB)"), ui->kernelManagerSubTab);
    QFont f = grubHeader->font(); f.setBold(true); grubHeader->setFont(f);
    layout->addWidget(grubHeader);

    kmGrubDefaultLabel = new QLabel(ui->kernelManagerSubTab);
    kmGrubDefaultLabel->setStyleSheet("color:#888;");
    layout->addWidget(kmGrubDefaultLabel);

    kmGrubList = new QListWidget(ui->kernelManagerSubTab);
    kmGrubList->setMaximumHeight(140);
    kmGrubList->setAlternatingRowColors(true);
    layout->addWidget(kmGrubList);

    QHBoxLayout *grubBar = new QHBoxLayout();
    QPushButton *setDefaultBtn = new QPushButton(tr("⭐ Set Selected as Default Boot Entry"), ui->kernelManagerSubTab);
    QPushButton *regenBtn = new QPushButton(tr("🔧 Regenerate GRUB Config"), ui->kernelManagerSubTab);
    grubBar->addStretch();
    grubBar->addWidget(setDefaultBtn);
    grubBar->addWidget(regenBtn);
    layout->addLayout(grubBar);

    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshKernelManager);

    connect(popularBtn, &QPushButton::clicked, this, [this]() {
        struct PopularKernel { QString pkg, source, desc, method; };
        const QList<PopularKernel> kernels = {
            {"linux",          tr("Official repo"),          tr("Vanilla Arch kernel"),                        "pacman"},
            {"linux-lts",      tr("Official repo"),          tr("Long-term support — the safe fallback"),      "pacman"},
            {"linux-zen",      tr("Official repo"),          tr("Desktop/gaming tuned (zen patches)"),         "pacman"},
            {"linux-hardened", tr("Official repo"),          tr("Security-hardened"),                          "pacman"},
            {"linux-rt",       tr("Official repo"),          tr("Realtime preemption (audio/robotics)"),       "pacman"},
            {"linux-cachyos",  tr("CachyOS repo / AUR"),     tr("BORE scheduler + CachyOS optimizations"),     "cachyos"},
            {"linux-xanmod",   tr("AUR"),                    tr("XanMod performance build"),                   "aur"},
            {"linux-mainline", tr("AUR"),                    tr("Latest mainline release candidate"),          "aur"},
            {"linux-tkg",      tr("GitHub — build from source"), tr("Frogging-Family TKG: interactive, fully customizable build"), "tkg"},
        };

        QDialog dlg(this);
        dlg.setWindowTitle(tr("Install Popular Kernel"));
        dlg.resize(620, 420);
        QVBoxLayout *dl = new QVBoxLayout(&dlg);
        dl->addWidget(new QLabel(tr("Pick a kernel — repo installs use pacman, AUR uses yay/paru, and TKG clones the\nGitHub repo and runs its interactive makepkg build (you choose the options in the terminal)."), &dlg));
        QTreeWidget *list = new QTreeWidget(&dlg);
        list->setColumnCount(3);
        list->setHeaderLabels(QStringList() << tr("Kernel") << tr("Source") << tr("Description"));
        list->setRootIsDecorated(false);
        list->setAlternatingRowColors(true);
        for (const PopularKernel &k : kernels) {
            QTreeWidgetItem *item = new QTreeWidgetItem(list);
            item->setText(0, k.pkg);
            item->setText(1, k.source);
            item->setText(2, k.desc);
            item->setData(0, Qt::UserRole, k.method);
        }
        for (int c = 0; c < 3; ++c) list->resizeColumnToContents(c);
        dl->addWidget(list);
        QHBoxLayout *btns = new QHBoxLayout();
        QPushButton *installBtn = new QPushButton(tr("⬇️ Install"), &dlg);
        QPushButton *cancelBtn = new QPushButton(tr("Cancel"), &dlg);
        btns->addStretch(); btns->addWidget(installBtn); btns->addWidget(cancelBtn);
        dl->addLayout(btns);
        connect(installBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
        connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
        if (dlg.exec() != QDialog::Accepted || !list->currentItem()) return;

        const QString pkg = list->currentItem()->text(0);
        const QString method = list->currentItem()->data(0, Qt::UserRole).toString();
        QString cmd;
        if (method == "pacman") {
            cmd = QString("sudo pacman -S %1 %1-headers").arg(pkg);
        } else if (method == "cachyos") {
            cmd = QString("if pacman -Si %1 >/dev/null 2>&1; then sudo pacman -S %1 %1-headers; "
                          "elif command -v yay >/dev/null; then yay -S %1 %1-headers; "
                          "elif command -v paru >/dev/null; then paru -S %1 %1-headers; "
                          "else echo 'Add the CachyOS repo or install yay/paru first (PKG Install tab).'; fi").arg(pkg);
        } else if (method == "aur") {
            cmd = QString("if command -v yay >/dev/null; then yay -S %1 %1-headers; "
                          "elif command -v paru >/dev/null; then paru -S %1 %1-headers; "
                          "else echo 'Install yay or paru first (PKG Install tab).'; fi").arg(pkg);
        } else { // tkg: clone + interactive source build, the officially supported way
            cmd = "D=\"$HOME/.cache/cachyostools/linux-tkg\"; "
                  "if [ -d \"$D/.git\" ]; then git -C \"$D\" pull; else git clone https://github.com/Frogging-Family/linux-tkg.git \"$D\"; fi && "
                  "cd \"$D\" && echo && echo '=== TKG interactive build — answer the prompts to configure your kernel ===' && makepkg -si";
        }
        runSudoCommandInTerminal(cmd + "; echo; echo 'Done. New kernels appear in GRUB after Regenerate GRUB Config.'; read -p 'Press Enter...'");
    });

    connect(removeBtn, &QPushButton::clicked, this, [this]() {
        int row = kmTable->currentRow();
        if (row < 0 || !kmTable->item(row, 0)) {
            QMessageBox::information(this, tr("No Selection"), tr("Select an installed kernel first."));
            return;
        }
        QString image = kmTable->item(row, 0)->text();
        QString pkg = kmTable->item(row, 1)->text();
        if (kmTable->item(row, 3)->text().contains("✅")) {
            QMessageBox::warning(this, tr("Refused"), tr("'%1' is the kernel you are running right now. Boot another kernel first.").arg(image));
            return;
        }
        if (kmTable->rowCount() <= 1) {
            QMessageBox::warning(this, tr("Refused"), tr("That is your only installed kernel."));
            return;
        }
        if (pkg == "-") {
            QMessageBox::information(this, tr("Not Package-Managed"),
                tr("'%1' was not installed by pacman, so it must be removed by hand:\n\n"
                   "  sudo rm /boot/vmlinuz-%1\n"
                   "  sudo rm /boot/initramfs-%1*.img\n"
                   "  sudo rm -r /usr/lib/modules/<its version>\n\n"
                   "Then Regenerate GRUB Config. Double-check each path before deleting!").arg(image));
            return;
        }
        if (QMessageBox::warning(this, tr("Remove Kernel"),
                tr("Remove package '%1' (boot image '%2') and its headers?\n\n"
                   "pacman + the mkinitcpio hook clean up /boot and the modules; GRUB is regenerated automatically afterwards.").arg(pkg, image),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
        runSudoCommandInTerminal(QString(
            "(sudo pacman -R %1 %1-headers 2>/dev/null || sudo pacman -R %1) && "
            "sudo grub-mkconfig -o /boot/grub/grub.cfg && "
            "echo && echo 'Kernel removed and GRUB menu updated.'; read -p 'Press Enter...'").arg(pkg));
    });

    connect(setDefaultBtn, &QPushButton::clicked, this, [this]() {
        QListWidgetItem *item = kmGrubList->currentItem();
        if (!item) {
            QMessageBox::information(this, tr("No Selection"), tr("Select a boot entry from the list first."));
            return;
        }
        QString entry = item->data(Qt::UserRole).toString();   // full "submenu>entry" path
        entry.replace('\'', ' ');
        if (QMessageBox::question(this, tr("Set Default Boot Entry"),
                tr("Boot this entry by default from now on?\n\n%1").arg(item->text())) != QMessageBox::Yes) return;
        runSudoCommandInTerminal(QString(
            "sudo sed -i 's/^GRUB_DEFAULT=.*/GRUB_DEFAULT=saved/' /etc/default/grub && "
            "sudo sed -i 's/^#*GRUB_SAVEDEFAULT=.*/GRUB_SAVEDEFAULT=false/' /etc/default/grub && "
            "sudo grub-mkconfig -o /boot/grub/grub.cfg && "
            "sudo grub-set-default '%1' && "
            "echo && echo 'Default boot entry saved.'; read -p 'Press Enter...'").arg(entry));
    });

    connect(regenBtn, &QPushButton::clicked, this, [this]() {
        runSudoCommandInTerminal("sudo grub-mkconfig -o /boot/grub/grub.cfg && echo 'GRUB config regenerated.'; read -p 'Press Enter...'");
    });
}

void MainWindow::refreshKernelManager() {
    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, proc](int, QProcess::ExitStatus) {
                const QStringList sections = QString::fromUtf8(proc->readAllStandardOutput()).split("===GRUB===");

                // --- kernels found in /boot: "K|image|ownerPkg|pkgVer|kernelVer|headers"
                kmTable->setRowCount(0);
                QString running;
                for (const QString &line : sections.value(0).split('\n', Qt::SkipEmptyParts)) {
                    if (line.startsWith("RUNNING:")) { running = line.section(':', 1).trimmed(); continue; }
                    if (!line.startsWith("K|")) continue;
                    const QStringList f = line.split('|');
                    if (f.size() < 6) continue;
                    int row = kmTable->rowCount();
                    kmTable->insertRow(row);
                    kmTable->setItem(row, 0, new QTableWidgetItem(f[1]));
                    kmTable->setItem(row, 1, new QTableWidgetItem(f[2]));
                    kmTable->setItem(row, 2, new QTableWidgetItem(f[3] != "-" ? f[3] : f[4]));
                    kmTable->setItem(row, 3, new QTableWidgetItem(""));
                    kmTable->setItem(row, 4, new QTableWidgetItem(
                        f[2] == "-" ? tr("—") : (f[5] == "yes" ? tr("✅ installed") : tr("—"))));
                    if (f[2] == "-") kmTable->item(row, 1)->setForeground(QColor("#e67e22"));
                }
                // running match: the boot image whose embedded version equals uname -r
                for (const QString &line : sections.value(0).split('\n', Qt::SkipEmptyParts)) {
                    if (!line.startsWith("K|")) continue;
                    const QStringList f = line.split('|');
                    if (f.size() < 6) continue;
                    if (!running.isEmpty() && f[4] == running) {
                        for (int row = 0; row < kmTable->rowCount(); ++row) {
                            if (kmTable->item(row, 0)->text() == f[1]) {
                                kmTable->item(row, 3)->setText(tr("✅ current (%1)").arg(running));
                                kmTable->item(row, 0)->setForeground(QColor("#27ae60"));
                            }
                        }
                    }
                }
                kmTable->resizeColumnsToContents();

                // --- GRUB: default setting + menu entries (top level and submenu)
                kmGrubList->clear();
                QString grubDefault = tr("(unknown)");
                QString submenu;
                for (const QString &line : sections.value(1).split('\n')) {
                    if (line.startsWith("DEFAULT:")) { grubDefault = line.section(':', 1).trimmed(); continue; }
                    QRegularExpressionMatch sm = QRegularExpression("^submenu '([^']+)'").match(line.trimmed());
                    if (sm.hasMatch() && !line.startsWith('\t') && !line.startsWith(' ')) { submenu = sm.captured(1); continue; }
                    QRegularExpressionMatch me = QRegularExpression("^\\s*menuentry '([^']+)'").match(line);
                    if (me.hasMatch()) {
                        bool nested = line.startsWith('\t') || line.startsWith("    ");
                        QString title = me.captured(1);
                        QString path = (nested && !submenu.isEmpty()) ? submenu + ">" + title : title;
                        QListWidgetItem *item = new QListWidgetItem((nested ? "   ↳ " : "") + title, kmGrubList);
                        item->setData(Qt::UserRole, path);
                    }
                }
                kmGrubDefaultLabel->setText(tr("GRUB_DEFAULT is currently: %1 — select an entry below and set it as default.").arg(grubDefault));
                proc->deleteLater();
            });
    proc->start("bash", QStringList() << "-c"
        << "for f in /boot/vmlinuz-*; do "
           "  [ -e \"$f\" ] || continue; "
           "  n=\"${f#/boot/vmlinuz-}\"; "
           "  kver=$(file -b \"$f\" 2>/dev/null | grep -o 'version [^ ]*' | cut -d' ' -f2); "
           // /boot images are copied by install hooks, not owned by the package —
           // the modules dir's pkgbase file is the reliable image->package mapping
           "  owner=$(cat \"/usr/lib/modules/$kver/pkgbase\" 2>/dev/null || pacman -Qoq \"$f\" 2>/dev/null || echo -); "
           "  if [ \"$owner\" != - ] && pacman -Q \"$owner\" >/dev/null 2>&1; then ver=$(pacman -Q \"$owner\" | awk '{print $2}'); else owner=-; ver=-; fi; "
           "  hdr=no; [ \"$owner\" != - ] && pacman -Q \"${owner}-headers\" >/dev/null 2>&1 && hdr=yes; "
           "  echo \"K|$n|$owner|$ver|$kver|$hdr\"; "
           "done; "
           "echo \"RUNNING:$(uname -r)\"; "
           "echo ===GRUB===; "
           "grep '^GRUB_DEFAULT=' /etc/default/grub 2>/dev/null | sed 's/^GRUB_DEFAULT=/DEFAULT:/'; "
           "cat /boot/grub/grub.cfg 2>/dev/null | grep -E \"^(submenu|[[:space:]]*menuentry)\" || sudo -n cat /boot/grub/grub.cfg 2>/dev/null | grep -E \"^(submenu|[[:space:]]*menuentry)\"");
}
