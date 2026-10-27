#include "file_manager.h"

#include <QApplication>
#include <QDBusContext>
#include <QDBusError>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

class FakeFileManager : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.FileManager1")
public:
    QStringList uris;
    QString startupId;
    int calls = 0;
    bool reject = false;

public slots:
    void ShowFolders(const QStringList &folders, const QString &startup)
    {
        ++calls;
        uris = folders;
        startupId = startup;
        if (reject)
            sendErrorReply(QDBusError::NotSupported, "Test service cannot open folders");
    }
};

class FileManagerTests : public QObject {
    Q_OBJECT
    QTemporaryDir temp;
    FakeFileManager manager;
    QString directory, bin, launchLog, genericLog, errorText;
    QTimer dismissError;

    static bool writeFile(const QString &path, const QByteArray &contents, bool executable = false)
    {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
        if (file.write(contents) != contents.size()) return false;
        return !executable || file.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    }

private slots:
    void initTestCase()
    {
        QVERIFY(temp.isValid());
        directory = temp.path() + QString::fromUtf8("/music & pictures #100% '日本語'");
        bin = temp.path() + "/bin";
        launchLog = temp.path() + "/manager-arguments";
        genericLog = temp.path() + "/wrong-opener";
        QVERIFY(QDir().mkpath(directory));
        QVERIFY(QDir().mkpath(bin));
        qputenv("PATH", bin.toUtf8());
        qputenv("CACHY_FILE_MANAGER_TEST_LOG", launchLog.toUtf8());
        qputenv("CACHY_FILE_MANAGER_WRONG_OPENER", genericLog.toUtf8());
        QVERIFY(writeFile(bin + "/xdg-open",
            "#!/bin/sh\nprintf 'wrong opener' > \"$CACHY_FILE_MANAGER_WRONG_OPENER\"\n", true));

        auto bus = QDBusConnection::sessionBus();
        QVERIFY(bus.isConnected());
        QVERIFY(bus.registerService("org.freedesktop.FileManager1"));
        QVERIFY(bus.registerObject("/org/freedesktop/FileManager1", &manager,
                                   QDBusConnection::ExportAllSlots));

        // Error dialogs are inspected and dismissed without a real display.
        connect(&dismissError, &QTimer::timeout, this, [this]() {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *box = qobject_cast<QMessageBox *>(widget)) {
                    errorText = box->text();
                    box->accept();
                }
            }
        });
        dismissError.start(10);
    }

    void init()
    {
        manager.calls = 0;
        manager.uris.clear();
        manager.reject = false;
        errorText.clear();
        QFile::remove(launchLog);
        QFile::remove(genericLog);
        QVERIFY(writeFile(bin + "/dolphin",
            "#!/bin/sh\nprintf '%s\\n' \"$#\" \"$1\" > \"$CACHY_FILE_MANAGER_TEST_LOG\"\n", true));
    }

    void usesFileManagerService()
    {
        QWidget owner;
        FileManager::openFolder(&owner, directory);
        QTRY_COMPARE(manager.calls, 1);
        QCOMPARE(manager.uris, QStringList{QUrl::fromLocalFile(directory).toString(QUrl::FullyEncoded)});
        QCOMPARE(manager.startupId, QString());
        QTRY_VERIFY(owner.findChildren<QDBusPendingCallWatcher *>().isEmpty());
        QVERIFY(!QFile::exists(launchLog));
        QVERIFY(!QFile::exists(genericLog));
        QVERIFY(errorText.isEmpty());
    }

    void serviceFailureUsesRealManagerWithLiteralPath()
    {
        manager.reject = true;
        QWidget owner;
        FileManager::openFolder(&owner, directory);
        QTRY_VERIFY(QFile::exists(launchLog));
        QFile log(launchLog);
        QVERIFY(log.open(QIODevice::ReadOnly));
        QCOMPARE(log.readAll(), QByteArray("1\n") + directory.toUtf8() + '\n');
        QCOMPARE(manager.calls, 1);
        QVERIFY(!QFile::exists(genericLog));
        QVERIFY(errorText.isEmpty());
    }

    void unavailableServiceUsesInstalledManager()
    {
        auto bus = QDBusConnection::sessionBus();
        QVERIFY(bus.unregisterService("org.freedesktop.FileManager1"));
        QWidget owner;
        FileManager::openFolder(&owner, directory);
        QTRY_VERIFY(QFile::exists(launchLog));
        QCOMPARE(manager.calls, 0);
        QVERIFY(!QFile::exists(genericLog));
        QVERIFY(bus.registerService("org.freedesktop.FileManager1"));
    }

    void noManagerShowsUsefulError()
    {
        manager.reject = true;
        QVERIFY(QFile::remove(bin + "/dolphin"));
        QWidget owner;
        FileManager::openFolder(&owner, directory);
        QTRY_VERIFY(errorText.contains(directory));
        QVERIFY(errorText.contains("file manager"));
        QVERIFY(!QFile::exists(genericLog));
    }

    void missingFolderDoesNotLaunchAnything()
    {
        const QString missing = temp.path() + "/missing";
        QWidget owner;
        FileManager::openFolder(&owner, missing);
        QVERIFY(errorText.contains(missing));
        QCOMPARE(manager.calls, 0);
        QVERIFY(!QFile::exists(launchLog));
        QVERIFY(!QFile::exists(genericLog));
    }
};

int main(int argc, char **argv)
{
    if (qgetenv("CACHY_FILE_MANAGER_TEST_BUS") != "1") {
        qCritical("Run through CTest so these tests use their private D-Bus session.");
        return 1;
    }
    qputenv("QT_QPA_PLATFORM", "offscreen");
    qputenv("QT_QPA_PLATFORMTHEME", "");
    QApplication app(argc, argv);
    FileManagerTests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "file_manager_tests.moc"
