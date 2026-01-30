MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) , ui(new Ui::MainWindow) , currentProcess(nullptr)
{
    ui->setupUi(this);
    
    // Set current tab first before applying visibility preferences
    ui->tabWidget->setCurrentWidget(ui->driveToolsTab);
    
    // Load all preferences after UI is fully set up and all connections are made
    // Use a longer delay to ensure all signal connections are established before modifying tabs
    QTimer::singleShot(500, this, [this]() {
        loadAllPreferences();
    });
    
    //hellow world 
    qDebug() << "Hello World";

    // Set window title
    setWindowTitle("Arch OS Tools - Linux System Manager");
    
    // Load window geometry if enabled, otherwise center the window
    if (loadWindowSizeEnabled()) {
        loadWindowGeometry();
    } else {
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
    
    // Connect drive filter checkboxes
    connect(ui->diskFilterCheckBox, &QCheckBox::stateChanged, this, &MainWindow::on_diskFilterCheckBox_stateChanged);
    connect(ui->partitionFilterCheckBox, &QCheckBox::stateChanged, this, &MainWindow::on_partitionFilterCheckBox_stateChanged);
    connect(ui->minSizeFilterCheckBox, &QCheckBox::stateChanged, this, &MainWindow::on_minSizeFilterCheckBox_stateChanged);
    connect(ui->minSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::on_minSizeSpinBox_valueChanged);
    
    // Shell/alias management setup
    // shellConfigFiles["bash"] = {QString::fromUtf8(getenv("HOME")) + "/.bash_aliases", QString::fromUtf8(getenv("HOME")) + "/.bashrc", QString::fromUtf8(getenv("HOME")) + "/.bash_profile"};
    // shellConfigFiles["zsh"] = {QString::fromUtf8(getenv("HOME")) + "/.zshrc"};
    // shellConfigFiles["fish"] = {QString::fromUtf8(getenv("HOME")) + "/.config/fish/config.fish"};
    // shellConfigFiles["tcsh"] = {QString::fromUtf8(getenv("HOME")) + "/.tcshrc", QString::fromUtf8(getenv("HOME")) + "/.cshrc"};
    // shellConfigFiles["csh"] = {QString::fromUtf8(getenv("HOME")) + "/.cshrc"};
    // shellConfigFiles["ksh"] = {QString::fromUtf8(getenv("HOME")) + "/.kshrc", QString::fromUtf8(getenv("HOME")) + "/.profile"};

    shellConfigFiles["bash"] = QStringList()
<< QString::fromUtf8(getenv("HOME")) + "/.bash_aliases"
<< QString::fromUtf8(getenv("HOME")) + "/.bashrc"
<< QString::fromUtf8(getenv("HOME")) + "/.bash_profile";

shellConfigFiles["zsh"] = QStringList()
<< QString::fromUtf8(getenv("HOME")) + "/.zshrc";

shellConfigFiles["fish"] = QStringList()
<< QString::fromUtf8(getenv("HOME")) + "/.config/fish/config.fish";

shellConfigFiles["tcsh"] = QStringList()
<< QString::fromUtf8(getenv("HOME")) + "/.tcshrc"
<< QString::fromUtf8(getenv("HOME")) + "/.cshrc";

shellConfigFiles["csh"] = QStringList()
<< QString::fromUtf8(getenv("HOME")) + "/.cshrc";

shellConfigFiles["ksh"] = QStringList()
<< QString::fromUtf8(getenv("HOME")) + "/.kshrc"
<< QString::fromUtf8(getenv("HOME")) + "/.profile";

    
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
    connect(ui->applyFilterButton, &QPushButton::clicked, this, &MainWindow::on_applyFilterButton_clicked);
    connect(ui->copyLogButton, &QPushButton::clicked, this, &MainWindow::on_copyLogButton_clicked);
    connect(ui->saveLogButton, &QPushButton::clicked, this, &MainWindow::on_saveLogButton_clicked);
    connect(ui->logFileComboBox, &QComboBox::currentTextChanged, this, &MainWindow::on_logFileComboBox_currentTextChanged);
    
    // Package Manager tab setup
    checkAurHelpers();
    
    // Network tab initialization
    refreshNetworkInfo();
    refreshInterfaceStats();
    refreshBridges();
    refreshLibvirtNetworks();
    refreshInterfaceConfig();
    // Connect search buttons
    connect(ui->pacmanAurSearchButton, &QPushButton::clicked, this, &MainWindow::on_pacmanAurSearchButton_clicked);
    connect(ui->pacmanAurSearch, &QLineEdit::returnPressed, this, &MainWindow::on_pacmanAurSearch_returnPressed);
    connect(ui->yaySearchButton, &QPushButton::clicked, this, &MainWindow::on_yaySearchButton_clicked);
    connect(ui->yaySearch, &QLineEdit::returnPressed, this, &MainWindow::on_yaySearch_returnPressed);
    connect(ui->paruSearchButton, &QPushButton::clicked, this, &MainWindow::on_paruSearchButton_clicked);
    connect(ui->paruSearch, &QLineEdit::returnPressed, this, &MainWindow::on_paruSearch_returnPressed);
    // Auto-refresh installed packages when Package Manager tab is opened
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        if (ui->tabWidget->widget(index) == ui->packageManagerTab) {
            refreshPacmanInstalled();
        }
    });

    // Tweaks tab toggle button connections
    disconnect(ui->zramToggle, nullptr, nullptr, nullptr);
    disconnect(ui->ipv6Toggle, nullptr, nullptr, nullptr);
    disconnect(ui->trimToggle, nullptr, nullptr, nullptr);
    disconnect(ui->tmpfsToggle, nullptr, nullptr, nullptr);
    disconnect(ui->showHiddenFilesToggle, nullptr, nullptr, nullptr);
    disconnect(ui->mitigationsToggle, nullptr, nullptr, nullptr);
    disconnect(ui->performanceHacksToggle, nullptr, nullptr, nullptr);
    disconnect(ui->ptraceToggle, nullptr, nullptr, nullptr);
    disconnect(ui->hidepidToggle, nullptr, nullptr, nullptr);
    disconnect(ui->cpuGovernorToggle, nullptr, nullptr, nullptr);
    disconnect(ui->dnsToggle, nullptr, nullptr, nullptr);
    
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

    // Tweaks tab config button connections
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

    // ISO Creator offline mode setup
    offlinePackagePath = QDir::currentPath() + "/" + OFFLINE_PACKAGE_FILENAME;
    
    // Connect radio button signals
    connect(ui->onlineModeRadio, &QRadioButton::toggled, this, &MainWindow::on_onlineModeRadio_toggled);
    connect(ui->offlineModeRadio, &QRadioButton::toggled, this, &MainWindow::on_offlineModeRadio_toggled);
    
    // Disconnect any existing connections to prevent duplicates
    disconnect(ui->downloadOfflineButton, nullptr, this, nullptr);
    disconnect(ui->checkAvailabilityButton, nullptr, this, nullptr);
    
    connect(ui->downloadOfflineButton, &QPushButton::clicked, this, &MainWindow::on_downloadOfflineButton_clicked);
    connect(ui->checkAvailabilityButton, &QPushButton::clicked, this, &MainWindow::checkOfflinePackageAvailability);
    
    // Initialize offline status for online mode (default)
    ui->offlineStatusLabel->setText("Online mode selected - packages will be downloaded during ISO creation");
    ui->offlineStatusLabel->setStyleSheet("color: #666666;");
    ui->downloadOfflineButton->setVisible(false);
    ui->checkAvailabilityButton->setVisible(false);
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