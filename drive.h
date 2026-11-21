#ifndef DRIVE_H
#define DRIVE_H
#include "volume_commands.h"
#include "file_manager.h"

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
    driveSummary = new QLabel(ui->drivesTable->parentWidget());
    driveSummary->setObjectName("driveSummary");
    driveSummary->setProperty("muted", true);
    if (auto *layout = qobject_cast<QBoxLayout *>(ui->drivesTable->parentWidget()->layout()))
        layout->insertWidget(layout->indexOf(ui->drivesTable), driveSummary);
    connect(ui->diskFilterCheckBox, &QCheckBox::toggled, this, [this] { populateDriveTable(); });
    connect(ui->partitionFilterCheckBox, &QCheckBox::toggled, this, [this] { populateDriveTable(); });
    ui->drivesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->drivesTable, &QTreeWidget::customContextMenuRequested,
            this, &MainWindow::showDriveContextMenu);
    ui->openDriveButton->setIcon(QIcon::fromTheme("system-file-manager",
                                                style()->standardIcon(QStyle::SP_DirOpenIcon)));
    ui->openDriveButton->setText(tr("Open files"));
    ui->openDriveButton->setToolTip(tr("Browse this volume in your default file manager"));
}

void MainWindow::showDriveContextMenu(const QPoint &position)
{
    auto *item = ui->drivesTable->itemAt(position);
    if (!item) return;
    ui->drivesTable->setCurrentItem(item);
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

    auto *open = menu.addAction(ui->openDriveButton->icon(), tr("Open in file manager"));
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
    QPolygon triangle;
    triangle << QPoint(9, 3) << QPoint(15, 11) << QPoint(3, 11);
    painter.drawPolygon(triangle);
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
    if (ui->drivesTable->selectDevice(device)) {
        updateButtonStates();
        if (chosen == mount) ui->mountButton->click();
        else if (chosen == unmount) ui->unmountButton->click();
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
        QMessageBox::information(this, tr("Not Mounted"), tr("Mount the selected partition before browsing its files."));
        refreshDrives();
        return;
    }
    FileManager::openFolder(this, mountPoint);
    logMessage(tr("📂 Opening %1 in the file manager").arg(mountPoint.toHtmlEscaped()));
}

void MainWindow::refreshDrives()
{
    updateButtonStates();
    logMessage("🔄 Refreshing drive list...");

    QStringList lsblkArgs;
    lsblkArgs << "-o" << "NAME,PATH,PKNAME,SIZE,TYPE,LABEL,MOUNTPOINT,MODEL,SERIAL,UUID,FSTYPE" << "--bytes" << "--paths" << "--pairs";

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
    executeCommand("udisksctl", CachyVolumes::mount(selectedDrive));
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
    executeCommand("udisksctl", CachyVolumes::mount(selectedDrive, true));
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
    executeCommand("udisksctl", CachyVolumes::unmount(selectedDrive));
}








QList<DriveInfo> MainWindow::parseDriveList(const QString &output)
{
    return DriveTree::parse(output);
}

void MainWindow::populateDriveTable()
{
    ui->minSizeSpinBox->setEnabled(ui->minSizeFilterCheckBox->isChecked());
    ui->drivesTable->setDrives(drivesList, ui->minSizeFilterCheckBox->isChecked()
        ? quint64(ui->minSizeSpinBox->value()) * 1024 * 1024 : 0,
        ui->diskFilterCheckBox->isChecked(), ui->partitionFilterCheckBox->isChecked());
    driveSummary->setText(ui->drivesTable->summary());
    updateButtonStates();
}

QString MainWindow::getSelectedDrive()
{
    return ui->drivesTable->selectedDevice();
}

void MainWindow::on_ejectButton_clicked() {
    QString selectedDrive = getSelectedDrive();
    if (selectedDrive.isEmpty()) {
        QMessageBox::warning(this, tr("No Drive Selected"), tr("Please select a drive to eject."));
        return;
    }
    
    const QString driveType = ui->drivesTable->selectedType();
    
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
    
    const QString driveType = ui->drivesTable->selectedType();
    // Include mounted descendants, even when collapsed or filtered out.
    const bool isMounted = ui->drivesTable->selectedInUse();
    
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
    
    const QString driveType = ui->drivesTable->selectedType();
    
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
    const QString device = getSelectedDrive();
    const auto drive = std::find_if(drivesList.cbegin(), drivesList.cend(),
        [&device](const DriveInfo &item) { return item.device == device; });
    const bool selected = drive != drivesList.cend();
    const bool disk = selected && drive->type == "disk";
    const bool children = std::any_of(drivesList.cbegin(), drivesList.cend(),
        [&device](const DriveInfo &item) { return item.parent == device; });
    const bool volume = selected && (drive->type == "part" || (disk && !children && !drive->filesystem.isEmpty()));
    const bool mounted = selected && drive->isMounted;
    const bool browsable = mounted && drive->mountPoint.startsWith('/');
    const bool inUse = ui->drivesTable->selectedInUse();
    ui->mountButton->setEnabled(volume && !inUse);
    ui->forceMountButton->setEnabled(volume && !inUse);
    ui->unmountButton->setEnabled(volume && mounted);
    ui->openDriveButton->setEnabled(browsable);
    ui->ejectButton->setEnabled(disk && !inUse);
    ui->formatButton->setEnabled(selected && !inUse);
    ui->smartInfoButton->setEnabled(disk);
    ui->driveHealthButton->setEnabled(disk);
    ui->burnIsoButton->setEnabled(disk && !inUse);
    ui->mount777Button->setEnabled(volume && !inUse);
    ui->takeOwnershipButton->setEnabled(browsable);
    ui->setDriveLabelButton->setEnabled(volume);
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
    QStringList terminals = {"konsole", "gnome-terminal", "kitty", "alacritty", "xterm"};
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
    const QString mountPoint = ui->drivesTable->selectedMount();
    if (!mountPoint.startsWith('/')) {
        QMessageBox::warning(this, tr("Not Mounted"), tr("The selected drive is not mounted."));
        return;
    }
    QString user = QString::fromUtf8(qgetenv("USER"));
    logMessage(QString("Taking ownership of %1 as user %2...").arg(mountPoint, user));
    if (QMessageBox::question(this, tr("Take Ownership"), tr("This will recursively change the owner and permissions of all files on the drive to your user (chown -R and chmod -R 777). Continue?"), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;
    logMessage("⏳ Waiting for sudo password in terminal...");
    QString chownCmd = QString("sudo chown -R %1:%1 '%2' && sudo chmod -R 777 '%2'; echo; echo 'Press Enter to close...'; read").arg(user, mountPoint);
    QStringList terminals = {"konsole", "gnome-terminal", "kitty", "alacritty", "xterm"};
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
