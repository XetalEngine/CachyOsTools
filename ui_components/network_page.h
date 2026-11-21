#pragma once
#include <QMap>
#include <QWidget>

class QAbstractButton;
class QAction;
class QGridLayout;
class QLabel;
class QPushButton;
class QTableWidget;
class QTabWidget;
class QTimer;
class QVBoxLayout;

// Reuses the backend's original widgets and connections: changing the layout
// must not change the selected adapter, connection UUID, or privileged action.
class NetworkPage final : public QWidget {
    Q_OBJECT
public:
    explicit NetworkPage(QWidget *legacy, QWidget *parent = nullptr);
    void sync();
signals:
    void refreshRequested(const QString &section);
protected:
    void showEvent(QShowEvent *) override;
    void resizeEvent(QResizeEvent *) override;
private:
    struct List {
        QTableWidget *table;
        QLabel *empty;
        QList<QAbstractButton *> selectionButtons;
        QString selectedKey;
        QString emptyText;
        int maxRows = 5;
    };
    QWidget *legacy;
    QTabWidget *sections;
    QTimer *updateTimer = nullptr;
    QLabel *summary, *adapterDetail, *savedHint;
    QAbstractButton *savedToggle;
    QGridLayout *metrics;
    QList<QWidget *> metricCards;
    QMap<QString, List> lists;
    QMap<QAbstractButton *, QAction *> menuActions;
    QList<QPair<QGridLayout *, QList<QWidget *>>> responsive;
    bool syncing = false;
    bool refreshScheduled = false;
    QWidget *card(const QString &title, const QString &description, const QString &icon);
    QVBoxLayout *content(QWidget *card) const;
    QPushButton *button(const char *name, const QString &label, const QString &icon = {}, bool primary = false);
    QTableWidget *list(QWidget *card, const char *name, const QString &empty, int maxRows = 5);
    void actions(QWidget *card, const QList<QPushButton *> &buttons);
    void menu(QWidget *card, const QList<QPushButton *> &buttons);
    QString selectionKey(QTableWidget *table, int row) const;
    void arrange();
    void requestRefresh();
};
