#include "drive_tree.h"
#include "drivelabel.h"
#include <QCollator>
#include <QDir>
#include <QFileInfo>
#include <QHeaderView>
#include <QLocale>
#include <QPainter>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QStyle>

namespace {
QString devicePath(const QString &name) {
    return name.isEmpty() || name.startsWith("/dev/") ? name : "/dev/" + name;
}
QString identity(const DriveInfo &d) {
    return d.diskId.isEmpty() ? d.device : d.diskId;
}
class DriveItem : public QTreeWidgetItem {
public:
    using QTreeWidgetItem::QTreeWidgetItem;
    bool operator<(const QTreeWidgetItem &other) const override {
        const int column = treeWidget()->sortColumn();
        if (column == 2) return data(0, DriveTree::BytesRole).toULongLong() < other.data(0, DriveTree::BytesRole).toULongLong();
        QCollator collator; collator.setNumericMode(true);
        return collator.compare(text(column), other.text(column)) < 0;
    }
};
}

DriveTree::DriveTree(QWidget *parent) : QTreeWidget(parent) {
    setColumnCount(8);
    setHeaderLabels({tr("Device"), tr("Device Path"), tr("Size"), tr("Type"), tr("Label"), tr("Mount Point"), tr("Status"), tr("Disk ID")});
    setAccessibleName(tr("Disks and partitions"));
    setToolTip(tr("Expand a disk with + or the Right arrow key to see its partitions."));
    setAlternatingRowColors(true);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setRootIsDecorated(true);
    setItemsExpandable(true);
    setExpandsOnDoubleClick(true);
    setUniformRowHeights(true);
    setIndentation(24);
    setIconSize(QSize(18, 18));
    setWordWrap(false);
    setTextElideMode(Qt::ElideRight);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    header()->setSectionResizeMode(QHeaderView::Interactive);
    header()->setStretchLastSection(true);
    const int widths[] = {174, 150, 100, 84, 180, 160, 170, 260};
    for (int column = 0; column < 8; ++column) setColumnWidth(column, widths[column]);
    sortItems(0, Qt::AscendingOrder);
    setSortingEnabled(true);
    connect(this, &QTreeWidget::itemExpanded, this, [this](QTreeWidgetItem *item) {
        if (!m_loading) m_expanded.insert(item->data(0, IdentityRole).toString());
    });
    connect(this, &QTreeWidget::itemCollapsed, this, [this](QTreeWidgetItem *item) {
        if (!m_loading) m_expanded.remove(item->data(0, IdentityRole).toString());
        // A hidden child must not remain the target of a destructive action.
        auto *selected = currentItem();
        for (auto *p = selected ? selected->parent() : nullptr; p; p = p->parent()) {
            if (p == item) { setCurrentItem(item); break; }
        }
    });
}

QList<DriveInfo> DriveTree::parse(const QString &output, const QString &byIdDirectory) {
    QList<DriveInfo> result;
    QSet<QString> seen;
    const QRegularExpression pair("(\\w+)=\"([^\"]*)\"");
    QMap<QString, QString> ids;
    for (const auto &file : QDir(byIdDirectory).entryInfoList(QDir::Files | QDir::System | QDir::NoDotAndDotDot, QDir::Name)) {
        const auto alias = file.fileName();
        if (!file.isSymLink() || alias.contains("-part") || alias.startsWith("wwn-") || alias.startsWith("nvme-eui.")) continue;
        const auto target = file.canonicalFilePath();
        if (!target.isEmpty() && !ids.contains(target)) ids[target] = alias;
    }
    for (const auto &line : output.split('\n', Qt::SkipEmptyParts)) {
        QMap<QString, QString> fields;
        auto matches = pair.globalMatch(line);
        while (matches.hasNext()) {
            const auto m = matches.next();
            fields[m.captured(1)] = DriveLabel::decodeLsblkValue(m.captured(2));
        }
        DriveInfo d;
        d.device = devicePath(fields.value("PATH", fields.value("NAME")));
        if (d.device.isEmpty() || seen.contains(d.device)) continue;
        seen.insert(d.device);
        d.type = fields.value("TYPE");
        d.parent = devicePath(fields.value("PKNAME"));
        d.sizeBytes = fields.value("SIZE").toULongLong();
        d.size = QLocale().formattedDataSize(d.sizeBytes, 1, QLocale::DataSizeIecFormat);
        d.mountPoint = fields.value("MOUNTPOINT");
        d.isMounted = !d.mountPoint.isEmpty();
        d.label = fields.value("LABEL"); d.filesystem = fields.value("FSTYPE");
        d.uuid = fields.value("UUID"); d.model = fields.value("MODEL"); d.serial = fields.value("SERIAL");
        d.diskId = ids.value(d.type == "disk" ? d.device : d.parent);
        result.append(d);
    }
    return result;
}

void DriveTree::setDrives(const QList<DriveInfo> &drives, quint64 minimumBytes,
                          bool showDisks, bool showPartitions) {
    const auto selected = selectedDevice();
    const auto selectedParent = currentItem() && currentItem()->parent() ? currentItem()->parent()->data(0, DeviceRole).toString() : QString();
    const int scroll = verticalScrollBar()->value();
    const bool sorting = isSortingEnabled();
    const QSignalBlocker block(this);
    const QSignalBlocker selectionBlock(selectionModel());
    m_loading = true; setSortingEnabled(false); clear(); m_items.clear();
    QHash<QString, QList<DriveInfo>> children;
    QHash<QString, QString> parents;
    for (const auto &d : drives) {
        parents[d.device] = d.parent;
        if (!d.parent.isEmpty()) children[d.parent].append(d);
    }
    auto inUse = [&](const QString &device) {
        QStringList mounts;
        for (const auto &d : drives) {
            if (!d.isMounted) continue;
            QString path = d.device; QSet<QString> visited;
            while (!path.isEmpty() && !visited.contains(path)) {
                if (path == device) { mounts.append(d.device + ": " + d.mountPoint); break; }
                visited.insert(path); path = parents.value(path);
            }
        }
        return mounts;
    };
    auto add = [&](const DriveInfo &d, QTreeWidgetItem *parent) {
        auto *item = new DriveItem;
        if (parent) parent->addChild(item); else addTopLevelItem(item);
        const auto mounts = inUse(d.device);
        item->setData(0, DeviceRole, d.device); item->setData(0, TypeRole, d.type);
        item->setData(0, BytesRole, QVariant::fromValue(d.sizeBytes));
        item->setData(0, InUseRole, !mounts.isEmpty()); item->setData(0, MountRole, d.mountPoint);
        item->setData(0, IdentityRole, identity(d));
        item->setText(0, QFileInfo(d.device).fileName()); item->setText(1, d.device);
        item->setText(2, d.size); item->setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);
        item->setText(3, d.type == "disk" ? tr("Disk") : tr("Partition"));
        item->setText(4, d.label.isEmpty() ? QString::fromUtf8("—") : d.label);
        item->setText(5, d.mountPoint.isEmpty() ? QString::fromUtf8("—") : d.mountPoint);
        item->setText(6, d.isMounted ? tr("Mounted") : mounts.isEmpty() ? tr("Not mounted") : (mounts.size() == 1 ? tr("1 mounted volume") : tr("%1 mounted volumes").arg(mounts.size())));
        item->setText(7, d.diskId.isEmpty() ? QString::fromUtf8("—") : d.diskId);
        item->setIcon(0, QIcon::fromTheme(d.type == "disk" ? "drive-harddisk" : "drive-partition",
                                        style()->standardIcon(QStyle::SP_DriveHDIcon)));
        if (d.type == "disk") { auto font = item->font(0); font.setBold(true); item->setFont(0, font); }
        for (int column = 0; column < 8; ++column) item->setToolTip(column, item->text(column));
        item->setToolTip(0, tr("%1\nModel: %2\nSerial: %3").arg(d.device, d.model, d.serial));
        item->setToolTip(1, tr("%1\nFilesystem: %2\nUUID: %3").arg(d.device, d.filesystem, d.uuid));
        if (!mounts.isEmpty()) item->setToolTip(6, mounts.join('\n'));
        m_items[d.device] = item;
        return item;
    };
    int diskCount = 0, partitionCount = 0, hidden = 0;
    QSet<QString> existing;
    QSet<QString> physicalDevices;
    for (const auto &d : drives) {
        if (d.type != "disk") continue;
        physicalDevices.insert(d.device);
        existing.insert(identity(d));
        if (d.sizeBytes < minimumBytes) { ++hidden; continue; }
        auto *root = showDisks ? add(d, nullptr) : nullptr;
        if (root) ++diskCount;
        for (const auto &child : children.value(d.device)) {
            if (!showPartitions || child.type != "part") continue;
            if (child.sizeBytes < minimumBytes) { ++hidden; continue; }
            add(child, root); ++partitionCount;
        }
        if (root) root->setExpanded(m_expanded.contains(identity(d)));
    }
    // Preserve standalone partitions when lsblk cannot report their parent
    // (for example during a hotplug refresh or inside a container).
    if (showPartitions) for (const auto &d : drives) {
        if (d.type != "part" || physicalDevices.contains(d.parent) || m_items.contains(d.device)) continue;
        if (d.sizeBytes < minimumBytes) { ++hidden; continue; }
        add(d, nullptr); ++partitionCount;
    }
    m_expanded.intersect(existing);
    setSortingEnabled(sorting);
    if (!selected.isEmpty() && !selectDevice(selected) && !selectedParent.isEmpty()) selectDevice(selectedParent);
    verticalScrollBar()->setValue(scroll);
    m_summary = (diskCount == 1 ? tr("1 disk") : tr("%1 disks").arg(diskCount)) + " · "
        + (partitionCount == 1 ? tr("1 partition") : tr("%1 partitions").arg(partitionCount));
    if (hidden) m_summary += " · " + tr("%1 below minimum").arg(hidden);
    m_loading = false;
}

QTreeWidgetItem *DriveTree::deviceItem(const QString &device) const { return m_items.value(device, nullptr); }
QString DriveTree::selectedDevice() const { return currentItem() && currentItem()->isSelected() ? currentItem()->data(0, DeviceRole).toString() : QString(); }
QString DriveTree::selectedType() const { return selectedDevice().isEmpty() ? QString() : currentItem()->data(0, TypeRole).toString(); }
QString DriveTree::selectedMount() const { return selectedDevice().isEmpty() ? QString() : currentItem()->data(0, MountRole).toString(); }
bool DriveTree::selectedInUse() const { return !selectedDevice().isEmpty() && currentItem()->data(0, InUseRole).toBool(); }
bool DriveTree::selectDevice(const QString &device) {
    auto *item = deviceItem(device);
    if (!item) return false;
    for (auto *parent = item->parent(); parent; parent = parent->parent()) parent->setExpanded(true);
    setCurrentItem(item); item->setSelected(true); return true;
}

void DriveTree::drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const {
    if (!model()->hasChildren(index)) return;
    const auto p = palette();
    const bool selected = selectionModel()->isSelected(index);
    const auto color = p.color(selected ? QPalette::HighlightedText : QPalette::Text);
    const int x = layoutDirection() == Qt::RightToLeft ? rect.left() + indentation()/2 : rect.right() - indentation()/2 + 1;
    const QRectF box(x - 7, rect.center().y() - 7, 14, 14);
    painter->save(); painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(color, 1)); painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(box, 3, 3);
    painter->drawLine(QPointF(x - 3, box.center().y()), QPointF(x + 3, box.center().y()));
    if (!isExpanded(index)) painter->drawLine(QPointF(x, box.center().y() - 3), QPointF(x, box.center().y() + 3));
    painter->restore();
}
