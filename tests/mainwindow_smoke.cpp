#include "mainwindow.h"
#include "preferencesdialog.h"
#include "ui_components/grub_editor.h"
#include "ui_components/network_page.h"
#include "ui_components/disk_visualizer.h"
#include <QtTest>
#include <QtWidgets>

class MainWindowSmoke final : public QObject {
    Q_OBJECT
private slots:
    void originalToolsAndNewPagesRemainReachable() {
        QVERIFY(qEnvironmentVariableIsSet("CACHY_SMOKE_ISOLATED"));
        MainWindow window;
        window.resize(1280, 900); window.show();
        for (int i=0; i<3; ++i)
            QVERIFY(QMetaObject::invokeMethod(&window, "on_refreshButton_clicked", Qt::DirectConnection));
        QTest::qWait(30); // Failed starts and rapid replacements must not destroy a process in its callback.
        auto *tabs = window.findChild<QTabWidget *>("tabWidget"); QVERIFY(tabs);
        QCOMPARE(tabs->count(), AppPreferences::tabs().size());
        QVERIFY(window.findChild<GrubEditor *>());
        QVERIFY(window.findChild<NetworkPage *>());
        QVERIFY(window.findChild<DiskVisualizer *>());
        auto *theme = window.findChild<QComboBox *>("themePresetCombo"); QVERIFY(theme);
        auto *tweaks = window.findChild<QTabWidget *>("tweaksTabWidget"); QVERIFY(tweaks);
        const QString pictures = qEnvironmentVariable("CACHY_UI_SCREENSHOTS");
        if (!pictures.isEmpty()) QDir().mkpath(pictures);
        const QStringList tools{"dashboardTab", "driveToolsTab", "grubEditorTab", "shellToolsTab", "servicesTab",
            "tweaksTab", "kvmTab", "configTab", "logsTab", "backupTab", "restoreTab", "isoCreatorTab",
            "packageManagerTab", "uninstallTab", "cleanerTab", "networkTab", "themeTab"};
        for (int preset : {1, 5, 6}) {
            theme->setCurrentIndex(preset);
            emit theme->activated(preset);
            QCOMPARE(qApp->palette().color(QPalette::Window), QColor(preset == 1 ? "#1d1f24" : preset == 5 ? "#f5f1e8" : "#191221"));
            for (const auto &name : tools) {
                auto *page = window.findChild<QWidget *>(name); QVERIFY2(page, qPrintable(name));
                const int at = tabs->indexOf(page); QVERIFY2(at >= 0, qPrintable(name));
                tabs->setCurrentIndex(at); window.resize(1280, 900); QTest::qWait(60);
                if (!pictures.isEmpty()) QVERIFY(window.grab().save(pictures + '/' + name + '-' + QString::number(preset) + ".png"));
                window.resize(1100, 720); QTest::qWait(20);
                QVERIFY2(window.width() <= 1100 && window.height() <= 720, qPrintable(name + " exceeds the requested window size"));
            }
        }
        auto *gallery = window.findChild<QGroupBox *>("themeGallery"); QVERIFY(gallery);
        const auto cards = gallery->findChildren<QAbstractButton *>(); QCOMPARE(cards.size(), 6);
        QTest::mouseClick(cards.first(), Qt::LeftButton);
        QCOMPARE(theme->currentIndex(), 1);
        QCOMPARE(qApp->palette().color(QPalette::Window), QColor("#1d1f24"));
        tabs->setCurrentWidget(window.findChild<QWidget *>("tweaksTab"));
        for (int i=0; i<tweaks->count(); ++i) {
            tweaks->setCurrentIndex(i); QTest::qWait(20);
            auto *page = tweaks->currentWidget();
            auto *search = page->findChild<QLineEdit *>("tweakSearch"); QVERIFY(search);
            const auto groups = page->findChildren<QGroupBox *>(); QVERIFY(!groups.isEmpty());
            int visible = 0;
            for (auto *group : groups) if (group->isVisibleTo(page)) ++visible;
            QVERIFY2(visible > 0, qPrintable(tweaks->tabText(i)));
            search->setText("nonexistent setting fixture"); QTest::qWait(20);
            for (auto *group : groups) QVERIFY(!group->isVisibleTo(page));
            search->clear(); QTest::qWait(20);
            visible = 0; for (auto *group : groups) if (group->isVisibleTo(page)) ++visible;
            QVERIFY(visible > 0);
        }
    }
};
QTEST_MAIN(MainWindowSmoke)
#include "mainwindow_smoke.moc"
