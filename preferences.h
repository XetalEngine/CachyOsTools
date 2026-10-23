
// Application preferences and runtime application of saved values.

#include "preferencesdialog.h"
#include <QSignalBlocker>
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
    QSettings settings("CachyOsTools", "CachyOsTools");
    QVariantMap saved = AppPreferences::read(settings);
    PreferencesDialog dialog(saved, [this, &saved](const QVariantMap &values) {
        QSettings destination("CachyOsTools", "CachyOsTools");
        if (!AppPreferences::write(destination, values)) return false;
        applyPreferenceValues(values, saved);
        saved = values;
        if (loadWindowSizeEnabled()) saveWindowGeometry();
        return true;
    }, this);
    dialog.exec();
}

void MainWindow::applyPreferenceValues(const QVariantMap &values, const QVariantMap &previous) {
    auto changed = [&](const QString &key) { return !previous.contains(key) || values.value(key) != previous.value(key); };
    bool tabsChanged = false;
    for (const auto &tab : AppPreferences::tabs()) {
        QString key = "tabs/" + tab.id;
        if (changed(key)) { tabVisibilityMap[tab.id] = values.value(key).toBool(); tabsChanged = true; }
    }
    if (tabsChanged) applyTabVisibility();
    if (changed("preferences/debugIsoCreatorEnabled"))
        ui->createDummyIsoButton->setVisible(values.value("preferences/debugIsoCreatorEnabled").toBool());

    if (changed("language/code")) {
        QSignalBlocker blocker(ui->dashLangCombo);
        int index = ui->dashLangCombo->findData(values.value("language/code"));
        if (index >= 0) ui->dashLangCombo->setCurrentIndex(index);
    }
    const QString accent = values.value("theme/accent").toString();
    const QString background = values.value("theme/background").toString();
    const QString text = values.value("theme/text").toString();
    const bool colorsChanged = themeAccent != accent || themeBg != background || themeText != text;
    themeAccent = accent; themeBg = background; themeText = text;
    {
        QSignalBlocker blocker(ui->themePresetCombo);
        ui->themePresetCombo->setCurrentIndex(values.value("theme/preset").toInt());
    }
    updateThemeSwatches();
    if (colorsChanged) { applyAppTheme(); refreshDashboard(); }

    // Only changed defaults replace a tool's current choices. Saving an
    // unrelated preference must not reset work in progress in another tab.
    bool drivesChanged = false;
    for (auto pair : {qMakePair(QString("defaults/drives/showDisks"), ui->diskFilterCheckBox),
                      qMakePair(QString("defaults/drives/showPartitions"), ui->partitionFilterCheckBox),
                      qMakePair(QString("defaults/drives/minimumEnabled"), ui->minSizeFilterCheckBox)}) {
        if (!changed(pair.first)) continue;
        QSignalBlocker blocker(pair.second);
        pair.second->setChecked(values.value(pair.first).toBool());
        drivesChanged = true;
    }
    if (changed("defaults/drives/minimumMB")) {
        QSignalBlocker blocker(ui->minSizeSpinBox);
        ui->minSizeSpinBox->setValue(values.value("defaults/drives/minimumMB").toInt());
        drivesChanged = true;
    }
    if (drivesChanged) populateDriveTable();
    if (changed("defaults/config/editor")) ui->editorComboBox->setCurrentText(values.value("defaults/config/editor").toString());
    if (changed("defaults/packages/searchMode")) {
        for (QComboBox *combo : {ui->pacmanAurSearchMode, ui->yaySearchMode, ui->paruSearchMode}) {
            QSignalBlocker blocker(combo);
            combo->setCurrentIndex(values.value("defaults/packages/searchMode").toInt());
        }
    }
    if (changed("defaults/packages/searchType")) {
        for (QComboBox *combo : {ui->pacmanAurSearchType, ui->yaySearchType, ui->paruSearchType}) {
            QSignalBlocker blocker(combo);
            combo->setCurrentIndex(values.value("defaults/packages/searchType").toInt());
        }
    }
    if (isoExclusionsUi) {
        if (changed("defaults/iso/folderMinimumMB")) isoExclusionsUi->isoFolderSizeSpin->setValue(values.value("defaults/iso/folderMinimumMB").toInt());
        if (changed("defaults/iso/fileMinimumMB")) isoExclusionsUi->isoFileSizeSpin->setValue(values.value("defaults/iso/fileMinimumMB").toInt());
    }
}

void MainWindow::on_actionAbout_triggered() {
    QMessageBox aboutBox(this);
    aboutBox.setWindowTitle(tr("About Arch OS Tools"));
    
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


void MainWindow::saveTabVisibilityPreferences() {
    QSettings settings("CachyOsTools", "CachyOsTools");
    
    // Save visibility for each tab
    for (auto it = tabVisibilityMap.begin(); it != tabVisibilityMap.end(); ++it) {
        settings.setValue(QString("tabs/%1").arg(it.key()), it.value());
    }
    
    settings.sync();
}

QMap<QString, QWidget *> MainWindow::preferenceTabWidgets() const {
    return {{"dashboardTab", ui->dashboardTab}, {"driveToolsTab", ui->driveToolsTab},
            {"shellToolsTab", ui->shellToolsTab}, {"servicesTab", ui->servicesTab},
            {"tweaksTab", ui->tweaksTab}, {"kvmTab", ui->kvmTab},
            {"configTab", ui->configTab}, {"logsTab", ui->logsTab},
            {"backupTab", ui->backupTab}, {"restoreTab", ui->restoreTab},
            {"isoCreatorTab", ui->isoCreatorTab}, {"packageManagerTab", ui->packageManagerTab},
            {"uninstallTab", ui->uninstallTab}, {"cleanerTab", cleanerTab},
            {"networkTab", ui->networkTab}, {"themeTab", ui->themeTab}};
}

void MainWindow::applyTabVisibility() {
    if (!ui || !ui->tabWidget) return;
    const auto widgets = preferenceTabWidgets();
    QWidget *current = ui->tabWidget->currentWidget();
    {
        // removeTab keeps each page owned by the stack. Block signals throughout
        // the rebuild so tab handlers never see an intermediate empty selection.
        QSignalBlocker blocker(ui->tabWidget);
        while (ui->tabWidget->count()) ui->tabWidget->removeTab(0);
        for (const auto &tab : AppPreferences::tabs()) {
            if (widgets.value(tab.id) && tabVisibilityMap.value(tab.id, true))
                ui->tabWidget->addTab(widgets.value(tab.id), tab.title);
        }
        if (!ui->tabWidget->count()) {
            tabVisibilityMap["dashboardTab"] = true;
            ui->tabWidget->addTab(ui->dashboardTab, tr("Dashboard"));
        }
        if (current && ui->tabWidget->indexOf(current) >= 0) ui->tabWidget->setCurrentWidget(current);
    }
    if (current != ui->tabWidget->currentWidget())
        emit ui->tabWidget->currentChanged(ui->tabWidget->currentIndex());
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
    QSettings settings("CachyOsTools", "CachyOsTools");
    const auto values = AppPreferences::read(settings);
    applyPreferenceValues(values, {});
    QWidget *startup = preferenceTabWidgets().value(values.value("preferences/startupTab").toString());
    if (startup && ui->tabWidget->indexOf(startup) >= 0) ui->tabWidget->setCurrentWidget(startup);
    if (loadWindowSizeEnabled()) loadWindowGeometry();
}

bool MainWindow::loadWindowSizeEnabled() {
    QSettings settings("CachyOsTools", "CachyOsTools");
    return settings.value("preferences/windowSizeEnabled", true).toBool();
}

bool MainWindow::loadAutoSaveEnabled() {
    QSettings settings("CachyOsTools", "CachyOsTools");
    return settings.value("preferences/autoSaveEnabled", true).toBool();
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

    // Clear any minimum size constraints that might have been restored
    setMinimumSize(0, 0);

    // A geometry saved on a big screen must not come back bigger than the screen
    // it is being restored onto. This also rescues a session saved while a layout
    // bug was forcing the window oversized: without the clamp that size persists
    // in QSettings and every later launch reopens huge even after the bug is
    // fixed. Leaves the window alone when it already fits.
    if (QScreen *screen = this->screen() ? this->screen() : QGuiApplication::primaryScreen()) {
        const QRect avail = screen->availableGeometry();
        const int w = qMin(width(), avail.width());
        const int h = qMin(height(), avail.height());
        if (w != width() || h != height()) resize(w, h);
        if (!avail.contains(frameGeometry().topLeft()))
            move(avail.topLeft());
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (loadWindowSizeEnabled()) {
        saveWindowGeometry();
    }
    saveAllPreferences();
    QMainWindow::closeEvent(event);
}

void MainWindow::scheduleWindowGeometrySave() {
    if (!loadWindowSizeEnabled() || !loadAutoSaveEnabled()) return;
    if (!preferencesGeometryTimer) {
        preferencesGeometryTimer = new QTimer(this);
        preferencesGeometryTimer->setSingleShot(true);
        preferencesGeometryTimer->setInterval(500);
        connect(preferencesGeometryTimer, &QTimer::timeout, this, [this]() {
            if (loadWindowSizeEnabled() && loadAutoSaveEnabled()) saveWindowGeometry();
        });
    }
    preferencesGeometryTimer->start();
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    scheduleWindowGeometrySave();
}

void MainWindow::moveEvent(QMoveEvent *event) {
    QMainWindow::moveEvent(event);
    scheduleWindowGeometrySave();
}
