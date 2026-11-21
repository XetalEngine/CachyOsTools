#include "grub_appearance.h"
#include <QtWidgets>
#include <QProcess>

namespace {
QLabel *note(const QString &text, QWidget *parent, const char *name = "") {
  auto *label = new QLabel(text, parent); label->setObjectName(name);
  label->setTextFormat(Qt::PlainText); label->setWordWrap(true); return label;
}
QWidget *sliderRow(QSlider *slider, QWidget *parent) {
  auto *row = new QWidget(parent); auto *layout = new QHBoxLayout(row);
  layout->setContentsMargins(0, 0, 0, 0); auto *value = new QLabel(row);
  value->setMinimumWidth(38); value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  layout->addWidget(slider, 1); layout->addWidget(value);
  QObject::connect(slider, &QSlider::valueChanged, value, [value](int n) { value->setText(QString::number(n)+"%"); });
  return row;
}
}

class GrubDesignPreview final : public QWidget {
public:
  QPixmap image;
  explicit GrubDesignPreview(QWidget *parent) : QWidget(parent) {
    setObjectName("grubDesignPreview"); setMinimumSize(240, 160);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAccessibleName("Boot menu design preview");
  }
  void paintEvent(QPaintEvent *) override {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    QRectF bounds = rect().adjusted(2, 2, -2, -2);
    p.setPen(QPen(palette().color(QPalette::Mid), 1)); p.setBrush(palette().color(QPalette::Base));
    p.drawRoundedRect(bounds, 14, 14);
    if (image.isNull()) {
      p.setPen(palette().color(QPalette::PlaceholderText));
      p.drawText(rect(), Qt::AlignCenter, "Choose a theme to preview your boot screen"); return;
    }
    QSizeF size = image.size(); size.scale(bounds.size()-QSizeF(16, 16), Qt::KeepAspectRatio);
    QRectF target(QPointF(), size); target.moveCenter(bounds.center());
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    QPainterPath clip; clip.addRoundedRect(target, 9, 9); p.setClipPath(clip);
    p.drawPixmap(target, image, image.rect());
  }
};

GrubAppearance::GrubAppearance(const QString &helper, QWidget *parent) : QWidget(parent), helper(helper) {
  setObjectName("grubAppearanceTab");
  auto *layout = new QVBoxLayout(this); layout->setSpacing(10);
  layout->setSizeConstraint(QLayout::SetNoConstraint);
  auto *intro = new QHBoxLayout;
  intro->addWidget(note("Choose a theme, then make it yours. Review your backups before applying.", this), 1);
  themePicker = new QComboBox(this); themePicker->setObjectName("grubAppearanceThemePicker");
  themePicker->setMinimumWidth(180); themePicker->hide(); intro->addWidget(themePicker); layout->addLayout(intro);
  content = new QWidget(this); auto *canvas = new QVBoxLayout(content);
  canvas->setContentsMargins(0, 0, 0, 0); canvas->setSpacing(10);
  themeShelf = new QWidget(content); auto *shelf = new QHBoxLayout(themeShelf); shelf->setContentsMargins(0, 0, 0, 0);
  auto *previous = new QToolButton(themeShelf); previous->setArrowType(Qt::LeftArrow); previous->setToolTip("Previous themes");
  auto *next = new QToolButton(themeShelf); next->setArrowType(Qt::RightArrow); next->setToolTip("More themes");
  themes = new QListWidget(themeShelf); themes->setObjectName("grubAppearanceThemes");
  themes->setViewMode(QListView::IconMode); themes->setFlow(QListView::LeftToRight);
  themes->setWrapping(false); themes->setMovement(QListView::Static); themes->setResizeMode(QListView::Adjust);
  themes->setSelectionMode(QAbstractItemView::SingleSelection); themes->setIconSize(QSize(126, 66));
  themes->setGridSize(QSize(148, 102)); themes->setFixedHeight(108);
  themes->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  themes->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); themes->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  themes->setAccessibleName("Boot screen themes");
  shelf->addWidget(previous); shelf->addWidget(themes, 1); shelf->addWidget(next); canvas->addWidget(themeShelf);
  connect(previous, &QToolButton::clicked, this, [this] { themes->horizontalScrollBar()->setValue(themes->horizontalScrollBar()->value()-themes->viewport()->width()); });
  connect(next, &QToolButton::clicked, this, [this] { themes->horizontalScrollBar()->setValue(themes->horizontalScrollBar()->value()+themes->viewport()->width()); });
  body = new QBoxLayout(QBoxLayout::LeftToRight); body->setSpacing(16); canvas->addLayout(body, 1);
  auto *previewColumn = new QWidget(content); auto *previewLayout = new QVBoxLayout(previewColumn);
  previewLayout->setContentsMargins(0, 0, 0, 0);
  selection = note("BOOT SCREEN PREVIEW", previewColumn, "muted"); previewLayout->addWidget(selection);
  preview = new GrubDesignPreview(previewColumn); previewLayout->addWidget(preview, 1);
  caption = note("Preview only · Apply opens a backup and recovery review.", previewColumn, "grubPreviewStatus");
  previewLayout->addWidget(caption); body->addWidget(previewColumn, 1);
  auto *settingsColumn = new QWidget(content); settingsColumn->setMinimumWidth(320); settingsColumn->setMaximumWidth(380);
  auto *column = new QVBoxLayout(settingsColumn); column->setContentsMargins(0, 0, 0, 0);
  controls = new QTabWidget(settingsColumn); controls->setObjectName("grubAppearanceControls");
  column->addWidget(controls, 1);
  auto *picturePage = new QWidget(controls); auto *pictureForm = new QVBoxLayout(picturePage); pictureForm->setSpacing(10);
  pictures = new QComboBox(picturePage); pictures->setObjectName("grubAppearancePicture");
  pictures->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon); pictures->setMinimumContentsLength(18);
  pictureForm->addWidget(note("Background picture", picturePage)); pictureForm->addWidget(pictures);
  browse = new QPushButton(QIcon::fromTheme("folder-pictures"), "Choose picture…", picturePage);
  browse->setObjectName("grubAppearanceBrowse"); pictureForm->addWidget(browse);
  dimming = new QSlider(Qt::Horizontal, picturePage); dimming->setRange(0, 95); dimming->setObjectName("grubAppearanceDimming");
  pictureForm->addWidget(note("Picture dimming", picturePage)); pictureForm->addWidget(sliderRow(dimming, picturePage));
  contrast = new QSlider(Qt::Horizontal, picturePage); contrast->setRange(0, 95); contrast->setObjectName("grubAppearanceContrast");
  pictureForm->addWidget(note("Menu shading", picturePage)); pictureForm->addWidget(sliderRow(contrast, picturePage)); pictureForm->addStretch();
  controls->addTab(picturePage, "Picture");
  auto *menuPage = new QWidget(controls); auto *form = new QFormLayout(menuPage); form->setVerticalSpacing(12);
  form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
  position = new QComboBox(menuPage); position->setObjectName("grubAppearancePosition");
  position->addItem("Left", "left"); position->addItem("Center", "center"); position->addItem("Right", "right"); form->addRow("Position", position);
  heading = new QLineEdit(menuPage); heading->setObjectName("grubAppearanceHeading"); heading->setMaxLength(64);
  subtitle = new QLineEdit(menuPage); subtitle->setObjectName("grubAppearanceSubtitle"); subtitle->setMaxLength(100);
  form->addRow("Heading", heading); form->addRow("Subtitle", subtitle);
  textSize = new QComboBox(menuPage); textSize->setObjectName("grubAppearanceTextSize");
  textSize->addItem("Small", 18); textSize->addItem("Regular", 22); textSize->addItem("Large", 28); form->addRow("Text size", textSize);
  hints = new QCheckBox("Show navigation hints", menuPage); hints->setObjectName("grubAppearanceHints"); form->addRow(hints);
  controls->addTab(menuPage, "Menu");
  auto *colorPage = new QWidget(controls); auto *colors = new QVBoxLayout(colorPage); colors->setSpacing(12);
  colors->addWidget(note("Fine-tune the colors of this boot screen.", colorPage));
  accent = new QPushButton("Selection & accent", colorPage); textColor = new QPushButton("Menu text", colorPage); base = new QPushButton("Menu shade", colorPage);
  accent->setObjectName("grubAppearanceAccent"); textColor->setObjectName("grubAppearanceTextColor"); base->setObjectName("grubAppearanceBase");
  colors->addWidget(accent); colors->addWidget(textColor); colors->addWidget(base); colors->addStretch(); controls->addTab(colorPage, "Colors");
  auto *reset = new QPushButton("Reset this theme", settingsColumn); reset->setObjectName("grubAppearanceReset"); column->addWidget(reset);
  body->addWidget(settingsColumn); layout->addWidget(content, 1);
  auto *actions = new QHBoxLayout;
  reload = new QPushButton("Reload design", this); reload->setObjectName("grubAppearanceReload");
  restore = new QPushButton("Use system appearance", this); restore->setObjectName("grubAppearanceSystem");
  undo = new QPushButton("Undo appearance", this); undo->setObjectName("grubAppearanceUndo");
  apply = new QPushButton("Review && apply…", this); apply->setProperty("primary", true); apply->setObjectName("grubAppearanceApply");
  actions->addWidget(reload); actions->addWidget(restore); actions->addWidget(undo); actions->addStretch(); actions->addWidget(apply); layout->addLayout(actions);

  catalog = new QProcess(this); renderer = new QProcess(this); debounce = new QTimer(this);
  debounce->setSingleShot(true); debounce->setInterval(220);
  connect(debounce, &QTimer::timeout, this, &GrubAppearance::renderPreview);
  connect(reload, &QPushButton::clicked, this, &GrubAppearance::loadCatalog);
  connect(themes, &QListWidget::currentItemChanged, this, [this] {
    if (!syncing && themes->currentItem()) loadOptions(themes->currentItem()->data(Qt::UserRole).toJsonObject()["options"].toObject());
  });
  connect(themePicker, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int row) { if (!syncing) themes->setCurrentRow(row); });
  connect(reset, &QPushButton::clicked, this, [this] {
    if (themes->currentItem()) loadOptions(themes->currentItem()->data(Qt::UserRole).toJsonObject()["options"].toObject());
  });
  for (auto *combo : {pictures, position, textSize}) connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), this, &GrubAppearance::changed);
  for (auto *edit : {heading, subtitle}) connect(edit, &QLineEdit::textChanged, this, &GrubAppearance::changed);
  for (auto *slider : {dimming, contrast}) connect(slider, &QSlider::valueChanged, this, &GrubAppearance::changed);
  connect(hints, &QCheckBox::toggled, this, &GrubAppearance::changed);
  connect(accent, &QPushButton::clicked, this, [this] { chooseColor("accent", accent); });
  connect(textColor, &QPushButton::clicked, this, [this] { chooseColor("text", textColor); });
  connect(base, &QPushButton::clicked, this, [this] { chooseColor("base", base); });
  connect(browse, &QPushButton::clicked, this, [this] {
    const QString path = QFileDialog::getOpenFileName(this, "Choose a boot screen picture", QStandardPaths::writableLocation(QStandardPaths::PicturesLocation), "Pictures (*.png *.jpg *.jpeg *.webp *.bmp);;All files (*)");
    if (path.isEmpty()) return;
    int index = pictures->findData(path);
    if (index < 0) { pictures->addItem(QFileInfo(path).fileName(), path); index = pictures->count()-1; }
    pictures->setCurrentIndex(index);
  });
  connect(apply, &QPushButton::clicked, this, [this] {
    if (!readable) { emit requested({{"action", "appearance-load"}}); return; }
    if (previewReady) emit requested({{"action", "appearance-review"}, {"operation", "apply"}, {"options", settings},
      {"appearance_revision", revision}, {"background_sha", backgroundHash}});
  });
  connect(restore, &QPushButton::clicked, this, [this] { emit requested({{"action", "appearance-review"}, {"operation", "system"}, {"appearance_revision", revision}}); });
  connect(undo, &QPushButton::clicked, this, [this] { emit requested({{"action", "appearance-review"}, {"operation", "undo"}, {"appearance_revision", revision}}); });
  connect(catalog, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, [this](int code, QProcess::ExitStatus exit) {
    const auto data = QJsonDocument::fromJson(catalog->readAllStandardOutput()).object();
    if (code || exit != QProcess::NormalExit || !data.contains("presets")) {
      caption->setText(data["error"].toString("Could not load the boot screen themes. Use Reload applied design to retry."));
      previewReady = false; updateActions(); return;
    }
    syncing = true; themes->clear(); themePicker->clear(); pictures->clear(); pictures->addItem("Solid color", "");
    for (const auto &value : data["pictures"].toArray()) {
      const auto picture = value.toObject(); pictures->addItem(picture["name"].toString(), picture["path"].toString());
    }
    for (const auto &value : data["presets"].toArray()) {
      const auto preset = value.toObject();
      QPixmap art(preset["options"].toObject()["background"].toString());
      if (art.isNull()) {
        const auto options = preset["options"].toObject();
        art = QPixmap(308, 160);
        art.fill(QColor(options["base"].toString("#1d1f24")));
        QPainter painter(&art);
        const QColor accent(options["accent"].toString("#27ae60"));
        QLinearGradient glow(0, 0, 308, 160);
        auto soft = accent; soft.setAlpha(70);
        glow.setColorAt(0, soft); glow.setColorAt(1, Qt::transparent);
        painter.fillRect(art.rect(), glow);
        painter.setPen(QColor(options["text"].toString("#f2f4f8")));
        painter.setFont(QFont("sans-serif", 12, QFont::Bold));
        painter.drawText(QRect(20, 16, 270, 30), Qt::AlignLeft | Qt::AlignVCenter, "CACHYOSTOOLS");
        painter.fillRect(20, 58, 180, 24, accent);
        painter.setOpacity(.4);
        for (int y : {92, 115}) painter.fillRect(24, y, 142, 4, painter.pen().color());
      }
      auto *item = new QListWidgetItem(QIcon(art.scaled(308, 160, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation)), preset["name"].toString(), themes);
      item->setData(Qt::UserRole, preset); item->setToolTip(preset["name"].toString());
      themePicker->addItem(preset["name"].toString());
    }
    active = data["active"].toBool(); canUndo = data["can_undo"].toBool() || active;
    revision = data["revision"].toString(); syncing = false;
    auto options = data["options"].toObject();
    if (options.isEmpty() && themes->count()) options = themes->item(0)->data(Qt::UserRole).toJsonObject()["options"].toObject();
    loadOptions(options);
  });
  connect(catalog, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
    if (error == QProcess::FailedToStart) { caption->setText("Could not start the theme reader."); updateActions(); }
  });
  connect(renderer, &QProcess::started, this, [this] {
    renderer->write(QJsonDocument(rendering).toJson(QJsonDocument::Compact)); renderer->closeWriteChannel();
  });
  connect(renderer, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, [this](int code, QProcess::ExitStatus exit) {
    const auto data = QJsonDocument::fromJson(renderer->readAllStandardOutput()).object();
    if (renderingGeneration != generation) { debounce->start(); return; }
    QPixmap picture;
    previewReady = !code && exit == QProcess::NormalExit && picture.loadFromData(QByteArray::fromBase64(data["image"].toString().toLatin1()), "PNG");
    if (previewReady) {
      preview->image = picture; preview->update(); backgroundHash = data["background_sha"].toString();
      caption->setText("Preview · "+QString(menu.isEmpty() ? "Sample boot entries" : "Your current boot entries")+" · Firmware resolution can affect text spacing.");
    } else caption->setText(data["error"].toString("Could not preview this picture. Choose another image."));
    updateActions();
  });
  connect(renderer, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
    if (error == QProcess::FailedToStart) { caption->setText("Could not start the design preview."); previewReady = false; updateActions(); }
  });
  updateActions();
}

GrubAppearance::~GrubAppearance() {
  for (auto *process : {catalog, renderer}) if (process->state() != QProcess::NotRunning) { process->kill(); process->waitForFinished(1500); }
}
void GrubAppearance::showEvent(QShowEvent *event) {
  QWidget::showEvent(event); if (!loaded) { loaded = true; loadCatalog(); }
}
void GrubAppearance::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);
  const bool compact = height() < 520;
  themeShelf->setVisible(!compact); themePicker->setVisible(compact);
}
void GrubAppearance::loadCatalog() {
  if (busy || catalog->state() != QProcess::NotRunning) return;
  previewReady = false; ++generation; caption->setText("Loading boot screen themes…"); updateActions();
  catalog->start("python3", {helper, "catalog"});
}
void GrubAppearance::loadOptions(const QJsonObject &value) {
  if (value.isEmpty()) return;
  syncing = true; settings = value;
  for (int i=0; i<themes->count(); ++i)
    if (themes->item(i)->data(Qt::UserRole).toJsonObject()["id"] == settings["preset"]) { themes->setCurrentRow(i); themePicker->setCurrentIndex(i); themes->scrollToItem(themes->item(i)); break; }
  const auto path = settings["background"].toString(); int picture = pictures->findData(path);
  if (picture < 0) { pictures->addItem("Applied picture · "+QFileInfo(path).fileName(), path); picture = pictures->count()-1; }
  pictures->setCurrentIndex(picture); position->setCurrentIndex(qMax(0, position->findData(settings["layout"].toString())));
  textSize->setCurrentIndex(qMax(0, textSize->findData(settings["size"].toInt(22))));
  heading->setText(settings["heading"].toString()); subtitle->setText(settings["subtitle"].toString());
  dimming->setValue(settings["dimming"].toInt()); contrast->setValue(settings["contrast"].toInt()); hints->setChecked(settings["hints"].toBool());
  colorButtons(); syncing = false; changed();
}
void GrubAppearance::changed() {
  if (syncing || settings.isEmpty()) return;
  settings["background"] = pictures->currentData().toString(); settings["layout"] = position->currentData().toString();
  settings["size"] = textSize->currentData().toInt(); settings["heading"] = heading->text(); settings["subtitle"] = subtitle->text();
  settings["dimming"] = dimming->value(); settings["contrast"] = contrast->value(); settings["hints"] = hints->isChecked();
  selection->setText((themes->currentItem() ? themes->currentItem()->text().toUpper() : "BOOT SCREEN")+"  /  PREVIEW");
  previewReady = false; ++generation; caption->setText("Updating preview…"); updateActions(); debounce->start();
}
void GrubAppearance::renderPreview() {
  if (settings.isEmpty() || renderer->state() != QProcess::NotRunning) return;
  rendering = {{"options", settings}, {"entries", menu}}; renderingGeneration = generation;
  renderer->start("python3", {helper, "preview"});
}
void GrubAppearance::chooseColor(const QString &key, QPushButton *) {
  const QColor color = QColorDialog::getColor(QColor(settings[key].toString()), this, "Choose "+key+" color");
  if (color.isValid()) { settings[key] = color.name(); colorButtons(); changed(); }
}
void GrubAppearance::colorButtons() {
  for (const auto &item : {qMakePair(accent, QString("accent")), qMakePair(textColor, QString("text")), qMakePair(base, QString("base"))}) {
    QPixmap swatch(18, 18); swatch.fill(QColor(settings[item.second].toString()));
    item.first->setIcon(QIcon(swatch)); item.first->setToolTip(settings[item.second].toString());
  }
}
void GrubAppearance::setMenu(const QJsonArray &entries) {
  QJsonArray names;
  for (const auto &value : entries) { const auto entry = value.toObject(); if (entry["parent_id"].toString().isEmpty()) names.append(entry["name"]); }
  if (menu != names) { menu = names; if (loaded) changed(); }
}
void GrubAppearance::setAvailability(bool working, bool available, bool hasMenu) {
  busy = working; supported = available; readable = hasMenu; updateActions();
}
void GrubAppearance::updateActions() {
  content->setEnabled(!busy); themePicker->setEnabled(!busy); reload->setEnabled(!busy && catalog->state() == QProcess::NotRunning);
  apply->setText(readable ? "Review && apply…" : "Load GRUB menu…");
  apply->setEnabled(!busy && supported && (!readable || previewReady));
  restore->setEnabled(!busy && supported && readable && active);
  undo->setEnabled(!busy && supported && readable && canUndo);
  apply->setToolTip(!supported ? "An existing GRUB installation is required. See the status below." :
    readable ? "Prepare backups and review recovery instructions before saving" :
    "Authenticate to read your GRUB menu. Your chosen design stays in preview.");
  if (previewReady) caption->setText(!supported ? "GRUB is unavailable · See the status below." :
    !readable ? "Preview only · Choose Load GRUB menu to authenticate and load your boot entries." :
    "Preview · "+QString(menu.isEmpty() ? "Sample boot entries" : "Your current boot entries")+" · Firmware resolution can affect text spacing.");
}
void GrubAppearance::acceptResult(const QJsonObject &data) {
  revision = data["revision"].toString(); active = data["active"].toBool(); canUndo = data["can_undo"].toBool();
  if (!data["options"].toObject().isEmpty()) loadOptions(data["options"].toObject());
  updateActions();
}

void GrubAppearance::reviewChanges(const QJsonObject &data) {
  if (data["token"].toString().isEmpty() || data["files"].toArray().isEmpty()) return;
  QDialog dialog(this); dialog.setObjectName("grubRecoveryReview");
  dialog.setWindowTitle("Review your GRUB backups"); dialog.setProperty("xetalWindowSized", true);
  auto *layout = new QVBoxLayout(&dialog); layout->setSpacing(12);
  layout->addWidget(note("Take a picture of the screen. Here are your backups.", &dialog, "panelTitle"));
  layout->addWidget(note("The backups below have been saved. Your boot files will change only after you confirm Apply.", &dialog));
  auto selectable = [](const QString &text, QWidget *parent) {
    auto *label = note(text, parent); label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred); return label;
  };
  auto *folder = selectable(data["backup"].toString(), &dialog); folder->setObjectName("grubRecoveryBackupFolder");
  layout->addWidget(folder);
  auto *pages = new QTabWidget(&dialog); pages->setObjectName("grubRecoveryPages"); layout->addWidget(pages, 1);
  auto *filesPage = new QWidget(pages); auto *files = new QVBoxLayout(filesPage);
  auto *grid = new QGridLayout; grid->setHorizontalSpacing(18); grid->setVerticalSpacing(14);
  grid->addWidget(note("File that will change", filesPage, "muted"), 0, 0);
  grid->addWidget(note("Backup already saved", filesPage, "muted"), 0, 1);
  int row = 1;
  for (const auto &value : data["files"].toArray()) {
    const auto file = value.toObject();
    grid->addWidget(selectable(file["change"].toString()+" · "+file["path"].toString(), filesPage), row, 0);
    grid->addWidget(selectable(file["backup"].toString().isEmpty() ?
      "New file — no previous copy. Recovery removes it." : file["backup"].toString(), filesPage), row++, 1);
  }
  grid->setColumnStretch(0, 1); grid->setColumnStretch(1, 1); files->addLayout(grid); files->addStretch();
  files->addWidget(note("Recovery Instructions: If Linux starts, open a terminal and run:", filesPage));
  auto *command = selectable(data["command"].toString(), filesPage); command->setObjectName("grubRecoveryCommand"); files->addWidget(command);
  files->addWidget(note("If Linux cannot start, use a live USB. Open Recovery instructions for the mounting steps and command.", filesPage));
  pages->addTab(filesPage, "Files && backups");
  if (!data["created"].toArray().isEmpty()) {
    auto *assetsPage = new QWidget(pages); auto *assets = new QVBoxLayout(assetsPage);
    const auto paths = data["created"].toArray();
    assets->addWidget(note("New theme files · Existing themes are kept", assetsPage, "panelTitle"));
    assets->addWidget(selectable(QFileInfo(paths.first().toString()).absolutePath(), assetsPage));
    auto *names = new QGridLayout;
    for (int i = 0; i < paths.size(); ++i)
      names->addWidget(selectable(QFileInfo(paths[i].toString()).fileName(), assetsPage), i/3, i%3);
    assets->addLayout(names); assets->addStretch();
    assets->addWidget(note("These files do not replace existing files. Recovery restores the previous menu; these unused assets can stay on disk.", assetsPage));
    pages->addTab(assetsPage, "New theme files");
  }
  auto *recoveryPage = new QWidget(pages); auto *recovery = new QVBoxLayout(recoveryPage);
  auto *instructions = selectable(data["instructions"].toString(), recoveryPage); instructions->setObjectName("grubRecoveryInstructions");
  recovery->addWidget(instructions); recovery->addStretch(); pages->addTab(recoveryPage, "Recovery instructions");
  layout->addWidget(note("A complete copy is saved as RECOVERY.txt in the backup folder above. You can also save a copy to another drive.", &dialog, "muted"));
  auto *expiry = note("This review expires after five minutes. Changed boot files require fresh backups.", &dialog, "muted"); layout->addWidget(expiry);
  auto *buttons = new QDialogButtonBox(&dialog); layout->addWidget(buttons);
  auto *copy = buttons->addButton("Copy all details", QDialogButtonBox::ActionRole);
  auto *save = buttons->addButton("Save instructions…", QDialogButtonBox::ActionRole);
  auto *cancel = buttons->addButton(QDialogButtonBox::Cancel); cancel->setDefault(true);
  auto *confirm = buttons->addButton("Apply for next boot", QDialogButtonBox::AcceptRole);
  confirm->setObjectName("grubRecoveryConfirm"); confirm->setAutoDefault(false);
  connect(copy, &QPushButton::clicked, &dialog, [data] { QApplication::clipboard()->setText(data["report"].toString()); });
  connect(save, &QPushButton::clicked, &dialog, [&dialog, data] {
    const auto path = QFileDialog::getSaveFileName(&dialog, "Save GRUB recovery instructions",
      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)+"/grub-recovery-"+QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss")+".txt", "Text files (*.txt)");
    if (path.isEmpty()) return;
    QSaveFile file(path); const auto bytes = data["report"].toString().toUtf8();
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
      QMessageBox::warning(&dialog, "Could not save instructions", file.errorString());
  });
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  QTimer timer; timer.setSingleShot(true); timer.start(300000);
  connect(&timer, &QTimer::timeout, &dialog, [confirm, expiry] {
    confirm->setEnabled(false); expiry->setText("This review expired. Cancel and choose Review & apply again for fresh backups.");
  });
  dialog.resize(1120, 740);
  const bool confirmed = dialog.exec() == QDialog::Accepted && confirm->isEnabled();
  emit requested({{"action", confirmed ? "appearance-confirm" : "appearance-cancel"}, {"token", data["token"]}});
}
