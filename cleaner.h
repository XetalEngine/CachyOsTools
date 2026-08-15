
// Cleaner tab: finds disposable files (caches, trash, rotated logs, build
// leftovers) and removes them. Two panels, mirroring the ISO Creator:
//   left  - a curated category list, each with a safety tier and its real cost
//   right - a du-based size tree for hunting big folders by hand
//
// Safety model: a category only ever runs the command written next to it, and
// anything checked in the size tree must pass isCleanablePath() - a whitelist
// of cache roots plus a deny list. Nothing else in the filesystem is
// reachable from this tab, by construction.

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QSplitter>
#include <QCheckBox>
#include <QGroupBox>
#include <QToolButton>
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QCoreApplication>
#include <algorithm>

// Shell-quotes a path for single-quoted context.
static QString cleanQuote(const QString &s) {
    QString q = s;
    q.replace("'", "'\\''");
    return "'" + q + "'";
}

// The catalogue. Paths are bash patterns evaluated with nullglob set and $H
// bound to the user's home, so globs work while a home with spaces still does.
QList<CleanTarget> MainWindow::buildCleanTargets() const {
    QList<CleanTarget> t;

    auto add = [&t](const QString &id, const QString &label, const QString &detail,
                    int tier, bool system, const QStringList &paths) {
        CleanTarget c;
        c.id = id; c.label = label; c.detail = detail;
        c.tier = tier; c.system = system; c.paths = paths;
        t.append(c);
    };

    // ---- User locations (tier 0 = regenerates for free) -------------------
    add("trash", tr("🗑️ Trash"),
        tr("Files you already deleted, on every mounted drive."),
        0, false, {"\"$H\"/.local/share/Trash", "/run/media/\"$U\"/*/.Trash-\"$I\"",
                   "/media/\"$U\"/*/.Trash-\"$I\""});
    t.last().cleanCmd =
        "for d in \"$H\"/.local/share/Trash /run/media/\"$U\"/*/.Trash-\"$I\" /media/\"$U\"/*/.Trash-\"$I\"; do\n"
        "  [ -d \"$d\" ] || continue\n"
        "  echo \"  emptying $d\"\n"
        "  $RM \"$d/files\" \"$d/info\" \"$d/expunged\"\n"
        "done\n";

    add("thumbnails", tr("🖼️ Thumbnail cache"),
        tr("Preview images for your file manager. Rebuilt as you browse."),
        0, false, {"\"$H\"/.cache/thumbnails", "\"$H\"/.thumbnails"});

    add("aur", tr("📦 AUR build leftovers"),
        tr("Cloned sources and build dirs from yay/paru. Re-cloned on the next AUR install."),
        0, false, {"\"$H\"/.cache/yay", "\"$H\"/.cache/paru/clone", "\"$H\"/.cache/pikaur",
                   "\"$H\"/.cache/trizen", "\"$H\"/.cache/aurutils"});

    add("fontcache", tr("🔤 Font, icon & desktop caches"),
        tr("Font and menu indexes. Regenerated within seconds of being needed."),
        0, false, {"\"$H\"/.cache/fontconfig", "\"$H\"/.cache/icon-cache.kcache",
                   "\"$H\"/.cache/ksycoca*", "\"$H\"/.cache/plasma*", "\"$H\"/.cache/mesa",
                   "\"$H\"/.cache/qtshadercache*", "\"$H\"/.cache/kioexec"});

    add("appcache", tr("💬 App caches (Discord, Spotify, Electron…)"),
        tr("Only the Cache/GPUCache folders inside app configs — logins and settings stay put."),
        0, false, {"\"$H\"/.config/*/Cache", "\"$H\"/.config/*/Code\\ Cache",
                   "\"$H\"/.config/*/GPUCache", "\"$H\"/.config/*/DawnCache",
                   "\"$H\"/.config/*/ShaderCache", "\"$H\"/.config/*/component_crx_cache",
                   "\"$H\"/.cache/spotify", "\"$H\"/.var/app/*/cache"});

    // ---- User locations (tier 1 = you pay something back) -----------------
    add("browser", tr("🌐 Browser caches"),
        tr("You stay logged in — only cached pages go. First loads are slower for a day."),
        1, false, {"\"$H\"/.cache/mozilla", "\"$H\"/.cache/chromium", "\"$H\"/.cache/google-chrome",
                   "\"$H\"/.cache/BraveSoftware", "\"$H\"/.cache/vivaldi", "\"$H\"/.cache/opera",
                   "\"$H\"/.cache/microsoft-edge", "\"$H\"/.cache/librewolf", "\"$H\"/.cache/floorp",
                   "\"$H\"/.cache/zen", "\"$H\"/.cache/qutebrowser"});

    add("shader", tr("🎮 GPU shader caches"),
        tr("Games recompile shaders on next launch — expect stutter for one session, then it's back."),
        1, false, {"\"$H\"/.cache/mesa_shader_cache", "\"$H\"/.cache/mesa_shader_cache_db",
                   "\"$H\"/.cache/radv_builtin_shaders*", "\"$H\"/.cache/nvidia", "\"$H\"/.nv/GLCache",
                   "\"$H\"/.cache/AMD", "\"$H\"/.steam/steam/steamapps/shadercache",
                   "\"$H\"/.local/share/Steam/steamapps/shadercache", "\"$H\"/.cache/wine"});

    add("dev", tr("🛠️ Developer caches"),
        tr("npm/pip/cargo/go/gradle downloads. Your next build re-fetches them."),
        1, false, {"\"$H\"/.npm/_cacache", "\"$H\"/.cache/yarn", "\"$H\"/.cache/pnpm",
                   "\"$H\"/.cache/pip", "\"$H\"/.cache/uv", "\"$H\"/.cache/go-build",
                   "\"$H\"/.cargo/registry/cache", "\"$H\"/.cargo/registry/src",
                   "\"$H\"/.gradle/caches", "\"$H\"/.cache/JetBrains", "\"$H\"/.cache/composer",
                   "\"$H\"/.cache/ccache"});

    add("baloo", tr("🔍 Baloo file index (KDE)"),
        tr("KDE re-indexes your files afterwards — heavy disk use for a while, then search works again."),
        1, false, {"\"$H\"/.local/share/baloo"});
    // balooctl's "Stopping the File Indexer" spins dots forever when the index is
    // busy and then reports "failed to stop!" anyway, so every call is bounded by
    // `timeout` and we make sure the process is really gone before deleting under
    // it. balooctl talks to the *calling user's* KDE session, so it never gets
    // sudo — only the removal does.
    t.last().cleanCmd =
        "B=\"\"\n"
        "command -v balooctl6 >/dev/null 2>&1 && B=balooctl6\n"
        "[ -z \"$B\" ] && command -v balooctl >/dev/null 2>&1 && B=balooctl\n"
        "if [ -n \"$B\" ]; then\n"
        "  echo '  suspending the indexer'\n"
        "  timeout 15 \"$B\" suspend >/dev/null 2>&1 || true\n"
        "  echo '  disabling the indexer (bounded to 30s)'\n"
        "  timeout 30 \"$B\" disable  >/dev/null 2>&1 || true\n"
        "fi\n"
        "if pgrep -x baloo_file >/dev/null 2>&1; then\n"
        "  echo '  indexer still running — stopping it'\n"
        "  pkill -x baloo_file 2>/dev/null || true\n"
        "  sleep 1\n"
        "  pkill -9 -x baloo_file 2>/dev/null || true\n"
        "fi\n"
        "echo \"  removing $H/.local/share/baloo\"\n"
        "$RM \"$H\"/.local/share/baloo\n"
        "echo '  re-enable later with:  balooctl6 enable'\n";

    // Commands filled in at the bottom of this function, once every other
    // category has registered the ~/.cache paths it claims.
    add("cacherest", tr("🧹 Everything else in ~/.cache"),
        tr("XDG says anything here is disposable. A few sloppy apps still keep session data in it."),
        1, false, {});

    // ---- System locations (need root) -------------------------------------
    add("pacman", tr("📦 Pacman package cache"),
        tr("Keeps the 2 newest versions of every package, so downgrading still works."),
        0, true, {"/var/cache/pacman/pkg"});
    t.last().needsBinary = "paccache";
    t.last().sizeCmd =
        "{ paccache -dvk2 2>/dev/null; paccache -dvuk0 2>/dev/null; } "
        "| grep '^/var/cache' | sort -u | tr '\\n' '\\0' "
        "| du -scb --files0-from=- 2>/dev/null | tail -n1 | cut -f1";
    t.last().cleanCmd = "paccache -rk2\npaccache -ruk0\n";

    add("orphans", tr("🧩 Orphan packages"),
        tr("Packages pulled in as dependencies that nothing needs any more."),
        0, true, {});
    t.last().sizeCmd =
        "if command -v expac >/dev/null 2>&1; then "
        "pacman -Qtdq 2>/dev/null | xargs -r expac -Q '%m' 2>/dev/null | awk '{s+=$1} END{print s+0}'; "
        "else echo 0; fi";
    t.last().cleanCmd =
        "orph=$(pacman -Qtdq 2>/dev/null)\n"
        "if [ -n \"$orph\" ]; then echo \"$orph\" | pacman -Rns --noconfirm -; else echo '  no orphans'; fi\n";

    add("journal", tr("📓 Systemd journal history"),
        tr("Trimmed to the most recent 200 MB — you keep current logs, lose ancient ones."),
        0, true, {"/var/log/journal"});
    t.last().sizeCmd =
        "s=$(du -sxb /var/log/journal 2>/dev/null | cut -f1); s=${s:-0}; "
        "if [ \"$s\" -gt 209715200 ]; then echo $((s-209715200)); else echo 0; fi";
    t.last().cleanCmd = "journalctl --vacuum-size=200M\n";

    add("coredumps", tr("💥 Core dumps"),
        tr("Crash memory images. Only useful if you are actively debugging a crash."),
        0, true, {"/var/lib/systemd/coredump/*"});

    add("oldlogs", tr("🧾 Rotated log files"),
        tr("Already-archived .old/.gz/.1 logs under /var/log. Live logs are untouched."),
        0, true, {"/var/log/*.old", "/var/log/*.gz", "/var/log/*.[0-9]",
                  "/var/log/*/*.gz", "/var/log/*/*.[0-9]", "/var/log/old"});

    add("rootcache", tr("👤 Root's cache"),
        tr("~/.cache for the root account — usually AUR builds you ran with sudo."),
        0, true, {"/root/.cache/*"});

    add("flatpak", tr("📦 Flatpak caches & unused runtimes"),
        tr("Runtimes no installed app requires any more, plus download caches."),
        0, true, {"\"$H\"/.cache/flatpak", "/var/tmp/flatpak-cache-*"});
    t.last().needsBinary = "flatpak";
    t.last().cleanCmd =
        "rm -rf -- \"$H\"/.cache/flatpak /var/tmp/flatpak-cache-*\n"
        "flatpak uninstall --unused -y || true\n";

    add("vartmp", tr("🗂️ /var/tmp older than 10 days"),
        tr("Stale scratch files. A long-running build older than 10 days would also go."),
        1, true, {});
    t.last().sizeCmd =
        "find /var/tmp -mindepth 1 -maxdepth 1 -mtime +10 -print0 2>/dev/null "
        "| du -scb --files0-from=- 2>/dev/null | tail -n1 | cut -f1";
    t.last().cleanCmd =
        "find /var/tmp -mindepth 1 -maxdepth 1 -mtime +10 -exec rm -rf -- {} + 2>/dev/null || true\n";

    // "Everything ELSE in ~/.cache" means exactly that: every ~/.cache path any
    // other category claims is skipped, so the catch-all neither double-counts
    // in the totals nor swallows a sibling the user deliberately left unchecked.
    QStringList covered;
    for (const CleanTarget &c : t) {
        if (c.id == "cacherest") continue;
        for (const QString &p : c.paths) {
            if (p.startsWith("\"$H\"/.cache")) covered << p;
        }
    }
    // One level deep, dotted entries included; an entry is kept only when no
    // covered pattern is it or lives beneath it.
    const QString selectRest =
        "COV=( " + covered.join(' ') + " )\n"
        "p=()\n"
        "for f in \"$H\"/.cache/* \"$H\"/.cache/.[!.]*; do\n"
        "  [ -e \"$f\" ] || continue\n"
        "  keep=1\n"
        "  for c in \"${COV[@]}\"; do case \"$c\" in \"$f\"|\"$f\"/*) keep=0; break;; esac; done\n"
        "  [ $keep -eq 1 ] && p+=(\"$f\")\n"
        "done\n";
    for (CleanTarget &c : t) {
        if (c.id != "cacherest") continue;
        c.sizeCmd = selectRest +
            "if [ ${#p[@]} -eq 0 ]; then echo 0; "
            "else du -scxb \"${p[@]}\" 2>/dev/null | tail -n1 | cut -f1; fi\n";
        c.cleanCmd = selectRest +
            "for f in \"${p[@]}\"; do echo \"  removing $f\"; $RM \"$f\"; done\n";
    }

    return t;
}

// ---------------------------------------------------------------------------
// Safety guard for the size tree. A path is deletable only if it sits STRICTLY
// below one of these roots (never the root itself) and is not explicitly denied.
// Categories above carry their own commands and do not go through here.
// ---------------------------------------------------------------------------
bool MainWindow::isCleanablePath(const QString &path) const {
    const QString home = QDir::homePath();
    static const QStringList denied = {
        "/var/log/journal", "/var/cache/pacman/pkg", "/var/cache/pacman"
    };
    for (const QString &d : denied) {
        if (path == d) return false;
    }

    const QStringList roots = {
        home + "/.cache", home + "/.thumbnails", home + "/.local/share/Trash",
        home + "/.local/share/baloo", home + "/.var/app", home + "/.npm/_cacache",
        home + "/.cargo/registry", home + "/.gradle/caches", home + "/.nv",
        home + "/.steam", home + "/.local/share/Steam",
        "/var/cache", "/var/tmp", "/var/log", "/var/lib/systemd/coredump", "/root/.cache"
    };
    for (const QString &r : roots) {
        if (path.startsWith(r + "/")) return true;   // strictly below, never equal
    }
    return false;
}

// ---------------------------------------------------------------------------
// Tab construction
// ---------------------------------------------------------------------------
void MainWindow::setupCleanerTab() {
    cleanerTab = new QWidget();
    cleanerTab->setObjectName("cleanerTab");
    QVBoxLayout *root = new QVBoxLayout(cleanerTab);

    // --- top bar ---------------------------------------------------------
    QHBoxLayout *bar = new QHBoxLayout();
    QPushButton *scanBtn = new QPushButton(tr("🔍 Analyze"), cleanerTab);
    scanBtn->setToolTip(tr("Measure every category. Nothing is deleted."));
    cleanSystemCheck = new QCheckBox(tr("Include system locations (sudo)"), cleanerTab);
    cleanSystemCheck->setToolTip(tr("Pacman cache, journal, core dumps, rotated logs."));
    cleanStatusLabel = new QLabel(tr("Press Analyze to see what can be freed."), cleanerTab);
    cleanStatusLabel->setStyleSheet("color:#888;");
    bar->addWidget(scanBtn);
    bar->addWidget(cleanSystemCheck);
    bar->addWidget(cleanStatusLabel);
    bar->addStretch();
    root->addLayout(bar);

    // --- two panels ------------------------------------------------------
    QSplitter *split = new QSplitter(Qt::Horizontal, cleanerTab);

    QWidget *leftBox = new QWidget(split);
    QVBoxLayout *leftLay = new QVBoxLayout(leftBox);
    leftLay->setContentsMargins(0, 0, 0, 0);
    QLabel *leftTitle = new QLabel(tr("<b>What can go</b> — checked items are removed"), leftBox);
    leftLay->addWidget(leftTitle);
    cleanCatTree = new QTreeWidget(leftBox);
    cleanCatTree->setColumnCount(2);
    cleanCatTree->setHeaderLabels(QStringList() << tr("Item") << tr("Size"));
    cleanCatTree->header()->setStretchLastSection(false);
    cleanCatTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    cleanCatTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    cleanCatTree->setRootIsDecorated(true);
    cleanCatTree->setAlternatingRowColors(true);
    leftLay->addWidget(cleanCatTree);

    QLabel *neverLabel = new QLabel(
        tr("🔴 <b>Never touched:</b> ~/.config itself, browser profiles, ~/.mozilla, "
           ".ssh, .gnupg, /var/lib, Documents — the Cleaner cannot reach them."), leftBox);
    leftLay->addWidget(neverLabel);   // styled and sized with treeHint, below

    QWidget *rightBox = new QWidget(split);
    QVBoxLayout *rightLay = new QVBoxLayout(rightBox);
    rightLay->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *folderBar = new QHBoxLayout();
    QLabel *rightTitle = new QLabel(tr("<b>Size tree</b>"), rightBox);
    cleanFolderEdit = new QLineEdit(QDir::homePath() + "/.cache", rightBox);
    QPushButton *browseBtn = new QPushButton(tr("📁"), rightBox);
    browseBtn->setFixedWidth(34);
    browseBtn->setToolTip(tr("Pick a folder to measure"));
    QPushButton *treeScanBtn = new QPushButton(tr("Scan folder"), rightBox);
    folderBar->addWidget(rightTitle);
    folderBar->addWidget(cleanFolderEdit, 1);
    folderBar->addWidget(browseBtn);
    folderBar->addWidget(treeScanBtn);
    // The folder bar lives in its own widget so its height can be pinned to
    // exactly the same value as the left title: both trees then start on the
    // same line whatever the theme does to the line edit and buttons.
    QWidget *rightHeader = new QWidget(rightBox);
    folderBar->setContentsMargins(0, 0, 0, 0);   // a layout on a widget gets margins otherwise
    rightHeader->setLayout(folderBar);
    rightLay->addWidget(rightHeader);

    const int headerRowHeight = qMax(folderBar->sizeHint().height(),
                                     leftTitle->sizeHint().height());
    leftTitle->setFixedHeight(headerRowHeight);
    rightHeader->setFixedHeight(headerRowHeight);
    leftTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    cleanSizeTree = new QTreeWidget(rightBox);
    cleanSizeTree->setColumnCount(2);
    cleanSizeTree->setHeaderLabels(QStringList() << tr("Path") << tr("Size"));
    cleanSizeTree->header()->setStretchLastSection(false);
    cleanSizeTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    cleanSizeTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    cleanSizeTree->setTextElideMode(Qt::ElideMiddle);
    rightLay->addWidget(cleanSizeTree);

    QLabel *treeHint = new QLabel(
        tr("Any folder can be measured. Only items inside cache roots can be checked "
           "for deletion — everything else is shown for its size only."), rightBox);
    rightLay->addWidget(treeHint);

    // ...and both trees must end on the same line: identical two-line footers.
    // The size comes from a real QFont rather than a stylesheet so the metrics
    // used for the height are the ones actually rendered.
    for (QLabel *hint : {neverLabel, treeHint}) {
        QFont hf = hint->font();
        if (hf.pointSizeF() > 0) hf.setPointSizeF(hf.pointSizeF() * 0.85);
        else if (hf.pixelSize() > 0) hf.setPixelSize(qMax(9, int(hf.pixelSize() * 0.85)));
        hint->setFont(hf);
        hint->setStyleSheet("color:#888;");
        hint->setWordWrap(true);
        hint->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        hint->setFixedHeight(QFontMetrics(hf).height() * 2 + 4);
    }

    split->addWidget(leftBox);
    split->addWidget(rightBox);
    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 1);
    root->addWidget(split, 1);

    // --- bottom bar ------------------------------------------------------
    QHBoxLayout *bottom = new QHBoxLayout();
    cleanSummaryLabel = new QLabel(tr("Nothing selected."), cleanerTab);
    cleanRunButton = new QPushButton(tr("🗑️ Clean Selected"), cleanerTab);
    cleanRunButton->setEnabled(false);
    bottom->addWidget(cleanSummaryLabel);
    bottom->addStretch();
    bottom->addWidget(cleanRunButton);
    root->addLayout(bottom);

    cleanTargets = buildCleanTargets();
    rebuildCleanerCategoryTree();

    connect(scanBtn, &QPushButton::clicked, this, &MainWindow::startCleanerScan);
    connect(treeScanBtn, &QPushButton::clicked, this, [this]() {
        startCleanerTreeScan(cleanFolderEdit->text().trimmed());
    });
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Folder to measure"),
                                                        cleanFolderEdit->text());
        if (!dir.isEmpty()) {
            cleanFolderEdit->setText(dir);
            startCleanerTreeScan(dir);
        }
    });
    connect(cleanCatTree, &QTreeWidget::itemChanged, this,
            [this](QTreeWidgetItem *item, int column) {
                if (column != 0 || !item) return;
                QString id = item->data(0, Qt::UserRole).toString();
                if (id.isEmpty()) return;
                if (item->checkState(0) == Qt::Checked) cleanSelectedIds.insert(id);
                else cleanSelectedIds.remove(id);
                updateCleanerSummary();
            });
    connect(cleanSizeTree, &QTreeWidget::itemChanged, this,
            [this](QTreeWidgetItem *item, int column) {
                if (column != 0 || !item) return;
                QString path = item->data(0, Qt::UserRole).toString();
                if (path.isEmpty()) return;
                if (item->checkState(0) == Qt::Checked) cleanSelectedPaths.insert(path);
                else cleanSelectedPaths.remove(path);
                updateCleanerSummary();
            });
    connect(cleanRunButton, &QPushButton::clicked, this, &MainWindow::runCleanerClean);

    ui->tabWidget->addTab(cleanerTab, tr("Cleaner"));
}

// Groups the catalogue under its two safety tiers. Sizes come from the last
// scan; before the first one every row simply shows a dash.
void MainWindow::rebuildCleanerCategoryTree() {
    cleanCatTree->blockSignals(true);
    cleanCatTree->clear();

    QTreeWidgetItem *safeGroup = new QTreeWidgetItem(cleanCatTree);
    safeGroup->setText(0, tr("🟢 Safe — regenerates on its own"));
    safeGroup->setFlags(safeGroup->flags() & ~Qt::ItemIsSelectable);
    QFont bold = safeGroup->font(0);
    bold.setBold(true);
    safeGroup->setFont(0, bold);

    QTreeWidgetItem *costGroup = new QTreeWidgetItem(cleanCatTree);
    costGroup->setText(0, tr("🟡 Free to delete, but it costs you something"));
    costGroup->setFlags(costGroup->flags() & ~Qt::ItemIsSelectable);
    costGroup->setFont(0, bold);

    qint64 safeTotal = 0, costTotal = 0;
    for (const CleanTarget &c : cleanTargets) {
        if (!c.available) continue;
        QTreeWidgetItem *item = new QTreeWidgetItem(c.tier == 0 ? safeGroup : costGroup);
        item->setText(0, c.label + (c.system ? tr("  (system)") : QString()));
        item->setText(1, cleanScanDone ? formatSize(c.bytes) : QString("—"));
        item->setData(0, Qt::UserRole, c.id);
        item->setToolTip(0, c.detail);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(0, cleanSelectedIds.contains(c.id) ? Qt::Checked : Qt::Unchecked);

        QTreeWidgetItem *note = new QTreeWidgetItem(item);
        note->setText(0, c.detail);
        note->setFlags(Qt::ItemIsEnabled);
        note->setForeground(0, QBrush(QColor("#888888")));

        if (c.tier == 0) safeTotal += c.bytes; else costTotal += c.bytes;
    }

    safeGroup->setText(1, cleanScanDone ? formatSize(safeTotal) : QString());
    costGroup->setText(1, cleanScanDone ? formatSize(costTotal) : QString());
    cleanCatTree->expandItem(safeGroup);
    cleanCatTree->expandItem(costGroup);
    cleanCatTree->blockSignals(false);
    updateCleanerSummary();
}

// ---------------------------------------------------------------------------
// Measuring
// ---------------------------------------------------------------------------
// One bash run prints "id<TAB>bytes" per category. System categories are only
// included (and the whole script only elevated) when the user asked for them.
void MainWindow::startCleanerScan() {
    bool wantSystem = cleanSystemCheck->isChecked();
    if (wantSystem && !authenticateSudo()) {
        cleanSystemCheck->setChecked(false);
        wantSystem = false;
    }

    cleanStatusLabel->setText(tr("⏳ Measuring… the first scan of a big home can take a minute."));
    cleanStatusLabel->setStyleSheet("color:#888;");

    QString script =
        "shopt -s nullglob\n"
        "H=" + cleanQuote(QDir::homePath()) + "\n"
        "U=" + cleanQuote(qgetenv("USER").isEmpty() ? QDir::home().dirName()
                                                    : QString::fromLocal8Bit(qgetenv("USER"))) + "\n"
        "I=" + QString::number(getuid()) + "\n";

    for (const CleanTarget &c : cleanTargets) {
        if (c.system && !wantSystem) continue;
        if (!c.needsBinary.isEmpty()) {
            script += QString("command -v %1 >/dev/null 2>&1 || { printf '%2\\tskip\\n'; }\n")
                          .arg(c.needsBinary, c.id);
            script += QString("command -v %1 >/dev/null 2>&1 && {\n").arg(c.needsBinary);
        }
        if (!c.sizeCmd.isEmpty()) {
            script += QString("v=$(%1); printf '%2\\t%3\\n' \"${v:-0}\"\n")
                          .arg(c.sizeCmd, c.id, "%s");
        } else {
            script += "p=()\n";
            script += QString("for f in %1; do [ -e \"$f\" ] && p+=(\"$f\"); done\n")
                          .arg(c.paths.join(' '));
            script += QString("if [ ${#p[@]} -eq 0 ]; then printf '%1\\t0\\n'; "
                              "else v=$(du -scxb \"${p[@]}\" 2>/dev/null | tail -n1 | cut -f1); "
                              "printf '%1\\t%2\\n' \"${v:-0}\"; fi\n").arg(c.id, "%s");
        }
        if (!c.needsBinary.isEmpty()) script += "}\n";
    }

    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, proc](int, QProcess::ExitStatus) {
                QHash<QString, QString> raw;
                const QStringList lines =
                    QString::fromUtf8(proc->readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
                for (const QString &line : lines) {
                    int tab = line.indexOf('\t');
                    if (tab <= 0) continue;
                    raw.insert(line.left(tab), line.mid(tab + 1).trimmed());
                }

                qint64 total = 0;
                for (CleanTarget &c : cleanTargets) {
                    if (!raw.contains(c.id)) {          // system row while sudo was off
                        c.available = !c.system || cleanSystemCheck->isChecked();
                        c.bytes = 0;
                        continue;
                    }
                    const QString v = raw.value(c.id);
                    if (v == "skip") {                   // required binary missing
                        c.available = false;
                        c.bytes = 0;
                        cleanSelectedIds.remove(c.id);
                        continue;
                    }
                    c.available = true;
                    c.bytes = v.toLongLong();
                    total += c.bytes;
                }

                cleanScanDone = true;
                rebuildCleanerCategoryTree();
                cleanStatusLabel->setText(tr("Found %1 of disposable files.").arg(formatSize(total)));
                cleanStatusLabel->setStyleSheet("color:#27ae60; font-weight:bold;");
                proc->deleteLater();
            });

    QStringList args;
    if (wantSystem) args << "-n" << "bash" << "-c" << script;
    else args << "-c" << script;
    proc->start(wantSystem ? "sudo" : "bash", args);
}

// du/find scan of one folder, same shape as the ISO Creator's big-folder tree.
void MainWindow::startCleanerTreeScan(const QString &rootPath) {
    QString root = rootPath;
    if (root.endsWith('/') && root.size() > 1) root.chop(1);
    if (root.isEmpty() || !QFileInfo::exists(root)) {
        QMessageBox::information(this, tr("No Such Folder"),
                                 tr("That folder does not exist:\n%1").arg(rootPath));
        return;
    }

    bool elevate = !root.startsWith(QDir::homePath());
    if (elevate && !authenticateSudo()) return;

    cleanTreeRoot = root;
    cleanStatusLabel->setText(tr("⏳ Measuring %1 …").arg(root));

    QString script = QString(
        "du -x -B1 --max-depth=4 %1 2>/dev/null; "
        "echo '===FILES==='; "
        "find %1 -xdev -type f -size +50M -printf '%s\\t%p\\n' 2>/dev/null"
    ).arg(cleanQuote(root));   // %s/%p belong to find, only %1 is a placeholder

    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, proc, root](int, QProcess::ExitStatus) {
                cleanTreeDirSizes.clear();
                cleanTreeBigFiles.clear();
                const QStringList lines =
                    QString::fromUtf8(proc->readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
                bool inFiles = false;
                for (const QString &line : lines) {
                    if (line == "===FILES===") { inFiles = true; continue; }
                    int tab = line.indexOf('\t');
                    if (tab <= 0) continue;
                    qint64 bytes = line.left(tab).toLongLong();
                    QString path = line.mid(tab + 1);
                    if (path == root) continue;
                    if (inFiles) cleanTreeBigFiles.append(qMakePair(path, bytes));
                    else cleanTreeDirSizes.insert(path, bytes);
                }
                rebuildCleanerSizeTree();
                proc->deleteLater();
            });

    if (elevate) proc->start("sudo", QStringList() << "-n" << "bash" << "-c" << script);
    else proc->start("bash", QStringList() << "-c" << script);
}

void MainWindow::rebuildCleanerSizeTree() {
    cleanSizeTree->blockSignals(true);
    cleanSizeTree->clear();

    QStringList dirs = cleanTreeDirSizes.keys();
    dirs.sort();   // parents before children, so makeItem always finds its parent

    QHash<QString, QTreeWidgetItem*> itemForPath;
    auto makeItem = [&](const QString &path, qint64 bytes, bool isDir) {
        QTreeWidgetItem *parent = nullptr;
        QString parentPath = path.section('/', 0, -2);
        while (parentPath.length() > cleanTreeRoot.length()) {
            if (itemForPath.contains(parentPath)) { parent = itemForPath.value(parentPath); break; }
            parentPath = parentPath.section('/', 0, -2);
        }
        QString label = parent ? path.mid(parent->data(0, Qt::UserRole).toString().length() + 1)
                               : path.mid(cleanTreeRoot.length() + 1);
        QTreeWidgetItem *item = parent ? new QTreeWidgetItem(parent)
                                       : new QTreeWidgetItem(cleanSizeTree);
        item->setText(0, (isDir ? "📁 " : "📄 ") + label);
        item->setText(1, formatSize(bytes));
        item->setData(0, Qt::UserRole, path);
        item->setData(1, Qt::UserRole, bytes);
        if (isCleanablePath(path)) {
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(0, cleanSelectedPaths.contains(path) ? Qt::Checked : Qt::Unchecked);
        } else {
            item->setToolTip(0, tr("Outside the cleanable cache roots — shown for size only."));
            item->setForeground(0, QBrush(QColor("#888888")));
        }
        if (isDir) itemForPath.insert(path, item);
        return item;
    };

    for (const QString &dir : dirs) makeItem(dir, cleanTreeDirSizes.value(dir), true);
    for (const auto &f : cleanTreeBigFiles) makeItem(f.first, f.second, false);

    // Re-sort every level by size, biggest first (the tree was built path-sorted)
    auto bySizeDesc = [](QTreeWidgetItem *a, QTreeWidgetItem *b) {
        return a->data(1, Qt::UserRole).toLongLong() > b->data(1, Qt::UserRole).toLongLong();
    };
    QList<QTreeWidgetItem*> top;
    while (cleanSizeTree->topLevelItemCount() > 0) top << cleanSizeTree->takeTopLevelItem(0);
    std::stable_sort(top.begin(), top.end(), bySizeDesc);
    for (QTreeWidgetItem *item : top) cleanSizeTree->addTopLevelItem(item);
    QList<QTreeWidgetItem*> pending = top;
    while (!pending.isEmpty()) {
        QTreeWidgetItem *parent = pending.takeLast();
        QList<QTreeWidgetItem*> kids = parent->takeChildren();
        std::stable_sort(kids.begin(), kids.end(), bySizeDesc);
        parent->addChildren(kids);
        pending << kids;
    }

    cleanSizeTree->expandToDepth(0);
    cleanSizeTree->blockSignals(false);

    cleanStatusLabel->setText(tr("%1: %2 folders measured.")
                              .arg(cleanTreeRoot).arg(cleanTreeDirSizes.size()));
    cleanStatusLabel->setStyleSheet("color:#888;");
    updateCleanerSummary();
}

// Category sizes plus tree paths, skipping any tree path whose ancestor is
// already checked so the total is not counted twice.
void MainWindow::updateCleanerSummary() {
    qint64 total = 0;
    int items = 0;
    bool needsSudo = false;

    for (const CleanTarget &c : cleanTargets) {
        if (!cleanSelectedIds.contains(c.id) || !c.available) continue;
        total += c.bytes;
        items++;
        if (c.system) needsSudo = true;
    }
    for (const QString &p : cleanSelectedPaths) {
        QString parent = p.section('/', 0, -2);
        bool covered = false;
        while (parent.length() > 1) {
            if (cleanSelectedPaths.contains(parent)) { covered = true; break; }
            parent = parent.section('/', 0, -2);
        }
        if (covered) continue;
        total += cleanTreeDirSizes.value(p, 0);
        items++;
        if (!p.startsWith(QDir::homePath())) needsSudo = true;
    }

    if (items == 0) {
        cleanSummaryLabel->setText(tr("Nothing selected."));
        cleanSummaryLabel->setStyleSheet("color:#888;");
        cleanRunButton->setEnabled(false);
        return;
    }
    cleanSummaryLabel->setText(tr("🗑️ %1 item(s) selected — frees about %2%3")
                               .arg(items).arg(formatSize(total))
                               .arg(needsSudo ? tr("  (needs sudo)") : QString()));
    cleanSummaryLabel->setStyleSheet("color:#e67e22; font-weight:bold;");
    cleanRunButton->setEnabled(true);
}

// ---------------------------------------------------------------------------
// Deleting — always in a visible terminal, so every removed path is on screen.
// ---------------------------------------------------------------------------
void MainWindow::runCleanerClean() {
    QStringList summary;
    QList<const CleanTarget*> chosen;
    bool needsSudo = false;

    for (const CleanTarget &c : cleanTargets) {
        if (!cleanSelectedIds.contains(c.id) || !c.available) continue;
        chosen.append(&c);
        summary << QString("• %1  (%2)").arg(c.label, formatSize(c.bytes));
        if (c.system) needsSudo = true;
    }

    QStringList paths;
    for (const QString &p : cleanSelectedPaths) {
        if (!isCleanablePath(p)) continue;          // re-checked at the last moment
        paths << p;
        summary << QString("• %1").arg(p);
        if (!p.startsWith(QDir::homePath())) needsSudo = true;
    }
    paths.sort();

    if (chosen.isEmpty() && paths.isEmpty()) return;

    QString detail = summary.mid(0, 25).join('\n');
    if (summary.size() > 25) detail += tr("\n… and %1 more").arg(summary.size() - 25);

    QMessageBox confirm(this);
    confirm.setWindowTitle(tr("Clean These?"));
    confirm.setIcon(QMessageBox::Warning);
    confirm.setText(tr("About to delete %1 item(s). This cannot be undone.").arg(summary.size()));
    confirm.setInformativeText(detail + tr("\n\nEverything runs in a terminal so you see each path. "
                                           "You will be asked for your sudo password once — caches "
                                           "routinely contain root-owned files that would otherwise "
                                           "be silently skipped."));
    confirm.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    confirm.setDefaultButton(QMessageBox::Cancel);
    if (confirm.exec() != QMessageBox::Yes) return;

    QString script =
        "shopt -s nullglob\n"
        "H=" + cleanQuote(QDir::homePath()) + "\n"
        "U=" + cleanQuote(qgetenv("USER").isEmpty() ? QDir::home().dirName()
                                                    : QString::fromLocal8Bit(qgetenv("USER"))) + "\n"
        "I=" + QString::number(getuid()) + "\n"
        "before=$(df -B1 --output=avail / | tail -n1)\n"
        "echo '=== CachyOsTools Cleaner ==='\n"
        "\n"
        // Caches collect root-owned strays — anything a tool once ran under sudo
        // (memflow PDBs, docker, pip as root) leaves files the user cannot delete,
        // and the clean silently half-finished. So ask once, up front, always.
        "echo 'Cleaning needs root for files other users or root left in your caches.'\n"
        "sudo -v || { echo 'No sudo — nothing was deleted.'; exit 1; }\n"
        // A big clean outlives sudo's 5-minute timestamp; refresh until we exit.
        "( while kill -0 $$ 2>/dev/null; do sudo -n -v 2>/dev/null; sleep 45; done ) &\n"
        "\n"
        // Every deletion goes through $RM. Tools that talk to the user's session
        // (balooctl, flatpak --user) deliberately do NOT, or they would act on
        // root's session instead of yours.
        "RM=\"sudo rm -rf --\"\n";
    Q_UNUSED(needsSudo)

    for (const CleanTarget *c : chosen) {
        script += QString("echo; echo '--- %1 ---'\n").arg(QString(c->label).replace('\'', ' '));
        QString body;
        if (!c->cleanCmd.isEmpty()) {
            body = c->cleanCmd;
        } else {
            body = QString("for f in %1; do [ -e \"$f\" ] || continue; "
                           "echo \"  removing $f\"; $RM \"$f\"; done\n").arg(c->paths.join(' '));
        }
        // System categories run each line under sudo rather than the whole
        // script, so $H stays the user's home and not /root.
        if (c->system) {
            script += "sudo bash -s <<'CLEANEOF'\n";
            script += "shopt -s nullglob\n";
            script += QString("H=%1\n").arg(cleanQuote(QDir::homePath()));
            script += "RM=\"rm -rf --\"\n";   // already root inside this heredoc
            script += body;
            script += "CLEANEOF\n";
        } else {
            script += body;
        }
    }

    for (const QString &p : paths) {
        script += QString("echo \"  removing %1\"\n").arg(p);
        script += QString("$RM %1\n").arg(cleanQuote(p));
    }

    script +=
        "after=$(df -B1 --output=avail / | tail -n1)\n"
        "freed=$(( after - before ))\n"
        "echo; echo \"Freed $(numfmt --to=iec ${freed#-} 2>/dev/null || echo ${freed}) on /.\"\n";

    // Written and launched as the user; sudo is used only on the lines above.
    static int cleanRunId = 0;
    QString scriptPath = QDir::tempPath() + QString("/cachyos_cleaner_%1_%2.sh")
                         .arg(QCoreApplication::applicationPid()).arg(++cleanRunId);
    QFile f(scriptPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), tr("Could not create the cleaning script."));
        return;
    }
    f.write("#!/bin/bash\n");
    f.write(script.toUtf8());
    f.write("\necho ''; echo 'Done. Press Enter to close.'; read -r\nrm -f \"$0\"\n");
    f.close();
    f.setPermissions(f.permissions() | QFile::ExeOwner);
    runSudoCommandInTerminal(scriptPath);

    cleanSelectedIds.clear();
    cleanSelectedPaths.clear();
    cleanScanDone = false;
    rebuildCleanerCategoryTree();
    cleanStatusLabel->setText(tr("Cleaning in the terminal — press Analyze again when it finishes."));
    cleanStatusLabel->setStyleSheet("color:#888;");
}
