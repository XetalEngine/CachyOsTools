#include "presentation.h"
#include <QtWidgets>
#include <functional>

namespace CachyUi {
QPalette palette(const QPalette &system, const QColor &accent,
                 const QString &background, const QString &foreground) {
    auto result = system;
    const QColor window = background.isEmpty() ? system.color(QPalette::Window) : QColor(background);
    const bool dark = window.lightness() < 128;
    const QColor text = foreground.isEmpty()
        ? (background.isEmpty() ? system.color(QPalette::WindowText) : QColor(dark ? "#e9edf2" : "#20252c"))
        : QColor(foreground);
    const QColor card = dark ? window.lighter(130) : window.darker(103);
    const QColor base = dark ? window.darker(125) : window.lighter(105);
    const QColor line = dark ? card.lighter(155) : card.darker(120);
    for (auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) result.setColor(role, text);
    result.setColor(QPalette::Window, window); result.setColor(QPalette::Base, base);
    result.setColor(QPalette::AlternateBase, card); result.setColor(QPalette::Button, card);
    result.setColor(QPalette::Mid, line); result.setColor(QPalette::Midlight, line);
    result.setColor(QPalette::Highlight, accent); result.setColor(QPalette::Link, accent);
    result.setColor(QPalette::HighlightedText, accent.lightness() > 155 ? QColor("#101820") : QColor(Qt::white));
    result.setColor(QPalette::PlaceholderText, dark ? text.darker(130) : text.lighter(150));
    for (auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
        result.setColor(QPalette::Disabled, role, result.color(QPalette::PlaceholderText));
    return result;
}

QString stylesheet() {
    return QStringLiteral(R"(
      QMainWindow { background:palette(window); }
      QWidget#centralwidget { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 palette(alternate-base),stop:1 palette(window)); }
      QWidget[modernCanvas="true"], QScrollArea[modernScroll="true"] { background:transparent; border:0; }
      QLabel#panelTitle { font-size:23px; font-weight:700; background:transparent; border:0; padding:0; }
      QLabel#muted, QLabel#pageDescription { color:palette(placeholder-text); background:transparent; border:0; padding:0; }
      QGroupBox { background:palette(alternate-base); color:palette(window-text); border:1px solid palette(mid); border-radius:10px; border-top:1px solid palette(mid); margin-top:20px; padding:16px 12px 12px; font-weight:600; }
      QGroupBox::title { subcontrol-origin:margin; left:14px; top:2px; padding:0 5px; color:palette(link); }
      QGroupBox QLabel { background:transparent; border:0; }
      QTabWidget::pane { background:palette(window); border:1px solid palette(mid); border-radius:8px; top:-1px; }
      QTabBar::tab { background:palette(button); color:palette(button-text); border:1px solid palette(mid); border-radius:6px; padding:8px 13px; margin:0 3px 4px 0; }
      QTabBar::tab:selected { background:palette(highlight); color:palette(highlighted-text); border-color:palette(highlight); border-top:1px solid palette(highlight); font-weight:600; }
      QTabBar::tab:hover:!selected { border-color:palette(highlight); }
      QPushButton, QToolButton { background:palette(button); color:palette(button-text); border:1px solid palette(mid); border-radius:6px; padding:7px 12px; }
      QToolButton { padding:6px; }
      QTabBar QToolButton { padding:0; width:24px; border-radius:4px; }
      QTabBar::scroller { width:52px; }
      QPushButton:hover, QToolButton:hover { border-color:palette(highlight); }
      QPushButton:pressed, QToolButton:pressed { background:palette(base); }
      QPushButton[primary="true"]:enabled { background:palette(highlight); color:palette(highlighted-text); border-color:palette(highlight); font-weight:600; }
      QPushButton:disabled, QToolButton:disabled { color:palette(placeholder-text); background:palette(window); }
      QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox { background:palette(base); color:palette(text); border:1px solid palette(mid); border-radius:6px; padding:6px 8px; selection-background-color:palette(highlight); selection-color:palette(highlighted-text); }
      QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus { border-color:palette(highlight); }
      QComboBox QAbstractItemView { background:palette(base); color:palette(text); selection-background-color:palette(highlight); selection-color:palette(highlighted-text); }
      QTableView, QTreeView, QListView, QTextEdit, QPlainTextEdit { background:palette(base); color:palette(text); alternate-background-color:palette(alternate-base); border:1px solid palette(mid); border-radius:8px; selection-background-color:palette(highlight); selection-color:palette(highlighted-text); gridline-color:palette(mid); outline:0; }
      QTableView::item, QTreeView::item, QListView::item { padding:5px 7px; border:0; }
      QHeaderView::section { background:palette(alternate-base); color:palette(window-text); border:0; border-bottom:1px solid palette(mid); padding:8px; font-weight:600; }
      QTextEdit, QPlainTextEdit { padding:8px; }
      QMenu { background:palette(window); color:palette(window-text); border:1px solid palette(mid); padding:5px; }
      QMenu::item { padding:7px 24px 7px 12px; border-radius:4px; }
      QMenu::item:selected { background:palette(highlight); color:palette(highlighted-text); }
      QMenu::item:disabled { color:palette(placeholder-text); }
      QScrollBar:vertical { background:transparent; width:10px; margin:2px; }
      QScrollBar::handle:vertical { background:palette(mid); border-radius:4px; min-height:28px; }
      QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; }
      QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background:transparent; }
      QProgressBar { background:palette(base); color:palette(text); border:1px solid palette(mid); border-radius:5px; min-height:8px; text-align:center; }
      QProgressBar::chunk { background:palette(highlight); border-radius:4px; }
      QSplitter::handle { background:palette(mid); }
      QCheckBox, QRadioButton { spacing:8px; }
      QStatusBar { background:palette(window); color:palette(window-text); border-top:1px solid palette(mid); }
    )");
}

void pageHeading(QWidget *page, const QString &title, const QString &description) {
    if (!page || !page->layout() || page->property("modernHeading").toBool()) return;
    page->setProperty("modernHeading", true);
    auto *content = new QWidget(page); content->setProperty("modernCanvas", true);
    content->setLayout(page->layout());
    auto *layout = new QVBoxLayout(page); layout->setContentsMargins(16, 14, 16, 12); layout->setSpacing(12);
    auto *heading = new QLabel(title, page); heading->setObjectName("panelTitle");
    heading->setTextFormat(Qt::PlainText); layout->addWidget(heading);
    auto *hint = new QLabel(description, page); hint->setObjectName("pageDescription");
    hint->setTextFormat(Qt::PlainText); hint->setWordWrap(true); layout->addWidget(hint);
    auto *scroll = new QScrollArea(page);
    scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
    scroll->setProperty("modernScroll", true);
    scroll->setWidget(content); layout->addWidget(scroll, 1);
}

// Keep the original buttons and signals; let a card's actions wrap naturally
// when the window or the desktop's text scaling leaves less horizontal room.
class ActionFlow final : public QLayout {
    QList<QLayoutItem *> items;
    int arrange(const QRect &rect, bool measure) const {
        const auto margins = contentsMargins();
        const auto area = rect.adjusted(margins.left(), margins.top(), -margins.right(), -margins.bottom());
        int x = area.x(), y = area.y(), row = 0;
        for (auto *item : items) {
            if (item->isEmpty()) continue;
            const auto size = item->sizeHint();
            if (x > area.x() && x + size.width() > area.right() + 1) {
                x = area.x(); y += row + spacing(); row = 0;
            }
            if (!measure) item->setGeometry(QRect(QPoint(x, y), size));
            x += size.width() + spacing(); row = qMax(row, size.height());
        }
        return y + row - rect.y() + margins.bottom();
    }
public:
    explicit ActionFlow(QWidget *parent) : QLayout(parent) { setContentsMargins(8, 8, 8, 8); setSpacing(8); }
    ~ActionFlow() override { while (auto *item = takeAt(0)) delete item; }
    void addItem(QLayoutItem *item) override { items << item; }
    int count() const override { return items.size(); }
    QLayoutItem *itemAt(int index) const override { return items.value(index); }
    QLayoutItem *takeAt(int index) override { return index >= 0 && index < items.size() ? items.takeAt(index) : nullptr; }
    Qt::Orientations expandingDirections() const override { return {}; }
    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override { return arrange(QRect(0, 0, width, 0), true); }
    QSize minimumSize() const override {
        QSize result;
        for (auto *item : items) if (!item->isEmpty()) result = result.expandedTo(item->minimumSize());
        const auto m = contentsMargins(); return result + QSize(m.left()+m.right(), m.top()+m.bottom());
    }
    QSize sizeHint() const override { return QSize(400, heightForWidth(400)); }
    void setGeometry(const QRect &rect) override { QLayout::setGeometry(rect); arrange(rect, false); }
};

void wrapActions(QGroupBox *group) {
    QList<QWidget *> widgets;
    bool simple = true;
    std::function<void(QLayout *)> collect = [&](QLayout *layout) {
        if (!layout) { simple = false; return; }
        for (int i=0; i<layout->count(); ++i) {
            auto *item = layout->itemAt(i);
            if (item->layout()) collect(item->layout());
            else if (auto *widget = item->widget()) {
                if (!qobject_cast<QLabel *>(widget) && !qobject_cast<QAbstractButton *>(widget)) simple = false;
                widgets << widget;
            }
        }
    };
    collect(group->layout());
    if (!simple || widgets.isEmpty()) return; // Keep forms and nested controls intact.
    delete group->layout();
    auto *flow = new ActionFlow(group);
    for (auto *widget : widgets) flow->addWidget(widget);
}

class Cards final : public QWidget {
    QGridLayout *grid;
    QList<QGroupBox *> groups;
    QString filter;
    int columns = 0;
    void arrange() {
        // Allow wide controls to keep their natural width instead of squeezing
        // actions into unreadable cards. Narrow windows become one column.
        int minimum = 400;
        for (auto *group : groups) minimum = qMax(minimum, group->minimumSizeHint().width());
        const int next = width() >= 2 * minimum + 28 ? 2 : 1;
        if (next == columns) return;
        columns = next;
        while (auto *item = grid->takeAt(0)) delete item;
        int index = 0;
        for (auto *group : groups) {
            QString words = group->title();
            for (auto *label : group->findChildren<QLabel *>()) words += ' ' + label->text();
            for (auto *button : group->findChildren<QAbstractButton *>()) words += ' ' + button->text();
            const bool shown = filter.isEmpty() || words.contains(filter, Qt::CaseInsensitive);
            group->setVisible(shown);
            if (shown) { grid->addWidget(group, index / columns, index % columns, Qt::AlignTop); ++index; }
        }
        grid->setColumnStretch(0, 1); grid->setColumnStretch(1, columns > 1 ? 1 : 0);
    }
public:
    explicit Cards(const QList<QGroupBox *> &groups, QWidget *parent) : QWidget(parent), groups(groups) {
        setProperty("modernCanvas", true);
        grid = new QGridLayout(this); grid->setContentsMargins(4, 4, 4, 12); grid->setSpacing(16);
        for (auto *group : groups) { group->setParent(this); wrapActions(group); }
        arrange();
    }
    void search(const QString &text) { filter = text.trimmed(); columns = 0; arrange(); }
protected:
    void resizeEvent(QResizeEvent *event) override { QWidget::resizeEvent(event); arrange(); }
};

void tweakCards(QTabWidget *tabs) {
    for (int i = 0; i < tabs->count(); ++i) {
        auto *page = tabs->widget(i);
        auto *old = new QWidget(page); old->setLayout(page->layout());
        QList<QGroupBox *> groups;
        for (auto *group : old->findChildren<QGroupBox *>()) {
            bool nested = false;
            for (auto *ancestor = group->parentWidget(); ancestor && ancestor != old; ancestor = ancestor->parentWidget())
                if (qobject_cast<QGroupBox *>(ancestor)) { nested = true; break; }
            if (!nested) groups << group;
        }
        if (groups.isEmpty()) { page->setLayout(old->layout()); old->deleteLater(); continue; }
        auto *layout = new QVBoxLayout(page); layout->setContentsMargins(10, 12, 10, 8); layout->setSpacing(8);
        auto *search = new QLineEdit(page); search->setObjectName("tweakSearch");
        search->setPlaceholderText(QObject::tr("Find a setting in this category…")); search->setClearButtonEnabled(true);
        search->setAccessibleName(QObject::tr("Search settings")); layout->addWidget(search);
        auto *scroll = new QScrollArea(page); scroll->setWidgetResizable(true); scroll->setProperty("modernScroll", true);
        auto *cards = new Cards(groups, scroll); scroll->setWidget(cards); layout->addWidget(scroll, 1);
        QObject::connect(search, &QLineEdit::textChanged, cards, [cards](const QString &text) { cards->search(text); });
        old->hide(); // Keeps legacy Ui pointers and their action connections alive.
    }
}

void polishViews(QWidget *root) {
    for (auto *tabs : root->findChildren<QTabWidget *>()) {
        tabs->setUsesScrollButtons(true); tabs->setElideMode(Qt::ElideNone);
        tabs->tabBar()->setExpanding(false);
    }
    for (auto *table : root->findChildren<QTableView *>()) {
        table->setAlternatingRowColors(true); table->setShowGrid(false);
        table->verticalHeader()->setDefaultSectionSize(32);
        table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    }
    for (auto *tree : root->findChildren<QTreeView *>()) {
        tree->setAlternatingRowColors(true); tree->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    }
}
}
