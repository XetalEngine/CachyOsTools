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

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_refreshButton_clicked();
    void on_mountButton_clicked();
    void on_forceMountButton_clicked();
    void on_unmountButton_clicked();
    void on_mount777Button_clicked();
    void on_takeOwnershipButton_clicked();
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
    void on_cpuGovernorToggle_clicked();
    void on_cpuGovernorConfigButton_clicked();
    void on_ipv6Toggle_clicked();
    void on_ipv6ConfigButton_clicked();
    void on_trimToggle_clicked();
    void on_trimConfigButton_clicked();
    void on_tmpfsToggle_clicked();
    void on_tmpfsConfigButton_clicked();
    void on_dnsToggle_clicked();
    void on_dnsConfigButton_clicked();
    void on_showHiddenFilesToggle_clicked();
    void on_showHiddenFilesConfigButton_clicked();
    void on_mitigationsToggle_clicked();
    void on_mitigationsConfigButton_clicked();
    void on_performanceHacksToggle_clicked();
    void on_performanceHacksConfigButton1_clicked();
    void on_performanceHacksConfigButton2_clicked();
    void on_ptraceToggle_clicked();
    void on_ptraceConfigButton_clicked();
    void on_hidepidToggle_clicked();
    void on_hidepidConfigButton_clicked();
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
    void on_estimateSizeButton_clicked();
    void on_loadPresetButton_clicked();
    void on_savePresetButton_clicked();
    void on_isoOutputBrowseButton_clicked();

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
    void populateTweaksState();
    void checkZramState();
    void checkCpuGovernorState();
    void checkIpv6State();
    void checkTrimState();
    void checkTmpfsState();
    void checkShowHiddenFilesState();
    void checkMitigationsState();
    void onTweakStateCheckFinished(int exitCode, QProcess::ExitStatus exitStatus);
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
};

#endif // MAINWINDOW_H
