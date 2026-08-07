
// ISO Creator exclusion panels: "Exclude Big Folders" (home) and "~/.config filter".
// Checked items are excluded from the system snapshot when the ISO is built.

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>

// One full scan caches every directory size (du) and every file >= 10 MB (find).
// Filter changes only re-filter the cache, so the tree refreshes instantly.
void MainWindow::startIsoHomeScan() {
    ui->isoScanStatusLabel->setText("⏳ Scanning home folder sizes (first scan can take a minute)...");
    ui->isoRescanButton->setEnabled(false);

    QString home = QDir::homePath();
    QString script = QString(
        "du -x -B1 --max-depth=6 '%1' 2>/dev/null; "
        "echo '===FILES==='; "
        "find '%1' -xdev -type f -size +10M -printf '%s\\t%p\\n' 2>/dev/null"
    ).arg(home);

    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, proc, home](int, QProcess::ExitStatus) {
                isoHomeDirSizes.clear();
                isoHomeBigFiles.clear();

                const QStringList lines = QString::fromUtf8(proc->readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
                bool inFiles = false;
                for (const QString &line : lines) {
                    if (line == "===FILES===") { inFiles = true; continue; }
                    int tab = line.indexOf('\t');
                    if (tab <= 0) continue;
                    qint64 bytes = line.left(tab).toLongLong();
                    QString path = line.mid(tab + 1);
                    if (path == home) continue;                 // the home root itself
                    if (path.startsWith(home + "/iso")) continue; // ISO build/output dir, always excluded anyway
                    if (inFiles) {
                        isoHomeBigFiles.append(qMakePair(path, bytes));
                    } else {
                        isoHomeDirSizes.insert(path, bytes);
                    }
                }

                isoHomeScanDone = true;
                ui->isoRescanButton->setEnabled(true);
                rebuildIsoBigTree();
                refreshIsoExcludedSizes();
                proc->deleteLater();
            });
    proc->start("bash", QStringList() << "-c" << script);
}

// Rebuilds the big-folders tree from the cached scan using the current
// size filters. Check states survive rebuilds via isoExcludedPaths.
void MainWindow::rebuildIsoBigTree() {
    qint64 folderMin = (qint64)ui->isoFolderSizeSpin->value() * 1024 * 1024;
    qint64 fileMin = (qint64)ui->isoFileSizeSpin->value() * 1024 * 1024;
    QString home = QDir::homePath();

    ui->isoBigTree->blockSignals(true);
    ui->isoBigTree->clear();
    // Path column fills the panel (long paths elide), Size column stays visible
    ui->isoBigTree->header()->setStretchLastSection(false);
    ui->isoBigTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->isoBigTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->isoBigTree->setTextElideMode(Qt::ElideMiddle);

    // Qualifying directories, sorted so parents come before children
    QStringList dirs;
    for (auto it = isoHomeDirSizes.constBegin(); it != isoHomeDirSizes.constEnd(); ++it) {
        if (it.value() >= folderMin) dirs << it.key();
    }
    dirs.sort();

    QHash<QString, QTreeWidgetItem*> itemForPath;
    auto makeItem = [&](const QString &path, qint64 bytes, bool isDir) {
        // Attach to the deepest qualifying ancestor directory, else top level
        QTreeWidgetItem *parent = nullptr;
        QString parentPath = path.section('/', 0, -2);
        while (parentPath.length() > home.length()) {
            if (itemForPath.contains(parentPath)) { parent = itemForPath.value(parentPath); break; }
            parentPath = parentPath.section('/', 0, -2);
        }
        QString label = parent ? path.mid(parent->data(0, Qt::UserRole).toString().length() + 1)
                               : "~" + path.mid(home.length());
        QTreeWidgetItem *item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(ui->isoBigTree);
        item->setText(0, (isDir ? "📁 " : "📄 ") + label);
        item->setText(1, formatSize(bytes));
        item->setData(0, Qt::UserRole, path);
        item->setData(1, Qt::UserRole, bytes);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(0, isoExcludedPaths.contains(path) ? Qt::Checked : Qt::Unchecked);
        if (isDir) itemForPath.insert(path, item);
        return item;
    };

    for (const QString &dir : dirs) {
        makeItem(dir, isoHomeDirSizes.value(dir), true);
    }
    for (const auto &file : isoHomeBigFiles) {
        if (file.second >= fileMin) makeItem(file.first, file.second, false);
    }

    // Sort every level by size, largest first (folders and files mixed),
    // mirroring the ~/.config panel. Built path-sorted first because makeItem
    // needs parents to exist before their children.
    auto bySizeDesc = [](QTreeWidgetItem *a, QTreeWidgetItem *b) {
        return a->data(1, Qt::UserRole).toLongLong() > b->data(1, Qt::UserRole).toLongLong();
    };
    QList<QTreeWidgetItem*> top;
    while (ui->isoBigTree->topLevelItemCount() > 0) top << ui->isoBigTree->takeTopLevelItem(0);
    std::stable_sort(top.begin(), top.end(), bySizeDesc);
    QList<QTreeWidgetItem*> pending = top;
    for (QTreeWidgetItem *item : top) ui->isoBigTree->addTopLevelItem(item);
    while (!pending.isEmpty()) {
        QTreeWidgetItem *parent = pending.takeLast();
        QList<QTreeWidgetItem*> kids = parent->takeChildren();
        std::stable_sort(kids.begin(), kids.end(), bySizeDesc);
        parent->addChildren(kids);
        pending << kids;
    }

    ui->isoBigTree->expandAll();
    ui->isoBigTree->blockSignals(false);

    ui->isoScanStatusLabel->setText(QString("%1 folders and %2 large files shown — checked = contents excluded (folders themselves stay, empty).")
                                    .arg(dirs.size())
                                    .arg(std::count_if(isoHomeBigFiles.begin(), isoHomeBigFiles.end(),
                                                       [fileMin](const QPair<QString, qint64> &f) { return f.second >= fileMin; })));
}

void MainWindow::startIsoConfigScan() {
    ui->isoConfigRescanButton->setEnabled(false);

    QString configDir = QDir::homePath() + "/.config";
    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, proc, configDir](int, QProcess::ExitStatus) {
                // Collect entry -> size, largest first
                QList<QPair<QString, qint64>> entries;
                const QStringList lines = QString::fromUtf8(proc->readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
                for (const QString &line : lines) {
                    int tab = line.indexOf('\t');
                    if (tab <= 0) continue;
                    qint64 bytes = line.left(tab).toLongLong();
                    QString path = line.mid(tab + 1);
                    if (path == configDir) continue;
                    entries.append(qMakePair(path, bytes));
                }
                std::sort(entries.begin(), entries.end(),
                          [](const QPair<QString, qint64> &a, const QPair<QString, qint64> &b) { return a.second > b.second; });

                ui->isoConfigTree->blockSignals(true);
                ui->isoConfigTree->clear();
                for (const auto &entry : entries) {
                    QTreeWidgetItem *item = new QTreeWidgetItem(ui->isoConfigTree);
                    item->setText(0, QFileInfo(entry.first).fileName());
                    item->setText(1, formatSize(entry.second));
                    item->setData(0, Qt::UserRole, entry.first);
                    item->setData(1, Qt::UserRole, entry.second);
                    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                    item->setCheckState(0, isoExcludedPaths.contains(entry.first) ? Qt::Checked : Qt::Unchecked);
                }
                ui->isoConfigTree->resizeColumnToContents(0);
                ui->isoConfigTree->blockSignals(false);

                isoConfigScanDone = true;
                ui->isoConfigRescanButton->setEnabled(true);
                proc->deleteLater();
            });
    proc->start("bash", QStringList() << "-c"
                << QString("du -x -B1 --max-depth=1 '%1' 2>/dev/null; "
                           "find '%1' -maxdepth 1 -type f -printf '0\\t%p\\n' 2>/dev/null").arg(configDir));
}

void MainWindow::onIsoExcludeItemChanged(QTreeWidgetItem *item, int column) {
    if (column != 0 || !item) return;
    QString path = item->data(0, Qt::UserRole).toString();
    if (path.isEmpty()) return;

    if (item->checkState(0) == Qt::Checked) {
        isoExcludedPaths.insert(path);
        isoExcludedSizes.insert(path, item->data(1, Qt::UserRole).toLongLong());
    } else {
        isoExcludedPaths.remove(path);
        isoExcludedSizes.remove(path);
    }
    updateIsoExcludeSummary();
}

// Back-fills sizes for excluded paths from the scan cache. A profile loaded
// before the first scan finishes stores 0 for directories (the cache is still
// empty), which undercounts the savings label — the exclusions themselves are
// path-based and unaffected. Never downgrades a known size to 0.
void MainWindow::refreshIsoExcludedSizes() {
    for (auto it = isoExcludedSizes.begin(); it != isoExcludedSizes.end(); ++it) {
        qint64 bytes = isoHomeDirSizes.value(it.key(), 0);
        if (bytes == 0) {
            QFileInfo info(it.key());
            if (info.isFile()) bytes = info.size();
        }
        if (bytes > 0) it.value() = bytes;
    }
    updateIsoExcludeSummary();
}

void MainWindow::updateIsoExcludeSummary() {
    if (isoExcludedPaths.isEmpty()) {
        ui->isoExcludeSummaryLabel->setText("Nothing excluded — the ISO will contain the complete system.");
        ui->isoExcludeSummaryLabel->setStyleSheet("");
        return;
    }

    // Sum only paths whose ancestors are not also excluded (avoid double counting)
    qint64 total = 0;
    for (auto it = isoExcludedSizes.constBegin(); it != isoExcludedSizes.constEnd(); ++it) {
        QString parent = it.key().section('/', 0, -2);
        bool ancestorExcluded = false;
        while (parent.length() > QDir::homePath().length()) {
            if (isoExcludedPaths.contains(parent)) { ancestorExcluded = true; break; }
            parent = parent.section('/', 0, -2);
        }
        if (!ancestorExcluded) total += it.value();
    }

    ui->isoExcludeSummaryLabel->setText(QString("🚫 %1 item(s) excluded from the ISO — about %2 smaller.")
                                        .arg(isoExcludedPaths.size()).arg(formatSize(total)));
    ui->isoExcludeSummaryLabel->setStyleSheet("color: #e67e22; font-weight: bold;");
}

QStringList MainWindow::collectIsoExcludePaths() const {
    QStringList paths = isoExcludedPaths.values();
    paths.sort();
    return paths;
}

// Re-applies isoExcludedPaths check states to the ~/.config tree
// (the big-folders tree gets the same treatment inside rebuildIsoBigTree).
void MainWindow::syncIsoConfigTreeChecks() {
    ui->isoConfigTree->blockSignals(true);
    for (int i = 0; i < ui->isoConfigTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = ui->isoConfigTree->topLevelItem(i);
        QString path = item->data(0, Qt::UserRole).toString();
        item->setCheckState(0, isoExcludedPaths.contains(path) ? Qt::Checked : Qt::Unchecked);
    }
    ui->isoConfigTree->blockSignals(false);
}

void MainWindow::saveIsoExcludeProfile() {
    if (isoExcludedPaths.isEmpty()) {
        QMessageBox::information(this, "Nothing to Save",
                                 "No exclusions are checked yet.\nCheck some folders/files first, then save.");
        return;
    }

    QString profileDir = QDir::homePath() + "/.config/cachyostools";
    QDir().mkpath(profileDir);
    QString path = QFileDialog::getSaveFileName(this, "Save Exclusion Profile",
                                                profileDir + "/my-exclusions.json",
                                                "Exclusion Profiles (*.json)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".json", Qt::CaseInsensitive)) path += ".json";

    QJsonArray pathsArray;
    for (const QString &p : collectIsoExcludePaths()) {
        pathsArray.append(p);
    }
    QJsonObject profile;
    profile["version"] = 1;
    profile["app"] = "CachyOsTools";
    profile["saved"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    profile["excludedPaths"] = pathsArray;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Save Failed", "Could not write to:\n" + path);
        return;
    }
    file.write(QJsonDocument(profile).toJson(QJsonDocument::Indented));
    file.close();

    QMessageBox::information(this, "Profile Saved",
                             QString("Saved %1 exclusion(s) to:\n%2").arg(isoExcludedPaths.size()).arg(path));
}

void MainWindow::loadIsoExcludeProfile() {
    QString profileDir = QDir::homePath() + "/.config/cachyostools";
    QString path = QFileDialog::getOpenFileName(this, "Load Exclusion Profile",
                                                QDir(profileDir).exists() ? profileDir : QDir::homePath(),
                                                "Exclusion Profiles (*.json)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Load Failed", "Could not read:\n" + path);
        return;
    }
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError || !doc.isObject() ||
        !doc.object().contains("excludedPaths")) {
        QMessageBox::warning(this, "Invalid Profile",
                             "This file is not a valid exclusion profile.");
        return;
    }

    // Replace the current selection with the profile; paths that no longer
    // exist on this system are skipped and reported, the rest still apply.
    isoExcludedPaths.clear();
    isoExcludedSizes.clear();
    int applied = 0;
    QStringList missing;
    const QJsonArray pathsArray = doc.object().value("excludedPaths").toArray();
    for (const QJsonValue &value : pathsArray) {
        QString p = value.toString();
        if (p.isEmpty()) continue;
        if (!QFileInfo::exists(p)) {
            missing << p;
            continue;
        }
        isoExcludedPaths.insert(p);
        qint64 bytes = isoHomeDirSizes.value(p, 0);
        if (bytes == 0) {
            QFileInfo info(p);
            if (info.isFile()) bytes = info.size();
        }
        isoExcludedSizes.insert(p, bytes);
        applied++;
    }

    if (isoHomeScanDone) rebuildIsoBigTree();
    syncIsoConfigTreeChecks();
    updateIsoExcludeSummary();

    QString report = QString("Applied %1 exclusion(s) from the profile.").arg(applied);
    if (!missing.isEmpty()) {
        QString missingList = missing.mid(0, 10).join("\n  ");
        if (missing.size() > 10) missingList += QString("\n  ... and %1 more").arg(missing.size() - 10);
        report += QString("\n\n%1 path(s) no longer exist on this system and were skipped:\n  %2")
                  .arg(missing.size()).arg(missingList);
    }
    QMessageBox::information(this, "Profile Loaded", report);
}

void MainWindow::clearIsoExclusions() {
    if (isoExcludedPaths.isEmpty()) return;
    isoExcludedPaths.clear();
    isoExcludedSizes.clear();
    if (isoHomeScanDone) rebuildIsoBigTree();
    syncIsoConfigTreeChecks();
    updateIsoExcludeSummary();
}
