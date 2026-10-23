#include "preferencesdialog.h"
#include <QtTest>
#include <QtWidgets>

class PreferencesTests : public QObject {
    Q_OBJECT
    template<class T> T *control(PreferencesDialog &dialog, const char *name) {
        auto *widget = dialog.findChild<T *>(QString::fromLatin1(name));
        if (!widget) qFatal("Missing preference control: %s", name);
        return widget;
    }
private slots:
    void legacySettingsAndUnknownKeysSurvive() {
        QTemporaryDir directory;
        const auto path = directory.filePath("settings.ini");
        QSettings settings(path, QSettings::IniFormat);
        settings.setValue("tabs/backupTab", false);
        settings.setValue("preferences/windowSizeEnabled", false);
        settings.setValue("theme/background", "#1d1f24");
        settings.setValue("window/geometry", QByteArray("keep-geometry"));
        settings.setValue("unrelated/setting", "keep-this");
        auto values = AppPreferences::read(settings);
        QVERIFY(!values["tabs/backupTab"].toBool());
        QVERIFY(!values["preferences/windowSizeEnabled"].toBool());
        QVERIFY(!values["preferences/debugIsoCreatorEnabled"].toBool());
        values["preferences/debugIsoCreatorEnabled"] = true;
        values["defaults/config/editor"] = "kate";
        QVERIFY(AppPreferences::write(settings, values));
        QSettings reopened(path, QSettings::IniFormat);
        const auto restored = AppPreferences::read(reopened);
        QCOMPARE(restored, values);
        QCOMPARE(reopened.value("window/geometry").toByteArray(), QByteArray("keep-geometry"));
        QCOMPARE(reopened.value("unrelated/setting").toString(), QString("keep-this"));
    }

    void autosaveCanBeSwitchedOffAndBackOn() {
        auto persisted = AppPreferences::defaults();
        int writes = 0;
        PreferencesDialog dialog(persisted, [&](const auto &values) { persisted = values; ++writes; return true; });
        auto *debug = control<QCheckBox>(dialog, "debugIsoCreatorCheck");
        auto *automatic = control<QCheckBox>(dialog, "preferences_autoSaveEnabled");
        QVERIFY(debug->isEnabled()); QVERIFY(!debug->isChecked());
        debug->setChecked(true);
        QCOMPARE(writes, 1); QVERIFY(persisted["preferences/debugIsoCreatorEnabled"].toBool());
        automatic->setChecked(false);
        debug->setChecked(false);
        QCOMPARE(writes, 1);
        control<QPushButton>(dialog, "preferencesApply")->click();
        QCOMPARE(writes, 2); QVERIFY(!persisted["preferences/autoSaveEnabled"].toBool());
        QVERIFY(!persisted["preferences/debugIsoCreatorEnabled"].toBool());
        debug->setChecked(true);
        QCOMPARE(writes, 2);
        automatic->setChecked(true);
        QCOMPARE(writes, 3);
        QVERIFY(persisted["preferences/debugIsoCreatorEnabled"].toBool());
        QVERIFY(persisted["preferences/autoSaveEnabled"].toBool());
    }

    void cancelDiscardsManualChanges() {
        auto saved = AppPreferences::defaults(); saved["preferences/autoSaveEnabled"] = false;
        int writes = 0;
        PreferencesDialog dialog(saved, [&](const auto &) { ++writes; return true; });
        control<QCheckBox>(dialog, "debugIsoCreatorCheck")->setChecked(true);
        control<QComboBox>(dialog, "language_code")->setCurrentIndex(2);
        control<QPushButton>(dialog, "preferencesCancel")->click();
        QCOMPARE(dialog.result(), int(QDialog::Rejected));
        QCOMPARE(writes, 0);
    }

    void failedSaveCanBeRetried() {
        bool writable = false;
        int attempts = 0;
        PreferencesDialog dialog(AppPreferences::defaults(), [&](const auto &) { ++attempts; return writable; });
        control<QCheckBox>(dialog, "debugIsoCreatorCheck")->setChecked(true);
        QCOMPARE(attempts, 1);
        QVERIFY(control<QPushButton>(dialog, "preferencesApply")->isEnabled());
        QVERIFY(control<QLabel>(dialog, "preferencesStatus")->text().contains("Could not save"));
        writable = true;
        control<QPushButton>(dialog, "preferencesApply")->click();
        QCOMPARE(attempts, 2);
        QVERIFY(!control<QPushButton>(dialog, "preferencesApply")->isEnabled());
    }

    void failedSettingsWriteDoesNotLeakTheDraft() {
        QTemporaryDir directory;
        const auto path = directory.filePath("directory-not-a-settings-file");
        QVERIFY(QDir().mkpath(path));
        QSettings blocked(path, QSettings::IniFormat);
        auto values = AppPreferences::defaults();
        values["preferences/debugIsoCreatorEnabled"] = true;
        QVERIFY(!AppPreferences::write(blocked, values));
        QVERIFY(!blocked.contains("preferences/debugIsoCreatorEnabled"));
        QVERIFY(!blocked.contains("theme/accent"));
    }

    void workspaceAlwaysHasAUsableStartupTab() {
        auto saved = AppPreferences::defaults(); saved["preferences/autoSaveEnabled"] = false;
        int writes = 0;
        PreferencesDialog dialog(saved, [&](const auto &values) { saved = values; ++writes; return true; });
        for (const auto &tab : AppPreferences::tabs()) dialog.findChild<QCheckBox *>("tabs_" + tab.id)->setChecked(false);
        QVERIFY(!control<QPushButton>(dialog, "preferencesApply")->isEnabled());
        QVERIFY(!control<QPushButton>(dialog, "preferencesDone")->isEnabled());
        QCOMPARE(writes, 0);
        control<QCheckBox>(dialog, "tabs_networkTab")->setChecked(true);
        auto *startup = control<QComboBox>(dialog, "preferences_startupTab");
        QCOMPARE(startup->count(), 1);
        QCOMPARE(startup->currentData().toString(), QString("networkTab"));
        control<QPushButton>(dialog, "preferencesApply")->click();
        QCOMPARE(writes, 1);
        QCOMPARE(saved["preferences/startupTab"].toString(), QString("networkTab"));
    }

    void bulkWorkspaceChangesSaveOnce() {
        auto saved = AppPreferences::defaults(); saved["preferences/startupTab"] = "isoCreatorTab";
        int writes = 0;
        PreferencesDialog dialog(saved, [&](const auto &values) { saved = values; ++writes; return true; });
        control<QPushButton>(dialog, "preferencesEssentials")->click();
        QCOMPARE(writes, 1);
        QCOMPARE(saved["preferences/startupTab"].toString(), QString("dashboardTab"));
        QVERIFY(!saved["tabs/isoCreatorTab"].toBool());
        control<QPushButton>(dialog, "preferencesShowAll")->click();
        QCOMPARE(writes, 2);
        for (const auto &tab : AppPreferences::tabs()) QVERIFY(saved["tabs/" + tab.id].toBool());
    }

    void resettingAPageStagesOnlyThatPage() {
        auto saved = AppPreferences::defaults();
        saved["preferences/debugIsoCreatorEnabled"] = true;
        saved["defaults/iso/folderMinimumMB"] = 1500;
        saved["defaults/config/editor"] = "kate";
        int writes = 0;
        PreferencesDialog dialog(saved, [&](const auto &values) { saved = values; ++writes; return true; });
        control<QListWidget>(dialog, "preferencesNavigation")->setCurrentRow(4);
        control<QPushButton>(dialog, "preferencesReset")->click();
        QCOMPARE(writes, 0);
        QVERIFY(!control<QCheckBox>(dialog, "debugIsoCreatorCheck")->isChecked());
        QCOMPARE(control<QSpinBox>(dialog, "defaults_iso_folderMinimumMB")->value(), 500);
        control<QPushButton>(dialog, "preferencesApply")->click();
        QCOMPARE(writes, 1);
        QCOMPARE(saved["defaults/config/editor"].toString(), QString("kate"));
    }

    void themePreviewDoesNotCommitManualEdits() {
        auto saved = AppPreferences::defaults(); saved["preferences/autoSaveEnabled"] = false;
        int writes = 0;
        PreferencesDialog dialog(saved, [&](const auto &values) { saved = values; ++writes; return true; });
        control<QComboBox>(dialog, "theme_preset")->setCurrentIndex(3);
        QVERIFY(control<QWidget>(dialog, "preferencesThemePreview")->styleSheet().contains("#0f1626"));
        QCOMPARE(writes, 0);
        control<QPushButton>(dialog, "preferencesApply")->click();
        QCOMPARE(writes, 1);
        QCOMPARE(saved["theme/accent"].toString(), QString("#3498db"));
        QCOMPARE(saved["theme/background"].toString(), QString("#0f1626"));
    }

    void searchFindsSettingsAcrossPages() {
        PreferencesDialog dialog(AppPreferences::defaults(), [](const auto &) { return true; });
        auto *search = control<QLineEdit>(dialog, "preferencesSearch");
        auto *pages = control<QStackedWidget>(dialog, "preferencesPages");
        auto *navigation = control<QListWidget>(dialog, "preferencesNavigation");
        search->setText("dummy ISO");
        QCOMPARE(pages->currentIndex(), 4);
        QVERIFY(!navigation->item(4)->isHidden());
        QVERIFY(navigation->item(0)->isHidden());
        search->setText("no-such-preference");
        QCOMPARE(pages->currentIndex(), 5);
        QVERIFY(!control<QPushButton>(dialog, "preferencesReset")->isEnabled());
        search->clear();
        for (int i = 0; i < 5; ++i) QVERIFY(!navigation->item(i)->isHidden());
        QVERIFY(pages->currentIndex() < 5);
    }

    void searchKeyboardDoesNotCloseTheDialog() {
        PreferencesDialog dialog(AppPreferences::defaults(), [](const auto &) { return true; });
        dialog.show();
        auto *search = control<QLineEdit>(dialog, "preferencesSearch");
        QSignalSpy closed(&dialog, &QDialog::finished);
        search->setText("dummy"); search->setFocus();
        QTest::keyClick(search, Qt::Key_Return);
        QCOMPARE(closed.count(), 0);
        search->setFocus();
        QTest::keyClick(search, Qt::Key_Escape);
        QCOMPARE(search->text(), QString());
        QCOMPARE(closed.count(), 0);
    }

    void systemPresetRestoresTheDesktopPalette() {
        auto values = AppPreferences::defaults();
        values["theme/preset"] = 1; values["theme/background"] = "#1d1f24";
        PreferencesDialog dialog(values, [](const auto &) { return true; });
        QCOMPARE(dialog.palette().color(QPalette::Window), QColor("#1d1f24"));
        control<QComboBox>(dialog, "theme_preset")->setCurrentIndex(0);
        QCOMPARE(dialog.palette().color(QPalette::Window), QApplication::palette().color(QPalette::Window));
    }

    void corruptSettingsRecoverToUsableValues() {
        QTemporaryDir directory;
        QSettings settings(directory.filePath("settings.ini"), QSettings::IniFormat);
        for (const auto &tab : AppPreferences::tabs()) settings.setValue("tabs/" + tab.id, false);
        settings.setValue("theme/accent", "invalid-color");
        settings.setValue("defaults/iso/fileMinimumMB", -20);
        settings.setValue("preferences/startupTab", "missingTab");
        const auto values = AppPreferences::read(settings);
        QVERIFY(values["tabs/dashboardTab"].toBool());
        QCOMPARE(values["preferences/startupTab"].toString(), QString("dashboardTab"));
        QCOMPARE(values["theme/accent"].toString(), QString("#27ae60"));
        QCOMPARE(values["defaults/iso/fileMinimumMB"].toInt(), 10);
    }

    void layoutAndPreviews() {
        auto values = AppPreferences::defaults();
        values["theme/preset"] = 1; values["theme/background"] = "#1d1f24";
        PreferencesDialog dialog(values, [](const auto &) { return true; });
        dialog.resize(960, 740);
        dialog.show();
        QTest::qWait(30);
        const QString output = qEnvironmentVariable("PREFERENCES_PREVIEW_DIR");
        const QStringList names{"general", "appearance", "workspace", "tools", "iso"};
        for (int i = 0; i < names.size(); ++i) {
            control<QListWidget>(dialog, "preferencesNavigation")->setCurrentRow(i);
            QTest::qWait(10);
            QVERIFY(dialog.rect().contains(control<QPushButton>(dialog, "preferencesDone")->geometry()));
            if (!output.isEmpty()) { QDir().mkpath(output); QVERIFY(dialog.grab().save(output + '/' + names[i] + ".png")); }
        }
        control<QListWidget>(dialog, "preferencesNavigation")->setCurrentRow(1);
        control<QComboBox>(dialog, "theme_preset")->setCurrentIndex(5);
        QTest::qWait(10);
        if (!output.isEmpty()) QVERIFY(dialog.grab().save(output + "/light.png"));
        control<QComboBox>(dialog, "theme_preset")->setCurrentIndex(1);
        control<QListWidget>(dialog, "preferencesNavigation")->setCurrentRow(4);
        dialog.resize(720, 520);
        QTest::qWait(10);
        QCOMPARE(dialog.size(), QSize(720, 520));
        if (!output.isEmpty()) QVERIFY(dialog.grab().save(output + "/compact.png"));
    }
};

QTEST_MAIN(PreferencesTests)
#include "preferences_dialog_tests.moc"
