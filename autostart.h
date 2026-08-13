
// Services -> 🚀 Autostart sub-tab: msconfig for Arch.
//
// Three things start programs when you log in, and nothing shows them together:
//   ~/.config/autostart/*.desktop   your own entries
//   /etc/xdg/autostart/*.desktop    entries every package drops in
//   systemctl --user                enabled user units
// This lists all three, and adds new ones from any executable or script in one
// file-picker click.
//
// Everything here is per-user: no sudo, no terminal. Disabling a system-wide
// entry writes a Hidden=true override into your own autostart folder, which is
// the XDG-blessed way to say "not for me" without touching /etc.

#include <QGroupBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QSpinBox>
#include "script_helpers.h"

static const char *AUTOSTART_MARKER = "X-CachyOsTools-Created";

// Read the [Desktop Entry] group of a .desktop file. Locale variants
// (Name[de]=…) are skipped — we only ever display and edit the plain keys.
static QHash<QString, QString> parseDesktopEntry(const QString &path) {
    QHash<QString, QString> kv;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return kv;
    QTextStream ts(&f);
    bool inEntry = false;
    while (!ts.atEnd()) {
        const QString line = ts.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;
        if (line.startsWith('[')) { inEntry = (line == "[Desktop Entry]"); continue; }
        if (!inEntry) continue;
        const int eq = line.indexOf('=');
        if (eq <= 0) continue;
        const QString key = line.left(eq).trimmed();
        if (key.contains('[')) continue;
        kv.insert(key, line.mid(eq + 1).trimmed());
    }
    return kv;
}

// Set one key inside [Desktop Entry], leaving every other line of the file
// exactly as it was. Creates the key if it is missing.
static bool setDesktopKey(const QString &path, const QString &key, const QString &value, QString &err) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        err = QObject::tr("Cannot read %1").arg(path);
        return false;
    }
    QStringList lines;
    bool inEntry = false, replaced = false;
    int entryHeader = -1;
    QTextStream ts(&f);
    while (!ts.atEnd()) lines << ts.readLine();
    f.close();

    for (int i = 0; i < lines.size(); ++i) {
        const QString t = lines[i].trimmed();
        if (t.startsWith('[')) {
            if (t == "[Desktop Entry]") { inEntry = true; entryHeader = i; }
            else if (inEntry) { lines.insert(i, key + "=" + value); replaced = true; break; }
            continue;
        }
        if (!inEntry) continue;
        const int eq = t.indexOf('=');
        if (eq > 0 && t.left(eq).trimmed() == key) {
            lines[i] = key + "=" + value;
            replaced = true;
            break;
        }
    }
    if (!replaced) {
        if (entryHeader >= 0) lines.insert(entryHeader + 1, key + "=" + value);
        else lines << "[Desktop Entry]" << (key + "=" + value);
    }

    if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        err = QObject::tr("Cannot write %1").arg(path);
        return false;
    }
    QTextStream out(&f);
    for (const QString &l : lines) out << l << "\n";
    f.close();
    return true;
}

// Quote one argument for a .desktop Exec field. Per the Desktop Entry spec the
// argument is wrapped in double quotes and \ " ` $ are backslash-escaped.
static QString desktopExecArg(const QString &arg) {
    QString out = arg;
    out.replace("\\", "\\\\");
    out.replace("\"", "\\\"");
    out.replace("`", "\\`");
    out.replace("$", "\\$");
    return "\"" + out + "\"";
}

// Strip the %f/%U/%i style field codes before handing an Exec line to a shell.
static QString execForShell(const QString &exec) {
    QString s = exec;
    s.remove(QRegularExpression("%[fFuUdDnNickvm]"));
    return s.trimmed();
}

// ============================================================
// Tab construction
// ============================================================
void MainWindow::setupAutostartTab() {
    // ---------- the Autostart page ----------
    autostartTab = new QWidget();
    autostartTab->setObjectName("autostartSubTab");
    QVBoxLayout *root = new QVBoxLayout(autostartTab);

    QHBoxLayout *bar = new QHBoxLayout();
    QPushButton *addBtn = new QPushButton(tr("➕ Add Program or Script…"), autostartTab);
    addBtn->setToolTip(tr("Pick any executable or script — it starts automatically at your next login."));
    QFont addFont = addBtn->font();
    addFont.setBold(true);
    addBtn->setFont(addFont);
    QPushButton *refreshBtn = new QPushButton(tr("🔄 Refresh"), autostartTab);
    autoShowSystemCheck = new QCheckBox(tr("Include entries installed by packages"), autostartTab);
    autoShowSystemCheck->setToolTip(tr("/etc/xdg/autostart — the things your desktop and applications "
                                       "added without asking."));
    autoStatusLabel = new QLabel(autostartTab);
    autoStatusLabel->setStyleSheet("color:#888;");
    bar->addWidget(addBtn);
    bar->addWidget(refreshBtn);
    bar->addWidget(autoShowSystemCheck);
    bar->addWidget(autoStatusLabel, 1);
    root->addLayout(bar);

    autoTable = new QTableWidget(autostartTab);
    autoTable->setColumnCount(5);
    autoTable->setHorizontalHeaderLabels(QStringList()
        << tr("Starts") << tr("Name") << tr("Command") << tr("Kind") << tr("Defined in"));
    autoTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    autoTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    autoTable->setSelectionMode(QAbstractItemView::SingleSelection);
    autoTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    autoTable->setAlternatingRowColors(true);
    autoTable->setSortingEnabled(true);
    autoTable->verticalHeader()->setVisible(false);
    root->addWidget(autoTable);

    QHBoxLayout *actionBar = new QHBoxLayout();
    QPushButton *enableBtn = new QPushButton(tr("✅ Enable"), autostartTab);
    QPushButton *disableBtn = new QPushButton(tr("🚫 Disable"), autostartTab);
    QPushButton *runBtn = new QPushButton(tr("▶️ Run Now"), autostartTab);
    runBtn->setToolTip(tr("Start it once, right now, to check that it actually works."));
    QPushButton *editBtn = new QPushButton(tr("✏️ Edit"), autostartTab);
    QPushButton *removeBtn = new QPushButton(tr("🗑️ Remove"), autostartTab);
    QPushButton *folderBtn = new QPushButton(tr("📁 Open Folder"), autostartTab);
    actionBar->addStretch();
    for (QPushButton *b : {enableBtn, disableBtn, runBtn, editBtn, removeBtn, folderBtn}) actionBar->addWidget(b);
    root->addLayout(actionBar);

    // ---------- fold the existing Services content into a sub-tab ----------
    // servicesTab owns its widgets from the .ui file; move them into a page of
    // their own so Autostart can sit beside them without touching that layout.
    QWidget *servicesPage = new QWidget();
    servicesPage->setObjectName("servicesMainSubTab");
    QVBoxLayout *pageLay = new QVBoxLayout(servicesPage);

    if (QLayout *orig = ui->servicesTab->layout()) {
        pageLay->setContentsMargins(orig->contentsMargins());
        QList<QLayoutItem *> items;
        while (QLayoutItem *item = orig->takeAt(0)) items << item;
        for (QLayoutItem *item : items) {
            if (QWidget *w = item->widget()) {
                // The item is a QWidgetItem wrapper; addWidget makes a fresh one.
                pageLay->addWidget(w);
                delete item;
            } else if (QLayout *sub = item->layout()) {
                // A QLayout *is* its own QLayoutItem — addLayout takes ownership,
                // so deleting `item` here would destroy the row we just added.
                pageLay->addLayout(sub);
            } else {
                pageLay->addItem(item);   // spacers move across as-is
            }
        }
        delete orig;   // now empty; frees servicesTab to take a new layout
    }

    QVBoxLayout *servicesRoot = new QVBoxLayout(ui->servicesTab);
    servicesRoot->setContentsMargins(0, 0, 0, 0);
    QTabWidget *servicesTabs = new QTabWidget(ui->servicesTab);
    servicesTabs->setObjectName("servicesSubTabs");
    servicesTabs->addTab(servicesPage, tr("⚙️ Services"));
    servicesTabs->addTab(autostartTab, tr("🚀 Autostart"));
    servicesRoot->addWidget(servicesTabs);

    connect(servicesTabs, &QTabWidget::currentChanged, this, [this, servicesTabs](int index) {
        if (servicesTabs->widget(index) == autostartTab) refreshAutostart();
    });

    // ---------- wiring ----------
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshAutostart);
    connect(autoShowSystemCheck, &QCheckBox::toggled, this, [this](bool) { refreshAutostart(); });
    connect(addBtn, &QPushButton::clicked, this, &MainWindow::addAutostartEntryDialog);
    connect(enableBtn, &QPushButton::clicked, this, [this]() { setAutostartEnabled(true); });
    connect(disableBtn, &QPushButton::clicked, this, [this]() { setAutostartEnabled(false); });
    connect(autoTable, &QTableWidget::cellDoubleClicked, this, [this](int, int) {
        const int idx = currentAutostartIndex();
        if (idx >= 0) setAutostartEnabled(!autostartList[idx].enabled);
    });

    connect(folderBtn, &QPushButton::clicked, this, [this]() {
        const QString dir = QDir::homePath() + "/.config/autostart";
        QDir().mkpath(dir);
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
    });

    connect(runBtn, &QPushButton::clicked, this, [this]() {
        const int idx = currentAutostartIndex();
        if (idx < 0) return;
        const AutostartEntry &e = autostartList[idx];
        if (e.type == "systemd") {
            QProcess::startDetached("systemctl", QStringList() << "--user" << "start" << e.name);
            QMessageBox::information(this, tr("Started"), tr("systemctl --user start %1").arg(e.name));
            return;
        }
        const QString cmd = execForShell(e.exec);
        if (cmd.isEmpty()) {
            QMessageBox::warning(this, tr("Nothing to Run"), tr("This entry has no Exec line."));
            return;
        }
        if (QProcess::startDetached("bash", QStringList() << "-c" << cmd))
            QMessageBox::information(this, tr("Started"),
                tr("Launched:\n\n%1\n\nIf nothing happened, the program failed silently — "
                   "check the Logs tab.").arg(cmd));
        else
            QMessageBox::warning(this, tr("Could Not Start"), tr("The shell refused to run:\n\n%1").arg(cmd));
    });

    connect(editBtn, &QPushButton::clicked, this, [this]() {
        const int idx = currentAutostartIndex();
        if (idx < 0) return;
        const AutostartEntry &e = autostartList[idx];
        if (e.path.isEmpty()) {
            QMessageBox::information(this, tr("Nothing to Edit"), tr("This entry has no file on disk."));
            return;
        }
        if (e.system) {
            QMessageBox::information(this, tr("Package-Owned File"),
                tr("%1 belongs to a package — the next upgrade would overwrite your edits.\n\n"
                   "Use Disable instead: it writes a personal override and leaves the original alone.").arg(e.path));
            return;
        }
        openBuiltinEditor(e.path);
    });

    connect(removeBtn, &QPushButton::clicked, this, [this]() {
        const int idx = currentAutostartIndex();
        if (idx < 0) return;
        const AutostartEntry &e = autostartList[idx];

        if (e.type == "systemd") {
            if (QMessageBox::question(this, tr("Disable Unit"),
                    tr("%1 is a systemd user unit — it cannot be deleted from here, only disabled.\n\n"
                       "Run  systemctl --user disable %1  now?").arg(e.name),
                    QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
            QProcess::execute("systemctl", QStringList() << "--user" << "disable" << e.name);
            refreshAutostart();
            return;
        }
        if (e.system) {
            QMessageBox::information(this, tr("Package-Owned Entry"),
                tr("%1 was installed by a package, so removing the file would just come back on the next "
                   "upgrade — and pacman would flag it.\n\nUse Disable instead.").arg(e.path));
            return;
        }
        if (QMessageBox::question(this, tr("Remove Autostart Entry"),
                tr("Delete this autostart entry?\n\n%1\n\nOnly the .desktop file goes away — "
                   "the program itself is untouched.").arg(e.path),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
        if (!QFile::remove(e.path))
            QMessageBox::warning(this, tr("Could Not Remove"), tr("Failed to delete %1").arg(e.path));
        refreshAutostart();
    });
}

// Row -> autostartList index. Sorting is on, so the row number is not the index.
int MainWindow::currentAutostartIndex() {
    const int row = autoTable->currentRow();
    if (row < 0 || !autoTable->item(row, 1)) {
        QMessageBox::information(this, tr("No Selection"), tr("Select an entry from the list first."));
        return -1;
    }
    bool ok = false;
    const int idx = autoTable->item(row, 1)->data(Qt::UserRole).toInt(&ok);
    if (!ok || idx < 0 || idx >= autostartList.size()) return -1;
    return idx;
}

// ============================================================
// Loading
// ============================================================
void MainWindow::refreshAutostart() {
    autostartList.clear();

    const QString userDir = QDir::homePath() + "/.config/autostart";
    const QString sysDir = "/etc/xdg/autostart";

    // User entries first — a file here shadows the system one with the same name.
    QSet<QString> userFileNames;
    for (const QFileInfo &fi : QDir(userDir).entryInfoList(QStringList() << "*.desktop", QDir::Files)) {
        userFileNames.insert(fi.fileName());
        const QHash<QString, QString> kv = parseDesktopEntry(fi.absoluteFilePath());
        if (kv.isEmpty()) continue;

        AutostartEntry e;
        e.name = kv.value("Name", fi.completeBaseName());
        e.exec = kv.value("Exec");
        e.comment = kv.value("Comment");
        e.path = fi.absoluteFilePath();
        e.type = "xdg";
        e.system = false;
        e.enabled = kv.value("Hidden").compare("true", Qt::CaseInsensitive) != 0 &&
                    kv.value("X-GNOME-Autostart-enabled").compare("false", Qt::CaseInsensitive) != 0;
        e.delay = kv.value("X-GNOME-Autostart-Delay").toInt();
        e.onlyShowIn = kv.value("OnlyShowIn");
        // A Hidden-only file is an override we wrote for a package entry
        e.isOverride = !kv.contains("Exec") || kv.value(AUTOSTART_MARKER).isEmpty()
                       ? QFile::exists(sysDir + "/" + fi.fileName())
                       : false;
        autostartList.append(e);
    }

    if (autoShowSystemCheck->isChecked()) {
        for (const QFileInfo &fi : QDir(sysDir).entryInfoList(QStringList() << "*.desktop", QDir::Files)) {
            if (userFileNames.contains(fi.fileName())) continue;   // already listed, user copy wins
            const QHash<QString, QString> kv = parseDesktopEntry(fi.absoluteFilePath());
            if (kv.isEmpty()) continue;

            AutostartEntry e;
            e.name = kv.value("Name", fi.completeBaseName());
            e.exec = kv.value("Exec");
            e.comment = kv.value("Comment");
            e.path = fi.absoluteFilePath();
            e.type = "xdg";
            e.system = true;
            e.enabled = kv.value("Hidden").compare("true", Qt::CaseInsensitive) != 0 &&
                        kv.value("X-GNOME-Autostart-enabled").compare("false", Qt::CaseInsensitive) != 0;
            e.delay = kv.value("X-GNOME-Autostart-Delay").toInt();
            e.onlyShowIn = kv.value("OnlyShowIn");
            autostartList.append(e);
        }
    }

    // systemd user units come from a process, so the table is filled in its callback
    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, proc](int, QProcess::ExitStatus) {
        for (const QString &line : QString::fromUtf8(proc->readAllStandardOutput()).split('\n', Qt::SkipEmptyParts)) {
            if (!line.startsWith("U|")) continue;
            const QStringList f = line.mid(2).split('|');
            if (f.size() < 4) continue;
            AutostartEntry e;
            e.name = f[0];
            e.comment = f[1];
            e.exec = f[2];
            e.path = f[3];
            e.type = "systemd";
            e.system = !f[3].startsWith(QDir::homePath());
            e.enabled = true;   // we only list enabled units
            e.delay = 0;
            autostartList.append(e);
        }
        proc->deleteLater();
        populateAutostartTable();
    });
    proc->start("bash", QStringList() << "-c" << R"BASH(
systemctl --user list-unit-files --type=service --state=enabled --no-legend --no-pager 2>/dev/null |
awk '{print $1}' | while read -r u; do
  [ -n "$u" ] || continue
  d=$(systemctl --user show -p Description --value "$u" 2>/dev/null)
  x=$(systemctl --user show -p ExecStart --value "$u" 2>/dev/null |
      sed -n 's/.*argv\[\]=\([^;]*\);.*/\1/p' | head -1)
  p=$(systemctl --user show -p FragmentPath --value "$u" 2>/dev/null)
  printf 'U|%s|%s|%s|%s\n' "$u" "$d" "$x" "$p"
done
)BASH");
}

void MainWindow::populateAutostartTable() {
    autoTable->setSortingEnabled(false);
    autoTable->setRowCount(0);

    int enabledCount = 0, systemCount = 0;
    for (int i = 0; i < autostartList.size(); ++i) {
        const AutostartEntry &e = autostartList[i];
        const int row = autoTable->rowCount();
        autoTable->insertRow(row);

        QString when;
        if (!e.enabled) when = tr("⚪ disabled");
        else if (e.delay > 0) when = tr("🟢 login +%1s").arg(e.delay);
        else when = tr("🟢 at login");
        QTableWidgetItem *whenItem = new QTableWidgetItem(when);
        whenItem->setForeground(e.enabled ? QColor("#27ae60") : QColor("#888888"));
        autoTable->setItem(row, 0, whenItem);

        QTableWidgetItem *nameItem = new QTableWidgetItem(e.name);
        nameItem->setData(Qt::UserRole, i);          // survives sorting
        if (!e.comment.isEmpty()) nameItem->setToolTip(e.comment);
        autoTable->setItem(row, 1, nameItem);

        QTableWidgetItem *execItem = new QTableWidgetItem(e.exec);
        execItem->setToolTip(e.exec);
        autoTable->setItem(row, 2, execItem);

        QString kind = (e.type == "systemd") ? tr("systemd user unit") : tr("desktop entry");
        if (!e.onlyShowIn.isEmpty()) kind += tr("  (only in %1)").arg(e.onlyShowIn);
        autoTable->setItem(row, 3, new QTableWidgetItem(kind));

        QTableWidgetItem *srcItem = new QTableWidgetItem(e.system ? tr("package") : tr("you"));
        srcItem->setToolTip(e.path);
        if (e.system) srcItem->setForeground(QColor("#888888"));
        autoTable->setItem(row, 4, srcItem);

        if (e.enabled) enabledCount++;
        if (e.system) systemCount++;
    }

    autoTable->setSortingEnabled(true);
    autoTable->resizeColumnToContents(0);
    autoTable->resizeColumnToContents(1);
    autoTable->resizeColumnToContents(3);
    autoTable->resizeColumnToContents(4);

    autoStatusLabel->setText(tr("%1 starting at login · %2 total%3")
        .arg(enabledCount)
        .arg(autostartList.size())
        .arg(systemCount ? tr(" · %1 from packages").arg(systemCount) : QString()));
}

// ============================================================
// Enable / disable
// ============================================================
void MainWindow::setAutostartEnabled(bool enabled) {
    const int idx = currentAutostartIndex();
    if (idx < 0) return;
    const AutostartEntry e = autostartList[idx];

    if (e.enabled == enabled) {
        refreshAutostart();
        return;
    }

    if (e.type == "systemd") {
        QProcess::execute("systemctl", QStringList() << "--user"
                          << (enabled ? "enable" : "disable") << e.name);
        refreshAutostart();
        return;
    }

    QString err;
    if (e.system) {
        // Never edit /etc/xdg/autostart — shadow it with our own copy instead.
        const QString userDir = QDir::homePath() + "/.config/autostart";
        QDir().mkpath(userDir);
        const QString target = userDir + "/" + QFileInfo(e.path).fileName();
        if (!QFile::exists(target) && !QFile::copy(e.path, target)) {
            QMessageBox::warning(this, tr("Could Not Override"),
                                 tr("Failed to copy %1 to %2").arg(e.path, target));
            return;
        }
        QFile::setPermissions(target, QFile::ReadOwner | QFile::WriteOwner |
                                      QFile::ReadGroup | QFile::ReadOther);
        if (!setDesktopKey(target, "Hidden", enabled ? "false" : "true", err) ||
            !setDesktopKey(target, "X-GNOME-Autostart-enabled", enabled ? "true" : "false", err)) {
            QMessageBox::warning(this, tr("Could Not Save"), err);
            return;
        }
        if (enabled) {
            QMessageBox::information(this, tr("Re-enabled"),
                tr("A personal copy in ~/.config/autostart now re-enables this entry.\n\n"
                   "Delete %1 if you would rather go back to whatever the package ships.").arg(target));
        }
    } else {
        if (!setDesktopKey(e.path, "Hidden", enabled ? "false" : "true", err) ||
            !setDesktopKey(e.path, "X-GNOME-Autostart-enabled", enabled ? "true" : "false", err)) {
            QMessageBox::warning(this, tr("Could Not Save"), err);
            return;
        }
    }
    refreshAutostart();
}

// ============================================================
// Add — pick an executable or script, get an autostart entry
// ============================================================
void MainWindow::addAutostartEntryDialog() {
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Add to Autostart"));
    dlg.setMinimumWidth(560);
    QFormLayout *form = new QFormLayout(&dlg);

    QHBoxLayout *fileLay = new QHBoxLayout();
    QLineEdit *fileEdit = new QLineEdit(&dlg);
    fileEdit->setPlaceholderText(tr("/home/you/scripts/backup.sh   or   /usr/bin/nextcloud"));
    QPushButton *pickBtn = new QPushButton(tr("📂 Choose…"), &dlg);
    fileLay->addWidget(fileEdit, 1);
    fileLay->addWidget(pickBtn);
    form->addRow(tr("Program or script:"), fileLay);

    QLineEdit *nameEdit = new QLineEdit(&dlg);
    form->addRow(tr("Name:"), nameEdit);

    QLineEdit *argsEdit = new QLineEdit(&dlg);
    argsEdit->setPlaceholderText(tr("optional, e.g.  --minimized --profile work"));
    form->addRow(tr("Arguments:"), argsEdit);

    QLineEdit *commentEdit = new QLineEdit(&dlg);
    commentEdit->setPlaceholderText(tr("optional — a note to your future self"));
    form->addRow(tr("Comment:"), commentEdit);

    QSpinBox *delaySpin = new QSpinBox(&dlg);
    delaySpin->setRange(0, 300);
    delaySpin->setSuffix(tr(" seconds"));
    delaySpin->setToolTip(tr("Wait before starting. Useful for anything that needs the network, "
                             "a tray, or a mounted drive to exist first."));
    form->addRow(tr("Delay:"), delaySpin);

    QCheckBox *terminalCheck = new QCheckBox(tr("Run in a terminal window"), &dlg);
    terminalCheck->setToolTip(tr("For scripts whose output you want to see."));
    form->addRow(QString(), terminalCheck);

    QLabel *status = new QLabel(&dlg);
    status->setWordWrap(true);
    status->setTextFormat(Qt::RichText);
    form->addRow(status);

    // Live feedback on the chosen file: exists? executable? has a shebang?
    auto validate = [fileEdit, status]() -> bool {
        const QString p = fileEdit->text().trimmed();
        if (p.isEmpty()) { status->clear(); return false; }
        QFileInfo fi(p);
        if (!fi.exists()) {
            status->setText(tr("<span style='color:#c0392b;'>❌ No such file.</span>"));
            return false;
        }
        if (fi.isDir()) {
            status->setText(tr("<span style='color:#c0392b;'>❌ That is a folder, not a program.</span>"));
            return false;
        }
        if (!fi.isExecutable()) {
            status->setText(tr("<span style='color:#e67e22;'>⚠️ Not executable — "
                               "the file's execute bit will be set for you when you click Add.</span>"));
            return true;
        }
        status->setText(tr("<span style='color:#27ae60;'>✅ Executable.</span>"));
        return true;
    };

    connect(fileEdit, &QLineEdit::textChanged, &dlg, [validate, nameEdit, fileEdit](const QString &p) {
        validate();
        if (nameEdit->text().isEmpty() || nameEdit->property("auto").toBool()) {
            QString base = QFileInfo(p).completeBaseName();
            if (!base.isEmpty()) base[0] = base[0].toUpper();
            nameEdit->setText(base);
            nameEdit->setProperty("auto", true);
        }
        Q_UNUSED(fileEdit)
    });
    connect(nameEdit, &QLineEdit::textEdited, &dlg, [nameEdit]() { nameEdit->setProperty("auto", false); });

    connect(pickBtn, &QPushButton::clicked, &dlg, [&dlg, fileEdit]() {
        const QString start = fileEdit->text().isEmpty() ? QDir::homePath()
                                                         : QFileInfo(fileEdit->text()).absolutePath();
        const QString f = QFileDialog::getOpenFileName(&dlg, tr("Choose a program or script"), start,
                                                       tr("Programs and scripts (*);;Shell scripts (*.sh);;"
                                                          "Python scripts (*.py);;AppImages (*.AppImage)"));
        if (!f.isEmpty()) fileEdit->setText(f);
    });

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    bb->button(QDialogButtonBox::Ok)->setText(tr("Add"));
    form->addRow(bb);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    const QString program = fileEdit->text().trimmed();
    const QString name = nameEdit->text().trimmed();
    const QString args = argsEdit->text().trimmed();
    const int delay = delaySpin->value();

    QFileInfo fi(program);
    if (program.isEmpty() || !fi.exists() || fi.isDir()) {
        QMessageBox::warning(this, tr("Pick a Program"), tr("Choose an executable file or a script."));
        return;
    }
    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("Name Required"), tr("Give the entry a name so you recognise it later."));
        return;
    }

    // Make it runnable rather than silently creating an entry that can't start
    if (!fi.isExecutable()) {
        QFile f(program);
        if (!f.setPermissions(f.permissions() | QFile::ExeOwner | QFile::ExeGroup)) {
            QMessageBox::warning(this, tr("Not Executable"),
                tr("%1 cannot be run and the execute bit could not be set "
                   "(the file may belong to another user).").arg(program));
            return;
        }
    }
    // A script with no shebang gets run by whatever the desktop feels like
    if (fi.suffix() == "sh" || fi.suffix() == "py" || fi.suffix().isEmpty()) {
        QFile f(program);
        if (f.open(QIODevice::ReadOnly)) {
            const QByteArray head = f.read(2);
            f.close();
            if (head != "#!") {
                if (QMessageBox::warning(this, tr("No Shebang Line"),
                        tr("%1 does not start with a #! line, so nothing knows which interpreter to use. "
                           "It may fail to start at login.\n\nAdd it anyway?").arg(fi.fileName()),
                        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
            }
        }
    }

    // Build the Exec line. A delay needs a shell, so the whole thing becomes one
    // quoted argument to bash -c.
    QString execLine;
    if (delay > 0) {
        QString inner = QString("sleep %1; exec %2").arg(delay).arg(shQuote(program));
        if (!args.isEmpty()) inner += " " + args;
        execLine = "bash -c " + desktopExecArg(inner);
    } else {
        execLine = desktopExecArg(program);
        if (!args.isEmpty()) execLine += " " + args;
    }

    const QString userDir = QDir::homePath() + "/.config/autostart";
    if (!QDir().mkpath(userDir)) {
        QMessageBox::critical(this, tr("Cannot Create Folder"), tr("Failed to create %1").arg(userDir));
        return;
    }

    QString slug = name.toLower();
    slug.replace(QRegularExpression("[^a-z0-9]+"), "-");
    slug.remove(QRegularExpression("^-+|-+$"));
    if (slug.isEmpty()) slug = "autostart";
    QString target = userDir + "/" + slug + ".desktop";
    for (int n = 2; QFile::exists(target); ++n)
        target = QString("%1/%2-%3.desktop").arg(userDir, slug).arg(n);

    QFile out(target);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Cannot Write"), tr("Failed to create %1").arg(target));
        return;
    }
    QTextStream ts(&out);
    ts << "[Desktop Entry]\n"
       << "Type=Application\n"
       << "Name=" << name << "\n";
    if (!commentEdit->text().trimmed().isEmpty())
        ts << "Comment=" << commentEdit->text().trimmed() << "\n";
    ts << "Exec=" << execLine << "\n"
       << "Terminal=" << (terminalCheck->isChecked() ? "true" : "false") << "\n"
       << "Hidden=false\n"
       << "X-GNOME-Autostart-enabled=true\n";
    if (delay > 0) ts << "X-GNOME-Autostart-Delay=" << delay << "\n";
    ts << AUTOSTART_MARKER << "=true\n";
    out.close();

    refreshAutostart();
    QMessageBox::information(this, tr("Added to Autostart"),
        tr("%1 will start at your next login.\n\nEntry written to:\n%2\n\n"
           "Use Run Now to check it works before you reboot.").arg(name, target));
}
