#pragma once
#include <QColor>
#include <QPalette>
#include <QString>
class QWidget;
class QTabWidget;
namespace CachyUi {
QPalette palette(const QPalette &system, const QColor &accent,
                 const QString &background, const QString &text);
QString stylesheet();
void pageHeading(QWidget *page, const QString &title, const QString &description);
void tweakCards(QTabWidget *tabs);
void polishViews(QWidget *root);
}
