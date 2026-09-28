#include "window_enum.h"
#include "window_enum_backend.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollBar>
#include <QShowEvent>
#include <QSortFilterProxyModel>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QTableView>
#include <QTextStream>
#include <QUrl>
#include <QVBoxLayout>

namespace {
struct Column { const char *key; const char *label; int width; };
const QList<Column> columns{
    {"title", QT_TRANSLATE_NOOP("WindowEnumTab", "Window name"), 260},
    {"class", QT_TRANSLATE_NOOP("WindowEnumTab", "Class"), 160},
    {"executable", QT_TRANSLATE_NOOP("WindowEnumTab", "Executable path"), 300},
    {"pid", QT_TRANSLATE_NOOP("WindowEnumTab", "PID"), 80},
    {"type", QT_TRANSLATE_NOOP("WindowEnumTab", "Type"), 180},
    {"x", "X", 75}, {"y", "Y", 75}, {"width", QT_TRANSLATE_NOOP("WindowEnumTab", "Width"), 80},
    {"height", QT_TRANSLATE_NOOP("WindowEnumTab", "Height"), 80},
    {"above", QT_TRANSLATE_NOOP("WindowEnumTab", "Topmost"), 90},
    {"below", QT_TRANSLATE_NOOP("WindowEnumTab", "Keep below"), 100},
    {"minimized", QT_TRANSLATE_NOOP("WindowEnumTab", "Minimized"), 95},
    {"maximized", QT_TRANSLATE_NOOP("WindowEnumTab", "Maximized"), 95},
    {"fullscreen", QT_TRANSLATE_NOOP("WindowEnumTab", "Fullscreen"), 95},
    {"opacity", QT_TRANSLATE_NOOP("WindowEnumTab", "Opacity %"), 95},
    {"id", QT_TRANSLATE_NOOP("WindowEnumTab", "Window ID"), 210},
    {"instance", QT_TRANSLATE_NOOP("WindowEnumTab", "Instance"), 160},
    {"appId", QT_TRANSLATE_NOOP("WindowEnumTab", "Application ID"), 220},
    {"backend", QT_TRANSLATE_NOOP("WindowEnumTab", "Backend"), 90},
    {"desktop", QT_TRANSLATE_NOOP("WindowEnumTab", "Workspace"), 100},
    {"monitor", QT_TRANSLATE_NOOP("WindowEnumTab", "Monitor"), 140},
    {"active", QT_TRANSLATE_NOOP("WindowEnumTab", "Active"), 80},
    {"mapped", QT_TRANSLATE_NOOP("WindowEnumTab", "Mapped"), 80},
    {"visibility", QT_TRANSLATE_NOOP("WindowEnumTab", "Visibility"), 100},
    {"hidden", QT_TRANSLATE_NOOP("WindowEnumTab", "Hidden"), 80},
    {"managed", QT_TRANSLATE_NOOP("WindowEnumTab", "Managed"), 85},
    {"overrideRedirect", QT_TRANSLATE_NOOP("WindowEnumTab", "Override redirect"), 140},
    {"inputOnly", QT_TRANSLATE_NOOP("WindowEnumTab", "Input only"), 100},
    {"parent", QT_TRANSLATE_NOOP("WindowEnumTab", "Parent"), 150},
    {"transientFor", QT_TRANSLATE_NOOP("WindowEnumTab", "Transient for"), 180},
    {"stack", QT_TRANSLATE_NOOP("WindowEnumTab", "Stack order"), 95},
    {"siblingStack", QT_TRANSLATE_NOOP("WindowEnumTab", "Sibling stack"), 110},
    {"role", QT_TRANSLATE_NOOP("WindowEnumTab", "Role"), 140},
    {"decorated", QT_TRANSLATE_NOOP("WindowEnumTab", "Decorated"), 95},
    {"skipTaskbar", QT_TRANSLATE_NOOP("WindowEnumTab", "Skip taskbar"), 110},
    {"skipPager", QT_TRANSLATE_NOOP("WindowEnumTab", "Skip pager"), 100},
    {"skipSwitcher", QT_TRANSLATE_NOOP("WindowEnumTab", "Skip switcher"), 115},
    {"modal", QT_TRANSLATE_NOOP("WindowEnumTab", "Modal"), 80},
    {"attention", QT_TRANSLATE_NOOP("WindowEnumTab", "Attention"), 95},
    {"user", QT_TRANSLATE_NOOP("WindowEnumTab", "Owner"), 120},
    {"uid", "UID", 80},
    {"pidSource", QT_TRANSLATE_NOOP("WindowEnumTab", "PID source"), 190},
    {"command", QT_TRANSLATE_NOOP("WindowEnumTab", "Command line"), 350},
    {"machine", QT_TRANSLATE_NOOP("WindowEnumTab", "Client machine"), 150},
    {"frameX", QT_TRANSLATE_NOOP("WindowEnumTab", "Frame X"), 90},
    {"frameY", QT_TRANSLATE_NOOP("WindowEnumTab", "Frame Y"), 90},
    {"frameWidth", QT_TRANSLATE_NOOP("WindowEnumTab", "Frame width"), 105},
    {"frameHeight", QT_TRANSLATE_NOOP("WindowEnumTab", "Frame height"), 105},
    {"border", QT_TRANSLATE_NOOP("WindowEnumTab", "Border width"), 110},
    {"depth", QT_TRANSLATE_NOOP("WindowEnumTab", "Depth"), 75},
    {"states", QT_TRANSLATE_NOOP("WindowEnumTab", "State flags"), 300},
    {"protocols", QT_TRANSLATE_NOOP("WindowEnumTab", "Protocols"), 250},
    {"allowedActions", QT_TRANSLATE_NOOP("WindowEnumTab", "Allowed actions"), 300},
    {"activities", QT_TRANSLATE_NOOP("WindowEnumTab", "Activities"), 220},
    {"geometryUnits", QT_TRANSLATE_NOOP("WindowEnumTab", "Geometry coordinates"), 300},
    {"capabilities", QT_TRANSLATE_NOOP("WindowEnumTab", "Available controls"), 300},
};

void propertiesDialog(QWidget *parent, const QJsonObject &window) {
    auto *dialog = new QDialog(parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(QObject::tr("Window properties — %1").arg(window["title"].toString()));
    dialog->resize(880, 650);
    auto *layout = new QVBoxLayout(dialog);
    auto *text = new QPlainTextEdit(QString::fromUtf8(QJsonDocument(window).toJson(QJsonDocument::Indented)), dialog);
    text->setReadOnly(true);
    text->setLineWrapMode(QPlainTextEdit::NoWrap);
    layout->addWidget(text);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
    QObject::connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::close);
    layout->addWidget(buttons);
    dialog->show();
}
}

WindowEnumTab::WindowEnumTab(QWidget *parent) : QWidget(parent)
{
    setObjectName("windowEnumSubTab");
    controller = new WindowEnumController(this);
    auto *layout = new QVBoxLayout(this);
    auto *toolbar = new QHBoxLayout();
    auto *refreshButton = new QPushButton(tr("Refresh"), this);
    live = new QCheckBox(tr("Live"), this);
    live->setChecked(true);
    live->setToolTip(tr("Refresh every two seconds while this tab is visible."));
    source = new QComboBox(this);
    source->addItem(tr("Automatic"), "auto");
    source->addItem(tr("X11 / XWayland — full tree"), "x11");
    source->addItem(tr("KDE — compositor windows"), "kwin");
    source->addItem(tr("GNOME — compositor windows"), "gnome");
    source->addItem(tr("Wayland — foreign toplevels"), "wayland");
    search = new QLineEdit(this);
    search->setPlaceholderText(tr("Filter any column…"));
    search->setClearButtonEnabled(true);
    auto *exportButton = new QPushButton(tr("Export…"), this);
    gnomeButton = new QPushButton(tr("Enable GNOME Bridge"), this);
    gnomeButton->setToolTip(tr("Installs this application's GNOME Shell extension for your user and enables it. A new extension may require logging out and back in."));
    gnomeButton->setVisible(qEnvironmentVariable("XDG_CURRENT_DESKTOP").contains("gnome", Qt::CaseInsensitive));
    toolbar->addWidget(refreshButton);
    toolbar->addWidget(live);
    toolbar->addWidget(source);
    toolbar->addWidget(search, 1);
    toolbar->addWidget(gnomeButton);
    toolbar->addWidget(exportButton);
    layout->addLayout(toolbar);
    coverage = new QLabel(tr("Open this tab to enumerate windows. — means the desktop did not expose that field."), this);
    coverage->setWordWrap(true);
    coverage->setTextFormat(Qt::PlainText);
    layout->addWidget(coverage);

    table = new QTableView(this);
    table->setObjectName("windowEnumTable");
    model = new QStandardItemModel(0, columns.size(), this);
    QStringList headers;
    for (const auto &column : columns) headers << tr(column.label);
    model->setHorizontalHeaderLabels(headers);
    proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(model);
    proxy->setFilterKeyColumn(-1);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxy->setSortRole(Qt::UserRole + 1);
    table->setModel(proxy);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setSortingEnabled(true);
    table->setWordWrap(false);
    table->verticalHeader()->hide();
    table->horizontalHeader()->setSectionsMovable(true);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    for (int i = 0; i < columns.size(); ++i) table->setColumnWidth(i, columns[i].width);
    table->sortByColumn(0, Qt::AscendingOrder);
    table->setContextMenuPolicy(Qt::CustomContextMenu);
    table->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(table->horizontalHeader(), &QWidget::customContextMenuRequested, this, [this](const QPoint &point) {
        QMenu menu(this);
        menu.addAction(tr("Show all columns"), this, [this]() {
            for (int i = 0; i < columns.size(); ++i) table->setColumnHidden(i, false);
        });
        menu.addSeparator();
        for (int i = 0; i < columns.size(); ++i) {
            auto *action = menu.addAction(tr(columns[i].label));
            action->setCheckable(true);
            action->setChecked(!table->isColumnHidden(i));
            connect(action, &QAction::toggled, this, [this, i](bool checked) { table->setColumnHidden(i, !checked); });
        }
        menu.exec(table->horizontalHeader()->mapToGlobal(point));
    });
    layout->addWidget(table, 1);
    selectionLabel = new QLabel(tr("Select a window to inspect or change it."), this);
    selectionLabel->setTextFormat(Qt::PlainText);
    selectionLabel->setWordWrap(true);
    layout->addWidget(selectionLabel);
    auto *controls = new QHBoxLayout();
    auto spin = [this, controls](const QString &label, int low, int high) {
        controls->addWidget(new QLabel(label, this));
        auto *box = new QSpinBox(this);
        box->setRange(low, high);
        box->setMinimumWidth(85);
        controls->addWidget(box);
        connect(box, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
            if (!filling) geometryDirty = true;
        });
        return box;
    };
    x = spin("X", -32768, 32767);
    y = spin("Y", -32768, 32767);
    width = spin(tr("Width"), 1, 65535);
    height = spin(tr("Height"), 1, 65535);
    geometry = new QPushButton(tr("Apply geometry"), this);
    actions = new QPushButton(tr("Window actions ▾"), this);
    details = new QPushButton(tr("All properties"), this);
    controls->addWidget(geometry);
    controls->addStretch();
    controls->addWidget(actions);
    controls->addWidget(details);
    layout->addLayout(controls);
    status = new QLabel(this);
    status->setTextFormat(Qt::PlainText);
    status->setWordWrap(true);
    layout->addWidget(status);

    connect(refreshButton, &QPushButton::clicked, this, &WindowEnumTab::refresh);
    connect(search, &QLineEdit::textChanged, proxy, &QSortFilterProxyModel::setFilterFixedString);
    connect(exportButton, &QPushButton::clicked, this, &WindowEnumTab::exportRows);
    connect(gnomeButton, &QPushButton::clicked, this, &WindowEnumTab::installGnomeBridge);
    connect(source, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        controller->setBackend(source->currentData().toString());
        rows = {};
        model->removeRows(0, model->rowCount());
        selectedId.clear();
        gnomeButton->setVisible(source->currentData() == "gnome" ||
            qEnvironmentVariable("XDG_CURRENT_DESKTOP").contains("gnome", Qt::CaseInsensitive));
        updateSelection();
        refresh();
    });
    connect(controller, &WindowEnumController::result, this, &WindowEnumTab::populate);
    connect(controller, &WindowEnumController::busyChanged, this, [this, refreshButton](bool busy) {
        refreshButton->setEnabled(!busy);
        updateSelection();
    });
    connect(table->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
        if (!filling) updateSelection();
    });
    connect(table, &QTableView::doubleClicked, this, &WindowEnumTab::showDetails);
    connect(details, &QPushButton::clicked, this, &WindowEnumTab::showDetails);
    connect(actions, &QPushButton::clicked, this, &WindowEnumTab::actionMenu);
    connect(table, &QWidget::customContextMenuRequested, this, [this](const QPoint &point) {
        const auto index = table->indexAt(point);
        if (index.isValid()) table->selectRow(index.row());
        actionMenu();
    });
    connect(geometry, &QPushButton::clicked, this, [this]() {
        const auto row = selected();
        if (row.isEmpty()) return;
        QJsonObject request{{"id", row["id"]}, {"pid", row["pid"]}, {"action", "geometry"}};
        request["x"] = x->value(); request["y"] = y->value();
        request["width"] = width->value(); request["height"] = height->value();
        request["move"] = row["capabilities"].toArray().contains("move");
        request["resize"] = row["capabilities"].toArray().contains("resize");
        geometryDirty = false;
        controller->request(request);
    });
    timer.setInterval(2000);
    connect(&timer, &QTimer::timeout, this, [this]() {
        if (live->isChecked() && isVisible() && !window()->isMinimized()) refresh();
    });
    timer.start();
    updateSelection();
}

void WindowEnumTab::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    QTimer::singleShot(0, this, &WindowEnumTab::refresh);
}

void WindowEnumTab::refresh()
{
    if (!controller->busy()) {
        status->setText(tr("Reading windows…"));
        controller->request({{"action", "list"}});
    }
}

QJsonObject WindowEnumTab::selected() const
{
    const auto indexes = table->selectionModel()->selectedRows();
    if (indexes.isEmpty()) return {};
    const auto index = proxy->mapToSource(indexes.first());
    return model->item(index.row(), 0)->data(Qt::UserRole).toJsonObject();
}

void WindowEnumTab::populate(const QJsonObject &result)
{
    if (result.contains("error")) {
        status->setText(result["error"].toString());
        live->setChecked(false);
        // Never leave stale rows actionable after a failed/disconnected collector.
        rows = {};
        model->removeRows(0, model->rowCount());
        updateSelection();
        return;
    }
    if (result.contains("window")) { propertiesDialog(this, result["window"].toObject()); return; }
    if (result.contains("ok")) {
        status->setText(result["message"].toString());
        QTimer::singleShot(350, this, &WindowEnumTab::refresh);
        return;
    }
    if (!result.contains("windows")) return;
    const QString id = selected()["id"].toString();
    const int scroll = table->verticalScrollBar()->value();
    filling = true;
    rows = result["windows"].toArray();
    table->setUpdatesEnabled(false);
    proxy->setDynamicSortFilter(false);
    model->removeRows(0, model->rowCount());
    int selectedRow = -1;
    for (const auto &value : rows) {
        const auto row = value.toObject();
        QList<QStandardItem *> items;
        for (const auto &column : columns) {
            auto data = row.value(column.key);
            QString text = windowEnumCell(data);
            if (QString::fromLatin1(column.key) == "desktop" && data.isDouble())
                text = data.toInt() < 0 ? tr("All") : QString::number(data.toInt() + 1);
            auto *item = new QStandardItem(text);
            item->setToolTip(text == "—" ? tr("Not exposed by this window or compositor.") : text);
            item->setData(data.isArray() || data.isObject() ? QVariant(text) : data.toVariant(), Qt::UserRole + 1);
            items.append(item);
        }
        items[0]->setData(row, Qt::UserRole);
        if (row["id"].toString() == id) selectedRow = model->rowCount();
        model->appendRow(items);
    }
    proxy->setDynamicSortFilter(true);
    proxy->sort(table->horizontalHeader()->sortIndicatorSection(), table->horizontalHeader()->sortIndicatorOrder());
    if (selectedRow >= 0) {
        const auto index = proxy->mapFromSource(model->index(selectedRow, 0));
        if (index.isValid()) table->selectRow(index.row());
    }
    table->verticalScrollBar()->setValue(scroll);
    table->setUpdatesEnabled(true);
    filling = false;
    coverage->setText(result["coverage"].toString() + tr("  — = unavailable."));
    status->setText(tr("%1 windows • %2 matching the filter").arg(rows.size()).arg(proxy->rowCount()));
    updateSelection();
}

void WindowEnumTab::updateSelection()
{
    if (filling) return;
    const auto row = selected();
    const auto caps = row["capabilities"].toArray();
    const bool available = !row.isEmpty() && !controller->busy();
    const bool canMove = !row.isEmpty() && caps.contains("move");
    const bool canResize = !row.isEmpty() && caps.contains("resize");
    x->setEnabled(canMove); y->setEnabled(canMove);
    width->setEnabled(canResize); height->setEnabled(canResize);
    geometry->setEnabled(available && (canMove || canResize));
    details->setEnabled(available);
    actions->setEnabled(available);
    if (row.isEmpty()) {
        selectionLabel->setText(tr("Select a window to inspect or change it."));
        selectedId.clear();
        geometryDirty = false;
        return;
    }
    selectionLabel->setText(tr("%1  •  %2").arg(row["title"].toString(), row["geometryUnits"].toString()));
    if (selectedId != row["id"].toString() || !geometryDirty) {
        filling = true;
        x->setValue(qRound(row["x"].toDouble())); y->setValue(qRound(row["y"].toDouble()));
        width->setValue(qRound(row["width"].toDouble())); height->setValue(qRound(row["height"].toDouble()));
        filling = false;
        geometryDirty = false;
    }
    selectedId = row["id"].toString();
}

void WindowEnumTab::act(const QString &name, const QJsonValue &value)
{
    const auto row = selected();
    if (row.isEmpty() || controller->busy()) return;
    QJsonObject request{{"id", row["id"]}, {"pid", row["pid"]}, {"action", name}};
    if (!value.isUndefined() && !value.isNull()) request["value"] = value;
    controller->request(request);
}

void WindowEnumTab::actionMenu()
{
    const auto row = selected();
    if (row.isEmpty() || controller->busy()) return;
    const auto caps = row["capabilities"].toArray();
    QMenu menu(this);
    menu.setToolTipsVisible(true);
    auto add = [&](const QString &label, const QString &key, bool toggle = false) {
        auto *action = menu.addAction(label);
        action->setEnabled(caps.contains(key));
        if (!caps.contains(key)) action->setToolTip(tr("Not supported for this window by the current desktop."));
        if (toggle) { action->setCheckable(true); action->setChecked(row[key].toBool()); }
        connect(action, &QAction::triggered, this, [this, key, toggle, row]() {
            act(key, toggle ? QJsonValue(!row[key].toBool()) : QJsonValue());
        });
    };
    add(tr("Activate / focus"), "activate");
    add(tr("Raise"), "raise");
    add(tr("Lower"), "lower");
    menu.addSeparator();
    add(tr("Always on top"), "above", true);
    add(tr("Keep below"), "below", true);
    add(tr("Minimized"), "minimized", true);
    add(tr("Maximized"), "maximized", true);
    add(tr("Fullscreen"), "fullscreen", true);
    add(tr("Window decorations"), "decorated", true);
    auto *opacity = menu.addAction(tr("Set opacity…"));
    opacity->setEnabled(caps.contains("opacity"));
    connect(opacity, &QAction::triggered, this, [this, row]() {
        bool ok;
        int value = QInputDialog::getInt(this, tr("Window opacity"), tr("Opacity (%)"), row["opacity"].toInt(100), 0, 100, 1, &ok);
        if (ok) act("opacity", value);
    });
    auto *desktop = menu.addAction(tr("Move to workspace…"));
    desktop->setEnabled(caps.contains("desktop"));
    connect(desktop, &QAction::triggered, this, [this, row]() {
        bool ok;
        int value = QInputDialog::getInt(this, tr("Workspace"), tr("Workspace number (0 = all)"),
            row["desktop"].toInt() + 1, 0, 1000, 1, &ok);
        if (ok) act("desktop", value - 1);
    });
    auto *title = menu.addAction(tr("Set window title…"));
    title->setEnabled(caps.contains("title"));
    connect(title, &QAction::triggered, this, [this, row]() {
        bool ok;
        const QString value = QInputDialog::getText(this, tr("Window title"), tr("Title"), QLineEdit::Normal, row["title"].toString(), &ok);
        if (ok) act("title", value);
    });
    menu.addSeparator();
    menu.addAction(tr("All properties"), this, &WindowEnumTab::showDetails);
    menu.addAction(tr("Copy row"), this, [row]() {
        QApplication::clipboard()->setText(QString::fromUtf8(QJsonDocument(row).toJson(QJsonDocument::Indented)));
    });
    auto *path = menu.addAction(tr("Open executable folder"));
    path->setEnabled(!row["executable"].toString().isEmpty());
    connect(path, &QAction::triggered, this, [row]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(row["executable"].toString()).absolutePath()));
    });
    menu.addSeparator();
    add(tr("Close window"), "close");
    timer.stop();
    menu.exec(QCursor::pos());
    timer.start();
}

void WindowEnumTab::showDetails()
{
    auto row = selected();
    if (row.isEmpty()) return;
    if (row["backend"] == "X11") {
        controller->request({{"action", "details"}, {"id", row["id"]}});
    } else propertiesDialog(this, row);
}

void WindowEnumTab::exportRows()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("Export matching windows"),
        QDir::homePath() + "/windows.json", tr("JSON (*.json);;CSV (*.csv)"));
    if (path.isEmpty()) return;
    QJsonArray exported;
    for (int i = 0; i < proxy->rowCount(); ++i) {
        const auto index = proxy->mapToSource(proxy->index(i, 0));
        exported.append(model->item(index.row(), 0)->data(Qt::UserRole).toJsonObject());
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) { status->setText(file.errorString()); return; }
    QByteArray bytes;
    if (path.endsWith(".csv", Qt::CaseInsensitive)) {
        auto quote = [](QString text) {
            // Titles and command lines can contain spreadsheet formulas.
            if (!text.isEmpty() && QString("=+-@\t\r").contains(text[0])) text.prepend('\'');
            return "\"" + text.replace("\"", "\"\"") + "\"";
        };
        QStringList header;
        for (const auto &column : columns) header << quote(tr(column.label));
        bytes += header.join(',').toUtf8() + "\r\n";
        for (const auto &entry : exported) {
            QStringList values;
            for (const auto &column : columns) values << quote(windowEnumCell(entry.toObject().value(column.key)));
            bytes += values.join(',').toUtf8() + "\r\n";
        }
    } else bytes = QJsonDocument(exported).toJson(QJsonDocument::Indented);
    if (file.write(bytes) != bytes.size() || !file.commit()) status->setText(file.errorString());
    else status->setText(tr("Exported %1 windows to %2").arg(exported.size()).arg(path));
}

void WindowEnumTab::installGnomeBridge()
{
    const QString uuid = "window-enum@cachyostools.xetal.net";
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + "/gnome-shell/extensions/" + uuid;
    if (!QDir().mkpath(directory)) { status->setText(tr("Could not create the GNOME extension directory.")); return; }
    for (const QString &name : {QString("metadata.json"), QString("extension.js")}) {
        QFile input(":/window_enum/gnome/" + name);
        QSaveFile output(directory + "/" + name);
        if (!input.open(QIODevice::ReadOnly) || !output.open(QIODevice::WriteOnly)) {
            status->setText(tr("Could not install the GNOME bridge.")); return;
        }
        const auto contents = input.readAll();
        if (output.write(contents) != contents.size() || !output.commit()) {
            status->setText(output.errorString()); return;
        }
    }
    auto *process = new QProcess(this);
    gnomeButton->setEnabled(false);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
        [this, process](int code, QProcess::ExitStatus) {
        gnomeButton->setEnabled(true);
        if (code == 0) { status->setText(tr("GNOME bridge enabled.")); refresh(); }
        else status->setText(tr("GNOME bridge installed. Log out and back in, enable CachyOsTools Window Enum in GNOME Extensions, then refresh. %1")
            .arg(QString::fromUtf8(process->readAllStandardError()).trimmed()));
        process->deleteLater();
    });
    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart) return;
        gnomeButton->setEnabled(true);
        status->setText(tr("Bridge installed. Enable CachyOsTools Window Enum in GNOME Extensions after logging out and back in."));
        process->deleteLater();
    });
    QTimer::singleShot(10000, process, [process]() { if (process->state() != QProcess::NotRunning) process->kill(); });
    process->start("gnome-extensions", {"enable", uuid});
}
