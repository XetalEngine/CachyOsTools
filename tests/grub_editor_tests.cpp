#include "grub_editor.h"
#include "grub_appearance.h"
#include "presentation.h"
#include <QtTest>
#include <QtWidgets>

class GrubEditorTests final : public QObject {
  Q_OBJECT
private slots:
  void appearancePreviewAndExplicitApply() {
    const auto helper = QFINDTESTDATA("../tools/grub_appearance.py"); QVERIFY(!helper.isEmpty());
    GrubAppearance page(helper); page.resize(1500, 620);
    page.setStyleSheet("QPushButton, QComboBox, QLineEdit { padding:7px 10px; } "
                      "QTabBar::tab { padding:8px 12px; } QWidget { font-size:13px; }");
    page.setMenu({QJsonObject{{"name", "My Linux"}, {"kind", "menuentry"}},
                  QJsonObject{{"name", "UEFI Firmware Settings"}, {"kind", "menuentry"}}});
    page.setAvailability(false, true, true);
    QSignalSpy requests(&page, &GrubAppearance::requested); page.show();
    auto *themes = page.findChild<QListWidget *>("grubAppearanceThemes");
    auto *apply = page.findChild<QPushButton *>("grubAppearanceApply");
    auto *caption = page.findChild<QLabel *>("grubPreviewStatus");
    QTRY_VERIFY_WITH_TIMEOUT(themes->count() == 6, 10000);
    QTRY_VERIFY_WITH_TIMEOUT(apply->isEnabled(), 10000);
    QCOMPARE(requests.count(), 0); QVERIFY(caption->text().contains("Your current boot entries"));
    themes->setCurrentRow(1);
    auto *heading = page.findChild<QLineEdit *>("grubAppearanceHeading");
    QCOMPARE(heading->text(), QString("CACHYOSTOOLS"));
    heading->setText("My custom boot screen");
    page.findChild<QComboBox *>("grubAppearancePosition")->setCurrentIndex(2);
    page.findChild<QComboBox *>("grubAppearanceTextSize")->setCurrentIndex(2);
    QTRY_VERIFY_WITH_TIMEOUT(apply->isEnabled(), 10000);
    QCOMPARE(requests.count(), 0); apply->click(); QCOMPARE(requests.count(), 1);
    const auto request = qvariant_cast<QJsonObject>(requests.takeFirst().at(0));
    QCOMPARE(request["action"].toString(), QString("appearance-review"));
    QCOMPARE(request["options"].toObject()["heading"].toString(), QString("My custom boot screen"));
    QCOMPARE(request["options"].toObject()["layout"].toString(), QString("right"));
    QCOMPARE(request["options"].toObject()["size"].toInt(), 28);
    QVERIFY(request["background_sha"].toString().isEmpty());
    page.setAvailability(true, true, true); QVERIFY(!apply->isEnabled()); QVERIFY(!themes->isEnabled());
    page.setAvailability(false, true, true);
    page.acceptResult({{"active", true}, {"can_undo", true}, {"revision", "theme-saved"}});
    auto *undo = page.findChild<QPushButton *>("grubAppearanceUndo"); QVERIFY(undo->isEnabled());
    undo->click(); QCOMPARE(qvariant_cast<QJsonObject>(requests.takeFirst().at(0))["appearance_revision"].toString(), QString("theme-saved"));
    // Rapid edits discard older worker replies, and all controls remain reachable.
    heading->setText("Earlier draft"); heading->setText("Latest draft");
    for (const auto size : {QSize(960, 460), QSize(1500, 620)}) {
      page.resize(size); QTest::qWait(50);
      QCOMPARE(page.size(), size);
      QVERIFY(page.findChildren<QScrollArea *>().isEmpty());
      for (auto *bar : page.findChildren<QScrollBar *>()) QVERIFY(!bar->isVisible());
      auto *controls = page.findChild<QTabWidget *>("grubAppearanceControls");
      for (int tab=0; tab<controls->count(); ++tab) {
        controls->setCurrentIndex(tab); QTest::qWait(20);
        for (auto *widget : controls->currentWidget()->findChildren<QWidget *>()) {
          if (!widget->isVisible() || (!qobject_cast<QPushButton *>(widget) && !qobject_cast<QComboBox *>(widget) && !qobject_cast<QLineEdit *>(widget))) continue;
          QVERIFY(page.rect().contains(QRect(widget->mapTo(&page, QPoint()), widget->size())));
        }
      }
      controls->setCurrentIndex(0);
      QVERIFY(page.rect().contains(apply->geometry()));
      QVERIFY(page.rect().contains(page.findChild<QPushButton *>("grubAppearanceReload")->geometry()));
    }
    QTRY_VERIFY_WITH_TIMEOUT(apply->isEnabled(), 10000);
    if (qEnvironmentVariableIsSet("CACHY_GRUB_APPEARANCE_SCREENSHOT"))
      QVERIFY(page.grab().save(qEnvironmentVariable("CACHY_GRUB_APPEARANCE_SCREENSHOT")));
    auto *pictures = page.findChild<QComboBox *>("grubAppearancePicture");
    pictures->addItem("Missing picture", "/no-such-cachyostools-picture.png"); pictures->setCurrentIndex(pictures->count()-1);
    QTRY_VERIFY_WITH_TIMEOUT(!caption->text().contains("Updating"), 10000); QVERIFY(!apply->isEnabled());
    QCOMPARE(requests.count(), 0);
  }
  void appearanceFitsInControlPanelHost() {
    QTemporaryDir fixture;
    const auto appearance = QFINDTESTDATA("../tools/grub_appearance.py");
    QVERIFY(QFile::link(appearance, fixture.filePath("grub_appearance.py")));
    QFile helper(fixture.filePath("helper.py")); QVERIFY(helper.open(QIODevice::WriteOnly));
    helper.write("import json\nprint(json.dumps(dict(supported=True,mode='uefi',drives=[],entries=[],current_entries=[dict(name='Linux',kind='menuentry',id='linux')],revision='fixture',menu_revision='fixture',config='/boot/grub/grub.cfg')))\n");
    helper.close();
    QScrollArea host; host.setWidgetResizable(true);
    auto *page = new GrubEditor(helper.fileName()); host.setWidget(page);
    qApp->setPalette(CachyUi::palette(qApp->palette(), QColor("#27ae60"), "#1d1f24", ""));
    host.setStyleSheet(CachyUi::stylesheet()); host.setFont(QFont("sans-serif", 10));
    page->findChild<QTabWidget *>("grubTabs")->setCurrentIndex(2);
    host.resize(1650, 750); host.show();
    auto *apply = page->findChild<QPushButton *>("grubAppearanceApply");
    QTRY_VERIFY_WITH_TIMEOUT(apply->isEnabled(), 10000);
    for (const auto size : {QSize(1000, 650), QSize(1650, 750)}) {
      host.resize(size); QTest::qWait(150);
      if (host.verticalScrollBar()->isVisible()) {
        auto *appearance = page->findChild<GrubAppearance *>();
        qDebug() << "Host" << size << "page" << page->size() << page->minimumSize() << page->minimumSizeHint()
                 << "hfw" << page->hasHeightForWidth() << page->layout()->minimumHeightForWidth(page->width())
                 << "appearance" << appearance->size() << appearance->minimumSize() << appearance->minimumSizeHint();
        host.grab().save("/tmp/cachyostools-grub-host-overflow.png");
      }
      QVERIFY(!host.verticalScrollBar()->isVisible()); QVERIFY(!host.horizontalScrollBar()->isVisible());
      for (auto *bar : page->findChildren<QScrollBar *>()) QVERIFY(!bar->isVisible());
      const QRect button(apply->mapTo(host.viewport(), QPoint()), apply->size());
      QVERIFY(host.viewport()->rect().contains(button));
    }
    if (qEnvironmentVariableIsSet("CACHY_GRUB_HOST_SCREENSHOT"))
      QVERIFY(host.grab().save(qEnvironmentVariable("CACHY_GRUB_HOST_SCREENSHOT")));
  }
  void appearanceReviewFlowsThroughHelper_data() {
    QTest::addColumn<bool>("privateMenu"); QTest::addColumn<bool>("cancelOnce");
    QTest::newRow("readable menu") << false << false;
    QTest::newRow("private menu") << true << false;
    QTest::newRow("cancel authentication and retry") << true << true;
  }
  void appearanceReviewFlowsThroughHelper() {
    QFETCH(bool, privateMenu); QFETCH(bool, cancelOnce);
    QTemporaryDir fixture;
    if (privateMenu) { QFile flag(fixture.filePath("private-menu")); QVERIFY(flag.open(QIODevice::WriteOnly)); }
    if (cancelOnce) { QFile flag(fixture.filePath("cancel-next")); QVERIFY(flag.open(QIODevice::WriteOnly)); }
    const auto appearance = QFINDTESTDATA("../tools/grub_appearance.py"); QVERIFY(!appearance.isEmpty());
    QVERIFY(QFile::link(appearance, fixture.filePath("grub_appearance.py")));
    QFile helper(fixture.filePath("helper.py")); QVERIFY(helper.open(QIODevice::WriteOnly));
    helper.write(R"PY(import json,sys
from pathlib import Path
root=Path(__file__).parent
state=dict(supported=True,mode='uefi',drives=[],current_entries=[dict(id='linux',name='Linux',kind='menuentry',commands='linux /vmlinuz',editable=True)],menu_revision='original-menu',revision='original')
def emit(value): print(json.dumps(value),flush=True)
if sys.argv[1]=='status':
 emit(dict(state,current_entries=[],menu_revision='',message='Select Load current menu to authenticate.') if (root/'private-menu').exists() else state)
else:
 for line in sys.stdin:
  request=json.loads(line)
  with (root/'calls').open('a') as log: log.write(line)
  if request['action']=='appearance-load':
   if (root/'cancel-next').exists():
    (root/'cancel-next').unlink();sys.exit(126)
   emit(dict(result=dict(state,appearance=dict(active=True,can_undo=True,revision='existing-design'),message='GRUB menu loaded.')))
  elif request['action']=='appearance-review':
   review=dict(token='review-token',backup='/fixture/backups/one',files=[dict(path='/boot/grub/grub.cfg',backup='/fixture/backups/one/config',change='Replace')],instructions='Recovery Instructions: fixture',command='fixture-restore',report='fixture report')
   emit(dict(result=dict(state,appearance_review=review)))
  elif request['action']=='appearance-confirm' and request['token']=='review-token':
   (root/'confirmed').write_text('yes')
   emit(dict(result=dict(state,appearance=dict(active=True,can_undo=True,revision='saved'),message='Appearance saved for your next restart.')))
  else: emit(dict(error='Unexpected fixture request'))
)PY"); helper.close();
    GrubEditor page(helper.fileName()); page.resize(1400, 800);
    page.findChild<QTabWidget *>("grubTabs")->setCurrentIndex(2); page.show();
    auto *apply = page.findChild<QPushButton *>("grubAppearanceApply");
    QTRY_VERIFY_WITH_TIMEOUT(page.findChild<QListWidget *>("grubAppearanceThemes")->count() == 6, 10000);
    QTRY_VERIFY_WITH_TIMEOUT(apply->isEnabled(), 10000);
    auto *heading = page.findChild<QLineEdit *>("grubAppearanceHeading"); heading->setText("Keep my selected design");
    auto *caption = page.findChild<QLabel *>("grubPreviewStatus");
    if (privateMenu) {
      QCOMPARE(apply->text(), QString("Load GRUB menu…"));
      QTRY_VERIFY(caption->text().contains("authenticate"));
      auto *tabs = page.findChild<QTabWidget *>("grubTabs"); tabs->setCurrentIndex(0); tabs->setCurrentIndex(2);
      QVERIFY(page.findChild<QLabel *>("grubStatus")->text().contains("Administrator access"));
      QVERIFY(!QFile::exists(fixture.filePath("calls"))); // Switching tabs does not prompt.
      if (cancelOnce) {
        apply->click();
        QTRY_VERIFY_WITH_TIMEOUT(page.findChild<QLabel *>("grubStatus")->text().contains("cancelled"), 10000);
        QVERIFY(apply->isEnabled()); QCOMPARE(apply->text(), QString("Load GRUB menu…"));
        QCOMPARE(heading->text(), QString("Keep my selected design"));
        QVERIFY(!QFile::exists(fixture.filePath("confirmed")));
      }
      apply->click();
      QTRY_COMPARE_WITH_TIMEOUT(apply->text(), QString("Review && apply…"), 10000);
      QTRY_VERIFY_WITH_TIMEOUT(apply->isEnabled(), 10000);
      QVERIFY(page.findChild<QPushButton *>("grubAppearanceSystem")->isEnabled());
      QVERIFY(page.findChild<QPushButton *>("grubAppearanceUndo")->isEnabled());
      QCOMPARE(heading->text(), QString("Keep my selected design"));
      QVERIFY(caption->text().contains("Your current boot entries"));
      QVERIFY(!QFile::exists(fixture.filePath("confirmed")));
    }
    QTRY_VERIFY_WITH_TIMEOUT(apply->isEnabled(), 10000);
    const int loadingRequests = (privateMenu ? 1 : 0)+(cancelOnce ? 1 : 0);
    bool reviewed = false;
    QTimer confirm; confirm.setInterval(20);
    connect(&confirm, &QTimer::timeout, &page, [&] {
      auto *dialog = page.findChild<QDialog *>("grubRecoveryReview"); if (!dialog) return;
      confirm.stop(); QVERIFY(!QFile::exists(fixture.filePath("confirmed")));
      QFile calls(fixture.filePath("calls")); QVERIFY(calls.open(QIODevice::ReadOnly));
      QCOMPARE(calls.readAll().trimmed().split('\n').size(), loadingRequests+1);
      reviewed = true; dialog->findChild<QPushButton *>("grubRecoveryConfirm")->click();
    });
    confirm.start(); apply->click();
    QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(fixture.filePath("confirmed")), 10000); QVERIFY(reviewed);
    QTRY_VERIFY(page.findChild<QLabel *>("grubStatus")->text().startsWith("Appearance saved"));
    QFile calls(fixture.filePath("calls")); QVERIFY(calls.open(QIODevice::ReadOnly));
    const auto lines = calls.readAll().trimmed().split('\n'); QCOMPARE(lines.size(), loadingRequests+2);
    for (int i=0; i<loadingRequests; ++i)
      QCOMPARE(QJsonDocument::fromJson(lines[i]).object()["action"].toString(), QString("appearance-load"));
    const auto review = QJsonDocument::fromJson(lines[loadingRequests]).object();
    QCOMPARE(review["operation"].toString(), QString("apply"));
    QCOMPARE(review["options"].toObject()["heading"].toString(), QString("Keep my selected design"));
    QCOMPARE(QJsonDocument::fromJson(lines.last()).object()["action"].toString(), QString("appearance-confirm"));
  }
  void recoveryReviewNeedsExplicitConfirmation() {
    GrubAppearance page("/no-helper-needed");
    QSignalSpy requests(&page, &GrubAppearance::requested);
    const QString backup = "/var/lib/cachyostools/grub-appearance/backups/20261104T200000-12345678";
    const QString command = "sudo sh -- "+backup+"/restore.sh";
    const QJsonObject review{{"token", "single-use-review"}, {"backup", backup},
      {"command", command}, {"report", "Take a picture of the screen. Here are your backups.\nRecovery Instructions: fixture"},
      {"instructions", "Recovery Instructions:\nIf Linux starts, open a terminal and run:\n"+command+"\n\nIf Linux cannot start, boot a Linux live USB. Mount Linux at /mnt and any separate /boot at /mnt/boot.\nThen run: sudo sh -- /mnt"+backup+"/restore.sh /mnt"},
      {"files", QJsonArray{
        QJsonObject{{"path", "/boot/grub/grub.cfg"}, {"backup", backup+"/config"}, {"change", "Replace"}},
        QJsonObject{{"path", "/etc/grub.d/99_cachyostools_appearance"}, {"backup", ""}, {"change", "Create"}},
        QJsonObject{{"path", "/var/lib/cachyostools/grub-appearance/entries.json"}, {"backup", ""}, {"change", "Create"}},
        QJsonObject{{"path", "/var/lib/cachyostools/grub-appearance/last.json"}, {"backup", backup+"/undo-history.json"}, {"change", "Replace"}}}},
      {"created", QJsonArray{"/boot/grub/themes/cachyostools-editor/design-fixture/background.png", "/boot/grub/themes/cachyostools-editor/design-fixture/theme.txt"}}};
    bool inspected = false;
    QTimer::singleShot(0, &page, [&] {
      auto *dialog = page.findChild<QDialog *>("grubRecoveryReview"); QVERIFY(dialog);
      QCOMPARE(requests.count(), 0);
      QCOMPARE(dialog->findChild<QLabel *>("grubRecoveryBackupFolder")->text(), backup);
      QCOMPARE(dialog->findChild<QLabel *>("grubRecoveryCommand")->text(), command);
      QVERIFY(dialog->findChild<QLabel *>("grubRecoveryInstructions")->text().contains("/mnt/boot"));
      QVERIFY(dialog->findChildren<QScrollArea *>().isEmpty());
      if (qEnvironmentVariableIsSet("CACHY_GRUB_RECOVERY_SCREENSHOT"))
        QVERIFY(dialog->grab().save(qEnvironmentVariable("CACHY_GRUB_RECOVERY_SCREENSHOT")));
      inspected = true; dialog->reject();
    });
    page.reviewChanges(review); QVERIFY(inspected); QCOMPARE(requests.count(), 1);
    QCOMPARE(qvariant_cast<QJsonObject>(requests.takeFirst().at(0))["action"].toString(), QString("appearance-cancel"));
    QTimer::singleShot(0, &page, [&] {
      auto *dialog = page.findChild<QDialog *>("grubRecoveryReview"); QVERIFY(dialog);
      QCOMPARE(requests.count(), 0);
      dialog->findChild<QPushButton *>("grubRecoveryConfirm")->click();
    });
    page.reviewChanges(review); QCOMPARE(requests.count(), 1);
    const auto confirmed = qvariant_cast<QJsonObject>(requests.takeFirst().at(0));
    QCOMPARE(confirmed["action"].toString(), QString("appearance-confirm"));
    QCOMPARE(confirmed["token"].toString(), QString("single-use-review"));
  }
  void driveToBootEntry() {
    QTemporaryDir fixture;
    QFile helper(fixture.filePath("helper.py"));
    QVERIFY(helper.open(QIODevice::WriteOnly));
    helper.write(R"PY(import json,sys,time
from pathlib import Path
root=Path(__file__).parent
drive=dict(path='/dev/fixture',token='disk-token',size=1000000000000,model='Test SSD',serial='fixture-serial',partitions=1,mountpoints=[],disk_id='nvme-Fixture',volumes=[dict(path='/dev/fixture1',size=999000000000,type='part',fstype='ext4',label='Other Linux',uuid='ABCD-1234',mountpoints=[])])
current=dict(path='/dev/current',token='current-token',size=1000000000000,model='System SSD',mountpoints=['/'],current_system=True)
extra=[dict(path='/dev/extra'+str(i),token='extra-'+str(i),size=1000000000000,model='Another SSD',mountpoints=[]) for i in range(6)]
native=dict(id='current-native',name='Existing Linux',kind='menuentry',source='/etc/grub.d/10_linux',editable=True,managed_id='',commands='linux /vmlinuz root=UUID=fixture')
state=dict(supported=True,mode='uefi',secure_boot=False,drives=[drive,current,*extra],entries=[],current_entries=[native],revision='original',menu_revision='menu-original',can_undo=False,config='/boot/grub/grub.cfg')
def emit(v): print(json.dumps(v),flush=True)
if sys.argv[1]=='status': emit(state)
else:
    for line in sys.stdin:
        request=json.loads(line)
        with (root/'calls').open('a') as log: log.write(line)
        emit(dict(progress='Inspecting test drive'))
        time.sleep(.1)
        if request['action']=='scan':
            systems=[dict(id='windows',title='Windows Boot Manager',available=True,drive='/dev/fixture',drive_token='disk-token',loader_hash='loader-verified',loader='/EFI/Microsoft/Boot/bootmgfw.efi',kind='efi',partition='/dev/fixture1'),
                     dict(id='linux',title='Linux test system',available=True,drive='/dev/fixture',drive_token='disk-token',loader_hash='linux-verified',loader='/boot/grub/grub.cfg',kind='config',partition='/dev/fixture2')]
            emit(dict(result=dict(state,candidates=systems,notes=[])))
        elif request['action']=='add':
            state.update(entries=[dict(id=request['candidate'],name=request['name'],title='Linux test system',drive='/dev/fixture')],revision='saved',can_undo=True,message='Saved. Your boot menu is ready for the next restart.')
            state['current_entries']=[native,dict(id='current-added',name=request['name'],kind='menuentry',managed_id=request['candidate'],editable=True,commands='configfile /boot/grub/grub.cfg')]
            emit(dict(result=state))
        else: emit(dict(error='Fixture error kept your boot menu unchanged.'))
)PY");
    helper.close();
    GrubEditor page(helper.fileName()); page.resize(1000, 850); page.show();
    auto *drives = page.findChild<QTreeWidget *>("grubDrives");
    auto *systems = page.findChild<QListWidget *>("grubSystems");
    auto *name = page.findChild<QLineEdit *>("grubName");
    auto *add = page.findChild<QPushButton *>("grubAdd");
    auto *entries = page.findChild<QTreeWidget *>("grubEntries");
    auto *tabs = page.findChild<QTabWidget *>("grubTabs");
    auto *status = page.findChild<QLabel *>("grubStatus");
    QTRY_COMPARE(drives->topLevelItemCount(), 8);
    QCOMPARE(tabs->count(), 3); QCOMPARE(tabs->tabText(0), QString("Current Grub"));
    QCOMPARE(tabs->tabText(2), QString("Appearance"));
    QCOMPARE(tabs->tabText(1), QString("Add Entry")); QCOMPARE(tabs->currentIndex(), 0);
    QCOMPARE(entries->topLevelItemCount(), 1); QCOMPARE(entries->topLevelItem(0)->text(0), QString("Existing Linux"));
    tabs->setCurrentIndex(1);
    // Theme metrics can arrive after drive detection. The list and its actions
    // must still fit their group at both supported layout widths.
    page.setStyleSheet("QGroupBox { padding:8px; margin-top:16px; border:1px solid gray; } "
                      "QHeaderView::section { padding:10px; }");
    auto *properties = page.findChild<QPushButton *>("grubProperties");
    for (int width : {1650, 1000}) {
      page.resize(width, 768);
      QTRY_VERIFY(drives->geometry().bottom() < properties->geometry().top());
      QTRY_VERIFY(!drives->parentWidget()->geometry().intersects(systems->parentWidget()->geometry()));
      QTRY_VERIFY(drives->parentWidget()->rect().contains(properties->geometry()));
      QTRY_VERIFY(drives->parentWidget()->mapTo(&page, drives->parentWidget()->rect().bottomLeft()).y() < status->geometry().top());
    }
    QCOMPARE(drives->topLevelItem(0)->text(3), QString("Not mounted"));
    QCOMPARE(drives->topLevelItem(1)->text(3), QString("Current system"));
    QVERIFY(!QFile::exists(fixture.filePath("calls"))); // Browsing never authenticates or writes.
    QVERIFY(!add->isEnabled());
    drives->setCurrentItem(drives->topLevelItem(0));
    bool sawProperties = false;
    QString filesystem, volumeLabel, diskId, uuid;
    auto inspectProperties = [&] {
      auto *dialog = page.findChild<QDialog *>("grubDriveProperties");
      if (!dialog) return;
      auto *volumes = dialog->findChild<QTreeWidget *>("grubVolumeDetails");
      sawProperties = volumes && volumes->topLevelItemCount() == 1;
      if (sawProperties) {
        auto *row = volumes->topLevelItem(0);
        filesystem = row->text(4); volumeLabel = row->text(5); diskId = row->text(8); uuid = row->text(9);
      }
      if (qEnvironmentVariableIsSet("CACHY_GRUB_PROPERTIES_SCREENSHOT"))
        dialog->grab().save(qEnvironmentVariable("CACHY_GRUB_PROPERTIES_SCREENSHOT"));
      dialog->reject();
    };
    QTimer::singleShot(0, &page, [&] {
      auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
      QVERIFY(menu);
      QTimer::singleShot(0, &page, inspectProperties);
      menu->setActiveAction(menu->actions().first());
      QTest::keyClick(menu, Qt::Key_Return);
    });
    const auto position = drives->visualItemRect(drives->topLevelItem(0)).center();
    QContextMenuEvent context(QContextMenuEvent::Mouse, position, drives->viewport()->mapToGlobal(position));
    QApplication::sendEvent(drives->viewport(), &context);
    QVERIFY(sawProperties); QCOMPARE(filesystem, QString("ext4")); QCOMPARE(volumeLabel, QString("Other Linux"));
    QCOMPARE(diskId, QString("nvme-Fixture")); QCOMPARE(uuid, QString("ABCD-1234"));
    QVERIFY(!QFile::exists(fixture.filePath("calls"))); // Properties require no elevated helper.
    page.findChild<QPushButton *>("grubDetect")->click();
    QTRY_COMPARE(systems->count(), 2);
    QTRY_VERIFY(add->isEnabled());
    QCOMPARE(name->text(), QString("Windows Boot Manager"));
    systems->setCurrentRow(1); QCOMPARE(name->text(), QString("Linux test system"));
    name->setText("My Linux drive"); add->click();
    QVERIFY(!add->isEnabled()); QVERIFY(!drives->isEnabled());
    QTRY_COMPARE(entries->topLevelItemCount(), 2);
    QCOMPARE(entries->topLevelItem(1)->text(0), QString("My Linux drive"));
    QCOMPARE(tabs->currentIndex(), 0);
    QTRY_VERIFY(drives->isEnabled());
    QVERIFY(status->text().startsWith("Saved."));
    QFile calls(fixture.filePath("calls")); QVERIFY(calls.open(QIODevice::ReadOnly));
    const auto lines = calls.readAll().trimmed().split('\n'); QCOMPARE(lines.size(), 2);
    const auto request = QJsonDocument::fromJson(lines[1]).object();
    QCOMPARE(request["action"].toString(), QString("add"));
    QCOMPARE(request["candidate"].toString(), QString("linux"));
    QCOMPARE(request["name"].toString(), QString("My Linux drive"));
    QCOMPARE(request["revision"].toString(), QString("original"));
    QVERIFY(!request.contains("partition")); QVERIFY(!request.contains("command"));
    if (qEnvironmentVariableIsSet("CACHY_GRUB_TEST_SCREENSHOT"))
      QVERIFY(page.grab().save(qEnvironmentVariable("CACHY_GRUB_TEST_SCREENSHOT")));
    auto *undo = page.findChild<QPushButton *>("grubUndo"); QVERIFY(undo->isEnabled());
    page.findChild<QPushButton *>("grubLoad")->click();
    QTRY_VERIFY(status->text().contains("Fixture error"));
    QVERIFY(drives->isEnabled()); QCOMPARE(entries->topLevelItemCount(), 2);
  }
  void editAndRemoveCurrentEntries() {
    QTemporaryDir fixture;
    QFile helper(fixture.filePath("helper.py")); QVERIFY(helper.open(QIODevice::WriteOnly));
    helper.write(R"PY(import json,sys
from pathlib import Path
root=Path(__file__).parent
native=dict(id='native',name='Installed Linux',kind='menuentry',source='/etc/grub.d/10_linux',editable=True,managed_id='',commands='linux /vmlinuz root=UUID=fixture')
firmware=dict(id='firmware',name='Firmware',kind='menuentry',source='/etc/grub.d/30_uefi-firmware',editable=True,managed_id='',commands='fwsetup')
state=dict(supported=True,drives=[],entries=[],current_entries=[native,firmware],revision='',menu_revision='public-menu',config='/boot/grub/grub.cfg')
def emit(value): print(json.dumps(value),flush=True)
if sys.argv[1]=='status': emit(state)
else:
    for line in sys.stdin:
        request=json.loads(line)
        with (root/'calls').open('a') as stream: stream.write(line)
        if request['action']=='edit-current':
            native.update(name=request['name'],commands=request.get('commands',native['commands']))
        elif request['action']=='remove-current':
            state['current_entries']=[entry for entry in state['current_entries'] if entry['id']!=request['id']]
        state.update(revision='authenticated',menu_revision='changed-menu',can_undo=True,message='Saved.')
        emit(dict(result=state))
)PY");
    helper.close();
    GrubEditor page(helper.fileName()); page.resize(1100, 760); page.show();
    auto *entries = page.findChild<QTreeWidget *>("grubEntries");
    auto *edit = page.findChild<QPushButton *>("grubEdit");
    QTRY_COMPARE(entries->topLevelItemCount(), 2); QVERIFY(edit->isEnabled());
    QVERIFY(!QFile::exists(fixture.filePath("calls")));
    QTimer::singleShot(0, &page, [&] { page.findChild<QDialog *>("grubEditDialog")->reject(); });
    edit->click(); QVERIFY(!QFile::exists(fixture.filePath("calls")));
    QTimer::singleShot(0, &page, [&] {
      auto *dialog = page.findChild<QDialog *>("grubEditDialog"); QVERIFY(dialog);
      dialog->findChild<QLineEdit *>("grubEditName")->setText("My main Linux");
      dialog->findChild<QGroupBox *>("grubAdvanced")->setChecked(true);
      dialog->findChild<QPlainTextEdit *>("grubCommands")->setPlainText("linux /vmlinuz root=UUID=fixture quiet");
      dialog->accept();
    });
    edit->click();
    QTRY_COMPARE(entries->topLevelItem(0)->text(0), QString("My main Linux"));
    QFile calls(fixture.filePath("calls")); QVERIFY(calls.open(QIODevice::ReadOnly));
    const auto request = QJsonDocument::fromJson(calls.readLine()).object(); calls.close();
    QCOMPARE(request["action"].toString(), QString("edit-current"));
    QCOMPARE(request["menu_revision"].toString(), QString("public-menu"));
    QCOMPARE(request["commands"].toString(), QString("linux /vmlinuz root=UUID=fixture quiet"));
    entries->setCurrentItem(entries->topLevelItem(1));
    QTimer::singleShot(0, &page, [&] {
      auto *question = qobject_cast<QMessageBox *>(QApplication::activeModalWidget()); QVERIFY(question);
      question->button(QMessageBox::Yes)->click();
    });
    page.findChild<QPushButton *>("grubRemove")->click();
    QTRY_COMPARE(entries->topLevelItemCount(), 1);
    QCOMPARE(entries->topLevelItem(0)->text(0), QString("My main Linux"));
  }
  void cancelledAuthentication() {
    QTemporaryDir fixture;
    QFile helper(fixture.filePath("helper.py")); QVERIFY(helper.open(QIODevice::WriteOnly));
    helper.write("import sys,json\nif sys.argv[1]=='status': print(json.dumps(dict(supported=True,drives=[dict(path='/dev/fake',model='Fixture',token='x',size=1000000000)],entries=[])))\nelse: sys.exit(126)\n");
    helper.close();
    GrubEditor page(helper.fileName()); page.show();
    page.findChild<QTabWidget *>("grubTabs")->setCurrentIndex(1);
    auto *drives = page.findChild<QTreeWidget *>("grubDrives"); QTRY_COMPARE(drives->topLevelItemCount(), 1);
    drives->setCurrentItem(drives->topLevelItem(0));
    page.findChild<QPushButton *>("grubDetect")->click();
    QTRY_VERIFY(page.findChild<QLabel *>("grubStatus")->text().contains("cancelled"));
    QVERIFY(!page.findChild<QPushButton *>("grubAdd")->isEnabled()); QVERIFY(drives->isEnabled());
    QVERIFY(page.findChild<QPushButton *>("grubDetect")->isEnabled());
  }
};
QTEST_MAIN(GrubEditorTests)
#include "grub_editor_tests.moc"
