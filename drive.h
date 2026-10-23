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
#include <QSignalBlocker>
#include <QMenu>
#include <QWidgetAction>
#include <QStyle>
#include <QStorageInfo>
#include <QPainter>
#include <algorithm>
#include "drivelabel.h"
#include <unistd.h>

void MainWindow::setupDriveTable()
{
    // Set up table headers
    QStringList headers;
    headers << tr("Device Name") << tr("Device Path") << tr("Size") << tr("Type") << tr("Label") << tr("Mount Point") << tr("Status") << tr("Disk ID");
    ui->drivesTable->setColumnCount(headers.size());
    ui->drivesTable->setHorizontalHeaderLabels(headers);
    
    // Set table properties
    ui->drivesTable->setAlternatingRowColors(true);
    ui->drivesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->drivesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->drivesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
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
    ui->drivesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->drivesTable, &QTableWidget::customContextMenuRequested,
            this, &MainWindow::showDriveContextMenu);
    ui->openDriveButton->setIcon(QIcon::fromTheme("system-file-manager",
                                                style()->standardIcon(QStyle::SP_DirOpenIcon)));
}

void MainWindow::showDriveContextMenu(const QPoint &position)
{
    const auto index = ui->drivesTable->indexAt(position);
    if (!index.isValid()) return;
    ui->drivesTable->setCurrentCell(index.row(), index.column());
    ui->drivesTable->selectRow(index.row());
    updateButtonStates();

    const QString device = getSelectedDrive();
    const auto found = std::find_if(drivesList.cbegin(), drivesList.cend(),
                                   [&device](const DriveInfo &drive) { return drive.device == device; });
    if (found == drivesList.cend()) return;
    const DriveInfo drive = *found;

    QMenu menu(this);
    menu.setObjectName("driveContextMenu");
    menu.setAttribute(Qt::WA_TranslucentBackground);
    menu.setToolTipsVisible(true);
    menu.setMinimumWidth(310);
    const QColor accent(themeAccent.isEmpty() ? "#27ae60" : themeAccent);
    const QColor background = themeBg.isEmpty() ? palette().color(QPalette::Window) : QColor(themeBg);
    const bool dark = background.lightness() < 128;
    const QColor surface = dark ? background.lighter(120) : background.darker(102);
    const QColor text = themeText.isEmpty() ? (dark ? QColor("#eeeeee") : QColor("#202124")) : QColor(themeText);
    const QString muted = dark ? "#a5acb8" : "#646b75";
    const QString tint = QString("rgba(%1,%2,%3,0.20)").arg(accent.red()).arg(accent.green()).arg(accent.blue());
    menu.setStyleSheet(QString(
        "QMenu#driveContextMenu { background: %1; color: %2; border: 1px solid rgba(127,127,127,0.4);"
        " border-radius: 10px; padding: 6px; }"
        "QMenu#driveContextMenu::item { padding: 10px 22px 10px 38px; margin: 2px 4px; border-radius: 6px; }"
        "QMenu#driveContextMenu::item:selected { background: %3; }"
        "QMenu#driveContextMenu::item:disabled { color: %4; }"
        "QMenu#driveContextMenu::icon { left: 12px; }"
        "QMenu#driveContextMenu::separator { height: 1px; background: rgba(127,127,127,0.25); margin: 5px 12px; }")
        .arg(surface.name(), text.name(), tint, dark ? "#666d77" : "#a0a5ac"));

    auto *header = new QWidget(&menu);
    auto *layout = new QVBoxLayout(header);
    layout->setContentsMargins(14, 10, 14, 10);
    layout->setSpacing(6);
    auto *title = new QLabel(header);
    title->setTextFormat(Qt::PlainText);
    const QString name = drive.label.isEmpty() ? QFileInfo(device).fileName() : drive.label;
    QFont titleFont = title->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    title->setFont(titleFont);
    title->setText(title->fontMetrics().elidedText(name, Qt::ElideRight, 330));
    title->setToolTip(name);
    title->setStyleSheet(QString("color: %1; background: transparent;").arg(text.name()));
    layout->addWidget(title);
    QStringList details{device};
    if (!drive.size.isEmpty()) details << drive.size;
    if (!drive.filesystem.isEmpty()) details << drive.filesystem.toUpper();
    auto *subtitle = new QLabel(header);
    subtitle->setTextFormat(Qt::PlainText);
    subtitle->setText(subtitle->fontMetrics().elidedText(details.join("  ·  "), Qt::ElideRight, 330));
    subtitle->setToolTip(details.join("  ·  "));
    subtitle->setStyleSheet(QString("color: %1; background: transparent;").arg(muted));
    layout->addWidget(subtitle);
    auto *status = new QLabel(drive.isMounted ? tr("Mounted") : tr("Not mounted"), header);
    status->setToolTip(drive.mountPoint);
    status->setStyleSheet(QString("color: %1; background: %2; border-radius: 8px; padding: 3px 8px;")
        .arg(drive.isMounted ? accent.name() : muted, drive.isMounted ? tint : "rgba(127,127,127,0.12)"));
    layout->addWidget(status, 0, Qt::AlignLeft);
    auto *heading = new QWidgetAction(&menu);
    heading->setDefaultWidget(header);
    heading->setEnabled(false);
    menu.addAction(heading);
    menu.addSeparator();

    auto *open = menu.addAction(ui->openDriveButton->icon(), tr("Open in Dolphin"));
    open->setEnabled(ui->openDriveButton->isEnabled());
    open->setToolTip(open->isEnabled() ? drive.mountPoint : tr("Mount this partition to browse its files."));
    menu.addSeparator();
    auto *mount = menu.addAction(QIcon::fromTheme("media-mount", style()->standardIcon(QStyle::SP_DriveHDIcon)), tr("Mount"));
    QPixmap ejectIcon(18, 18);
    ejectIcon.fill(Qt::transparent);
    QPainter painter(&ejectIcon);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(text);
    painter.drawPolygon(QPolygon{QPoint(9, 3), QPoint(15, 11), QPoint(3, 11)});
    painter.drawRoundedRect(QRect(3, 13, 12, 2), 1, 1);
    painter.end();
    auto *unmount = menu.addAction(QIcon::fromTheme("media-eject", QIcon(ejectIcon)), tr("Unmount"));
    mount->setEnabled(ui->mountButton->isEnabled());
    unmount->setEnabled(ui->unmountButton->isEnabled());
    menu.setDefaultAction(open->isEnabled() ? open : mount);

    const auto *chosen = menu.exec(ui->drivesTable->viewport()->mapToGlobal(position));
    if (!chosen) return;
    if (chosen == open) {
        openDriveInDolphin(device);
        return;
    }
    // A refresh or sort can run while the menu is open. Find the same device
    // again instead of operating on whichever partition inherited its row.
    for (int row = 0; row < ui->drivesTable->rowCount(); ++row) {
        if (ui->drivesTable->item(row, 1)->text() != device) continue;
        ui->drivesTable->setCurrentCell(row, 1);
        ui->drivesTable->selectRow(row);
        updateButtonStates();
        if (chosen == mount) ui->mountButton->click();
        else if (chosen == unmount) ui->unmountButton->click();
        break;
    }
}

void MainWindow::on_openDriveButton_clicked()
{
    openDriveInDolphin(getSelectedDrive());
}

void MainWindow::openDriveInDolphin(const QString &device)
{
    if (device.isEmpty()) return;
    const auto devicePath = [](const QString &path) {
        const QString canonical = QFileInfo(path).canonicalFilePath();
        return canonical.isEmpty() ? path : canonical;
    };
    const QString canonicalDevice = devicePath(device);
    // Read the live mount table: the partition may have been unmounted outside
    // this app since the last refresh. Never open its now-empty mount folder.
    QString mountPoint;
    for (const auto &volume : QStorageInfo::mountedVolumes()) {
        if (volume.isReady() && devicePath(QString::fromLocal8Bit(volume.device())) == canonicalDevice) {
            mountPoint = volume.rootPath();
            break;
        }
    }
    if (mountPoint.isEmpty()) {
        QMessageBox::information(this, tr("Not Mounted"), tr("Mount the selected partition before opening it in Dolphin."));
        refreshDrives();
        return;
    }
    // Pass the path as one argument, preserving spaces and special characters.
    if (!QProcess::startDetached("dolphin", QStringList{"--", mountPoint})) {
        QMessageBox::warning(this, tr("Could Not Open Dolphin"),
                             tr("Dolphin could not be started. Make sure it is installed, then try again."));
        return;
    }
    logMessage(tr("📂 Opened %1 in Dolphin").arg(mountPoint.toHtmlEscaped()));
}

void MainWindow::refreshDrives()
{
    logMessage("🔄 Refreshing drive list...");

    QStringList lsblkArgs;
    lsblkArgs << "-o" << "NAME,SIZE,TYPE,LABEL,MOUNTPOINT,MODEL,SERIAL,UUID,FSTYPE" << "--noheadings" << "--pairs";

    // Unprivileged lsblk depends on udev's cached metadata, which can be missing
    // labels after boot. Root lsblk reads filesystem superblocks directly, so use
    // it when a sudo session is already cached (never prompts: -n).
    QProcess sudoCheck;
    sudoCheck.start("sudo", QStringList() << "-n" << "true");
    sudoCheck.waitForFinished(2000);
    if (sudoCheck.exitCode() == 0) {
        executeCommand("sudo", QStringList() << "-n" << "lsblk" << lsblkArgs);
    } else {
        executeCommand("lsblk", lsblkArgs);
    }
}

void MainWindow::on_refreshButton_clicked()
{
    refreshDrives();
}

void MainWindow::on_setDriveLabelButton_clicked()
{
    const QString device = getSelectedDrive();
    if (device.isEmpty()) return;
    DriveLabelDialog dialog(device, this);
    bool refreshNeeded = false;
    connect(&dialog, &DriveLabelDialog::refreshRequested, &dialog, [&refreshNeeded]() { refreshNeeded = true; });
    connect(&dialog, &DriveLabelDialog::labelChanged, this, [this, device, &refreshNeeded](const QString &label) {
        refreshNeeded = true;
        logMessage(label.isEmpty() ? tr("✅ Removed volume label from %1").arg(device)
                                   : tr("✅ Volume label for %1 is now: %2").arg(device, label.toHtmlEscaped()));
        for (auto &drive : drivesList) if (drive.device == device) drive.label = label;
        populateDriveTable();
    });
    dialog.exec();
    if (refreshNeeded) refreshDrives();
}

void MainWindow::on_mountButton_clicked()
{
    QString selectedDrive = getSelectedDrive();
    if (selectedDrive.isEmpty()) {
        QMessageBox::warning(this, tr("No Drive Selected"), tr("Please select a drive to mount."));
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
        QMessageBox::warning(this, tr("No Drive Selected"), tr("Please select a drive to mount."));
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
        QMessageBox::warning(this, tr("No Drive Selected"), tr("Please select a drive to unmount."));
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
            QString value = DriveLabel::decodeLsblkValue(match.captured(2));
            
            if (key == "NAME") devName = value;
            else if (key == "SIZE") drive.size = value;
            else if (key == "TYPE") drive.type = value;
            else if (key == "LABEL") drive.label = value;
            else if (key == "MOUNTPOINT") drive.mountPoint = value;
            else if (key == "MODEL") drive.model = value;
            else if (key == "SERIAL") drive.serial = value;
            else if (key == "UUID") drive.uuid = value;
            else if (key == "FSTYPE") drive.filesystem = value;
        }
        if (!devName.isEmpty()) {
            drive.device = "/dev/" + devName;
            drive.isMounted = !drive.mountPoint.isEmpty();
            
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
    const QString selectedDevice = getSelectedDrive();
    const bool sorting = ui->drivesTable->isSortingEnabled();
    const QSignalBlocker selectionBlocker(ui->drivesTable->selectionModel());
    // Sorting while inserting individual cells can move a half-built row and
    // associate a label with the wrong device. Restore sorting after all cells.
    ui->drivesTable->setSortingEnabled(false);
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
        QString mountText = drive.mountPoint.isEmpty() ? tr("Not mounted") : drive.mountPoint;
        QTableWidgetItem *mountItem = new QTableWidgetItem(mountText);
        if (!drive.mountPoint.isEmpty()) {
            mountItem->setForeground(QColor(34, 139, 34)); // Forest green
        }
        ui->drivesTable->setItem(row, 5, mountItem);
        
        // Status
        QString status = drive.isMounted ? tr("✓ Mounted") : tr("✗ Unmounted");
        QTableWidgetItem *statusItem = new QTableWidgetItem(status);
        statusItem->setData(Qt::UserRole, drive.isMounted); // logic reads this, never the display text
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
    ui->drivesTable->setSortingEnabled(sorting);
    for (int row = 0; row < ui->drivesTable->rowCount(); ++row) {
        if (ui->drivesTable->item(row, 1)->text() == selectedDevice) {
            ui->drivesTable->selectRow(row);
            break;
        }
    }
    updateButtonStates();
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
        QMessageBox::warning(this, tr("No Drive Selected"), tr("Please select a drive to eject."));
        return;
    }
    
    int row = ui->drivesTable->currentRow();
    QString driveType = ui->drivesTable->item(row, 3)->text(); // Type column
    
    if (driveType != "disk") {
        QMessageBox::warning(this, tr("Invalid Selection"), tr("Eject can only be used on disk devices, not partitions."));
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
        QMessageBox::warning(this, tr("No Drive Selected"), tr("Please select a drive or partition to format."));
        return;
    }
    
    int row = ui->drivesTable->currentRow();
    QString driveType = ui->drivesTable->item(row, 3)->text(); // Type column
    // Data role, not display text: "✗ Unmounted" contains "Mounted", so the old
    // text check reported every drive as mounted and always blocked formatting.
    bool isMounted = ui->drivesTable->item(row, 6)->data(Qt::UserRole).toBool();
    
    if (isMounted) {
        QMessageBox::warning(this, tr("Drive Mounted"), tr("Please unmount the drive before formatting."));
        return;
    }
    
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Format Drive"));
    dialog->setMinimumWidth(400);
    
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    
    QLabel *warningLabel = new QLabel(QString("⚠️ WARNING: This will permanently erase all data on:\n%1\n\nThis action cannot be undone!").arg(selectedDrive), dialog);
    warningLabel->setStyleSheet("color: red; font-weight: bold; padding: 10px;");
    warningLabel->setWordWrap(true);
    layout->addWidget(warningLabel);
    
    QLabel *filesystemLabel = new QLabel(tr("Filesystem:"), dialog);
    layout->addWidget(filesystemLabel);
    
    QComboBox *filesystemCombo = new QComboBox(dialog);
    filesystemCombo->addItems(QStringList() << "ext4" << "ext3" << "ext2" << "btrfs" << "xfs" << "ntfs" << "fat32" << "exfat");
    filesystemCombo->setCurrentText("ext4");
    layout->addWidget(filesystemCombo);
    
    QCheckBox *quickFormatCheck = new QCheckBox("Quick Format (skip bad blocks check)", dialog);
    quickFormatCheck->setChecked(true);
    layout->addWidget(quickFormatCheck);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *formatButton = new QPushButton(tr("Format"), dialog);
    formatButton->setStyleSheet("background-color: #dc3545; color: white; font-weight: bold;");
    QPushButton *cancelButton = new QPushButton(tr("Cancel"), dialog);
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
        QMessageBox::warning(this, tr("No Drive Selected"), tr("Please select a drive to view SMART information."));
        return;
    }
    
    int row = ui->drivesTable->currentRow();
    QString driveType = ui->drivesTable->item(row, 3)->text(); // Type column
    
    if (driveType != "disk") {
        QMessageBox::warning(this, tr("Invalid Selection"), tr("SMART information is only available for disk devices, not partitions."));
        return;
    }
    
    QProcess checkProc;
    checkProc.start("which", QStringList() << "smartctl");
    checkProc.waitForFinished();
    
    if (checkProc.exitCode() != 0) {
        QMessageBox::information(this, tr("SMART Not Available"), 
                                  tr("smartmontools is not installed.\n\n"
                                  "Install it with: sudo pacman -S smartmontools"));
        return;
    }
    
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(QString("SMART Information: %1").arg(selectedDrive));
    dialog->setMinimumSize(700, 500);
    
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    
    QLabel *label = new QLabel(tr("Retrieving SMART information..."), dialog);
    layout->addWidget(label);
    
    QTextEdit *smartOutput = new QTextEdit(dialog);
    smartOutput->setReadOnly(true);
    smartOutput->setFont(QFont("monospace", 9));
    layout->addWidget(smartOutput);
    
    QPushButton *closeButton = new QPushButton(tr("Close"), dialog);
    layout->addWidget(closeButton);
    
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    
    QProcess *proc = new QProcess(dialog);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [label, smartOutput, proc](int exitCode, QProcess::ExitStatus exitStatus) {
                QString output = QString::fromUtf8(proc->readAllStandardOutput());
                QString error = QString::fromUtf8(proc->readAllStandardError());
                
                if (exitCode == 0) {
                    label->setText(tr("SMART Information:"));
                    smartOutput->setPlainText(output);
                } else {
                    label->setText(tr("Error retrieving SMART information:"));
                    smartOutput->setPlainText((output + "\n" + error).trimmed());
                }
                proc->deleteLater();
            });

    // Reading SMART data requires root — reuse the app's cached sudo session
    if (!authenticateSudo()) {
        dialog->deleteLater();
        return;
    }
    proc->start("sudo", QStringList() << "smartctl" << "-a" << selectedDrive);

    dialog->exec();
    dialog->deleteLater();
}

void MainWindow::updateButtonStates()
{
    QString selectedDrive = getSelectedDrive();
    bool hasSelection = !selectedDrive.isEmpty();
    const auto drive = std::find_if(drivesList.cbegin(), drivesList.cend(),
                                   [&selectedDrive](const DriveInfo &item) { return item.device == selectedDrive; });
    const bool mounted = drive != drivesList.cend() && drive->isMounted;
    
    ui->mountButton->setEnabled(hasSelection && !mounted);
    ui->forceMountButton->setEnabled(hasSelection);
    ui->unmountButton->setEnabled(hasSelection && mounted);
    ui->openDriveButton->setEnabled(hasSelection && mounted && !drive->mountPoint.isEmpty() && drive->mountPoint != "[SWAP]");
    ui->ejectButton->setEnabled(hasSelection);
    ui->formatButton->setEnabled(hasSelection);
    ui->smartInfoButton->setEnabled(hasSelection);
    ui->mount777Button->setEnabled(hasSelection);
    ui->takeOwnershipButton->setEnabled(hasSelection);
    ui->setDriveLabelButton->setEnabled(hasSelection);
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
        QMessageBox::warning(this, tr("No Drive Selected"), tr("Please select a drive to mount."));
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
        QMessageBox::warning(this, tr("NTFS Hibernation Warning"), tr("If a Windows hibernation file is present, it will be deleted to allow write access. Unsaved Windows session data will be lost."));
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
        QMessageBox::warning(this, tr("Terminal Not Found"), tr("Could not find a suitable terminal emulator. Please install one of: konsole, gnome-terminal, xterm, alacritty, or kitty"));
    } else if (fsType != "ntfs") {
        QMessageBox::information(this, tr("Mount 777"), tr("If the drive is still not writable after mounting, please check for Windows Fast Startup, hibernation, or filesystem errors."));
    }
}

void MainWindow::on_takeOwnershipButton_clicked()
{
    QList<QTableWidgetItem*> selectedItems = ui->drivesTable->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, tr("No Drive Selected"), tr("Please select a drive to take ownership of."));
        return;
    }
    
    int row = selectedItems.first()->row();
    if (row < 0 || row >= ui->drivesTable->rowCount()) {
        QMessageBox::warning(this, tr("No Drive Selected"), tr("Please select a drive to take ownership of."));
        return;
    }
    
    QString mountPoint = ui->drivesTable->item(row, 5) ? ui->drivesTable->item(row, 5)->text() : ""; // Column 5 is Mount Point
    if (mountPoint.isEmpty() || mountPoint == "Unmounted") {
        QMessageBox::warning(this, tr("Not Mounted"), tr("The selected drive is not mounted."));
        return;
    }
    QString user = QString::fromUtf8(qgetenv("USER"));
    logMessage(QString("Taking ownership of %1 as user %2...").arg(mountPoint, user));
    if (QMessageBox::question(this, tr("Take Ownership"), tr("This will recursively change the owner and permissions of all files on the drive to your user (chown -R and chmod -R 777). Continue?"), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
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
        QMessageBox::warning(this, tr("Terminal Not Found"), tr("Could not find a suitable terminal emulator. Please install one of: konsole, gnome-terminal, xterm, alacritty, or kitty"));
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
