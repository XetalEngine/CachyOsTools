
// PKG Install -> Pacman -> 🩺 Repair sub-tab: the five things that actually
// break pacman on an Arch box, each with a diagnosis and a one-click fix.
//
//   stale mirrors · broken keyring · leftover db.lck · a bad update that
//   needs a downgrade · a pacman.conf nobody ever tuned
//
// Every repair runs in a visible terminal (Show, don't hide) and every config
// write is preceded by a timestamped backup (Backup before touch).

#include <QFormLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QFileSystemWatcher>
#include <QScrollArea>
#include "script_helpers.h"

// Everything the diagnosis reads that can change under us. Watching these means
// the panel goes green the moment a repair lands — including repairs the user ran
// in their own terminal, with this app just sitting there.
static QStringList pacmanWatchPaths() {
    QStringList paths = {
        "/etc/pacman.conf",
        "/etc/pacman.d",              // mirrorlist replaced, gnupg rebuilt
        "/etc/pacman.d/mirrorlist",
        "/var/lib/pacman",            // db.lck appears and disappears here
        "/var/lib/pacman/sync",       // -Sy rewrites the databases
        "/var/lib/pacman/local",      // a package installed or removed
        "/var/cache/pacman/pkg",      // cache trimmed or filled
        "/etc",                       // .pacnew files merged away
    };
    if (QFile::exists("/etc/pacman.d/cachyos-mirrorlist"))
        paths << "/etc/pacman.d/cachyos-mirrorlist";
    return paths;
}

// Repairs are detached terminals with no exit signal we can see. The watcher above
// catches most of it instantly; this covers what it cannot — .pacnew files merged
// from all over /etc, orphan counts, a keyring rebuild. Each tick is a no-op unless
// the panel is actually on screen.
void MainWindow::scheduleStagedRecheck(const std::function<void()> &recheck) {
    for (int delay : {2000, 8000, 20000, 45000, 90000})
        QTimer::singleShot(delay, this, recheck);
}

bool MainWindow::pacmanDoctorVisible() const {
    return pacDocTab && isVisible() && !isMinimized()
        && ui->tabWidget->currentWidget() == ui->packageManagerTab
        && ui->pacmanTabWidget->currentWidget() == pacDocTab;
}

// ============================================================
// Tab construction
// ============================================================
void MainWindow::setupPacmanDoctorTab() {
    pacDocTab = new QWidget();
    pacDocTab->setObjectName("pacmanDoctorTab");

    // Four stacked cards plus a file list do not fit once PKG Install has eaten
    // ~180px in nested tab bars. Without a scroll area the diagnosis card is the
    // one that gets squeezed, and it clips its last check straight off the bottom.
    QVBoxLayout *tabLay = new QVBoxLayout(pacDocTab);
    tabLay->setContentsMargins(0, 0, 0, 0);
    QScrollArea *pacDocScroll = new QScrollArea(pacDocTab);
    pacDocScroll->setWidgetResizable(true);
    pacDocScroll->setFrameShape(QFrame::NoFrame);
    tabLay->addWidget(pacDocScroll);
    QWidget *content = new QWidget();
    QVBoxLayout *root = new QVBoxLayout(content);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(6);

    // --- top bar ---------------------------------------------------------
    QHBoxLayout *bar = new QHBoxLayout();
    QPushButton *diagBtn = new QPushButton(tr("🩺 Diagnose"), pacDocTab);
    diagBtn->setToolTip(tr("Read-only health check. Nothing is changed."));
    pacDocStatusLabel = new QLabel(tr("Press Diagnose to check the health of your package manager."), pacDocTab);
    pacDocStatusLabel->setStyleSheet("color:#888;");
    bar->addWidget(diagBtn);
    bar->addWidget(pacDocStatusLabel, 1);
    root->addLayout(bar);

    // --- diagnosis card --------------------------------------------------
    QGroupBox *diagBox = new QGroupBox(tr("Diagnosis"), pacDocTab);
    QVBoxLayout *diagLay = new QVBoxLayout(diagBox);
    pacDocDiagLabel = new QLabel(tr("Not checked yet."), diagBox);
    pacDocDiagLabel->setTextFormat(Qt::RichText);
    pacDocDiagLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    pacDocDiagLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    pacDocDiagLabel->setWordWrap(true);
    diagLay->addWidget(pacDocDiagLabel);
    root->addWidget(diagBox);

    // --- repair actions --------------------------------------------------
    QGroupBox *fixBox = new QGroupBox(tr("Repair actions"), pacDocTab);
    QHBoxLayout *fixLay = new QHBoxLayout(fixBox);
    QPushButton *mirrorBtn = new QPushButton(tr("🌍 Rank Mirrors…"), fixBox);
    mirrorBtn->setToolTip(tr("Re-sort /etc/pacman.d/mirrorlist by speed. Fixes slow or 404-ing downloads."));
    QPushButton *keyBtn = new QPushButton(tr("🔑 Fix Keyring…"), fixBox);
    keyBtn->setToolTip(tr("For \"invalid or corrupted package (PGP signature)\" and \"unknown trust\" errors."));
    QPushButton *lockBtn = new QPushButton(tr("🔓 Remove db.lck"), fixBox);
    lockBtn->setToolTip(tr("For \"unable to lock database\" after pacman was killed mid-run."));
    QPushButton *downBtn = new QPushButton(tr("⏬ Downgrade Package…"), fixBox);
    downBtn->setToolTip(tr("Roll a package back to an older version still sitting in your cache."));
    QPushButton *trimBtn = new QPushButton(tr("🧹 Trim Cache…"), fixBox);
    trimBtn->setToolTip(tr("Delete old cached versions but keep enough to downgrade with."));
    QPushButton *syncBtn = new QPushButton(tr("🔄 Force Refresh"), fixBox);
    syncBtn->setToolTip(tr("pacman -Syyu — re-downloads every database even if it looks current."));
    for (QPushButton *b : {mirrorBtn, keyBtn, lockBtn, downBtn, trimBtn, syncBtn}) fixLay->addWidget(b);
    fixLay->addStretch();
    root->addWidget(fixBox);

    connect(diagBtn, &QPushButton::clicked, this, &MainWindow::refreshPacmanDoctor);
    connect(mirrorBtn, &QPushButton::clicked, this, &MainWindow::pacmanRankMirrorsDialog);
    connect(keyBtn, &QPushButton::clicked, this, &MainWindow::pacmanFixKeyringDialog);
    connect(downBtn, &QPushButton::clicked, this, &MainWindow::pacmanDowngradeDialog);

    connect(lockBtn, &QPushButton::clicked, this, [this]() {
        if (!QFile::exists("/var/lib/pacman/db.lck")) {
            QMessageBox::information(this, tr("No Lock File"),
                tr("/var/lib/pacman/db.lck does not exist — the database is not locked."));
            return;
        }
        // Refuse the footgun: a live pacman owns that lock legitimately.
        QProcess busy;
        busy.start("bash", QStringList() << "-c" << "pgrep -x pacman >/dev/null && echo busy || echo free");
        busy.waitForFinished(3000);
        if (QString::fromUtf8(busy.readAllStandardOutput()).trimmed() == "busy") {
            QMessageBox::warning(this, tr("pacman Is Running"),
                tr("A pacman process is running right now — the lock is real, not stale.\n\n"
                   "Wait for it to finish (check the Updates tab or your terminal). "
                   "Deleting the lock under a live pacman corrupts the package database."));
            return;
        }
        if (QMessageBox::question(this, tr("Remove Stale Lock"),
                tr("No pacman process is running, so /var/lib/pacman/db.lck is left over from a "
                   "crashed or killed run.\n\nDelete it?"),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
        runScriptInTerminal(repairScript(
            "if pgrep -x pacman >/dev/null; then\n"
            "  echo 'pacman started in the meantime — leaving the lock alone.'\n"
            "else\n"
            "  rm -v /var/lib/pacman/db.lck\n"
            "  echo 'Lock removed. pacman is usable again.'\n"
            "fi\n"), "pacman_unlock");
        scheduleStagedRecheck([this]() { if (pacmanDoctorVisible()) refreshPacmanDoctor(); });
    });

    connect(trimBtn, &QPushButton::clicked, this, [this]() {
        QStringList opts;
        opts << tr("Keep the last 3 versions of each package (recommended)")
             << tr("Keep the last 1 version of each package")
             << tr("Remove only packages that are no longer installed")
             << tr("Empty the cache completely (no downgrades possible afterwards)");
        bool ok;
        QString choice = QInputDialog::getItem(this, tr("Trim Package Cache"),
            tr("The cache is what makes downgrades possible. Pick how much history to keep:"),
            opts, 0, false, &ok);
        if (!ok) return;
        int idx = opts.indexOf(choice);

        QString cmd;
        if (QFile::exists("/usr/bin/paccache")) {
            if (idx == 0) cmd = "paccache -rvk3";
            else if (idx == 1) cmd = "paccache -rvk1";
            else if (idx == 2) cmd = "paccache -rvuk0";
            else cmd = "paccache -rvk0";
        } else {
            if (idx == 3) cmd = "pacman -Scc --noconfirm";
            else {
                QMessageBox::information(this, tr("paccache Missing"),
                    tr("Version-aware trimming needs paccache (package: pacman-contrib).\n\n"
                       "Install it with:  sudo pacman -S pacman-contrib"));
                return;
            }
        }
        if (idx == 3 && QMessageBox::warning(this, tr("Empty the Cache"),
                tr("Emptying the cache means you can no longer downgrade a package that breaks — "
                   "you would have to re-download every version from a mirror or the archive.\n\nContinue?"),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;

        runScriptInTerminal(repairScript("du -sh /var/cache/pacman/pkg\n" + cmd +
                                         "\necho ''; echo 'Cache is now:'; du -sh /var/cache/pacman/pkg\n"),
                            "pacman_trim_cache");
        scheduleStagedRecheck([this]() { if (pacmanDoctorVisible()) refreshPacmanDoctor(); });
    });

    connect(syncBtn, &QPushButton::clicked, this, [this]() {
        if (QMessageBox::question(this, tr("Force Database Refresh"),
                tr("Run a full  pacman -Syyu  in a terminal?\n\n"
                   "This re-downloads every package database and then upgrades the system. "
                   "Read the Arch news on the Updates tab first if you have not upgraded in a while."),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
        runScriptInTerminal("pacman -Syyu\n", "pacman_force_sync");
    });

    // --- pacman.conf tuning ----------------------------------------------
    QGroupBox *confBox = new QGroupBox(tr("pacman.conf"), pacDocTab);
    QHBoxLayout *confLay = new QHBoxLayout(confBox);
    confLay->addWidget(new QLabel(tr("Parallel downloads:"), confBox));
    pacDocParallelSpin = new QSpinBox(confBox);
    pacDocParallelSpin->setRange(1, 20);
    pacDocParallelSpin->setValue(5);
    pacDocParallelSpin->setToolTip(tr("How many packages download at once. 5–10 is a good range."));
    confLay->addWidget(pacDocParallelSpin);
    pacDocColorCheck = new QCheckBox(tr("Color"), confBox);
    pacDocCandyCheck = new QCheckBox(tr("ILoveCandy"), confBox);
    pacDocCandyCheck->setToolTip(tr("Pac-Man progress bar. Purely decorative, entirely worth it."));
    pacDocVerboseCheck = new QCheckBox(tr("VerbosePkgLists"), confBox);
    pacDocVerboseCheck->setToolTip(tr("Show version and size columns before confirming an upgrade."));
    pacDocCheckSpaceCheck = new QCheckBox(tr("CheckSpace"), confBox);
    pacDocCheckSpaceCheck->setToolTip(tr("Refuse to start an upgrade that would fill the disk."));
    pacDocTimeoutCheck = new QCheckBox(tr("DisableDownloadTimeout"), confBox);
    pacDocTimeoutCheck->setToolTip(tr("Turn off the slow-download abort. Useful on bad connections."));
    for (QCheckBox *c : {pacDocColorCheck, pacDocCandyCheck, pacDocVerboseCheck,
                         pacDocCheckSpaceCheck, pacDocTimeoutCheck}) confLay->addWidget(c);
    confLay->addStretch();
    QPushButton *confBackupBtn = new QPushButton(tr("💾 Backup"), confBox);
    QPushButton *confApplyBtn = new QPushButton(tr("✅ Apply"), confBox);
    QPushButton *confEditBtn = new QPushButton(tr("✏️ Edit"), confBox);
    for (QPushButton *b : {confBackupBtn, confApplyBtn, confEditBtn}) confLay->addWidget(b);
    root->addWidget(confBox);

    connect(confBackupBtn, &QPushButton::clicked, this, [this]() {
        backupConfigFile("/etc/pacman.conf", "pacman configuration");
    });
    connect(confEditBtn, &QPushButton::clicked, this, [this]() { openBuiltinEditor("/etc/pacman.conf"); });
    connect(confApplyBtn, &QPushButton::clicked, this, &MainWindow::pacmanApplyConfTuning);

    // --- pending .pacnew / .pacsave --------------------------------------
    QGroupBox *pacnewBox = new QGroupBox(tr("Pending .pacnew / .pacsave files"), pacDocTab);
    QVBoxLayout *pacnewLay = new QVBoxLayout(pacnewBox);
    QLabel *pacnewHint = new QLabel(
        tr("An upgrade could not merge these config files for you. Until you deal with them, "
           "you are running the <i>old</i> config — that is how a working service quietly stops working."), pacnewBox);
    pacnewHint->setWordWrap(true);
    pacnewHint->setStyleSheet("color:#888;");
    pacnewLay->addWidget(pacnewHint);
    pacDocPacnewList = new QListWidget(pacnewBox);
    pacDocPacnewList->setMaximumHeight(96);
    pacDocPacnewList->setFont(QFont("monospace"));
    pacnewLay->addWidget(pacDocPacnewList);
    QHBoxLayout *pacnewBar = new QHBoxLayout();
    QPushButton *pacdiffBtn = new QPushButton(tr("🔍 Merge All (pacdiff)"), pacnewBox);
    pacdiffBtn->setToolTip(tr("Walk through every pending file side by side in a terminal."));
    QPushButton *pacnewEditBtn = new QPushButton(tr("✏️ Open Selected"), pacnewBox);
    QPushButton *pacnewOldBtn = new QPushButton(tr("📄 Open Current Version"), pacnewBox);
    pacnewBar->addStretch();
    for (QPushButton *b : {pacdiffBtn, pacnewEditBtn, pacnewOldBtn}) pacnewBar->addWidget(b);
    pacnewLay->addLayout(pacnewBar);
    root->addWidget(pacnewBox);

    connect(pacdiffBtn, &QPushButton::clicked, this, [this]() {
        if (!QFile::exists("/usr/bin/pacdiff")) {
            QMessageBox::information(this, tr("pacdiff Missing"),
                tr("pacdiff ships with pacman-contrib:\n\n  sudo pacman -S pacman-contrib"));
            return;
        }
        // pacdiff is interactive by design — the terminal is the right place for it.
        runSudoCommandInTerminal("sudo DIFFPROG=\"${DIFFPROG:-vimdiff}\" pacdiff; "
                                 "echo ''; echo 'pacdiff finished. Press Enter to close.'; read -r");
    });
    auto openPacnew = [this](bool original) {
        QListWidgetItem *it = pacDocPacnewList->currentItem();
        if (!it) {
            QMessageBox::information(this, tr("No Selection"), tr("Select a file from the list first."));
            return;
        }
        QString path = it->text();
        if (original) {
            path.remove(QRegularExpression("\\.pac(new|save)$"));
            if (!QFile::exists(path)) {
                QMessageBox::information(this, tr("No Current Version"),
                    tr("%1 does not exist — the package added this file for the first time.").arg(path));
                return;
            }
        }
        openBuiltinEditor(path);
    };
    connect(pacnewEditBtn, &QPushButton::clicked, this, [openPacnew]() { openPacnew(false); });
    connect(pacnewOldBtn, &QPushButton::clicked, this, [openPacnew]() { openPacnew(true); });

    root->addStretch();
    pacDocScroll->setWidget(content);
    ui->pacmanTabWidget->addTab(pacDocTab, tr("🩺 Repair"));

    // Every check here is read-only and unprivileged, so re-diagnose whenever the
    // tab is opened rather than making the user press the button first.
    connect(ui->pacmanTabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        if (ui->pacmanTabWidget->widget(index) == pacDocTab) refreshPacmanDoctor();
    });

    // The shared package-output pane sits below the sub-tabs and belongs to the
    // install/search pages — Repair never writes to it, and its 150px is exactly
    // what the diagnosis needs to fit without scrolling. Give it back while
    // Repair is on screen, restore it the moment anything else is.
    auto syncOutputPane = [this]() {
        const bool onRepair = ui->packageManagerTabWidget->currentWidget() == ui->pacmanTab
                           && ui->pacmanTabWidget->currentWidget() == pacDocTab;
        ui->packageOutputText->setVisible(!onRepair);
    };
    connect(ui->pacmanTabWidget, &QTabWidget::currentChanged, this, syncOutputPane);
    connect(ui->packageManagerTabWidget, &QTabWidget::currentChanged, this, syncOutputPane);
    syncOutputPane();

    // --- live re-diagnosis --------------------------------------------------
    // A fix runs in its own terminal window; we never learn when it finished.
    // Watching the files the diagnosis reads turns each dot green the instant the
    // repair actually lands, instead of at the next app start.
    pacDocDebounce = new QTimer(this);
    pacDocDebounce->setSingleShot(true);
    pacDocDebounce->setInterval(3000);   // coalesce the storm a -Syu makes
    connect(pacDocDebounce, &QTimer::timeout, this, [this]() {
        if (pacmanDoctorVisible()) refreshPacmanDoctor();
    });

    pacDocWatcher = new QFileSystemWatcher(this);
    auto rearmWatcher = [this]() {
        // reflector --save and sed -i replace the file, and the watch dies with the
        // old inode — so re-add anything that fell off after every event.
        const QStringList watched = pacDocWatcher->files() + pacDocWatcher->directories();
        for (const QString &p : pacmanWatchPaths())
            if (QFile::exists(p) && !watched.contains(p)) pacDocWatcher->addPath(p);
    };
    rearmWatcher();
    auto onChanged = [this, rearmWatcher](const QString &) {
        rearmWatcher();
        pacDocDebounce->start();
    };
    connect(pacDocWatcher, &QFileSystemWatcher::fileChanged, this, onChanged);
    connect(pacDocWatcher, &QFileSystemWatcher::directoryChanged, this, onChanged);
}

// ============================================================
// Diagnosis
// ============================================================
void MainWindow::refreshPacmanDoctor() {
    if (pacDocBusy) return;   // the watcher can fire faster than du can finish
    pacDocBusy = true;
    pacDocStatusLabel->setText(tr("Checking…"));
    pacDocStatusLabel->setStyleSheet("color:#888;");

    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, proc](int, QProcess::ExitStatus) {
        pacDocBusy = false;
        QHash<QString, QString> v;
        QStringList pacnewFiles;
        for (const QString &line : QString::fromUtf8(proc->readAllStandardOutput()).split('\n')) {
            if (line.startsWith("PACNEWFILE=")) { pacnewFiles << line.mid(11); continue; }
            int eq = line.indexOf('=');
            if (eq > 0) v.insert(line.left(eq), line.mid(eq + 1).trimmed());
        }
        proc->deleteLater();

        int problems = 0, warnings = 0;
        QString rows;
        // status: 0 ok, 1 warning, 2 problem
        auto row = [&](int status, const QString &check, const QString &verdict, const QString &advice) {
            if (status == 2) problems++;
            else if (status == 1) warnings++;
            const char *dot = status == 2 ? "🔴" : (status == 1 ? "🟠" : "🟢");
            const char *color = status == 2 ? "#c0392b" : (status == 1 ? "#e67e22" : "#27ae60");
            rows += QString("<tr>"
                            "<td style='padding:4px 10px 4px 0; vertical-align:top;'>%1</td>"
                            "<td style='padding:4px 18px 4px 0; vertical-align:top; color:#888;'>%2</td>"
                            "<td style='padding:4px 0; vertical-align:top;'><b style='color:%3;'>%4</b>%5</td>"
                            "</tr>")
                        .arg(dot, check.toHtmlEscaped(), color, verdict.toHtmlEscaped(),
                             advice.isEmpty() ? QString()
                                              : QString("<br><span style='color:#888;'>%1</span>").arg(advice.toHtmlEscaped()));
        };

        // --- database lock ---
        if (v.value("LOCK") == "1") {
            bool live = v.value("LOCKPROC") == "1";
            row(live ? 1 : 2, tr("Database lock"),
                live ? tr("Locked — pacman is running") : tr("Stale lock file"),
                live ? tr("Legitimate: wait for the running pacman to finish.")
                     : tr("Nothing is running. Use Remove db.lck."));
        } else {
            row(0, tr("Database lock"), tr("Not locked"), QString());
        }

        // --- mirrorlist freshness ---
        int mirrors = v.value("MIRRORS").toInt();
        int mirrorAge = v.value("MIRRORAGE").toInt();
        if (mirrors == 0)
            row(2, tr("Mirrorlist"), tr("No active mirrors"),
                tr("Every Server line is commented out — pacman cannot download anything. Use Rank Mirrors."));
        else if (mirrorAge > 180)
            row(1, tr("Mirrorlist"), tr("%1 mirrors, last ranked %2 days ago").arg(mirrors).arg(mirrorAge),
                tr("Old mirror lists go stale and 404. Re-rank them."));
        else
            row(0, tr("Mirrorlist"), tr("%1 mirrors, last ranked %2 days ago").arg(mirrors).arg(mirrorAge), QString());

        // --- sync database age ---
        int syncAge = v.value("SYNCAGE").toInt();
        if (v.value("SYNCAGE").isEmpty() || syncAge < 0)
            row(2, tr("Sync databases"), tr("Missing"), tr("No databases in /var/lib/pacman/sync. Use Force Refresh."));
        else if (syncAge > 14)
            row(1, tr("Sync databases"), tr("%1 days old").arg(syncAge),
                tr("A partial upgrade risk: never install a single package against a stale database."));
        else
            row(0, tr("Sync databases"), tr("%1 days old").arg(syncAge), QString());

        // --- keyring ---
        if (v.value("GNUPG") != "1")
            row(2, tr("Keyring"), tr("Not initialised"),
                tr("/etc/pacman.d/gnupg has no trust database. Use Fix Keyring."));
        else {
            int keyAge = v.value("KEYRINGAGE").toInt();
            QString kv = v.value("KEYRINGVER");
            if (keyAge > 120)
                row(1, tr("Keyring"), tr("%1, installed %2 days ago").arg(kv.isEmpty() ? tr("unknown") : kv).arg(keyAge),
                    tr("An old keyring is the usual cause of \"invalid or corrupted package (PGP signature)\"."));
            else
                row(0, tr("Keyring"), tr("%1, installed %2 days ago").arg(kv.isEmpty() ? tr("unknown") : kv).arg(keyAge), QString());
        }

        // --- cache ---
        qint64 cacheBytes = v.value("CACHEBYTES").toLongLong();
        int cacheCount = v.value("CACHECOUNT").toInt();
        qint64 freeBytes = v.value("VARFREE").toLongLong();
        if (freeBytes > 0 && freeBytes < 2LL * 1024 * 1024 * 1024)
            row(2, tr("Disk space on /var"), tr("%1 free").arg(formatSize(freeBytes)),
                tr("Upgrades need room to unpack. Trim the cache or free space now."));
        else
            row(0, tr("Disk space on /var"), tr("%1 free").arg(formatSize(freeBytes)), QString());
        row(cacheBytes > 20LL * 1024 * 1024 * 1024 ? 1 : 0, tr("Package cache"),
            tr("%1 in %2 files").arg(formatSize(cacheBytes)).arg(cacheCount),
            cacheBytes > 20LL * 1024 * 1024 * 1024 ? tr("Large. Trim it, but keep 3 versions so downgrades stay possible.") : QString());

        // --- pacnew files ---
        if (!pacnewFiles.isEmpty())
            row(1, tr("Config merges"), tr("%1 pending .pacnew/.pacsave").arg(pacnewFiles.size()),
                tr("Listed below — merge them before they bite."));
        else
            row(0, tr("Config merges"), tr("None pending"), QString());

        // --- orphans / foreign ---
        int orphans = v.value("ORPHANS").toInt();
        row(orphans > 0 ? 1 : 0, tr("Orphaned packages"), tr("%1").arg(orphans),
            orphans > 0 ? tr("Dependencies nothing needs any more. The Uninstall tab can clean them.") : QString());
        row(0, tr("Foreign (AUR) packages"), tr("%1").arg(v.value("FOREIGN").toInt()), QString());

        // --- available rankers, for the mirror dialog ---
        pacDocRankers = v.value("RANKERS").split(' ', Qt::SkipEmptyParts);
        row(pacDocRankers.isEmpty() ? 1 : 0, tr("Mirror ranking tool"),
            pacDocRankers.isEmpty() ? tr("None installed") : pacDocRankers.join(", "),
            pacDocRankers.isEmpty() ? tr("Install reflector or rate-mirrors to re-rank automatically.") : QString());

        pacDocDiagLabel->setText("<table style='border-collapse:collapse;'>" + rows + "</table>");

        if (problems > 0) {
            pacDocStatusLabel->setText(tr("🔴 %n problem(s) found", "", problems)
                                       + (warnings ? tr(" · %n warning(s)", "", warnings) : QString()));
            pacDocStatusLabel->setStyleSheet("color:#c0392b; font-weight:bold;");
        } else if (warnings > 0) {
            pacDocStatusLabel->setText(tr("🟠 %n thing(s) worth fixing", "", warnings));
            pacDocStatusLabel->setStyleSheet("color:#e67e22; font-weight:bold;");
        } else {
            pacDocStatusLabel->setText(tr("🟢 pacman is healthy."));
            pacDocStatusLabel->setStyleSheet("color:#27ae60; font-weight:bold;");
        }

        // --- pacman.conf controls reflect the file as it is now ---
        pacDocParallelSpin->setValue(qBound(1, v.value("PARALLEL").toInt() ? v.value("PARALLEL").toInt() : 5, 20));
        pacDocColorCheck->setChecked(v.value("COLOR") == "1");
        pacDocCandyCheck->setChecked(v.value("CANDY") == "1");
        pacDocVerboseCheck->setChecked(v.value("VERBOSE") == "1");
        pacDocCheckSpaceCheck->setChecked(v.value("CHECKSPACE") == "1");
        pacDocTimeoutCheck->setChecked(v.value("NOTIMEOUT") == "1");

        pacDocPacnewList->clear();
        pacDocPacnewList->addItems(pacnewFiles);
        if (pacnewFiles.isEmpty())
            pacDocPacnewList->addItem(tr("— nothing pending —"));
    });

    // One pass, all read-only. No sudo: everything here is world-readable.
    proc->start("bash", QStringList() << "-c" << R"BASH(
ML=/etc/pacman.d/mirrorlist
[ -r /etc/pacman.d/cachyos-mirrorlist ] && ML=/etc/pacman.d/cachyos-mirrorlist
NOW=$(date +%s)

[ -e /var/lib/pacman/db.lck ] && echo "LOCK=1" || echo "LOCK=0"
pgrep -x pacman >/dev/null && echo "LOCKPROC=1" || echo "LOCKPROC=0"

echo "MIRRORS=$(grep -c '^[[:space:]]*Server[[:space:]]*=' "$ML" 2>/dev/null || echo 0)"
if [ -r "$ML" ]; then echo "MIRRORAGE=$(( (NOW - $(stat -c %Y "$ML")) / 86400 ))"; else echo "MIRRORAGE=-1"; fi

NEWEST=$(ls -t /var/lib/pacman/sync/*.db 2>/dev/null | head -1)
if [ -n "$NEWEST" ]; then echo "SYNCAGE=$(( (NOW - $(stat -c %Y "$NEWEST")) / 86400 ))"; else echo "SYNCAGE=-1"; fi

[ -s /etc/pacman.d/gnupg/trustdb.gpg ] && echo "GNUPG=1" || echo "GNUPG=0"
KR=$(pacman -Q archlinux-keyring 2>/dev/null | awk '{print $2}')
echo "KEYRINGVER=$KR"
KRDATE=$(pacman -Qi archlinux-keyring 2>/dev/null | awk -F': ' '/^Install Date/{print $2}')
if [ -n "$KRDATE" ]; then echo "KEYRINGAGE=$(( (NOW - $(date -d "$KRDATE" +%s 2>/dev/null || echo $NOW)) / 86400 ))"; else echo "KEYRINGAGE=0"; fi

echo "CACHEBYTES=$(du -sb /var/cache/pacman/pkg 2>/dev/null | cut -f1)"
echo "CACHECOUNT=$(ls -1 /var/cache/pacman/pkg/*.pkg.tar.* 2>/dev/null | wc -l)"
echo "VARFREE=$(df -B1 --output=avail /var 2>/dev/null | tail -1 | tr -d ' ')"

echo "ORPHANS=$(pacman -Qtdq 2>/dev/null | wc -l)"
echo "FOREIGN=$(pacman -Qmq 2>/dev/null | wc -l)"

CONF=/etc/pacman.conf
awk '/^[[:space:]]*ParallelDownloads/{gsub(/[^0-9]/,"",$0); print "PARALLEL=" $0}' "$CONF" 2>/dev/null | head -1
grep -qE '^[[:space:]]*Color[[:space:]]*$' "$CONF" && echo "COLOR=1" || echo "COLOR=0"
grep -qE '^[[:space:]]*ILoveCandy[[:space:]]*$' "$CONF" && echo "CANDY=1" || echo "CANDY=0"
grep -qE '^[[:space:]]*VerbosePkgLists[[:space:]]*$' "$CONF" && echo "VERBOSE=1" || echo "VERBOSE=0"
grep -qE '^[[:space:]]*CheckSpace[[:space:]]*$' "$CONF" && echo "CHECKSPACE=1" || echo "CHECKSPACE=0"
grep -qE '^[[:space:]]*DisableDownloadTimeout[[:space:]]*$' "$CONF" && echo "NOTIMEOUT=1" || echo "NOTIMEOUT=0"

R=""
for t in cachyos-rate-mirrors rate-mirrors reflector; do command -v "$t" >/dev/null 2>&1 && R="$R $t"; done
echo "RANKERS=$R"

find /etc /boot /usr/share/config -xdev \( -name '*.pacnew' -o -name '*.pacsave' \) 2>/dev/null \
  | head -40 | sed 's/^/PACNEWFILE=/'
)BASH");
}

// ============================================================
// Rank mirrors
// ============================================================
void MainWindow::pacmanRankMirrorsDialog() {
    if (pacDocRankers.isEmpty()) {
        // Diagnose may not have run yet — probe directly before giving up.
        QProcess p;
        p.start("bash", QStringList() << "-c" <<
                "for t in cachyos-rate-mirrors rate-mirrors reflector; do command -v $t >/dev/null && echo $t; done");
        p.waitForFinished(3000);
        pacDocRankers = QString::fromUtf8(p.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
    }
    if (pacDocRankers.isEmpty()) {
        if (QMessageBox::question(this, tr("No Ranking Tool"),
                tr("Ranking mirrors needs reflector or rate-mirrors, and neither is installed.\n\n"
                   "Install reflector now?"),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
        runScriptInTerminal("pacman -S --needed reflector\n", "install_reflector");
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Rank Mirrors"));
    QFormLayout *form = new QFormLayout(&dlg);

    QComboBox *toolCombo = new QComboBox(&dlg);
    toolCombo->addItems(pacDocRankers);
    form->addRow(tr("Tool:"), toolCombo);

    QLineEdit *countryEdit = new QLineEdit(&dlg);
    countryEdit->setPlaceholderText(tr("e.g. Germany,Netherlands — empty = worldwide"));
    form->addRow(tr("Countries:"), countryEdit);

    QSpinBox *countSpin = new QSpinBox(&dlg);
    countSpin->setRange(3, 50);
    countSpin->setValue(10);
    form->addRow(tr("Keep fastest:"), countSpin);

    QComboBox *protoCombo = new QComboBox(&dlg);
    protoCombo->addItems(QStringList() << "https" << "https,http");
    form->addRow(tr("Protocols:"), protoCombo);

    QLabel *note = new QLabel(tr("The current mirrorlist is backed up to ~/configbackups first."), &dlg);
    note->setStyleSheet("color:#888;");
    note->setWordWrap(true);
    form->addRow(note);

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    bb->button(QDialogButtonBox::Ok)->setText(tr("Rank Now"));
    form->addRow(bb);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    // reflector's country list is the only field a typo can silently ruin
    connect(toolCombo, &QComboBox::currentTextChanged, &dlg, [countryEdit](const QString &t) {
        countryEdit->setEnabled(t == "reflector");
        countryEdit->setPlaceholderText(t == "reflector"
            ? tr("e.g. Germany,Netherlands — empty = worldwide")
            : tr("(not used by this tool — it measures from your location)"));
    });
    countryEdit->setEnabled(toolCombo->currentText() == "reflector");

    if (dlg.exec() != QDialog::Accepted) return;

    const QString tool = toolCombo->currentText();
    const QString country = countryEdit->text().trimmed();
    if (!country.isEmpty() && !QRegularExpression("^[A-Za-z ,'-]+$").match(country).hasMatch()) {
        QMessageBox::warning(this, tr("Invalid Countries"),
                             tr("Use country names separated by commas, e.g. Germany,France."));
        return;
    }

    const QString mirrorFile = QFile::exists("/etc/pacman.d/cachyos-mirrorlist")
                               ? "/etc/pacman.d/cachyos-mirrorlist" : "/etc/pacman.d/mirrorlist";
    backupConfigFile(mirrorFile, "pacman mirrorlist");

    QString script = QString("cp -v %1 %1.bak.$(date +%F_%H-%M-%S)\n"
                             "echo 'Ranking mirrors — this contacts every candidate and takes a minute.'\n")
                     .arg(mirrorFile);
    if (tool == "reflector") {
        script += QString("reflector --save %1 --protocol %2 --latest %3 --sort rate%4\n")
                      .arg(mirrorFile, protoCombo->currentText()).arg(countSpin->value())
                      .arg(country.isEmpty() ? QString() : QString(" --country '%1'").arg(country));
    } else if (tool == "cachyos-rate-mirrors") {
        script += "cachyos-rate-mirrors\n";
    } else {
        script += QString("rate-mirrors --save=%1 --protocol=https arch\n").arg(mirrorFile);
    }
    script += QString("echo ''; echo 'New mirrorlist:'; grep -m5 '^Server' %1\n"
                      "pacman -Syy\n").arg(mirrorFile);

    runScriptInTerminal(repairScript(script), "rank_mirrors");
    scheduleStagedRecheck([this]() { if (pacmanDoctorVisible()) refreshPacmanDoctor(); });
}

// ============================================================
// Fix keyring
// ============================================================
void MainWindow::pacmanFixKeyringDialog() {
    QStringList opts;
    opts << tr("Refresh — update the keyring package and re-populate (try this first)")
         << tr("Rebuild — delete /etc/pacman.d/gnupg and start the trust database over")
         << tr("Refresh keys from a keyserver (slow, needs working network and DNS)");
    bool ok;
    QString choice = QInputDialog::getItem(this, tr("Fix Keyring"),
        tr("Symptoms: \"invalid or corrupted package (PGP signature)\",\n"
           "\"signature from ... is unknown trust\", \"key ... could not be looked up\".\n\n"
           "Pick a repair:"),
        opts, 0, false, &ok);
    if (!ok) return;
    int idx = opts.indexOf(choice);

    // CachyOS and friends ship their own keyring alongside the Arch one.
    QString extraKeyrings =
        "EXTRA=''\n"
        "for k in cachyos-keyring chaotic-keyring endeavouros-keyring manjaro-keyring; do\n"
        "  pacman -Qq \"$k\" >/dev/null 2>&1 && EXTRA=\"$EXTRA $k\"\n"
        "done\n";

    QString script;
    if (idx == 0) {
        script = extraKeyrings +
                 "echo 'Updating keyring packages...'\n"
                 "pacman -Sy --noconfirm archlinux-keyring $EXTRA\n"
                 "echo 'Populating trust database...'\n"
                 "pacman-key --populate archlinux\n"
                 "for k in $EXTRA; do pacman-key --populate \"${k%-keyring}\" || true; done\n"
                 "echo 'Done. Retry your install or upgrade.'\n";
    } else if (idx == 1) {
        if (QMessageBox::warning(this, tr("Rebuild Keyring"),
                tr("This deletes /etc/pacman.d/gnupg entirely and rebuilds it.\n\n"
                   "Any keys you locally signed or manually trusted are lost. The standard "
                   "distribution keys come back automatically.\n\nContinue?"),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
        script = extraKeyrings +
                 "cp -a /etc/pacman.d/gnupg /etc/pacman.d/gnupg.bak.$(date +%F_%H-%M-%S) 2>/dev/null || true\n"
                 "rm -rf /etc/pacman.d/gnupg\n"
                 "pacman-key --init\n"
                 "pacman-key --populate archlinux\n"
                 "for k in $EXTRA; do pacman-key --populate \"${k%-keyring}\" || true; done\n"
                 "pacman -Sy --noconfirm archlinux-keyring $EXTRA\n"
                 "echo 'Keyring rebuilt.'\n";
    } else {
        script = "echo 'Refreshing keys from the keyserver — this can take several minutes.'\n"
                 "pacman-key --refresh-keys\n";
    }

    runScriptInTerminal(repairScript(script), "fix_keyring");
    scheduleStagedRecheck([this]() { if (pacmanDoctorVisible()) refreshPacmanDoctor(); });
}

// ============================================================
// Downgrade
// ============================================================
void MainWindow::pacmanDowngradeDialog() {
    // Everything downgradable lives in the cache as an older .pkg.tar.*
    QProcess p;
    p.start("bash", QStringList() << "-c" << R"BASH(
for f in /var/cache/pacman/pkg/*.pkg.tar.*; do
  case "$f" in *.sig) continue;; esac
  [ -e "$f" ] || continue
  b=$(basename "$f")
  # name-version-release-arch.pkg.tar.zst -> strip the last three dash-fields
  stem=${b%.pkg.tar.*}
  arch=${stem##*-}; rest=${stem%-*}
  rel=${rest##*-};  rest=${rest%-*}
  ver=${rest##*-};  name=${rest%-*}
  echo "$name|$ver-$rel|$arch|$f"
done | sort
)BASH");
    p.waitForFinished(15000);
    const QStringList lines = QString::fromUtf8(p.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);

    if (lines.isEmpty()) {
        QMessageBox::information(this, tr("Empty Cache"),
            tr("There are no cached packages in /var/cache/pacman/pkg, so there is nothing to roll back to.\n\n"
               "You can still fetch old versions from the Arch Linux Archive with the 'downgrade' package."));
        return;
    }

    // Currently installed versions, so we can mark what "back" means
    QProcess q;
    q.start("bash", QStringList() << "-c" << "pacman -Q");
    q.waitForFinished(10000);
    QHash<QString, QString> installed;
    for (const QString &l : QString::fromUtf8(q.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts)) {
        const QStringList f = l.split(' ');
        if (f.size() >= 2) installed.insert(f[0], f[1]);
    }

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Downgrade a Package"));
    dlg.resize(720, 520);
    QVBoxLayout *lay = new QVBoxLayout(&dlg);

    QLabel *hint = new QLabel(tr("Pick an <b>older</b> version from your package cache. "
                                 "The currently installed version is marked ●."), &dlg);
    hint->setWordWrap(true);
    lay->addWidget(hint);

    QLineEdit *filter = new QLineEdit(&dlg);
    filter->setPlaceholderText(tr("Filter packages…"));
    lay->addWidget(filter);

    QTreeWidget *tree = new QTreeWidget(&dlg);
    tree->setColumnCount(3);
    tree->setHeaderLabels(QStringList() << tr("Package") << tr("Cached version") << tr("Installed now"));
    tree->setRootIsDecorated(false);
    tree->setAlternatingRowColors(true);
    tree->setSortingEnabled(true);
    for (const QString &line : lines) {
        const QStringList f = line.split('|');
        if (f.size() < 4) continue;
        const QString cur = installed.value(f[0]);
        const bool isCurrent = (cur == f[1]);
        QTreeWidgetItem *it = new QTreeWidgetItem(tree);
        it->setText(0, f[0]);
        it->setText(1, (isCurrent ? "● " : "   ") + f[1]);
        it->setText(2, cur.isEmpty() ? tr("(not installed)") : cur);
        it->setData(0, Qt::UserRole, f[3]);
        it->setData(1, Qt::UserRole, isCurrent);
        if (isCurrent) it->setForeground(1, QColor("#888888"));
    }
    tree->sortByColumn(0, Qt::AscendingOrder);
    tree->resizeColumnToContents(0);
    lay->addWidget(tree, 1);

    connect(filter, &QLineEdit::textChanged, &dlg, [tree](const QString &t) {
        for (int i = 0; i < tree->topLevelItemCount(); ++i) {
            QTreeWidgetItem *it = tree->topLevelItem(i);
            it->setHidden(!t.isEmpty() && !it->text(0).contains(t, Qt::CaseInsensitive));
        }
    });

    QCheckBox *holdCheck = new QCheckBox(tr("Also add it to IgnorePkg so the next -Syu does not undo this"), &dlg);
    holdCheck->setToolTip(tr("Remember to remove it from /etc/pacman.conf once the upstream bug is fixed."));
    lay->addWidget(holdCheck);

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    bb->button(QDialogButtonBox::Ok)->setText(tr("Downgrade"));
    lay->addWidget(bb);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;
    QTreeWidgetItem *sel = tree->currentItem();
    if (!sel) {
        QMessageBox::information(this, tr("No Selection"), tr("Select a cached version first."));
        return;
    }
    if (sel->data(1, Qt::UserRole).toBool()) {
        QMessageBox::information(this, tr("Already Installed"),
            tr("That is the version you are already running. Pick an older one."));
        return;
    }

    const QString pkg = sel->text(0);
    const QString file = sel->data(0, Qt::UserRole).toString();
    if (QMessageBox::question(this, tr("Downgrade %1").arg(pkg),
            tr("Install this cached version?\n\n%1\n\n"
               "Downgrading a library below what other packages were built against can break them. "
               "If pacman warns about dependencies, read the warning before answering yes.")
            .arg(QFileInfo(file).fileName()),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;

    QString script = "pacman -U " + shQuote(file) + "\n";
    if (holdCheck->isChecked()) {
        script += QString(
            "cp -v /etc/pacman.conf /etc/pacman.conf.bak.$(date +%F_%H-%M-%S)\n"
            "if grep -qE '^[[:space:]]*IgnorePkg' /etc/pacman.conf; then\n"
            "  sed -i -E '0,/^[[:space:]]*IgnorePkg[[:space:]]*=/{s//IgnorePkg = %1 /}' /etc/pacman.conf\n"
            "else\n"
            "  sed -i '0,/^\\[options\\]/{s//[options]\\nIgnorePkg = %1/}' /etc/pacman.conf\n"
            "fi\n"
            "echo ''; echo 'IgnorePkg line is now:'; grep -E '^[[:space:]]*IgnorePkg' /etc/pacman.conf\n"
            "echo 'Remember to remove %1 from IgnorePkg once upstream fixes the bug.'\n").arg(pkg);
    }
    runScriptInTerminal(repairScript(script), "downgrade_" + pkg);
}

// ============================================================
// pacman.conf tuning
// ============================================================
void MainWindow::pacmanApplyConfTuning() {
    struct Flag { QCheckBox *box; const char *key; };
    const QList<Flag> flags = {
        {pacDocColorCheck,      "Color"},
        {pacDocCandyCheck,      "ILoveCandy"},
        {pacDocVerboseCheck,    "VerbosePkgLists"},
        {pacDocCheckSpaceCheck, "CheckSpace"},
        {pacDocTimeoutCheck,    "DisableDownloadTimeout"},
    };

    QString summary = tr("ParallelDownloads = %1").arg(pacDocParallelSpin->value());
    for (const Flag &f : flags)
        summary += "\n" + QString(f.key) + " — " + (f.box->isChecked() ? tr("on") : tr("off"));

    if (QMessageBox::question(this, tr("Apply pacman.conf Settings"),
            tr("Write these to the [options] section of /etc/pacman.conf?\n\n%1\n\n"
               "A timestamped backup is made first. Nothing else in the file is touched.").arg(summary),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;

    // A section-aware awk rewrite rather than sed one-liners. Real pacman.conf
    // files in the wild contain the same key twice inside [options] (every tool
    // that ever "added ILoveCandy for you" appended a second block). A first-match
    // sed edits the copy pacman doesn't use; this keeps the first occurrence,
    // drops the later duplicates, and never looks outside [options].
    QString script =
        "CONF=/etc/pacman.conf\n"
        "cp -v \"$CONF\" \"$CONF.bak.$(date +%F_%H-%M-%S)\"\n"
        "\n";

    script += QString("PD=%1\n").arg(pacDocParallelSpin->value());
    for (const Flag &f : flags)
        script += QString("%1=%2\n").arg(QString(f.key).toUpper(), f.box->isChecked() ? "on" : "off");

    script +=
        "\n"
        "awk -v pd=\"$PD\" -v f_color=\"$COLOR\" -v f_candy=\"$ILOVECANDY\" \\\n"
        "    -v f_verbose=\"$VERBOSEPKGLISTS\" -v f_space=\"$CHECKSPACE\" \\\n"
        "    -v f_notimeout=\"$DISABLEDOWNLOADTIMEOUT\" '\n"
        "function want(k) {\n"
        "  if (k == \"Color\")                  return f_color\n"
        "  if (k == \"ILoveCandy\")             return f_candy\n"
        "  if (k == \"VerbosePkgLists\")        return f_verbose\n"
        "  if (k == \"CheckSpace\")             return f_space\n"
        "  if (k == \"DisableDownloadTimeout\") return f_notimeout\n"
        "  return \"\"\n"
        "}\n"
        "function emit(   i, line, key, m) {\n"
        "  for (i = 1; i <= bn; i++) {\n"
        "    line = buf[i]\n"
        "    if (line ~ /^[[:space:]]*#?[[:space:]]*ParallelDownloads[[:space:]]*=/) {\n"
        "      if (!seen_pd) { print \"ParallelDownloads = \" pd; seen_pd = 1 }\n"
        "      continue\n"
        "    }\n"
        "    key = line\n"
        "    sub(/^[[:space:]]*#?[[:space:]]*/, \"\", key)\n"
        "    sub(/[[:space:]]*$/, \"\", key)\n"
        "    if (want(key) != \"\") {\n"
        "      if (!seen[key]) { seen[key] = 1; print (want(key) == \"on\" ? key : \"#\" key) }\n"
        "      continue\n"
        "    }\n"
        "    print line\n"
        "  }\n"
        "  if (!seen_pd) print \"ParallelDownloads = \" pd\n"
        "  split(\"Color ILoveCandy VerbosePkgLists CheckSpace DisableDownloadTimeout\", all, \" \")\n"
        "  for (i = 1; i <= 5; i++)\n"
        "    if (!seen[all[i]] && want(all[i]) == \"on\") print all[i]\n"
        "  bn = 0\n"
        "}\n"
        "/^[[:space:]]*\\[/ {\n"
        "  if (insec) { emit(); insec = 0 }\n"
        "  if ($0 ~ /^[[:space:]]*\\[options\\][[:space:]]*$/) insec = 1\n"
        "  print; next\n"
        "}\n"
        "{ if (insec) buf[++bn] = $0; else print }\n"
        "END { if (insec) emit() }\n"
        "' \"$CONF\" > \"$CONF.new\"\n"
        "\n"
        "if [ ! -s \"$CONF.new\" ]; then\n"
        "  echo 'Rewrite produced an empty file — keeping the original untouched.'\n"
        "  rm -f \"$CONF.new\"\n"
        "else\n"
        "  cat \"$CONF.new\" > \"$CONF\"\n"
        "  rm -f \"$CONF.new\"\n"
        "  echo ''; echo '--- [options] section is now: ---'\n"
        "  awk '/^\\[options\\]/{p=1} /^\\[/&&!/^\\[options\\]/{p=0} p' \"$CONF\"\n"
        "  echo ''; echo 'Verifying pacman can still parse it...'\n"
        "  pacman-conf >/dev/null && echo 'OK.'\n"
        "fi\n";

    runScriptInTerminal(repairScript(script), "tune_pacman_conf");
    scheduleStagedRecheck([this]() { if (pacmanDoctorVisible()) refreshPacmanDoctor(); });
}
