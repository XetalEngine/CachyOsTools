
void MainWindow::on_startRestoreButton_clicked()
{
    QString src = getSelectedRestoreSourcePath();
    QString dst = getSelectedRestoreDestPath();
    
    if (src.isEmpty() || dst.isEmpty()) {
        QMessageBox::warning(this, "Invalid Selection", "Please select both source backup and destination.");
        return;
    }
    
    // Check if this looks like a system restore (destination is /)
    bool isSystemRestore = (dst == "/");
    
    if (isSystemRestore) {
        QMessageBox::StandardButton reply = QMessageBox::question(this, "System Restore", 
            "This will restore your system backup to the root filesystem.\n\n"
            "⚠️  WARNING: This will overwrite the current system!\n\n"
            "Make sure you:\n"
            "1. Have a working live environment ready\n"
            "2. Have backed up any important data\n"
            "3. Are running this from a live environment\n\n"
            "Continue with system restore?",
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply != QMessageBox::Yes) {
            return;
        }
    }
    
    // Build the restore command
    QStringList excludes = {
        "--exclude=/dev/*",
        "--exclude=/proc/*",
        "--exclude=/sys/*",
        "--exclude=/run/*",
        "--exclude=/tmp/*",
        "--exclude=/mnt/*",
        "--exclude=/media/*",
        "--exclude=/lost+found",
        "--exclude=/swapfile",
        "--exclude=.local/share/Steam/steamapps/*"
    };
    
    QStringList args = {"-aAXv"};
    args << excludes << src << dst;
    
    QString cmd = "sudo rsync ";
    for (const QString &arg : args) {
        cmd += "'" + QString(arg).replace("'", "'\\''") + "' ";
    }
    
    // Add post-restore tasks for system restore
    if (isSystemRestore) {
        cmd += " && echo 'Restore completed. Running post-restore tasks...'";
        cmd += " && echo 'Updating fstab with current UUIDs...'";
        cmd += " && sudo blkid | grep -E '^/dev/[^:]+:' | while read line; do";
        cmd += "   device=$(echo \"$line\" | cut -d: -f1);";
        cmd += "   uuid=$(echo \"$line\" | grep -o 'UUID=\"[^\"]*\"' | cut -d'\"' -f2);";
        cmd += "   if [ ! -z \"$uuid\" ]; then";
        cmd += "     sudo sed -i \"s|UUID=[^ ]*|UUID=$uuid|g\" /etc/fstab;";
        cmd += "   fi;";
        cmd += " done";
        cmd += " && echo 'Regenerating initramfs...'";
        cmd += " && sudo mkinitcpio -P";
        cmd += " && echo 'Updating bootloader...'";
        cmd += " && sudo grub-mkconfig -o /boot/grub/grub.cfg";
        cmd += " && echo 'System restore completed successfully!'";
        cmd += " && echo 'You may need to reboot and check for hardware-specific drivers.'";
    }
    
    logMessage("🔄 Starting system restore...");
    logMessage("⏳ Waiting for sudo password in terminal...");
    // Launch in terminal
    QStringList terminals = {"konsole", "gnome-terminal", "xterm", "alacritty", "kitty"};
    bool launched = false;
    for (const QString &terminal : terminals) {
        if (QProcess::startDetached(terminal, QStringList() << "-e" << "bash" << "-c" << cmd)) {
            launched = true;
            break;
        }
    }
    
    if (launched) {
        QString message = "Restore started in a terminal window. Please enter your password if prompted.";
        if (isSystemRestore) {
            message += "\n\nThis is a system restore. The terminal will show progress and run post-restore tasks.";
        }
        ui->restoreSummaryLabel->setText(message);
    } else {
        ui->restoreSummaryLabel->setText("Could not launch a terminal emulator. Please install konsole, gnome-terminal, xterm, alacritty, or kitty.");
    }
}



// Restore tab functions
void MainWindow::populateRestoreDrives() {
    ui->restoreSourceDriveCombo->clear();
    ui->restoreDestDriveCombo->clear();
    QList<QPair<QString, QString>> drives; // (mountPoint, label)
    QProcess proc;
    proc.start("lsblk", QStringList() << "-o" << "MOUNTPOINT,LABEL" << "-nr");
    proc.waitForFinished();
    QString output = proc.readAllStandardOutput();
    for (const QString &line : output.split('\n')) {
        if (line.trimmed().isEmpty()) continue;
        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.isEmpty()) continue;
        QString mount = parts[0];
        QString label = (parts.size() > 1) ? parts[1] : mount;
        if (mount.startsWith("/mnt") || mount.startsWith("/media") || mount.isEmpty()) continue;
        drives.append(qMakePair(mount, label));
    }
    for (const auto &d : drives) {
        ui->restoreSourceDriveCombo->addItem(d.first + " (" + d.second + ")", d.first);
        ui->restoreDestDriveCombo->addItem(d.first + " (" + d.second + ")", d.first);
    }
    updateRestoreSourcePathTree();
    updateRestoreDestPathTree();
}

void MainWindow::updateRestoreSourcePathTree() {
    if (restoreSourceFsModel) { delete restoreSourceFsModel; restoreSourceFsModel = nullptr; }
    int idx = ui->restoreSourceDriveCombo->currentIndex();
    if (idx < 0) return;
    restoreSourceMountPoint = ui->restoreSourceDriveCombo->itemData(idx).toString();
    restoreSourceFsModel = new QFileSystemModel(this);
    restoreSourceFsModel->setRootPath(restoreSourceMountPoint);
    restoreSourceFsModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Files);
    ui->restoreSourcePathTree->setModel(restoreSourceFsModel);
    ui->restoreSourcePathTree->setRootIndex(restoreSourceFsModel->index(restoreSourceMountPoint));
    ui->restoreSourcePathTree->setColumnHidden(1, true);
    ui->restoreSourcePathTree->setColumnHidden(2, true);
    ui->restoreSourcePathTree->setColumnHidden(3, true);
}

void MainWindow::updateRestoreDestPathTree() {
    if (restoreDestFsModel) { delete restoreDestFsModel; restoreDestFsModel = nullptr; }
    int idx = ui->restoreDestDriveCombo->currentIndex();
    if (idx < 0) return;
    restoreDestMountPoint = ui->restoreDestDriveCombo->itemData(idx).toString();
    restoreDestFsModel = new QFileSystemModel(this);
    restoreDestFsModel->setRootPath(restoreDestMountPoint);
    restoreDestFsModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
    ui->restoreDestPathTree->setModel(restoreDestFsModel);
    ui->restoreDestPathTree->setRootIndex(restoreDestFsModel->index(restoreDestMountPoint));
    ui->restoreDestPathTree->setColumnHidden(1, true);
    ui->restoreDestPathTree->setColumnHidden(2, true);
    ui->restoreDestPathTree->setColumnHidden(3, true);
}

QString MainWindow::getSelectedRestoreSourcePath() {
    QModelIndex idx = ui->restoreSourcePathTree->currentIndex();
    if (!idx.isValid()) return restoreSourceMountPoint;
    return restoreSourceFsModel->filePath(idx);
}

QString MainWindow::getSelectedRestoreDestPath() {
    QModelIndex idx = ui->restoreDestPathTree->currentIndex();
    if (!idx.isValid()) return restoreDestMountPoint;
    return restoreDestFsModel->filePath(idx);
}

bool MainWindow::validateRestoreSelection() {
    QString src = getSelectedRestoreSourcePath();
    QString dst = getSelectedRestoreDestPath();
    if (src.isEmpty() || dst.isEmpty()) return false;
    if (src == dst) return false;
    if (src.startsWith("/mnt") || src.startsWith("/media")) return false;
    if (dst.startsWith("/mnt") || dst.startsWith("/media")) return false;
    if (dst.startsWith(src)) return false;
    if (src.startsWith(dst)) return false;
    if (ui->restoreSourceDriveCombo->currentIndex() == ui->restoreDestDriveCombo->currentIndex()) return false;
    return true;
}

void MainWindow::updateRestoreSummary() {
    QString src = getSelectedRestoreSourcePath();
    QString dst = getSelectedRestoreDestPath();
    ui->restorePathSummaryLabel->setText("Source: " + src + "\nDestination: " + dst);
    if (!validateRestoreSelection()) {
        ui->restoreSummaryLabel->setText("Please select valid source and destination (must be different drives, not /mnt or /media, and not subfolders of each other).");
        ui->startRestoreButton->setEnabled(false);
        return;
    }
    ui->restoreSummaryLabel->setText("Restore from: " + src + "\nTo: " + dst);
    ui->startRestoreButton->setEnabled(true);
}

void MainWindow::on_restoreSourceDriveCombo_currentIndexChanged(int) { 
    updateRestoreSourcePathTree(); 
    updateRestoreSummary(); 
}

void MainWindow::on_restoreDestDriveCombo_currentIndexChanged(int) { 
    updateRestoreDestPathTree(); 
    updateRestoreSummary(); 
}

void MainWindow::on_restoreSourcePathTree_clicked(const QModelIndex &) { 
    updateRestoreSummary(); 
}

void MainWindow::on_restoreDestPathTree_clicked(const QModelIndex &) { 
    updateRestoreSummary(); 
}
