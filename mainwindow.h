#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QProcess>
#include <QTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QMap>
#include <QPair>
#include <QFileSystemModel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRadioButton>
#include <QLabel>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

struct DriveInfo {
    QString device;
    QString size;
    QString type;
    QString mountPoint;
    bool isMounted;
    QString label;
    QString diskId;
    QString filesystem;
    QString uuid;
    QString model;
    QString serial;
};

struct AliasEntry {
    QString name;
    QString command;
};

struct ServiceEntry {
    QString name;
    QString description;
    QString loadState;
    QString activeState;
    QString subState;
    QString status;
    QString enabled;
};

struct ConfigFileEntry {
    QString filePath;
    QString description;
    QString category;
};

struct LogFileEntry {
    QString filePath;
    QString description;
    QString category;
    bool exists;
    qint64 size;
};

struct PackageEntry {
    QString name;
    QString version;
    QString description;
    QString repository;
    bool isInstalled;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void moveEvent(QMoveEvent *event) override;

private slots:
    void on_refreshButton_clicked();
    void on_mountButton_clicked();
    void on_forceMountButton_clicked();
    void on_unmountButton_clicked();
    void on_mount777Button_clicked();
    void on_takeOwnershipButton_clicked();
    void on_ejectButton_clicked();
    void on_formatButton_clicked();
    void on_smartInfoButton_clicked();
    void on_diskFilterCheckBox_stateChanged(int state);
    void on_partitionFilterCheckBox_stateChanged(int state);
    void on_minSizeFilterCheckBox_stateChanged(int state);
    void on_minSizeSpinBox_valueChanged(int value);
    qint64 parseSizeToMB(const QString &sizeStr);
    void on_reloadAliasButton_clicked();
    void on_addAliasButton_clicked();
    void on_editAliasButton_clicked();
    void on_removeAliasButton_clicked();
    void on_userServicesToggle_stateChanged(int state);
    void on_serviceSearchBar_textChanged(const QString &text);
    void on_startServiceButton_clicked();
    void on_stopServiceButton_clicked();
    void on_restartServiceButton_clicked();
    void on_enableServiceButton_clicked();
    void on_disableServiceButton_clicked();
    void on_configTable_cellDoubleClicked(int row, int column);
    // Log management slots
    void on_refreshLogsButton_clicked();
    void on_clearLogButton_clicked();
    void on_applyFilterButton_clicked();
    void on_copyLogButton_clicked();
    void on_saveLogButton_clicked();
    void on_logFileComboBox_currentTextChanged(const QString &text);
    void onLogProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void refreshDrives();
    void refreshServices();
    void handleServiceAction(const QString &action);
    // Tweaks tab slots
    void on_zramToggle_clicked();
    void on_zramConfigButton_clicked();
    void on_zramApplyButton_clicked();
    void on_cpuGovernorToggle_clicked();
    void on_cpuGovernorConfigButton_clicked();
    void on_cpuGovernorApplyButton_clicked();
    void on_ipv6Toggle_clicked();
    void on_ipv6ConfigButton_clicked();
    void on_ipv6ApplyButton_clicked();
    void on_trimToggle_clicked();
    void on_trimConfigButton_clicked();
    void on_trimApplyButton_clicked();
    void on_tmpfsToggle_clicked();
    void on_tmpfsConfigButton_clicked();
    void on_tmpfsApplyButton_clicked();
    void on_dnsToggle_clicked();
    void on_dnsConfigButton_clicked();
    void on_dnsApplyButton_clicked();
    void on_showHiddenFilesToggle_clicked();
    void on_showHiddenFilesConfigButton_clicked();
    void on_showHiddenFilesApplyButton_clicked();
    void on_mitigationsToggle_clicked();
    void on_mitigationsConfigButton_clicked();
    void on_mitigationsApplyButton_clicked();
    void on_performanceHacksToggle_clicked();
    void on_performanceHacksConfigButton1_clicked();
    void on_performanceHacksConfigButton2_clicked();
    void on_performanceHacksApplyButton_clicked();
    void on_ptraceToggle_clicked();
    void on_ptraceConfigButton_clicked();
    void on_ptraceApplyButton_clicked();
    void on_hidepidToggle_clicked();
    void on_hidepidConfigButton_clicked();
    void on_hidepidApplyButton_clicked();
    void on_openBackupFolderButton_clicked();
    void on_zramBackupButton_clicked();
    void on_cpuGovernorBackupButton_clicked();
    void on_ipv6BackupButton_clicked();
    void on_trimBackupButton_clicked();
    void on_tmpfsBackupButton_clicked();
    void on_dnsBackupButton_clicked();
    void on_showHiddenFilesBackupButton_clicked();
    void on_mitigationsBackupButton_clicked();
    void on_performanceHacksBackupButton1_clicked();
    void on_performanceHacksBackupButton2_clicked();
    void on_ptraceBackupButton_clicked();
    void on_hidepidBackupButton_clicked();
    void on_tweaksTabWidget_currentChanged(int index);
    // New tweaks
    void on_swappinessToggle_clicked();
    void on_swappinessApplyButton_clicked();
    void on_swappinessConfigButton_clicked();
    void on_swappinessBackupButton_clicked();
    void on_ioSchedulerToggle_clicked();
    void on_ioSchedulerApplyButton_clicked();
    void on_ioSchedulerConfigButton_clicked();
    void on_ioSchedulerBackupButton_clicked();
    void on_thpToggle_clicked();
    void on_thpApplyButton_clicked();
    void on_thpConfigButton_clicked();
    void on_thpBackupButton_clicked();
    void on_tcpOptimizationsToggle_clicked();
    void on_tcpOptimizationsApplyButton_clicked();
    void on_tcpOptimizationsConfigButton_clicked();
    void on_tcpOptimizationsBackupButton_clicked();
    void on_firewallToggle_clicked();
    void on_firewallApplyButton_clicked();
    void on_firewallConfigButton_clicked();
    void on_firewallBackupButton_clicked();
    void on_pacmanOptimizationsToggle_clicked();
    void on_pacmanOptimizationsApplyButton_clicked();
    void on_pacmanOptimizationsConfigButton_clicked();
    void on_pacmanOptimizationsBackupButton_clicked();
    void on_journaldToggle_clicked();
    void on_journaldApplyButton_clicked();
    void on_journaldConfigButton_clicked();
    void on_journaldBackupButton_clicked();
    

    // KVM VM tab slots
    void on_multiGpuXorgToggle_clicked();
    void on_multiGpuXorgConfigButton_clicked();
    void on_multiGpuXorgBackupButton_clicked();
    void on_grubConfigToggle_clicked();
    void on_grubConfigButton_clicked();
    void on_grubConfigBackupButton_clicked();
    void on_verifyIommuToggle_clicked();
    void on_verifyIommuConfigButton_clicked();
    void on_verifyIommuBackupButton_clicked();
    void on_isolateGpuToggle_clicked();
    void on_isolateGpuConfigButton_clicked();
    void on_isolateGpuBackupButton_clicked();
    void on_bindGpuToggle_clicked();
    void on_bindGpuConfigButton_clicked();
    void on_bindGpuBackupButton_clicked();
    void on_loadVfioToggle_clicked();
    void on_loadVfioConfigButton_clicked();
    void on_loadVfioBackupButton_clicked();
    void on_hugePagesToggle_clicked();
    void on_hugePagesConfigButton_clicked();
    void on_hugePagesBackupButton_clicked();
    void on_openKvmBackupFolderButton_clicked();


    // Backup helper function
    void backupConfigFile(const QString &filePath, const QString &description);
    
    // Backup tab slots
    void on_destDriveCombo_currentIndexChanged(int index);
    void on_restoreSourceDriveCombo_currentIndexChanged(int index);
    void on_restoreDestDriveCombo_currentIndexChanged(int index);
    void on_startBackupButton_clicked();
    void on_startRestoreButton_clicked();
    void on_sourcePathTree_clicked(const QModelIndex &index);
    void on_destPathTree_clicked(const QModelIndex &index);
    void on_restoreSourcePathTree_clicked(const QModelIndex &index);
    void on_restoreDestPathTree_clicked(const QModelIndex &index);
    void handleBackupProgress();
    void handleBackupFinished(int exitCode, QProcess::ExitStatus exitStatus);
    // ISO Creator tab slots
    void on_createIsoButton_clicked();
    void on_browseIsoOutputButton_clicked();
    void on_onlineModeRadio_toggled(bool checked);
    void on_offlineModeRadio_toggled(bool checked);
    void on_downloadOfflineButton_clicked();
    void checkOfflinePackageAvailability();
    // Preferences menu slots
    void on_actionShowHideTabs_triggered();
    void on_actionAbout_triggered();
    // Network tab slots
    void on_refreshIpButton_clicked();
    void on_refreshBridgeButton_clicked();
    void on_createBridgeButton_clicked();
    void on_deleteBridgeButton_clicked();
    void on_refreshLibvirtNetworkButton_clicked();
    void on_startLibvirtNetworkButton_clicked();
    void on_stopLibvirtNetworkButton_clicked();
    void on_restartLibvirtNetworkButton_clicked();
    void on_configureInterfaceButton_clicked();
    void on_upInterfaceButton_clicked();
    void on_downInterfaceButton_clicked();
    void on_refreshInterfaceConfigButton_clicked();
    void refreshNetworkInfo();
    void refreshBridges();
    void refreshLibvirtNetworks();
    void refreshInterfaceStats();
    void refreshInterfaceConfig();
    void configureInterface(const QString &interfaceName);
    QString formatBytes(qint64 bytes);
    QString convertNetmaskToCIDR(const QString &netmask);
    // Package Manager tab slots
    void on_clearCacheButton_clicked();
    void on_refreshPackagesButton_clicked();
    void on_pacmanInstalledSearch_textChanged(const QString &text);
    void on_pacmanRefreshInstalledButton_clicked();
    void on_pacmanAurSearchButton_clicked();
    void on_pacmanAurSearch_returnPressed();
    void on_pacmanUninstallButton_clicked();
    void on_pacmanReinstallButton_clicked();
    void on_pacmanAurInstallButton_clicked();
    void on_yaySearchButton_clicked();
    void on_yaySearch_returnPressed();
    void on_yayInstallPackageButton_clicked();
    void on_yayUninstallPackageButton_clicked();
    void on_paruSearchButton_clicked();
    void on_paruSearch_returnPressed();
    void on_paruInstallPackageButton_clicked();
    void on_paruUninstallPackageButton_clicked();
    void on_yayInstallButton_clicked();
    void on_yayReinstallButton_clicked();
    void on_yayUninstallButton_clicked();
    void on_paruInstallButton_clicked();
    void on_paruReinstallButton_clicked();
    void on_paruUninstallButton_clicked();

private:
    Ui::MainWindow *ui;
    QList<DriveInfo> drivesList;
    QProcess *currentProcess;
    
    void setupDriveTable();
    void populateDriveTable();
    void logMessage(const QString &message);
    void showTweakInstructions(const QString &title, const QString &instructions);
    void executeCommand(const QString &command, const QStringList &arguments = QStringList());
    QString getSelectedDrive();
    void updateButtonStates();
    QList<DriveInfo> parseDriveList(const QString &output);
    // Shell/alias management
    QString detectedShell;
    QString detectedConfigFile;
    QList<AliasEntry> aliasList;
    QMap<QString, QStringList> shellConfigFiles; // shell -> list of config files
    void detectShellAndConfig();
    void loadAliases();
    void populateAliasTable();
    void saveAliases();
    AliasEntry parseAliasLine(const QString &line, const QString &shell);
    QString aliasToLine(const AliasEntry &alias, const QString &shell);
    // Service management
    QList<ServiceEntry> serviceList;
    bool showUserServices = false;
    void populateServicesTable();
    void filterServicesTable(const QString &filter);
    void loadNumlockSetting();
    void setNumlockSetting(bool enabled);
    // Config file management
    QList<ConfigFileEntry> configFileList;
    void populateConfigTable();
    void editConfigFile(const QString &filePath);
    void setupConfigFiles();
    // Log file management
    QList<LogFileEntry> logFileList;
    QProcess *logProcess;
    void setupLogFiles();
    void populateLogComboBox();
    void loadLogContent();
    void clearLogFile(const QString &filePath);
    void searchLogFiles();
    void applyLogFilters();
    void openConfigInNano(const QString &filePath);
    void runSudoCommandInTerminal(const QString &cmd);
    
    // Tweaks tab state checking functions
    void refreshTweaksStatus();
    void checkZramState();
    void checkCpuGovernorState();
    void checkIpv6State();
    void checkTrimState();
    void checkTmpfsState();
    void checkShowHiddenFilesState();
    void checkMitigationsState();
    void checkPtraceState();
    void checkHidepidState();
    void checkDnsState();
    void checkPerformanceHacksState();
    void checkSwappinessState();
    void checkIoSchedulerState();
    void checkThpState();
    void checkTcpOptimizationsState();
    void checkFirewallState();
    void checkPacmanOptimizationsState();
    void checkJournaldState();
    void updateTweakStatusLabel(QLabel *label, const QString &status, bool enabled);
    // Backup tab helpers
    void populateBackupDrives();
    void updateSourcePathTree();
    void updateDestPathTree();
    void updateBackupSummary();
    bool validateBackupSelection();
    QString getSelectedSourcePath();
    QString getSelectedDestPath();
    QString getSelectedRestoreSourcePath();
    QString getSelectedRestoreDestPath();
    void populateRestoreDrives();
    void updateRestoreSourcePathTree();
    void updateRestoreDestPathTree();
    void updateRestoreSummary();
    bool validateRestoreSelection();
    QProcess *backupProcess;
    QFileSystemModel *sourceFsModel;
    QFileSystemModel *destFsModel;
    QString sourceMountPoint;
    QString destMountPoint;
    bool backupInProgress;
    // Restore tab variables
    QFileSystemModel *restoreSourceFsModel;
    QFileSystemModel *restoreDestFsModel;
    QString restoreSourceMountPoint;
    QString restoreDestMountPoint;
    
    // ISO Creator helper functions
    QString createIsoScript(const QString &isoName, const QString &outputDir, const QString &sudoPassword, bool offlineMode = false);
    QString formatSize(qint64 bytes);
    
    // Offline mode variables
    QString offlinePackagePath;
    const QString OFFLINE_PACKAGE_FILENAME = "offline-iso-packages-complete.tar.gz";
    
    // Package Manager helper functions
    void checkAurHelpers();
    void refreshPacmanInstalled();
    void searchPacmanAur(const QString &query, int searchMode = 0, int searchType = 0);
    void searchYay(const QString &query, int searchMode = 0, int searchType = 0);
    void searchParu(const QString &query, int searchMode = 0, int searchType = 0);
    
    // Preferences functions
    void loadTabVisibilityPreferences();
    void saveTabVisibilityPreferences();
    void applyTabVisibility();
    void showPreferencesDialog();
    void saveAllPreferences();
    void loadAllPreferences();
    bool loadWindowSizeEnabled();
    void saveWindowSizeEnabled(bool enabled);
    bool loadAutoSaveEnabled();
    void saveAutoSaveEnabled(bool enabled);
    void saveWindowGeometry();
    void loadWindowGeometry();
    QMap<QString, bool> tabVisibilityMap; // tab name -> visible
};

#endif // MAINWINDOW_H
