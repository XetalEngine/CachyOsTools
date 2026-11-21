#include "drive_tree.h"
#include "presentation.h"
#include <QtTest>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

class DriveTreeTests : public QObject {
    Q_OBJECT
    QList<DriveInfo> fixture() {
        return DriveTree::parse(
          "NAME=\"/dev/mmcblk0p1\" PKNAME=\"/dev/mmcblk0\" TYPE=\"part\" SIZE=\"104857600\" MOUNTPOINT=\"/boot\"\n"
          "NAME=\"/dev/mmcblk0\" TYPE=\"disk\" SIZE=\"8589934592\"\n"
          "NAME=\"/dev/nvme0n1\" TYPE=\"disk\" SIZE=\"1099511627776\" MODEL=\"Test SSD\"\n"
          "NAME=\"/dev/nvme0n1p1\" PKNAME=\"/dev/nvme0n1\" TYPE=\"part\" SIZE=\"1073741824\" LABEL=\"Guest\\x20Files\"\n"
          "NAME=\"/dev/nvme0n1p2\" PKNAME=\"/dev/nvme0n1\" TYPE=\"part\" SIZE=\"10737418240\"\n"
          "NAME=\"/dev/dm-0\" PKNAME=\"/dev/nvme0n1p2\" TYPE=\"crypt\" SIZE=\"10737418240\" MOUNTPOINT=\"/\"\n"
          "NAME=\"/dev/sdz1\" PKNAME=\"/dev/sdz\" TYPE=\"part\" SIZE=\"1073741824\"\n", "/does-not-exist");
    }
private slots:
    void hierarchyAndFiltering() {
        DriveTree tree; auto drives = fixture(); tree.setDrives(drives, 500*1024*1024);
        QCOMPARE(tree.topLevelItemCount(), 3);
        QVERIFY(tree.deviceItem("/dev/sdz1"));
        auto *mmc = tree.deviceItem("/dev/mmcblk0");
        QCOMPARE(mmc->childCount(), 0);
        QVERIFY(tree.selectDevice("/dev/mmcblk0")); QVERIFY(tree.selectedInUse());
        tree.setDrives(drives, 0);
        QCOMPARE(tree.deviceItem("/dev/mmcblk0p1")->parent(), tree.deviceItem("/dev/mmcblk0"));
        auto *ssd = tree.deviceItem("/dev/nvme0n1"); QCOMPARE(ssd->childCount(), 2); QVERIFY(!ssd->isExpanded());
        QCOMPARE(tree.deviceItem("/dev/nvme0n1p1")->text(4), QString("Guest Files"));
        QVERIFY(tree.selectDevice("/dev/nvme0n1p2")); QVERIFY(tree.selectedInUse());
        QVERIFY(tree.selectedMount().isEmpty());
    }
    void sortingRefreshAndCollapseKeepActionTarget() {
        DriveTree tree; auto drives = fixture(); tree.setDrives(drives, 0);
        QVERIFY(tree.selectDevice("/dev/nvme0n1p1"));
        tree.sortItems(2, Qt::DescendingOrder); QCOMPARE(tree.selectedDevice(), QString("/dev/nvme0n1p1"));
        tree.setDrives(drives, 0); QCOMPARE(tree.selectedDevice(), QString("/dev/nvme0n1p1"));
        QVERIFY(tree.deviceItem("/dev/nvme0n1")->isExpanded());
        tree.deviceItem("/dev/nvme0n1")->setExpanded(false);
        QCOMPARE(tree.selectedDevice(), QString("/dev/nvme0n1"));
        tree.setDrives(drives, 0); QVERIFY(!tree.deviceItem("/dev/nvme0n1")->isExpanded());
        tree.selectDevice("/dev/mmcblk0p1"); tree.setDrives(drives, 500*1024*1024);
        QCOMPARE(tree.selectedDevice(), QString("/dev/mmcblk0"));
        tree.setDrives({}, 0); QVERIFY(tree.selectedDevice().isEmpty());
    }
    void byteSorting() {
        DriveTree tree; tree.setDrives(fixture(), 0); tree.sortItems(2, Qt::AscendingOrder);
        QCOMPARE(tree.topLevelItem(0)->data(0, DriveTree::DeviceRole).toString(), QString("/dev/sdz1"));
        QCOMPARE(tree.topLevelItem(1)->data(0, DriveTree::DeviceRole).toString(), QString("/dev/mmcblk0"));
        QCOMPARE(tree.topLevelItem(2)->data(0, DriveTree::DeviceRole).toString(), QString("/dev/nvme0n1"));
    }
    void keyboardAndMouseExpansion() {
        DriveTree tree; tree.setDrives(fixture(), 0); tree.resize(1200,500); tree.show();
        QVERIFY(tree.selectDevice("/dev/nvme0n1"));
        QTest::keyClick(&tree, Qt::Key_Right); QVERIFY(tree.currentItem()->isExpanded());
        QTest::keyClick(&tree, Qt::Key_Left); QVERIFY(!tree.currentItem()->isExpanded());
        const auto rect = tree.visualItemRect(tree.currentItem());
        QTest::mouseClick(tree.viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(12, rect.center().y()));
        QVERIFY(tree.currentItem()->isExpanded());
    }
    void filtersPreserveSelectionAndMountProtection() {
        DriveTree tree; auto drives = fixture();
        tree.setDrives(drives, 0, false, true);
        QVERIFY(!tree.deviceItem("/dev/nvme0n1"));
        QVERIFY(tree.selectDevice("/dev/nvme0n1p2"));
        QVERIFY(tree.selectedInUse());
        tree.setDrives(drives, 0, true, false);
        QVERIFY(tree.deviceItem("/dev/nvme0n1"));
        QVERIFY(!tree.deviceItem("/dev/nvme0n1p2"));
        QVERIFY(tree.selectedDevice().isEmpty());
        tree.setDrives(drives, 0, false, false);
        QCOMPARE(tree.topLevelItemCount(), 0);
    }
    void themeRenderings() {
        QDir directory(qEnvironmentVariable("CACHY_DRIVE_TEST_THEMES"));
        if (directory.path() == ".") return;
        for (const auto &name : directory.entryList({"*.json"}, QDir::Files)) {
            QFile file(directory.filePath(name)); QVERIFY(file.open(QIODevice::ReadOnly));
            const auto theme = QJsonDocument::fromJson(file.readAll()).object();
            qApp->setStyleSheet(theme["stylesheet"].toString());
            auto palette = qApp->palette(); const auto colors = theme["colors"].toObject();
            for (auto role : {QPalette::Window, QPalette::Base, QPalette::AlternateBase, QPalette::Text, QPalette::HighlightedText, QPalette::Highlight}) {
                const auto key = QMetaEnum::fromType<QPalette::ColorRole>().valueToKey(role);
                palette.setColor(role, QColor(colors[key].toString()));
            }
            qApp->setPalette(palette);
            DriveTree tree; tree.setObjectName("drivesTable"); tree.setDrives(fixture(),0);
            tree.resize(1500,450); tree.show(); tree.selectDevice("/dev/nvme0n1p1"); QTest::qWait(25);
            QVERIFY(tree.visualItemRect(tree.currentItem()).height() >= 28);
            QVERIFY(tree.grab().save(directory.filePath(name+".png")));
        }
    }
};
QTEST_MAIN(DriveTreeTests)
#include "drive_tree_tests.moc"
