#include "drivelabel.h"

#include <QDBusObjectPath>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
#include <memory>

namespace {
struct FakeStorage {
    QVariantMap block{
        {"Device", QByteArray("/dev/test1\0", 11)}, {"DeviceNumber", qulonglong(2049)},
        {"IdUsage", "filesystem"}, {"IdType", "ntfs"}, {"IdUUID", "1234-5678"},
        {"IdLabel", "Windows_10_SATA"}, {"ReadOnly", false}, {"Size", qulonglong(1024ULL * 1024 * 1024 * 100)},
        {"Drive", QVariant::fromValue(QDBusObjectPath("/org/freedesktop/UDisks2/drives/test"))}
    };
    QVariantMap fs{{"MountPoints", QVariant::fromValue(QList<QByteArray>{})}};
    QList<QDBusMessage> calls;
    QString readError, writeError;
    bool present = true, duplicate = false, keepOldLabel = false, holdWrite = false;
    DriveLabelService::Reply pendingWrite;
    QDBusMessage pendingMessage;
    int writeCount() const {
        int count = 0;
        for (const auto &call : calls) if (call.member() == "SetLabel") ++count;
        return count;
    }
    void respond(const QDBusMessage &message, DriveLabelService::Reply reply) {
        calls << message;
        if (message.member() == "ResolveDevice") {
            if (!readError.isEmpty()) { reply(message.createErrorReply("org.freedesktop.UDisks2.Error.Failed", readError)); return; }
            QList<QDBusObjectPath> paths;
            if (present) paths << QDBusObjectPath("/org/freedesktop/UDisks2/block_devices/test1");
            if (duplicate) paths << paths.first();
            reply(message.createReply({QVariant::fromValue(paths)}));
        } else if (message.member() == "GetAll") {
            const bool isBlock = message.arguments().first().toString().endsWith(".Block");
            reply(message.createReply(QVariantList{isBlock ? block : fs}));
        } else if (message.member() == "SetLabel") {
            if (holdWrite) { pendingWrite = reply; pendingMessage = message; return; }
            finishWrite(message, reply);
        } else {
            QFAIL("Unexpected method: labeling must never format, mount, repair or change a UUID.");
        }
    }
    void finishWrite(const QDBusMessage &message, DriveLabelService::Reply reply) {
        if (!writeError.isEmpty()) {
            reply(message.createErrorReply("org.freedesktop.UDisks2.Error.Failed", writeError));
        } else {
            if (!keepOldLabel) block["IdLabel"] = message.arguments().first().toString();
            reply(message.createReply());
        }
    }
};
DriveLabelService::Transport transport(const std::shared_ptr<FakeStorage> &storage) {
    return [storage](const QDBusMessage &message, DriveLabelService::Reply reply) { storage->respond(message, reply); };
}
DriveLabel::Volume inspect(DriveLabelService &service) {
    QSignalSpy spy(&service, &DriveLabelService::inspected);
    service.inspect("/dev/test1");
    return spy.isEmpty() ? DriveLabel::Volume{} : qvariant_cast<DriveLabel::Volume>(spy.first().first());
}
}

class DriveLabelTests : public QObject {
    Q_OBJECT
    QTemporaryDir m_tools;
    QByteArray m_oldPath;
private slots:
    void initTestCase() {
        // Only presence is inspected. No test invokes a real filesystem utility.
        QVERIFY(m_tools.isValid());
        QFile stub(m_tools.filePath("ntfslabel"));
        QVERIFY(stub.open(QIODevice::WriteOnly));
        stub.write("#!/bin/sh\nexit 99\n"); stub.close();
        QVERIFY(stub.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        m_oldPath = qgetenv("PATH");
        qputenv("PATH", m_tools.path().toUtf8() + ":" + m_oldPath);
    }
    void cleanupTestCase() { qputenv("PATH", m_oldPath); }
    void labelRules_data() {
        QTest::addColumn<QString>("filesystem"); QTest::addColumn<QString>("label"); QTest::addColumn<bool>("valid");
        QTest::newRow("windows name") << "ntfs" << "Windows 10 SATA" << true;
        QTest::newRow("ntfs boundary") << "ntfs" << QString(32, 'N') << true;
        QTest::newRow("ntfs long") << "ntfs" << QString(33, 'N') << false;
        QTest::newRow("utf16 boundary") << "exfat" << QString::fromUtf8("😀😀😀😀😀A") << true;
        QTest::newRow("utf16 long") << "exfat" << QString::fromUtf8("😀😀😀😀😀😀") << false;
        QTest::newRow("ext byte boundary") << "ext4" << QString::fromUtf8("éééééééé") << true;
        QTest::newRow("ext bytes long") << "ext4" << QString::fromUtf8("ééééééééA") << false;
        QTest::newRow("fat upper") << "vfat" << "GAMES DISK" << true;
        QTest::newRow("fat lower") << "vfat" << "Games" << false;
        QTest::newRow("fat reserved") << "vfat" << "NO NAME" << false;
        QTest::newRow("fat punctuation") << "vfat" << "GAME.DISK" << false;
        QTest::newRow("fat encoding") << "vfat" << QString::fromUtf8("CAFÉ") << false;
        QTest::newRow("xfs byte boundary") << "xfs" << QString(12, 'X') << true;
        QTest::newRow("xfs spaces") << "xfs" << "MY DATA" << false;
        QTest::newRow("btrfs long") << "btrfs" << QString(256, 'B') << false;
        QTest::newRow("remove label") << "ext2" << "" << true;
        QTest::newRow("shell characters are data") << "ext3" << "$(id);`id`" << true;
        QTest::newRow("leading option") << "ntfs" << "--new-serial" << false;
        QTest::newRow("null") << "ntfs" << QString("hello") + QChar(0) + "world" << false;
        QTest::newRow("newline") << "ntfs" << "hello\nworld" << false;
        QTest::newRow("bidi control") << "ntfs" << QString("hello") + QChar(0x202e) << false;
        QTest::newRow("leading whitespace") << "ext4" << " label" << false;
        QTest::newRow("unsupported") << "crypto_LUKS" << "Data" << false;
    }
    void labelRules() {
        QFETCH(QString, filesystem); QFETCH(QString, label); QFETCH(bool, valid);
        QCOMPARE(DriveLabel::validate(filesystem, label).isEmpty(), valid);
    }
    void onlyLabelIsChanged() {
        auto disk = std::make_shared<FakeStorage>();
        DriveLabelService service(nullptr, transport(disk));
        const auto v = inspect(service);
        QCOMPARE(v.device, QString("/dev/test1"));
        QCOMPARE(v.label, QString("Windows_10_SATA"));
        const QString label = "Joe's $HOME & Games";
        QSignalSpy saved(&service, &DriveLabelService::saved), failed(&service, &DriveLabelService::failed);
        service.save(v, label);
        QCOMPARE(saved.size(), 1); QCOMPARE(failed.size(), 0); QCOMPARE(disk->writeCount(), 1);
        QCOMPARE(disk->block["IdUUID"].toString(), v.uuid);
        for (const auto &message : disk->calls) {
            if (message.member() == "SetLabel") {
                QCOMPARE(message.interface(), QString("org.freedesktop.UDisks2.Filesystem"));
                QCOMPARE(message.path(), v.objectPath);
                QCOMPARE(message.arguments(), QVariantList({label, QVariantMap{}}));
                QVERIFY(message.isInteractiveAuthorizationAllowed());
            }
        }
    }
    void removeLabelAndNoOp() {
        auto disk = std::make_shared<FakeStorage>();
        DriveLabelService service(nullptr, transport(disk));
        auto v = inspect(service);
        service.save(v, v.label);
        QCOMPARE(disk->writeCount(), 0);
        QSignalSpy saved(&service, &DriveLabelService::saved);
        service.save(v, "");
        QCOMPARE(disk->writeCount(), 1); QCOMPARE(saved.size(), 1);
        QCOMPARE(saved.first().first().toString(), QString());
    }
    void changedOrUnsafeVolume_data() {
        QTest::addColumn<QString>("change");
        for (const char *change : {"mounted", "readOnly", "uuid", "deviceNumber", "size", "filesystem", "drive", "label", "removed", "ambiguous", "missingProperty", "malformedReadOnly", "missingUuid", "missingMounts", "malformedMounts", "encrypted", "readFailure"})
            QTest::newRow(change) << change;
    }
    void changedOrUnsafeVolume() {
        QFETCH(QString, change);
        auto disk = std::make_shared<FakeStorage>();
        DriveLabelService service(nullptr, transport(disk));
        auto v = inspect(service);
        if (change == "mounted") disk->fs["MountPoints"] = QVariant::fromValue(QList<QByteArray>{QByteArray("/\0", 2)});
        if (change == "readOnly") disk->block["ReadOnly"] = true;
        if (change == "uuid") disk->block["IdUUID"] = "new-volume";
        if (change == "deviceNumber") disk->block["DeviceNumber"] = qulonglong(2050);
        if (change == "size") disk->block["Size"] = qulonglong(1024);
        if (change == "filesystem") disk->block["IdType"] = "ext4";
        if (change == "drive") disk->block["Drive"] = QVariant::fromValue(QDBusObjectPath("/different"));
        if (change == "label") disk->block["IdLabel"] = "External rename";
        if (change == "removed") disk->present = false;
        if (change == "ambiguous") disk->duplicate = true;
        if (change == "missingProperty") disk->block.remove("ReadOnly");
        if (change == "malformedReadOnly") disk->block["ReadOnly"] = "unknown";
        if (change == "missingUuid") disk->block["IdUUID"] = "";
        if (change == "missingMounts") disk->fs.remove("MountPoints");
        if (change == "malformedMounts") disk->fs["MountPoints"] = "not-an-array";
        if (change == "encrypted") disk->block["IdUsage"] = "crypto";
        if (change == "readFailure") disk->readError = "Storage service unavailable";
        QSignalSpy saved(&service, &DriveLabelService::saved), failed(&service, &DriveLabelService::failed);
        service.save(v, "Games");
        QCOMPARE(disk->writeCount(), 0); QCOMPARE(saved.size(), 0); QCOMPARE(failed.size(), 1);
        QVERIFY(!service.busy());
    }
    void invalidNameNeverWrites() {
        auto disk = std::make_shared<FakeStorage>();
        DriveLabelService service(nullptr, transport(disk));
        auto v = inspect(service);
        QSignalSpy failed(&service, &DriveLabelService::failed);
        service.save(v, "--new-serial");
        QCOMPARE(failed.size(), 1); QCOMPARE(disk->writeCount(), 0);
    }
    void hibernationErrorIsNotRetriedOrForced() {
        auto disk = std::make_shared<FakeStorage>();
        disk->writeError = "Windows is hibernated, refused to mount.";
        DriveLabelService service(nullptr, transport(disk));
        auto v = inspect(service);
        QSignalSpy failed(&service, &DriveLabelService::failed), saved(&service, &DriveLabelService::saved);
        service.save(v, "Games");
        QCOMPARE(failed.size(), 1); QCOMPARE(saved.size(), 0); QCOMPARE(disk->writeCount(), 1);
        QVERIFY(failed.first().first().toString().contains(disk->writeError));
        QCOMPARE(disk->block["IdLabel"].toString(), v.label);
    }
    void verificationFailureDoesNotReportSuccess() {
        auto disk = std::make_shared<FakeStorage>(); disk->keepOldLabel = true;
        DriveLabelService service(nullptr, transport(disk));
        auto v = inspect(service);
        QSignalSpy failed(&service, &DriveLabelService::failed), saved(&service, &DriveLabelService::saved);
        service.save(v, "Games");
        QTRY_COMPARE(failed.size(), 1); QCOMPARE(saved.size(), 0); QCOMPARE(disk->writeCount(), 1);
    }
    void concurrentSaveIsIgnored() {
        auto disk = std::make_shared<FakeStorage>(); disk->holdWrite = true;
        DriveLabelService service(nullptr, transport(disk));
        auto v = inspect(service);
        service.save(v, "Games");
        QVERIFY(service.busy());
        service.save(v, "Other name");
        QCOMPARE(disk->writeCount(), 1);
        disk->finishWrite(disk->pendingMessage, disk->pendingWrite);
        QVERIFY(!service.busy()); QCOMPARE(disk->block["IdLabel"].toString(), QString("Games"));
    }
    void dialogCancelAndValidation() {
        auto disk = std::make_shared<FakeStorage>();
        DriveLabelDialog dialog("/dev/test1", nullptr, transport(disk));
        dialog.show();
        auto *edit = dialog.findChild<QLineEdit *>("volumeLabelEdit");
        auto *save = dialog.findChild<QPushButton *>("saveVolumeLabelButton");
        QTRY_VERIFY(edit->isEnabled());
        QVERIFY(!save->isEnabled());
        edit->setText("--new-serial"); QVERIFY(!save->isEnabled());
        edit->setText("Games"); QVERIFY(save->isEnabled());
        edit->clear(); QVERIFY(save->isEnabled()); QCOMPARE(save->text(), QString("Remove Label"));
        edit->setText("Games");
        const auto preview = qEnvironmentVariable("DRIVE_LABEL_PREVIEW_DIR");
        if (!preview.isEmpty()) {
            QDir().mkpath(preview);
            QTest::qWait(20);
            QVERIFY(dialog.grab().save(preview + "/set-label.png"));
        }
        dialog.reject();
        QCOMPARE(disk->writeCount(), 0);
    }
    void dialogCannotCloseDuringSave() {
        auto disk = std::make_shared<FakeStorage>(); disk->holdWrite = true;
        DriveLabelDialog dialog("/dev/test1", nullptr, transport(disk)); dialog.show();
        auto *edit = dialog.findChild<QLineEdit *>("volumeLabelEdit");
        auto *save = dialog.findChild<QPushButton *>("saveVolumeLabelButton");
        QTRY_VERIFY(edit->isEnabled()); edit->setText("Games"); save->click();
        dialog.reject(); QVERIFY(dialog.isVisible()); QVERIFY(!save->isEnabled());
        QSignalSpy changed(&dialog, &DriveLabelDialog::labelChanged);
        disk->finishWrite(disk->pendingMessage, disk->pendingWrite);
        QCOMPARE(changed.size(), 1); QCOMPARE(dialog.result(), int(QDialog::Accepted));
    }
    void dialogReportsMountedVolume() {
        auto disk = std::make_shared<FakeStorage>();
        disk->fs["MountPoints"] = QVariant::fromValue(QList<QByteArray>{QByteArray("/boot/efi\0", 10)});
        DriveLabelDialog dialog("/dev/test1", nullptr, transport(disk)); dialog.show();
        auto *status = dialog.findChild<QLabel *>("volumeLabelStatus");
        QTRY_VERIFY(status->text().contains("Unmount"));
        QVERIFY(!dialog.findChild<QLineEdit *>("volumeLabelEdit")->isEnabled());
        QVERIFY(!dialog.findChild<QPushButton *>("saveVolumeLabelButton")->isEnabled());
        QCOMPARE(disk->writeCount(), 0);
    }
    void escapedLabelsAndFstabReferences() {
        QCOMPARE(DriveLabel::decodeLsblkValue("Joe\\x27s\\x20Disk"), QString("Joe's Disk"));
        QCOMPARE(DriveLabel::decodeLsblkValue("caf\\xc3\\xa9\\x5c"), QString::fromUtf8("café\\"));
        QVERIFY(DriveLabel::fstabUsesLabel("LABEL=My\\040Disk /games ntfs defaults 0 0", "My Disk"));
        QVERIFY(DriveLabel::fstabUsesLabel("/dev/disk/by-label/Games /games ntfs defaults 0 0", "Games"));
        QVERIFY(DriveLabel::fstabUsesLabel("/dev/disk/by-label/My\\x20Disk /games ntfs defaults 0 0", "My Disk"));
        QVERIFY(!DriveLabel::fstabUsesLabel("# LABEL=Games /games ntfs defaults 0 0", "Games"));
        QVERIFY(!DriveLabel::fstabUsesLabel("UUID=123 /games ntfs defaults 0 0", "Games"));
        QVERIFY(!DriveLabel::fstabUsesLabel("LABEL=Games2 /games ntfs defaults 0 0", "Games"));
    }
};
QTEST_MAIN(DriveLabelTests)
#include "drive_label_tests.moc"
