#include "drivelabel.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QDBusMetaType>
#include <QDBusPendingCallWatcher>
#include <QDBusReply>
#include <QDialogButtonBox>
#include <QFile>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>
#include <QVBoxLayout>

namespace {
const QString blockInterface = QStringLiteral("org.freedesktop.UDisks2.Block");
const QString filesystemInterface = QStringLiteral("org.freedesktop.UDisks2.Filesystem");
QString text(const char *s) { return QCoreApplication::translate("DriveLabel", s); }
QString readFstab() {
    QFile file(QStringLiteral("/etc/fstab"));
    return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
}
QString fromBytes(QByteArray bytes) {
    if (bytes.endsWith('\0')) bytes.chop(1);
    return QString::fromUtf8(bytes);
}
QString replyError(const QDBusMessage &reply) {
    if (reply.type() == QDBusMessage::ErrorMessage)
        return reply.errorMessage().isEmpty() ? reply.errorName() : reply.errorMessage();
    if (reply.type() != QDBusMessage::ReplyMessage)
        return text("No reply from the storage service.");
    return {};
}
}

namespace DriveLabel {
bool supported(const QString &fs) {
    return QStringList{"ntfs", "vfat", "exfat", "ext2", "ext3", "ext4", "xfs", "btrfs"}.contains(fs);
}

QString rules(const QString &fs) {
    if (fs == "ntfs") return text("Up to 32 characters for Windows compatibility. Spaces and mixed case are allowed. Requires ntfs-3g on Arch.");
    if (fs == "vfat") return text("Up to 11 ASCII characters, in UPPERCASE. Requires dosfstools on Arch.");
    if (fs == "exfat") return text("Up to 11 UTF-16 characters. Requires exfatprogs on Arch.");
    if (fs.startsWith("ext")) return text("Up to 16 UTF-8 bytes. Accented letters may use more than one byte. Requires e2fsprogs on Arch.");
    if (fs == "xfs") return text("Up to 12 UTF-8 bytes, without spaces. Requires xfsprogs on Arch.");
    if (fs == "btrfs") return text("Up to 255 UTF-8 bytes. Requires btrfs-progs on Arch.");
    return text("Supported filesystems: NTFS, FAT, exFAT, ext2/3/4, XFS and Btrfs.");
}

QString missingDependency(const QString &fs) {
    const QMap<QString, QPair<QString, QString>> tools{
        {"ntfs", {"ntfslabel", "ntfs-3g"}}, {"vfat", {"fatlabel", "dosfstools"}},
        {"exfat", {"exfatlabel", "exfatprogs"}}, {"ext2", {"e2label", "e2fsprogs"}},
        {"ext3", {"e2label", "e2fsprogs"}}, {"ext4", {"e2label", "e2fsprogs"}},
        {"xfs", {"xfs_admin", "xfsprogs"}}, {"btrfs", {"btrfs", "btrfs-progs"}}
    };
    const auto tool = tools.value(fs);
    if (tool.first.isEmpty()) return {};
    if (!QStandardPaths::findExecutable(tool.first).isEmpty()
        || !QStandardPaths::findExecutable(tool.first, {"/usr/sbin", "/sbin"}).isEmpty()) return {};
    return text("Labeling %1 requires %2. Install the %3 package from PKG Install, then reopen this dialog.")
        .arg(fs.toUpper(), tool.first, tool.second);
}

QString validate(const QString &fs, const QString &label) {
    if (!supported(fs)) return text("This filesystem does not support labeling here.");
    // Some filesystem utilities parse options even after their device argument.
    if (label.startsWith('-')) return text("Start the label with a letter or number, rather than a dash.");
    if (label != label.trimmed()) return text("Remove spaces at the beginning or end of the label.");
    for (QChar ch : label) {
        if (ch.category() == QChar::Other_Control || ch.category() == QChar::Other_Format
            || ch.category() == QChar::Separator_Line || ch.category() == QChar::Separator_Paragraph)
            return text("Use a single line without control characters.");
    }
    if (QString::fromUtf8(label.toUtf8()) != label)
        return text("The label contains an invalid Unicode character.");
    const bool windows = fs == "ntfs" || fs == "exfat" || fs == "vfat";
    const QString forbidden = fs == "vfat" ? QStringLiteral("\"*?.,;:/\\|+=<>[]") : QStringLiteral("\"*/:<>?\\|");
    if (windows) {
        for (QChar ch : label)
            if (forbidden.contains(ch)) return text("The label contains a character this filesystem does not allow: ") + ch;
    }
    if (fs == "ntfs" && label.size() > 32) return text("NTFS labels are limited to 32 UTF-16 characters here for Windows compatibility.");
    if (fs == "exfat" && label.size() > 11) return text("exFAT labels can contain at most 11 UTF-16 characters.");
    if (fs == "vfat") {
        if (label.size() > 11) return text("FAT labels can contain at most 11 characters.");
        for (QChar ch : label)
            if (ch.unicode() > 126) return text("Use ASCII characters for a portable FAT label.");
        if (label != label.toUpper()) return text("Use uppercase letters for a FAT label.");
        if (label == "NO NAME") return text("NO NAME is reserved by FAT. Leave the field empty to remove the label.");
    }
    int limit = fs.startsWith("ext") ? 16 : fs == "xfs" ? 12 : fs == "btrfs" ? 255 : 0;
    if (limit && label.toUtf8().size() > limit)
        return text("The label is too long: this filesystem allows %1 UTF-8 bytes.").arg(limit);
    if (fs == "xfs") {
        for (QChar ch : label) if (ch.isSpace()) return text("XFS labels cannot contain spaces.");
    }
    return {};
}

QString unavailable(const Volume &v) {
    if (!supported(v.filesystem)) return text("This is not a supported filesystem. Select a formatted volume or partition.");
    if (v.readOnly) return text("This volume is read-only. Its label cannot be changed.");
    if (!v.mountPoints.isEmpty()) return text("Unmount this volume first using the Drives tab. Mounted at: %1").arg(v.mountPoints.join(", "));
    return {};
}

bool sameVolume(const Volume &a, const Volume &b) {
    return a.device == b.device && a.objectPath == b.objectPath && a.deviceNumber == b.deviceNumber
        && a.uuid == b.uuid && a.filesystem == b.filesystem && a.size == b.size && a.drive == b.drive;
}

bool fstabUsesLabel(const QString &fstab, const QString &label) {
    if (label.isEmpty()) return false;
    for (const QString &line : fstab.split('\n')) {
        const QString entry = line.trimmed();
        if (entry.isEmpty() || entry.startsWith('#')) continue;
        QString source = entry.section(QRegularExpression("\\s+"), 0, 0);
        // fstab escapes whitespace and backslashes with octal sequences.
        const QRegularExpression escape("\\\\([0-7]{3})");
        auto matches = escape.globalMatch(source);
        QString decoded;
        int offset = 0;
        while (matches.hasNext()) {
            const auto match = matches.next();
            decoded += source.mid(offset, match.capturedStart() - offset);
            decoded += QChar(match.captured(1).toUShort(nullptr, 8));
            offset = match.capturedEnd();
        }
        decoded += source.mid(offset);
        if (decoded == "LABEL=" + label || decoded == "LABEL=\"" + label + "\"") return true;
        const QString byLabel = "/dev/disk/by-label/";
        if (decoded.startsWith(byLabel)
            && decodeLsblkValue(decoded.mid(byLabel.size())) == label) return true;
    }
    return false;
}

QString decodeLsblkValue(const QString &value) {
    QByteArray bytes = value.toUtf8(), decoded;
    for (int i = 0; i < bytes.size(); ++i) {
        if (bytes[i] == '\\' && i + 3 < bytes.size() && bytes[i + 1] == 'x') {
            bool ok = false;
            const auto ch = bytes.mid(i + 2, 2).toUInt(&ok, 16);
            if (ok) { decoded += char(ch); i += 3; continue; }
        }
        decoded += bytes[i];
    }
    return QString::fromUtf8(decoded);
}
}

DriveLabelService::DriveLabelService(QObject *parent, Transport transport)
    : QObject(parent), m_transport(std::move(transport)) {
    qRegisterMetaType<DriveLabel::Volume>();
    qDBusRegisterMetaType<QList<QByteArray>>();
    qDBusRegisterMetaType<QList<QDBusObjectPath>>();
}

void DriveLabelService::call(const QString &path, const QString &interface, const QString &method,
                            const QVariantList &args, Reply reply) {
    auto message = QDBusMessage::createMethodCall("org.freedesktop.UDisks2", path, interface, method);
    message.setArguments(args);
    // Polkit owns authentication; no passwords or shell commands enter this code.
    message.setInteractiveAuthorizationAllowed(true);
    const QPointer<DriveLabelService> guard(this);
    auto guarded = [guard, reply](const QDBusMessage &result) { if (guard) reply(result); };
    if (m_transport) { m_transport(message, guarded); return; }
    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(
        message, method == "SetLabel" ? 120000 : 10000), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [watcher, guarded]() {
        const auto result = watcher->reply();
        watcher->deleteLater();
        guarded(result);
    });
}

void DriveLabelService::read(const QString &device, ReadReply done) {
    call("/org/freedesktop/UDisks2/Manager", "org.freedesktop.UDisks2.Manager", "ResolveDevice",
         {QVariantMap{{"path", device}}, QVariantMap{}}, [this, device, done](const QDBusMessage &reply) {
        QDBusReply<QList<QDBusObjectPath>> paths(reply);
        if (!paths.isValid() || paths.value().size() != 1) {
            done({}, text("Could not identify this volume through UDisks. Refresh the drive list and try again. ") + replyError(reply));
            return;
        }
        const QString path = paths.value().first().path();
        call(path, "org.freedesktop.DBus.Properties", "GetAll", {blockInterface},
             [this, path, device, done](const QDBusMessage &blockReply) {
            QDBusReply<QVariantMap> block(blockReply);
            if (!block.isValid()) { done({}, text("Could not read volume information: ") + replyError(blockReply)); return; }
            const auto properties = block.value();
            for (const auto &key : {"Device", "DeviceNumber", "IdUsage", "IdType", "IdUUID", "IdLabel", "ReadOnly", "Size", "Drive"}) {
                if (!properties.contains(key)) { done({}, text("The storage service returned incomplete volume information.")); return; }
            }
            if (properties.value("ReadOnly").userType() != QMetaType::Bool
                || properties.value("Device").userType() != QMetaType::QByteArray
                || properties.value("DeviceNumber").userType() != QMetaType::ULongLong
                || properties.value("Size").userType() != QMetaType::ULongLong
                || properties.value("Drive").userType() != qMetaTypeId<QDBusObjectPath>()) {
                done({}, text("The storage service returned invalid volume information.")); return;
            }
            for (const auto &key : {"IdUsage", "IdType", "IdUUID", "IdLabel"}) {
                if (properties.value(key).userType() != QMetaType::QString) {
                    done({}, text("The storage service returned invalid filesystem information.")); return;
                }
            }
            DriveLabel::Volume v;
            v.device = fromBytes(properties.value("Device").toByteArray());
            v.objectPath = path;
            v.deviceNumber = properties.value("DeviceNumber").toULongLong();
            v.filesystem = properties.value("IdType").toString();
            v.label = properties.value("IdLabel").toString();
            v.uuid = properties.value("IdUUID").toString();
            v.size = properties.value("Size").toULongLong();
            v.drive = qdbus_cast<QDBusObjectPath>(properties.value("Drive")).path();
            v.readOnly = properties.value("ReadOnly").toBool();
            if (v.device != device || !v.deviceNumber || !v.size) {
                done({}, text("The selected device no longer matches the volume. Refresh the drive list.")); return;
            }
            if (properties.value("IdUsage").toString() != "filesystem" || !DriveLabel::supported(v.filesystem)) {
                done({}, text("Select a formatted volume or partition. ") + DriveLabel::rules({})); return;
            }
            if (v.uuid.isEmpty()) { done({}, text("The filesystem UUID is unavailable, so this volume cannot be identified safely.")); return; }
            call(path, "org.freedesktop.DBus.Properties", "GetAll", {filesystemInterface},
                 [v, done](const QDBusMessage &fsReply) mutable {
                QDBusReply<QVariantMap> fs(fsReply);
                if (!fs.isValid() || !fs.value().contains("MountPoints")) {
                    done({}, text("Could not check whether this volume is mounted. ") + replyError(fsReply)); return;
                }
                const QVariant mounts = fs.value().value("MountPoints");
                // Do not mistake an invalid D-Bus value for an empty mount list.
                if (mounts.userType() != qMetaTypeId<QList<QByteArray>>()
                    && (mounts.userType() != qMetaTypeId<QDBusArgument>()
                        || mounts.value<QDBusArgument>().currentSignature() != "aay")) {
                    done({}, text("The storage service returned an invalid mount list.")); return;
                }
                for (const auto &mount : qdbus_cast<QList<QByteArray>>(mounts))
                    v.mountPoints << fromBytes(mount);
                done(v, {});
            });
        });
    });
}

void DriveLabelService::fail(const QString &message) {
    m_busy = false;
    emit failed(message);
}

void DriveLabelService::inspect(const QString &device) {
    if (m_busy) return;
    m_busy = true;
    read(device, [this](const DriveLabel::Volume &volume, const QString &error) {
        if (!error.isEmpty()) { fail(error); return; }
        m_busy = false;
        emit inspected(volume);
    });
}

void DriveLabelService::save(const DriveLabel::Volume &expected, const QString &label) {
    if (m_busy) return;
    const QString invalid = DriveLabel::validate(expected.filesystem, label);
    if (!invalid.isEmpty()) { fail(invalid); return; }
    m_busy = true;
    read(expected.device, [this, expected, label](const DriveLabel::Volume &current, const QString &error) {
        if (!error.isEmpty()) { fail(error); return; }
        if (!DriveLabel::sameVolume(expected, current) || expected.label != current.label) {
            fail(text("The volume or its label changed while this dialog was open. Close it, refresh the drive list and try again.")); return;
        }
        const auto blocked = DriveLabel::unavailable(current);
        if (!blocked.isEmpty()) { fail(blocked); return; }
        if (label == current.label) { m_busy = false; emit saved(label); return; }
        call(current.objectPath, filesystemInterface, "SetLabel", {label, QVariantMap{}},
             [this, current, label](const QDBusMessage &reply) {
            emit refreshRequested();
            const auto error = replyError(reply);
            if (!error.isEmpty()) {
                QString advice;
                if (current.filesystem == "ntfs") advice = text("\n\nIf Windows left this volume hibernated or dirty, fully shut down Windows first. NTFS labeling requires the ntfs-3g package.");
                fail(text("The label change could not be confirmed. Refresh before retrying.\n\n") + error + advice); return;
            }
            verify(current, label, 3);
        });
    });
}

void DriveLabelService::verify(const DriveLabel::Volume &expected, const QString &label, int retries) {
    read(expected.device, [this, expected, label, retries](const DriveLabel::Volume &current, const QString &error) {
        if (error.isEmpty() && DriveLabel::sameVolume(expected, current) && current.label == label) {
            m_busy = false;
            emit saved(current.label);
        } else if (error.isEmpty() && DriveLabel::sameVolume(expected, current) && retries > 0) {
            QTimer::singleShot(250, this, [this, expected, label, retries]() { verify(expected, label, retries - 1); });
        } else {
            fail(text("The storage service completed the request, but the new label could not be verified. Refresh the drive list before trying again. ") + error);
        }
    });
}

DriveLabelDialog::DriveLabelDialog(const QString &device, QWidget *parent, DriveLabelService::Transport transport)
    : QDialog(parent), m_service(new DriveLabelService(this, std::move(transport))) {
    setWindowTitle(tr("Set Volume Label"));
    setMinimumWidth(520);
    resize(600, 340);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(14);
    auto *intro = new QLabel(tr("Give this volume a name you can recognize in Linux and, for Windows filesystems, in Windows."), this);
    intro->setWordWrap(true);
    layout->addWidget(intro);
    auto *group = new QGroupBox(tr("Selected volume"), this);
    auto *volumeLayout = new QVBoxLayout(group);
    m_details = new QLabel(device, group);
    m_details->setTextFormat(Qt::PlainText);
    m_details->setWordWrap(true);
    m_details->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    m_details->setTextInteractionFlags(Qt::TextSelectableByMouse);
    volumeLayout->addWidget(m_details);
    layout->addWidget(group);
    auto *form = new QFormLayout;
    m_label = new QLineEdit(this);
    m_label->setObjectName("volumeLabelEdit");
    m_label->setPlaceholderText(tr("Leave empty to remove the label"));
    m_label->setClearButtonEnabled(true);
    m_label->setEnabled(false);
    form->addRow(tr("New &label"), m_label);
    layout->addLayout(form);
    m_rules = new QLabel(this);
    m_rules->setWordWrap(true);
    m_rules->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    layout->addWidget(m_rules);
    m_mountWarning = new QLabel(tr("This label is used in /etc/fstab. Renaming it will require updating that mount entry before the next mount or boot."), this);
    m_mountWarning->setWordWrap(true);
    m_mountWarning->hide();
    layout->addWidget(m_mountWarning);
    m_fstabAcknowledged = new QCheckBox(tr("I will update the mount entry to use the new label or UUID."), this);
    m_fstabAcknowledged->hide();
    layout->addWidget(m_fstabAcknowledged);
    layout->addStretch();
    m_status = new QLabel(tr("Checking volume…"), this);
    m_status->setObjectName("volumeLabelStatus");
    m_status->setWordWrap(true);
    m_status->setTextFormat(Qt::PlainText);
    layout->addWidget(m_status);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    m_save = buttons->button(QDialogButtonBox::Save);
    m_save->setObjectName("saveVolumeLabelButton");
    m_save->setText(tr("Save Label"));
    m_save->setEnabled(false);
    m_cancel = buttons->button(QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &DriveLabelDialog::save);
    connect(buttons, &QDialogButtonBox::rejected, this, &DriveLabelDialog::reject);
    connect(m_label, &QLineEdit::textChanged, this, &DriveLabelDialog::updateState);
    connect(m_fstabAcknowledged, &QCheckBox::toggled, this, &DriveLabelDialog::updateState);
    connect(m_service, &DriveLabelService::inspected, this, [this](const DriveLabel::Volume &v) {
        m_volume = v;
        m_details->setText(tr("%1  •  %2  •  %3\nCurrent label: %4")
            .arg(v.device, v.filesystem.toUpper(), QLocale().formattedDataSize(v.size), v.label.isEmpty() ? tr("None") : v.label));
        m_rules->setText(DriveLabel::rules(v.filesystem));
        m_dependencyError = DriveLabel::missingDependency(v.filesystem);
        m_label->setText(v.label);
        m_loaded = true;
        const bool reference = DriveLabel::fstabUsesLabel(readFstab(), v.label);
        m_mountWarning->setVisible(reference);
        m_fstabAcknowledged->setVisible(reference);
        updateState();
        m_label->setFocus();
        m_label->selectAll();
        adjustSize();
    });
    connect(m_service, &DriveLabelService::failed, this, [this](const QString &error) {
        m_saving = false;
        // A fresh dialog is required after an error: mount state or identity may
        // have changed, and a timed-out write may still complete in the daemon.
        m_loaded = false;
        m_label->setEnabled(false);
        m_save->setEnabled(false);
        m_cancel->setEnabled(true);
        m_cancel->setText(tr("Close"));
        m_status->setText(error);
        adjustSize();
    });
    connect(m_service, &DriveLabelService::saved, this, [this](const QString &label) {
        m_saving = false;
        emit labelChanged(label);
        accept();
    });
    connect(m_service, &DriveLabelService::refreshRequested, this, &DriveLabelDialog::refreshRequested);
    QTimer::singleShot(0, this, [this, device]() { m_service->inspect(device); });
}

void DriveLabelDialog::updateState() {
    if (!m_loaded || m_saving) return;
    auto blocked = DriveLabel::unavailable(m_volume);
    if (blocked.isEmpty()) blocked = m_dependencyError;
    const auto invalid = DriveLabel::validate(m_volume.filesystem, m_label->text());
    const bool changed = m_label->text() != m_volume.label;
    const bool acknowledged = m_fstabAcknowledged->isHidden() || m_fstabAcknowledged->isChecked();
    m_label->setEnabled(blocked.isEmpty());
    m_save->setEnabled(blocked.isEmpty() && invalid.isEmpty() && changed && acknowledged);
    m_save->setText(m_label->text().isEmpty() ? tr("Remove Label") : tr("Save Label"));
    m_status->setText(!blocked.isEmpty() ? blocked : !invalid.isEmpty() ? invalid
        : tr("Changes the volume name only. Files, partitions and the filesystem UUID are preserved."));
}

void DriveLabelDialog::save() {
    if (!m_save->isEnabled() || m_saving) return;
    // Recheck fstab as well: it may have been edited while the dialog was open.
    if (DriveLabel::fstabUsesLabel(readFstab(), m_volume.label) && !m_fstabAcknowledged->isChecked()) {
        m_mountWarning->show();
        m_fstabAcknowledged->show();
        updateState();
        return;
    }
    m_saving = true;
    m_label->setEnabled(false);
    m_save->setEnabled(false);
    m_cancel->setEnabled(false);
    m_status->setText(tr("Rechecking volume and saving label… Approve the system authentication prompt if it appears."));
    m_service->save(m_volume, m_label->text());
}

void DriveLabelDialog::reject() {
    if (!m_saving) QDialog::reject();
}
