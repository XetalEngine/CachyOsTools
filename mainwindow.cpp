#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QProcess>
#include <QMessageBox>
#include <QHeaderView>
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QClipboard>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSettings>
#include <QTimer>
#include <QDebug>
#include <QFileSystemModel>
#include <unistd.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , currentProcess(nullptr)
{
    ui->setupUi(this);
    ui->tabWidget->setCurrentWidget(ui->driveToolsTab);
    
    //hellow world 
    qDebug() << "Hello World";

    // Set window title
    setWindowTitle("CachyOS Tools - Linux System Manager");
    
    // Center the window on the primary screen
    QScreen *primaryScreen = QGuiApplication::primaryScreen();
    if (primaryScreen) {
        QRect screenGeometry = primaryScreen->geometry();
        QRect windowGeometry = this->geometry();
        
        int x = (screenGeometry.width() - windowGeometry.width()) / 2;
        int y = (screenGeometry.height() - windowGeometry.height()) / 2;
        
        // Ensure the window stays within screen bounds
        x = qMax(0, qMin(x, screenGeometry.width() - windowGeometry.width()));
        y = qMax(0, qMin(y, screenGeometry.height() - windowGeometry.height()));
        
        this->move(x, y);
    }
    
    // Setup the drive table
    setupDriveTable();
    
    // Initial drive refresh
    refreshDrives();
    
    // Update button states
    updateButtonStates();
    
    // Connect table selection changes
    connect(ui->drivesTable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::updateButtonStates);
    
    // Shell/alias management setup
    shellConfigFiles["bash"] = {QString::fromUtf8(getenv("HOME")) + "/.bash_aliases", QString::fromUtf8(getenv("HOME")) + "/.bashrc", QString::fromUtf8(getenv("HOME")) + "/.bash_profile"};
    shellConfigFiles["zsh"] = {QString::fromUtf8(getenv("HOME")) + "/.zshrc"};
    shellConfigFiles["fish"] = {QString::fromUtf8(getenv("HOME")) + "/.config/fish/config.fish"};
    shellConfigFiles["tcsh"] = {QString::fromUtf8(getenv("HOME")) + "/.tcshrc", QString::fromUtf8(getenv("HOME")) + "/.cshrc"};
    shellConfigFiles["csh"] = {QString::fromUtf8(getenv("HOME")) + "/.cshrc"};
    shellConfigFiles["ksh"] = {QString::fromUtf8(getenv("HOME")) + "/.kshrc", QString::fromUtf8(getenv("HOME")) + "/.profile"};
    detectShellAndConfig();
    loadAliases();
    populateAliasTable();
    connect(ui->reloadAliasButton, &QPushButton::clicked, this, &MainWindow::on_reloadAliasButton_clicked);
    connect(ui->addAliasButton, &QPushButton::clicked, this, &MainWindow::on_addAliasButton_clicked);
    connect(ui->editAliasButton, &QPushButton::clicked, this, &MainWindow::on_editAliasButton_clicked);
    connect(ui->removeAliasButton, &QPushButton::clicked, this, &MainWindow::on_removeAliasButton_clicked);

    // Service management setup
    connect(ui->userServicesToggle, &QCheckBox::stateChanged, this, &MainWindow::on_userServicesToggle_stateChanged);
    connect(ui->serviceSearchBar, &QLineEdit::textChanged, this, &MainWindow::on_serviceSearchBar_textChanged);
    connect(ui->startServiceButton, &QPushButton::clicked, this, &MainWindow::on_startServiceButton_clicked);
    connect(ui->stopServiceButton, &QPushButton::clicked, this, &MainWindow::on_stopServiceButton_clicked);
    connect(ui->restartServiceButton, &QPushButton::clicked, this, &MainWindow::on_restartServiceButton_clicked);
    connect(ui->enableServiceButton, &QPushButton::clicked, this, &MainWindow::on_enableServiceButton_clicked);
    connect(ui->disableServiceButton, &QPushButton::clicked, this, &MainWindow::on_disableServiceButton_clicked);
    refreshServices();

    // Tweaks tab: NumLock at startup
    //connect(ui->numlockStartupCheckbox, &QCheckBox::toggled, this, &MainWindow::on_numlockStartupCheckbox_toggled);
    //loadNumlockSetting();
    
    // Config files tab setup
    setupConfigFiles();
    populateConfigTable();
    connect(ui->configTable, &QTableWidget::cellDoubleClicked, this, &MainWindow::on_configTable_cellDoubleClicked);
    
    // Log files tab setup
    logProcess = nullptr;
    setupLogFiles();
    populateLogComboBox();
    connect(ui->refreshLogsButton, &QPushButton::clicked, this, &MainWindow::on_refreshLogsButton_clicked);
    connect(ui->clearLogButton, &QPushButton::clicked, this, &MainWindow::on_clearLogButton_clicked);
    connect(ui->applyFilterButton, &QPushButton::clicked, this, &MainWindow::on_applyFilterButton_clicked);
    connect(ui->copyLogButton, &QPushButton::clicked, this, &MainWindow::on_copyLogButton_clicked);
    connect(ui->saveLogButton, &QPushButton::clicked, this, &MainWindow::on_saveLogButton_clicked);
    connect(ui->logFileComboBox, &QComboBox::currentTextChanged, this, &MainWindow::on_logFileComboBox_currentTextChanged);

    // Tweaks tab connections (ensure only one connection per button)
    disconnect(ui->zramToggle, nullptr, nullptr, nullptr);
    disconnect(ui->ipv6Toggle, nullptr, nullptr, nullptr);
    disconnect(ui->trimToggle, nullptr, nullptr, nullptr);
    disconnect(ui->tmpfsToggle, nullptr, nullptr, nullptr);
    disconnect(ui->showHiddenFilesToggle, nullptr, nullptr, nullptr);
    disconnect(ui->mitigationsToggle, nullptr, nullptr, nullptr);
    disconnect(ui->performanceHacksToggle, nullptr, nullptr, nullptr);
    connect(ui->zramToggle, &QPushButton::clicked, this, &MainWindow::on_zramToggle_clicked);
    connect(ui->ipv6Toggle, &QPushButton::clicked, this, &MainWindow::on_ipv6Toggle_clicked);
    connect(ui->trimToggle, &QPushButton::clicked, this, &MainWindow::on_trimToggle_clicked);
    connect(ui->tmpfsToggle, &QPushButton::clicked, this, &MainWindow::on_tmpfsToggle_clicked);
    connect(ui->dnsToggle, &QPushButton::clicked, this, &MainWindow::on_dnsToggle_clicked);
    connect(ui->showHiddenFilesToggle, &QPushButton::clicked, this, &MainWindow::on_showHiddenFilesToggle_clicked);
    connect(ui->mitigationsToggle, &QPushButton::clicked, this, &MainWindow::on_mitigationsToggle_clicked);
    connect(ui->performanceHacksToggle, &QPushButton::clicked, this, &MainWindow::on_performanceHacksToggle_clicked);
    connect(ui->ptraceToggle, &QPushButton::clicked, this, &MainWindow::on_ptraceToggle_clicked);
    connect(ui->hidepidToggle, &QPushButton::clicked, this, &MainWindow::on_hidepidToggle_clicked);
    connect(ui->cpuGovernorToggle, &QPushButton::clicked, this, &MainWindow::on_cpuGovernorToggle_clicked);

    // Tweaks tab connections (ensure only one connection per button)
    disconnect(ui->zramConfigButton, nullptr, nullptr, nullptr);
    disconnect(ui->cpuGovernorConfigButton, nullptr, nullptr, nullptr);
    disconnect(ui->ipv6ConfigButton, nullptr, nullptr, nullptr);
    disconnect(ui->trimConfigButton, nullptr, nullptr, nullptr);
    disconnect(ui->tmpfsConfigButton, nullptr, nullptr, nullptr);
    disconnect(ui->dnsConfigButton, nullptr, nullptr, nullptr);
    disconnect(ui->showHiddenFilesConfigButton, nullptr, nullptr, nullptr);
    disconnect(ui->mitigationsConfigButton, nullptr, nullptr, nullptr);
    disconnect(ui->performanceHacksConfigButton1, nullptr, nullptr, nullptr);
    disconnect(ui->performanceHacksConfigButton2, nullptr, nullptr, nullptr);
    disconnect(ui->ptraceConfigButton, nullptr, nullptr, nullptr);
    disconnect(ui->hidepidConfigButton, nullptr, nullptr, nullptr);
    connect(ui->zramConfigButton, &QPushButton::clicked, this, &MainWindow::on_zramConfigButton_clicked);
    connect(ui->cpuGovernorConfigButton, &QPushButton::clicked, this, &MainWindow::on_cpuGovernorConfigButton_clicked);
    connect(ui->ipv6ConfigButton, &QPushButton::clicked, this, &MainWindow::on_ipv6ConfigButton_clicked);
    connect(ui->trimConfigButton, &QPushButton::clicked, this, &MainWindow::on_trimConfigButton_clicked);
    connect(ui->tmpfsConfigButton, &QPushButton::clicked, this, &MainWindow::on_tmpfsConfigButton_clicked);
    connect(ui->dnsConfigButton, &QPushButton::clicked, this, &MainWindow::on_dnsConfigButton_clicked);
    connect(ui->showHiddenFilesConfigButton, &QPushButton::clicked, this, &MainWindow::on_showHiddenFilesConfigButton_clicked);
    connect(ui->mitigationsConfigButton, &QPushButton::clicked, this, &MainWindow::on_mitigationsConfigButton_clicked);
    connect(ui->performanceHacksConfigButton1, &QPushButton::clicked, this, &MainWindow::on_performanceHacksConfigButton1_clicked);
    connect(ui->performanceHacksConfigButton2, &QPushButton::clicked, this, &MainWindow::on_performanceHacksConfigButton2_clicked);
    connect(ui->ptraceConfigButton, &QPushButton::clicked, this, &MainWindow::on_ptraceConfigButton_clicked);
    connect(ui->hidepidConfigButton, &QPushButton::clicked, this, &MainWindow::on_hidepidConfigButton_clicked);
    
    // Tweak state process removed - now using instruction dialogs
    
    // Tweaks state population removed - now using instruction dialogs instead

    // Backup tab setup
    backupProcess = nullptr;
    sourceFsModel = nullptr;
    destFsModel = nullptr;
    sourceMountPoint = "/";
    destMountPoint = "";
    backupInProgress = false;
    populateBackupDrives();
    connect(ui->destDriveCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::on_destDriveCombo_currentIndexChanged);
    connect(ui->startBackupButton, &QPushButton::clicked, this, &MainWindow::on_startBackupButton_clicked);
    connect(ui->sourcePathTree, &QTreeView::clicked, this, &MainWindow::on_sourcePathTree_clicked);
    connect(ui->destPathTree, &QTreeView::clicked, this, &MainWindow::on_destPathTree_clicked);
    ui->backupProgressBar->setValue(0);
    ui->backupSummaryLabel->setText("Select destination to begin system backup.");

    // Restore tab setup
    restoreSourceFsModel = nullptr;
    restoreDestFsModel = nullptr;
    restoreSourceMountPoint = "";
    restoreDestMountPoint = "";
    populateRestoreDrives();
    connect(ui->restoreSourceDriveCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::on_restoreSourceDriveCombo_currentIndexChanged);
    connect(ui->restoreDestDriveCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::on_restoreDestDriveCombo_currentIndexChanged);
    connect(ui->startRestoreButton, &QPushButton::clicked, this, &MainWindow::on_startRestoreButton_clicked);
    connect(ui->restoreSourcePathTree, &QTreeView::clicked, this, &MainWindow::on_restoreSourcePathTree_clicked);
    connect(ui->restoreDestPathTree, &QTreeView::clicked, this, &MainWindow::on_restoreDestPathTree_clicked);
    ui->restoreProgressBar->setValue(0);
    ui->restoreSummaryLabel->setText("Select backup source and restore destination.");

    // In MainWindow constructor, after ui->setupUi(this):
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        if (ui->tabWidget->widget(index)->objectName() == "backupTab") {
            populateBackupDrives();
        }
        if (ui->tabWidget->widget(index)->objectName() == "restoreTab") {
            populateRestoreDrives();
        }
    });
}

MainWindow::~MainWindow()
{
    if (currentProcess) {
        currentProcess->kill();
        delete currentProcess;
    }
    if (logProcess) {
        if (logProcess->state() != QProcess::NotRunning) {
            logProcess->terminate();
            if (!logProcess->waitForFinished(2000)) { // wait 2 seconds
                logProcess->kill();
                logProcess->waitForFinished(1000);
            }
        }
        delete logProcess;
        logProcess = nullptr;
    }

    if (backupProcess) {
        backupProcess->kill();
        delete backupProcess;
        backupProcess = nullptr;
    }
    if (sourceFsModel) delete sourceFsModel;
    if (destFsModel) delete destFsModel;
    if (restoreSourceFsModel) delete restoreSourceFsModel;
    if (restoreDestFsModel) delete restoreDestFsModel;
    delete ui;
}

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

void MainWindow::executeCommand(const QString &command, const QStringList &arguments)
{
    if (currentProcess) {
        currentProcess->kill();
        delete currentProcess;
    }
    
    currentProcess = new QProcess(this);
    
    connect(currentProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::onProcessFinished);
    connect(currentProcess, &QProcess::errorOccurred,
            this, &MainWindow::onProcessError);
    
    currentProcess->start(command, arguments);
}

void MainWindow::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QString output = QString::fromUtf8(currentProcess->readAllStandardOutput());
    QString error = QString::fromUtf8(currentProcess->readAllStandardError());
    
    if (exitCode == 0) {
        if (currentProcess->program() == "lsblk") {
            logMessage("lsblk output:\n" + output); // Log raw output for debugging
            drivesList = parseDriveList(output);
            populateDriveTable();
            logMessage("✅ Drive list refreshed successfully");
        } else {
            logMessage("✅ Command completed successfully");
            // Refresh drive list after mount/unmount operations
            QTimer::singleShot(1000, this, &MainWindow::refreshDrives);
        }
    } else {
        logMessage("❌ Command failed with exit code: " + QString::number(exitCode));
        if (!error.isEmpty()) {
            logMessage("Error: " + error);
        }
    }
    
    delete currentProcess;
    currentProcess = nullptr;
}

void MainWindow::onProcessError(QProcess::ProcessError error)
{
    QString errorMsg;
    switch (error) {
        case QProcess::FailedToStart:
            errorMsg = "Failed to start command";
            break;
        case QProcess::Crashed:
            errorMsg = "Command crashed";
            break;
        case QProcess::Timedout:
            errorMsg = "Command timed out";
            break;
        case QProcess::WriteError:
            errorMsg = "Write error";
            break;
        case QProcess::ReadError:
            errorMsg = "Read error";
            break;
        case QProcess::UnknownError:
            errorMsg = "Unknown error";
            break;
    }
    
    logMessage("❌ Process error: " + errorMsg);
    delete currentProcess;
    currentProcess = nullptr;
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

void MainWindow::populateDriveTable()
{
    ui->drivesTable->setRowCount(0);
    
    for (const DriveInfo &drive : drivesList) {
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
    if (row >= 0 && row < drivesList.size()) {
        return drivesList[row].device;
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



void MainWindow::showTweakInstructions(const QString &title, const QString &instructions)
{
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(title);
    dialog->setModal(false);
    dialog->resize(800, 600);
    
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    
    QTextEdit *textEdit = new QTextEdit(dialog);
    textEdit->setPlainText(instructions);
    textEdit->setReadOnly(true);
    textEdit->setFont(QFont("Monospace", 10));
    layout->addWidget(textEdit);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *copyButton = new QPushButton("Copy to Clipboard", dialog);
    QPushButton *closeButton = new QPushButton("Close", dialog);
    
    connect(copyButton, &QPushButton::clicked, [textEdit]() {
        QApplication::clipboard()->setText(textEdit->toPlainText());
    });
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    
    buttonLayout->addWidget(copyButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    layout->addLayout(buttonLayout);
    
    dialog->show();
}

void MainWindow::detectShellAndConfig()
{
    detectedShell.clear();
    detectedConfigFile.clear();
    // Detect shell from $SHELL
    QString shellPath = QProcessEnvironment::systemEnvironment().value("SHELL");
    if (!shellPath.isEmpty()) {
        detectedShell = QFileInfo(shellPath).fileName();
    }
    // Fallback: check /etc/passwd
    if (detectedShell.isEmpty()) {
        detectedShell = "bash";
    }
    // Find config file
    for (const QString &file : shellConfigFiles.value(detectedShell)) {
        if (QFile::exists(file)) {
            detectedConfigFile = file;
            break;
        }
    }
    // Fallback: let user pick
    if (detectedConfigFile.isEmpty()) {
        detectedConfigFile = QFileDialog::getOpenFileName(this, "Select Shell Config File", QDir::homePath());
    }
    ui->shellLabel->setText("Detected Shell: " + detectedShell + "\nConfig: " + detectedConfigFile);
}

void MainWindow::loadAliases()
{
    aliasList.clear();
    if (detectedConfigFile.isEmpty()) return;
    QFile file(detectedConfigFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.startsWith("alias ") || (detectedShell == "fish" && (line.startsWith("abbr ") || line.startsWith("function ")))) {
            AliasEntry entry = parseAliasLine(line, detectedShell);
            if (!entry.name.isEmpty()) aliasList.append(entry);
        }
    }
    file.close();
}

AliasEntry MainWindow::parseAliasLine(const QString &line, const QString &shell)
{
    AliasEntry entry;
    if (shell == "fish") {
        // fish: abbr NAME "COMMAND" or function NAME
        if (line.startsWith("abbr ")) {
            QRegularExpression rx("abbr \\s+(\\S+) \"(.+)\"");
            QRegularExpressionMatch match = rx.match(line);
            if (match.hasMatch()) {
                entry.name = match.captured(1);
                entry.command = match.captured(2);
            }
        } else if (line.startsWith("function ")) {
            QRegularExpression rx("function \\s+(\\S+)");
            QRegularExpressionMatch match = rx.match(line);
            if (match.hasMatch()) {
                entry.name = match.captured(1);
                entry.command = "(function)";
            }
        }
    } else if (shell == "csh" || shell == "tcsh") {
        // csh/tcsh: alias NAME 'COMMAND'
        QRegularExpression rx("alias \\s+(\\S+) \\s+'(.+)'$");
        QRegularExpressionMatch match = rx.match(line);
        if (match.hasMatch()) {
            entry.name = match.captured(1);
            entry.command = match.captured(2);
        }
    } else {
        // bash/zsh/ksh: alias NAME='COMMAND'
        QRegularExpression rx("alias \\s*(\\S+)='(.+)'$");
        QRegularExpressionMatch match = rx.match(line);
        if (match.hasMatch()) {
            entry.name = match.captured(1);
            entry.command = match.captured(2);
        }
    }
    return entry;
}

void MainWindow::populateAliasTable()
{
    ui->aliasTable->setRowCount(0);
    ui->aliasTable->setColumnCount(2);
    QStringList headers;
    headers << "Alias" << "Command";
    ui->aliasTable->setHorizontalHeaderLabels(headers);
    
    // Set column sizing behavior
    ui->aliasTable->horizontalHeader()->setStretchLastSection(true);  // Command column stretches
    ui->aliasTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);  // Alias column auto-resizes
    ui->aliasTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);  // Command column stretches
    
    for (const AliasEntry &entry : aliasList) {
        int row = ui->aliasTable->rowCount();
        ui->aliasTable->insertRow(row);
        ui->aliasTable->setItem(row, 0, new QTableWidgetItem(entry.name));
        ui->aliasTable->setItem(row, 1, new QTableWidgetItem(entry.command));
    }
}

void MainWindow::on_addAliasButton_clicked() {
    bool ok1, ok2;
    QString name = QInputDialog::getText(this, "Add Alias", "Alias name:", QLineEdit::Normal, "", &ok1);
    if (!ok1 || name.trimmed().isEmpty()) return;
    QString command = QInputDialog::getText(this, "Add Alias", "Alias command:", QLineEdit::Normal, "", &ok2);
    if (!ok2 || command.trimmed().isEmpty()) return;
    // Check for duplicate
    for (const AliasEntry &entry : aliasList) {
        if (entry.name == name) {
            QMessageBox::warning(this, "Duplicate Alias", "An alias with this name already exists.");
            return;
        }
    }
    AliasEntry newEntry{name, command};
    aliasList.append(newEntry);
    saveAliases();
    populateAliasTable();
}

void MainWindow::on_editAliasButton_clicked() {
    int row = ui->aliasTable->currentRow();
    if (row < 0 || row >= aliasList.size()) return;
    AliasEntry &entry = aliasList[row];
    bool ok1, ok2;
    QString name = QInputDialog::getText(this, "Edit Alias", "Alias name:", QLineEdit::Normal, entry.name, &ok1);
    if (!ok1 || name.trimmed().isEmpty()) return;
    QString command = QInputDialog::getText(this, "Edit Alias", "Alias command:", QLineEdit::Normal, entry.command, &ok2);
    if (!ok2 || command.trimmed().isEmpty()) return;
    // Check for duplicate (except self)
    for (int i = 0; i < aliasList.size(); ++i) {
        if (i != row && aliasList[i].name == name) {
            QMessageBox::warning(this, "Duplicate Alias", "An alias with this name already exists.");
            return;
        }
    }
    entry.name = name;
    entry.command = command;
    saveAliases();
    populateAliasTable();
}

void MainWindow::on_removeAliasButton_clicked() {
    int row = ui->aliasTable->currentRow();
    if (row < 0 || row >= aliasList.size()) return;
    if (QMessageBox::question(this, "Remove Alias", "Are you sure you want to remove this alias?") != QMessageBox::Yes) return;
    aliasList.removeAt(row);
    saveAliases();
    populateAliasTable();
}

void MainWindow::on_reloadAliasButton_clicked() {
    loadAliases();
    populateAliasTable();
    if (!detectedConfigFile.isEmpty()) {
        // Open a new Konsole window, source the config, start a new shell, and close when the shell exits
        QString command = QString("%1 -c 'source %2; %1; exit'").arg(detectedShell, detectedConfigFile);
        QProcess::startDetached("konsole", QStringList() << "-e" << command);
    }
}

QString MainWindow::aliasToLine(const AliasEntry &alias, const QString &shell) {
    if (shell == "fish") {
        return QString("abbr %1 \"%2\"").arg(alias.name, alias.command);
    } else if (shell == "csh" || shell == "tcsh") {
        return QString("alias %1 '%2'").arg(alias.name, alias.command);
    } else {
        return QString("alias %1='%2'").arg(alias.name, alias.command);
    }
}

void MainWindow::saveAliases() {
    if (detectedConfigFile.isEmpty()) return;
    QFile file(detectedConfigFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QStringList lines;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        // Only keep lines that are not aliases
        if (!(line.trimmed().startsWith("alias ") || (detectedShell == "fish" && (line.trimmed().startsWith("abbr ") || line.trimmed().startsWith("function "))))) {
            lines << line;
        }
    }
    file.close();
    // Add all current aliases
    for (const AliasEntry &entry : aliasList) {
        lines << aliasToLine(entry, detectedShell);
    }
    // Write back
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) return;
    QTextStream out(&file);
    for (const QString &line : lines) {
        out << line << "\n";
    }
    file.close();
}

void MainWindow::refreshServices() {
    serviceList.clear();
    QStringList args;
    if (showUserServices) {
        args << "--user";
    }
    args << "list-units" << "--type=service" << "--all" << "--no-pager" << "--no-legend";
    QProcess proc;
    proc.start("systemctl", args);
    proc.waitForFinished();
    QString output = proc.readAllStandardOutput();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        // Format: UNIT LOAD ACTIVE SUB DESCRIPTION
        // Example: systemd-journald.service loaded active running Journal Service
        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() < 5) continue;
        int offset = 0;
        // If the first part is not a service (doesn't end with .service), skip it
        if (!parts[0].endsWith(".service")) {
            offset = 1;
        }
        if (parts.size() < 5 + offset) continue;
        ServiceEntry entry;
        entry.name = parts[0 + offset];
        entry.loadState = parts[1 + offset];
        entry.activeState = parts[2 + offset];
        entry.subState = parts[3 + offset];
        entry.description = parts.mid(4 + offset).join(" ");
        // Get enabled/disabled
        QProcess enabledProc;
        QStringList enabledArgs;
        if (showUserServices) enabledArgs << "--user";
        enabledArgs << "is-enabled" << entry.name;
        enabledProc.start("systemctl", enabledArgs);
        enabledProc.waitForFinished();
        entry.enabled = QString(enabledProc.readAllStandardOutput()).trimmed();
        if (entry.enabled == "not-found") entry.enabled = "N/A";
        entry.status = entry.activeState;
        serviceList.append(entry);
    }
    populateServicesTable();
}

void MainWindow::populateServicesTable() {
    ui->servicesTable->setRowCount(0);
    ui->servicesTable->setColumnCount(5);
    QStringList headers;
    headers << "Service" << "Description" << "Status" << "Enabled" << "SubState";
    ui->servicesTable->setHorizontalHeaderLabels(headers);
    // Set column sizing behavior
    ui->servicesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive); // Service
    ui->servicesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); // Description (make this the biggest)
    ui->servicesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive); // Status
    ui->servicesTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive); // Enabled
    ui->servicesTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Interactive); // SubState
    for (const ServiceEntry &entry : serviceList) {
        int row = ui->servicesTable->rowCount();
        ui->servicesTable->insertRow(row);
        ui->servicesTable->setItem(row, 0, new QTableWidgetItem(entry.name));
        ui->servicesTable->setItem(row, 1, new QTableWidgetItem(entry.description));
        ui->servicesTable->setItem(row, 2, new QTableWidgetItem(entry.status));
        ui->servicesTable->setItem(row, 3, new QTableWidgetItem(entry.enabled));
        ui->servicesTable->setItem(row, 4, new QTableWidgetItem(entry.subState));
    }
    // Optionally, set default widths for the small columns
    ui->servicesTable->setColumnWidth(0, 120); // Service
    ui->servicesTable->setColumnWidth(2, 100); // Status
    ui->servicesTable->setColumnWidth(3, 80);  // Enabled
    ui->servicesTable->setColumnWidth(4, 100); // SubState
}

void MainWindow::filterServicesTable(const QString &filter) {
    for (int row = 0; row < ui->servicesTable->rowCount(); ++row) {
        bool match = false;
        for (int col = 0; col < ui->servicesTable->columnCount(); ++col) {
            QTableWidgetItem *item = ui->servicesTable->item(row, col);
            if (item && item->text().contains(filter, Qt::CaseInsensitive)) {
                match = true;
                break;
            }
        }
        ui->servicesTable->setRowHidden(row, !match);
    }
}

void MainWindow::on_userServicesToggle_stateChanged(int state) {
    showUserServices = (state == Qt::Checked);
    refreshServices();
}

void MainWindow::on_serviceSearchBar_textChanged(const QString &text) {
    filterServicesTable(text);
}

void MainWindow::handleServiceAction(const QString &action) {
    int row = ui->servicesTable->currentRow();
    if (row < 0 || row >= serviceList.size()) return;
    QString service = serviceList[row].name;
    QStringList args;
    if (showUserServices) args << "--user";
    args << action << service;
    QProcess proc;
    proc.start("systemctl", args);
    proc.waitForFinished();
    refreshServices();
}

void MainWindow::on_startServiceButton_clicked() { handleServiceAction("start"); }
void MainWindow::on_stopServiceButton_clicked() { handleServiceAction("stop"); }
void MainWindow::on_restartServiceButton_clicked() { handleServiceAction("restart"); }
void MainWindow::on_enableServiceButton_clicked() { handleServiceAction("enable"); }
void MainWindow::on_disableServiceButton_clicked() { handleServiceAction("disable"); }

// void MainWindow::loadNumlockSetting() {
//     QSettings settings(QDir::homePath() + "/.config/kcminputrc", QSettings::IniFormat);
//     settings.beginGroup("Keyboard");
//     int numlock = settings.value("NumLock", 2).toInt();
//     settings.endGroup();
//     ui->numlockStartupCheckbox->blockSignals(true);
//     ui->numlockStartupCheckbox->setChecked(numlock == 1);
//     ui->numlockStartupCheckbox->blockSignals(false);
// }

void MainWindow::setNumlockSetting(bool enabled) {
    QString value = enabled ? "1" : "0";
    QProcess::startDetached("kwriteconfig5", QStringList() << "--file" << "kcminputrc" << "--group" << "Keyboard" << "--key" << "NumLock" << value);
}



void MainWindow::setupConfigFiles() {
    configFileList.clear();
    
    // System Configuration
    configFileList.append({"/etc/pacman.conf", "Pacman settings and enabling parallel downloads or custom repos (like chaotic-aur)", "System Configuration"});
    configFileList.append({"/etc/mkinitcpio.conf", "Initramfs settings, especially after installing kernel modules or changing hooks", "System Configuration"});
    configFileList.append({"/etc/default/grub", "GRUB bootloader settings (e.g., timeout, kernel params). Requires grub-mkconfig after edits", "System Configuration"});
    configFileList.append({"/etc/locale.gen", "Uncomment your locale (e.g., en_US.UTF-8) in locale.gen, then run locale-gen", "System Configuration"});
    configFileList.append({"/etc/locale.conf", "System locale configuration", "System Configuration"});
    configFileList.append({"/etc/vconsole.conf", "Console font and keymap (TTY)", "System Configuration"});
    configFileList.append({"/etc/hostname", "Set your system's hostname", "System Configuration"});
    configFileList.append({"/etc/hosts", "Matching entries for localhost", "System Configuration"});
    
    // User Environment
    configFileList.append({QDir::homePath() + "/.bashrc", "Shell configuration, aliases, prompt, etc.", "User Environment"});
    configFileList.append({QDir::homePath() + "/.zshrc", "Shell configuration, aliases, prompt, etc. (zsh is default in CachyOS)", "User Environment"});
    configFileList.append({QDir::homePath() + "/.profile", "Environment variables for X11 sessions", "User Environment"});
    configFileList.append({QDir::homePath() + "/.xprofile", "Environment variables for X11 sessions", "User Environment"});
    configFileList.append({QDir::homePath() + "/.config/fish/config.fish", "If using fish shell", "User Environment"});
    configFileList.append({QDir::homePath() + "/.config/gtk-3.0/settings.ini", "GTK theme tweaks for GTK-based apps", "User Environment"});
    configFileList.append({QDir::homePath() + "/.config/xdg-user-dirs.conf", "User directory mappings (e.g., Downloads → ~/dl)", "User Environment"});
    configFileList.append({QDir::homePath() + "/.Xresources", "X11 terminal font/colors (e.g., for urxvt)", "User Environment"});
    configFileList.append({QDir::homePath() + "/.Xdefaults", "X11 terminal font/colors (e.g., for urxvt)", "User Environment"});
    
    // Desktop Environment / Window Manager
    configFileList.append({QDir::homePath() + "/.config/kwinrc", "KWin window manager settings", "Desktop Environment"});
    configFileList.append({QDir::homePath() + "/.config/kscreenlockerrc", "KDE Plasma screen locker settings", "Desktop Environment"});
    configFileList.append({QDir::homePath() + "/.config/plasmarc", "KDE Plasma settings", "Desktop Environment"});
    configFileList.append({QDir::homePath() + "/.config/plasmashellrc", "KDE Plasma shell settings", "Desktop Environment"});
    configFileList.append({"/etc/sddm.conf", "SDDM display manager theme and behavior", "Desktop Environment"});
    configFileList.append({QDir::homePath() + "/.config/compiz/", "Compiz settings, if using", "Desktop Environment"});
    
    // Performance and Kernel
    configFileList.append({"/etc/sysctl.d/", "Kernel parameters (e.g., vm.swappiness)", "Performance and Kernel"});
    configFileList.append({"/etc/modprobe.d/", "Module options (e.g., disabling nouveau)", "Performance and Kernel"});
    configFileList.append({"/etc/X11/xorg.conf.d/", "Manual GPU/input driver configs", "Performance and Kernel"});
    configFileList.append({"/etc/udev/rules.d/", "Custom udev rules (for devices, permissions)", "Performance and Kernel"});
    
    // Systemd and Services
    configFileList.append({QDir::homePath() + "/.config/systemd/user/", "User-specific services (e.g., overlay auto-launch)", "Systemd and Services"});
    configFileList.append({"/etc/systemd/system/", "System-wide services", "Systemd and Services"});
    configFileList.append({QDir::homePath() + "/.config/autostart/", "Apps to auto-start in GUI", "Systemd and Services"});
    
    // KVM/QEMU/Libvirt & GPU Passthrough
    configFileList.append({"/etc/default/grub", "Kernel parameters for IOMMU, vfio, GPU isolation, etc. (intel_iommu=on, amd_iommu=on, vfio-pci.ids, etc.)", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/etc/mkinitcpio.conf", "Add vfio modules to initramfs for passthrough (MODULES=(... vfio vfio_iommu_type1 vfio_pci vfio_virqfd))", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/etc/modprobe.d/vfio.conf", "Bind devices to vfio-pci for passthrough (options vfio-pci ids=...)", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/etc/modprobe.d/blacklist.conf", "Blacklist drivers (nouveau, radeon, etc.) for passthrough devices", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/etc/modules-load.d/vfio.conf", "Ensure vfio modules load at boot", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/etc/grub.d/40_custom", "Custom GRUB entries or hooks for GPU selection", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/etc/libvirt/qemu.conf", "Libvirt QEMU daemon config (user/group, security, cgroup)", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/etc/libvirt/qemu/<vmname>.xml", "VM-specific XML config (edit with virsh edit <vmname>)", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/etc/libvirt/hooks/qemu", "Custom scripts for VM start/stop (GPU/PCI rebinding, audio switching)", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/etc/libvirt/hooks/qemu.d/", "Hook scripts for advanced automation", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/usr/share/OVMF/OVMF_VARS.fd", "UEFI NVRAM for Windows VMs (copy and edit for each VM if needed)", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/etc/udev/rules.d/", "Custom udev rules for device permissions, binding, or isolation", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/etc/X11/xorg.conf.d/", "Manual GPU assignment, disabling X on passthrough GPU", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/etc/sysctl.d/99-kvm.conf", "Kernel tweaks for virtualization (e.g., kernel.nmi_watchdog=0)", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/etc/security/limits.conf", "Resource limits for QEMU/libvirt users", "KVM/QEMU/GPU Passthrough"});
    
    // Optional but Common on CachyOS
    configFileList.append({"/etc/reflector.conf", "Controls mirror selection by reflector", "CachyOS Specific"});
    configFileList.append({"/etc/xdg/reflector/reflector.conf", "Alternate location depending on version", "CachyOS Specific"});
    configFileList.append({"/etc/paru.conf", "AUR helper configuration (if using paru)", "CachyOS Specific"});
    configFileList.append({QDir::homePath() + "/.config/paru/paru.conf", "AUR helper configuration (if using paru)", "CachyOS Specific"});
}

void MainWindow::populateConfigTable() {
    ui->configTable->setRowCount(0);
    ui->configTable->setColumnCount(3);
    QStringList headers;
    headers << "File Path" << "Description" << "Edit";
    ui->configTable->setHorizontalHeaderLabels(headers);
    
    // Set column sizing behavior
    ui->configTable->horizontalHeader()->setStretchLastSection(false);
    ui->configTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);  // File Path
    ui->configTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);  // Description stretches
    ui->configTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);  // Edit button fixed width
    
    ui->configTable->setColumnWidth(2, 80); // Edit button width
    
    for (const ConfigFileEntry &entry : configFileList) {
        int row = ui->configTable->rowCount();
        ui->configTable->insertRow(row);
        
        // File path
        ui->configTable->setItem(row, 0, new QTableWidgetItem(entry.filePath));
        
        // Description
        ui->configTable->setItem(row, 1, new QTableWidgetItem(entry.description));
        
        // Edit button
        QPushButton *editButton = new QPushButton("Edit");
        editButton->setProperty("filePath", entry.filePath);
        connect(editButton, &QPushButton::clicked, [this, entry]() {
            editConfigFile(entry.filePath);
        });
        ui->configTable->setCellWidget(row, 2, editButton);
    }
}

void MainWindow::editConfigFile(const QString &filePath) {
    QString editor = ui->editorComboBox->currentText();
    
    // Check if file exists
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        QMessageBox::warning(this, "File Not Found", 
                           QString("The file '%1' does not exist.\n\nThis might be a directory or the file hasn't been created yet.").arg(filePath));
        return;
    }
    
    // Prepare the command
    QString command;
    if (editor == "kate") {
        command = QString("kate --new '%1'").arg(filePath);
    } else {
        command = QString("%1 '%2'").arg(editor, filePath);
    }
    
    // Check if file needs sudo (system files)
    bool needsSudo = filePath.startsWith("/etc/") || filePath.startsWith("/usr/") || filePath.startsWith("/var/");
    
    if (needsSudo) {
        command = QString("sudo %1").arg(command);
    }
    
    // Open a new terminal window with the command
    QStringList terminals = {"konsole", "gnome-terminal", "xterm", "alacritty", "kitty"};
    bool launched = false;
    
    for (const QString &terminal : terminals) {
        if (QProcess::startDetached(terminal, QStringList() << "-e" << "bash" << "-c" << command)) {
            launched = true;
            break;
        }
    }
    
    if (!launched) {
        QMessageBox::warning(this, "Terminal Not Found", 
                           QString("Could not find a suitable terminal emulator.\n\nPlease install one of: konsole, gnome-terminal, xterm, alacritty, or kitty"));
    }
}

void MainWindow::on_configTable_cellDoubleClicked(int row, int column) {
    if (column == 0 || column == 1) { // File path or description columns
        if (row >= 0 && row < configFileList.size()) {
            editConfigFile(configFileList[row].filePath);
        }
    }
}

void MainWindow::setupLogFiles() {
    logFileList.clear();
    
    // System logs
    logFileList.append({"/var/log/messages", "General system messages", "System Logs", false, 0});
    logFileList.append({"/var/log/syslog", "System log messages", "System Logs", false, 0});
    logFileList.append({"/var/log/kern.log", "Kernel log messages", "System Logs", false, 0});
    logFileList.append({"/var/log/auth.log", "Authentication and authorization logs", "System Logs", false, 0});
    logFileList.append({"/var/log/dmesg", "Kernel ring buffer messages", "System Logs", false, 0});
    logFileList.append({"/var/log/daemon.log", "System daemon logs", "System Logs", false, 0});
    logFileList.append({"/var/log/user.log", "User-level messages", "System Logs", false, 0});
    
    // Boot and startup logs
    logFileList.append({"/var/log/boot.log", "Boot process messages", "Boot Logs", false, 0});
    logFileList.append({"/var/log/wtmp", "User login/logout records", "Boot Logs", false, 0});
    logFileList.append({"/var/log/btmp", "Failed login attempts", "Boot Logs", false, 0});
    logFileList.append({"/var/log/lastlog", "Last login information", "Boot Logs", false, 0});
    
    // Package management logs
    logFileList.append({"/var/log/pacman.log", "Pacman package manager logs", "Package Logs", false, 0});
    logFileList.append({"/var/log/paru.log", "Paru AUR helper logs", "Package Logs", false, 0});
    
    // X11 and display logs
    logFileList.append({"/var/log/Xorg.0.log", "X11 server logs", "Display Logs", false, 0});
    logFileList.append({"/var/log/Xorg.0.log.old", "Previous X11 server logs", "Display Logs", false, 0});
    
    // Network logs
    logFileList.append({"/var/log/network.log", "Network service logs", "Network Logs", false, 0});
    logFileList.append({"/var/log/ufw.log", "Uncomplicated Firewall logs", "Network Logs", false, 0});
    
    // Application logs
    logFileList.append({"/var/log/cups/error_log", "CUPS printing system errors", "Application Logs", false, 0});
    logFileList.append({"/var/log/cups/access_log", "CUPS printing system access", "Application Logs", false, 0});
    logFileList.append({"/var/log/lightdm/lightdm.log", "LightDM display manager logs", "Application Logs", false, 0});
    logFileList.append({"/var/log/sddm.log", "SDDM display manager logs", "Application Logs", false, 0});
    
    // Systemd journal logs (special handling)
    logFileList.append({"/var/log/journal/", "Systemd journal logs", "Systemd Logs", false, 0});
    
    // System information commands
    logFileList.append({"inxi", "Complete system information (hardware, drivers, etc.)", "System Info", false, 0});
    logFileList.append({"inxi -G", "Graphics/GPU information", "System Info", false, 0});
    logFileList.append({"inxi -C", "CPU information", "System Info", false, 0});
    logFileList.append({"inxi -M", "Machine information", "System Info", false, 0});
    logFileList.append({"inxi -D", "Disk/Storage information", "System Info", false, 0});
    logFileList.append({"inxi -N", "Network information", "System Info", false, 0});
    logFileList.append({"inxi -A", "Audio information", "System Info", false, 0});
    
    // GPU and graphics logs
    logFileList.append({"/var/log/gpu-manager.log", "GPU manager logs", "GPU Logs", false, 0});
    logFileList.append({"/var/log/nvidia-installer.log", "NVIDIA driver installation logs", "GPU Logs", false, 0});
    logFileList.append({"/var/log/amd-installer.log", "AMD driver installation logs", "GPU Logs", false, 0});
    
    // Security logs
    logFileList.append({"/var/log/audit/audit.log", "Audit system logs", "Security Logs", false, 0});
    logFileList.append({"/var/log/secure", "Security-related messages", "Security Logs", false, 0});
    
    // Check which logs actually exist
    for (LogFileEntry &entry : logFileList) {
        if (entry.filePath.startsWith("inxi")) {
            // For inxi commands, check if inxi is available
            QProcess checkProcess;
            checkProcess.start("which", QStringList() << "inxi");
            checkProcess.waitForFinished();
            entry.exists = (checkProcess.exitCode() == 0);
            entry.size = 0; // Commands don't have a file size
        } else {
            QFileInfo fileInfo(entry.filePath);
            entry.exists = fileInfo.exists();
            if (entry.exists) {
                entry.size = fileInfo.size();
            }
        }
    }
}

void MainWindow::populateLogComboBox() {
    ui->logFileComboBox->clear();
    
    for (const LogFileEntry &entry : logFileList) {
        if (entry.exists) {
            QString displayText = QString("%1 (%2)").arg(entry.filePath, entry.description);
            ui->logFileComboBox->addItem(displayText, entry.filePath);
        }
    }
    
    if (ui->logFileComboBox->count() > 0) {
        ui->logFileComboBox->setCurrentIndex(0);
        loadLogContent();
    }
}

void MainWindow::loadLogContent() {
    QString selectedLog = ui->logFileComboBox->currentData().toString();
    if (selectedLog.isEmpty()) return;
    
    ui->logStatusLabel->setText("Loading...");
    
    // Kill any existing log process (defensive)
    if (logProcess) {
        if (logProcess->state() != QProcess::NotRunning) {
            logProcess->terminate();
            if (!logProcess->waitForFinished(2000)) {
                logProcess->kill();
                logProcess->waitForFinished(1000);
            }
        }
        delete logProcess;
        logProcess = nullptr;
    }
    
    logProcess = new QProcess(this);
    connect(logProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::onLogProcessFinished);
    
    QStringList args;
    int lines = ui->logLinesSpinBox->value();
    QString filter = ui->logFilterEdit->text();
    
    if (selectedLog == "/var/log/journal/") {
        // Special handling for systemd journal
        args << "-n" << QString::number(lines);
        if (!filter.isEmpty()) {
            args << "-g" << filter;
        }
        logProcess->start("journalctl", args);
    } else if (selectedLog.startsWith("inxi")) {
        // Special handling for inxi commands
        QStringList inxiArgs = selectedLog.split(" ", Qt::SkipEmptyParts);
        if (inxiArgs.size() > 1) {
            // Remove "inxi" from the beginning and use the rest as arguments
            inxiArgs.removeFirst();
            logProcess->start("inxi", inxiArgs);
        } else {
            logProcess->start("inxi", QStringList());
        }
    } else {
        // Regular log files
        if (ui->tailLogCheckBox->isChecked()) {
            args << "-f" << "-n" << QString::number(lines);
        } else {
            args << "-n" << QString::number(lines);
        }
        
        if (!filter.isEmpty()) {
            args << "grep" << filter;
            logProcess->start("tail", args);
        } else {
            args << selectedLog;
            logProcess->start("tail", args);
        }
    }
}

void MainWindow::onLogProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    QString output = QString::fromUtf8(logProcess->readAllStandardOutput());
    QString error = QString::fromUtf8(logProcess->readAllStandardError());
    
    if (exitCode == 0 || !output.isEmpty()) {
        ui->logContentTextEdit->setPlainText(output);
        ui->logStatusLabel->setText(QString("Loaded %1 lines").arg(output.count('\n')));
    } else {
        ui->logContentTextEdit->setPlainText("Error loading log file or no content found.");
        ui->logStatusLabel->setText("Error");
    }
    
    delete logProcess;
    logProcess = nullptr;
}

void MainWindow::clearLogFile(const QString &filePath) {
    if (filePath.isEmpty()) return;
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Clear Log File",
        QString("Are you sure you want to clear '%1'?\n\nThis action cannot be undone.").arg(filePath),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        QProcess::startDetached("sudo", QStringList() << "sh" << "-c" << QString("echo '' > '%1'").arg(filePath));
        loadLogContent(); // Reload the content
    }
}

void MainWindow::searchLogFiles() {
    // This could be extended to search across multiple log files
    loadLogContent();
}

void MainWindow::applyLogFilters() {
    loadLogContent();
}

// Log management slots
void MainWindow::on_refreshLogsButton_clicked() {
    setupLogFiles();
    populateLogComboBox();
}

void MainWindow::on_clearLogButton_clicked() {
    QString selectedLog = ui->logFileComboBox->currentData().toString();
    clearLogFile(selectedLog);
}

void MainWindow::on_applyFilterButton_clicked() {
    applyLogFilters();
}

void MainWindow::on_copyLogButton_clicked() {
    QString content = ui->logContentTextEdit->toPlainText();
    if (!content.isEmpty()) {
        QApplication::clipboard()->setText(content);
        ui->logStatusLabel->setText("Copied to clipboard");
    }
}

void MainWindow::on_saveLogButton_clicked() {
    QString content = ui->logContentTextEdit->toPlainText();
    if (content.isEmpty()) return;
    
    QString fileName = QFileDialog::getSaveFileName(this, "Save Log Content", 
        QString("log_%1.txt").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "Text Files (*.txt);;All Files (*)");
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << content;
            file.close();
            ui->logStatusLabel->setText("Saved to " + fileName);
        } else {
            QMessageBox::warning(this, "Error", "Could not save file.");
        }
    }
}

void MainWindow::on_logFileComboBox_currentTextChanged(const QString &text) {
    Q_UNUSED(text)
    loadLogContent();
}

// Helper to open a config file in nano in a terminal with sudo (shared by all tabs)
void MainWindow::openConfigInNano(const QString &filePath) {
    QString command = QString("sudo nano '%1'").arg(filePath);
    QStringList terminals = {"konsole", "gnome-terminal", "xterm", "alacritty", "kitty"};
    for (const QString &terminal : terminals) {
        if (QProcess::startDetached(terminal, QStringList() << "-e" << "bash" << "-c" << command)) {
            return;
        }
    }
    QMessageBox::warning(this, "Terminal Not Found", "Could not find a suitable terminal emulator. Please install one of: konsole, gnome-terminal, xterm, alacritty, or kitty");
}

// Helper to run a sudo command in a terminal
void MainWindow::runSudoCommandInTerminal(const QString &cmd) {
    QStringList terminals = {"konsole", "gnome-terminal", "xterm", "alacritty", "kitty"};
    for (const QString &terminal : terminals) {
        if (QProcess::startDetached(terminal, QStringList() << "-e" << "bash" << "-c" << cmd)) {
            return;
        }
    }
    QMessageBox::warning(this, "Terminal Not Found", "Could not find a suitable terminal emulator. Please install one of: konsole, gnome-terminal, xterm, alacritty, or kitty");
}

// Tweaks tab slots
void MainWindow::on_zramToggle_clicked() {
    QString instructions = R"(
# ZRAM Configuration Instructions
# ==============================
# 
# ZRAM creates compressed swap space in RAM, which can improve performance on systems with limited RAM.
# 
# To ENABLE ZRAM:
# Click the "Edit Config" button to create/modify the config file
# Copy ONLY the following content to /etc/systemd/zram-generator.conf:

[zram0]
zram-size = ram / 2

# Then run these commands:
# sudo systemctl daemon-reload
# sudo systemctl enable systemd-zram-setup@zram0.service
# sudo systemctl start systemd-zram-setup@zram0.service
# 
# To DISABLE ZRAM:
# sudo systemctl stop systemd-zram-setup@zram0.service
# sudo systemctl disable systemd-zram-setup@zram0.service
# sudo rm -f /etc/systemd/zram-generator.conf
# 
# To check if ZRAM is active:
# cat /proc/swaps
# lsblk | grep zram
)";
    
    showTweakInstructions("ZRAM Configuration", instructions);
}
void MainWindow::on_zramConfigButton_clicked() {
    // Check if zram config exists and show current state
    QFile zramConfig("/etc/systemd/zram-generator.conf");
    QString message = zramConfig.exists() ? "Opening existing ZRAM config file for editing." : "Creating new ZRAM config file for editing.";
    openConfigInNano("/etc/systemd/zram-generator.conf");
}
void MainWindow::on_cpuGovernorToggle_clicked() {
    QString instructions = R"(
# CPU Governor Configuration Instructions
# ===================================
# 
# CPU governors control how the CPU scales its frequency based on load.
# 
# To set PERFORMANCE governor (maximum performance, higher power consumption):
# Click the "Edit Config" button to create/modify the config file
# Copy this content to /etc/default/cpupower:

GOVERNOR="performance"

# Then run this command:
# sudo systemctl restart cpupower
# 
# To set POWERSAVE governor (lower performance, better power efficiency):
# Click the "Edit Config" button to create/modify the config file
# Copy this content to /etc/default/cpupower:

GOVERNOR="powersave"

# Then run this command:
# sudo systemctl restart cpupower
# 
# To check current governor:
# cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
# 
# Available governors on your system:
# cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors
# 
# Note: Some systems may have additional governors like 'schedutil', 'ondemand', etc.
)";
    
    showTweakInstructions("CPU Governor Configuration", instructions);
}
void MainWindow::on_cpuGovernorConfigButton_clicked() {
    // Check current governor and show state
    QProcess proc;
    proc.start("cat", QStringList() << "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
    proc.waitForFinished();
    QString currentGovernor = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    openConfigInNano("/etc/default/cpupower");
}
void MainWindow::on_ipv6Toggle_clicked() {
    QString instructions = R"(
# IPv6 Configuration Instructions
# ==============================
# 
# IPv6 can be enabled or disabled system-wide. Some applications may require IPv6 to function properly.
# 
# To ENABLE IPv6:
# Click the "Edit Config" button to create/modify the config file
# Copy this content to /etc/sysctl.d/40-ipv6.conf:

net.ipv6.conf.all.disable_ipv6 = 0
net.ipv6.conf.default.disable_ipv6 = 0

# Then run this command:
# sudo sysctl -p /etc/sysctl.d/40-ipv6.conf
# 
# To DISABLE IPv6:
# Click the "Edit Config" button to create/modify the config file
# Copy this content to /etc/sysctl.d/40-ipv6.conf:

net.ipv6.conf.all.disable_ipv6 = 1
net.ipv6.conf.default.disable_ipv6 = 1

# Then run this command:
# sudo sysctl -p /etc/sysctl.d/40-ipv6.conf
# 
# To check current IPv6 status:
# sysctl net.ipv6.conf.all.disable_ipv6
# sysctl net.ipv6.conf.default.disable_ipv6
)";
    
    showTweakInstructions("IPv6 Configuration", instructions);
}
void MainWindow::on_ipv6ConfigButton_clicked() {
    // Check current IPv6 state
    QProcess proc;
    proc.start("sysctl", QStringList() << "-n" << "net.ipv6.conf.all.disable_ipv6");
    proc.waitForFinished();
    QString ipv6Disabled = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    QString status = (ipv6Disabled == "1") ? "disabled" : "enabled";
    openConfigInNano("/etc/sysctl.d/40-ipv6.conf");
}
void MainWindow::on_trimToggle_clicked() {
    QString instructions = R"(
# TRIM Configuration Instructions
# ==============================
# 
# TRIM helps maintain SSD performance by informing the drive which blocks are no longer in use.
# 
# To ENABLE TRIM:
# sudo systemctl enable fstrim.timer
# sudo systemctl start fstrim.timer
# 
# To DISABLE TRIM:
# sudo systemctl stop fstrim.timer
# sudo systemctl disable fstrim.timer
# 
# To manually run TRIM once:
# sudo fstrim -v /
# 
# To check if TRIM is supported by your SSD:
# sudo fstrim -v /
)";
    
    showTweakInstructions("TRIM Configuration", instructions);
}
void MainWindow::on_trimConfigButton_clicked() {
    // Check if fstrim timer is enabled
    QProcess proc;
    proc.start("systemctl", QStringList() << "is-enabled" << "fstrim.timer");
    proc.waitForFinished();
    QString status = (proc.exitCode() == 0) ? "enabled" : "disabled";
    openConfigInNano("/etc/fstab");
}
void MainWindow::on_tmpfsToggle_clicked() {
    QString instructions = R"(
# tmpfs Configuration Instructions
# ===============================
# 
# Mounting /tmp as tmpfs (RAM disk) can improve performance by keeping temporary files in RAM.
# 
# To ENABLE tmpfs for /tmp:
# Click the "Edit Config" button to create/modify the config file
# Add this line to /etc/fstab:

/tmp tmpfs tmpfs defaults,noatime,mode=1777 0 0

# Then reboot the system for changes to take effect:
# sudo reboot
# 
# To DISABLE tmpfs for /tmp:
# Click the "Edit Config" button to create/modify the config file
# Remove the tmpfs line from /etc/fstab
# 
# Then reboot the system for changes to take effect:
# sudo reboot
# 
# To check current /tmp mount status:
# findmnt /tmp
# mount | grep tmp
# 
# To temporarily mount /tmp as tmpfs (without editing fstab):
# sudo mount -t tmpfs -o defaults,noatime,mode=1777 tmpfs /tmp
)";
    
    showTweakInstructions("tmpfs Configuration", instructions);
}
void MainWindow::on_tmpfsConfigButton_clicked() {
    // Check if /tmp is mounted as tmpfs
    QProcess proc;
    proc.start("findmnt", QStringList() << "/tmp" << "-o" << "FSTYPE" << "--noheadings");
    proc.waitForFinished();
    QString fsType = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    QString status = (fsType == "tmpfs") ? "mounted as tmpfs" : "not mounted as tmpfs";
    openConfigInNano("/etc/fstab");
}
void MainWindow::on_dnsToggle_clicked() {
    QString instructions = R"(
# DNS Configuration Instructions
# ============================
# 
# DNS (Domain Name System) configuration controls how your system resolves domain names to IP addresses.
# 
# To configure DNS servers manually:
# Click the "Edit Config" button to create/modify the config file
# Add or modify nameserver entries in /etc/resolv.conf:

nameserver 8.8.8.8
nameserver 8.8.4.4
nameserver 1.1.1.1

# Common DNS servers:
# - Google DNS: 8.8.8.8, 8.8.4.4
# - Cloudflare: 1.1.1.1, 1.0.0.1
# - OpenDNS: 208.67.222.222, 208.67.220.220
# - Quad9: 9.9.9.9, 149.112.112.112
# 
# To use your ISP's DNS servers (automatic):
# Remove or comment out all nameserver lines
# 
# To check current DNS configuration:
# cat /etc/resolv.conf
# 
# To test DNS resolution:
# nslookup google.com
# dig google.com
# 
# Note: Some systems use systemd-resolved or NetworkManager which may override /etc/resolv.conf
# Check if systemd-resolved is active: systemctl is-active systemd-resolved
)";
    
    showTweakInstructions("DNS Configuration", instructions);
}

void MainWindow::on_dnsConfigButton_clicked() {
    openConfigInNano("/etc/resolv.conf");
}
void MainWindow::on_showHiddenFilesToggle_clicked() {
    QString instructions = R"(
# Hidden Files Configuration Instructions
# =====================================
# 
# Configure file managers to show hidden files by default.
# 
# For KDE Dolphin:
# Click the "Edit Config" button to create/modify the config file
# Add or modify the [General] section in ~/.config/dolphinrc:

[General]
ShowHiddenFiles=true

# To hide hidden files by default, set ShowHiddenFiles=false
# 
# For GNOME Files (Nautilus):
# Open GNOME Files
# Press Ctrl+H to toggle hidden files
# Go to Edit > Preferences > Views
# Check "Show hidden and backup files"
# 
# For other file managers:
# - Thunar (Xfce): View > Show Hidden Files
# - PCManFM: View > Show Hidden
# - Ranger: Press 'zh' to toggle hidden files
# 
# To show all hidden files in terminal:
# ls -la
)";
    
    showTweakInstructions("Hidden Files Configuration", instructions);
}
void MainWindow::on_showHiddenFilesConfigButton_clicked() {
    // Check current Dolphin settings
    QString path = QDir::homePath() + "/.config/dolphinrc";
    QSettings settings(path, QSettings::IniFormat);
    settings.beginGroup("General");
    bool showHidden = settings.value("ShowHiddenFiles", false).toBool();
    settings.endGroup();
    QString status = showHidden ? "enabled" : "disabled";
    openConfigInNano(QDir::homePath() + "/.config/dolphinrc");
}
void MainWindow::on_mitigationsToggle_clicked() {
    QString instructions = R"(
# Security Mitigations Configuration Instructions
# ============================================
# 
# Spectre/Meltdown mitigations protect against CPU vulnerabilities but may impact performance.
# 
# To ENABLE mitigations (recommended for security):
# Click the "Edit Config" button to create/modify the config file
# Remove 'mitigations=off' from GRUB_CMDLINE_LINUX if present:
# GRUB_CMDLINE_LINUX="... other options ..."
# 
# Then update GRUB configuration:
# sudo grub-mkconfig -o /boot/grub/grub.cfg
# 
# Reboot for changes to take effect:
# sudo reboot
# 
# To DISABLE mitigations (for performance, not recommended):
# Click the "Edit Config" button to create/modify the config file
# Add 'mitigations=off' to GRUB_CMDLINE_LINUX:
# GRUB_CMDLINE_LINUX="... other options ... mitigations=off"
# 
# Then update GRUB configuration:
# sudo grub-mkconfig -o /boot/grub/grub.cfg
# 
# Reboot for changes to take effect:
# sudo reboot
# 
# To check current mitigation status:
# cat /sys/devices/system/cpu/vulnerabilities/*
)";
    
    showTweakInstructions("Security Mitigations Configuration", instructions);
}

void MainWindow::on_performanceHacksToggle_clicked() {
    QString instructions = R"(
# Performance Hacks Configuration Instructions
# ========================================
# 
# These performance optimizations are contributed by forAUR from CachyOS forums.
# 
# 1. Making locate locating again:
# sudo pacman -S plocate
# 
# Click the "Edit Config" button to edit /etc/updatedb.conf:
PRUNE_BIND_MOUNTS = "no"
PRUNEFS = ""
PRUNENAMES = ".git .hg .svn"
PRUNEPATHS = ""

# 2. Gas, gas, gas! (Set performance mode):
# sudo powerprofilesctl set performance

# 3. Remove plymouth from boot (faster boot):
# Click the "Edit Config" button to edit /etc/mkinitcpio.conf
# Remove 'plymouth' from the HOOKS line

# Then run these commands:
# sudo pacman -Rns cachyos-plymouth-bootanimation plymouth-kcm plymouth
# sudo mkinitcpio -P

# Credits: forAUR from CachyOS forums
)";
    
    showTweakInstructions("Performance Hacks Configuration", instructions);
}

void MainWindow::on_performanceHacksConfigButton1_clicked() {
    // Open the updatedb.conf config file for plocate configuration
    openConfigInNano("/etc/updatedb.conf");
}

void MainWindow::on_performanceHacksConfigButton2_clicked() {
    // Open the mkinitcpio.conf config file for removing plymouth
    openConfigInNano("/etc/mkinitcpio.conf");
}
void MainWindow::on_mitigationsConfigButton_clicked() {
    // Check current mitigations state
    QProcess proc;
    proc.start("grep", QStringList() << "-q" << "mitigations=off" << "/etc/default/grub");
    QString status = (proc.exitCode() == 0) ? "disabled" : "enabled";
    openConfigInNano("/etc/default/grub");
}

void MainWindow::on_ptraceToggle_clicked() {
    QString instructions = R"(
# ptrace Configuration Instructions
# ===============================
# 
# ptrace is a system call used by debuggers and some security tools to trace processes.
# By default, non-root users cannot use ptrace on processes they don't own.
# 
# To ENABLE ptrace for non-root users (allows debugging and security tools):
# Click the "Edit Config" button to create/modify the config file
# Copy this content to /etc/sysctl.d/10-ptrace.conf:

kernel.yama.ptrace_scope = 0

# Then run this command to apply the changes:
# sudo sysctl -p /etc/sysctl.d/10-ptrace.conf
# 
# To DISABLE ptrace for non-root users (more secure, prevents debugging):
# Click the "Edit Config" button to create/modify the config file
# Copy this content to /etc/sysctl.d/10-ptrace.conf:

kernel.yama.ptrace_scope = 1

# Then run this command to apply the changes:
# sudo sysctl -p /etc/sysctl.d/10-ptrace.conf
# 
# ptrace_scope values:
# 0 - No restrictions (any process can ptrace any other process)
# 1 - Restricted ptrace (only parent processes can ptrace their children)
# 2 - Admin-only ptrace (only root can use ptrace)
# 3 - No ptrace (ptrace is completely disabled)
# 
# To check current ptrace scope:
# sysctl kernel.yama.ptrace_scope
# 
# Note: Some debugging tools, security scanners, and development tools require ptrace access.
# Disabling ptrace can break these tools but improves security.
)";
    
    showTweakInstructions("ptrace Configuration", instructions);
}

void MainWindow::on_ptraceConfigButton_clicked() {
    // Check current ptrace scope
    QProcess proc;
    proc.start("sysctl", QStringList() << "-n" << "kernel.yama.ptrace_scope");
    proc.waitForFinished();
    QString currentScope = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    QString status;
    switch (currentScope.toInt()) {
        case 0: status = "unrestricted"; break;
        case 1: status = "restricted"; break;
        case 2: status = "admin-only"; break;
        case 3: status = "disabled"; break;
        default: status = "unknown"; break;
    }
    openConfigInNano("/etc/sysctl.d/10-ptrace.conf");
}

void MainWindow::on_hidepidToggle_clicked() {
    QString instructions = R"(
# hidepid Configuration Instructions
# ================================
# 
# hidepid is a mount option for /proc that controls process visibility.
# It prevents users from seeing processes they don't own, improving security.
# 
# To ENABLE hidepid (more secure, hides other users' processes):
# Click the "Edit Config" button to create/modify the config file
# Find the /proc line in /etc/fstab and add hidepid=2:
# 
# proc /proc proc defaults,hidepid=2 0 0
# 
# Then remount /proc to apply changes:
# sudo mount -o remount /proc
# 
# To DISABLE hidepid (less secure, shows all processes):
# Click the "Edit Config" button to create/modify the config file
# Find the /proc line in /etc/fstab and remove hidepid=2:
# 
# proc /proc proc defaults 0 0
# 
# Then remount /proc to apply changes:
# sudo mount -o remount /proc
# 
# hidepid values:
# 0 - No hiding (default, all processes visible)
# 1 - Hide processes from other users (processes not owned by current user)
# 2 - Hide processes from other users and hide /proc/PID directories
# 
# To check current hidepid setting:
# mount | grep proc
# 
# To test if hidepid is working:
# ls /proc | grep -E '^[0-9]+$' | head -5
# 
# Note: Some system monitoring tools and debugging utilities may not work
# properly with hidepid enabled. This is a security vs functionality trade-off.
# 
# Security benefits:
# - Prevents process enumeration attacks
# - Hides sensitive process information
# - Reduces information leakage
)";
    
    showTweakInstructions("hidepid Configuration", instructions);
}

void MainWindow::on_hidepidConfigButton_clicked() {
    // Check current hidepid setting
    QProcess proc;
    proc.start("mount", QStringList());
    proc.waitForFinished();
    QString mountOutput = QString::fromUtf8(proc.readAllStandardOutput());
    QString status = "not set";
    
    if (mountOutput.contains("hidepid=1")) {
        status = "hidepid=1 (hide other users' processes)";
    } else if (mountOutput.contains("hidepid=2")) {
        status = "hidepid=2 (hide other users' processes and directories)";
    } else if (mountOutput.contains("proc")) {
        status = "hidepid=0 (no hiding, all processes visible)";
    }
    
    openConfigInNano("/etc/fstab");
}

// Tweaks tab state checking functions removed - now using instruction dialogs instead

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
        if (mount.startsWith("/mnt") || mount.startsWith("/media") || mount.isEmpty()) continue;
        drives.append(qMakePair(mount, label));
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

void MainWindow::on_takeOwnershipButton_clicked()
{
    int row = ui->drivesTable->currentRow();
    if (row < 0) {
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

// ISO Creator tab functions
void MainWindow::on_createIsoButton_clicked()
{
    // TODO: Implement ISO creation functionality
    ui->isoStatusLabel->setText("ISO creation functionality not yet implemented.");
    ui->isoProgressBar->setValue(0);
}

void MainWindow::on_estimateSizeButton_clicked()
{
    // TODO: Implement size estimation functionality
    ui->isoStatusLabel->setText("Size estimation functionality not yet implemented.");
}

void MainWindow::on_loadPresetButton_clicked()
{
    // TODO: Implement preset loading functionality
    ui->isoStatusLabel->setText("Preset loading functionality not yet implemented.");
}

void MainWindow::on_savePresetButton_clicked()
{
    // TODO: Implement preset saving functionality
    ui->isoStatusLabel->setText("Preset saving functionality not yet implemented.");
}

void MainWindow::on_isoOutputBrowseButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Output Directory", 
                                                   QDir::homePath() + "/Downloads");
    if (!dir.isEmpty()) {
        ui->isoOutputEdit->setText(dir);
    }
}
