/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.9.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionExit;
    QAction *actionAbout;
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QTabWidget *tabWidget;
    QWidget *driveToolsTab;
    QVBoxLayout *verticalLayout_2;
    QLabel *label;
    QTableWidget *drivesTable;
    QHBoxLayout *horizontalLayout;
    QPushButton *refreshButton;
    QPushButton *mountButton;
    QPushButton *forceMountButton;
    QPushButton *unmountButton;
    QPushButton *mount777Button;
    QPushButton *takeOwnershipButton;
    QSpacerItem *horizontalSpacer;
    QTextEdit *logOutput;
    QWidget *shellToolsTab;
    QVBoxLayout *verticalLayout_shell;
    QLabel *shellLabel;
    QTableWidget *aliasTable;
    QHBoxLayout *aliasButtonLayout;
    QPushButton *addAliasButton;
    QPushButton *editAliasButton;
    QPushButton *removeAliasButton;
    QPushButton *reloadAliasButton;
    QWidget *servicesTab;
    QVBoxLayout *verticalLayout_services;
    QHBoxLayout *servicesTopLayout;
    QLineEdit *serviceSearchBar;
    QCheckBox *userServicesToggle;
    QTableWidget *servicesTable;
    QHBoxLayout *servicesButtonLayout;
    QPushButton *startServiceButton;
    QPushButton *stopServiceButton;
    QPushButton *restartServiceButton;
    QPushButton *enableServiceButton;
    QPushButton *disableServiceButton;
    QWidget *tweaksTab;
    QVBoxLayout *verticalLayout_tweaks;
    QLabel *tweaksWarningLabel;
    QGridLayout *tweaksGridLayout;
    QCheckBox *zramToggle;
    QPushButton *zramConfigButton;
    QComboBox *cpuGovernorCombo;
    QPushButton *cpuGovernorConfigButton;
    QCheckBox *ipv6Toggle;
    QPushButton *ipv6ConfigButton;
    QCheckBox *trimToggle;
    QPushButton *trimConfigButton;
    QCheckBox *tmpfsToggle;
    QPushButton *tmpfsConfigButton;
    QPushButton *dnsConfigButton;
    QCheckBox *showHiddenFilesToggle;
    QPushButton *showHiddenFilesConfigButton;
    QCheckBox *mitigationsToggle;
    QPushButton *mitigationsConfigButton;
    QWidget *configTab;
    QVBoxLayout *verticalLayout_config;
    QHBoxLayout *configTopLayout;
    QLabel *configLabel;
    QComboBox *editorComboBox;
    QSpacerItem *configSpacer;
    QTableWidget *configTable;
    QWidget *logsTab;
    QVBoxLayout *verticalLayout_logs;
    QHBoxLayout *logsTopLayout;
    QLabel *logFileLabel;
    QComboBox *logFileComboBox;
    QPushButton *refreshLogsButton;
    QPushButton *clearLogButton;
    QHBoxLayout *logsFilterLayout;
    QLabel *filterLabel;
    QLineEdit *logFilterEdit;
    QLabel *linesLabel;
    QSpinBox *logLinesSpinBox;
    QCheckBox *tailLogCheckBox;
    QPushButton *applyFilterButton;
    QTextEdit *logContentTextEdit;
    QHBoxLayout *logsBottomLayout;
    QLabel *logStatusLabel;
    QSpacerItem *logsBottomSpacer;
    QPushButton *copyLogButton;
    QPushButton *saveLogButton;
    QWidget *backupTab;
    QVBoxLayout *verticalLayout_backup;
    QSplitter *backupSplitter;
    QWidget *backupSourceWidget;
    QVBoxLayout *verticalLayout_source;
    QLabel *sourceLabel;
    QLabel *sourcePathLabel;
    QTreeView *sourcePathTree;
    QWidget *backupDestWidget;
    QVBoxLayout *verticalLayout_dest;
    QLabel *destLabel;
    QComboBox *destDriveCombo;
    QLabel *destPathLabel;
    QTreeView *destPathTree;
    QHBoxLayout *backupBottomLayout;
    QLabel *backupSummaryLabel;
    QPushButton *startBackupButton;
    QProgressBar *backupProgressBar;
    QLabel *backupPathSummaryLabel;
    QWidget *restoreTab;
    QVBoxLayout *verticalLayout_restore;
    QSplitter *restoreSplitter;
    QWidget *restoreSourceWidget;
    QVBoxLayout *verticalLayout_restore_source;
    QLabel *restoreSourceLabel;
    QComboBox *restoreSourceDriveCombo;
    QLabel *restoreSourcePathLabel;
    QTreeView *restoreSourcePathTree;
    QWidget *restoreDestWidget;
    QVBoxLayout *verticalLayout_restore_dest;
    QLabel *restoreDestLabel;
    QComboBox *restoreDestDriveCombo;
    QLabel *restoreDestPathLabel;
    QTreeView *restoreDestPathTree;
    QHBoxLayout *restoreBottomLayout;
    QLabel *restoreSummaryLabel;
    QPushButton *startRestoreButton;
    QProgressBar *restoreProgressBar;
    QLabel *restorePathSummaryLabel;
    QWidget *isoCreatorTab;
    QVBoxLayout *verticalLayout_iso;
    QScrollArea *isoScrollArea;
    QWidget *isoScrollContent;
    QVBoxLayout *verticalLayout_iso_content;
    QGroupBox *isoBasicGroup;
    QVBoxLayout *verticalLayout_basic;
    QHBoxLayout *horizontalLayout_iso_name;
    QLabel *isoNameLabel;
    QLineEdit *isoNameEdit;
    QHBoxLayout *horizontalLayout_iso_output;
    QLabel *isoOutputLabel;
    QLineEdit *isoOutputEdit;
    QPushButton *isoOutputBrowseButton;
    QGroupBox *isoKernelGroup;
    QVBoxLayout *verticalLayout_kernel;
    QCheckBox *includeLinuxKernelCheck;
    QCheckBox *includeLinuxLtsKernelCheck;
    QCheckBox *includeLinuxZenKernelCheck;
    QCheckBox *includeLinuxHardenedKernelCheck;
    QCheckBox *includeCachyosKernelCheck;
    QGroupBox *isoDriversGroup;
    QVBoxLayout *verticalLayout_drivers;
    QCheckBox *includeAllWifiDriversCheck;
    QCheckBox *includeAllFirmwareCheck;
    QCheckBox *includeNvidiaDriversCheck;
    QCheckBox *includeAmdDriversCheck;
    QCheckBox *includeIntelDriversCheck;
    QCheckBox *includeBluetoothDriversCheck;
    QCheckBox *includeAudioDriversCheck;
    QGroupBox *isoSoftwareGroup;
    QVBoxLayout *verticalLayout_software;
    QCheckBox *includeCalamareInstallerCheck;
    QCheckBox *includeSystemToolsCheck;
    QCheckBox *includeNetworkToolsCheck;
    QCheckBox *includeMediaCodecsCheck;
    QCheckBox *includeOfficeSuiteCheck;
    QCheckBox *includeWebBrowserCheck;
    QGroupBox *isoDesktopGroup;
    QVBoxLayout *verticalLayout_desktop;
    QCheckBox *includeKdePlasmaCheck;
    QCheckBox *includeGnomeCheck;
    QCheckBox *includeXfceCheck;
    QCheckBox *includeI3wmCheck;
    QCheckBox *includeSwayCheck;
    QGroupBox *isoAdvancedGroup;
    QVBoxLayout *verticalLayout_advanced;
    QCheckBox *includeCustomConfigsCheck;
    QCheckBox *includeUserDataCheck;
    QCheckBox *includeDevelopmentToolsCheck;
    QCheckBox *includeGamingToolsCheck;
    QCheckBox *includeVirtualizationCheck;
    QHBoxLayout *horizontalLayout_compression;
    QLabel *compressionLabel;
    QComboBox *compressionCombo;
    QHBoxLayout *horizontalLayout_arch;
    QLabel *archLabel;
    QComboBox *archCombo;
    QGroupBox *isoSecurityGroup;
    QVBoxLayout *verticalLayout_security;
    QCheckBox *includeSecurityToolsCheck;
    QCheckBox *includeEncryptionToolsCheck;
    QCheckBox *includeFirewallCheck;
    QCheckBox *includeVpnToolsCheck;
    QHBoxLayout *horizontalLayout_iso_buttons;
    QPushButton *createIsoButton;
    QPushButton *estimateSizeButton;
    QPushButton *loadPresetButton;
    QPushButton *savePresetButton;
    QSpacerItem *horizontalSpacer_iso;
    QProgressBar *isoProgressBar;
    QLabel *isoStatusLabel;
    QMenuBar *menubar;
    QMenu *menuFile;
    QMenu *menuHelp;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1000, 700);
        actionExit = new QAction(MainWindow);
        actionExit->setObjectName("actionExit");
        actionAbout = new QAction(MainWindow);
        actionAbout->setObjectName("actionAbout");
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName("verticalLayout");
        tabWidget = new QTabWidget(centralwidget);
        tabWidget->setObjectName("tabWidget");
        driveToolsTab = new QWidget();
        driveToolsTab->setObjectName("driveToolsTab");
        verticalLayout_2 = new QVBoxLayout(driveToolsTab);
        verticalLayout_2->setObjectName("verticalLayout_2");
        label = new QLabel(driveToolsTab);
        label->setObjectName("label");
        QFont font;
        font.setPointSize(12);
        font.setBold(true);
        label->setFont(font);

        verticalLayout_2->addWidget(label);

        drivesTable = new QTableWidget(driveToolsTab);
        drivesTable->setObjectName("drivesTable");
        drivesTable->setAlternatingRowColors(true);
        drivesTable->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
        drivesTable->setSortingEnabled(true);

        verticalLayout_2->addWidget(drivesTable);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        refreshButton = new QPushButton(driveToolsTab);
        refreshButton->setObjectName("refreshButton");
        refreshButton->setMinimumSize(QSize(100, 35));

        horizontalLayout->addWidget(refreshButton);

        mountButton = new QPushButton(driveToolsTab);
        mountButton->setObjectName("mountButton");
        mountButton->setMinimumSize(QSize(100, 35));

        horizontalLayout->addWidget(mountButton);

        forceMountButton = new QPushButton(driveToolsTab);
        forceMountButton->setObjectName("forceMountButton");
        forceMountButton->setMinimumSize(QSize(120, 35));

        horizontalLayout->addWidget(forceMountButton);

        unmountButton = new QPushButton(driveToolsTab);
        unmountButton->setObjectName("unmountButton");
        unmountButton->setMinimumSize(QSize(100, 35));

        horizontalLayout->addWidget(unmountButton);

        mount777Button = new QPushButton(driveToolsTab);
        mount777Button->setObjectName("mount777Button");
        mount777Button->setMinimumSize(QSize(100, 35));

        horizontalLayout->addWidget(mount777Button);

        takeOwnershipButton = new QPushButton(driveToolsTab);
        takeOwnershipButton->setObjectName("takeOwnershipButton");
        takeOwnershipButton->setMinimumSize(QSize(100, 35));

        horizontalLayout->addWidget(takeOwnershipButton);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);


        verticalLayout_2->addLayout(horizontalLayout);

        logOutput = new QTextEdit(driveToolsTab);
        logOutput->setObjectName("logOutput");
        logOutput->setMaximumSize(QSize(16777215, 150));
        logOutput->setReadOnly(true);

        verticalLayout_2->addWidget(logOutput);

        tabWidget->addTab(driveToolsTab, QString());
        shellToolsTab = new QWidget();
        shellToolsTab->setObjectName("shellToolsTab");
        verticalLayout_shell = new QVBoxLayout(shellToolsTab);
        verticalLayout_shell->setObjectName("verticalLayout_shell");
        shellLabel = new QLabel(shellToolsTab);
        shellLabel->setObjectName("shellLabel");
        QFont font1;
        font1.setPointSize(11);
        font1.setBold(true);
        shellLabel->setFont(font1);

        verticalLayout_shell->addWidget(shellLabel);

        aliasTable = new QTableWidget(shellToolsTab);
        aliasTable->setObjectName("aliasTable");
        aliasTable->setAlternatingRowColors(true);
        aliasTable->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
        aliasTable->setSortingEnabled(true);

        verticalLayout_shell->addWidget(aliasTable);

        aliasButtonLayout = new QHBoxLayout();
        aliasButtonLayout->setObjectName("aliasButtonLayout");
        addAliasButton = new QPushButton(shellToolsTab);
        addAliasButton->setObjectName("addAliasButton");

        aliasButtonLayout->addWidget(addAliasButton);

        editAliasButton = new QPushButton(shellToolsTab);
        editAliasButton->setObjectName("editAliasButton");

        aliasButtonLayout->addWidget(editAliasButton);

        removeAliasButton = new QPushButton(shellToolsTab);
        removeAliasButton->setObjectName("removeAliasButton");

        aliasButtonLayout->addWidget(removeAliasButton);

        reloadAliasButton = new QPushButton(shellToolsTab);
        reloadAliasButton->setObjectName("reloadAliasButton");

        aliasButtonLayout->addWidget(reloadAliasButton);


        verticalLayout_shell->addLayout(aliasButtonLayout);

        tabWidget->addTab(shellToolsTab, QString());
        servicesTab = new QWidget();
        servicesTab->setObjectName("servicesTab");
        verticalLayout_services = new QVBoxLayout(servicesTab);
        verticalLayout_services->setObjectName("verticalLayout_services");
        servicesTopLayout = new QHBoxLayout();
        servicesTopLayout->setObjectName("servicesTopLayout");
        serviceSearchBar = new QLineEdit(servicesTab);
        serviceSearchBar->setObjectName("serviceSearchBar");

        servicesTopLayout->addWidget(serviceSearchBar);

        userServicesToggle = new QCheckBox(servicesTab);
        userServicesToggle->setObjectName("userServicesToggle");

        servicesTopLayout->addWidget(userServicesToggle);


        verticalLayout_services->addLayout(servicesTopLayout);

        servicesTable = new QTableWidget(servicesTab);
        servicesTable->setObjectName("servicesTable");
        servicesTable->setAlternatingRowColors(true);
        servicesTable->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
        servicesTable->setSortingEnabled(true);

        verticalLayout_services->addWidget(servicesTable);

        servicesButtonLayout = new QHBoxLayout();
        servicesButtonLayout->setObjectName("servicesButtonLayout");
        startServiceButton = new QPushButton(servicesTab);
        startServiceButton->setObjectName("startServiceButton");

        servicesButtonLayout->addWidget(startServiceButton);

        stopServiceButton = new QPushButton(servicesTab);
        stopServiceButton->setObjectName("stopServiceButton");

        servicesButtonLayout->addWidget(stopServiceButton);

        restartServiceButton = new QPushButton(servicesTab);
        restartServiceButton->setObjectName("restartServiceButton");

        servicesButtonLayout->addWidget(restartServiceButton);

        enableServiceButton = new QPushButton(servicesTab);
        enableServiceButton->setObjectName("enableServiceButton");

        servicesButtonLayout->addWidget(enableServiceButton);

        disableServiceButton = new QPushButton(servicesTab);
        disableServiceButton->setObjectName("disableServiceButton");

        servicesButtonLayout->addWidget(disableServiceButton);


        verticalLayout_services->addLayout(servicesButtonLayout);

        tabWidget->addTab(servicesTab, QString());
        tweaksTab = new QWidget();
        tweaksTab->setObjectName("tweaksTab");
        verticalLayout_tweaks = new QVBoxLayout(tweaksTab);
        verticalLayout_tweaks->setObjectName("verticalLayout_tweaks");
        tweaksWarningLabel = new QLabel(tweaksTab);
        tweaksWarningLabel->setObjectName("tweaksWarningLabel");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Minimum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(tweaksWarningLabel->sizePolicy().hasHeightForWidth());
        tweaksWarningLabel->setSizePolicy(sizePolicy);
        tweaksWarningLabel->setWordWrap(true);

        verticalLayout_tweaks->addWidget(tweaksWarningLabel);

        tweaksGridLayout = new QGridLayout();
        tweaksGridLayout->setObjectName("tweaksGridLayout");
        zramToggle = new QCheckBox(tweaksTab);
        zramToggle->setObjectName("zramToggle");

        tweaksGridLayout->addWidget(zramToggle, 0, 0, 1, 1);

        zramConfigButton = new QPushButton(tweaksTab);
        zramConfigButton->setObjectName("zramConfigButton");

        tweaksGridLayout->addWidget(zramConfigButton, 0, 1, 1, 1);

        cpuGovernorCombo = new QComboBox(tweaksTab);
        cpuGovernorCombo->addItem(QString());
        cpuGovernorCombo->addItem(QString());
        cpuGovernorCombo->setObjectName("cpuGovernorCombo");

        tweaksGridLayout->addWidget(cpuGovernorCombo, 1, 0, 1, 1);

        cpuGovernorConfigButton = new QPushButton(tweaksTab);
        cpuGovernorConfigButton->setObjectName("cpuGovernorConfigButton");

        tweaksGridLayout->addWidget(cpuGovernorConfigButton, 1, 1, 1, 1);

        ipv6Toggle = new QCheckBox(tweaksTab);
        ipv6Toggle->setObjectName("ipv6Toggle");

        tweaksGridLayout->addWidget(ipv6Toggle, 2, 0, 1, 1);

        ipv6ConfigButton = new QPushButton(tweaksTab);
        ipv6ConfigButton->setObjectName("ipv6ConfigButton");

        tweaksGridLayout->addWidget(ipv6ConfigButton, 2, 1, 1, 1);

        trimToggle = new QCheckBox(tweaksTab);
        trimToggle->setObjectName("trimToggle");

        tweaksGridLayout->addWidget(trimToggle, 3, 0, 1, 1);

        trimConfigButton = new QPushButton(tweaksTab);
        trimConfigButton->setObjectName("trimConfigButton");

        tweaksGridLayout->addWidget(trimConfigButton, 3, 1, 1, 1);

        tmpfsToggle = new QCheckBox(tweaksTab);
        tmpfsToggle->setObjectName("tmpfsToggle");

        tweaksGridLayout->addWidget(tmpfsToggle, 4, 0, 1, 1);

        tmpfsConfigButton = new QPushButton(tweaksTab);
        tmpfsConfigButton->setObjectName("tmpfsConfigButton");

        tweaksGridLayout->addWidget(tmpfsConfigButton, 4, 1, 1, 1);

        dnsConfigButton = new QPushButton(tweaksTab);
        dnsConfigButton->setObjectName("dnsConfigButton");

        tweaksGridLayout->addWidget(dnsConfigButton, 5, 0, 1, 1);

        showHiddenFilesToggle = new QCheckBox(tweaksTab);
        showHiddenFilesToggle->setObjectName("showHiddenFilesToggle");

        tweaksGridLayout->addWidget(showHiddenFilesToggle, 6, 0, 1, 1);

        showHiddenFilesConfigButton = new QPushButton(tweaksTab);
        showHiddenFilesConfigButton->setObjectName("showHiddenFilesConfigButton");

        tweaksGridLayout->addWidget(showHiddenFilesConfigButton, 6, 1, 1, 1);

        mitigationsToggle = new QCheckBox(tweaksTab);
        mitigationsToggle->setObjectName("mitigationsToggle");

        tweaksGridLayout->addWidget(mitigationsToggle, 7, 0, 1, 1);

        mitigationsConfigButton = new QPushButton(tweaksTab);
        mitigationsConfigButton->setObjectName("mitigationsConfigButton");

        tweaksGridLayout->addWidget(mitigationsConfigButton, 7, 1, 1, 1);


        verticalLayout_tweaks->addLayout(tweaksGridLayout);

        tabWidget->addTab(tweaksTab, QString());
        configTab = new QWidget();
        configTab->setObjectName("configTab");
        verticalLayout_config = new QVBoxLayout(configTab);
        verticalLayout_config->setObjectName("verticalLayout_config");
        configTopLayout = new QHBoxLayout();
        configTopLayout->setObjectName("configTopLayout");
        configLabel = new QLabel(configTab);
        configLabel->setObjectName("configLabel");

        configTopLayout->addWidget(configLabel);

        editorComboBox = new QComboBox(configTab);
        editorComboBox->addItem(QString());
        editorComboBox->addItem(QString());
        editorComboBox->addItem(QString());
        editorComboBox->addItem(QString());
        editorComboBox->setObjectName("editorComboBox");

        configTopLayout->addWidget(editorComboBox);

        configSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        configTopLayout->addItem(configSpacer);


        verticalLayout_config->addLayout(configTopLayout);

        configTable = new QTableWidget(configTab);
        configTable->setObjectName("configTable");
        configTable->setAlternatingRowColors(true);
        configTable->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
        configTable->setSortingEnabled(true);

        verticalLayout_config->addWidget(configTable);

        tabWidget->addTab(configTab, QString());
        logsTab = new QWidget();
        logsTab->setObjectName("logsTab");
        verticalLayout_logs = new QVBoxLayout(logsTab);
        verticalLayout_logs->setObjectName("verticalLayout_logs");
        logsTopLayout = new QHBoxLayout();
        logsTopLayout->setObjectName("logsTopLayout");
        logFileLabel = new QLabel(logsTab);
        logFileLabel->setObjectName("logFileLabel");

        logsTopLayout->addWidget(logFileLabel);

        logFileComboBox = new QComboBox(logsTab);
        logFileComboBox->setObjectName("logFileComboBox");

        logsTopLayout->addWidget(logFileComboBox);

        refreshLogsButton = new QPushButton(logsTab);
        refreshLogsButton->setObjectName("refreshLogsButton");

        logsTopLayout->addWidget(refreshLogsButton);

        clearLogButton = new QPushButton(logsTab);
        clearLogButton->setObjectName("clearLogButton");

        logsTopLayout->addWidget(clearLogButton);


        verticalLayout_logs->addLayout(logsTopLayout);

        logsFilterLayout = new QHBoxLayout();
        logsFilterLayout->setObjectName("logsFilterLayout");
        filterLabel = new QLabel(logsTab);
        filterLabel->setObjectName("filterLabel");

        logsFilterLayout->addWidget(filterLabel);

        logFilterEdit = new QLineEdit(logsTab);
        logFilterEdit->setObjectName("logFilterEdit");

        logsFilterLayout->addWidget(logFilterEdit);

        linesLabel = new QLabel(logsTab);
        linesLabel->setObjectName("linesLabel");

        logsFilterLayout->addWidget(linesLabel);

        logLinesSpinBox = new QSpinBox(logsTab);
        logLinesSpinBox->setObjectName("logLinesSpinBox");
        logLinesSpinBox->setMinimum(10);
        logLinesSpinBox->setMaximum(10000);
        logLinesSpinBox->setValue(100);

        logsFilterLayout->addWidget(logLinesSpinBox);

        tailLogCheckBox = new QCheckBox(logsTab);
        tailLogCheckBox->setObjectName("tailLogCheckBox");
        tailLogCheckBox->setChecked(false);

        logsFilterLayout->addWidget(tailLogCheckBox);

        applyFilterButton = new QPushButton(logsTab);
        applyFilterButton->setObjectName("applyFilterButton");

        logsFilterLayout->addWidget(applyFilterButton);


        verticalLayout_logs->addLayout(logsFilterLayout);

        logContentTextEdit = new QTextEdit(logsTab);
        logContentTextEdit->setObjectName("logContentTextEdit");
        QFont font2;
        font2.setFamilies({QString::fromUtf8("Monospace")});
        font2.setPointSize(9);
        logContentTextEdit->setFont(font2);
        logContentTextEdit->setReadOnly(true);

        verticalLayout_logs->addWidget(logContentTextEdit);

        logsBottomLayout = new QHBoxLayout();
        logsBottomLayout->setObjectName("logsBottomLayout");
        logStatusLabel = new QLabel(logsTab);
        logStatusLabel->setObjectName("logStatusLabel");

        logsBottomLayout->addWidget(logStatusLabel);

        logsBottomSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        logsBottomLayout->addItem(logsBottomSpacer);

        copyLogButton = new QPushButton(logsTab);
        copyLogButton->setObjectName("copyLogButton");

        logsBottomLayout->addWidget(copyLogButton);

        saveLogButton = new QPushButton(logsTab);
        saveLogButton->setObjectName("saveLogButton");

        logsBottomLayout->addWidget(saveLogButton);


        verticalLayout_logs->addLayout(logsBottomLayout);

        tabWidget->addTab(logsTab, QString());
        backupTab = new QWidget();
        backupTab->setObjectName("backupTab");
        verticalLayout_backup = new QVBoxLayout(backupTab);
        verticalLayout_backup->setObjectName("verticalLayout_backup");
        backupSplitter = new QSplitter(backupTab);
        backupSplitter->setObjectName("backupSplitter");
        backupSplitter->setOrientation(Qt::Orientation::Horizontal);
        backupSourceWidget = new QWidget(backupSplitter);
        backupSourceWidget->setObjectName("backupSourceWidget");
        verticalLayout_source = new QVBoxLayout(backupSourceWidget);
        verticalLayout_source->setObjectName("verticalLayout_source");
        verticalLayout_source->setContentsMargins(0, 0, 0, 0);
        sourceLabel = new QLabel(backupSourceWidget);
        sourceLabel->setObjectName("sourceLabel");

        verticalLayout_source->addWidget(sourceLabel);

        sourcePathLabel = new QLabel(backupSourceWidget);
        sourcePathLabel->setObjectName("sourcePathLabel");

        verticalLayout_source->addWidget(sourcePathLabel);

        sourcePathTree = new QTreeView(backupSourceWidget);
        sourcePathTree->setObjectName("sourcePathTree");

        verticalLayout_source->addWidget(sourcePathTree);

        backupSplitter->addWidget(backupSourceWidget);
        backupDestWidget = new QWidget(backupSplitter);
        backupDestWidget->setObjectName("backupDestWidget");
        verticalLayout_dest = new QVBoxLayout(backupDestWidget);
        verticalLayout_dest->setObjectName("verticalLayout_dest");
        verticalLayout_dest->setContentsMargins(0, 0, 0, 0);
        destLabel = new QLabel(backupDestWidget);
        destLabel->setObjectName("destLabel");

        verticalLayout_dest->addWidget(destLabel);

        destDriveCombo = new QComboBox(backupDestWidget);
        destDriveCombo->setObjectName("destDriveCombo");

        verticalLayout_dest->addWidget(destDriveCombo);

        destPathLabel = new QLabel(backupDestWidget);
        destPathLabel->setObjectName("destPathLabel");

        verticalLayout_dest->addWidget(destPathLabel);

        destPathTree = new QTreeView(backupDestWidget);
        destPathTree->setObjectName("destPathTree");

        verticalLayout_dest->addWidget(destPathTree);

        backupSplitter->addWidget(backupDestWidget);

        verticalLayout_backup->addWidget(backupSplitter);

        backupBottomLayout = new QHBoxLayout();
        backupBottomLayout->setObjectName("backupBottomLayout");
        backupSummaryLabel = new QLabel(backupTab);
        backupSummaryLabel->setObjectName("backupSummaryLabel");

        backupBottomLayout->addWidget(backupSummaryLabel);

        startBackupButton = new QPushButton(backupTab);
        startBackupButton->setObjectName("startBackupButton");

        backupBottomLayout->addWidget(startBackupButton);


        verticalLayout_backup->addLayout(backupBottomLayout);

        backupProgressBar = new QProgressBar(backupTab);
        backupProgressBar->setObjectName("backupProgressBar");
        backupProgressBar->setMinimum(0);
        backupProgressBar->setMaximum(100);
        backupProgressBar->setValue(0);
        backupProgressBar->setTextVisible(true);

        verticalLayout_backup->addWidget(backupProgressBar);

        backupPathSummaryLabel = new QLabel(backupTab);
        backupPathSummaryLabel->setObjectName("backupPathSummaryLabel");

        verticalLayout_backup->addWidget(backupPathSummaryLabel);

        tabWidget->addTab(backupTab, QString());
        restoreTab = new QWidget();
        restoreTab->setObjectName("restoreTab");
        verticalLayout_restore = new QVBoxLayout(restoreTab);
        verticalLayout_restore->setObjectName("verticalLayout_restore");
        restoreSplitter = new QSplitter(restoreTab);
        restoreSplitter->setObjectName("restoreSplitter");
        restoreSplitter->setOrientation(Qt::Orientation::Horizontal);
        restoreSourceWidget = new QWidget(restoreSplitter);
        restoreSourceWidget->setObjectName("restoreSourceWidget");
        verticalLayout_restore_source = new QVBoxLayout(restoreSourceWidget);
        verticalLayout_restore_source->setObjectName("verticalLayout_restore_source");
        verticalLayout_restore_source->setContentsMargins(0, 0, 0, 0);
        restoreSourceLabel = new QLabel(restoreSourceWidget);
        restoreSourceLabel->setObjectName("restoreSourceLabel");

        verticalLayout_restore_source->addWidget(restoreSourceLabel);

        restoreSourceDriveCombo = new QComboBox(restoreSourceWidget);
        restoreSourceDriveCombo->setObjectName("restoreSourceDriveCombo");

        verticalLayout_restore_source->addWidget(restoreSourceDriveCombo);

        restoreSourcePathLabel = new QLabel(restoreSourceWidget);
        restoreSourcePathLabel->setObjectName("restoreSourcePathLabel");

        verticalLayout_restore_source->addWidget(restoreSourcePathLabel);

        restoreSourcePathTree = new QTreeView(restoreSourceWidget);
        restoreSourcePathTree->setObjectName("restoreSourcePathTree");

        verticalLayout_restore_source->addWidget(restoreSourcePathTree);

        restoreSplitter->addWidget(restoreSourceWidget);
        restoreDestWidget = new QWidget(restoreSplitter);
        restoreDestWidget->setObjectName("restoreDestWidget");
        verticalLayout_restore_dest = new QVBoxLayout(restoreDestWidget);
        verticalLayout_restore_dest->setObjectName("verticalLayout_restore_dest");
        verticalLayout_restore_dest->setContentsMargins(0, 0, 0, 0);
        restoreDestLabel = new QLabel(restoreDestWidget);
        restoreDestLabel->setObjectName("restoreDestLabel");

        verticalLayout_restore_dest->addWidget(restoreDestLabel);

        restoreDestDriveCombo = new QComboBox(restoreDestWidget);
        restoreDestDriveCombo->setObjectName("restoreDestDriveCombo");

        verticalLayout_restore_dest->addWidget(restoreDestDriveCombo);

        restoreDestPathLabel = new QLabel(restoreDestWidget);
        restoreDestPathLabel->setObjectName("restoreDestPathLabel");

        verticalLayout_restore_dest->addWidget(restoreDestPathLabel);

        restoreDestPathTree = new QTreeView(restoreDestWidget);
        restoreDestPathTree->setObjectName("restoreDestPathTree");

        verticalLayout_restore_dest->addWidget(restoreDestPathTree);

        restoreSplitter->addWidget(restoreDestWidget);

        verticalLayout_restore->addWidget(restoreSplitter);

        restoreBottomLayout = new QHBoxLayout();
        restoreBottomLayout->setObjectName("restoreBottomLayout");
        restoreSummaryLabel = new QLabel(restoreTab);
        restoreSummaryLabel->setObjectName("restoreSummaryLabel");

        restoreBottomLayout->addWidget(restoreSummaryLabel);

        startRestoreButton = new QPushButton(restoreTab);
        startRestoreButton->setObjectName("startRestoreButton");

        restoreBottomLayout->addWidget(startRestoreButton);


        verticalLayout_restore->addLayout(restoreBottomLayout);

        restoreProgressBar = new QProgressBar(restoreTab);
        restoreProgressBar->setObjectName("restoreProgressBar");
        restoreProgressBar->setMinimum(0);
        restoreProgressBar->setMaximum(100);
        restoreProgressBar->setValue(0);
        restoreProgressBar->setTextVisible(true);

        verticalLayout_restore->addWidget(restoreProgressBar);

        restorePathSummaryLabel = new QLabel(restoreTab);
        restorePathSummaryLabel->setObjectName("restorePathSummaryLabel");

        verticalLayout_restore->addWidget(restorePathSummaryLabel);

        tabWidget->addTab(restoreTab, QString());
        isoCreatorTab = new QWidget();
        isoCreatorTab->setObjectName("isoCreatorTab");
        verticalLayout_iso = new QVBoxLayout(isoCreatorTab);
        verticalLayout_iso->setObjectName("verticalLayout_iso");
        isoScrollArea = new QScrollArea(isoCreatorTab);
        isoScrollArea->setObjectName("isoScrollArea");
        isoScrollArea->setWidgetResizable(true);
        isoScrollContent = new QWidget();
        isoScrollContent->setObjectName("isoScrollContent");
        isoScrollContent->setGeometry(QRect(0, 0, 944, 1495));
        verticalLayout_iso_content = new QVBoxLayout(isoScrollContent);
        verticalLayout_iso_content->setObjectName("verticalLayout_iso_content");
        isoBasicGroup = new QGroupBox(isoScrollContent);
        isoBasicGroup->setObjectName("isoBasicGroup");
        verticalLayout_basic = new QVBoxLayout(isoBasicGroup);
        verticalLayout_basic->setObjectName("verticalLayout_basic");
        horizontalLayout_iso_name = new QHBoxLayout();
        horizontalLayout_iso_name->setObjectName("horizontalLayout_iso_name");
        isoNameLabel = new QLabel(isoBasicGroup);
        isoNameLabel->setObjectName("isoNameLabel");

        horizontalLayout_iso_name->addWidget(isoNameLabel);

        isoNameEdit = new QLineEdit(isoBasicGroup);
        isoNameEdit->setObjectName("isoNameEdit");

        horizontalLayout_iso_name->addWidget(isoNameEdit);


        verticalLayout_basic->addLayout(horizontalLayout_iso_name);

        horizontalLayout_iso_output = new QHBoxLayout();
        horizontalLayout_iso_output->setObjectName("horizontalLayout_iso_output");
        isoOutputLabel = new QLabel(isoBasicGroup);
        isoOutputLabel->setObjectName("isoOutputLabel");

        horizontalLayout_iso_output->addWidget(isoOutputLabel);

        isoOutputEdit = new QLineEdit(isoBasicGroup);
        isoOutputEdit->setObjectName("isoOutputEdit");

        horizontalLayout_iso_output->addWidget(isoOutputEdit);

        isoOutputBrowseButton = new QPushButton(isoBasicGroup);
        isoOutputBrowseButton->setObjectName("isoOutputBrowseButton");

        horizontalLayout_iso_output->addWidget(isoOutputBrowseButton);


        verticalLayout_basic->addLayout(horizontalLayout_iso_output);


        verticalLayout_iso_content->addWidget(isoBasicGroup);

        isoKernelGroup = new QGroupBox(isoScrollContent);
        isoKernelGroup->setObjectName("isoKernelGroup");
        verticalLayout_kernel = new QVBoxLayout(isoKernelGroup);
        verticalLayout_kernel->setObjectName("verticalLayout_kernel");
        includeLinuxKernelCheck = new QCheckBox(isoKernelGroup);
        includeLinuxKernelCheck->setObjectName("includeLinuxKernelCheck");
        includeLinuxKernelCheck->setChecked(true);

        verticalLayout_kernel->addWidget(includeLinuxKernelCheck);

        includeLinuxLtsKernelCheck = new QCheckBox(isoKernelGroup);
        includeLinuxLtsKernelCheck->setObjectName("includeLinuxLtsKernelCheck");

        verticalLayout_kernel->addWidget(includeLinuxLtsKernelCheck);

        includeLinuxZenKernelCheck = new QCheckBox(isoKernelGroup);
        includeLinuxZenKernelCheck->setObjectName("includeLinuxZenKernelCheck");

        verticalLayout_kernel->addWidget(includeLinuxZenKernelCheck);

        includeLinuxHardenedKernelCheck = new QCheckBox(isoKernelGroup);
        includeLinuxHardenedKernelCheck->setObjectName("includeLinuxHardenedKernelCheck");

        verticalLayout_kernel->addWidget(includeLinuxHardenedKernelCheck);

        includeCachyosKernelCheck = new QCheckBox(isoKernelGroup);
        includeCachyosKernelCheck->setObjectName("includeCachyosKernelCheck");
        includeCachyosKernelCheck->setChecked(true);

        verticalLayout_kernel->addWidget(includeCachyosKernelCheck);


        verticalLayout_iso_content->addWidget(isoKernelGroup);

        isoDriversGroup = new QGroupBox(isoScrollContent);
        isoDriversGroup->setObjectName("isoDriversGroup");
        verticalLayout_drivers = new QVBoxLayout(isoDriversGroup);
        verticalLayout_drivers->setObjectName("verticalLayout_drivers");
        includeAllWifiDriversCheck = new QCheckBox(isoDriversGroup);
        includeAllWifiDriversCheck->setObjectName("includeAllWifiDriversCheck");
        includeAllWifiDriversCheck->setChecked(true);

        verticalLayout_drivers->addWidget(includeAllWifiDriversCheck);

        includeAllFirmwareCheck = new QCheckBox(isoDriversGroup);
        includeAllFirmwareCheck->setObjectName("includeAllFirmwareCheck");
        includeAllFirmwareCheck->setChecked(true);

        verticalLayout_drivers->addWidget(includeAllFirmwareCheck);

        includeNvidiaDriversCheck = new QCheckBox(isoDriversGroup);
        includeNvidiaDriversCheck->setObjectName("includeNvidiaDriversCheck");

        verticalLayout_drivers->addWidget(includeNvidiaDriversCheck);

        includeAmdDriversCheck = new QCheckBox(isoDriversGroup);
        includeAmdDriversCheck->setObjectName("includeAmdDriversCheck");

        verticalLayout_drivers->addWidget(includeAmdDriversCheck);

        includeIntelDriversCheck = new QCheckBox(isoDriversGroup);
        includeIntelDriversCheck->setObjectName("includeIntelDriversCheck");

        verticalLayout_drivers->addWidget(includeIntelDriversCheck);

        includeBluetoothDriversCheck = new QCheckBox(isoDriversGroup);
        includeBluetoothDriversCheck->setObjectName("includeBluetoothDriversCheck");

        verticalLayout_drivers->addWidget(includeBluetoothDriversCheck);

        includeAudioDriversCheck = new QCheckBox(isoDriversGroup);
        includeAudioDriversCheck->setObjectName("includeAudioDriversCheck");

        verticalLayout_drivers->addWidget(includeAudioDriversCheck);


        verticalLayout_iso_content->addWidget(isoDriversGroup);

        isoSoftwareGroup = new QGroupBox(isoScrollContent);
        isoSoftwareGroup->setObjectName("isoSoftwareGroup");
        verticalLayout_software = new QVBoxLayout(isoSoftwareGroup);
        verticalLayout_software->setObjectName("verticalLayout_software");
        includeCalamareInstallerCheck = new QCheckBox(isoSoftwareGroup);
        includeCalamareInstallerCheck->setObjectName("includeCalamareInstallerCheck");
        includeCalamareInstallerCheck->setChecked(true);

        verticalLayout_software->addWidget(includeCalamareInstallerCheck);

        includeSystemToolsCheck = new QCheckBox(isoSoftwareGroup);
        includeSystemToolsCheck->setObjectName("includeSystemToolsCheck");
        includeSystemToolsCheck->setChecked(true);

        verticalLayout_software->addWidget(includeSystemToolsCheck);

        includeNetworkToolsCheck = new QCheckBox(isoSoftwareGroup);
        includeNetworkToolsCheck->setObjectName("includeNetworkToolsCheck");

        verticalLayout_software->addWidget(includeNetworkToolsCheck);

        includeMediaCodecsCheck = new QCheckBox(isoSoftwareGroup);
        includeMediaCodecsCheck->setObjectName("includeMediaCodecsCheck");

        verticalLayout_software->addWidget(includeMediaCodecsCheck);

        includeOfficeSuiteCheck = new QCheckBox(isoSoftwareGroup);
        includeOfficeSuiteCheck->setObjectName("includeOfficeSuiteCheck");

        verticalLayout_software->addWidget(includeOfficeSuiteCheck);

        includeWebBrowserCheck = new QCheckBox(isoSoftwareGroup);
        includeWebBrowserCheck->setObjectName("includeWebBrowserCheck");

        verticalLayout_software->addWidget(includeWebBrowserCheck);


        verticalLayout_iso_content->addWidget(isoSoftwareGroup);

        isoDesktopGroup = new QGroupBox(isoScrollContent);
        isoDesktopGroup->setObjectName("isoDesktopGroup");
        verticalLayout_desktop = new QVBoxLayout(isoDesktopGroup);
        verticalLayout_desktop->setObjectName("verticalLayout_desktop");
        includeKdePlasmaCheck = new QCheckBox(isoDesktopGroup);
        includeKdePlasmaCheck->setObjectName("includeKdePlasmaCheck");
        includeKdePlasmaCheck->setChecked(true);

        verticalLayout_desktop->addWidget(includeKdePlasmaCheck);

        includeGnomeCheck = new QCheckBox(isoDesktopGroup);
        includeGnomeCheck->setObjectName("includeGnomeCheck");

        verticalLayout_desktop->addWidget(includeGnomeCheck);

        includeXfceCheck = new QCheckBox(isoDesktopGroup);
        includeXfceCheck->setObjectName("includeXfceCheck");

        verticalLayout_desktop->addWidget(includeXfceCheck);

        includeI3wmCheck = new QCheckBox(isoDesktopGroup);
        includeI3wmCheck->setObjectName("includeI3wmCheck");

        verticalLayout_desktop->addWidget(includeI3wmCheck);

        includeSwayCheck = new QCheckBox(isoDesktopGroup);
        includeSwayCheck->setObjectName("includeSwayCheck");

        verticalLayout_desktop->addWidget(includeSwayCheck);


        verticalLayout_iso_content->addWidget(isoDesktopGroup);

        isoAdvancedGroup = new QGroupBox(isoScrollContent);
        isoAdvancedGroup->setObjectName("isoAdvancedGroup");
        verticalLayout_advanced = new QVBoxLayout(isoAdvancedGroup);
        verticalLayout_advanced->setObjectName("verticalLayout_advanced");
        includeCustomConfigsCheck = new QCheckBox(isoAdvancedGroup);
        includeCustomConfigsCheck->setObjectName("includeCustomConfigsCheck");
        includeCustomConfigsCheck->setChecked(true);

        verticalLayout_advanced->addWidget(includeCustomConfigsCheck);

        includeUserDataCheck = new QCheckBox(isoAdvancedGroup);
        includeUserDataCheck->setObjectName("includeUserDataCheck");

        verticalLayout_advanced->addWidget(includeUserDataCheck);

        includeDevelopmentToolsCheck = new QCheckBox(isoAdvancedGroup);
        includeDevelopmentToolsCheck->setObjectName("includeDevelopmentToolsCheck");

        verticalLayout_advanced->addWidget(includeDevelopmentToolsCheck);

        includeGamingToolsCheck = new QCheckBox(isoAdvancedGroup);
        includeGamingToolsCheck->setObjectName("includeGamingToolsCheck");

        verticalLayout_advanced->addWidget(includeGamingToolsCheck);

        includeVirtualizationCheck = new QCheckBox(isoAdvancedGroup);
        includeVirtualizationCheck->setObjectName("includeVirtualizationCheck");

        verticalLayout_advanced->addWidget(includeVirtualizationCheck);

        horizontalLayout_compression = new QHBoxLayout();
        horizontalLayout_compression->setObjectName("horizontalLayout_compression");
        compressionLabel = new QLabel(isoAdvancedGroup);
        compressionLabel->setObjectName("compressionLabel");

        horizontalLayout_compression->addWidget(compressionLabel);

        compressionCombo = new QComboBox(isoAdvancedGroup);
        compressionCombo->addItem(QString());
        compressionCombo->addItem(QString());
        compressionCombo->addItem(QString());
        compressionCombo->setObjectName("compressionCombo");

        horizontalLayout_compression->addWidget(compressionCombo);


        verticalLayout_advanced->addLayout(horizontalLayout_compression);

        horizontalLayout_arch = new QHBoxLayout();
        horizontalLayout_arch->setObjectName("horizontalLayout_arch");
        archLabel = new QLabel(isoAdvancedGroup);
        archLabel->setObjectName("archLabel");

        horizontalLayout_arch->addWidget(archLabel);

        archCombo = new QComboBox(isoAdvancedGroup);
        archCombo->addItem(QString());
        archCombo->addItem(QString());
        archCombo->addItem(QString());
        archCombo->setObjectName("archCombo");

        horizontalLayout_arch->addWidget(archCombo);


        verticalLayout_advanced->addLayout(horizontalLayout_arch);


        verticalLayout_iso_content->addWidget(isoAdvancedGroup);

        isoSecurityGroup = new QGroupBox(isoScrollContent);
        isoSecurityGroup->setObjectName("isoSecurityGroup");
        verticalLayout_security = new QVBoxLayout(isoSecurityGroup);
        verticalLayout_security->setObjectName("verticalLayout_security");
        includeSecurityToolsCheck = new QCheckBox(isoSecurityGroup);
        includeSecurityToolsCheck->setObjectName("includeSecurityToolsCheck");

        verticalLayout_security->addWidget(includeSecurityToolsCheck);

        includeEncryptionToolsCheck = new QCheckBox(isoSecurityGroup);
        includeEncryptionToolsCheck->setObjectName("includeEncryptionToolsCheck");

        verticalLayout_security->addWidget(includeEncryptionToolsCheck);

        includeFirewallCheck = new QCheckBox(isoSecurityGroup);
        includeFirewallCheck->setObjectName("includeFirewallCheck");

        verticalLayout_security->addWidget(includeFirewallCheck);

        includeVpnToolsCheck = new QCheckBox(isoSecurityGroup);
        includeVpnToolsCheck->setObjectName("includeVpnToolsCheck");

        verticalLayout_security->addWidget(includeVpnToolsCheck);


        verticalLayout_iso_content->addWidget(isoSecurityGroup);

        horizontalLayout_iso_buttons = new QHBoxLayout();
        horizontalLayout_iso_buttons->setObjectName("horizontalLayout_iso_buttons");
        createIsoButton = new QPushButton(isoScrollContent);
        createIsoButton->setObjectName("createIsoButton");
        createIsoButton->setMinimumSize(QSize(150, 40));
        createIsoButton->setFont(font);

        horizontalLayout_iso_buttons->addWidget(createIsoButton);

        estimateSizeButton = new QPushButton(isoScrollContent);
        estimateSizeButton->setObjectName("estimateSizeButton");
        estimateSizeButton->setMinimumSize(QSize(120, 40));

        horizontalLayout_iso_buttons->addWidget(estimateSizeButton);

        loadPresetButton = new QPushButton(isoScrollContent);
        loadPresetButton->setObjectName("loadPresetButton");
        loadPresetButton->setMinimumSize(QSize(120, 40));

        horizontalLayout_iso_buttons->addWidget(loadPresetButton);

        savePresetButton = new QPushButton(isoScrollContent);
        savePresetButton->setObjectName("savePresetButton");
        savePresetButton->setMinimumSize(QSize(120, 40));

        horizontalLayout_iso_buttons->addWidget(savePresetButton);

        horizontalSpacer_iso = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_iso_buttons->addItem(horizontalSpacer_iso);


        verticalLayout_iso_content->addLayout(horizontalLayout_iso_buttons);

        isoProgressBar = new QProgressBar(isoScrollContent);
        isoProgressBar->setObjectName("isoProgressBar");
        isoProgressBar->setMinimum(0);
        isoProgressBar->setMaximum(100);
        isoProgressBar->setValue(0);
        isoProgressBar->setTextVisible(true);

        verticalLayout_iso_content->addWidget(isoProgressBar);

        isoStatusLabel = new QLabel(isoScrollContent);
        isoStatusLabel->setObjectName("isoStatusLabel");
        isoStatusLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);

        verticalLayout_iso_content->addWidget(isoStatusLabel);

        isoScrollArea->setWidget(isoScrollContent);

        verticalLayout_iso->addWidget(isoScrollArea);

        tabWidget->addTab(isoCreatorTab, QString());

        verticalLayout->addWidget(tabWidget);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1000, 23));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName("menuFile");
        menuHelp = new QMenu(menubar);
        menuHelp->setObjectName("menuHelp");
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menuFile->menuAction());
        menubar->addAction(menuHelp->menuAction());
        menuFile->addAction(actionExit);
        menuHelp->addAction(actionAbout);

        retranslateUi(MainWindow);

        tabWidget->setCurrentIndex(8);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "CachyOS Tools", nullptr));
        actionExit->setText(QCoreApplication::translate("MainWindow", "Exit", nullptr));
        actionAbout->setText(QCoreApplication::translate("MainWindow", "About", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "Available Drives", nullptr));
        refreshButton->setText(QCoreApplication::translate("MainWindow", "\360\237\224\204 Refresh", nullptr));
        mountButton->setText(QCoreApplication::translate("MainWindow", "\360\237\223\201 Mount", nullptr));
        forceMountButton->setText(QCoreApplication::translate("MainWindow", "\342\232\241 Force Mount", nullptr));
        unmountButton->setText(QCoreApplication::translate("MainWindow", "\360\237\223\244 Unmount", nullptr));
        mount777Button->setText(QCoreApplication::translate("MainWindow", "Mount 777", nullptr));
        takeOwnershipButton->setText(QCoreApplication::translate("MainWindow", "Take Ownership", nullptr));
        logOutput->setPlaceholderText(QCoreApplication::translate("MainWindow", "Command output will appear here...", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(driveToolsTab), QCoreApplication::translate("MainWindow", "Drive Tools", nullptr));
        shellLabel->setText(QCoreApplication::translate("MainWindow", "Detected Shell: ", nullptr));
        addAliasButton->setText(QCoreApplication::translate("MainWindow", "Add Alias", nullptr));
        editAliasButton->setText(QCoreApplication::translate("MainWindow", "Edit Alias", nullptr));
        removeAliasButton->setText(QCoreApplication::translate("MainWindow", "Remove Alias", nullptr));
        reloadAliasButton->setText(QCoreApplication::translate("MainWindow", "Reload", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(shellToolsTab), QCoreApplication::translate("MainWindow", "Shell Tools", nullptr));
        serviceSearchBar->setPlaceholderText(QCoreApplication::translate("MainWindow", "Search services...", nullptr));
        userServicesToggle->setText(QCoreApplication::translate("MainWindow", "Show User Services", nullptr));
        startServiceButton->setText(QCoreApplication::translate("MainWindow", "Start", nullptr));
        stopServiceButton->setText(QCoreApplication::translate("MainWindow", "Stop", nullptr));
        restartServiceButton->setText(QCoreApplication::translate("MainWindow", "Restart", nullptr));
        enableServiceButton->setText(QCoreApplication::translate("MainWindow", "Enable", nullptr));
        disableServiceButton->setText(QCoreApplication::translate("MainWindow", "Disable", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(servicesTab), QCoreApplication::translate("MainWindow", "Services/Startup", nullptr));
        tweaksWarningLabel->setStyleSheet(QCoreApplication::translate("MainWindow", "color: #d32f2f; padding: 4px; border: 1px solid #f44336; border-radius: 2px;", nullptr));
        tweaksWarningLabel->setText(QCoreApplication::translate("MainWindow", "The values below do not represent the current state of the option. They are not populated based on the current system settings. Please verify the current state by editing the config file.", nullptr));
        zramToggle->setText(QCoreApplication::translate("MainWindow", "Enable zram (compressed RAM swap)", nullptr));
        zramConfigButton->setText(QCoreApplication::translate("MainWindow", "Edit Config", nullptr));
        cpuGovernorCombo->setItemText(0, QCoreApplication::translate("MainWindow", "performance", nullptr));
        cpuGovernorCombo->setItemText(1, QCoreApplication::translate("MainWindow", "powersave", nullptr));

        cpuGovernorConfigButton->setText(QCoreApplication::translate("MainWindow", "Edit Config", nullptr));
        ipv6Toggle->setText(QCoreApplication::translate("MainWindow", "Enable IPv6 system-wide", nullptr));
        ipv6ConfigButton->setText(QCoreApplication::translate("MainWindow", "Edit Config", nullptr));
        trimToggle->setText(QCoreApplication::translate("MainWindow", "Enable TRIM for SSDs", nullptr));
        trimConfigButton->setText(QCoreApplication::translate("MainWindow", "Edit Config", nullptr));
        tmpfsToggle->setText(QCoreApplication::translate("MainWindow", "Mount /tmp as tmpfs (RAM disk)", nullptr));
        tmpfsConfigButton->setText(QCoreApplication::translate("MainWindow", "Edit Config", nullptr));
        dnsConfigButton->setText(QCoreApplication::translate("MainWindow", "Edit DNS Config", nullptr));
        showHiddenFilesToggle->setText(QCoreApplication::translate("MainWindow", "Show hidden files by default in file managers", nullptr));
        showHiddenFilesConfigButton->setText(QCoreApplication::translate("MainWindow", "Edit Config", nullptr));
        mitigationsToggle->setText(QCoreApplication::translate("MainWindow", "Enable Spectre/Meltdown mitigations (for security)", nullptr));
        mitigationsConfigButton->setText(QCoreApplication::translate("MainWindow", "Edit Config", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tweaksTab), QCoreApplication::translate("MainWindow", "Tweaks", nullptr));
        configLabel->setText(QCoreApplication::translate("MainWindow", "Text Editor:", nullptr));
        editorComboBox->setItemText(0, QCoreApplication::translate("MainWindow", "nano", nullptr));
        editorComboBox->setItemText(1, QCoreApplication::translate("MainWindow", "kate", nullptr));
        editorComboBox->setItemText(2, QCoreApplication::translate("MainWindow", "mousepad", nullptr));
        editorComboBox->setItemText(3, QCoreApplication::translate("MainWindow", "geany", nullptr));

        editorComboBox->setCurrentText(QCoreApplication::translate("MainWindow", "nano", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(configTab), QCoreApplication::translate("MainWindow", "System Config", nullptr));
        logFileLabel->setText(QCoreApplication::translate("MainWindow", "Log File:", nullptr));
        refreshLogsButton->setText(QCoreApplication::translate("MainWindow", "\360\237\224\204 Refresh", nullptr));
        clearLogButton->setText(QCoreApplication::translate("MainWindow", "\360\237\227\221\357\270\217 Clear Log", nullptr));
        filterLabel->setText(QCoreApplication::translate("MainWindow", "Filter:", nullptr));
        logFilterEdit->setPlaceholderText(QCoreApplication::translate("MainWindow", "Search in log content...", nullptr));
        linesLabel->setText(QCoreApplication::translate("MainWindow", "Lines:", nullptr));
        logLinesSpinBox->setSuffix(QCoreApplication::translate("MainWindow", " lines", nullptr));
        tailLogCheckBox->setText(QCoreApplication::translate("MainWindow", "Follow (tail)", nullptr));
        applyFilterButton->setText(QCoreApplication::translate("MainWindow", "Apply", nullptr));
        logContentTextEdit->setPlaceholderText(QCoreApplication::translate("MainWindow", "Log content will appear here...", nullptr));
        logStatusLabel->setText(QCoreApplication::translate("MainWindow", "Ready", nullptr));
        copyLogButton->setText(QCoreApplication::translate("MainWindow", "\360\237\223\213 Copy", nullptr));
        saveLogButton->setText(QCoreApplication::translate("MainWindow", "\360\237\222\276 Save", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(logsTab), QCoreApplication::translate("MainWindow", "System Logs", nullptr));
        sourceLabel->setText(QCoreApplication::translate("MainWindow", "System Backup Source:", nullptr));
        sourcePathLabel->setText(QCoreApplication::translate("MainWindow", "Source Path:", nullptr));
        destLabel->setText(QCoreApplication::translate("MainWindow", "Destination Drive:", nullptr));
        destPathLabel->setText(QCoreApplication::translate("MainWindow", "Destination Folder:", nullptr));
        backupSummaryLabel->setText(QCoreApplication::translate("MainWindow", "Select source and destination to begin backup.", nullptr));
        startBackupButton->setText(QCoreApplication::translate("MainWindow", "Start Backup", nullptr));
        backupPathSummaryLabel->setText(QCoreApplication::translate("MainWindow", "Source: -\\nDestination: -", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(backupTab), QCoreApplication::translate("MainWindow", "Backup", nullptr));
        restoreSourceLabel->setText(QCoreApplication::translate("MainWindow", "Backup Source:", nullptr));
        restoreSourcePathLabel->setText(QCoreApplication::translate("MainWindow", "Backup Path:", nullptr));
        restoreDestLabel->setText(QCoreApplication::translate("MainWindow", "Restore Destination:", nullptr));
        restoreDestPathLabel->setText(QCoreApplication::translate("MainWindow", "Destination Path:", nullptr));
        restoreSummaryLabel->setText(QCoreApplication::translate("MainWindow", "Select backup source and restore destination.", nullptr));
        startRestoreButton->setText(QCoreApplication::translate("MainWindow", "Start Restore", nullptr));
        restorePathSummaryLabel->setText(QCoreApplication::translate("MainWindow", "Source: -\\nDestination: -", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(restoreTab), QCoreApplication::translate("MainWindow", "Restore", nullptr));
        isoBasicGroup->setTitle(QCoreApplication::translate("MainWindow", "Basic ISO Settings", nullptr));
        isoNameLabel->setText(QCoreApplication::translate("MainWindow", "ISO Name:", nullptr));
        isoNameEdit->setPlaceholderText(QCoreApplication::translate("MainWindow", "cachyos-custom-2024-01-01", nullptr));
        isoOutputLabel->setText(QCoreApplication::translate("MainWindow", "Output Directory:", nullptr));
        isoOutputEdit->setPlaceholderText(QCoreApplication::translate("MainWindow", "/home/user/Downloads", nullptr));
        isoOutputBrowseButton->setText(QCoreApplication::translate("MainWindow", "Browse", nullptr));
        isoKernelGroup->setTitle(QCoreApplication::translate("MainWindow", "Kernel Selection", nullptr));
        includeLinuxKernelCheck->setText(QCoreApplication::translate("MainWindow", "Include Linux Kernel (default)", nullptr));
        includeLinuxLtsKernelCheck->setText(QCoreApplication::translate("MainWindow", "Include Linux LTS Kernel", nullptr));
        includeLinuxZenKernelCheck->setText(QCoreApplication::translate("MainWindow", "Include Linux Zen Kernel", nullptr));
        includeLinuxHardenedKernelCheck->setText(QCoreApplication::translate("MainWindow", "Include Linux Hardened Kernel", nullptr));
        includeCachyosKernelCheck->setText(QCoreApplication::translate("MainWindow", "Include CachyOS Kernel (optimized)", nullptr));
        isoDriversGroup->setTitle(QCoreApplication::translate("MainWindow", "Driver Inclusion", nullptr));
        includeAllWifiDriversCheck->setText(QCoreApplication::translate("MainWindow", "Include ALL WiFi Drivers", nullptr));
        includeAllFirmwareCheck->setText(QCoreApplication::translate("MainWindow", "Include ALL Firmware", nullptr));
        includeNvidiaDriversCheck->setText(QCoreApplication::translate("MainWindow", "Include NVIDIA Drivers", nullptr));
        includeAmdDriversCheck->setText(QCoreApplication::translate("MainWindow", "Include AMD Drivers", nullptr));
        includeIntelDriversCheck->setText(QCoreApplication::translate("MainWindow", "Include Intel Drivers", nullptr));
        includeBluetoothDriversCheck->setText(QCoreApplication::translate("MainWindow", "Include Bluetooth Drivers", nullptr));
        includeAudioDriversCheck->setText(QCoreApplication::translate("MainWindow", "Include Audio Drivers", nullptr));
        isoSoftwareGroup->setTitle(QCoreApplication::translate("MainWindow", "Software & Tools", nullptr));
        includeCalamareInstallerCheck->setText(QCoreApplication::translate("MainWindow", "Include Calamares Installer", nullptr));
        includeSystemToolsCheck->setText(QCoreApplication::translate("MainWindow", "Include System Tools (htop, neofetch, etc.)", nullptr));
        includeNetworkToolsCheck->setText(QCoreApplication::translate("MainWindow", "Include Network Tools", nullptr));
        includeMediaCodecsCheck->setText(QCoreApplication::translate("MainWindow", "Include Media Codecs", nullptr));
        includeOfficeSuiteCheck->setText(QCoreApplication::translate("MainWindow", "Include Office Suite (LibreOffice)", nullptr));
        includeWebBrowserCheck->setText(QCoreApplication::translate("MainWindow", "Include Web Browser (Firefox/Chromium)", nullptr));
        isoDesktopGroup->setTitle(QCoreApplication::translate("MainWindow", "Desktop Environment", nullptr));
        includeKdePlasmaCheck->setText(QCoreApplication::translate("MainWindow", "Include KDE Plasma", nullptr));
        includeGnomeCheck->setText(QCoreApplication::translate("MainWindow", "Include GNOME", nullptr));
        includeXfceCheck->setText(QCoreApplication::translate("MainWindow", "Include XFCE", nullptr));
        includeI3wmCheck->setText(QCoreApplication::translate("MainWindow", "Include i3 Window Manager", nullptr));
        includeSwayCheck->setText(QCoreApplication::translate("MainWindow", "Include Sway (Wayland)", nullptr));
        isoAdvancedGroup->setTitle(QCoreApplication::translate("MainWindow", "Advanced Options", nullptr));
        includeCustomConfigsCheck->setText(QCoreApplication::translate("MainWindow", "Include Custom System Configurations", nullptr));
        includeUserDataCheck->setText(QCoreApplication::translate("MainWindow", "Include User Data (Documents, Downloads, etc.)", nullptr));
        includeDevelopmentToolsCheck->setText(QCoreApplication::translate("MainWindow", "Include Development Tools", nullptr));
        includeGamingToolsCheck->setText(QCoreApplication::translate("MainWindow", "Include Gaming Tools (Steam, Wine, etc.)", nullptr));
        includeVirtualizationCheck->setText(QCoreApplication::translate("MainWindow", "Include Virtualization Tools (QEMU, VirtualBox)", nullptr));
        compressionLabel->setText(QCoreApplication::translate("MainWindow", "Compression Level:", nullptr));
        compressionCombo->setItemText(0, QCoreApplication::translate("MainWindow", "None", nullptr));
        compressionCombo->setItemText(1, QCoreApplication::translate("MainWindow", "Fast (gzip)", nullptr));
        compressionCombo->setItemText(2, QCoreApplication::translate("MainWindow", "Best (xz)", nullptr));

        archLabel->setText(QCoreApplication::translate("MainWindow", "Target Architecture:", nullptr));
        archCombo->setItemText(0, QCoreApplication::translate("MainWindow", "x86_64", nullptr));
        archCombo->setItemText(1, QCoreApplication::translate("MainWindow", "aarch64 (ARM64)", nullptr));
        archCombo->setItemText(2, QCoreApplication::translate("MainWindow", "Multi-arch (x86_64 + aarch64)", nullptr));

        isoSecurityGroup->setTitle(QCoreApplication::translate("MainWindow", "Security & Privacy", nullptr));
        includeSecurityToolsCheck->setText(QCoreApplication::translate("MainWindow", "Include Security Tools", nullptr));
        includeEncryptionToolsCheck->setText(QCoreApplication::translate("MainWindow", "Include Encryption Tools", nullptr));
        includeFirewallCheck->setText(QCoreApplication::translate("MainWindow", "Include Firewall Configuration", nullptr));
        includeVpnToolsCheck->setText(QCoreApplication::translate("MainWindow", "Include VPN Tools", nullptr));
        createIsoButton->setText(QCoreApplication::translate("MainWindow", "\360\237\232\200 Create ISO", nullptr));
        estimateSizeButton->setText(QCoreApplication::translate("MainWindow", "\360\237\223\212 Estimate Size", nullptr));
        loadPresetButton->setText(QCoreApplication::translate("MainWindow", "\360\237\223\202 Load Preset", nullptr));
        savePresetButton->setText(QCoreApplication::translate("MainWindow", "\360\237\222\276 Save Preset", nullptr));
        isoStatusLabel->setText(QCoreApplication::translate("MainWindow", "Ready to create custom ISO. Configure your options above.", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(isoCreatorTab), QCoreApplication::translate("MainWindow", "System ISO Creator", nullptr));
        menuFile->setTitle(QCoreApplication::translate("MainWindow", "File", nullptr));
        menuHelp->setTitle(QCoreApplication::translate("MainWindow", "Help", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
