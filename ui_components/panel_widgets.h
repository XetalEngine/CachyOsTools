#pragma once
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QLayout>
#include <QObject>
#include <QString>
#include <QTabBar>
#include <QTabWidget>
#include <QVector>
// Inactive tools must not impose their minimum size on the selected tool.
class PanelTabs : public QTabWidget {
public:
  explicit PanelTabs(QWidget *parent = nullptr) : QTabWidget(parent) {
    connect(this, &QTabWidget::currentChanged, this,
            [this] { updateGeometry(); });
  }
  QSize sizeHint() const override {
    return currentWidget()
               ? currentWidget()->sizeHint() +
                     QSize(4, tabBar()->isHidden()
                                  ? 4
                                  : tabBar()->sizeHint().height() + 4)
               : QSize(0, 0);
  }
  QSize minimumSizeHint() const override {
    return currentWidget()
               ? currentWidget()->minimumSizeHint() +
                     QSize(4, tabBar()->isHidden()
                                  ? 4
                                  : tabBar()->minimumSizeHint().height() + 4)
               : QSize(0, 0);
  }
  bool hasHeightForWidth() const override {
    return currentWidget() && currentWidget()->hasHeightForWidth();
  }
  int heightForWidth(int width) const override {
    if (!currentWidget()) return -1;
    // A scroll area's minimum height must not include a text editor's preferred
    // 200px height. Let expanding outputs shrink while wrapped labels fit.
    const auto *layout = currentWidget()->layout();
    const int height = layout ? layout->minimumHeightForWidth(width - 4)
                              : currentWidget()->heightForWidth(width - 4);
    return height < 0 ? -1 : height + (tabBar()->isHidden() ? 4 : tabBar()->sizeHint().height() + 4);
  }
};

#include <QCoreApplication>
#include <QFileInfo>
namespace CachyUi {
inline QString toolPath(const QString &name) {
    const auto app = QCoreApplication::applicationDirPath();
    QStringList roots{app, app + "/../share/cachyostools"};
#ifdef CACHYOSTOOLS_DATA_DIR
    roots << QString::fromUtf8(CACHYOSTOOLS_DATA_DIR);
#endif
#ifdef CACHYOSTOOLS_SOURCE_DIR
    roots << QString::fromUtf8(CACHYOSTOOLS_SOURCE_DIR);
#endif
    for (const auto &root : roots) {
        const auto path = QDir(root).filePath("tools/" + name);
        if (QFileInfo(path).isFile()) return QDir::cleanPath(path);
    }
    return QDir(app).filePath("tools/" + name);
}
}
