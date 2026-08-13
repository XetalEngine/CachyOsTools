
// Network -> 📁 Shares sub-tab: the "Network Locations" applet Arch never had.
//
//   top    — what is mounted right now, and whether it survives a reboot
//   middle — what is out there (mDNS + NetBIOS discovery, or type a host)
//   bottom — what *you* are sharing, via Samba
//
// Passwords are never written into a script or a log: mount.cifs asks for them
// in the terminal, and the persistent path reads them with `read -rs` straight
// into a 0600 credentials file.

#include <QGroupBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QSpinBox>
#include "script_helpers.h"

// Mount points we refuse to bury a network share under.
static bool isProtectedMountPoint(const QString &path) {
    const QString p = QDir::cleanPath(path);
    static const QStringList forbidden = {
        "/", "/home", "/etc", "/boot", "/boot/efi", "/usr", "/var", "/root",
        "/bin", "/sbin", "/lib", "/lib64", "/opt", "/srv", "/proc", "/sys", "/dev", "/run"
    };
    return forbidden.contains(p) || p == QDir::homePath();
}

// Folders that must never be exported to the network.
static bool isProtectedSharePath(const QString &path) {
    const QString p = QDir::cleanPath(path);
    const QString home = QDir::homePath();
    static const QStringList forbidden = {
        "/", "/etc", "/boot", "/boot/efi", "/usr", "/var", "/root", "/home",
        "/bin", "/sbin", "/lib", "/lib64", "/proc", "/sys", "/dev", "/run"
    };
    if (forbidden.contains(p)) return true;
    if (p == home) return true;
    for (const QString &sub : {"/.ssh", "/.gnupg", "/.config", "/.local/share/keyrings"})
        if (p == home + sub) return true;
    return false;
}

bool MainWindow::networkSharesVisible() const {
    return sharesSubTab && isVisible() && !isMinimized()
        && ui->tabWidget->currentWidget() == ui->networkTab
        && ui->networkTabWidget->currentWidget() == sharesSubTab;
}

// ============================================================
// Tab construction
// ============================================================
void MainWindow::setupNetworkSharesTab() {
    sharesSubTab = new QWidget();
    sharesSubTab->setObjectName("networkSharesSubTab");
    QVBoxLayout *root = new QVBoxLayout(sharesSubTab);

    // ---------- mounted ----------
    QGroupBox *mountedBox = new QGroupBox(tr("Mounted network shares"), sharesSubTab);
    QVBoxLayout *mountedLay = new QVBoxLayout(mountedBox);

    shrMountedTable = new QTableWidget(mountedBox);
    shrMountedTable->setColumnCount(5);
    shrMountedTable->setHorizontalHeaderLabels(QStringList()
        << tr("Remote") << tr("Mounted at") << tr("Type") << tr("Options") << tr("At boot"));
    shrMountedTable->horizontalHeader()->setStretchLastSection(false);
    shrMountedTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    shrMountedTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    shrMountedTable->setSelectionMode(QAbstractItemView::SingleSelection);
    shrMountedTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    shrMountedTable->setAlternatingRowColors(true);
    shrMountedTable->verticalHeader()->setVisible(false);
    shrMountedTable->setMaximumHeight(170);
    mountedLay->addWidget(shrMountedTable);

    QHBoxLayout *mountedBar = new QHBoxLayout();
    shrStatusLabel = new QLabel(tr("Nothing mounted."), mountedBox);
    shrStatusLabel->setStyleSheet("color:#888;");
    QPushButton *mRefreshBtn = new QPushButton(tr("🔄 Refresh"), mountedBox);
    QPushButton *mMountBtn = new QPushButton(tr("➕ Mount Share…"), mountedBox);
    QPushButton *mUnmountBtn = new QPushButton(tr("⏏️ Unmount"), mountedBox);
    QPushButton *mForgetBtn = new QPushButton(tr("🗑️ Remove from fstab"), mountedBox);
    mForgetBtn->setToolTip(tr("Stop mounting this share at boot. Does not unmount it now."));
    mountedBar->addWidget(shrStatusLabel, 1);
    for (QPushButton *b : {mRefreshBtn, mMountBtn, mUnmountBtn, mForgetBtn}) mountedBar->addWidget(b);
    mountedLay->addLayout(mountedBar);
    root->addWidget(mountedBox);

    // ---------- discovery ----------
    QGroupBox *findBox = new QGroupBox(tr("Find shares on the network"), sharesSubTab);
    QVBoxLayout *findLay = new QVBoxLayout(findBox);

    QHBoxLayout *findBar = new QHBoxLayout();
    QPushButton *scanBtn = new QPushButton(tr("🔍 Scan Network"), findBox);
    scanBtn->setToolTip(tr("Asks the local network over mDNS and NetBIOS. Finds most NAS boxes and Windows PCs."));
    shrHostEdit = new QLineEdit(findBox);
    shrHostEdit->setPlaceholderText(tr("…or type a hostname / IP address"));
    QPushButton *listBtn = new QPushButton(tr("📃 List Shares"), findBox);
    findBar->addWidget(scanBtn);
    findBar->addWidget(shrHostEdit, 1);
    findBar->addWidget(listBtn);
    findLay->addLayout(findBar);

    shrDiscoverTree = new QTreeWidget(findBox);
    shrDiscoverTree->setColumnCount(3);
    shrDiscoverTree->setHeaderLabels(QStringList() << tr("Host / Share") << tr("Type") << tr("Comment"));
    shrDiscoverTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    shrDiscoverTree->setAlternatingRowColors(true);
    shrDiscoverTree->setMaximumHeight(180);
    shrDiscoverTree->setToolTip(tr("Double-click a share to mount it."));
    findLay->addWidget(shrDiscoverTree);
    root->addWidget(findBox);

    // ---------- samba server ----------
    QGroupBox *sambaBox = new QGroupBox(tr("Folders you are sharing (Samba)"), sharesSubTab);
    QVBoxLayout *sambaLay = new QVBoxLayout(sambaBox);

    shrSambaStatusLabel = new QLabel(sambaBox);
    shrSambaStatusLabel->setTextFormat(Qt::RichText);
    shrSambaStatusLabel->setWordWrap(true);
    sambaLay->addWidget(shrSambaStatusLabel);

    shrSambaTable = new QTableWidget(sambaBox);
    shrSambaTable->setColumnCount(5);
    shrSambaTable->setHorizontalHeaderLabels(QStringList()
        << tr("Share") << tr("Folder") << tr("Access") << tr("Guests") << tr("Users"));
    shrSambaTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    shrSambaTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    shrSambaTable->setSelectionMode(QAbstractItemView::SingleSelection);
    shrSambaTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    shrSambaTable->setAlternatingRowColors(true);
    shrSambaTable->verticalHeader()->setVisible(false);
    shrSambaTable->setMaximumHeight(160);
    sambaLay->addWidget(shrSambaTable);

    QHBoxLayout *sambaBar = new QHBoxLayout();
    QPushButton *sAddBtn = new QPushButton(tr("➕ Share a Folder…"), sambaBox);
    QPushButton *sStopBtn = new QPushButton(tr("🚫 Stop Sharing"), sambaBox);
    QPushButton *sPassBtn = new QPushButton(tr("👤 Set Samba Password…"), sambaBox);
    sPassBtn->setToolTip(tr("Samba keeps its own password database — your login password does not apply."));
    QPushButton *sServerBtn = new QPushButton(tr("▶️ Enable Server"), sambaBox);
    QPushButton *sFwBtn = new QPushButton(tr("🛡️ Open Firewall"), sambaBox);
    QPushButton *sConfBtn = new QPushButton(tr("✏️ smb.conf"), sambaBox);
    sambaBar->addStretch();
    for (QPushButton *b : {sAddBtn, sStopBtn, sPassBtn, sServerBtn, sFwBtn, sConfBtn}) sambaBar->addWidget(b);
    sambaLay->addLayout(sambaBar);
    root->addWidget(sambaBox);

    // ---------- wiring ----------
    connect(mRefreshBtn, &QPushButton::clicked, this, &MainWindow::refreshNetworkShares);
    connect(mMountBtn, &QPushButton::clicked, this, [this]() { showMountShareDialog("cifs", QString(), QString()); });
    connect(scanBtn, &QPushButton::clicked, this, &MainWindow::discoverNetworkShares);
    connect(listBtn, &QPushButton::clicked, this, [this]() {
        const QString host = shrHostEdit->text().trimmed();
        if (host.isEmpty()) {
            QMessageBox::information(this, tr("No Host"), tr("Type a hostname or IP address first."));
            return;
        }
        listHostShares(host);
    });
    connect(shrHostEdit, &QLineEdit::returnPressed, listBtn, &QPushButton::click);

    connect(shrDiscoverTree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int) {
        const QString kind = item->data(0, Qt::UserRole).toString();
        if (kind == "host") { listHostShares(item->text(0)); return; }
        if (kind != "share") return;
        showMountShareDialog(item->data(1, Qt::UserRole).toString(),   // cifs | nfs
                             item->data(2, Qt::UserRole).toString(),   // server
                             item->text(0));                           // share / export
    });

    connect(mUnmountBtn, &QPushButton::clicked, this, [this]() {
        const int row = shrMountedTable->currentRow();
        if (row < 0) {
            QMessageBox::information(this, tr("No Selection"), tr("Select a mounted share first."));
            return;
        }
        const QString mp = shrMountedTable->item(row, 1)->text();
        runScriptInTerminal(repairScript(QString(
            "umount %1 || { echo 'Busy — retrying lazily (the mount detaches once nothing is using it).';"
            " umount -l %1; }\n"
            "echo 'Unmounted.'\n").arg(shQuote(mp))), "unmount_share");
        scheduleStagedRecheck([this]() { if (networkSharesVisible()) refreshMountedShares(); });
    });

    connect(mForgetBtn, &QPushButton::clicked, this, [this]() {
        const int row = shrMountedTable->currentRow();
        if (row < 0) {
            QMessageBox::information(this, tr("No Selection"), tr("Select a share first."));
            return;
        }
        const QString mp = shrMountedTable->item(row, 1)->text();
        if (shrMountedTable->item(row, 4)->text().contains(tr("no"), Qt::CaseInsensitive)) {
            QMessageBox::information(this, tr("Not in fstab"),
                tr("%1 is not listed in /etc/fstab, so it already does not mount at boot.").arg(mp));
            return;
        }
        if (QMessageBox::question(this, tr("Remove from fstab"),
                tr("Stop mounting %1 at boot?\n\n/etc/fstab is backed up first. "
                   "Any credentials file stays where it is.").arg(mp),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
        runScriptInTerminal(repairScript(QString(
            "cp -v /etc/fstab /etc/fstab.bak.$(date +%F_%H-%M-%S)\n"
            "awk -v mp=%1 '$2 != mp' /etc/fstab > /tmp/fstab.new\n"
            "cat /tmp/fstab.new > /etc/fstab; rm -f /tmp/fstab.new\n"
            "systemctl daemon-reload\n"
            "echo 'Removed. Remaining network entries:'\n"
            "grep -E 'cifs|nfs' /etc/fstab || echo '  (none)'\n").arg(shQuote(mp))), "fstab_forget");
        scheduleStagedRecheck([this]() { if (networkSharesVisible()) refreshMountedShares(); });
    });

    connect(sAddBtn, &QPushButton::clicked, this, &MainWindow::showAddSambaShareDialog);

    connect(sStopBtn, &QPushButton::clicked, this, [this]() {
        const int row = shrSambaTable->currentRow();
        if (row < 0) {
            QMessageBox::information(this, tr("No Selection"), tr("Select one of your shares first."));
            return;
        }
        const QString name = shrSambaTable->item(row, 0)->text();
        if (QMessageBox::question(this, tr("Stop Sharing"),
                tr("Remove the [%1] share from /etc/samba/smb.conf?\n\n"
                   "The folder and its contents are untouched — only the share definition goes away.").arg(name),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
        runScriptInTerminal(repairScript(QString(
            "CONF=/etc/samba/smb.conf\n"
            "cp -v \"$CONF\" \"$CONF.bak.$(date +%F_%H-%M-%S)\"\n"
            "awk -v s=%1 'BEGIN{skip=0}\n"
            "  /^[[:space:]]*\\[/ { sec=$0; gsub(/^[[:space:]]*\\[|\\][[:space:]]*$/,\"\",sec); skip=(sec==s) }\n"
            "  !skip' \"$CONF\" > /tmp/smb.conf.new\n"
            "cat /tmp/smb.conf.new > \"$CONF\"; rm -f /tmp/smb.conf.new\n"
            "testparm -s >/dev/null\n"
            "systemctl reload smb 2>/dev/null || systemctl restart smb 2>/dev/null || true\n"
            "echo 'Share removed.'\n").arg(shQuote(name))), "samba_unshare");
        scheduleStagedRecheck([this]() { if (networkSharesVisible()) refreshSambaShares(); });
    });

    connect(sPassBtn, &QPushButton::clicked, this, [this]() {
        bool ok;
        const QString user = QInputDialog::getText(this, tr("Samba Password"),
            tr("Samba keeps a separate password database from your Linux login.\n"
               "Which existing system user should get a Samba password?"),
            QLineEdit::Normal, QProcessEnvironment::systemEnvironment().value("USER"), &ok).trimmed();
        if (!ok || user.isEmpty()) return;
        if (!QRegularExpression("^[a-z_][a-z0-9_-]*$").match(user).hasMatch()) {
            QMessageBox::warning(this, tr("Invalid User"), tr("That is not a valid Linux user name."));
            return;
        }
        runScriptInTerminal(repairScript(QString(
            "if ! id %1 >/dev/null 2>&1; then\n"
            "  echo 'No such system user — create the account first (Dashboard → Users & Groups).'\n"
            "else\n"
            "  smbpasswd -a %1\n"
            "  smbpasswd -e %1\n"
            "  echo 'Samba password set and account enabled.'\n"
            "fi\n").arg(shQuote(user))), "samba_passwd");
    });

    connect(sServerBtn, &QPushButton::clicked, this, [this]() {
        runScriptInTerminal(repairScript(
            "if ! command -v smbd >/dev/null 2>&1; then\n"
            "  echo 'Installing samba...'\n"
            "  pacman -S --needed samba\n"
            "fi\n"
            "if [ ! -f /etc/samba/smb.conf ]; then\n"
            "  install -d /etc/samba\n"
            "  printf '[global]\\n   workgroup = WORKGROUP\\n   server string = %s\\n"
            "   security = user\\n   map to guest = Bad User\\n' \"$(hostname)\" > /etc/samba/smb.conf\n"
            "  echo 'Created a minimal /etc/samba/smb.conf.'\n"
            "fi\n"
            "systemctl enable --now smb nmb\n"
            "systemctl --no-pager status smb | head -5\n"), "samba_enable");
        scheduleStagedRecheck([this]() { if (networkSharesVisible()) refreshSambaShares(); });
    });

    connect(sFwBtn, &QPushButton::clicked, this, [this]() {
        QProcess which;
        which.start("bash", QStringList() << "-c"
                    << "command -v ufw >/dev/null && echo ufw || (command -v firewall-cmd >/dev/null && echo firewalld || echo none)");
        which.waitForFinished(3000);
        const QString backend = QString::fromUtf8(which.readAllStandardOutput()).trimmed();
        if (backend == "none") {
            QMessageBox::information(this, tr("No Firewall"),
                tr("Neither ufw nor firewalld is installed, so nothing is blocking Samba."));
            return;
        }
        if (QMessageBox::question(this, tr("Open Firewall for Samba"),
                tr("Allow Samba traffic through %1?\n\n"
                   "This opens ports 137, 138, 139 and 445 to your local network. "
                   "Only do this on a network you trust.").arg(backend),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
        runScriptInTerminal(repairScript(backend == "ufw"
            ? "ufw allow samba\nufw status numbered | head -20\n"
            : "firewall-cmd --permanent --add-service=samba\nfirewall-cmd --reload\nfirewall-cmd --list-services\n"),
            "samba_firewall");
    });

    connect(sConfBtn, &QPushButton::clicked, this, [this]() {
        if (!QFile::exists("/etc/samba/smb.conf")) {
            QMessageBox::information(this, tr("No smb.conf"),
                tr("/etc/samba/smb.conf does not exist yet. Use Enable Server to create a minimal one."));
            return;
        }
        backupConfigFile("/etc/samba/smb.conf", "Samba configuration");
        openBuiltinEditor("/etc/samba/smb.conf");
    });

    ui->networkTabWidget->addTab(sharesSubTab, tr("📁 Shares"));

    // Mounting a sleeping NAS or installing samba takes longer than any fixed
    // delay we could guess at, so watch the files instead: fstab and smb.conf
    // change the moment the work is really done.
    shrDebounce = new QTimer(this);
    shrDebounce->setSingleShot(true);
    shrDebounce->setInterval(1500);
    connect(shrDebounce, &QTimer::timeout, this, [this]() {
        if (networkSharesVisible()) refreshNetworkShares();
    });

    shrWatcher = new QFileSystemWatcher(this);
    auto rearmShareWatcher = [this]() {
        const QStringList watched = shrWatcher->files() + shrWatcher->directories();
        for (const QString &p : {QStringLiteral("/etc/fstab"),
                                 QStringLiteral("/etc/samba/smb.conf"),
                                 QStringLiteral("/etc/samba")})
            if (QFile::exists(p) && !watched.contains(p)) shrWatcher->addPath(p);
    };
    rearmShareWatcher();
    auto onShareChanged = [this, rearmShareWatcher](const QString &) {
        rearmShareWatcher();
        shrDebounce->start();
    };
    connect(shrWatcher, &QFileSystemWatcher::fileChanged, this, onShareChanged);
    connect(shrWatcher, &QFileSystemWatcher::directoryChanged, this, onShareChanged);
}

void MainWindow::refreshNetworkShares() {
    refreshMountedShares();
    refreshSambaShares();
}

// ============================================================
// What is mounted
// ============================================================
void MainWindow::refreshMountedShares() {
    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, proc](int, QProcess::ExitStatus) {
        const QString out = QString::fromUtf8(proc->readAllStandardOutput());
        proc->deleteLater();

        // Everything listed in fstab, so we can say which mounts are permanent
        QSet<QString> inFstab;
        QFile fstab("/etc/fstab");
        if (fstab.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream ts(&fstab);
            while (!ts.atEnd()) {
                const QString line = ts.readLine().trimmed();
                if (line.isEmpty() || line.startsWith('#')) continue;
                const QStringList f = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                if (f.size() >= 3 && (f[2] == "cifs" || f[2].startsWith("nfs") || f[2] == "smb3"))
                    inFstab.insert(f[1]);
            }
            fstab.close();
        }

        shrMountedTable->setRowCount(0);
        for (const QString &line : out.split('\n', Qt::SkipEmptyParts)) {
            const QStringList f = line.split('\t');
            if (f.size() < 4) continue;
            const int row = shrMountedTable->rowCount();
            shrMountedTable->insertRow(row);
            shrMountedTable->setItem(row, 0, new QTableWidgetItem(f[0]));
            shrMountedTable->setItem(row, 1, new QTableWidgetItem(f[1]));
            shrMountedTable->setItem(row, 2, new QTableWidgetItem(f[2]));
            // The options string is long and mostly kernel defaults; show the interesting half
            QStringList opts;
            for (const QString &o : f[3].split(','))
                if (o.startsWith("vers") || o.startsWith("username") || o == "ro" || o == "rw" ||
                    o.startsWith("uid=") || o.startsWith("addr=") || o.startsWith("credentials"))
                    opts << o;
            shrMountedTable->setItem(row, 3, new QTableWidgetItem(opts.join(',')));
            const bool persistent = inFstab.contains(f[1]);
            QTableWidgetItem *bootItem = new QTableWidgetItem(persistent ? tr("✅ yes") : tr("— no"));
            bootItem->setForeground(persistent ? QColor("#27ae60") : QColor("#888888"));
            shrMountedTable->setItem(row, 4, bootItem);
        }
        shrMountedTable->resizeColumnToContents(0);
        shrMountedTable->resizeColumnToContents(1);
        shrMountedTable->resizeColumnToContents(2);

        const int n = shrMountedTable->rowCount();
        if (n == 0) {
            shrStatusLabel->setText(tr("No network shares mounted."));
            shrStatusLabel->setStyleSheet("color:#888;");
        } else {
            shrStatusLabel->setText(tr("%n share(s) mounted", "", n));
            shrStatusLabel->setStyleSheet("color:#27ae60; font-weight:bold;");
        }
    });
    proc->start("bash", QStringList() << "-c"
                << "findmnt -rno SOURCE,TARGET,FSTYPE,OPTIONS -t cifs,smb3,nfs,nfs4 2>/dev/null | tr ' ' '\\t'");
}

// ============================================================
// Discovery
// ============================================================
void MainWindow::discoverNetworkShares() {
    shrDiscoverTree->clear();
    QTreeWidgetItem *busy = new QTreeWidgetItem(shrDiscoverTree);
    busy->setText(0, tr("Scanning…"));

    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, proc](int, QProcess::ExitStatus) {
        const QString out = QString::fromUtf8(proc->readAllStandardOutput());
        proc->deleteLater();
        shrDiscoverTree->clear();

        QMap<QString, QString> hosts;   // name -> address
        for (const QString &line : out.split('\n', Qt::SkipEmptyParts)) {
            if (!line.startsWith("H|")) continue;
            const QStringList f = line.mid(2).split('|');
            if (f.size() < 2 || f[0].isEmpty()) continue;
            if (!hosts.contains(f[0]) || hosts.value(f[0]).isEmpty()) hosts.insert(f[0], f[1]);
        }

        if (hosts.isEmpty()) {
            QTreeWidgetItem *none = new QTreeWidgetItem(shrDiscoverTree);
            none->setText(0, tr("Nothing found"));
            none->setText(2, tr("Discovery needs avahi (mDNS) or smbclient (NetBIOS). "
                                "You can always type a host above and press List Shares."));
            none->setForeground(0, QColor("#e67e22"));
            return;
        }

        for (auto it = hosts.begin(); it != hosts.end(); ++it) {
            QTreeWidgetItem *hostItem = new QTreeWidgetItem(shrDiscoverTree);
            hostItem->setText(0, it.key());
            hostItem->setText(1, tr("host"));
            hostItem->setText(2, it.value().isEmpty() ? tr("double-click to list shares")
                                                      : it.value() + tr("  ·  double-click to list shares"));
            hostItem->setData(0, Qt::UserRole, "host");
            QFont f = hostItem->font(0);
            f.setBold(true);
            hostItem->setFont(0, f);
        }
    });

    proc->start("bash", QStringList() << "-c" << R"BASH(
if command -v avahi-browse >/dev/null 2>&1; then
  timeout 6 avahi-browse -artp 2>/dev/null |
    awk -F';' '/^=/ && ($5=="_smb._tcp" || $5=="_nfs._tcp") { print "H|" $7 "|" $8 }'
fi
if command -v nmblookup >/dev/null 2>&1; then
  timeout 6 nmblookup -S '*' 2>/dev/null | awk '
    /^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+ / { ip=$1; next }
    /<00> - / && !/<GROUP>/ { print "H|" $1 "|" ip }'
fi
)BASH");
}

void MainWindow::listHostShares(const QString &host) {
    if (!QRegularExpression("^[A-Za-z0-9._:-]+$").match(host).hasMatch()) {
        QMessageBox::warning(this, tr("Invalid Host"), tr("Use a hostname or IP address."));
        return;
    }

    // Reuse the host's node if the scan already created one
    QTreeWidgetItem *hostItem = nullptr;
    for (int i = 0; i < shrDiscoverTree->topLevelItemCount(); ++i) {
        if (shrDiscoverTree->topLevelItem(i)->text(0).compare(host, Qt::CaseInsensitive) == 0) {
            hostItem = shrDiscoverTree->topLevelItem(i);
            break;
        }
    }
    if (!hostItem) {
        hostItem = new QTreeWidgetItem(shrDiscoverTree);
        hostItem->setText(0, host);
        hostItem->setText(1, tr("host"));
        hostItem->setData(0, Qt::UserRole, "host");
        QFont f = hostItem->font(0);
        f.setBold(true);
        hostItem->setFont(0, f);
    }
    qDeleteAll(hostItem->takeChildren());
    hostItem->setText(2, tr("querying…"));

    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, proc, hostItem, host](int, QProcess::ExitStatus) {
        const QString out = QString::fromUtf8(proc->readAllStandardOutput());
        proc->deleteLater();

        int found = 0;
        for (const QString &line : out.split('\n', Qt::SkipEmptyParts)) {
            if (!line.startsWith("S|")) continue;
            const QStringList f = line.mid(2).split('|');
            if (f.size() < 2) continue;
            QTreeWidgetItem *item = new QTreeWidgetItem(hostItem);
            item->setText(0, f[1]);
            item->setText(1, f[0] == "cifs" ? "SMB" : "NFS");
            item->setText(2, f.size() > 2 ? f[2] : QString());
            item->setData(0, Qt::UserRole, "share");
            item->setData(1, Qt::UserRole, f[0]);
            item->setData(2, Qt::UserRole, host);
            found++;
        }
        hostItem->setExpanded(true);
        if (found == 0) {
            hostItem->setText(2, tr("no shares listed — the host may require a login, or "
                                    "smbclient / showmount is not installed"));
            hostItem->setForeground(2, QColor("#e67e22"));
        } else {
            hostItem->setText(2, tr("%n share(s) — double-click one to mount", "", found));
            hostItem->setForeground(2, QColor("#27ae60"));
        }
    });

    // -N = try anonymously; a host that refuses simply returns nothing.
    proc->start("bash", QStringList() << "-c" << QString(R"BASH(
H=%1
if command -v smbclient >/dev/null 2>&1; then
  timeout 10 smbclient -L "$H" -N -g 2>/dev/null |
    awk -F'|' '$1=="Disk" && $2 !~ /\$$/ { print "S|cifs|" $2 "|" $3 }'
fi
if command -v showmount >/dev/null 2>&1; then
  timeout 10 showmount -e "$H" 2>/dev/null | tail -n +2 |
    awk '{ print "S|nfs|" $1 "|" $2 }'
fi
)BASH").arg(shQuote(host)));
}

// ============================================================
// Mount dialog
// ============================================================
void MainWindow::showMountShareDialog(const QString &type, const QString &server, const QString &share) {
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Mount a Network Share"));
    QFormLayout *form = new QFormLayout(&dlg);

    QComboBox *typeCombo = new QComboBox(&dlg);
    typeCombo->addItem(tr("SMB / CIFS  (Windows, Samba, most NAS)"), "cifs");
    typeCombo->addItem(tr("NFS  (Unix exports)"), "nfs");
    typeCombo->setCurrentIndex(type == "nfs" ? 1 : 0);
    form->addRow(tr("Protocol:"), typeCombo);

    QLineEdit *serverEdit = new QLineEdit(server, &dlg);
    serverEdit->setPlaceholderText(tr("nas.local or 192.168.1.50"));
    form->addRow(tr("Server:"), serverEdit);

    QLineEdit *shareEdit = new QLineEdit(share, &dlg);
    shareEdit->setPlaceholderText(tr("media   (SMB share name, or /export/path for NFS)"));
    form->addRow(tr("Share:"), shareEdit);

    QLineEdit *mpEdit = new QLineEdit(&dlg);
    mpEdit->setPlaceholderText("/mnt/…");
    form->addRow(tr("Mount at:"), mpEdit);

    QLineEdit *userEdit = new QLineEdit(&dlg);
    userEdit->setPlaceholderText(tr("leave empty for guest access"));
    form->addRow(tr("Username:"), userEdit);

    QLineEdit *domainEdit = new QLineEdit(&dlg);
    domainEdit->setPlaceholderText(tr("optional — Windows domain or workgroup"));
    form->addRow(tr("Domain:"), domainEdit);

    QCheckBox *roCheck = new QCheckBox(tr("Read-only"), &dlg);
    form->addRow(QString(), roCheck);
    QCheckBox *persistCheck = new QCheckBox(tr("Mount automatically at boot (writes /etc/fstab)"), &dlg);
    form->addRow(QString(), persistCheck);
    QCheckBox *automountCheck = new QCheckBox(tr("Connect on first use instead of at boot"), &dlg);
    automountCheck->setToolTip(tr("systemd automount. Keeps boot fast and stops a sleeping NAS from hanging startup."));
    automountCheck->setChecked(true);
    automountCheck->setEnabled(false);
    form->addRow(QString(), automountCheck);
    connect(persistCheck, &QCheckBox::toggled, automountCheck, &QCheckBox::setEnabled);

    QLabel *pwNote = new QLabel(tr("🔒 The password is asked for in the terminal, never stored in this dialog. "
                                   "For boot-time mounts it goes straight into a root-only credentials file."), &dlg);
    pwNote->setWordWrap(true);
    pwNote->setStyleSheet("color:#888;");
    form->addRow(pwNote);

    // Sensible mount point suggestion that follows what's typed
    auto suggest = [mpEdit, shareEdit]() {
        QString base = shareEdit->text().trimmed();
        base = base.section('/', -1);
        base.remove(QRegularExpression("[^A-Za-z0-9._-]"));
        if (!base.isEmpty()) mpEdit->setPlaceholderText("/mnt/" + base);
    };
    connect(shareEdit, &QLineEdit::textChanged, &dlg, suggest);
    suggest();

    auto syncType = [typeCombo, userEdit, domainEdit, shareEdit]() {
        const bool cifs = typeCombo->currentData().toString() == "cifs";
        userEdit->setEnabled(cifs);
        domainEdit->setEnabled(cifs);
        shareEdit->setPlaceholderText(cifs ? tr("media   (the share name on the server)")
                                           : tr("/export/media   (the exported path)"));
    };
    connect(typeCombo, &QComboBox::currentTextChanged, &dlg, syncType);
    syncType();

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    bb->button(QDialogButtonBox::Ok)->setText(tr("Mount"));
    form->addRow(bb);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    const QString proto = typeCombo->currentData().toString();
    const QString srv = serverEdit->text().trimmed();
    QString shr = shareEdit->text().trimmed();
    QString mp = mpEdit->text().trimmed();
    if (mp.isEmpty()) mp = mpEdit->placeholderText();
    const QString user = userEdit->text().trimmed();
    const QString domain = domainEdit->text().trimmed();

    if (srv.isEmpty() || shr.isEmpty()) {
        QMessageBox::warning(this, tr("Missing Details"), tr("Server and share are both required."));
        return;
    }
    if (!QRegularExpression("^[A-Za-z0-9._:-]+$").match(srv).hasMatch()) {
        QMessageBox::warning(this, tr("Invalid Server"), tr("Use a hostname or IP address."));
        return;
    }
    if (!user.isEmpty() && !QRegularExpression("^[A-Za-z0-9._@\\\\-]+$").match(user).hasMatch()) {
        QMessageBox::warning(this, tr("Invalid Username"), tr("That username contains characters Samba will not accept."));
        return;
    }
    if (!mp.startsWith('/')) {
        QMessageBox::warning(this, tr("Invalid Mount Point"), tr("The mount point must be an absolute path, e.g. /mnt/media."));
        return;
    }
    if (isProtectedMountPoint(mp)) {
        QMessageBox::warning(this, tr("Refusing That Mount Point"),
            tr("Mounting a network share over %1 would hide the local system underneath it.\n\n"
               "Use a directory under /mnt or /media instead.").arg(mp));
        return;
    }
    // Mounting over a directory that already has files hides them until unmount
    QDir mpDir(mp);
    if (mpDir.exists() && !mpDir.isEmpty(QDir::AllEntries | QDir::NoDotAndDotDot)) {
        if (QMessageBox::warning(this, tr("Mount Point Is Not Empty"),
                tr("%1 already contains files.\n\nMounting here hides them until you unmount again "
                   "(they are not deleted). Continue?").arg(mp),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    }

    const QString remote = (proto == "cifs") ? QString("//%1/%2").arg(srv, shr)
                                             : QString("%1:%2").arg(srv, shr.startsWith('/') ? shr : "/" + shr);

    // Build the option string. uid/gid map the share onto the current user —
    // without them a CIFS mount ends up owned by root and unusable in a file manager.
    QStringList opts;
    if (proto == "cifs") {
        opts << QString("uid=%1").arg(getuid()) << QString("gid=%1").arg(getgid())
             << "iocharset=utf8" << "file_mode=0664" << "dir_mode=0775";
        if (user.isEmpty()) opts << "guest";
        else {
            opts << "username=" + user;
            if (!domain.isEmpty()) opts << "domain=" + domain;
        }
    }
    opts << (roCheck->isChecked() ? "ro" : "rw");

    QString body = QString("install -d -m 0755 %1\n").arg(shQuote(mp));

    if (persistCheck->isChecked()) {
        const QString credName = QString("%1-%2").arg(srv, shr.section('/', -1))
                                     .remove(QRegularExpression("[^A-Za-z0-9._-]"));
        QStringList fstabOpts = opts;
        fstabOpts << "_netdev" << "nofail";
        if (automountCheck->isChecked())
            fstabOpts << "x-systemd.automount" << "x-systemd.idle-timeout=60";

        if (proto == "cifs" && !user.isEmpty()) {
            fstabOpts.removeAll("username=" + user);
            fstabOpts.removeAll("domain=" + domain);
            fstabOpts << "credentials=/etc/samba/credentials/" + credName;
            body += QString(
                "install -d -m 0700 /etc/samba/credentials\n"
                "printf 'Samba password for %1@%2: '\n"
                "read -rs PW; echo\n"
                "( umask 077; { printf 'username=%1\\n'; printf 'password=%s\\n' \"$PW\";"
                " %3 } > /etc/samba/credentials/%4 )\n"
                "chmod 600 /etc/samba/credentials/%4\n"
                "unset PW\n"
                "echo 'Credentials stored root-only in /etc/samba/credentials/%4'\n")
                .arg(user, srv,
                     domain.isEmpty() ? QString() : QString("printf 'domain=%1\\n';").arg(domain),
                     credName);
        }

        const QString fstabLine = QString("%1 %2 %3 %4 0 0")
                                      .arg(remote, mp, proto == "cifs" ? "cifs" : "nfs", fstabOpts.join(','));
        body += QString(
            "cp -v /etc/fstab /etc/fstab.bak.$(date +%F_%H-%M-%S)\n"
            "if awk '$2 == %1 && $1 !~ /^#/' /etc/fstab | grep -q .; then\n"
            "  echo 'An fstab entry for that mount point already exists — replacing it.'\n"
            "  awk '$2 != %1' /etc/fstab > /tmp/fstab.new && cat /tmp/fstab.new > /etc/fstab && rm -f /tmp/fstab.new\n"
            "fi\n"
            "printf '%s\\n' %2 >> /etc/fstab\n"
            "systemctl daemon-reload\n"
            "echo 'Added to /etc/fstab:'; tail -1 /etc/fstab\n"
            "mount %1\n").arg(shQuote(mp), shQuote(fstabLine));
    } else {
        body += QString("mount -t %1 %2 %3 -o %4\n")
                    .arg(proto, shQuote(remote), shQuote(mp), opts.join(','));
    }

    body += QString("echo ''; echo 'Mounted:'; findmnt %1\n").arg(shQuote(mp));

    runScriptInTerminal(repairScript(body), "mount_share");
    scheduleStagedRecheck([this]() { if (networkSharesVisible()) refreshMountedShares(); });
}

// ============================================================
// Samba server side
// ============================================================
void MainWindow::refreshSambaShares() {
    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, proc](int, QProcess::ExitStatus) {
        const QString out = QString::fromUtf8(proc->readAllStandardOutput());
        proc->deleteLater();

        QHash<QString, QString> meta;
        shrSambaTable->setRowCount(0);
        for (const QString &line : out.split('\n', Qt::SkipEmptyParts)) {
            if (line.startsWith("SHARE|")) {
                const QStringList f = line.mid(6).split('|');
                if (f.size() < 5) continue;
                const int row = shrSambaTable->rowCount();
                shrSambaTable->insertRow(row);
                shrSambaTable->setItem(row, 0, new QTableWidgetItem(f[0]));
                shrSambaTable->setItem(row, 1, new QTableWidgetItem(f[1]));
                // testparm omits keys left at their default; "read only" defaults to yes
                const bool writable = f[2].startsWith("No", Qt::CaseInsensitive);
                QTableWidgetItem *access = new QTableWidgetItem(writable ? tr("read / write") : tr("read-only"));
                access->setForeground(writable ? QColor("#e67e22") : QColor("#27ae60"));
                shrSambaTable->setItem(row, 2, access);
                const bool guest = f[3].startsWith("Yes", Qt::CaseInsensitive);
                QTableWidgetItem *guestItem = new QTableWidgetItem(guest ? tr("⚠️ allowed") : tr("no"));
                if (guest) guestItem->setForeground(QColor("#e67e22"));
                shrSambaTable->setItem(row, 3, guestItem);
                shrSambaTable->setItem(row, 4, new QTableWidgetItem(f[4].isEmpty() ? tr("any valid user") : f[4]));
                continue;
            }
            const int eq = line.indexOf('=');
            if (eq > 0) meta.insert(line.left(eq), line.mid(eq + 1).trimmed());
        }
        shrSambaTable->resizeColumnToContents(0);

        if (meta.value("SMBD") != "1") {
            shrSambaStatusLabel->setText(tr("<span style='color:#888;'>Samba is not installed. "
                                            "Use <b>Enable Server</b> to install it and create a starter config.</span>"));
            return;
        }
        const QString state = meta.value("SMBSTATE");
        const bool active = (state == "active");
        QString html = QString("<b style='color:%1;'>%2</b>")
                           .arg(active ? "#27ae60" : "#c0392b",
                                active ? tr("🟢 Samba server running") : tr("🔴 Samba server stopped"));
        html += QString("<span style='color:#888;'> · %1 at boot · %2</span>")
                    .arg(meta.value("SMBENABLED") == "enabled" ? tr("starts") : tr("does not start"),
                         tr("%n share(s) defined", "", shrSambaTable->rowCount()));
        if (active && shrSambaTable->rowCount() > 0 && !meta.value("IP").isEmpty())
            html += tr("<br><span style='color:#888;'>Reachable as <code>\\\\%1</code> or "
                       "<code>smb://%1/</code> from other machines.</span>").arg(meta.value("IP"));
        shrSambaStatusLabel->setText(html);
    });

    proc->start("bash", QStringList() << "-c" << R"BASH(
command -v smbd >/dev/null 2>&1 && echo "SMBD=1" || echo "SMBD=0"
echo "SMBSTATE=$(systemctl is-active smb 2>/dev/null)"
echo "SMBENABLED=$(systemctl is-enabled smb 2>/dev/null)"
echo "IP=$(ip -4 -o addr show scope global 2>/dev/null | awk '{print $4}' | cut -d/ -f1 | head -1)"

if command -v testparm >/dev/null 2>&1 && [ -f /etc/samba/smb.conf ]; then
  testparm -s 2>/dev/null | awk '
    /^[[:space:]]*\[.*\][[:space:]]*$/ {
      sec=$0; gsub(/^[[:space:]]*\[|\][[:space:]]*$/,"",sec)
      if (sec != "global" && sec != "printers" && sec != "print$") order[++n]=sec
      next
    }
    {
      if (sec=="" || sec=="global") next
      p=index($0,"=");  if (p==0) next
      k=substr($0,1,p-1); v=substr($0,p+1)
      gsub(/^[ \t]+|[ \t]+$/,"",k); gsub(/^[ \t]+|[ \t]+$/,"",v)
      vals[sec "|" k]=v
    }
    END {
      for (i=1;i<=n;i++) {
        s=order[i]
        printf "SHARE|%s|%s|%s|%s|%s\n", s, vals[s "|path"], vals[s "|read only"],
               vals[s "|guest ok"], vals[s "|valid users"]
      }
    }'
fi
)BASH");
}

void MainWindow::showAddSambaShareDialog() {
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Share a Folder"));
    QFormLayout *form = new QFormLayout(&dlg);

    QLineEdit *nameEdit = new QLineEdit(&dlg);
    nameEdit->setPlaceholderText(tr("Media   — how it appears to other machines"));
    form->addRow(tr("Share name:"), nameEdit);

    QHBoxLayout *pathLay = new QHBoxLayout();
    QLineEdit *pathEdit = new QLineEdit(&dlg);
    pathEdit->setPlaceholderText(QDir::homePath() + "/Public");
    QPushButton *browseBtn = new QPushButton(tr("📁"), &dlg);
    browseBtn->setFixedWidth(36);
    pathLay->addWidget(pathEdit, 1);
    pathLay->addWidget(browseBtn);
    form->addRow(tr("Folder:"), pathLay);

    QCheckBox *writableCheck = new QCheckBox(tr("Allow writing (otherwise read-only)"), &dlg);
    form->addRow(QString(), writableCheck);
    QCheckBox *guestCheck = new QCheckBox(tr("Allow guests — no password required"), &dlg);
    form->addRow(QString(), guestCheck);
    QCheckBox *browseCheck = new QCheckBox(tr("Show in network browsing"), &dlg);
    browseCheck->setChecked(true);
    form->addRow(QString(), browseCheck);

    QLineEdit *usersEdit = new QLineEdit(QProcessEnvironment::systemEnvironment().value("USER"), &dlg);
    usersEdit->setPlaceholderText(tr("space-separated user names, empty = any user with a Samba password"));
    form->addRow(tr("Allowed users:"), usersEdit);

    QLabel *warn = new QLabel(&dlg);
    warn->setWordWrap(true);
    warn->setTextFormat(Qt::RichText);
    form->addRow(warn);
    auto updateWarn = [warn, guestCheck, writableCheck]() {
        if (guestCheck->isChecked() && writableCheck->isChecked())
            warn->setText(tr("<b style='color:#c0392b;'>⚠️ Guest + writable</b> means anyone who can reach this "
                             "machine on the network can add, change and delete files in that folder. "
                             "Only do this on a network you fully control."));
        else if (guestCheck->isChecked())
            warn->setText(tr("<span style='color:#e67e22;'>Anyone on the network can read this folder "
                             "without a password.</span>"));
        else
            warn->setText(tr("<span style='color:#888;'>Users need a Samba password — set one with "
                             "<i>Set Samba Password</i> after creating the share.</span>"));
    };
    connect(guestCheck, &QCheckBox::toggled, &dlg, updateWarn);
    connect(writableCheck, &QCheckBox::toggled, &dlg, updateWarn);
    updateWarn();

    connect(browseBtn, &QPushButton::clicked, &dlg, [&dlg, pathEdit]() {
        const QString d = QFileDialog::getExistingDirectory(&dlg, tr("Choose a folder to share"),
                                                            pathEdit->text().isEmpty() ? QDir::homePath() : pathEdit->text());
        if (!d.isEmpty()) pathEdit->setText(d);
    });
    // Name the share after the folder unless the user typed something themselves
    connect(pathEdit, &QLineEdit::textChanged, &dlg, [nameEdit](const QString &p) {
        if (nameEdit->text().isEmpty() || nameEdit->property("auto").toBool()) {
            QString base = QDir(p).dirName();
            base.remove(QRegularExpression("[^A-Za-z0-9_-]"));
            nameEdit->setText(base);
            nameEdit->setProperty("auto", true);
        }
    });
    connect(nameEdit, &QLineEdit::textEdited, &dlg, [nameEdit]() { nameEdit->setProperty("auto", false); });

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    bb->button(QDialogButtonBox::Ok)->setText(tr("Share"));
    form->addRow(bb);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    const QString name = nameEdit->text().trimmed();
    const QString path = QDir::cleanPath(pathEdit->text().trimmed());
    const QString users = usersEdit->text().trimmed();

    if (!QRegularExpression("^[A-Za-z0-9_-]{1,64}$").match(name).hasMatch()) {
        QMessageBox::warning(this, tr("Invalid Share Name"),
                             tr("Use letters, digits, hyphens and underscores only."));
        return;
    }
    if (path.isEmpty() || !QDir(path).exists()) {
        QMessageBox::warning(this, tr("No Such Folder"), tr("Pick a folder that exists."));
        return;
    }
    if (isProtectedSharePath(path)) {
        QMessageBox::warning(this, tr("Refusing to Share That"),
            tr("%1 holds system or credential data — exporting it to the network would hand out "
               "keys, configs or the whole filesystem.\n\nShare a specific folder inside your home instead.").arg(path));
        return;
    }
    if (!users.isEmpty() && !QRegularExpression("^[a-z_][a-z0-9_ -]*$").match(users).hasMatch()) {
        QMessageBox::warning(this, tr("Invalid User List"), tr("Use space-separated Linux user names."));
        return;
    }
    for (int r = 0; r < shrSambaTable->rowCount(); ++r) {
        if (shrSambaTable->item(r, 0)->text().compare(name, Qt::CaseInsensitive) == 0) {
            QMessageBox::warning(this, tr("Name Already Used"),
                                 tr("A share called [%1] already exists. Pick another name.").arg(name));
            return;
        }
    }
    if (guestCheck->isChecked() && writableCheck->isChecked() &&
        QMessageBox::warning(this, tr("Guest-Writable Share"),
            tr("Anyone on this network will be able to delete files in %1.\n\nCreate it anyway?").arg(path),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;

    QString block = QString("\n[%1]\n   path = %2\n   browseable = %3\n   read only = %4\n   guest ok = %5\n")
                        .arg(name, path,
                             browseCheck->isChecked() ? "yes" : "no",
                             writableCheck->isChecked() ? "no" : "yes",
                             guestCheck->isChecked() ? "yes" : "no");
    if (!users.isEmpty()) block += QString("   valid users = %1\n").arg(users);
    if (writableCheck->isChecked()) block += "   create mask = 0664\n   directory mask = 0775\n";

    QString body =
        "CONF=/etc/samba/smb.conf\n"
        "if ! command -v smbd >/dev/null 2>&1; then echo 'Installing samba...'; pacman -S --needed samba; fi\n"
        "if [ ! -f \"$CONF\" ]; then\n"
        "  install -d /etc/samba\n"
        "  printf '[global]\\n   workgroup = WORKGROUP\\n   server string = %s\\n"
        "   security = user\\n' \"$(hostname)\" > \"$CONF\"\n"
        "fi\n"
        "cp -v \"$CONF\" \"$CONF.bak.$(date +%F_%H-%M-%S)\"\n";

    if (guestCheck->isChecked())
        body += "grep -q 'map to guest' \"$CONF\" || "
                "sed -i '0,/^\\[global\\]/{s//[global]\\n   map to guest = Bad User/}' \"$CONF\"\n";

    body += QString("cat >> \"$CONF\" <<'CACHYEOF'\n%1CACHYEOF\n").arg(block);
    body +=
        "echo ''; echo 'Checking the configuration...'\n"
        "testparm -s >/dev/null\n"
        "systemctl enable --now smb nmb\n"
        "systemctl reload smb 2>/dev/null || systemctl restart smb\n"
        "echo ''; echo 'Share is live. Shares now defined:'\n"
        "testparm -s 2>/dev/null | grep -E '^\\[' | grep -v global\n";

    if (!guestCheck->isChecked())
        body += QString("echo ''; echo 'Remember: %1 needs a Samba password before they can connect.'\n"
                        "echo 'Set one with:  sudo smbpasswd -a <user>'\n")
                    .arg(users.isEmpty() ? tr("each user") : users);

    runScriptInTerminal(repairScript(body), "samba_share");
    scheduleStagedRecheck([this]() { if (networkSharesVisible()) refreshSambaShares(); });
}
