#pragma once

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QFileInfo>
#include <QMessageBox>
#include <QPointer>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>

namespace FileManager {
namespace detail {
inline void openWithInstalledManager(QWidget *parent, const QString &directory)
{
    // Only file managers belong here. A generic URL/MIME opener can choose a
    // music player that advertises inode/directory (for importing music).
    const QStringList managers = {QStringLiteral("dolphin"), QStringLiteral("thunar"),
        QStringLiteral("nautilus"), QStringLiteral("nemo"), QStringLiteral("caja"),
        QStringLiteral("pcmanfm-qt"), QStringLiteral("pcmanfm")};
    for (const QString &manager : managers) {
        const QString executable = QStandardPaths::findExecutable(manager);
        if (!executable.isEmpty() && QProcess::startDetached(executable, {directory}))
            return;
    }

    QMessageBox::warning(parent, QCoreApplication::translate("FileManager", "Cannot Open Folder"),
        QCoreApplication::translate("FileManager",
            "No file manager could be started to open:\n%1\n\n"
            "Install a file manager such as Dolphin, Thunar, or Nautilus and try again.")
            .arg(directory));
}
} // namespace detail

inline void openFolder(QWidget *parent, const QString &directory)
{
    const QFileInfo info(directory);
    if (directory.isEmpty() || !info.isDir()) {
        QMessageBox::warning(parent, QCoreApplication::translate("FileManager", "Cannot Open Folder"),
            QCoreApplication::translate("FileManager", "This folder does not exist or is unavailable:\n%1")
                .arg(directory));
        return;
    }
    const QString absoluteDirectory = info.absoluteFilePath();

    // Ask the desktop's file manager directly, bypassing inode/directory and
    // OpenURI handlers. ShowFolders opens the directory, not its parent.
    // https://www.freedesktop.org/wiki/Specifications/file-manager-interface/
    const QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        detail::openWithInstalledManager(parent, absoluteDirectory);
        return;
    }
    QDBusMessage request = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.FileManager1"),
        QStringLiteral("/org/freedesktop/FileManager1"),
        QStringLiteral("org.freedesktop.FileManager1"), QStringLiteral("ShowFolders"));
    request << QStringList{QUrl::fromLocalFile(absoluteDirectory).toString(QUrl::FullyEncoded)}
            << QString();

    // Keep the UI responsive if the desktop service is slow or unavailable.
    QObject *owner = parent ? static_cast<QObject *>(parent) : QCoreApplication::instance();
    auto *watcher = new QDBusPendingCallWatcher(bus.asyncCall(request, 5000), owner);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, watcher,
        [parent = QPointer<QWidget>(parent), absoluteDirectory](QDBusPendingCallWatcher *call) {
            const QDBusPendingReply<> reply = *call;
            call->deleteLater();
            if (reply.isError())
                detail::openWithInstalledManager(parent.data(), absoluteDirectory);
        });
}
} // namespace FileManager
