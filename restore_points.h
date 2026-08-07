
// Dashboard -> Restore Points sub-tab: btrfs snapshots via snapper —
// the Linux answer to Windows System Restore. Reads need sudo, so the
// list only loads on explicit request (uses the cached sudo session).

void MainWindow::setupRestorePointsTab() {
    QVBoxLayout *layout = new QVBoxLayout(ui->restorePointsSubTab);

    QHBoxLayout *bar = new QHBoxLayout();
    rpStatusLabel = new QLabel(tr("Snapshots protect you from bad updates — create one before big changes."), ui->restorePointsSubTab);
    rpStatusLabel->setStyleSheet("color:#888;");
    QPushButton *loadBtn = new QPushButton(tr("🔓 Load Snapshots (sudo)"), ui->restorePointsSubTab);
    QPushButton *createBtn = new QPushButton(tr("📸 Create Restore Point"), ui->restorePointsSubTab);
    QPushButton *deleteBtn = new QPushButton(tr("🗑️ Delete"), ui->restorePointsSubTab);
    QPushButton *rollbackBtn = new QPushButton(tr("⏪ How to Roll Back"), ui->restorePointsSubTab);
    bar->addWidget(rpStatusLabel);
    bar->addStretch();
    for (QPushButton *b : {loadBtn, createBtn, deleteBtn, rollbackBtn}) bar->addWidget(b);
    layout->addLayout(bar);

    rpTable = new QTableWidget(ui->restorePointsSubTab);
    rpTable->setColumnCount(4);
    rpTable->setHorizontalHeaderLabels(QStringList() << tr("#") << tr("Type") << tr("Date") << tr("Description"));
    rpTable->horizontalHeader()->setStretchLastSection(true);
    rpTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    rpTable->setSelectionMode(QAbstractItemView::SingleSelection);
    rpTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    rpTable->setAlternatingRowColors(true);
    rpTable->verticalHeader()->setVisible(false);
    layout->addWidget(rpTable);

    connect(loadBtn, &QPushButton::clicked, this, &MainWindow::refreshRestorePoints);

    connect(createBtn, &QPushButton::clicked, this, [this]() {
        bool ok;
        QString desc = QInputDialog::getText(this, tr("Create Restore Point"), tr("Description:"),
                                             QLineEdit::Normal, tr("Manual restore point"), &ok);
        if (!ok) return;
        desc.replace('\'', ' ');
        runSudoCommandInTerminal(QString("sudo snapper -c root create -d '%1' && echo 'Restore point created.'; read -p 'Press Enter...'").arg(desc));
    });

    connect(deleteBtn, &QPushButton::clicked, this, [this]() {
        int row = rpTable->currentRow();
        if (row < 0 || !rpTable->item(row, 0)) {
            QMessageBox::information(this, tr("No Selection"), tr("Load and select a snapshot first."));
            return;
        }
        QString num = rpTable->item(row, 0)->text();
        if (num == "0") { QMessageBox::warning(this, tr("Refused"), tr("Snapshot 0 is the live system itself.")); return; }
        if (QMessageBox::question(this, tr("Delete Snapshot"),
                tr("Delete snapshot #%1? The disk space is reclaimed.").arg(num)) != QMessageBox::Yes) return;
        runSudoCommandInTerminal(QString("sudo snapper -c root delete %1 && echo 'Deleted snapshot %1.'; read -p 'Press Enter...'").arg(num));
    });

    connect(rollbackBtn, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this, tr("Rolling Back"),
            tr("Rolling back is deliberately not a one-click action — it replaces your running system.\n\n"
               "The safe ways:\n\n"
               "1. If the system still boots:\n"
               "     sudo snapper -c root rollback <number>\n"
               "     then reboot.\n\n"
               "2. If the system does not boot and grub-btrfs is installed:\n"
               "     pick the snapshot from the GRUB menu ('Arch snapshots'),\n"
               "     boot into it, then run the rollback command above.\n\n"
               "3. Nuclear option: boot your Safety ISO from the System ISO tab.\n\n"
               "Files-only recovery: snapshots are browsable at /.snapshots/<number>/snapshot/ —\n"
               "copy individual files back without rolling back anything."));
    });
}

void MainWindow::refreshRestorePoints() {
    // snapper needs root even to list; reuse the app's cached-sudo pattern
    QProcess check;
    check.start("bash", QStringList() << "-c" << "command -v snapper");
    check.waitForFinished(2000);
    if (check.exitCode() != 0) {
        rpStatusLabel->setText(tr("snapper is not installed — snapshots need a btrfs filesystem + snapper."));
        rpStatusLabel->setStyleSheet("color:#e67e22; font-weight:bold;");
        QProcess fs;
        fs.start("findmnt", QStringList() << "-no" << "FSTYPE" << "/");
        fs.waitForFinished(2000);
        QString fstype = QString::fromUtf8(fs.readAllStandardOutput()).trimmed();
        QString msg = (fstype == "btrfs")
            ? tr("Your root filesystem IS btrfs — you just need snapper:\n\n"
                 "  sudo pacman -S snapper snap-pac grub-btrfs\n"
                 "  sudo snapper -c root create-config /\n\n"
                 "snap-pac then creates automatic before/after snapshots on every pacman operation.")
            : tr("Your root filesystem is '%1', not btrfs, so snapper snapshots are unavailable.\n\n"
                 "Your alternative safety nets: the System ISO tab (full clone) and the Backup tab.").arg(fstype);
        QMessageBox::information(this, tr("Snapshots Not Set Up"), msg);
        return;
    }

    if (!authenticateSudo()) return;

    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, proc](int exitCode, QProcess::ExitStatus) {
                rpTable->setRowCount(0);
                if (exitCode != 0) {
                    rpStatusLabel->setText(tr("snapper has no 'root' config yet — run: sudo snapper -c root create-config /"));
                    rpStatusLabel->setStyleSheet("color:#e67e22; font-weight:bold;");
                    proc->deleteLater();
                    return;
                }
                // CSV columns: number,type,date,description
                const QStringList lines = QString::fromUtf8(proc->readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
                for (const QString &line : lines) {
                    const QStringList f = line.split(',');
                    if (f.size() < 4 || f[0] == "number") continue;
                    int row = rpTable->rowCount();
                    rpTable->insertRow(row);
                    rpTable->setItem(row, 0, new QTableWidgetItem(f[0]));
                    rpTable->setItem(row, 1, new QTableWidgetItem(f[1]));
                    rpTable->setItem(row, 2, new QTableWidgetItem(f[2]));
                    rpTable->setItem(row, 3, new QTableWidgetItem(f.mid(3).join(',')));
                }
                rpTable->resizeColumnsToContents();
                rpStatusLabel->setText(tr("%1 snapshot(s). Automatic ones come from snap-pac; # 0 is the live system.").arg(rpTable->rowCount()));
                rpStatusLabel->setStyleSheet("color:#888;");
                proc->deleteLater();
            });
    proc->start("bash", QStringList() << "-c"
        << "sudo -n snapper --machine-readable csv -c root list --columns number,type,date,description 2>/dev/null "
           "|| sudo -n snapper -c root list --columns number,type,date,description | tail -n +3 | sed 's/ *| */,/g'");
}
