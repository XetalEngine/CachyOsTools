#pragma once
#include <QWidget>
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QTemporaryFile>
#include <QTimer>
#include <memory>

class QComboBox;
class QCheckBox;
class QLabel;
class QLineEdit;
class QTableView;
class QStandardItemModel;
class QSortFilterProxyModel;
class QPushButton;
class QSpinBox;
class QShowEvent;

class WindowEnumController : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.xetal.CachyOsTools.WindowEnum")
public:
    explicit WindowEnumController(QObject *parent = nullptr);
    ~WindowEnumController() override;
    void setBackend(const QString &backend);
    QString backend() const { return m_backend; }
    bool busy() const { return m_busy; }
    void request(QJsonObject request);
public slots:
    Q_SCRIPTABLE void Publish(const QString &token, const QString &json);
signals:
    void result(const QJsonObject &value);
    void busyChanged(bool busy);
private:
    QString m_backend = "auto", m_activeBackend, m_token, m_path, m_scriptName;
    QProcess *m_process = nullptr;
    QTimer m_timeout;
    QByteArray m_buffer;
    bool m_busy = false;
    std::unique_ptr<QTemporaryFile> m_script;
    void complete(QJsonObject result);
    void stop();
    void kwin(const QJsonObject &request);
    void gnome(const QJsonObject &request);
    void helper(const QJsonObject &request, bool wayland);
};

class WindowEnumTab : public QWidget {
    Q_OBJECT
public:
    explicit WindowEnumTab(QWidget *parent = nullptr);
protected:
    void showEvent(QShowEvent *event) override;
private:
    WindowEnumController *controller;
    QComboBox *source;
    QCheckBox *live;
    QLineEdit *search;
    QLabel *coverage, *status, *selectionLabel;
    QTableView *table;
    QStandardItemModel *model;
    QSortFilterProxyModel *proxy;
    QTimer timer;
    QSpinBox *x, *y, *width, *height;
    QPushButton *geometry, *actions, *details, *gnomeButton;
    QJsonArray rows;
    QString selectedId;
    bool filling = false, geometryDirty = false;
    QJsonObject selected() const;
    void refresh();
    void populate(const QJsonObject &result);
    void updateSelection();
    void act(const QString &name, const QJsonValue &value = QJsonValue());
    void actionMenu();
    void showDetails();
    void exportRows();
    void installGnomeBridge();
};
