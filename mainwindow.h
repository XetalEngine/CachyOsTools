#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QTreeWidget>
#include <QHash>
#include <QSet>
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
#include <QComboBox>
#include <QListWidget>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QFileSystemWatcher>
#include <functional>

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

// One row of the Cleaner catalogue. paths are bash patterns (globs allowed,
// $H = home); sizeCmd/cleanCmd override the default du/rm handling when the
// right tool is something else (paccache, journalctl, balooctl...).
struct CleanTarget {
    QString id;
    QString label;
    QString detail;          // what deleting it actually costs you
    int tier = 0;            // 0 = regenerates for free, 1 = costs something
    bool system = false;     // lives outside home, needs root
    QStringList paths;
    QString sizeCmd;         // echoes a byte count instead of du-ing paths
    QString cleanCmd;        // runs instead of rm -rf on paths
    QString needsBinary;     // category is hidden when this tool is missing
    qint64 bytes = 0;
    bool available = true;
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

// One thing that starts itself when you log in: an XDG .desktop entry (yours or
// a package's) or an enabled systemd user unit.
struct AutostartEntry {
    QString name;
    QString exec;
    QString comment;
    QString path;          // file on disk; empty for units with no fragment
    QString type;          // "xdg" | "systemd"
    QString onlyShowIn;    // OnlyShowIn= — the entry is desktop-specific
    bool enabled = true;
    bool system = false;   // shipped by a package, not written by the user
    bool isOverride = false;
    int delay = 0;         // X-GNOME-Autostart-Delay, seconds
};

// First-boot hardware adaptation options for the System ISO (each independent;
// all off = exact clone, the untouched default behavior)
struct IsoFirstBootOptions {
    bool fixNetwork = false;
    bool fixGpu = false;
    bool changeUser = false;
    bool regenSsh = false;
    bool regenMachineId = false;
    bool any() const { return fixNetwork || fixGpu || changeUser || regenSsh || regenMachineId; }
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
    // ── Tweaks tab: KDE Plasma section ──────────────────────────────────────
    void on_kdeGlMaxFramesToggle_clicked();
    void on_kdeGlMaxFramesApplyButton_clicked();
    void on_kdeGlMaxFramesConfigButton_clicked();
    void on_kdeGlMaxFramesBackupButton_clicked();
    void on_kdeTripleBufferToggle_clicked();
    void on_kdeTripleBufferApplyButton_clicked();
    void on_kdeTripleBufferConfigButton_clicked();
    void on_kdeTripleBufferBackupButton_clicked();
    void on_kdeGlYieldToggle_clicked();
    void on_kdeGlYieldApplyButton_clicked();
    void on_kdeGlYieldConfigButton_clicked();
    void on_kdeGlYieldBackupButton_clicked();
    void on_kdeShaderCacheRamToggle_clicked();
    void on_kdeShaderCacheRamApplyButton_clicked();
    void on_kdeShaderCacheRamConfigButton_clicked();
    void on_kdeShaderCacheRamBackupButton_clicked();
    void on_kdeQmlCacheRamToggle_clicked();
    void on_kdeQmlCacheRamApplyButton_clicked();
    void on_kdeQmlCacheRamConfigButton_clicked();
    void on_kdeQmlCacheRamBackupButton_clicked();
    void on_kdePipelineCacheRamToggle_clicked();
    void on_kdePipelineCacheRamApplyButton_clicked();
    void on_kdePipelineCacheRamConfigButton_clicked();
    void on_kdePipelineCacheRamBackupButton_clicked();
    void on_kdeQtCacheRamToggle_clicked();
    void on_kdeQtCacheRamApplyButton_clicked();
    void on_kdeQtCacheRamConfigButton_clicked();
    void on_kdeQtCacheRamBackupButton_clicked();
    void on_kdeDisableQmlCacheToggle_clicked();
    void on_kdeDisableQmlCacheApplyButton_clicked();
    void on_kdeDisableQmlCacheConfigButton_clicked();
    void on_kdeDisableQmlCacheBackupButton_clicked();
    void on_kdePanelFloatingToggle_clicked();
    void on_kdePanelFloatingApplyButton_clicked();
    void on_kdePanelFloatingConfigButton_clicked();
    void on_kdePanelFloatingBackupButton_clicked();
    void on_kdeOpenEnvFolderButton_clicked();
    void on_kdeDisableAllButton_clicked();
    // KDE known-issue fixes
    void on_kdeFixFolderDragToggle_clicked();
    void on_kdeFixFolderDragApplyButton_clicked();
    void on_kdeFixFolderDragBackupButton_clicked();
    void on_kdeFixFolderDragConfigButton_clicked();
    void on_kdeFixWidgetsLockedToggle_clicked();
    void on_kdeFixWidgetsLockedApplyButton_clicked();
    void on_kdeFixConfigCorruptToggle_clicked();
    void on_kdeFixConfigCorruptApplyButton_clicked();
    void on_kdeFixConfigCorruptBackupButton_clicked();
    void on_kdeFixConfigCorruptConfigButton_clicked();
    void on_kdeFixPlasmaCacheToggle_clicked();
    void on_kdeFixPlasmaCacheApplyButton_clicked();
    void on_kdeRescanIssuesButton_clicked();
    void on_kdeRestartShellButton_clicked();
    void on_kdeRestartKwinButton_clicked();
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
    // Buffed tweaks
    void on_gamingToggle_clicked();
    void on_gamingApplyButton_clicked();
    void on_gamingConfigButton_clicked();
    void on_gamingBackupButton_clicked();
    void on_cpuBoostToggle_clicked();
    void on_cpuBoostApplyButton_clicked();
    void on_cpuBoostConfigButton_clicked();
    void on_cpuBoostBackupButton_clicked();
    void on_nmiWatchdogToggle_clicked();
    void on_nmiWatchdogApplyButton_clicked();
    void on_nmiWatchdogConfigButton_clicked();
    void on_nmiWatchdogBackupButton_clicked();
    void on_coreDumpToggle_clicked();
    void on_coreDumpApplyButton_clicked();
    void on_coreDumpConfigButton_clicked();
    void on_coreDumpBackupButton_clicked();
    void on_memoryTuningToggle_clicked();
    void on_memoryTuningApplyButton_clicked();
    void on_memoryTuningConfigButton_clicked();
    void on_memoryTuningBackupButton_clicked();
    void on_inotifyToggle_clicked();
    void on_inotifyApplyButton_clicked();
    void on_inotifyConfigButton_clicked();
    void on_inotifyBackupButton_clicked();
    

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
    void on_fixBridgeButton_clicked();
    void on_wifiScanButton_clicked();
    void on_wifiConnectButton_clicked();
    void on_wifiDisconnectButton_clicked();
    void on_wifiRadioToggleButton_clicked();
    void on_connRefreshButton_clicked();
    void on_connUpButton_clicked();
    void on_connDownButton_clicked();
    void on_connDeleteButton_clicked();
    void on_connAutoconnectButton_clicked();
    void on_restartNetworkManagerButton_clicked();
    void refreshNetworkInfo();
    void refreshBridges();
    void refreshLibvirtNetworks();
    void refreshInterfaceStats();
    void refreshInterfaceConfig();
    void refreshWifiNetworks(bool rescan = false);
    void refreshConnectionsList();
    void configureInterface(const QString &interfaceName);
    QString getWifiDevice();
    QStringList splitNmcliLine(const QString &line);
    void runScriptInTerminal(const QString &scriptContent, const QString &namePrefix);
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
    // Uninstall tab slots
    void on_uninstallRefreshButton_clicked();
    void on_uninstallButton_clicked();
    void on_uninstallSearch_textChanged(const QString &text);
    void on_uninstallSourceFilter_currentIndexChanged(int index);
    // Tab upgrade batch slots (drives / services / config / logs / updates / uninstall / ports)
    void on_driveHealthButton_clicked();
    void on_burnIsoButton_clicked();
    void on_loadLabelsButton_clicked();
    void on_bootAnalysisButton_clicked();
    void on_builtinEditButton_clicked();
    void on_followLogButton_clicked();
    void on_bootSelectCombo_activated(int index);
    void on_checkUpdatesButton_clicked();
    void on_upgradeSystemButton_clicked();
    void on_aurUpgradeButton_clicked();
    void on_orphanCleanButton_clicked();
    void on_cacheCleanButton_clicked();
    void on_portsRefreshButton_clicked();
    void on_dashRefreshButton_clicked();
    void on_installDebButton_clicked();
    void on_themePresetCombo_activated(int index);
    void on_themeAccentButton_clicked();
    void on_themeBgButton_clicked();
    void on_themeTextButton_clicked();
    void on_themeResetButton_clicked();

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
    // Alias -> .desktop shortcut (optionally pinned to the KDE start menu favorites)
    void showAliasToDesktopDialog(const AliasEntry &entry);
    bool writeAliasDesktopEntry(const AliasEntry &entry, const QString &appName, const QString &command,
                                const QString &icon, const QString &category, bool runInTerminal,
                                bool keepTerminalOpen, QString &desktopFileOut, QString &errorOut);
    bool addDesktopFileToKdeFavorites(const QString &desktopFilePath, QString &errorOut);
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
    // ── KDE Plasma tweaks ───────────────────────────────────────────────────
    void refreshKdeTweaksStatus();
    QString kdeEnvDir() const;
    QString kdeEnvFile(const QString &basename) const;
    bool    kdeEnvEnabled(const QString &basename) const;
    bool    kdeEnvWrite(const QString &basename, const QString &body);
    bool    kdeEnvRemove(const QString &basename);
    void    kdeEnvToggleApply(const QString &basename, const QString &body, const QString &title);
    void    kdeEnvEditConfig(const QString &basename, const QString &body);
    void    kdeEnvBackup(const QString &basename, const QString &description);
    void checkkdeGlMaxFramesState();
    void checkkdeTripleBufferState();
    void checkkdeGlYieldState();
    void checkkdeShaderCacheRamState();
    void checkkdeQmlCacheRamState();
    void checkkdePipelineCacheRamState();
    void checkkdeQtCacheRamState();
    void checkkdeDisableQmlCacheState();
    void checkkdePanelFloatingState();
    // ── KDE known-issue fixes ───────────────────────────────────────────────
    QString     kdeAppletsRcPath() const;
    bool        kdeStopPlasmaShell();
    void        kdeStartPlasmaShell();
    QStringList kdeFolderContainmentIds() const;
    QStringList kdeScanFolderDragIssues() const;
    QStringList kdeScanConfigCorruption() const;
    int         kdeWidgetsLockedState() const;   // -1 unknown, 0 unlocked, 1 locked
    qint64      kdePlasmaCacheBytes() const;
    void checkkdeFixFolderDragState();
    void checkkdeFixWidgetsLockedState();
    void checkkdeFixConfigCorruptState();
    void checkkdeFixPlasmaCacheState();
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
    void checkGamingState();
    void checkCpuBoostState();
    void checkNmiWatchdogState();
    void checkCoreDumpState();
    void checkMemoryTuningState();
    void checkInotifyState();
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
    QString createIsoScript(const QString &isoName, const QString &outputDir, const QString &sudoPassword, bool offlineMode = false,
                            const QStringList &excludePaths = QStringList(),
                            const IsoFirstBootOptions &firstBoot = IsoFirstBootOptions());
    QString formatSize(qint64 bytes);

    // ISO exclusion panels (big folders / ~/.config)
    void setupSingleGpuGuide();
    void setupDualGpuGuide();

    // Self-updater (Dashboard -> Welcome): git-based check + pull/rebuild/restart
    void checkForAppUpdates();
    void checkForAppUpdatesViaApi();
    void runAppUpdate();
    void runAppUpdateFromClone();
    QPushButton *dashUpdateBtn = nullptr;

    // System ISO tab: missing-dependency bar (one install button per tool)
    void setupIsoDepsCheck();
    void refreshIsoDeps();
    QWidget *isoDepsBar = nullptr;
    QHBoxLayout *isoDepsBarLayout = nullptr;

    // Control panel (Dashboard sub-tabs)
    void setupControlPanelTabs();
    void setupDeviceManagerTab();
    void refreshDeviceManager();
    void showDeviceProperties(QTreeWidgetItem *item);
    void showModuleInfo(const QString &mod);
    void dumpDeviceVbios(const QVariantMap &p);
    void patchVbiosIfNeeded(const QString &romPath);
    void dumpFirmware(const QString &kind);
    QLineEdit *devMgrFilter = nullptr;
    QLabel *devMgrCountLabel = nullptr;
    void setupUsersGroupsTab();
    void refreshUsersGroups();
    void setupRestorePointsTab();
    void refreshRestorePoints();
    void setupFirewallTab();
    void refreshFirewall();
    void setupKernelManagerTab();
    void refreshKernelManager();
    void setupEnvVarsTab();
    void refreshEnvVars();
    void setupSystemPropsTab();
    void refreshSystemProps();
    QTreeWidget *devMgrTree = nullptr;
    QTableWidget *ugUsersTable = nullptr;
    QTableWidget *rpTable = nullptr;
    QLabel *rpStatusLabel = nullptr;
    QTableWidget *fwTable = nullptr;
    QLabel *fwStatusLabel = nullptr;
    QString fwBackend;
    QTableWidget *kmTable = nullptr;
    QListWidget *kmGrubList = nullptr;
    QLabel *kmGrubDefaultLabel = nullptr;
    QTableWidget *envSysTable = nullptr;
    QTableWidget *envUserTable = nullptr;
    QLabel *spInfoLabel = nullptr;
    void startIsoHomeScan();
    void startIsoConfigScan();
    void rebuildIsoBigTree();
    void onIsoExcludeItemChanged(QTreeWidgetItem *item, int column);
    void updateIsoExcludeSummary();
    void refreshIsoExcludedSizes();
    QStringList collectIsoExcludePaths() const;
    void saveIsoExcludeProfile();
    void loadIsoExcludeProfile();
    void clearIsoExclusions();
    void syncIsoConfigTreeChecks();
    QHash<QString, qint64> isoHomeDirSizes;      // dir path -> bytes (full du cache)
    QList<QPair<QString, qint64>> isoHomeBigFiles; // files >= 10MB -> bytes
    QSet<QString> isoExcludedPaths;              // checked (excluded) absolute paths
    QHash<QString, qint64> isoExcludedSizes;     // checked path -> bytes (for the summary)
    bool isoHomeScanDone = false;
    bool isoConfigScanDone = false;
    
    // Offline mode variables
    QString offlinePackagePath;
    const QString OFFLINE_PACKAGE_FILENAME = "offline-iso-packages-complete.tar.gz";
    
    // Tab upgrade batch helpers
    bool authenticateSudo();
    void updateFailedServicesBanner();
    void showServiceJournal();
    void openBuiltinEditor(const QString &filePath);
    QString colorizeLogLine(const QString &line);
    void fetchArchNews();
    void refreshOpenPorts();
    void refreshDashboard();
    void applyAppTheme();
    void loadThemeSettings();
    void saveThemeSettings();
    void updateThemeSwatches();
    QString themeAccent = "#27ae60";
    QString themeBg;
    QString themeText;
    QProcess *journalFollowProcess = nullptr;

    // Cleaner tab: curated cache categories + a du size tree for manual hunting
    void setupCleanerTab();
    QList<CleanTarget> buildCleanTargets() const;
    bool isCleanablePath(const QString &path) const;
    void startCleanerScan();
    void rebuildCleanerCategoryTree();
    void startCleanerTreeScan(const QString &rootPath);
    void rebuildCleanerSizeTree();
    void updateCleanerSummary();
    void runCleanerClean();
    QWidget *cleanerTab = nullptr;
    QTreeWidget *cleanCatTree = nullptr;
    QTreeWidget *cleanSizeTree = nullptr;
    QLabel *cleanStatusLabel = nullptr;
    QLabel *cleanSummaryLabel = nullptr;
    QCheckBox *cleanSystemCheck = nullptr;
    QLineEdit *cleanFolderEdit = nullptr;
    QPushButton *cleanRunButton = nullptr;
    QList<CleanTarget> cleanTargets;
    QSet<QString> cleanSelectedIds;              // checked categories
    QSet<QString> cleanSelectedPaths;            // checked size-tree paths
    QHash<QString, qint64> cleanTreeDirSizes;    // dir path -> bytes (du cache)
    QList<QPair<QString, qint64>> cleanTreeBigFiles;
    QString cleanTreeRoot;
    bool cleanScanDone = false;

    // Pacman Doctor (PKG Install -> Pacman -> Repair): diagnose and fix the
    // five classic pacman failures — mirrors, keyring, db.lck, a bad update,
    // an untuned pacman.conf.
    void setupPacmanDoctorTab();
    void refreshPacmanDoctor();
    void pacmanRankMirrorsDialog();
    void pacmanFixKeyringDialog();
    void pacmanDowngradeDialog();
    void pacmanApplyConfTuning();
    QWidget *pacDocTab = nullptr;
    QLabel *pacDocDiagLabel = nullptr;
    QLabel *pacDocStatusLabel = nullptr;
    QListWidget *pacDocPacnewList = nullptr;
    QSpinBox *pacDocParallelSpin = nullptr;
    QCheckBox *pacDocColorCheck = nullptr;
    QCheckBox *pacDocCandyCheck = nullptr;
    QCheckBox *pacDocVerboseCheck = nullptr;
    QCheckBox *pacDocCheckSpaceCheck = nullptr;
    QCheckBox *pacDocTimeoutCheck = nullptr;
    QStringList pacDocRankers;                     // mirror tools found on this box
    QFileSystemWatcher *pacDocWatcher = nullptr;   // re-diagnose the moment a fix lands
    QTimer *pacDocDebounce = nullptr;
    bool pacDocBusy = false;
    bool pacmanDoctorVisible() const;

    // A repair runs detached in a terminal, so there is no finished() to wait on.
    // Re-run the given check on a decaying schedule until the dust settles.
    void scheduleStagedRecheck(const std::function<void()> &recheck);

    // Sensors & Power (Dashboard sub-tab, next to Devices)
    void setupSensorsPowerTab();
    void refreshSensorsPower();
    void refreshPowerPanel();
    QWidget *sensSubTab = nullptr;
    QTreeWidget *sensTree = nullptr;
    QLabel *sensSummaryLabel = nullptr;
    QLabel *sensMissingLabel = nullptr;
    QCheckBox *sensLiveCheck = nullptr;
    QTimer *sensTimer = nullptr;
    QHash<QString, QTreeWidgetItem *> sensChipItems;  // chip -> node, so live
    QHash<QString, QTreeWidgetItem *> sensRowItems;   // "chip|label" -> row, updates
    bool sensBusy = false;                            // don't stack reader processes
    bool sensPowerBusy = false;
    bool sensPopulating = false;                      // refilling combos, ignore activations
    QComboBox *powProfileCombo = nullptr;
    QComboBox *powGovCombo = nullptr;
    QComboBox *powEppCombo = nullptr;
    QLabel *powProfileLabel = nullptr;
    QGroupBox *powBatteryGroup = nullptr;
    QLabel *powBatteryLabel = nullptr;
    QSpinBox *powThresholdSpin = nullptr;
    QLabel *fanStatusLabel = nullptr;
    QString sensBatteryPath;

    // Network Shares (Network sub-tab): mount SMB/NFS, and export folders via Samba
    void setupNetworkSharesTab();
    void refreshNetworkShares();
    void refreshMountedShares();
    void refreshSambaShares();
    void discoverNetworkShares();
    void listHostShares(const QString &host);
    void showMountShareDialog(const QString &type, const QString &server, const QString &share);
    void showAddSambaShareDialog();
    QWidget *sharesSubTab = nullptr;
    QTableWidget *shrMountedTable = nullptr;
    QTreeWidget *shrDiscoverTree = nullptr;
    QTableWidget *shrSambaTable = nullptr;
    QLabel *shrStatusLabel = nullptr;
    QLabel *shrSambaStatusLabel = nullptr;
    QLineEdit *shrHostEdit = nullptr;
    QFileSystemWatcher *shrWatcher = nullptr;      // fstab / smb.conf changed under us
    QTimer *shrDebounce = nullptr;
    bool networkSharesVisible() const;

    // Autostart (Services sub-tab): XDG entries + systemd user units in one list
    void setupAutostartTab();
    void refreshAutostart();
    void populateAutostartTable();
    void addAutostartEntryDialog();
    void setAutostartEnabled(bool enabled);
    int currentAutostartIndex();
    QWidget *autostartTab = nullptr;
    QTableWidget *autoTable = nullptr;
    QLabel *autoStatusLabel = nullptr;
    QCheckBox *autoShowSystemCheck = nullptr;
    QList<AutostartEntry> autostartList;

    // Uninstall tab helper functions
    void refreshUninstallList();
    void filterUninstallTable();
    void updateUninstallSelectionInfo();
    qint64 parsePacmanSizeToBytes(const QString &sizeStr);

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
