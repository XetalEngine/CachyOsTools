#ifndef CACHYOS_DRIVE_LABEL_H
#define CACHYOS_DRIVE_LABEL_H

#include <QDialog>
#include <QDBusMessage>
#include <QStringList>
#include <functional>

class QLabel;
class QLineEdit;
class QPushButton;
class QCheckBox;

namespace DriveLabel {
struct Volume {
    QString device, objectPath, filesystem, label, uuid, drive;
    quint64 deviceNumber = 0, size = 0;
    bool readOnly = true;
    QStringList mountPoints;
};
bool supported(const QString &filesystem);
QString rules(const QString &filesystem);
QString missingDependency(const QString &filesystem);
QString validate(const QString &filesystem, const QString &label);
QString unavailable(const Volume &volume);
bool sameVolume(const Volume &a, const Volume &b);
bool fstabUsesLabel(const QString &fstab, const QString &label);
QString decodeLsblkValue(const QString &value);
}
Q_DECLARE_METATYPE(DriveLabel::Volume)

// The transport is injectable so tests can exercise the complete read/check/write
// sequence without connecting to a storage daemon or touching a block device.
class DriveLabelService : public QObject {
    Q_OBJECT
public:
    using Reply = std::function<void(const QDBusMessage &)>;
    using Transport = std::function<void(const QDBusMessage &, Reply)>;
    explicit DriveLabelService(QObject *parent = nullptr, Transport transport = {});
    void inspect(const QString &device);
    void save(const DriveLabel::Volume &expected, const QString &label);
    bool busy() const { return m_busy; }
signals:
    void inspected(const DriveLabel::Volume &volume);
    void saved(const QString &label);
    void failed(const QString &message);
    void refreshRequested();
private:
    using ReadReply = std::function<void(const DriveLabel::Volume &, const QString &)>;
    void call(const QString &path, const QString &interface, const QString &method,
              const QVariantList &arguments, Reply reply);
    void read(const QString &device, ReadReply reply);
    void verify(const DriveLabel::Volume &expected, const QString &label, int retries);
    void fail(const QString &message);
    Transport m_transport;
    bool m_busy = false;
};

class DriveLabelDialog : public QDialog {
    Q_OBJECT
public:
    explicit DriveLabelDialog(const QString &device, QWidget *parent = nullptr,
                              DriveLabelService::Transport transport = {});
    void reject() override;
signals:
    void labelChanged(const QString &label);
    void refreshRequested();
private:
    void updateState();
    void save();
    DriveLabelService *m_service;
    DriveLabel::Volume m_volume;
    QLabel *m_details, *m_rules, *m_status, *m_mountWarning;
    QLineEdit *m_label;
    QCheckBox *m_fstabAcknowledged;
    QPushButton *m_save, *m_cancel;
    QString m_dependencyError;
    bool m_loaded = false, m_saving = false;
};

#endif
