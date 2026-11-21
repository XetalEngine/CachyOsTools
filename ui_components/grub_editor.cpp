#include "grub_editor.h"
#include "grub_appearance.h"
#include "panel_widgets.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QtWidgets>

namespace {
QLabel *label(const QString &text, QWidget *parent, const char *name = "") {
  auto *value = new QLabel(text, parent);
  value->setTextFormat(Qt::PlainText);
  value->setWordWrap(true);
  value->setObjectName(name);
  return value;
}
QString capacity(double size) {
  return QLocale().formattedDataSize(qint64(size), 1, QLocale::DataSizeIecFormat);
}
QString mountPoints(const QJsonValue &value) {
  QStringList points;
  for (const auto &point : value.toArray()) if (!point.toString().isEmpty()) points << point.toString();
  return points.join(", ");
}
QString driveStatus(const QJsonObject &drive) {
  if (drive["current_system"].toBool()) return "Current system";
  return mountPoints(drive["mountpoints"]).isEmpty() ? "Not mounted" : "Mounted";
}
QJsonObject itemData(QListWidget *list) {
  return list->currentItem() ? list->currentItem()->data(Qt::UserRole).toJsonObject() : QJsonObject();
}
}

GrubEditor::GrubEditor(const QString &helper, QWidget *parent) : QWidget(parent), helper(helper) {
  setObjectName("grubEditorTab");
  auto *layout = new QVBoxLayout(this);
  layout->setSizeConstraint(QLayout::SetMinimumSize);
  layout->setSpacing(14);
  auto *heading = new QHBoxLayout;
  heading->addWidget(label("GRUB Boot Editor", this, "panelTitle"), 1);
  mode = label("", this, "muted"); heading->addWidget(mode);
  layout->addLayout(heading);
  tabs = new PanelTabs(this); tabs->setObjectName("grubTabs");
  auto *currentPage = new QWidget(tabs);
  auto *currentLayout = new QVBoxLayout(currentPage); currentLayout->setSpacing(14);
  auto *addPage = new QWidget(tabs);
  auto *addLayout = new QVBoxLayout(addPage); addLayout->setSpacing(14);
  tabs->addTab(currentPage, "Current Grub"); tabs->addTab(addPage, "Add Entry");
  appearance = new GrubAppearance(QFileInfo(helper).dir().filePath("grub_appearance.py"), tabs);
  tabs->addTab(appearance, "Appearance");
  connect(appearance, &GrubAppearance::requested, this, &GrubEditor::request);
  layout->addWidget(tabs, 1);
  currentLayout->addWidget(label("Your current boot menu. Select an entry to edit its name or boot commands, or remove it from the menu.", currentPage));
  addLayout->addWidget(label("Choose a drive, detect its bootable systems, and add the one you want to your current GRUB menu.", addPage));

  choicesLayout = new QBoxLayout(QBoxLayout::TopToBottom);
  choicesLayout->setContentsMargins(0, 0, 0, 0); choicesLayout->setSpacing(14);
  addLayout->addLayout(choicesLayout);
  auto *driveBox = new QGroupBox("1  Choose a drive", addPage);
  auto *driveLayout = new QVBoxLayout(driveBox);
  driveLayout->setSizeConstraint(QLayout::SetMinimumSize);
  auto *driveRow = new QHBoxLayout;
  driveCount = label("All drives · mounted and unmounted", driveBox, "grubDriveCount");
  driveCount->setWordWrap(false);
  driveRow->addWidget(driveCount, 1);
  refresh = new QPushButton("Refresh drives", driveBox); refresh->setObjectName("grubRefresh");
  refresh->setIcon(QIcon::fromTheme("view-refresh")); driveRow->addWidget(refresh);
  driveLayout->addLayout(driveRow);
  drives = new QTreeWidget(driveBox); drives->setObjectName("grubDrives");
  drives->setHeaderLabels({"Drive", "Device", "Capacity", "Status"});
  drives->setRootIsDecorated(false); drives->setUniformRowHeights(true);
  drives->setAlternatingRowColors(true); drives->setSelectionMode(QAbstractItemView::SingleSelection);
  drives->setSelectionBehavior(QAbstractItemView::SelectRows);
  drives->setEditTriggers(QAbstractItemView::NoEditTriggers);
  drives->setMinimumHeight(140); drives->setMaximumHeight(270);
  drives->header()->setSectionResizeMode(0, QHeaderView::Stretch);
  for (int column = 1; column < 4; ++column) drives->header()->setSectionResizeMode(column, QHeaderView::ResizeToContents);
  drives->setContextMenuPolicy(Qt::CustomContextMenu);
  driveLayout->addWidget(drives);
  auto *driveActions = new QHBoxLayout;
  properties = new QPushButton("Properties…", driveBox); properties->setObjectName("grubProperties");
  properties->setIcon(QIcon::fromTheme("document-properties")); driveActions->addWidget(properties);
  driveActions->addStretch(1);
  detect = new QPushButton("Detect bootable systems", driveBox); detect->setObjectName("grubDetect");
  detect->setIcon(QIcon::fromTheme("system-search")); driveActions->addWidget(detect);
  driveLayout->addLayout(driveActions);
  choicesLayout->addWidget(driveBox, 1);

  auto *systemBox = new QGroupBox("2  Boot partition to add", addPage);
  auto *systemLayout = new QVBoxLayout(systemBox);
  systemLayout->setSizeConstraint(QLayout::SetMinimumSize);
  systems = new QListWidget(systemBox); systems->setObjectName("grubSystems");
  systems->setMinimumHeight(78); systems->setMaximumHeight(150);
  systemLayout->addWidget(systems);
  selection = label("Select a drive above to find Windows or Linux.", systemBox, "grubSelection");
  systemLayout->addWidget(selection);
  notes = label("", systemBox, "muted"); notes->hide(); systemLayout->addWidget(notes);
  auto *nameRow = new QHBoxLayout;
  nameRow->addWidget(label("Name in GRUB", systemBox));
  name = new QLineEdit(systemBox); name->setObjectName("grubName"); name->setMaxLength(100);
  name->setPlaceholderText("For example: Windows 11 or My Linux");
  nameRow->addWidget(name, 1);
  add = new QPushButton("Add to GRUB", systemBox); add->setObjectName("grubAdd");
  add->setProperty("primary", true); add->setIcon(QIcon::fromTheme("list-add"));
  nameRow->addWidget(add);
  systemLayout->addLayout(nameRow);
  systemLayout->addStretch(1);
  choicesLayout->addWidget(systemBox, 1);
  addLayout->addStretch(1);

  progress = new QProgressBar(this); progress->setRange(0, 0);
  progress->setTextVisible(false); progress->setFixedHeight(6); progress->hide(); layout->addWidget(progress);
  status = label("Reading your current boot menu…", this, "grubStatus");
  status->setTextInteractionFlags(Qt::TextSelectableByMouse); layout->addWidget(status);

  auto *savedBox = new QGroupBox("Boot menu entries", currentPage);
  auto *savedLayout = new QVBoxLayout(savedBox);
  entries = new QTreeWidget(savedBox); entries->setObjectName("grubEntries");
  entries->setHeaderLabels({"Name in GRUB", "Type", "Source"});
  entries->setAlternatingRowColors(true); entries->setSelectionMode(QAbstractItemView::SingleSelection);
  entries->setSelectionBehavior(QAbstractItemView::SelectRows);
  entries->setEditTriggers(QAbstractItemView::NoEditTriggers);
  entries->header()->setSectionResizeMode(0, QHeaderView::Stretch);
  entries->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  entries->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  entries->setMinimumHeight(70); savedLayout->addWidget(entries, 1);
  entryDetails = label("Select a boot entry to see its details.", savedBox, "grubEntryDetails");
  savedLayout->addWidget(entryDetails);
  saved = label("Reading the current menu…", savedBox, "grubSaved");
  saved->setWordWrap(false); savedLayout->addWidget(saved);
  auto *savedActions = new QHBoxLayout;
  load = new QPushButton("Load current menu", savedBox); load->setObjectName("grubLoad");
  load->setIcon(QIcon::fromTheme("view-refresh"));
  rename = new QPushButton("Edit entry…", savedBox); rename->setObjectName("grubEdit");
  rename->setIcon(QIcon::fromTheme("document-edit"));
  remove = new QPushButton("Remove from menu…", savedBox); remove->setObjectName("grubRemove");
  undo = new QPushButton("Undo last change", savedBox); undo->setObjectName("grubUndo");
  savedActions->addWidget(load); savedActions->addWidget(rename); savedActions->addWidget(remove);
  savedActions->addStretch(); savedActions->addWidget(undo);
  savedLayout->addLayout(savedActions); currentLayout->addWidget(savedBox, 1);
  auto *footnote = label("Each change is backed up and checked before saving. Your GRUB theme is kept.", this, "muted");
  layout->addWidget(footnote);
  connect(tabs, &QTabWidget::currentChanged, footnote, [this, footnote](int index) {
    footnote->setText(index == 2 ? "Appearance changes are backed up. Boot entries, kernel commands, default system and timeout are kept." :
        "Each change is backed up and checked before saving. Your GRUB theme is kept.");
    if (index == 2 && !busy && supported) status->setText(menuRevision.isEmpty() ?
      "Administrator access is needed to read this GRUB menu. Choose Load GRUB menu to continue." :
      "Choose a design. Review & apply creates your backups before confirmation.");
  });

  reader = new QProcess(this); worker = new QProcess(this);
  connect(refresh, &QPushButton::clicked, this, &GrubEditor::refreshDrives);
  connect(drives, &QTreeWidget::currentItemChanged, this, &GrubEditor::chooseDrive);
  connect(drives, &QTreeWidget::itemActivated, this, &GrubEditor::scanDrive);
  connect(detect, &QPushButton::clicked, this, &GrubEditor::scanDrive);
  connect(properties, &QPushButton::clicked, this, [this] { showDriveProperties(drive()); });
  connect(drives, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint &point) {
    auto *row = drives->itemAt(point); if (!row) return;
    const auto metadata = row->data(0, Qt::UserRole).toJsonObject();
    QMenu menu(this);
    auto *action = menu.addAction(QIcon::fromTheme("document-properties"), "Properties…");
    if (menu.exec(drives->viewport()->mapToGlobal(point)) == action) showDriveProperties(metadata);
  });
  connect(systems, &QListWidget::currentRowChanged, this, &GrubEditor::chooseSystem);
  connect(entries, &QTreeWidget::currentItemChanged, this, &GrubEditor::chooseEntry);
  connect(entries, &QTreeWidget::itemActivated, this, &GrubEditor::editEntry);
  connect(name, &QLineEdit::textChanged, this, &GrubEditor::updateActions);
  connect(load, &QPushButton::clicked, this, [this] {
    if (menuRevision.isEmpty()) request({{"action", "refresh"}});
    else refreshDrives();
  });
  connect(add, &QPushButton::clicked, this, [this] {
    const auto target = system(); if (!target.value("available").toBool()) return;
    request({{"action", "add"}, {"drive", target["drive"]}, {"drive_token", target["drive_token"]},
             {"candidate", target["id"]}, {"loader_hash", target["loader_hash"]}, {"name", name->text().trimmed()}});
  });
  connect(rename, &QPushButton::clicked, this, &GrubEditor::editEntry);
  connect(remove, &QPushButton::clicked, this, [this] {
    const auto item = entry(); if (item.isEmpty()) return;
    QString message = "Remove “"+item["name"].toString()+"” from the GRUB menu?\nThe system and its drive stay intact.";
    if (item["kind"] == "submenu") message += "\nThe entries inside this submenu will also be removed.";
    if (item["managed_id"].toString().isEmpty()) message += "\nRegenerating GRUB can restore entries created by system generators.";
    if (QMessageBox::question(this, "Remove boot entry?", message, QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel) == QMessageBox::Yes)
      request({{"action", "remove-current"}, {"id", item["id"]}});
  });
  connect(undo, &QPushButton::clicked, this, [this] {
    if (QMessageBox::question(this, "Undo last GRUB change?", "Restore the boot menu from before your last saved change?",
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Yes)
      request({{"action", "undo"}});
  });
  connect(reader, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, [this](int code, QProcess::ExitStatus) {
    const auto data = QJsonDocument::fromJson(reader->readAllStandardOutput()).object();
    if (!code && data.contains("drives")) result(data);
    else status->setText(data.value("error").toString("Could not read drives. Try Refresh drives."));
    setBusy(false);
  });
  connect(reader, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
    if (error == QProcess::FailedToStart) { status->setText("Could not start drive detection."); setBusy(false); }
  });
  connect(worker, &QProcess::started, this, [this] {
    worker->write(QJsonDocument(pending).toJson(QJsonDocument::Compact)+'\n'); pending = {};
  });
  connect(worker, &QProcess::readyReadStandardOutput, this, &GrubEditor::readWorker);
  connect(worker, &QProcess::readyReadStandardError, this, [this] { errors += worker->readAllStandardError(); errors = errors.right(4096); });
  connect(worker, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, [this](int code, QProcess::ExitStatus) {
    readWorker();
    if (busy) {
      status->setText(code == 126 ? "Administrator authentication was cancelled. Nothing was changed." :
                       "The GRUB helper stopped. "+QString::fromUtf8(errors).trimmed());
      setBusy(false);
    }
  });
  connect(worker, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
    if (error == QProcess::FailedToStart) { status->setText("Could not open administrator authentication."); setBusy(false); }
  });
  updateActions();
}

GrubEditor::~GrubEditor() {
  worker->closeWriteChannel();
  if (!busy && worker->state() != QProcess::NotRunning) worker->waitForFinished(1000);
  if (reader->state() != QProcess::NotRunning) { reader->kill(); reader->waitForFinished(1000); }
}
int GrubEditor::heightForWidth(int width) const {
  // The outer Control Panel scroll area must be allowed to shrink the preview.
  // Its preferred height must not force a scrollbar before compact mode runs.
  if (tabs->currentIndex() == 2) return layout()->minimumHeightForWidth(width);
  return QWidget::heightForWidth(width);
}
void GrubEditor::showEvent(QShowEvent *event) {
  QWidget::showEvent(event);
  if (!loaded) { loaded = true; refreshDrives(); }
}
void GrubEditor::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);
  choicesLayout->setDirection(width() >= 1100 ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom);
}
QJsonObject GrubEditor::system() const { return itemData(systems); }
QJsonObject GrubEditor::entry() const {
  return entries->currentItem() ? entries->currentItem()->data(0, Qt::UserRole).toJsonObject() : QJsonObject();
}
QJsonObject GrubEditor::drive() const {
  return drives->currentItem() ? drives->currentItem()->data(0, Qt::UserRole).toJsonObject() : QJsonObject();
}
void GrubEditor::setBusy(bool value) { busy = value; progress->setVisible(value); updateActions(); }
void GrubEditor::updateActions() {
  appearance->setAvailability(busy, supported, !menuRevision.isEmpty());
  drives->setEnabled(!busy); refresh->setEnabled(!busy);
  detect->setEnabled(!busy && supported && !drive().isEmpty());
  properties->setEnabled(!drive().isEmpty());
  systems->setEnabled(!busy); name->setEnabled(!busy && system().value("available").toBool());
  add->setEnabled(!busy && supported && system().value("available").toBool() && !name->text().trimmed().isEmpty() && !revision.isEmpty());
  entries->setEnabled(!busy);
  load->setEnabled(!busy && supported); rename->setEnabled(!busy && entry()["editable"].toBool());
  remove->setEnabled(!busy && entry()["editable"].toBool()); undo->setEnabled(!busy && canUndo);
  bool existing = false;
  for (const auto &item : managedEntries)
    if (item.toObject()["id"] == system()["id"]) existing = true;
  add->setText(existing ? "Save name" : "Add to GRUB");
}
void GrubEditor::refreshDrives() {
  if (busy) return;
  systems->clear(); name->clear(); notes->hide();
  operation = "refresh"; status->setText("Finding drives…"); setBusy(true);
  if (worker->state() == QProcess::Running) {
    worker->write(QJsonDocument(QJsonObject{{"action", "refresh"}}).toJson(QJsonDocument::Compact)+'\n');
  } else reader->start("python3", {helper, "status"});
}
void GrubEditor::chooseDrive() {
  if (busy) return;
  systems->clear(); name->clear(); notes->hide();
  selection->setText(drive().isEmpty() ? "Select a drive above to find Windows or Linux." :
      "Select Detect bootable systems to inspect this drive’s partitions.");
  updateActions();
}
void GrubEditor::scanDrive() {
  if (busy || !supported || drive().isEmpty()) return;
  const auto selected = drive();
  systems->clear(); name->clear(); notes->hide();
  selection->setText("Finding bootable systems on this drive…");
  request({{"action", "scan"}, {"drive", selected["path"]}, {"drive_token", selected["token"]}});
}
void GrubEditor::showDriveProperties(const QJsonObject &metadata) {
  if (metadata.isEmpty()) return;
  QDialog dialog(this); dialog.setObjectName("grubDriveProperties");
  dialog.setProperty("xetalWindowSized", true);
  dialog.setWindowTitle("Drive properties · "+metadata["model"].toString());
  auto *layout = new QVBoxLayout(&dialog); layout->setSpacing(14);
  auto *hero = new QFrame(&dialog); hero->setObjectName("defaultsHero");
  auto *heroLayout = new QHBoxLayout(hero); heroLayout->setContentsMargins(20, 16, 20, 16);
  auto *icon = new QLabel(hero);
  icon->setPixmap(QIcon::fromTheme("drive-harddisk").pixmap(56, 56)); heroLayout->addWidget(icon);
  auto *identity = new QVBoxLayout;
  identity->addWidget(label(metadata["model"].toString(), hero, "panelTitle"));
  identity->addWidget(label(metadata["path"].toString()+" · "+driveStatus(metadata)+" · "+
      QString::number(metadata["partitions"].toInt())+" partitions", hero, "muted"));
  heroLayout->addLayout(identity, 1);
  heroLayout->addWidget(label(capacity(metadata["size"].toDouble()), hero, "panelTitle"));
  layout->addWidget(hero);
  auto *details = new QGridLayout;
  const QList<QPair<QString, QString>> fields = {
      {"Device path", metadata["path"].toString()}, {"Connection", metadata["connection"].toString().toUpper()},
      {"Disk ID", metadata["disk_id"].toString()}, {"Partition table", metadata["partition_table"].toString().toUpper()},
      {"Serial number", metadata["serial"].toString()}, {"WWN", metadata["wwn"].toString()},
      {"Mount points", mountPoints(metadata["mountpoints"])}, {"Access", metadata["read_only"].toBool() ? "Read-only" : "Read/write"},
      {"Capacity in bytes", QLocale().toString(qint64(metadata["size"].toDouble()))},
      {"Removable", metadata["removable"].toBool() ? "Yes" : "No"}};
  for (int i = 0; i < fields.size(); ++i) {
    const int row = i/2, column = (i%2)*2;
    details->addWidget(label(fields[i].first, &dialog, "muted"), row, column);
    auto *value = label(fields[i].second.isEmpty() ? "—" : fields[i].second, &dialog);
    value->setTextInteractionFlags(Qt::TextSelectableByMouse); details->addWidget(value, row, column+1);
  }
  details->setColumnStretch(1, 1); details->setColumnStretch(3, 1);
  details->setHorizontalSpacing(18); details->setVerticalSpacing(10); layout->addLayout(details);
  layout->addWidget(label("Partitions & volumes", &dialog, "panelTitle"));
  auto *volumes = new QTreeWidget(&dialog); volumes->setObjectName("grubVolumeDetails");
  volumes->setHeaderLabels({"Device", "Device path", "Size", "Type", "File system", "Label", "Mount point", "Status", "Disk ID", "UUID", "Partition UUID", "Partition type"});
  volumes->setRootIsDecorated(false); volumes->setAlternatingRowColors(true);
  volumes->setSelectionBehavior(QAbstractItemView::SelectRows);
  volumes->setEditTriggers(QAbstractItemView::NoEditTriggers);
  volumes->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  for (const auto &value : metadata["volumes"].toArray()) {
    const auto volume = value.toObject();
    const auto mount = mountPoints(volume["mountpoints"]);
    const QStringList values = {QFileInfo(volume["path"].toString()).fileName(), volume["path"].toString(),
        capacity(volume["size"].toDouble()), volume["type"].toString(), volume["fstype"].toString(),
        volume["label"].toString(), mount, mount.isEmpty() ? "Not mounted" : "Mounted", metadata["disk_id"].toString(),
        volume["uuid"].toString(), volume["partuuid"].toString(), volume["parttype"].toString()};
    auto *row = new QTreeWidgetItem(volumes);
    row->setIcon(0, QIcon::fromTheme("drive-harddisk"));
    for (int i = 0; i < values.size(); ++i) {
      row->setText(i, values[i].isEmpty() ? "—" : values[i]); row->setToolTip(i, values[i]);
    }
  }
  volumes->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  volumes->setMinimumHeight(110); layout->addWidget(volumes, 1);
  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
  auto *copy = buttons->addButton("Copy details", QDialogButtonBox::ActionRole);
  copy->setIcon(QIcon::fromTheme("edit-copy"));
  connect(copy, &QPushButton::clicked, &dialog, [metadata] {
    QApplication::clipboard()->setText(QString::fromUtf8(QJsonDocument(metadata).toJson(QJsonDocument::Indented)));
  });
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  layout->addWidget(buttons);
  const auto available = screen()->availableGeometry().size();
  dialog.resize(qMin(1100, available.width()-60), qMin(600, available.height()-60));
  dialog.exec();
}
void GrubEditor::chooseSystem() {
  const auto target = system();
  name->setText(target["title"].toString());
  selection->setText(target.isEmpty() ? "Select a drive above to find Windows or Linux." :
    target.value("available").toBool() ? target["title"].toString()+" is ready to add. You can change its menu name below."+
        "\n"+target["partition"].toString()+" · "+target["loader"].toString()+
        (target["detail"].toString().isEmpty() ? QString() : "\n"+target["detail"].toString()) : target["reason"].toString());
  updateActions();
}
void GrubEditor::chooseEntry() {
  const auto selected = entry();
  QString detail = selected.isEmpty() ? "Select a boot entry to see its details." :
      selected["managed_id"].toString().isEmpty() ?
        "System-generated entry · Changes affect the current menu. Regenerating GRUB can restore the original." :
        "CachyOsTools entry · Changes are also saved for future GRUB generation.";
  if (!selected.isEmpty() && !selected["editable"].toBool())
    detail = "This entry has a variable-generated name and is shown for reference.";
  entryDetails->setText(detail); updateActions();
}
void GrubEditor::editEntry() {
  const auto selected = entry();
  if (busy || !selected["editable"].toBool()) return;
  QDialog dialog(this); dialog.setObjectName("grubEditDialog");
  dialog.setWindowTitle("Edit boot entry"); dialog.resize(720, 330);
  auto *layout = new QVBoxLayout(&dialog); layout->setSpacing(14);
  layout->addWidget(label("Name in GRUB", &dialog));
  auto *title = new QLineEdit(selected["name"].toString(), &dialog);
  title->setObjectName("grubEditName"); title->setMaxLength(100); layout->addWidget(title);
  QGroupBox *advanced = nullptr;
  QPlainTextEdit *commands = nullptr;
  if (selected["kind"] != "submenu") {
    advanced = new QGroupBox("Edit boot commands (advanced)", &dialog);
    advanced->setObjectName("grubAdvanced"); advanced->setCheckable(true); advanced->setChecked(false);
    auto *body = new QVBoxLayout(advanced);
    commands = new QPlainTextEdit(advanced); commands->setObjectName("grubCommands");
    commands->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    commands->setLineWrapMode(QPlainTextEdit::NoWrap); commands->setMinimumHeight(240);
    QString text = selected["commands"].toString();
    if (text.startsWith('\n')) text.remove(0, 1);
    if (text.endsWith('\n')) text.chop(1);
    commands->setPlainText(text); commands->hide(); body->addWidget(commands);
    connect(advanced, &QGroupBox::toggled, commands, &QWidget::setVisible);
    connect(advanced, &QGroupBox::toggled, &dialog, [&dialog] { dialog.adjustSize(); });
    layout->addWidget(advanced);
  }
  layout->addWidget(label(selected["managed_id"].toString().isEmpty() ?
      "Changes apply to your current boot menu. Regenerating GRUB can restore this system-generated entry." :
      "This CachyOsTools entry is also updated in the saved menu generator.", &dialog, "muted"));
  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
  layout->addWidget(buttons);
  auto *save = buttons->button(QDialogButtonBox::Save);
  save->setEnabled(!title->text().trimmed().isEmpty());
  connect(title, &QLineEdit::textChanged, save, [save](const QString &text) { save->setEnabled(!text.trimmed().isEmpty()); });
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  if (dialog.exec() != QDialog::Accepted) return;
  QJsonObject change{{"action", "edit-current"}, {"id", selected["id"]}, {"name", title->text().trimmed()}};
  if (advanced && advanced->isChecked()) change["commands"] = commands->toPlainText();
  request(change);
}
void GrubEditor::request(const QJsonObject &data) {
  if (busy) return;
  if ((data["action"] == "appearance-confirm" || data["action"] == "appearance-cancel") && worker->state() != QProcess::Running) {
    status->setText(data["action"] == "appearance-cancel" ?
      "Cancelled. Boot files are unchanged; your recovery backups are kept." :
      "This recovery review expired. Choose Review & apply again for fresh backups.");
    return;
  }
  pending = data; pending["revision"] = revision; pending["menu_revision"] = menuRevision;
  operation = data["action"].toString();
  status->setText(operation == "scan" ? "Detecting the system. Authenticate if prompted…" :
    operation == "appearance-load" ? "Loading your GRUB menu. Authenticate if prompted…" : "Working on your boot menu…");
  setBusy(true);
  if (worker->state() == QProcess::Running) {
    worker->write(QJsonDocument(pending).toJson(QJsonDocument::Compact)+'\n'); pending = {};
  } else { output.clear(); errors.clear(); worker->start("python3", {helper, "connect"}); }
}
void GrubEditor::readWorker() {
  output += worker->readAllStandardOutput();
  while (output.contains('\n')) {
    const int end = output.indexOf('\n');
    const auto data = QJsonDocument::fromJson(output.left(end)).object(); output.remove(0, end+1);
    if (data.contains("progress")) status->setText(data["progress"].toString());
    else if (data.contains("result")) {
      const auto value = data["result"].toObject(); result(value); setBusy(false);
      if (value.contains("appearance_review")) {
        // Present after releasing the in-flight request; confirmation sends a
        // second request with the helper's single-use review token.
        QTimer::singleShot(0, appearance, [this, value] { appearance->reviewChanges(value["appearance_review"].toObject()); });
      }
    }
    else if (data.contains("error")) {
      status->setText(data["error"].toString());
      setBusy(false);
    }
  }
}
void GrubEditor::result(const QJsonObject &data) {
  if (data.contains("supported")) supported = data["supported"].toBool();
  if (data.contains("mode")) mode->setText(data["mode"] == "uefi" ?
      "UEFI" + QString(data["secure_boot"].toBool() ? " · Secure Boot" : "") : "Legacy BIOS");
  if (data.contains("drives")) {
    const auto previous = drive();
    const QSignalBlocker guard(drives); drives->clear();
    for (const auto &value : data["drives"].toArray()) {
      const auto metadata = value.toObject();
      auto *row = new QTreeWidgetItem(drives, {metadata["model"].toString(), metadata["path"].toString(),
          capacity(metadata["size"].toDouble()), driveStatus(metadata)});
      row->setIcon(0, QIcon::fromTheme(metadata["removable"].toBool() ? "drive-removable-media" : "drive-harddisk"));
      row->setData(0, Qt::UserRole, metadata);
      row->setToolTip(0, metadata["path"].toString()+"\nSerial: "+metadata["serial"].toString());
      if (metadata["token"] == previous["token"]) drives->setCurrentItem(row);
    }
    driveCount->setText(QString("%1 drives · mounted and unmounted").arg(drives->topLevelItemCount()));
    const int rowHeight = drives->topLevelItemCount() ? drives->sizeHintForRow(0) : 24;
    drives->setMinimumHeight(qBound(140, drives->header()->sizeHint().height()+
        rowHeight*drives->topLevelItemCount()+8, 270));
    if (!drives->currentItem()) selection->setText("Select a drive above to find Windows or Linux.");
    else if (operation == "refresh") selection->setText("Select Detect bootable systems to inspect this drive’s partitions.");
  }
  if (data.contains("revision")) revision = data["revision"].toString();
  if (data.contains("menu_revision")) menuRevision = data["menu_revision"].toString();
  if (data.contains("can_undo")) canUndo = data["can_undo"].toBool();
  if (data.contains("entries")) managedEntries = data["entries"].toArray();
  if (data.contains("current_entries")) {
    appearance->setMenu(data["current_entries"].toArray());
    const auto previous = entry()["id"].toString();
    QHash<QString, QTreeWidgetItem *> rows;
    const QSignalBlocker guard(entries);
    entries->clear();
    int count = 0;
    for (const auto &value : data["current_entries"].toArray()) {
      const auto item = value.toObject();
      const bool submenu = item["kind"] == "submenu";
      if (!submenu) ++count;
      const auto source = item["managed_id"].toString().isEmpty() ? QFileInfo(item["source"].toString()).fileName() : "CachyOsTools";
      auto *row = new QTreeWidgetItem({item["name"].toString(), submenu ? "Submenu" : "Boot entry", source.isEmpty() ? "Current menu" : source});
      row->setIcon(0, QIcon::fromTheme(submenu ? "folder" : "computer"));
      row->setData(0, Qt::UserRole, item); row->setToolTip(0, item["menu_id"].toString());
      if (auto *parent = rows.value(item["parent_id"].toString())) parent->addChild(row);
      else entries->addTopLevelItem(row);
      rows.insert(item["id"].toString(), row);
      if (item["id"].toString() == previous || (operation == "add" && item["managed_id"] == system()["id"]))
        entries->setCurrentItem(row);
    }
    entries->expandAll();
    if (!entries->currentItem() && entries->topLevelItemCount()) entries->setCurrentItem(entries->topLevelItem(0));
    saved->setText(count ? QString("%1 boot %2 · %3").arg(count).arg(count == 1 ? "entry" : "entries").arg(data["config"].toString()) : "No literal boot entries were found in the current menu.");
    load->setText(menuRevision.isEmpty() ? "Load current menu" : "Reload menu"); chooseEntry();
    if (operation == "add" || operation == "edit-current" || operation == "remove-current") tabs->setCurrentIndex(0);
  } else if (menuRevision.isEmpty()) {
    saved->setText("Load your current menu to see its boot entries.");
    load->setText("Load current menu");
  }
  if (data.contains("candidates")) {
    systems->clear();
    int first = -1;
    for (const auto &value : data["candidates"].toArray()) {
      const auto target = value.toObject();
      const auto method = target["kind"] == "config" ? "GRUB menu" :
          target["kind"] == "legacy-windows" ? "BIOS bootloader" : "UEFI bootloader";
      auto *row = new QListWidgetItem(QIcon::fromTheme(target["title"].toString().startsWith("Windows") ? "computer" : "distributor-logo"),
          target["title"].toString()+"  ·  "+method+"  ·  "+QFileInfo(target["partition"].toString()).fileName(), systems);
      row->setData(Qt::UserRole, target);
      row->setToolTip(target.value("available").toBool() ? target["loader"].toString() : target["reason"].toString());
      if (target.value("available").toBool() && first < 0) first = systems->count()-1;
    }
    if (systems->count()) systems->setCurrentRow(first < 0 ? 0 : first);
    else selection->setText("No supported bootloader was detected on this drive.");
    QStringList messages;
    for (const auto &note : data["notes"].toArray()) messages.append(note.toString());
    notes->setText(messages.join('\n')); notes->setVisible(!messages.isEmpty());
    status->setText(first >= 0 ? "Detected. Choose the name you want in GRUB, then Add to GRUB." : "Choose another drive or check the detection details above.");
  } else if (tabs->currentIndex() == 2 && supported && menuRevision.isEmpty()) {
    status->setText("Administrator access is needed to read this GRUB menu. Choose Load GRUB menu to continue.");
  } else status->setText(data.value("message").toString(tabs->currentIndex() == 2 ?
      "Choose a design. Review & apply creates your backups before confirmation." :
      "Select an entry to edit, or open Add Entry to add another system."));
  if (data.contains("backup")) saved->setToolTip("Backup: "+data["backup"].toString());
  if (data.contains("appearance")) appearance->acceptResult(data["appearance"].toObject());
}
