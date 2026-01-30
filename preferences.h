
// Preferences functions

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QSizePolicy>
#include <QSettings>
#include <QTimer>
#include <QDebug>
#include <QTabWidget>
#include <QGroupBox>
#include <QSpinBox>
#include <QComboBox>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QMoveEvent>
#include <QScreen>
#include <QGuiApplication>

void MainWindow::on_actionShowHideTabs_triggered() {
    // Open the comprehensive preferences dialog
    showPreferencesDialog();
}

void MainWindow::showPreferencesDialog() {
    // Create comprehensive preferences dialog with tabs
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Preferences");
    dialog->setMinimumSize(600, 500);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);
    
    // Create tab widget for different preference categories
    QTabWidget *tabWidget = new QTabWidget(dialog);
    
    // Tab 1: General (Window size, auto-save)
    QWidget *generalTab = new QWidget();
    QVBoxLayout *generalLayout = new QVBoxLayout(generalTab);
    
    QGroupBox *windowGroup = new QGroupBox("Window Settings", generalTab);
    QVBoxLayout *windowLayout = new QVBoxLayout(windowGroup);
    
    QCheckBox *rememberSizeCheck = new QCheckBox("Remember window size and position", windowGroup);
    rememberSizeCheck->setChecked(loadWindowSizeEnabled());
    windowLayout->addWidget(rememberSizeCheck);
    
    QGroupBox *autoSaveGroup = new QGroupBox("Auto Save/Load", generalTab);
    QVBoxLayout *autoSaveLayout = new QVBoxLayout(autoSaveGroup);
    
    QCheckBox *autoSaveCheck = new QCheckBox("Auto-save preferences on change", autoSaveGroup);
    autoSaveCheck->setChecked(loadAutoSaveEnabled());
    autoSaveLayout->addWidget(autoSaveCheck);
    
    generalLayout->addWidget(windowGroup);
    generalLayout->addWidget(autoSaveGroup);
    generalLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    
    tabWidget->addTab(generalTab, "General");
    
    // Tab 2: Tab Visibility (existing functionality)
    QWidget *tabsTab = new QWidget();
    QVBoxLayout *tabsLayout = new QVBoxLayout(tabsTab);
    
    QLabel *tabsLabel = new QLabel("Select which tabs to show:", tabsTab);
    tabsLayout->addWidget(tabsLabel);
    
    QMap<QString, QCheckBox*> checkboxes;
    QMap<QString, QString> tabNames;
    tabNames["driveToolsTab"] = "Drive Tools";
    tabNames["shellToolsTab"] = "Shell Tools";
    tabNames["servicesTab"] = "Services/Startup";
    tabNames["tweaksTab"] = "Tweaks";
    tabNames["kvmTab"] = "DUAL GPU KVM";
    tabNames["configTab"] = "System Config";
    tabNames["logsTab"] = "System Logs";
    tabNames["backupTab"] = "Backup";
    tabNames["restoreTab"] = "Restore";
    tabNames["isoCreatorTab"] = "System ISO Creator";
    tabNames["packageManagerTab"] = "Package Manager";
    tabNames["networkTab"] = "Network";
    
    for (auto it = tabNames.begin(); it != tabNames.end(); ++it) {
        QCheckBox *checkbox = new QCheckBox(it.value(), tabsTab);
        checkbox->setChecked(tabVisibilityMap.value(it.key(), true));
        checkbox->setProperty("tabName", it.key());
        checkboxes[it.key()] = checkbox;
        tabsLayout->addWidget(checkbox);
    }
    
    tabsLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    tabWidget->addTab(tabsTab, "Tab Visibility");
    
    // Add more tabs for each main tab's preferences (placeholder for now)
    // Drive Tools Tab
    QWidget *driveToolsPrefTab = new QWidget();
    QVBoxLayout *driveToolsPrefLayout = new QVBoxLayout(driveToolsPrefTab);
    QLabel *driveToolsLabel = new QLabel("Drive Tools preferences will be added here", driveToolsPrefTab);
    driveToolsPrefLayout->addWidget(driveToolsLabel);
    driveToolsPrefLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    tabWidget->addTab(driveToolsPrefTab, "Drive Tools");
    
    // Shell Tools Tab
    QWidget *shellToolsPrefTab = new QWidget();
    QVBoxLayout *shellToolsPrefLayout = new QVBoxLayout(shellToolsPrefTab);
    QLabel *shellToolsLabel = new QLabel("Shell Tools preferences will be added here", shellToolsPrefTab);
    shellToolsPrefLayout->addWidget(shellToolsLabel);
    shellToolsPrefLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    tabWidget->addTab(shellToolsPrefTab, "Shell Tools");
    
    // Package Manager Tab
    QWidget *packageManagerPrefTab = new QWidget();
    QVBoxLayout *packageManagerPrefLayout = new QVBoxLayout(packageManagerPrefTab);
    QLabel *packageManagerLabel = new QLabel("Package Manager preferences will be added here", packageManagerPrefTab);
    packageManagerPrefLayout->addWidget(packageManagerLabel);
    packageManagerPrefLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    tabWidget->addTab(packageManagerPrefTab, "Package Manager");
    
    // Network Tab
    QWidget *networkPrefTab = new QWidget();
    QVBoxLayout *networkPrefLayout = new QVBoxLayout(networkPrefTab);
    QLabel *networkLabel = new QLabel("Network preferences will be added here", networkPrefTab);
    networkPrefLayout->addWidget(networkLabel);
    networkPrefLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    tabWidget->addTab(networkPrefTab, "Network");
    
    mainLayout->addWidget(tabWidget);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton("OK", dialog);
    QPushButton *cancelButton = new QPushButton("Cancel", dialog);
    QPushButton *applyButton = new QPushButton("Apply", dialog);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(applyButton);
    mainLayout->addLayout(buttonLayout);
    
    // Save function
    auto savePreferences = [this, dialog, checkboxes, rememberSizeCheck, autoSaveCheck]() {
        // Save tab visibility
        for (auto it = checkboxes.begin(); it != checkboxes.end(); ++it) {
            QString tabName = it.key();
            QCheckBox *checkbox = it.value();
            if (checkbox) {
                bool visible = checkbox->isChecked();
                tabVisibilityMap[tabName] = visible;
            }
        }
        
        // Save window size preference
        saveWindowSizeEnabled(rememberSizeCheck->isChecked());
        
        // Save auto-save preference
        saveAutoSaveEnabled(autoSaveCheck->isChecked());
        
        // Apply tab visibility
        QTimer::singleShot(0, this, [this]() {
            applyTabVisibility();
        });
        
        // Save all preferences
        saveAllPreferences();
    };
    
    connect(okButton, &QPushButton::clicked, [savePreferences, dialog]() {
        savePreferences();
        dialog->accept();
    });
    
    connect(applyButton, &QPushButton::clicked, savePreferences);
    
    connect(cancelButton, &QPushButton::clicked, dialog, &QDialog::reject);
    
    // Auto-save on change if enabled
    if (loadAutoSaveEnabled()) {
        for (auto it = checkboxes.begin(); it != checkboxes.end(); ++it) {
            connect(it.value(), &QCheckBox::toggled, savePreferences);
        }
        connect(rememberSizeCheck, &QCheckBox::toggled, savePreferences);
        connect(autoSaveCheck, &QCheckBox::toggled, savePreferences);
    }
    
    dialog->exec();
    dialog->deleteLater();
}

void MainWindow::on_actionAbout_triggered() {
    QMessageBox aboutBox(this);
    aboutBox.setWindowTitle("About Arch OS Tools");
    
    QString aboutText = QString(
        "<h2>Arch OS Tools</h2>"
        "<p><b>Created by Xetal with ❤️ for Arch Linux and CachyOS</b></p>"
        "<p>A humble collection of system management tools designed to make "
        "your Linux experience smoother and more enjoyable.</p>"
        "<p>This application was crafted with care to help you manage your "
        "system effortlessly, from drive management to package handling, "
        "system tweaks, and more.</p>"
        "<p><i>Thank you for using Arch S Tools!</i></p>"
        "<hr>"
        "<p style='color: #666; font-size: 10px;'>"
        "Built with Qt, C++ and love for the Linux community"
        "</p>"
    );
    
    aboutBox.setTextFormat(Qt::RichText);
    aboutBox.setText(aboutText);
    aboutBox.setStandardButtons(QMessageBox::Ok);
    aboutBox.exec();
}

void MainWindow::loadTabVisibilityPreferences() {
    QSettings settings("CachyOsTools", "CachyOsTools");
    
    // Load visibility for each tab (default to true/visible)
    QStringList tabNames = {
        "driveToolsTab", "shellToolsTab", "servicesTab", "tweaksTab",
        "kvmTab", "configTab", "logsTab", "backupTab", "restoreTab",
        "isoCreatorTab", "packageManagerTab", "networkTab"
    };
    
    for (const QString &tabName : tabNames) {
        bool visible = settings.value(QString("tabs/%1").arg(tabName), true).toBool();
        tabVisibilityMap[tabName] = visible;
    }
    
    // Apply visibility after a short delay to ensure all widgets are initialized
    QTimer::singleShot(100, this, [this]() {
        applyTabVisibility();
    });
}

void MainWindow::saveTabVisibilityPreferences() {
    QSettings settings("CachyOsTools", "CachyOsTools");
    
    // Save visibility for each tab
    for (auto it = tabVisibilityMap.begin(); it != tabVisibilityMap.end(); ++it) {
        settings.setValue(QString("tabs/%1").arg(it.key()), it.value());
    }
    
    settings.sync();
}

void MainWindow::applyTabVisibility() {
    // Safety check - ensure UI is initialized
    if (!ui || !ui->tabWidget) {
        return;
    }
    
    // Apply visibility to each tab
    QMap<QString, QWidget*> tabs;
    tabs["driveToolsTab"] = ui->driveToolsTab;
    tabs["shellToolsTab"] = ui->shellToolsTab;
    tabs["servicesTab"] = ui->servicesTab;
    tabs["tweaksTab"] = ui->tweaksTab;
    tabs["kvmTab"] = ui->kvmTab;
    tabs["configTab"] = ui->configTab;
    tabs["logsTab"] = ui->logsTab;
    tabs["backupTab"] = ui->backupTab;
    tabs["restoreTab"] = ui->restoreTab;
    tabs["isoCreatorTab"] = ui->isoCreatorTab;
    tabs["packageManagerTab"] = ui->packageManagerTab;
    tabs["networkTab"] = ui->networkTab;
    
    // Verify all tabs exist before proceeding
    for (auto it = tabs.begin(); it != tabs.end(); ++it) {
        if (!it.value()) {
            qWarning() << "Tab widget not found:" << it.key();
            return; // Don't proceed if any tab is missing
        }
    }
    
    // Store current tab name (not widget pointer) to restore after visibility changes
    QString currentTabName;
    if (ui->tabWidget->count() > 0) {
        QWidget *currentWidget = ui->tabWidget->currentWidget();
        if (currentWidget) {
            for (auto it = tabs.begin(); it != tabs.end(); ++it) {
                if (it.value() == currentWidget) {
                    currentTabName = it.key();
                    break;
                }
            }
        }
    }
    
    // Define tab order and titles
    QMap<QString, QString> tabTitles;
    tabTitles["driveToolsTab"] = "Drive Tools";
    tabTitles["shellToolsTab"] = "Shell Tools";
    tabTitles["servicesTab"] = "Services/Startup";
    tabTitles["tweaksTab"] = "Tweaks";
    tabTitles["kvmTab"] = "DUAL GPU KVM";
    tabTitles["configTab"] = "System Config";
    tabTitles["logsTab"] = "System Logs";
    tabTitles["backupTab"] = "Backup";
    tabTitles["restoreTab"] = "Restore";
    tabTitles["isoCreatorTab"] = "System ISO Creator";
    tabTitles["packageManagerTab"] = "Package Manager";
    tabTitles["networkTab"] = "Network";
    
    QStringList tabOrder = {
        "driveToolsTab", "shellToolsTab", "servicesTab", "tweaksTab",
        "kvmTab", "configTab", "logsTab", "backupTab", "restoreTab",
        "isoCreatorTab", "packageManagerTab", "networkTab"
    };
    
    // Store all widgets before removing tabs to prevent deletion
    // Qt will delete widgets when tabs are removed unless we keep references
    QList<QWidget*> widgetsToKeep;
    int tabCount = ui->tabWidget->count();
    for (int i = 0; i < tabCount; i++) {
        QWidget *widget = ui->tabWidget->widget(i);
        if (widget) {
            // Reparent widgets to main window temporarily to prevent deletion
            widget->setParent(this);
            widgetsToKeep.append(widget);
        }
    }
    
    // Remove all tabs (widgets are kept alive because they're reparented)
    ui->tabWidget->blockSignals(true);
    while (ui->tabWidget->count() > 0) {
        ui->tabWidget->removeTab(0);
    }
    ui->tabWidget->blockSignals(false);
    
    // Re-add tabs in order, only if visible
    for (const QString &tabName : tabOrder) {
        if (tabVisibilityMap.value(tabName, true)) {
            QWidget *tab = tabs.value(tabName);
            if (tab) {
                QString title = tabTitles.value(tabName);
                // Reparent widget back to tab widget
                tab->setParent(ui->tabWidget);
                ui->tabWidget->addTab(tab, title);
            }
        }
    }
    
    // Restore current tab if it's still visible
    if (!currentTabName.isEmpty() && tabVisibilityMap.value(currentTabName, true)) {
        QWidget *tabToSelect = tabs.value(currentTabName);
        if (tabToSelect) {
            int index = ui->tabWidget->indexOf(tabToSelect);
            if (index >= 0 && index < ui->tabWidget->count()) {
                ui->tabWidget->setCurrentIndex(index);
                return; // Successfully restored
            }
        }
    }
    
    // Current tab was hidden or not found, switch to first visible tab
    if (ui->tabWidget->count() > 0) {
        ui->tabWidget->setCurrentIndex(0);
    }
}

void MainWindow::saveAllPreferences() {
    QSettings settings("CachyOsTools", "CachyOsTools");
    
    // Save tab visibility
    saveTabVisibilityPreferences();
    
    // Save window geometry if enabled
    if (loadWindowSizeEnabled()) {
        saveWindowGeometry();
    }
    
    settings.sync();
}

void MainWindow::loadAllPreferences() {
    loadTabVisibilityPreferences();
    if (loadWindowSizeEnabled()) {
        loadWindowGeometry();
    }
}

bool MainWindow::loadWindowSizeEnabled() {
    QSettings settings("CachyOsTools", "CachyOsTools");
    return settings.value("preferences/windowSizeEnabled", true).toBool();
}

void MainWindow::saveWindowSizeEnabled(bool enabled) {
    QSettings settings("CachyOsTools", "CachyOsTools");
    settings.setValue("preferences/windowSizeEnabled", enabled);
    settings.sync();
}

bool MainWindow::loadAutoSaveEnabled() {
    QSettings settings("CachyOsTools", "CachyOsTools");
    return settings.value("preferences/autoSaveEnabled", true).toBool();
}

void MainWindow::saveAutoSaveEnabled(bool enabled) {
    QSettings settings("CachyOsTools", "CachyOsTools");
    settings.setValue("preferences/autoSaveEnabled", enabled);
    settings.sync();
}

void MainWindow::saveWindowGeometry() {
    QSettings settings("CachyOsTools", "CachyOsTools");
    settings.setValue("window/geometry", saveGeometry());
    settings.setValue("window/state", saveState());
    settings.sync();
}

void MainWindow::loadWindowGeometry() {
    QSettings settings("CachyOsTools", "CachyOsTools");
    QByteArray geometry = settings.value("window/geometry").toByteArray();
    QByteArray state = settings.value("window/state").toByteArray();
    
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    if (!state.isEmpty()) {
        restoreState(state);
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (loadWindowSizeEnabled()) {
        saveWindowGeometry();
    }
    saveAllPreferences();
    QMainWindow::closeEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    if (loadWindowSizeEnabled() && loadAutoSaveEnabled()) {
        // Debounce saves to avoid too many writes
        static QTimer *saveTimer = nullptr;
        if (!saveTimer) {
            saveTimer = new QTimer(this);
            saveTimer->setSingleShot(true);
            saveTimer->setInterval(500); // Wait 500ms after last resize
            connect(saveTimer, &QTimer::timeout, this, &MainWindow::saveWindowGeometry);
        }
        saveTimer->start();
    }
}

void MainWindow::moveEvent(QMoveEvent *event) {
    QMainWindow::moveEvent(event);
    if (loadWindowSizeEnabled() && loadAutoSaveEnabled()) {
        // Debounce saves to avoid too many writes
        static QTimer *saveTimer = nullptr;
        if (!saveTimer) {
            saveTimer = new QTimer(this);
            saveTimer->setSingleShot(true);
            saveTimer->setInterval(500); // Wait 500ms after last move
            connect(saveTimer, &QTimer::timeout, this, &MainWindow::saveWindowGeometry);
        }
        saveTimer->start();
    }
}
