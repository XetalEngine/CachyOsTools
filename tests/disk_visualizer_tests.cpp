#include "disk_visualizer.h"
#include "disk_layout.h"
#include "panel_widgets.h"
#include "presentation.h"
#include <QtTest>
#include <QtWidgets>
#include <QJsonDocument>
#include <QProcess>

class DiskVisualizerTests final : public QObject {
  Q_OBJECT
private slots:
  void squarifiedAreaPreservesEveryByte() {
    const QVector<double> weights{110, 9000, 500, 0, 13, 88, 100, 1};
    const QRectF bounds(9, 13, 813, 415);
    const auto tiles = DiskLayout::rectangles(weights,bounds);
    const auto total = std::accumulate(weights.begin(),weights.end(),0.);
    for (int i = 0; i < tiles.size(); ++i) {
      const auto area = tiles[i].width() * tiles[i].height();
      QVERIFY(qAbs(area - weights[i] / total * bounds.width() * bounds.height()) < .0001);
      if (!weights[i]) continue;
      QVERIFY(bounds.adjusted(-.0001,-.0001,.0001,.0001).contains(tiles[i]));
      for (int j = i + 1; j < tiles.size(); ++j) {
        auto overlap = tiles[i].intersected(tiles[j]);
        QVERIFY(overlap.width() * overlap.height() < .0001);
      }
    }
    QVERIFY(DiskLayout::rectangles({0,0},bounds)[0].isEmpty());
  }
  void scansLazilyNavigatesAndFits() {
    qApp->setPalette(CachyUi::palette(qApp->palette(), QColor("#27ae60"), "#1d1f24", "#eeeeee"));
    qApp->setStyleSheet(CachyUi::stylesheet());
    QTemporaryDir fixture;
    QVERIFY(fixture.isValid());
    auto put = [&](QString name,int size){
      const auto path = fixture.path() + "/" + name; QDir().mkpath(QFileInfo(path).absolutePath());
      QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(QByteArray(size,'x')),qint64(size));
    };
    put("Projects/Binary Knife/symbols.db",900000);
    put("Projects/Echo/samples.wav",600000);
    put("Projects/Hyper Fusion/desktop.qml",340000);
    put("Videos/desktop-tour.mp4",1200000);
    put("Pictures/wallpaper.png",400000);
    put("Archives/system-backup.tar",700000);
    put("notes.txt",60000); put("readme.md",20000);
    bool authenticated = false;
    DiskVisualizer panel([&]{ authenticated = true; return false; });
    panel.resize(1220,650); panel.show();
    QSignalSpy ready(&panel,&DiskVisualizer::viewReady);
    panel.scanFixture(fixture.path());
    QTRY_VERIFY_WITH_TIMEOUT(ready.count() > 0,10000);
    QVERIFY(!authenticated);
    auto *tree = panel.findChild<QTreeWidget *>("diskTree"); QVERIFY(tree);
    QCOMPARE(tree->topLevelItemCount(),1);
    auto *root = tree->topLevelItem(0);
    QTRY_COMPARE(root->childCount(),5);
    QCOMPARE(root->child(0)->text(0),QString("Projects"));
    QCOMPARE(root->child(4)->text(0),QString("Files"));
    QVERIFY(root->child(0)->data(0,Qt::UserRole+1).toString() == "unloaded");
    root->child(0)->setExpanded(true);
    QTRY_COMPARE(root->child(0)->childCount(),3);
    const auto screenshot = qEnvironmentVariable("DISK_TEST_CAPTURE");
    if (!screenshot.isEmpty()) { QTest::qWait(220); QVERIFY(panel.grab().save(screenshot)); }
    auto *map = panel.findChild<QWidget *>("diskTreemap"); QVERIFY(map); QVERIFY(map->width() > 400); QVERIFY(map->height() > 240);
    const int before = ready.count();
    QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,tree->visualItemRect(root->child(4)).center());
    QTest::mouseDClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,tree->visualItemRect(root->child(4)).center());
    QTRY_VERIFY(ready.count() > before);
    QVERIFY(panel.findChildren<QPushButton *>().size() > 6);
    for (QSize size : {QSize(1220,650),QSize(1050,650)}) {
      panel.resize(size); QTest::qWait(20);
      for (auto *button : panel.findChildren<QPushButton *>()) {
        if (!button->isVisible()) continue;
        const auto position = button->mapTo(&panel,QPoint());
        QVERIFY2(position.x() + button->width() <= panel.width() + 1,qPrintable(button->text()));
      }
    }
  }
  void deletionUpdatesCachedMapAndKeepsCurrentFolder() {
    const auto profile = QDir(QFileInfo(QStringLiteral(__FILE__)).absolutePath()).absoluteFilePath("../../..");
    qputenv("XETAL_HYPR_ROOT",profile.toUtf8());
    QTemporaryDir fixture;
    QVERIFY(fixture.isValid());
    auto put = [&](const QString &name,int size) {
      const auto path = fixture.path() + "/" + name;
      QDir().mkpath(QFileInfo(path).absolutePath());
      QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly));
      QCOMPARE(file.write(QByteArray(size,'x')),qint64(size));
    };
    put("parent/remove/nested/file",50000);
    put("parent/stay",123);
    put("unrelated/keep",200);
    DiskVisualizer panel([]{ return false; });
    panel.resize(1220,650); panel.show();
    QSignalSpy ready(&panel,&DiskVisualizer::viewReady);
    panel.scanFixture(fixture.path());
    QTRY_VERIFY_WITH_TIMEOUT(ready.count() > 0,10000);
    auto *tree = panel.findChild<QTreeWidget *>("diskTree"); QVERIFY(tree);
    auto findItem = [&](const QString &relative) -> QTreeWidgetItem * {
      for (QTreeWidgetItemIterator it(tree); *it; ++it)
        if ((*it)->data(0,Qt::UserRole).toJsonObject()["path"] == fixture.path() + "/" + relative)
          return *it;
      return nullptr;
    };
    QTRY_VERIFY(findItem("parent"));
    findItem("parent")->setExpanded(true);
    QTRY_VERIFY(findItem("parent/remove"));
    int before = ready.count();
    tree->setCurrentItem(findItem("parent/remove"));
    QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,
                     tree->visualItemRect(findItem("parent/remove")).center());
    QTest::mouseDClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,
                      tree->visualItemRect(findItem("parent/remove")).center());
    QTRY_VERIFY(ready.count() > before);

    // A rescan would pick up this new file. An incremental delete must leave
    // unrelated cached branches alone and keep their original IDs and totals.
    put("unrelated/not-in-snapshot",9999);
    QPushButton *remove = nullptr;
    for (auto *button : panel.findChildren<QPushButton *>())
      if (button->text() == "Delete permanently…") remove = button;
    QVERIFY(remove); QVERIFY(remove->isEnabled());
    auto answerConfirmation = [&](bool accept) {
      bool answered = false;
      QTimer answer;
      connect(&answer,&QTimer::timeout,&panel,[&] {
        auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        if (!box) return;
        QCOMPARE(box->windowTitle(),QString("Delete permanently"));
        for (auto *button : box->buttons()) {
          if (box->buttonRole(button) != (accept ? QMessageBox::DestructiveRole : QMessageBox::RejectRole)) continue;
          answer.stop(); answered = true; button->click(); break;
        }
      });
      answer.start(10);
      remove->click();
      QTRY_VERIFY_WITH_TIMEOUT(answered,5000);
    };
    answerConfirmation(false);
    QVERIFY(QFileInfo::exists(fixture.path() + "/parent/remove/nested/file"));
    before = ready.count();
    answerConfirmation(true);
    QTRY_VERIFY_WITH_TIMEOUT(ready.count() > before,5000);
    QVERIFY(!QFileInfo::exists(fixture.path() + "/parent/remove"));
    QVERIFY(QFileInfo::exists(fixture.path() + "/parent/stay"));
    QTRY_VERIFY(findItem("parent"));
    QCOMPARE(tree->topLevelItem(0)->data(0,Qt::UserRole).toJsonObject()["size"].toInt(),323);
    QCOMPARE(tree->topLevelItem(0)->data(0,Qt::UserRole).toJsonObject()["files"].toInt(),2);
    QVERIFY(panel.findChild<QLabel *>("diskDetails")->text().startsWith(fixture.path() + "/parent\n"));
    QVERIFY(!findItem("parent/remove"));
    bool updated = false;
    for (auto *label : panel.findChildren<QLabel *>()) updated |= label->text().contains("Map updated without rescanning");
    QVERIFY(updated);

    // The parent's scanned identity must be refreshed after removing its child.
    tree->setCurrentItem(findItem("parent"));
    before = ready.count();
    answerConfirmation(true);
    QTRY_VERIFY_WITH_TIMEOUT(ready.count() > before,5000);
    QVERIFY(!QFileInfo::exists(fixture.path() + "/parent"));
    QCOMPARE(tree->topLevelItem(0)->data(0,Qt::UserRole).toJsonObject()["size"].toInt(),200);
    QVERIFY(QFileInfo::exists(fixture.path() + "/unrelated/not-in-snapshot"));
    QVERIFY(panel.findChild<QLabel *>("diskDetails")->text().startsWith(fixture.path() + "\n"));
  }
};
QTEST_MAIN(DiskVisualizerTests)
#include "disk_visualizer_tests.moc"
