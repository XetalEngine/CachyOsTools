#include "disk_visualizer.h"
#include "disk_layout.h"
#include "panel_widgets.h"
#include "../file_manager.h"
#include <QtWidgets>
#include <QStorageInfo>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonArray>
#include <QVariantAnimation>
#include <utility>

namespace {
QPointF mousePosition(QMouseEvent *event) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  return event->position();
#else
  return event->localPos();
#endif
}
QPoint mouseGlobalPosition(QMouseEvent *event) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  return event->globalPosition().toPoint();
#else
  return event->globalPos();
#endif
}

QString bytes(double value) {
  const QStringList units{"B", "KiB", "MiB", "GiB", "TiB", "PiB"};
  int unit = 0;
  while (value >= 1024 && unit < units.size() - 1) { value /= 1024; ++unit; }
  return QString::number(value, 'f', unit ? 1 : 0) + " " + units[unit];
}
qint64 id(const QJsonObject &node) { return qint64(node["id"].toDouble()); }
bool container(const QJsonObject &node) {
  return node["kind"] == "dir" || node["kind"] == "files" || node["kind"] == "remainder";
}
QIcon nodeIcon(const QJsonObject &node) {
  const auto name = node["kind"] == "dir" ? "folder" : node["kind"] == "files" ? "folder-documents"
                    : node["kind"] == "link" ? "emblem-symbolic-link" : "text-x-generic";
  return QIcon::fromTheme(name, QApplication::style()->standardIcon(
      node["kind"] == "dir" ? QStyle::SP_DirIcon : QStyle::SP_FileIcon));
}

class DiskMap final : public QWidget {
public:
  struct Tile { QRectF rect; QJsonObject node; QColor color; int depth; bool branch; };
  QVector<Tile> tiles;
  QJsonObject root;
  QString metric = "allocated";
  qint64 selected = 0;
  int hovered = -1;
  QVariantAnimation reveal;
  std::function<void(QJsonObject)> choose, enter;
  std::function<void(QJsonObject, QPoint)> context;
  explicit DiskMap(QWidget *parent) : QWidget(parent), reveal(this) {
    setObjectName("diskTreemap");
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(260, 200);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAccessibleName("Disk usage map. Select a tile; double-click a folder to zoom in.");
    reveal.setDuration(180);
    reveal.setStartValue(.35);
    reveal.setEndValue(1.);
    connect(&reveal, &QVariantAnimation::valueChanged, this, [this] { update(); });
  }
  void setMap(const QJsonObject &value, const QString &measure) {
    root = value; metric = measure; hovered = -1;
    rebuild(); reveal.start(); update();
  }
  void rebuild() {
    tiles.clear();
    auto build = [&](auto &&self, const QJsonObject &node, QRectF area, int depth, QColor inherited) -> void {
      const auto children = node["tiles"].toArray();
      QVector<double> weights;
      for (auto child : children) weights.append(child.toObject()[metric].toDouble());
      const auto rectangles = DiskLayout::rectangles(weights, area);
      for (int i = 0; i < children.size(); ++i) {
        const auto child = children[i].toObject();
        auto rect = rectangles[i].adjusted(1, 1, -1, -1);
        if (rect.width() < 2 || rect.height() < 2) continue;
        auto color = inherited;
        if (!depth) {
          const auto accent = palette().color(QPalette::Link);
          const auto hue = accent.hsvHueF() < 0 ? .55 : accent.hsvHueF();
          const double shift = (qHash(child["name"].toString()) % 7 - 3.) * .035;
          color = QColor::fromHsvF(std::fmod(hue + shift + 1., 1.), .58, .62);
          if (child["kind"] == "files") color = QColor::fromHsvF(std::fmod(hue + .18, 1.), .45, .65);
        } else color = color.lighter(100 + (qHash(child["name"].toString()) % 16));
        const bool branch = !child["tiles"].toArray().isEmpty() && rect.width() > 74 && rect.height() > 64;
        tiles.append({rect, child, color, depth, branch});
        if (branch) self(self, child, rect.adjusted(3, 24, -3, -3), depth + 1, color.darker(112));
      }
    };
    build(build, root, QRectF(rect()).adjusted(8, 8, -8, -8), 0, QColor());
  }
  int hit(QPointF pos) const {
    for (int i = tiles.size() - 1; i >= 0; --i) if (tiles[i].rect.contains(pos)) return i;
    return -1;
  }
protected:
  void resizeEvent(QResizeEvent *) override { rebuild(); }
  void changeEvent(QEvent *event) override {
    if (event->type() == QEvent::PaletteChange) { rebuild(); update(); }
    QWidget::changeEvent(event);
  }
  void paintEvent(QPaintEvent *) override {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(palette().color(QPalette::Base));
    p.setPen(QPen(palette().color(QPalette::Midlight), 1));
    p.drawRoundedRect(QRectF(rect()).adjusted(.5,.5,-.5,-.5), 10, 10);
    if (tiles.isEmpty()) {
      const auto center = rect().center();
      const auto accent = palette().color(QPalette::Link);
      for (const auto &cell : {QRect(-65,-75,75,56), QRect(15,-75,50,90), QRect(-65,-14,75,29), QRect(-65,20,130,24)}) {
        auto shade = accent; shade.setAlpha(60); p.setBrush(shade); p.setPen(QPen(accent, 1));
        p.drawRoundedRect(cell.translated(center), 5, 5);
      }
      p.setPen(palette().color(QPalette::Text));
      auto f = font(); f.setPointSizeF(14); f.setBold(true); p.setFont(f);
      p.drawText(QRect(12, center.y()+63, width()-24, 28), Qt::AlignCenter,
                 root.isEmpty() ? "Your storage, at a glance" : "No space used in this view");
      f.setPointSizeF(10); f.setBold(false); p.setFont(f);
      p.setPen(palette().color(QPalette::PlaceholderText));
      p.drawText(QRect(18, center.y()+98, width()-36, 60), Qt::AlignHCenter | Qt::TextWordWrap,
                 root.isEmpty() ? "Choose a partition and folder, then Scan with sudo." : "Zero-size items are still available in the tree.");
      return;
    }
    if (reveal.state() == QAbstractAnimation::Running) p.setOpacity(reveal.currentValue().toDouble());
    for (int i = 0; i < tiles.size(); ++i) {
      const auto &tile = tiles[i];
      auto color = i == hovered ? tile.color.lighter(125) : tile.color;
      QLinearGradient gradient(tile.rect.topLeft(), tile.rect.bottomRight());
      gradient.setColorAt(0, color.lighter(125)); gradient.setColorAt(.4, color); gradient.setColorAt(1, color.darker(165));
      p.setBrush(gradient); p.setPen(QPen(QColor(255,255,255,35), .8));
      p.drawRoundedRect(tile.rect, 4, 4);
      if (tile.rect.width() < 42 || tile.rect.height() < 20) continue;
      auto font = this->font(); font.setPointSizeF(tile.depth ? 9 : 10); font.setBold(tile.branch || !tile.depth); p.setFont(font);
      p.setPen(QColor("#f2f5fc"));
      auto labelRect = tile.rect.adjusted(7, tile.branch ? 0 : 5, -7, -2);
      labelRect.setHeight(tile.branch ? 23 : qMin(22., labelRect.height()));
      const auto name = p.fontMetrics().elidedText(tile.node["name"].toString(), Qt::ElideMiddle, int(labelRect.width()));
      p.drawText(labelRect, Qt::AlignLeft | Qt::AlignVCenter, name);
      if (!tile.branch && tile.rect.height() > 50 && tile.rect.width() > 66) {
        font.setPointSizeF(9); font.setBold(false); p.setFont(font); p.setPen(QColor(235,243,255,210));
        labelRect.translate(0, 23); p.drawText(labelRect, Qt::AlignLeft | Qt::AlignVCenter, bytes(tile.node[metric].toDouble()));
      }
    }
    // Selection is drawn last so nested blocks cannot obscure the outline.
    p.setBrush(Qt::NoBrush);
    for (const auto &tile : tiles) if (id(tile.node) == selected) {
      p.setPen(QPen(QColor(235,245,255,210), 2)); p.drawRoundedRect(tile.rect.adjusted(1,1,-1,-1), 4,4);
    }
  }
  void mouseMoveEvent(QMouseEvent *event) override {
    auto next = hit(mousePosition(event));
    if (next != hovered) {
      hovered = next; update();
      if (next >= 0) {
        const auto n = tiles[next].node;
        QToolTip::showText(mouseGlobalPosition(event), n["path"].toString().toHtmlEscaped() +
            "<br><b>" + bytes(n[metric].toDouble()) + "</b> · " + (container(n) ? "Double-click to explore" : "Right-click for actions"), this);
      } else QToolTip::hideText();
    }
  }
  void leaveEvent(QEvent *) override { hovered = -1; QToolTip::hideText(); update(); }
  void mousePressEvent(QMouseEvent *event) override {
    const int at = hit(mousePosition(event));
    if (at < 0) return;
    if (choose) choose(tiles[at].node);
    if (event->button() == Qt::RightButton && context) context(tiles[at].node, mouseGlobalPosition(event));
  }
  void mouseDoubleClickEvent(QMouseEvent *event) override {
    const int at = hit(mousePosition(event));
    if (at >= 0 && event->button() == Qt::LeftButton && container(tiles[at].node) && enter) enter(tiles[at].node);
  }
};
} // namespace

struct DiskVisualizer::Impl {
  DiskVisualizer *q;
  std::function<bool()> authenticate;
  QProcess helper, trash;
  QByteArray buffer;
  QHash<int, std::function<void(QJsonObject)>> pending;
  int sequence = 0, viewSequence = 0, scanSequence = 0, selectionSequence = 0;
  bool fixture = false, busy = false, complete = false;
  QString scannedPath;
  QComboBox *mounts, *measure;
  QLineEdit *folder;
  QPushButton *browseButton, *scanButton, *cancelButton, *up, *trashButton, *deleteButton, *openButton, *copyButton;
  QCheckBox *cross;
  QLabel *summary, *status, *details, *mapTitle;
  QProgressBar *progress;
  QTreeWidget *tree;
  DiskMap *map;
  QHBoxLayout *crumbs;
  QJsonObject root, focus, selection;
  QHash<qint64, QTreeWidgetItem *> items;

  Impl(DiskVisualizer *owner, std::function<bool()> auth) : q(owner), authenticate(std::move(auth)), helper(owner), trash(owner) {
    auto *layout = new QVBoxLayout(q); layout->setContentsMargins(4, 8, 4, 0); layout->setSpacing(10);
    auto *heading = new QHBoxLayout;
    auto *title = new QLabel("Disk Visualizer", q); title->setObjectName("panelTitle"); heading->addWidget(title);
    heading->addStretch();
    summary = new QLabel("Folders first · largest first", q); summary->setObjectName("muted"); heading->addWidget(summary);
    layout->addLayout(heading);
    auto *source = new QHBoxLayout;
    mounts = new QComboBox(q); mounts->setObjectName("diskMounts"); mounts->setMinimumWidth(190); mounts->setMaximumWidth(310);
    mounts->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon); mounts->setMinimumContentsLength(18);
    mounts->setToolTip("Mounted partitions · no drives are mounted or unmounted here");
    auto *refresh = new QToolButton(q); refresh->setIcon(QIcon::fromTheme("view-refresh")); refresh->setToolTip("Refresh mounted partitions");
    folder = new QLineEdit(QDir::homePath(), q); folder->setObjectName("diskFolder"); folder->setPlaceholderText("Starting folder"); folder->setMinimumWidth(100);
    browseButton = new QPushButton("Choose folder…", q); browseButton->setIcon(QIcon::fromTheme("folder-open"));
    scanButton = new QPushButton("Scan with sudo", q); scanButton->setIcon(QIcon::fromTheme("system-search"));
    cancelButton = new QPushButton("Stop", q); cancelButton->setEnabled(false);
    source->addWidget(mounts); source->addWidget(refresh); source->addWidget(folder, 1); source->addWidget(browseButton); source->addWidget(scanButton); source->addWidget(cancelButton);
    layout->addLayout(source);
    auto *options = new QHBoxLayout;
    cross = new QCheckBox("Include other mounted filesystems", q);
    cross->setToolTip("Off: stay on the chosen filesystem, including bind mount boundaries.\nSymbolic links are never followed; kernel virtual filesystems are skipped.");
    options->addWidget(cross); options->addStretch(); options->addWidget(new QLabel("Map size:", q));
    measure = new QComboBox(q); measure->addItem("Space on disk", "allocated"); measure->addItem("Apparent file size", "size");
    measure->setToolTip("Space on disk uses allocated blocks (useful for sparse VM images).\nHard links are counted once. Compressed/reflink filesystems may share blocks.");
    options->addWidget(measure); layout->addLayout(options);
    auto *breadcrumbRow = new QHBoxLayout;
    up = new QPushButton("↑", q); up->setFixedWidth(34); up->setToolTip("Up one level · Alt+Up"); up->setEnabled(false); breadcrumbRow->addWidget(up);
    auto *scroll = new QScrollArea(q); scroll->setFrameShape(QFrame::NoFrame); scroll->setWidgetResizable(true);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded); scroll->setFixedHeight(46);
    auto *crumbWidget = new QWidget(scroll); crumbs = new QHBoxLayout(crumbWidget); crumbs->setContentsMargins(0,0,0,0); crumbs->setSpacing(3); crumbs->addStretch();
    scroll->setWidget(crumbWidget); breadcrumbRow->addWidget(scroll,1); layout->addLayout(breadcrumbRow);
    auto *split = new QSplitter(Qt::Horizontal,q); split->setObjectName("diskSplit"); split->setChildrenCollapsible(false);
    tree = new QTreeWidget(split); tree->setObjectName("diskTree"); tree->setHeaderLabels({"Folder / files", "Size", "%"});
    tree->setMinimumWidth(260); tree->setAlternatingRowColors(true); tree->setUniformRowHeights(true); tree->setIconSize(QSize(20,20)); tree->setIndentation(20);
    tree->setTextElideMode(Qt::ElideMiddle); tree->setContextMenuPolicy(Qt::CustomContextMenu);
    tree->header()->setStretchLastSection(false); tree->header()->setSectionResizeMode(0,QHeaderView::Stretch);
    tree->header()->setSectionResizeMode(1,QHeaderView::Fixed); tree->header()->setSectionResizeMode(2,QHeaderView::Fixed);
    tree->setColumnWidth(1,94); tree->setColumnWidth(2,56);
    auto *visual = new QWidget(split); auto *visualLayout = new QVBoxLayout(visual); visualLayout->setContentsMargins(0,0,0,0); visualLayout->setSpacing(7);
    mapTitle = new QLabel("DISK MAP   /   double-click a folder to explore", visual); mapTitle->setObjectName("muted"); visualLayout->addWidget(mapTitle);
    map = new DiskMap(visual); visualLayout->addWidget(map,1);
    details = new QLabel("Select a tile or tree item to see its path and size.", visual); details->setObjectName("diskDetails"); details->setTextFormat(Qt::PlainText); details->setWordWrap(true); details->setMinimumHeight(52); details->setMaximumHeight(72); details->setTextInteractionFlags(Qt::TextSelectableByMouse); visualLayout->addWidget(details);
    split->addWidget(tree); split->addWidget(visual); split->setStretchFactor(0,2); split->setStretchFactor(1,3); split->setSizes({450,700}); layout->addWidget(split,1);
    auto *actions = new QHBoxLayout;
    openButton = new QPushButton("Open folder",q); openButton->setIcon(QIcon::fromTheme("folder-open"));
    copyButton = new QPushButton("Copy path",q); copyButton->setIcon(QIcon::fromTheme("edit-copy"));
    trashButton = new QPushButton("Send to Trash",q); trashButton->setIcon(QIcon::fromTheme("user-trash"));
    deleteButton = new QPushButton("Delete permanently…",q); deleteButton->setIcon(QIcon::fromTheme("edit-delete"));
    for (auto *button : {openButton,copyButton,trashButton,deleteButton}) { button->setEnabled(false); actions->addWidget(button); }
    actions->addStretch(); layout->addLayout(actions);
    auto *bottom = new QHBoxLayout;
    status = new QLabel("Ready · scans read file metadata; no file contents are opened.",q); status->setTextFormat(Qt::PlainText); status->setWordWrap(true); status->setObjectName("muted"); bottom->addWidget(status,1);
    progress = new QProgressBar(q); progress->setRange(0,0); progress->setFixedSize(130,8); progress->setTextVisible(false); progress->hide(); bottom->addWidget(progress); layout->addLayout(bottom);
    refreshMounts();
    QObject::connect(refresh,&QToolButton::clicked,q,[this]{ if (!busy) refreshMounts(); });
    QObject::connect(mounts,qOverload<int>(&QComboBox::activated),q,[this]{ folder->setText(mounts->currentData().toString()); });
    QObject::connect(browseButton,&QPushButton::clicked,q,[this]{ picker(); });
    QObject::connect(scanButton,&QPushButton::clicked,q,[this]{ startScan(); });
    QObject::connect(folder,&QLineEdit::returnPressed,q,[this]{ startScan(); });
    QObject::connect(cancelButton,&QPushButton::clicked,q,[this]{ send({{"op","cancel"}}, {}); cancelButton->setEnabled(false); status->setText("Stopping the scan…"); });
    QObject::connect(measure,qOverload<int>(&QComboBox::activated),q,[this]{ if (complete) { resetTree(); showView(id(focus)); } });
    QObject::connect(up,&QPushButton::clicked,q,[this]{ if (!focus["parent"].isNull()) showView(qint64(focus["parent"].toDouble())); });
    auto *upKey = new QShortcut(QKeySequence("Alt+Up"),q); upKey->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(upKey,&QShortcut::activated,up,&QPushButton::click);
    QObject::connect(tree,&QTreeWidget::itemExpanded,q,[this](QTreeWidgetItem *item){ if (item->data(0,Qt::UserRole+1).toString() == "unloaded") loadChildren(item,0); });
    QObject::connect(tree,&QTreeWidget::currentItemChanged,q,[this](QTreeWidgetItem *item){ if (item && item->data(0,Qt::UserRole).isValid()) select(item->data(0,Qt::UserRole).toJsonObject()); });
    QObject::connect(tree,&QTreeWidget::itemDoubleClicked,q,[this](QTreeWidgetItem *item){
      if (item->data(0,Qt::UserRole+1).toString() == "more") loadChildren(item->parent(),item->data(1,Qt::UserRole).toInt());
      else { const auto node = item->data(0,Qt::UserRole).toJsonObject(); if (container(node)) showView(id(node)); }
    });
    QObject::connect(tree,&QTreeWidget::customContextMenuRequested,q,[this](QPoint point){ if (auto *item = tree->itemAt(point)) menu(item->data(0,Qt::UserRole).toJsonObject(),tree->viewport()->mapToGlobal(point)); });
    map->choose = [this](QJsonObject node){ select(node); };
    map->enter = [this](QJsonObject node){ showView(id(node)); };
    map->context = [this](QJsonObject node,QPoint point){ menu(node,point); };
    QObject::connect(openButton,&QPushButton::clicked,q,[this]{ openSelection(); });
    QObject::connect(copyButton,&QPushButton::clicked,q,[this]{ QApplication::clipboard()->setText(selection["path"].toString()); });
    QObject::connect(trashButton,&QPushButton::clicked,q,[this]{ removeSelection(false); });
    QObject::connect(deleteButton,&QPushButton::clicked,q,[this]{ removeSelection(true); });
    QObject::connect(&helper,&QProcess::readyReadStandardOutput,q,[this]{
      buffer += helper.readAllStandardOutput();
      int newline;
      while ((newline = buffer.indexOf('\n')) >= 0) {
        const auto reply = QJsonDocument::fromJson(buffer.left(newline)).object(); buffer.remove(0,newline+1);
        if (reply["event"] == "progress") {
          status->setText(QString("Scanning · %1 entries · %2").arg(QLocale().toString(qint64(reply["entries"].toDouble())),reply["path"].toString().right(110)));
        } else {
          const auto callback = pending.take(reply["request"].toInt()); if (callback) callback(reply);
        }
      }
    });
    QObject::connect(&helper,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),q,[this](int code,QProcess::ExitStatus){
      complete = false; setBusy(false); updateActions();
      const auto error = QString::fromUtf8(helper.readAllStandardError()).trimmed();
      const auto callbacks = std::exchange(pending,{});
      for (const auto &callback : callbacks) if (callback) callback({{"ok",false},{"error",error.isEmpty() ? "Disk helper stopped. Start a new scan." : error}});
      if (code) status->setText("Disk helper stopped: " + error.left(350));
    });
  }
  ~Impl() {
    QObject::disconnect(&helper,nullptr,q,nullptr);
    helper.closeWriteChannel(); // EOF cancels the scan and drops root privileges.
    if (helper.state() != QProcess::NotRunning && !helper.waitForFinished(2500)) {
      helper.terminate(); helper.waitForFinished(1000);
    }
    if (trash.state() != QProcess::NotRunning) { trash.terminate(); trash.waitForFinished(1000); }
  }
  QString metric() const { return measure->currentData().toString(); }
  void refreshMounts() {
    const auto previous = mounts->currentData().toString(); mounts->clear();
    auto volumes = QStorageInfo::mountedVolumes();
    std::sort(volumes.begin(),volumes.end(),[](const auto &a,const auto &b){ return a.rootPath() < b.rootPath(); });
    for (const auto &volume : volumes) {
      if (!volume.isValid() || !volume.isReady() || volume.bytesTotal() <= 0) continue;
      const auto path = volume.rootPath();
      if (path == "/proc" || path.startsWith("/proc/") || path == "/sys" || path.startsWith("/sys/") || path == "/dev" || path.startsWith("/dev/") || path.startsWith("/run/user/")) continue;
      mounts->addItem(QIcon::fromTheme("drive-harddisk"),path + " · " + bytes(volume.bytesTotal()),path);
      mounts->setItemData(mounts->count()-1,QString::fromUtf8(volume.device()) + " · " + QString::fromUtf8(volume.fileSystemType()) + "\n" + bytes(volume.bytesAvailable()) + " available",Qt::ToolTipRole);
    }
    auto chosen = mounts->findData(previous.isEmpty() ? QStorageInfo(QDir::homePath()).rootPath() : previous);
    if (chosen >= 0) mounts->setCurrentIndex(chosen);
  }
  bool ensureHelper() {
    if (helper.state() == QProcess::Running) return true;
    if (!fixture && !authenticate()) return false;
    const auto script = CachyUi::toolPath("disk_scan.py");
    if (!QFileInfo::exists(script)) { status->setText("The disk scanner is missing. Reinstall the Control Panel files."); return false; }
    QStringList args{"-I",script};
    QString program = "/usr/bin/python3";
    if (!fixture) { args.prepend(program); args.prepend("-n"); program = "sudo"; }
    buffer.clear(); helper.start(program,args);
    if (!helper.waitForStarted(2500)) { status->setText("Could not start the disk scanner: " + helper.errorString()); return false; }
    return true;
  }
  void send(QJsonObject request, std::function<void(QJsonObject)> callback) {
    const int number = ++sequence; request["request"] = number;
    if (callback) pending[number] = std::move(callback);
    helper.write(QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n");
  }
  void setBusy(bool value) {
    busy = value;
    for (auto *widget : QList<QWidget *>{mounts,folder,browseButton,scanButton,cross,measure,tree,map}) widget->setEnabled(!value);
    cancelButton->setEnabled(value); progress->setVisible(value); updateActions();
  }
  void startScan() {
    if (busy || !ensureHelper()) return;
    const auto path = QDir::cleanPath(folder->text());
    if (!QDir::isAbsolutePath(path)) { status->setText("Choose an absolute folder path."); return; }
    const auto partition = mounts->currentData().toString();
    if (!fixture && partition != "/" && path != partition && !path.startsWith(partition + "/")) {
      status->setText("Choose a starting folder inside the selected mounted partition."); return;
    }
    ++scanSequence; ++selectionSequence; ++viewSequence;
    scannedPath = path; complete = false; root = {}; focus = {}; selection = {}; tree->clear(); items.clear(); map->setMap({},metric());
    clearCrumbs(); up->setEnabled(false); details->setText("Building the disk index…"); summary->setText("Reading filesystem metadata");
    setBusy(true); status->setText("Starting scan…");
    send({{"op","scan"},{"path",path},{"cross",cross->isChecked()}},[this](QJsonObject reply){
      setBusy(false);
      if (!reply["ok"].toBool()) { status->setText(reply["error"].toString()); details->setText("No completed scan. Choose Scan to try again."); summary->setText("Scan not completed"); return; }
      root = reply["root"].toObject(); complete = true;
      const auto errors = reply["errors"].toInt();
      status->setText(QString("%1 entries · %2 s · %3").arg(QLocale().toString(qint64(reply["entries"].toDouble()))).arg(reply["seconds"].toDouble(),0,'f',1)
                       .arg(errors ? QString("%1 unreadable or changed items (hover for details)").arg(errors) : "Scan complete · symlinks are not followed"));
      QStringList samples; for (auto sample : reply["samples"].toArray()) samples << sample.toString(); status->setToolTip(samples.join('\n'));
      summary->setText(bytes(root[metric()].toDouble()) + " · " + QLocale().toString(qint64(root["files"].toDouble())) + " files");
      resetTree(); showView(1);
    });
  }
  QTreeWidgetItem *append(QTreeWidgetItem *parent,const QJsonObject &node) {
    auto *item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(tree);
    item->setText(0,node["name"].toString()); item->setIcon(0,nodeIcon(node)); item->setData(0,Qt::UserRole,node);
    item->setText(1,bytes(node[metric()].toDouble())); item->setTextAlignment(1,Qt::AlignRight | Qt::AlignVCenter);
    const auto total = root[metric()].toDouble();
    item->setText(2,total ? QString::number(node[metric()].toDouble()*100./total,'f',1) + "%" : "—"); item->setTextAlignment(2,Qt::AlignRight | Qt::AlignVCenter);
    item->setToolTip(0,node["path"].toString() + (node["status"].toString().isEmpty() ? "" : "\n" + node["status"].toString()));
    if (node["kind"] == "files") { auto f = item->font(0); f.setItalic(true); item->setFont(0,f); }
    if (!node["status"].toString().isEmpty()) item->setForeground(0,q->palette().color(QPalette::PlaceholderText));
    items[id(node)] = item;
    if (node["children"].toBool()) { item->setData(0,Qt::UserRole+1,"unloaded"); auto *dummy = new QTreeWidgetItem(item); dummy->setText(0,"Loading…"); }
    return item;
  }
  void resetTree() {
    ++scanSequence; ++selectionSequence; ++viewSequence; tree->clear(); items.clear();
    summary->setText(bytes(root[metric()].toDouble()) + " · " + QLocale().toString(qint64(root["files"].toDouble())) + " files");
    auto *top = append(nullptr,root); top->setText(0,scannedPath); top->setExpanded(true);
  }
  void loadChildren(QTreeWidgetItem *item,int offset,std::function<void()> done = {}) {
    if (!complete || busy) return;
    const auto node = item->data(0,Qt::UserRole).toJsonObject(); const auto nodeId = id(node);
    item->setData(0,Qt::UserRole+1,"loading");
    if (!offset) qDeleteAll(item->takeChildren());
    else if (item->childCount() && item->child(item->childCount()-1)->data(0,Qt::UserRole+1).toString() == "more") delete item->takeChild(item->childCount()-1);
    const auto measureAtStart = metric();
    const int scan = scanSequence;
    send({{"op","children"},{"node",nodeId},{"metric",metric()},{"offset",offset}},[this,nodeId,measureAtStart,scan,done](QJsonObject reply){
      auto *parent = items.value(nodeId); if (!parent || metric() != measureAtStart || scan != scanSequence) return;
      parent->setData(0,Qt::UserRole+1,"loaded");
      if (!reply["ok"].toBool()) { status->setText(reply["error"].toString()); return; }
      for (auto node : reply["nodes"].toArray()) append(parent,node.toObject());
      if (reply["more"].toBool()) {
        auto *more = new QTreeWidgetItem(parent); more->setText(0,"Load more… (double-click)"); more->setData(0,Qt::UserRole+1,"more"); more->setData(1,Qt::UserRole,reply["offset"].toInt());
      }
      if (done) done();
    });
  }
  void clearCrumbs() {
    while (auto *item = crumbs->takeAt(0)) { if (item->widget()) item->widget()->deleteLater(); delete item; }
  }
  void showView(qint64 nodeId) {
    if (!complete || busy || !nodeId) return;
    const int generation = ++viewSequence;
    send({{"op","view"},{"node",nodeId},{"metric",metric()}},[this,generation](QJsonObject reply){
      if (generation != viewSequence) return;
      if (!reply["ok"].toBool()) { status->setText(reply["error"].toString()); return; }
      focus = reply["map"].toObject(); select(focus); map->setMap(focus,metric());
      mapTitle->setText("DISK MAP   /   " + bytes(focus[metric()].toDouble()) + "   /   double-click to explore");
      clearCrumbs();
      for (auto value : reply["crumbs"].toArray()) {
        const auto node = value.toObject();
        auto *button = new QPushButton(node["parent"].isNull() ? scannedPath : node["name"].toString(),q); button->setToolTip(node["path"].toString()); button->setMaximumWidth(260);
        button->setCheckable(true); button->setChecked(id(node) == id(focus)); crumbs->addWidget(button);
        QObject::connect(button,&QPushButton::clicked,q,[this,node]{ showView(id(node)); });
        auto *separator = new QLabel("›",q); separator->setObjectName("muted"); crumbs->addWidget(separator);
      }
      crumbs->addStretch(); up->setEnabled(!focus["parent"].isNull());
      emit q->viewReady();
    });
  }
  void select(QJsonObject node) {
    if (!node.contains("id")) return;
    selection = node; map->selected = id(node); map->update();
    const int generation = ++selectionSequence;
    if (auto *item = items.value(id(node))) { QSignalBlocker block(tree); tree->setCurrentItem(item); tree->scrollToItem(item); }
    else if (complete && !busy) {
      send({{"op","trail"},{"node",id(node)}},[this,generation](QJsonObject reply){
        if (generation == selectionSequence && reply["ok"].toBool()) revealTree(reply["crumbs"].toArray(),0,generation);
      });
    }
    details->setText(node["path"].toString() + (node["kind"] == "files" ? " / Files" : "") + "\n" +
        bytes(node["allocated"].toDouble()) + " on disk  ·  " + bytes(node["size"].toDouble()) + " apparent" +
        (node["files"].toDouble() > 1 ? "  ·  " + QLocale().toString(qint64(node["files"].toDouble())) + " files" : "") +
        (node["status"].toString().isEmpty() ? "" : "  ·  " + node["status"].toString()));
    updateActions();
  }
  void revealTree(QJsonArray trail,int position,int generation) {
    if (generation != selectionSequence || position >= trail.size()) return;
    const auto node = trail[position].toObject();
    auto *item = items.value(id(node)); if (!item) return;
    if (position == trail.size()-1) {
      QSignalBlocker block(tree); tree->setCurrentItem(item); tree->scrollToItem(item); return;
    }
    const auto state = item->data(0,Qt::UserRole+1).toString();
    { QSignalBlocker block(tree); item->setExpanded(true); }
    if (state == "unloaded") loadChildren(item,0,[this,trail,position,generation]{ revealTree(trail,position+1,generation); });
    else if (state == "loaded") revealTree(trail,position+1,generation);
  }
  void updateActions() {
    const bool available = complete && !busy && selection.contains("id");
    openButton->setEnabled(available); copyButton->setEnabled(available);
    const bool removable = available && id(selection) > 1 && selection["kind"] != "remainder";
    trashButton->setEnabled(removable && selection["utf8"].toBool()); deleteButton->setEnabled(removable);
  }
  void openSelection() {
    const auto path = selection["path"].toString();
    FileManager::openFolder(q,container(selection) ? path : QFileInfo(path).absolutePath());
  }
  void menu(QJsonObject node,QPoint position) {
    if (!node.contains("id") || busy) return;
    select(node); QMenu menu(q);
    if (container(node)) { auto *zoom = menu.addAction(QIcon::fromTheme("zoom-in"),"Explore this folder"); QObject::connect(zoom,&QAction::triggered,q,[this,node]{ showView(id(node)); }); }
    auto *open = menu.addAction(openButton->icon(),"Open containing folder"); QObject::connect(open,&QAction::triggered,openButton,&QPushButton::click);
    auto *copy = menu.addAction(copyButton->icon(),"Copy path"); QObject::connect(copy,&QAction::triggered,copyButton,&QPushButton::click);
    menu.addSeparator();
    auto *toTrash = menu.addAction(trashButton->icon(),"Send to Trash"); toTrash->setEnabled(trashButton->isEnabled()); QObject::connect(toTrash,&QAction::triggered,trashButton,&QPushButton::click);
    auto *remove = menu.addAction(deleteButton->icon(),"Delete permanently…"); remove->setEnabled(deleteButton->isEnabled()); QObject::connect(remove,&QAction::triggered,deleteButton,&QPushButton::click);
    menu.exec(position);
  }
  void removeSelection(bool permanent) {
    if (busy || !complete) return;
    const auto node = selection;
    send({{"op","validate"},{"node",id(node)}},[this,node,permanent](QJsonObject reply){
      if (!reply["ok"].toBool()) { QMessageBox::warning(q,"Cannot remove this item",reply["error"].toString()); return; }
      QMessageBox question(QMessageBox::Warning,permanent ? "Delete permanently" : "Send to Trash",
          (permanent ? "Permanently delete this item and everything inside it?\n\n" : "Move this item and its contents to your Trash?\n\n") + node["path"].toString() +
          "\n\n" + bytes(node["allocated"].toDouble()) + " on disk" + (permanent ? "\nThis cannot be undone." : "\nIf Trash is unavailable, the item will be left in place."),QMessageBox::Cancel,q);
      question.setTextFormat(Qt::PlainText);
      auto *accept = question.addButton(permanent ? "Delete permanently" : "Send to Trash",QMessageBox::DestructiveRole); question.setDefaultButton(QMessageBox::Cancel);
      question.exec(); if (question.clickedButton() != accept) return;
      ++scanSequence; ++selectionSequence; ++viewSequence;
      setBusy(true); cancelButton->setEnabled(false); status->setText(permanent ? "Deleting selected item…" : "Moving selected item to Trash…");
      auto rescan = [this](QString error) {
        setBusy(false);
        if (!error.isEmpty()) QMessageBox::warning(q,"File operation",error + "\n\nThe map will be rescanned to show the current contents.");
        folder->setText(scannedPath); startScan();
      };
      if (permanent) {
        send({{"op","delete"},{"node",id(node)},{"view",id(focus)}},[this,rescan](QJsonObject result){
          if (!result["ok"].toBool()) { rescan(result["error"].toString()); return; }
          root = result["root"].toObject(); selection = {}; focus = {};
          map->setMap({},metric()); clearCrumbs(); up->setEnabled(false);
          setBusy(false);
          status->setText(QString("%1 entries · Item deleted · Map updated without rescanning")
                           .arg(QLocale().toString(qint64(result["entries"].toDouble()))));
          resetTree(); showView(qint64(result["view"].toDouble()));
        });
      } else {
        // Trash may move the bytes into another folder inside the scanned
        // tree. Subtracting them would incorrectly report reclaimed space.
        const auto gio = QStandardPaths::findExecutable("gio");
        if (gio.isEmpty()) { rescan("The gio Trash helper is unavailable. Install glib2; nothing was deleted."); return; }
        QObject::disconnect(&trash,nullptr,q,nullptr);
        QObject::connect(&trash,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),q,[this,rescan](int code,QProcess::ExitStatus state){
          rescan(code == 0 && state == QProcess::NormalExit ? QString() : "Could not move this item to your Trash. No permanent-delete fallback was used.\n" + QString::fromUtf8(trash.readAllStandardError()));
        });
        QObject::connect(&trash,&QProcess::errorOccurred,q,[rescan](QProcess::ProcessError error){ if (error == QProcess::FailedToStart) rescan("Could not start the Trash helper. Nothing was deleted."); });
        trash.start(gio,{"trash","--",node["path"].toString()}); // Always the desktop user, never root's Trash.
      }
    });
  }
  void picker() {
    if (busy || !ensureHelper()) return;
    QDialog dialog(q); dialog.setWindowTitle("Choose where to scan"); dialog.resize(830,550);
    auto *layout = new QVBoxLayout(&dialog);
    auto *intro = new QLabel("Choose a folder on the mounted partition. Protected folders are listed with sudo.",&dialog); intro->setWordWrap(true); layout->addWidget(intro);
    auto *path = new QLineEdit(folder->text(),&dialog); layout->addWidget(path);
    auto *folders = new QTreeWidget(&dialog); folders->setHeaderLabels({"Folders"}); folders->setObjectName("diskFolderPicker"); folders->setIconSize(QSize(22,22)); layout->addWidget(folders,1);
    auto *error = new QLabel(&dialog); error->setWordWrap(true); layout->addWidget(error);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,&dialog); buttons->button(QDialogButtonBox::Ok)->setText("Use this folder"); layout->addWidget(buttons);
    QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    QPointer<QDialog> alive(&dialog);
    const auto partition = mounts->currentData().toString();
    auto load = [this,alive,folders,error,path](QTreeWidgetItem *item) {
      if (item->data(0,Qt::UserRole+1).toBool()) return;
      item->setData(0,Qt::UserRole+1,true);
      const auto folderPath = item->data(0,Qt::UserRole).toString();
      send({{"op","browse"},{"path",folderPath}},[alive,folders,error,path,item](QJsonObject reply){
        if (!alive) return;
        qDeleteAll(item->takeChildren());
        if (!reply["ok"].toBool()) { error->setText(reply["error"].toString()); item->setData(0,Qt::UserRole+1,false); return; }
        error->clear();
        for (auto value : reply["folders"].toArray()) {
          const auto data = value.toObject(); auto *child = new QTreeWidgetItem(item); child->setText(0,data["name"].toString()); child->setIcon(0,QIcon::fromTheme("folder")); child->setData(0,Qt::UserRole,data["path"].toString());
          child->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
          const auto target = path->text();
          if (target == data["path"].toString() || target.startsWith(data["path"].toString() + "/")) {
            child->setExpanded(true);
            if (target == data["path"].toString()) { folders->setCurrentItem(child); folders->scrollToItem(child); }
          }
        }
      });
    };
    QObject::connect(folders,&QTreeWidget::itemExpanded,&dialog,load);
    QObject::connect(folders,&QTreeWidget::itemClicked,&dialog,[path](QTreeWidgetItem *item){ path->setText(item->data(0,Qt::UserRole).toString()); });
    auto *top = new QTreeWidgetItem(folders); top->setText(0,partition); top->setIcon(0,QIcon::fromTheme("drive-harddisk")); top->setData(0,Qt::UserRole,partition); top->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator); top->setExpanded(true);
    QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,[this,alive,path,error,&dialog,partition]{
      const auto target = QDir::cleanPath(path->text());
      if (!QDir::isAbsolutePath(target) || (partition != "/" && target != partition && !target.startsWith(partition + "/"))) { error->setText("Choose a folder inside " + partition); return; }
      send({{"op","browse"},{"path",target}},[alive,this,error,target,&dialog](QJsonObject result){
        if (!alive) return;
        if (!result["ok"].toBool()) { error->setText(result["error"].toString()); return; }
        folder->setText(target); dialog.accept();
      });
    });
    dialog.exec();
  }
};

DiskVisualizer::DiskVisualizer(std::function<bool()> authenticate,QWidget *parent)
    : QWidget(parent), d(std::make_unique<Impl>(this,std::move(authenticate))) {
  setObjectName("diskVisualizerTab");
}
DiskVisualizer::~DiskVisualizer() = default;
void DiskVisualizer::scanFixture(const QString &path) {
  d->fixture = true; d->folder->setText(path); d->startScan();
}
