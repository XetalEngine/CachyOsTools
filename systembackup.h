
// Backup tab slots
void MainWindow::on_destDriveCombo_currentIndexChanged(int) { updateDestPathTree(); updateBackupSummary(); }
void MainWindow::on_sourcePathTree_clicked(const QModelIndex &) { updateBackupSummary(); }
void MainWindow::on_destPathTree_clicked(const QModelIndex &) { updateBackupSummary(); }
QString MainWindow::getSelectedSourcePath() {
    QModelIndex idx = ui->sourcePathTree->currentIndex();
    if (!idx.isValid()) {
        // If nothing is selected, default to "/" for system backup
        return "/";
    }
    QString path = sourceFsModel->filePath(idx);
    // If the selected path is empty or invalid, default to "/"
    if (path.isEmpty()) {
        return "/";
    }
    return path;
}
QString MainWindow::getSelectedDestPath() {
    QModelIndex idx = ui->destPathTree->currentIndex();
    if (!idx.isValid()) return destMountPoint;
    return destFsModel->filePath(idx);
}
bool MainWindow::validateBackupSelection() {
    QString src = getSelectedSourcePath();
    QString dst = getSelectedDestPath();
    
    // Debug output
    qDebug() << "Validating backup selection:";
    qDebug() << "  Source:" << src;
    qDebug() << "  Destination:" << dst;
    
    if (src.isEmpty() || dst.isEmpty()) {
        qDebug() << "  Validation failed: Empty source or destination";
        return false;
    }
    if (src == dst) {
        qDebug() << "  Validation failed: Source equals destination";
        return false;
    }
    if (src.startsWith("/mnt") || src.startsWith("/media")) {
        qDebug() << "  Validation failed: Source is in /mnt or /media";
        return false;
    }
    if (dst.startsWith("/mnt") || dst.startsWith("/media")) {
        qDebug() << "  Validation failed: Destination is in /mnt or /media";
        return false;
    }
    // For system backup, we need to be more careful about subdirectory checks
    if (src == "/") {
        // For system backup, only prevent backing up to a directory that's actually on the same filesystem
        // Check if destination is a direct subdirectory of source (not just starts with "/")
        if (dst.startsWith("/") && dst != "/") {
            // Allow backing up to mounted drives or other filesystems
            // Only prevent if the destination is actually a subdirectory of the source filesystem
            QString relativePath = dst.mid(1); // Remove leading "/"
            if (!relativePath.contains("/")) {
                // This is a direct subdirectory like "/home", "/etc", etc.
                qDebug() << "  Validation failed: Cannot backup to a subdirectory of the source filesystem";
                return false;
            }
        }
        qDebug() << "  System backup validation: PASSED";
        return true;
    }
    
    // For non-system backups, use the original subdirectory checks
    if (dst.startsWith(src)) {
        qDebug() << "  Validation failed: Destination is subdirectory of source";
        return false;
    }
    if (src.startsWith(dst)) {
        qDebug() << "  Validation failed: Source is subdirectory of destination";
        return false;
    }
    
    qDebug() << "  Validation PASSED";
    return true;
}
void MainWindow::updateBackupSummary() {
    QString src = getSelectedSourcePath();
    QString dst = getSelectedDestPath();
    // Update the new summary label at the bottom
    ui->backupPathSummaryLabel->setText("Source: " + src + "\nDestination: " + dst);
    
    bool isValid = validateBackupSelection();
    if (!isValid) {
        ui->backupSummaryLabel->setText("Please select a valid destination. The source is automatically set to '/' for system backup.");
        ui->startBackupButton->setEnabled(false);
        return;
    }
    
    ui->backupSummaryLabel->setText("System backup from: " + src + "\nTo: " + dst);
    ui->startBackupButton->setEnabled(true);
}
void MainWindow::on_startBackupButton_clicked() {
    if (!validateBackupSelection()) return;
    
    // Prevent multiple launches
    if (backupInProgress) {
        //QMessageBox::information(this, "Backup in Progress", "A backup is already in progress. Please wait for it to complete.");
        return;
    }
    
    QString src = getSelectedSourcePath();
    QString dst = getSelectedDestPath();
    
    // Check if this is a system backup (backing up /)
    bool isSystemBackup = (src == "/");
    
    // Compose rsync command with all recommended excludes
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
    
    // Build the full command string
    QString cmd = "sudo rsync ";
    for (const QString &arg : args) {
        cmd += "'" + QString(arg).replace("'", "'\\''") + "' ";
    }
    
    // If this is a system backup, add a note about restore process
    if (isSystemBackup) {
        cmd += " && echo 'System backup completed. To restore: 1) Install Arch on target, 2) Boot live environment, 3) Mount target system, 4) Run restore command.'";
    }
    
    logMessage("💾 Starting system backup...");
    logMessage("⏳ Waiting for sudo password in terminal...");
    // Launch in a terminal for password entry
    QStringList terminals = {"konsole", "gnome-terminal", "xterm", "alacritty", "kitty"};
    bool launched = false;
    
    // Try to find and launch a terminal
    for (const QString &terminal : terminals) {
        // Check if terminal exists first
        QProcess checkProc;
        checkProc.start("which", QStringList() << terminal);
        checkProc.waitForFinished();
        if (checkProc.exitCode() == 0) {
            // Terminal exists, try to launch it
            if (QProcess::startDetached(terminal, QStringList() << "-e" << "bash" << "-c" << cmd)) {
                launched = true;
                backupInProgress = true;
                break;
            }
        }
    }
    
    if (launched) {
        QString message = "Backup started in a terminal window. Please enter your password if prompted and monitor progress there.";
        if (isSystemBackup) {
            message += "\n\nThis is a system backup. For restore instructions, see the terminal output.";
        }
        ui->backupSummaryLabel->setText(message);
        
        // Reset the flag after a delay
        QTimer::singleShot(5000, this, [this]() {
            backupInProgress = false;
        });
    } else {
        ui->backupSummaryLabel->setText("Could not launch a terminal emulator. Please install konsole, gnome-terminal, xterm, alacritty, or kitty.");
    }
    ui->backupProgressBar->setValue(0);
}
void MainWindow::handleBackupProgress() {
    // Simple progress: count lines in stderr (rsync progress)
    QByteArray data = backupProcess->readAllStandardError();
    int lines = data.count('\n');
    int val = ui->backupProgressBar->value() + lines;
    ui->backupProgressBar->setValue(qMin(val, 100));
}
void MainWindow::handleBackupFinished(int exitCode, QProcess::ExitStatus) {
    if (exitCode == 0) {
        ui->backupSummaryLabel->setText("Backup completed successfully.");
        ui->backupProgressBar->setValue(100);
    } else {
        ui->backupSummaryLabel->setText("Backup failed. Please check destination and permissions.");
    }
}

// Helper: List all mounted drives except /mnt and /media
void MainWindow::populateBackupDrives() {
    ui->destDriveCombo->clear();
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
        // Only include external mounted drives (not root filesystem, not system mounts)
        if (!mount.isEmpty() && mount != "/" && 
            (mount.startsWith("/run/media") || mount.startsWith("/mnt") || mount.startsWith("/media"))) {
            drives.append(qMakePair(mount, label));
        }
    }
    for (const auto &d : drives) {
        ui->destDriveCombo->addItem(d.first + " (" + d.second + ")", d.first);
    }
    updateSourcePathTree();
    updateDestPathTree();
}
// Update source path tree
void MainWindow::updateSourcePathTree() {
    if (sourceFsModel) { delete sourceFsModel; sourceFsModel = nullptr; }
    sourceMountPoint = "/";
    sourceFsModel = new QFileSystemModel(this);
    sourceFsModel->setRootPath("/");
    sourceFsModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Files);
    ui->sourcePathTree->setModel(sourceFsModel);
    // Set root index to show "/" as a selectable item
    ui->sourcePathTree->setRootIndex(sourceFsModel->index(""));
    ui->sourcePathTree->setColumnHidden(1, true);
    ui->sourcePathTree->setColumnHidden(2, true);
    ui->sourcePathTree->setColumnHidden(3, true);
    
    // Automatically select "/" as the default source
    QModelIndex rootIndex = sourceFsModel->index("/");
    if (rootIndex.isValid()) {
        ui->sourcePathTree->setCurrentIndex(rootIndex);
    }
}
// Update dest path tree
void MainWindow::updateDestPathTree() {
    if (destFsModel) { delete destFsModel; destFsModel = nullptr; }
    int idx = ui->destDriveCombo->currentIndex();
    if (idx < 0) return;
    destMountPoint = ui->destDriveCombo->itemData(idx).toString();
    destFsModel = new QFileSystemModel(this);
    destFsModel->setRootPath(destMountPoint);
    destFsModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
    ui->destPathTree->setModel(destFsModel);
    ui->destPathTree->setRootIndex(destFsModel->index(destMountPoint));
    ui->destPathTree->setColumnHidden(1, true);
    ui->destPathTree->setColumnHidden(2, true);
    ui->destPathTree->setColumnHidden(3, true);
}



// Backup folder functions
void MainWindow::on_openBackupFolderButton_clicked()
{
    QString backupDir = QDir::homePath() + "/configbackups";
    QDir().mkpath(backupDir);
    
    // Try to open with dolphin, fallback to other file managers
    QStringList fileManagers = {"dolphin", "nautilus", "thunar", "pcmanfm", "caja"};
    bool launched = false;
    
    for (const QString &manager : fileManagers) {
        if (QProcess::startDetached(manager, QStringList() << backupDir)) {
            launched = true;
            break;
        }
    }
    
    if (!launched) {
        QMessageBox::information(this, "Backup Folder", 
                               QString("Backup folder created at: %1\n\nPlease open it manually with your file manager.").arg(backupDir));
    }
}

void MainWindow::backupConfigFile(const QString &filePath, const QString &description)
{
    QString backupDir = QDir::homePath() + "/configbackups";
    QDir().mkpath(backupDir);
    
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        QMessageBox::warning(this, "File Not Found", 
                           QString("The file %1 does not exist.").arg(filePath));
        return;
    }
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    QString backupFileName = QString("%1_%2.backup").arg(fileInfo.fileName(), timestamp);
    QString backupPath = backupDir + "/" + backupFileName;
    
    if (QFile::copy(filePath, backupPath)) {
        QMessageBox::information(this, "Backup Created", 
                               QString("Successfully backed up %1 to:\n%2").arg(description, backupPath));
    } else {
        QMessageBox::critical(this, "Backup Failed", 
                            QString("Failed to backup %1. You may need to run with sudo privileges.").arg(description));
    }
}

void MainWindow::on_zramBackupButton_clicked()
{
    backupConfigFile("/etc/systemd/zram-generator.conf", "ZRAM configuration");
}

void MainWindow::on_cpuGovernorBackupButton_clicked()
{
    backupConfigFile("/etc/default/cpupower", "CPU Governor configuration");
}

void MainWindow::on_ipv6BackupButton_clicked()
{
    backupConfigFile("/etc/sysctl.d/40-ipv6.conf", "IPv6 configuration");
}

void MainWindow::on_trimBackupButton_clicked()
{
    backupConfigFile("/etc/fstab", "fstab (for TRIM configuration)");
}

void MainWindow::on_tmpfsBackupButton_clicked()
{
    backupConfigFile("/etc/fstab", "fstab (for tmpfs configuration)");
}

void MainWindow::on_dnsBackupButton_clicked()
{
    backupConfigFile("/etc/resolv.conf", "DNS configuration");
}

void MainWindow::on_showHiddenFilesBackupButton_clicked()
{
    backupConfigFile(QDir::homePath() + "/.config/dolphinrc", "Dolphin configuration");
}

void MainWindow::on_mitigationsBackupButton_clicked()
{
    backupConfigFile("/etc/default/grub", "GRUB configuration");
}

void MainWindow::on_performanceHacksBackupButton1_clicked()
{
    backupConfigFile("/etc/updatedb.conf", "updatedb configuration");
}

void MainWindow::on_performanceHacksBackupButton2_clicked()
{
    backupConfigFile("/etc/mkinitcpio.conf", "mkinitcpio configuration");
}

void MainWindow::on_ptraceBackupButton_clicked()
{
    backupConfigFile("/etc/sysctl.d/10-ptrace.conf", "ptrace configuration");
}

void MainWindow::on_hidepidBackupButton_clicked()
{
    backupConfigFile("/etc/fstab", "fstab (for hidepid configuration)");
}

void MainWindow::on_swappinessBackupButton_clicked()
{
    backupConfigFile("/etc/sysctl.d/99-swappiness.conf", "Swappiness configuration");
}

void MainWindow::on_ioSchedulerBackupButton_clicked()
{
    backupConfigFile("/etc/udev/rules.d/60-ioschedulers.rules", "I/O Scheduler configuration");
}

void MainWindow::on_thpBackupButton_clicked()
{
    backupConfigFile("/etc/sysctl.d/99-thp.conf", "THP configuration");
}

void MainWindow::on_tcpOptimizationsBackupButton_clicked()
{
    backupConfigFile("/etc/sysctl.d/99-tcp-optimizations.conf", "TCP optimizations configuration");
}

void MainWindow::on_firewallBackupButton_clicked()
{
    QMessageBox::information(this, "Firewall Backup", 
        "Firewall rules are stored in:\n"
        "- firewalld: /etc/firewalld/\n"
        "- ufw: /etc/ufw/\n\n"
        "Backup these directories manually if needed.");
}

void MainWindow::on_pacmanOptimizationsBackupButton_clicked()
{
    backupConfigFile("/etc/pacman.conf", "Pacman configuration");
}

void MainWindow::on_journaldBackupButton_clicked()
{
    backupConfigFile("/etc/systemd/journald.conf", "Journald configuration");
}
