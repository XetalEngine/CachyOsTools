#ifndef DRIVE_H
#define DRIVE_H

#include <QDialog>
#include <QComboBox>
#include <QColor>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QTextEdit>
#include <QFont>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QTimer>
#include <QProcess>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QFileInfo>
#include <unistd.h>

void MainWindow::setupDriveTable()
{
    // Set up table headers
    QStringList headers;
    headers << "Device Name" << "Device Path" << "Size" << "Type" << "Label" << "Mount Point" << "Status" << "Disk ID";
    ui->drivesTable->setColumnCount(headers.size());
    ui->drivesTable->setHorizontalHeaderLabels(headers);
    
    // Set table properties
    ui->drivesTable->setAlternatingRowColors(true);
    ui->drivesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->drivesTable->setSortingEnabled(true);
    ui->drivesTable->horizontalHeader()->setStretchLastSection(false);
    ui->drivesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    
    // Set column widths
    ui->drivesTable->setColumnWidth(0, 120); // Device Name
    ui->drivesTable->setColumnWidth(1, 150); // Device Path
    ui->drivesTable->setColumnWidth(2, 60);  // Size (20% smaller: 100 * 0.8)
    ui->drivesTable->setColumnWidth(3, 60);  // Type
    ui->drivesTable->setColumnWidth(4, 130); // Label
    ui->drivesTable->setColumnWidth(5, 120); // Mount Point (30% smaller: 200 * 0.7)
    ui->drivesTable->setColumnWidth(6, 100);  // Status
    ui->drivesTable->setColumnWidth(7, 400);  // Disk ID (wider to show full IDs)
    
    // Enable word wrap for long text
    ui->drivesTable->setWordWrap(true);
}

void MainWindow::refreshDrives()
{
    logMessage("🔄 Refreshing drive list...");
    
    // Execute lsblk command to get drive information with MODEL and SERIAL for disk ID construction
    executeCommand("lsblk", QStringList() << "-o" << "NAME,SIZE,TYPE,LABEL,MOUNTPOINT,MODEL,SERIAL" << "--noheadings" << "--pairs");
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
    QList<DriveInfo> allDrives;
    QMap<QString, QString> diskIdMap; // device path -> disk ID
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    for (const QString &line : lines) {
        DriveInfo drive;
        QString devName;
        
        // Parse pairs format: KEY="VALUE" KEY2="VALUE2" ...
        // Handle quoted values that may contain spaces
        QRegularExpression pairRe("(\\w+)=\"([^\"]*)\"");
        QRegularExpressionMatchIterator i = pairRe.globalMatch(line);
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            QString key = match.captured(1);
            QString value = match.captured(2);
            
            if (key == "NAME") devName = value;
            else if (key == "SIZE") drive.size = value;
            else if (key == "TYPE") drive.type = value;
            else if (key == "LABEL") drive.label = value;
            else if (key == "MOUNTPOINT") drive.mountPoint = value;
            else if (key == "MODEL") drive.model = value;
            else if (key == "SERIAL") drive.serial = value;
        }
        if (!devName.isEmpty()) {
            drive.device = "/dev/" + devName;
            drive.isMounted = !drive.mountPoint.isEmpty();
            
            // Get additional info using blkid if available
            QProcess blkidProc;
            blkidProc.start("blkid", QStringList() << "-o" << "value" << "-s" << "UUID" << "-s" << "TYPE" << drive.device);
            blkidProc.waitForFinished();
            if (blkidProc.exitCode() == 0) {
                QString blkidOutput = QString::fromUtf8(blkidProc.readAllStandardOutput()).trimmed();
                QStringList blkidParts = blkidOutput.split('\n', Qt::SkipEmptyParts);
                if (blkidParts.size() >= 2) {
                    drive.uuid = blkidParts[0];
                    drive.filesystem = blkidParts[1];
                } else if (blkidParts.size() == 1) {
                    drive.uuid = blkidParts[0];
                }
            }
            
            // Get disk ID from /dev/disk/by-id/ symlinks (matches system format)
            if (drive.type == "disk") {
                // Determine prefix based on device type
                QString prefix = devName.startsWith("nvme") ? "nvme" : (devName.startsWith("sd") ? "sata" : "ata");
                
                // Use find to locate the symlink pointing to this device
                QProcess proc;
                QString cmd = QString("find /dev/disk/by-id/ -type l -name '%1-*' ! -name '*eui*' ! -name '*wwn*' ! -name '*part*' -exec sh -c 'target=$(readlink -f \"$1\"); if [ \"$target\" = \"%2\" ]; then basename \"$1\"; fi' _ {} \\; | head -1").arg(prefix, drive.device);
                proc.start("sh", QStringList() << "-c" << cmd);
                proc.waitForFinished(5000); // 5 second timeout
                
                QString output = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
                QString error = QString::fromUtf8(proc.readAllStandardError());
                
                if (!output.isEmpty() && output != prefix + "-") {
                    drive.diskId = output;
                    diskIdMap[drive.device] = output; // Store for partitions
                    logMessage(QString("Disk ID for %1: %2").arg(drive.device, drive.diskId));
                } else {
                    logMessage(QString("Disk ID lookup failed for %1. Output: '%2', Error: '%3'").arg(drive.device, output, error));
                }
            }
            
            allDrives.append(drive);
        }
    }
    
    // Second pass: assign disk IDs to partitions based on parent disk
    for (DriveInfo &drive : allDrives) {
        if (drive.type == "part" && drive.diskId.isEmpty()) {
            // Find parent disk by matching device name prefix
            QString devName = QFileInfo(drive.device).fileName();
            QString parentDevice;
            
            // Extract parent disk name (e.g., nvme3n1p1 -> nvme3n1)
            QRegularExpression parentRe("^(nvme\\d+n\\d+|sd[a-z]+)");
            QRegularExpressionMatch match = parentRe.match(devName);
            if (match.hasMatch()) {
                parentDevice = "/dev/" + match.captured(1);
                
                // Look up disk ID from map
                if (diskIdMap.contains(parentDevice)) {
                    drive.diskId = diskIdMap[parentDevice];
                } else {
                    // If not in map, try to find it now
                    QString prefix = devName.startsWith("nvme") ? "nvme" : "sata";
                    QProcess proc;
                    QString cmd = QString("find /dev/disk/by-id/ -type l -name '%1-*' ! -name '*eui*' ! -name '*wwn*' ! -name '*part*' -exec sh -c 'readlink -f \"$1\" | grep -q \"^%2$\" && basename \"$1\"' _ {} \\; | head -1").arg(prefix, parentDevice);
                    proc.start("sh", QStringList() << "-c" << cmd);
                    proc.waitForFinished(5000);
                    QString output = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
                    if (!output.isEmpty()) {
                        drive.diskId = output;
                        diskIdMap[parentDevice] = output;
                    }
                }
            }
        }
        
    }
    
    // Build final list
    QList<DriveInfo> drives;
    for (const DriveInfo &drive : allDrives) {
        if (drive.type == "disk" || drive.type == "part") {
            drives.append(drive);
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
        QTableWidgetItem *nameItem = new QTableWidgetItem(QFileInfo(drive.device).fileName());
        nameItem->setToolTip(drive.device);
        ui->drivesTable->setItem(row, 0, nameItem);
        
        // Device Path
        QTableWidgetItem *pathItem = new QTableWidgetItem(drive.device);
        pathItem->setToolTip(QString("UUID: %1\nFilesystem: %2").arg(drive.uuid, drive.filesystem));
        ui->drivesTable->setItem(row, 1, pathItem);
        
        // Size
        QTableWidgetItem *sizeItem = new QTableWidgetItem(drive.size);
        sizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        ui->drivesTable->setItem(row, 2, sizeItem);
        
        // Type
        QTableWidgetItem *typeItem = new QTableWidgetItem(drive.type);
        typeItem->setTextAlignment(Qt::AlignCenter);
        if (drive.type == "disk") {
            typeItem->setForeground(QColor(70, 130, 180)); // Steel blue
        } else {
            typeItem->setForeground(QColor(100, 149, 237)); // Cornflower blue
        }
        ui->drivesTable->setItem(row, 3, typeItem);
        
        // Label
        QString labelText = drive.label.isEmpty() ? "-" : drive.label;
        QTableWidgetItem *labelItem = new QTableWidgetItem(labelText);
        if (!drive.model.isEmpty()) {
            labelItem->setToolTip(QString("Model: %1\nSerial: %2").arg(drive.model, drive.serial));
        }
        ui->drivesTable->setItem(row, 4, labelItem);
        
        // Mount Point
        QString mountText = drive.mountPoint.isEmpty() ? "Not mounted" : drive.mountPoint;
        QTableWidgetItem *mountItem = new QTableWidgetItem(mountText);
        if (!drive.mountPoint.isEmpty()) {
            mountItem->setForeground(QColor(34, 139, 34)); // Forest green
        }
        ui->drivesTable->setItem(row, 5, mountItem);
        
        // Status
        QString status = drive.isMounted ? "✓ Mounted" : "✗ Unmounted";
        QTableWidgetItem *statusItem = new QTableWidgetItem(status);
        statusItem->setTextAlignment(Qt::AlignCenter);
        if (drive.isMounted) {
            statusItem->setForeground(QColor(0, 150, 0)); // Green
        } else {
            statusItem->setForeground(QColor(200, 0, 0)); // Red
        }
        ui->drivesTable->setItem(row, 6, statusItem);
        
        // Disk ID
        QString diskIdText = drive.diskId.isEmpty() ? "-" : drive.diskId;
        QTableWidgetItem *diskIdItem = new QTableWidgetItem(diskIdText);
        diskIdItem->setToolTip(diskIdText); // Show full ID in tooltip
        diskIdItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->drivesTable->setItem(row, 7, diskIdItem);
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

void MainWindow::on_ejectButton_clicked() {
    QString selectedDrive = getSelectedDrive();
    if (selectedDrive.isEmpty()) {
        QMessageBox::warning(this, "No Drive Selected", "Please select a drive to eject.");
        return;
    }
    
    int row = ui->drivesTable->currentRow();
    QString driveType = ui->drivesTable->item(row, 3)->text(); // Type column
    
    if (driveType != "disk") {
        QMessageBox::warning(this, "Invalid Selection", "Eject can only be used on disk devices, not partitions.");
        return;
    }
    
    int ret = QMessageBox::question(this, "Eject Drive", 
                                     QString("Are you sure you want to eject '%1'?\n\n"
                                     "The drive will be unmounted and ejected.").arg(selectedDrive),
                                     QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        logMessage("⏏️ Ejecting " + selectedDrive);
        logMessage("⏳ Waiting for sudo password in terminal...");
        executeCommand("udisksctl", QStringList() << "eject" << "-b" << selectedDrive);
    }
}

void MainWindow::on_formatButton_clicked() {
    QString selectedDrive = getSelectedDrive();
    if (selectedDrive.isEmpty()) {
        QMessageBox::warning(this, "No Drive Selected", "Please select a drive or partition to format.");
        return;
    }
    
    int row = ui->drivesTable->currentRow();
    QString driveType = ui->drivesTable->item(row, 3)->text(); // Type column
    bool isMounted = ui->drivesTable->item(row, 6)->text().contains("Mounted");
    
    if (isMounted) {
        QMessageBox::warning(this, "Drive Mounted", "Please unmount the drive before formatting.");
        return;
    }
    
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Format Drive");
    dialog->setMinimumWidth(400);
    
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    
    QLabel *warningLabel = new QLabel(QString("⚠️ WARNING: This will permanently erase all data on:\n%1\n\nThis action cannot be undone!").arg(selectedDrive), dialog);
    warningLabel->setStyleSheet("color: red; font-weight: bold; padding: 10px;");
    warningLabel->setWordWrap(true);
    layout->addWidget(warningLabel);
    
    QLabel *filesystemLabel = new QLabel("Filesystem:", dialog);
    layout->addWidget(filesystemLabel);
    
    QComboBox *filesystemCombo = new QComboBox(dialog);
    filesystemCombo->addItems(QStringList() << "ext4" << "ext3" << "ext2" << "btrfs" << "xfs" << "ntfs" << "fat32" << "exfat");
    filesystemCombo->setCurrentText("ext4");
    layout->addWidget(filesystemCombo);
    
    QCheckBox *quickFormatCheck = new QCheckBox("Quick Format (skip bad blocks check)", dialog);
    quickFormatCheck->setChecked(true);
    layout->addWidget(quickFormatCheck);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *formatButton = new QPushButton("Format", dialog);
    formatButton->setStyleSheet("background-color: #dc3545; color: white; font-weight: bold;");
    QPushButton *cancelButton = new QPushButton("Cancel", dialog);
    buttonLayout->addWidget(formatButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);
    
    connect(formatButton, &QPushButton::clicked, [this, dialog, selectedDrive, filesystemCombo, quickFormatCheck, driveType]() {
        QString fs = filesystemCombo->currentText();
        QString mkfsCmd;
        
        if (fs == "ext4") mkfsCmd = "mkfs.ext4";
        else if (fs == "ext3") mkfsCmd = "mkfs.ext3";
        else if (fs == "ext2") mkfsCmd = "mkfs.ext2";
        else if (fs == "btrfs") mkfsCmd = "mkfs.btrfs";
        else if (fs == "xfs") mkfsCmd = "mkfs.xfs";
        else if (fs == "ntfs") mkfsCmd = "mkfs.ntfs";
        else if (fs == "fat32") mkfsCmd = "mkfs.vfat -F 32";
        else if (fs == "exfat") mkfsCmd = "mkfs.exfat";
        
        QString command = QString("sudo %1 %2 %3").arg(mkfsCmd).arg(quickFormatCheck->isChecked() ? "-F" : "").arg(selectedDrive);
        command = command.trimmed();
        
        logMessage(QString("🔧 Formatting %1 as %2...").arg(selectedDrive, fs));
        logMessage("⏳ Waiting for sudo password in terminal...");
        runSudoCommandInTerminal(command);
        
        dialog->accept();
        QTimer::singleShot(3000, this, [this]() {
            refreshDrives();
        });
    });
    
    connect(cancelButton, &QPushButton::clicked, dialog, &QDialog::reject);
    
    dialog->exec();
    dialog->deleteLater();
}

void MainWindow::on_smartInfoButton_clicked() {
    QString selectedDrive = getSelectedDrive();
    if (selectedDrive.isEmpty()) {
        QMessageBox::warning(this, "No Drive Selected", "Please select a drive to view SMART information.");
        return;
    }
    
    int row = ui->drivesTable->currentRow();
    QString driveType = ui->drivesTable->item(row, 3)->text(); // Type column
    
    if (driveType != "disk") {
        QMessageBox::warning(this, "Invalid Selection", "SMART information is only available for disk devices, not partitions.");
        return;
    }
    
    QProcess checkProc;
    checkProc.start("which", QStringList() << "smartctl");
    checkProc.waitForFinished();
    
    if (checkProc.exitCode() != 0) {
        QMessageBox::information(this, "SMART Not Available", 
                                  "smartmontools is not installed.\n\n"
                                  "Install it with: sudo pacman -S smartmontools");
        return;
    }
    
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(QString("SMART Information: %1").arg(selectedDrive));
    dialog->setMinimumSize(700, 500);
    
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    
    QLabel *label = new QLabel("Retrieving SMART information...", dialog);
    layout->addWidget(label);
    
    QTextEdit *smartOutput = new QTextEdit(dialog);
    smartOutput->setReadOnly(true);
    smartOutput->setFont(QFont("monospace", 9));
    layout->addWidget(smartOutput);
    
    QPushButton *closeButton = new QPushButton("Close", dialog);
    layout->addWidget(closeButton);
    
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    
    QProcess *proc = new QProcess(dialog);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [label, smartOutput, proc](int exitCode, QProcess::ExitStatus exitStatus) {
                QString output = QString::fromUtf8(proc->readAllStandardOutput());
                QString error = QString::fromUtf8(proc->readAllStandardError());
                
                if (exitCode == 0) {
                    label->setText("SMART Information:");
                    smartOutput->setPlainText(output);
                } else {
                    label->setText("Error retrieving SMART information:");
                    smartOutput->setPlainText(error);
                }
                proc->deleteLater();
            });
    
    proc->start("smartctl", QStringList() << "-a" << selectedDrive);
    
    dialog->exec();
    dialog->deleteLater();
}

void MainWindow::updateButtonStates()
{
    QString selectedDrive = getSelectedDrive();
    bool hasSelection = !selectedDrive.isEmpty();
    
    ui->mountButton->setEnabled(hasSelection);
    ui->forceMountButton->setEnabled(hasSelection);
    ui->unmountButton->setEnabled(hasSelection);
    ui->ejectButton->setEnabled(hasSelection);
    ui->formatButton->setEnabled(hasSelection);
    ui->smartInfoButton->setEnabled(hasSelection);
    ui->mount777Button->setEnabled(hasSelection);
    ui->takeOwnershipButton->setEnabled(hasSelection);
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