
// Uninstall tab functions - "Add/Remove Programs" style list of all installed software

#include <QSet>

// Table item that sorts by the numeric value stored in Qt::UserRole
// instead of by display text, so the Size column sorts correctly.
class NumericTableWidgetItem : public QTableWidgetItem {
public:
    using QTableWidgetItem::QTableWidgetItem;
    bool operator<(const QTableWidgetItem &other) const override {
        return data(Qt::UserRole).toLongLong() < other.data(Qt::UserRole).toLongLong();
    }
};

qint64 MainWindow::parsePacmanSizeToBytes(const QString &sizeStr) {
    // Parses pacman -Qi "Installed Size" values like "6.51 MiB" (LC_ALL=C output)
    QStringList parts = sizeStr.trimmed().split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty()) return 0;

    double value = parts[0].toDouble();
    QString unit = (parts.size() > 1) ? parts[1] : "B";

    double multiplier = 1.0;
    if (unit == "KiB") multiplier = 1024.0;
    else if (unit == "MiB") multiplier = 1024.0 * 1024.0;
    else if (unit == "GiB") multiplier = 1024.0 * 1024.0 * 1024.0;
    else if (unit == "TiB") multiplier = 1024.0 * 1024.0 * 1024.0 * 1024.0;

    return static_cast<qint64>(value * multiplier);
}

void MainWindow::refreshUninstallList() {
    ui->uninstallCountLabel->setText("Loading installed software...");
    QApplication::processEvents();

    // Force C locale so sizes ("MiB") and field names are stable for parsing
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("LC_ALL", "C");

    // Foreign packages (installed from AUR or manually, not in sync repos)
    QSet<QString> foreignPackages;
    QProcess qmProc;
    qmProc.setProcessEnvironment(env);
    qmProc.start("pacman", QStringList() << "-Qm");
    qmProc.waitForFinished(30000);
    if (qmProc.exitCode() == 0) {
        const QStringList qmLines = QString::fromUtf8(qmProc.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
        for (const QString &line : qmLines) {
            QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            if (!parts.isEmpty()) foreignPackages.insert(parts[0]);
        }
    }

    // Full details for every installed package in a single call
    QProcess proc;
    proc.setProcessEnvironment(env);
    proc.start("pacman", QStringList() << "-Qi");
    proc.waitForFinished(120000);

    if (proc.exitCode() != 0) {
        ui->uninstallCountLabel->setText("❌ Failed to read installed packages.");
        return;
    }

    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    QStringList lines = output.split('\n');

    ui->uninstallTable->setSortingEnabled(false);
    ui->uninstallTable->setRowCount(0);
    ui->uninstallTable->setColumnCount(5);
    QStringList headers;
    headers << "Package Name" << "Version" << "Size" << "Source" << "Description";
    ui->uninstallTable->setHorizontalHeaderLabels(headers);
    ui->uninstallTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->uninstallTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->uninstallTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->uninstallTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    ui->uninstallTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);

    QString name, version, description, sizeStr;
    qint64 totalBytes = 0;
    int packageCount = 0;

    auto addRow = [&]() {
        if (name.isEmpty()) return;

        qint64 bytes = parsePacmanSizeToBytes(sizeStr);
        totalBytes += bytes;
        packageCount++;

        int row = ui->uninstallTable->rowCount();
        ui->uninstallTable->insertRow(row);
        ui->uninstallTable->setItem(row, 0, new QTableWidgetItem(name));
        ui->uninstallTable->setItem(row, 1, new QTableWidgetItem(version));

        NumericTableWidgetItem *sizeItem = new NumericTableWidgetItem(formatSize(bytes));
        sizeItem->setData(Qt::UserRole, bytes);
        ui->uninstallTable->setItem(row, 2, sizeItem);

        ui->uninstallTable->setItem(row, 3, new QTableWidgetItem(
            foreignPackages.contains(name) ? "AUR / Foreign" : "Official Repo"));
        ui->uninstallTable->setItem(row, 4, new QTableWidgetItem(description));

        name.clear(); version.clear(); description.clear(); sizeStr.clear();
    };

    for (const QString &line : lines) {
        if (line.trimmed().isEmpty()) {
            addRow();
            continue;
        }
        int colonPos = line.indexOf(':');
        if (colonPos <= 0) continue;

        QString key = line.left(colonPos).trimmed();
        QString value = line.mid(colonPos + 1).trimmed();

        if (key == "Name") name = value;
        else if (key == "Version") version = value;
        else if (key == "Description") description = value;
        else if (key == "Installed Size") sizeStr = value;
    }
    addRow(); // last package if output didn't end with a blank line

    ui->uninstallTable->setSortingEnabled(true);
    ui->uninstallTable->sortByColumn(2, Qt::DescendingOrder);

    ui->uninstallCountLabel->setText(QString("✅ %1 packages installed — total %2")
                                     .arg(packageCount).arg(formatSize(totalBytes)));

    filterUninstallTable();
    updateUninstallSelectionInfo();
}

void MainWindow::filterUninstallTable() {
    QString searchText = ui->uninstallSearch->text();
    int sourceFilter = ui->uninstallSourceFilter->currentIndex(); // 0=All, 1=Official, 2=AUR/Foreign

    for (int row = 0; row < ui->uninstallTable->rowCount(); ++row) {
        bool visible = true;

        if (sourceFilter != 0) {
            QTableWidgetItem *sourceItem = ui->uninstallTable->item(row, 3);
            bool isForeign = sourceItem && sourceItem->text().startsWith("AUR");
            if (sourceFilter == 1 && isForeign) visible = false;
            if (sourceFilter == 2 && !isForeign) visible = false;
        }

        if (visible && !searchText.isEmpty()) {
            QTableWidgetItem *nameItem = ui->uninstallTable->item(row, 0);
            QTableWidgetItem *descItem = ui->uninstallTable->item(row, 4);
            bool match = (nameItem && nameItem->text().contains(searchText, Qt::CaseInsensitive)) ||
                         (descItem && descItem->text().contains(searchText, Qt::CaseInsensitive));
            if (!match) visible = false;
        }

        ui->uninstallTable->setRowHidden(row, !visible);
    }
}

void MainWindow::updateUninstallSelectionInfo() {
    QModelIndexList selectedRows = ui->uninstallTable->selectionModel()->selectedRows();

    qint64 totalBytes = 0;
    for (const QModelIndex &index : selectedRows) {
        QTableWidgetItem *sizeItem = ui->uninstallTable->item(index.row(), 2);
        if (sizeItem) totalBytes += sizeItem->data(Qt::UserRole).toLongLong();
    }

    if (selectedRows.isEmpty()) {
        ui->uninstallSelectedSizeLabel->setText("Selected: 0 packages");
    } else {
        ui->uninstallSelectedSizeLabel->setText(QString("Selected: %1 packages (%2)")
                                                .arg(selectedRows.size()).arg(formatSize(totalBytes)));
    }
}

void MainWindow::on_uninstallRefreshButton_clicked() {
    refreshUninstallList();
}

void MainWindow::on_uninstallSearch_textChanged(const QString &text) {
    Q_UNUSED(text);
    filterUninstallTable();
}

void MainWindow::on_uninstallSourceFilter_currentIndexChanged(int index) {
    Q_UNUSED(index);
    filterUninstallTable();
}

void MainWindow::on_uninstallButton_clicked() {
    QModelIndexList selectedRows = ui->uninstallTable->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select one or more packages to uninstall.");
        return;
    }

    QStringList packages;
    qint64 totalBytes = 0;
    for (const QModelIndex &index : selectedRows) {
        QTableWidgetItem *nameItem = ui->uninstallTable->item(index.row(), 0);
        QTableWidgetItem *sizeItem = ui->uninstallTable->item(index.row(), 2);
        if (nameItem) packages << nameItem->text();
        if (sizeItem) totalBytes += sizeItem->data(Qt::UserRole).toLongLong();
    }

    // Pick removal tool. Removal always works through pacman even for AUR
    // packages, so Auto uses pacman; yay/paru are offered for users who prefer them.
    int helperIndex = ui->uninstallHelperCombo->currentIndex(); // 0=Auto, 1=Pacman, 2=Yay, 3=Paru
    QString helper = "pacman";
    if (helperIndex == 2) helper = "yay";
    else if (helperIndex == 3) helper = "paru";

    if (helper != "pacman") {
        QProcess checkProc;
        checkProc.start("which", QStringList() << helper);
        checkProc.waitForFinished();
        if (checkProc.exitCode() != 0) {
            QMessageBox::critical(this, "Error",
                                  QString("%1 is not installed. Install it from the Package Manager tab, "
                                          "or choose a different removal tool.").arg(helper));
            return;
        }
    }

    QString flags = ui->uninstallDepsCheckBox->isChecked() ? "-Rns" : "-R";

    QString packageListText = packages.mid(0, 20).join("\n  ");
    if (packages.size() > 20) {
        packageListText += QString("\n  ... and %1 more").arg(packages.size() - 20);
    }

    QMessageBox::StandardButton reply = QMessageBox::question(this, "Uninstall Packages",
        QString("Uninstall %1 package(s) freeing about %2?\n\n  %3\n\n"
                "This will run '%4 %5' in a terminal so you can review what gets removed.")
            .arg(packages.size()).arg(formatSize(totalBytes)).arg(packageListText)
            .arg(helper == "pacman" ? "sudo pacman" : helper).arg(flags),
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }

    QString command;
    if (helper == "pacman") {
        command = QString("sudo pacman %1 %2").arg(flags, packages.join(' '));
    } else {
        command = QString("%1 %2 %3").arg(helper, flags, packages.join(' '));
    }

    // The terminal runs detached, so completion is signaled through a marker
    // file touched right after the package command finishes (before the
    // "Press Enter" prompt). A poll timer picks it up and refreshes the list.
    static int uninstallRunId = 0;
    QString flagFile = QDir::temp().filePath(QString("cachyostools-uninstall-done-%1-%2")
                                             .arg(QApplication::applicationPid()).arg(++uninstallRunId));
    QFile::remove(flagFile);

    runSudoCommandInTerminal(command + QString("; touch '%1'; read -p 'Press Enter to close...'").arg(flagFile));

    ui->uninstallCountLabel->setText("Uninstall running in terminal — the list will refresh automatically when it finishes.");

    QTimer *watcher = new QTimer(this);
    watcher->setInterval(1000);
    connect(watcher, &QTimer::timeout, this, [this, watcher, flagFile]() {
        if (QFile::exists(flagFile)) {
            QFile::remove(flagFile);
            watcher->stop();
            watcher->deleteLater();
            refreshUninstallList();
            return;
        }
        // Stop watching after 30 minutes (terminal closed without running, etc.)
        int elapsed = watcher->property("elapsedSecs").toInt() + 1;
        if (elapsed >= 1800) {
            watcher->stop();
            watcher->deleteLater();
            return;
        }
        watcher->setProperty("elapsedSecs", elapsed);
    });
    watcher->start();
}
