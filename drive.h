#ifndef DRIVE_H
#define DRIVE_H

void MainWindow::setupDriveTable()
{
    // Set up table headers
    QStringList headers;
    headers << "Device Name" << "Device Path" << "Size" << "Type" << "Label" << "Mount Point" << "Status";
    ui->drivesTable->setColumnCount(headers.size());
    ui->drivesTable->setHorizontalHeaderLabels(headers);
    
    // Set table properties
    ui->drivesTable->setAlternatingRowColors(true);
    ui->drivesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->drivesTable->setSortingEnabled(true);
    ui->drivesTable->horizontalHeader()->setStretchLastSection(true);
    ui->drivesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    
    // Set column widths
    ui->drivesTable->setColumnWidth(0, 120); // Device Name
    ui->drivesTable->setColumnWidth(1, 150); // Device Path
    ui->drivesTable->setColumnWidth(2, 80);  // Size
    ui->drivesTable->setColumnWidth(3, 80);  // Type
    ui->drivesTable->setColumnWidth(4, 100); // Label
    ui->drivesTable->setColumnWidth(5, 150); // Mount Point
    ui->drivesTable->setColumnWidth(6, 80);  // Status
}

void MainWindow::refreshDrives()
{
    logMessage("🔄 Refreshing drive list...");
    
    // Execute lsblk command to get drive information
    executeCommand("lsblk", QStringList() << "-o" << "NAME,SIZE,TYPE,LABEL,MOUNTPOINT" << "--noheadings" << "--pairs");
}

void MainWindow::on_refreshButton_clicked()
{
    refreshDrives();
}

void MainWindow::on_mountButton_clicked()
{
    QString selectedDrive = getSelectedDrive();
    if (selectedDrive.isEmpty()) {
        QMessageBox::warning(this, "No Drive Selected", "Please select a drive to mount.");
        return;
    }
    logMessage("📁 Mounting " + selectedDrive);
    logMessage("⏳ Waiting for sudo password in terminal...");
    executeCommand("udisksctl", QStringList() << "mount" << "-b" << selectedDrive);
}

void MainWindow::on_forceMountButton_clicked()
{
    QString selectedDrive = getSelectedDrive();
    if (selectedDrive.isEmpty()) {
        QMessageBox::warning(this, "No Drive Selected", "Please select a drive to mount.");
        return;
    }
    logMessage("⚡ Force mounting " + selectedDrive);
    logMessage("⏳ Waiting for sudo password in terminal...");
    executeCommand("udisksctl", QStringList() << "mount" << "-o" << "force" << "-b" << selectedDrive);
}

void MainWindow::on_unmountButton_clicked()
{
    QString selectedDrive = getSelectedDrive();
    if (selectedDrive.isEmpty()) {
        QMessageBox::warning(this, "No Drive Selected", "Please select a drive to unmount.");
        return;
    }
    logMessage("📤 Unmounting " + selectedDrive);
    logMessage("⏳ Waiting for sudo password in terminal...");
    executeCommand("udisksctl", QStringList() << "unmount" << "-b" << selectedDrive);
}








QList<DriveInfo> MainWindow::parseDriveList(const QString &output)
{
    QList<DriveInfo> drives;
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    for (const QString &line : lines) {
        DriveInfo drive;
        QStringList fields = line.split(' ', Qt::SkipEmptyParts);
        QString devName;
        for (const QString &field : fields) {
            int eqIdx = field.indexOf('=');
            if (eqIdx == -1) continue;
            QString key = field.left(eqIdx);
            QString value = field.mid(eqIdx+1).remove('"');
            if (key == "NAME") devName = value;
            else if (key == "SIZE") drive.size = value;
            else if (key == "TYPE") drive.type = value;
            else if (key == "LABEL") drive.label = value;
            else if (key == "MOUNTPOINT") drive.mountPoint = value;
        }
        if (!devName.isEmpty()) {
            drive.device = "/dev/" + devName;
            drive.isMounted = !drive.mountPoint.isEmpty();
            logMessage(QString("Parsed: %1 (%2) [%3] Label: %4 Mount: %5").arg(devName, drive.device, drive.type, drive.label, drive.mountPoint));
            if (drive.type == "disk" || drive.type == "part") {
                drives.append(drive);
            }
        }
    }
    return drives;
}

// Helper function to convert size string to MB
qint64 MainWindow::parseSizeToMB(const QString &sizeStr)
{
    QString size = sizeStr.trimmed();
    qint64 value = 0;
    
    if (size.endsWith("G", Qt::CaseInsensitive)) {
        value = size.left(size.length() - 1).toDouble() * 1024;
    } else if (size.endsWith("M", Qt::CaseInsensitive)) {
        value = size.left(size.length() - 1).toDouble();
    } else if (size.endsWith("K", Qt::CaseInsensitive)) {
        value = size.left(size.length() - 1).toDouble() / 1024;
    } else if (size.endsWith("B", Qt::CaseInsensitive)) {
        value = size.left(size.length() - 1).toDouble() / (1024 * 1024);
    } else {
        // Try to parse as number (assume MB)
        value = size.toDouble();
    }
    
    return value;
}

void MainWindow::populateDriveTable()
{
    ui->drivesTable->setRowCount(0);
    
    bool showDisks = ui->diskFilterCheckBox->isChecked();
    bool showPartitions = ui->partitionFilterCheckBox->isChecked();
    bool useMinSizeFilter = ui->minSizeFilterCheckBox->isChecked();
    qint64 minSizeMB = ui->minSizeSpinBox->value();
    
    for (const DriveInfo &drive : drivesList) {
        // Apply type filters
        bool shouldShow = false;
        if (drive.type == "disk" && showDisks) {
            shouldShow = true;
        } else if (drive.type == "part" && showPartitions) {
            shouldShow = true;
        }
        
        if (!shouldShow) {
            continue;
        }
        
        // Apply size filter
        if (useMinSizeFilter) {
            qint64 driveSizeMB = parseSizeToMB(drive.size);
            if (driveSizeMB < minSizeMB) {
                continue;
            }
        }
        
        int row = ui->drivesTable->rowCount();
        ui->drivesTable->insertRow(row);
        // Device Name
        ui->drivesTable->setItem(row, 0, new QTableWidgetItem(QFileInfo(drive.device).fileName()));
        // Device Path
        ui->drivesTable->setItem(row, 1, new QTableWidgetItem(drive.device));
        // Size
        ui->drivesTable->setItem(row, 2, new QTableWidgetItem(drive.size));
        // Type
        ui->drivesTable->setItem(row, 3, new QTableWidgetItem(drive.type));
        // Label
        ui->drivesTable->setItem(row, 4, new QTableWidgetItem(drive.label));
        // Mount Point
        ui->drivesTable->setItem(row, 5, new QTableWidgetItem(drive.mountPoint));
        // Status
        QString status = drive.isMounted ? "Mounted" : "Unmounted";
        QTableWidgetItem *statusItem = new QTableWidgetItem(status);
        statusItem->setForeground(drive.isMounted ? Qt::green : Qt::red);
        ui->drivesTable->setItem(row, 6, statusItem);
    }
}

QString MainWindow::getSelectedDrive()
{
    QList<QTableWidgetItem*> selectedItems = ui->drivesTable->selectedItems();
    if (selectedItems.isEmpty()) {
        return QString();
    }
    
    int row = selectedItems.first()->row();
    if (row >= 0 && row < ui->drivesTable->rowCount()) {
        // Get the device path from the table directly
        QTableWidgetItem *devicePathItem = ui->drivesTable->item(row, 1); // Column 1 is Device Path
        if (devicePathItem) {
            return devicePathItem->text();
        }
    }
    
    return QString();
}

void MainWindow::updateButtonStates()
{
    QString selectedDrive = getSelectedDrive();
    bool hasSelection = !selectedDrive.isEmpty();
    
    ui->mountButton->setEnabled(hasSelection);
    ui->forceMountButton->setEnabled(hasSelection);
    ui->unmountButton->setEnabled(hasSelection);
}

void MainWindow::logMessage(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    ui->logOutput->append(QString("[%1] %2").arg(timestamp, message));
    
    // Auto-scroll to bottom
    QTextCursor cursor = ui->logOutput->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->logOutput->setTextCursor(cursor);
}



void MainWindow::on_mount777Button_clicked()
{
    QString selectedDrive = getSelectedDrive();
    if (selectedDrive.isEmpty()) {
        QMessageBox::warning(this, "No Drive Selected", "Please select a drive to mount.");
        return;
    }
    // Try to detect filesystem type and label
    QProcess proc;
    proc.start("lsblk", QStringList() << "-no" << "FSTYPE,LABEL" << selectedDrive);
    proc.waitForFinished();
    QStringList fsInfo = QString(proc.readAllStandardOutput()).split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    QString fsType = fsInfo.value(0);
    QString label = fsInfo.value(1);
    if (label.isEmpty()) label = QFileInfo(selectedDrive).baseName();
    QString user = QString::fromUtf8(qgetenv("USER"));
    QString uid = QString::number(getuid());
    QString gid = QString::number(getgid());
    QString mountPoint = "/run/media/" + user + "/" + label;
    QString makeDirCmd = "sudo mkdir -p '" + mountPoint + "'";
    QString ntfsfixCmd = QString("sudo ntfsfix %1").arg(selectedDrive);
    QString mountCmd;
    if (fsType == "ntfs") {
        // Use ntfs-3g with remove_hiberfile and set uid/gid for full user access
        mountCmd = QString("sudo mount -t ntfs-3g -o remove_hiberfile,rw,umask=000,uid=%1,gid=%2 %3 '%4'").arg(uid, gid, selectedDrive, mountPoint);
        QMessageBox::warning(this, "NTFS Hibernation Warning", "If a Windows hibernation file is present, it will be deleted to allow write access. Unsaved Windows session data will be lost.");
    } else if (fsType == "vfat" || fsType == "exfat" || fsType == "fat32" || fsType == "fat") {
        mountCmd = QString("sudo mount -o rw,umask=000,uid=%1,gid=%2 %3 '%4'").arg(uid, gid, selectedDrive, mountPoint);
    } else {
        // For Linux filesystems, just try rw
        mountCmd = QString("sudo mount -o rw %1 '%2'").arg(selectedDrive, mountPoint);
    }
    QString fullCmd;
    if (fsType == "ntfs") {
        fullCmd = makeDirCmd + " && " + ntfsfixCmd + " && " + mountCmd;
    } else {
        fullCmd = makeDirCmd + " && " + mountCmd;
    }
    logMessage("🔧 Mounting with 777 permissions for " + selectedDrive);
    logMessage("⏳ Waiting for sudo password in terminal...");
    // Launch in a terminal for password entry
    QStringList terminals = {"konsole", "gnome-terminal", "xterm", "alacritty", "kitty"};
    bool launched = false;
    for (const QString &terminal : terminals) {
        if (QProcess::startDetached(terminal, QStringList() << "-e" << "bash" << "-c" << fullCmd)) {
            launched = true;
            break;
        }
    }
    if (!launched) {
        QMessageBox::warning(this, "Terminal Not Found", "Could not find a suitable terminal emulator. Please install one of: konsole, gnome-terminal, xterm, alacritty, or kitty");
    } else if (fsType != "ntfs") {
        QMessageBox::information(this, "Mount 777", "If the drive is still not writable after mounting, please check for Windows Fast Startup, hibernation, or filesystem errors.");
    }
}

void MainWindow::on_takeOwnershipButton_clicked()
{
    QList<QTableWidgetItem*> selectedItems = ui->drivesTable->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, "No Drive Selected", "Please select a drive to take ownership of.");
        return;
    }
    
    int row = selectedItems.first()->row();
    if (row < 0 || row >= ui->drivesTable->rowCount()) {
        QMessageBox::warning(this, "No Drive Selected", "Please select a drive to take ownership of.");
        return;
    }
    
    QString mountPoint = ui->drivesTable->item(row, 5) ? ui->drivesTable->item(row, 5)->text() : ""; // Column 5 is Mount Point
    if (mountPoint.isEmpty() || mountPoint == "Unmounted") {
        QMessageBox::warning(this, "Not Mounted", "The selected drive is not mounted.");
        return;
    }
    QString user = QString::fromUtf8(qgetenv("USER"));
    logMessage(QString("Taking ownership of %1 as user %2...").arg(mountPoint, user));
    if (QMessageBox::question(this, "Take Ownership", "This will recursively change the owner and permissions of all files on the drive to your user (chown -R and chmod -R 777). Continue?", QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;
    logMessage("⏳ Waiting for sudo password in terminal...");
    QString chownCmd = QString("sudo chown -R %1:%1 '%2' && sudo chmod -R 777 '%2'; echo; echo 'Press Enter to close...'; read").arg(user, mountPoint);
    QStringList terminals = {"konsole", "gnome-terminal", "xterm", "alacritty", "kitty"};
    bool launched = false;
    for (const QString &terminal : terminals) {
        if (QProcess::startDetached(terminal, QStringList() << "-e" << "bash" << "-c" << chownCmd)) {
            launched = true;
            break;
        }
    }
    if (launched) {
        logMessage("Take Ownership command launched in terminal.");
    } else {
        logMessage("❌ Could not launch a terminal emulator for Take Ownership.");
        QMessageBox::warning(this, "Terminal Not Found", "Could not find a suitable terminal emulator. Please install one of: konsole, gnome-terminal, xterm, alacritty, or kitty");
    }
}

void MainWindow::on_diskFilterCheckBox_stateChanged(int state)
{
    Q_UNUSED(state)
    populateDriveTable();
}

void MainWindow::on_partitionFilterCheckBox_stateChanged(int state)
{
    Q_UNUSED(state)
    populateDriveTable();
}

void MainWindow::on_minSizeFilterCheckBox_stateChanged(int state)
{
    Q_UNUSED(state)
    populateDriveTable();
}

void MainWindow::on_minSizeSpinBox_valueChanged(int value)
{
    Q_UNUSED(value)
    if (ui->minSizeFilterCheckBox->isChecked()) {
        populateDriveTable();
    }
}


#endif