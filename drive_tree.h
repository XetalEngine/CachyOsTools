#ifndef XETAL_DRIVE_TREE_H
#define XETAL_DRIVE_TREE_H

#include <QTreeWidget>
#include <QHash>
#include <QSet>

struct DriveInfo {
    QString device, size, type, mountPoint, label, diskId, filesystem, uuid, model, serial, parent;
    quint64 sizeBytes = 0;
    bool isMounted = false;
};

// No device operations belong here: this widget only presents lsblk metadata.
class DriveTree : public QTreeWidget {
    Q_OBJECT
public:
    enum Role { DeviceRole = Qt::UserRole, TypeRole, BytesRole, InUseRole, MountRole, IdentityRole };
    explicit DriveTree(QWidget *parent = nullptr);
    static QList<DriveInfo> parse(const QString &output, const QString &byIdDirectory = "/dev/disk/by-id");
    void setDrives(const QList<DriveInfo> &drives, quint64 minimumBytes,
                   bool showDisks = true, bool showPartitions = true);
    QString selectedDevice() const;
    QString selectedType() const;
    QString selectedMount() const;
    bool selectedInUse() const;
    bool selectDevice(const QString &device);
    QTreeWidgetItem *deviceItem(const QString &device) const;
    QString summary() const { return m_summary; }
protected:
    void drawBranches(QPainter *, const QRect &, const QModelIndex &) const override;
private:
    QHash<QString, QTreeWidgetItem *> m_items;
    QSet<QString> m_expanded;
    QString m_summary;
    bool m_loading = false;
};
#endif
