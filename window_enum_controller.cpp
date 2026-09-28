#include "window_enum.h"
#include "window_enum_backend.h"
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QFile>
#include <QJsonDocument>
#include <QUuid>

namespace {
QDBusMessage call(const QString &service, const QString &path, const QString &interface,
                  const QString &method, const QVariantList &arguments = {}) {
    auto message = QDBusMessage::createMethodCall(service, path, interface, method);
    message.setArguments(arguments);
    return message;
}
}

WindowEnumController::WindowEnumController(QObject *parent) : QObject(parent)
{
    m_path = "/org/xetal/CachyOsTools/WindowEnum/w" +
        QUuid::createUuid().toString(QUuid::Id128);
    QDBusConnection::sessionBus().registerObject(m_path, this, QDBusConnection::ExportScriptableSlots);
    m_timeout.setSingleShot(true);
    m_timeout.setInterval(20000);
    connect(&m_timeout, &QTimer::timeout, this, [this]() {
        stop();
        complete({{"error", "Window enumeration timed out. Check the selected desktop backend and refresh."}});
    });
}

WindowEnumController::~WindowEnumController()
{
    stop();
    QDBusConnection::sessionBus().unregisterObject(m_path);
}

void WindowEnumController::stop()
{
    m_timeout.stop();
    m_token.clear();
    if (m_process) {
        auto *process = m_process;
        m_process = nullptr;
        process->disconnect(this);
        process->kill();
        process->waitForFinished(1000);
        process->deleteLater();
    }
    if (!m_scriptName.isEmpty()) {
        QDBusConnection::sessionBus().asyncCall(call("org.kde.KWin", "/Scripting",
            "org.kde.kwin.Scripting", "unloadScript", {m_scriptName}));
        m_scriptName.clear();
    }
    m_script.reset();
    m_buffer.clear();
    m_busy = false;
}

void WindowEnumController::setBackend(const QString &backend)
{
    stop();
    m_backend = backend;
    m_activeBackend.clear();
    emit busyChanged(false);
}

void WindowEnumController::complete(QJsonObject value)
{
    m_timeout.stop();
    m_busy = false;
    if (!m_scriptName.isEmpty()) {
        QDBusConnection::sessionBus().asyncCall(call("org.kde.KWin", "/Scripting",
            "org.kde.kwin.Scripting", "unloadScript", {m_scriptName}));
        m_scriptName.clear();
        m_script.reset();
    }
    if (value.contains("windows")) {
        QJsonArray windows;
        for (const auto &entry : value["windows"].toArray()) {
            auto row = entry.toObject();
            if (row.isEmpty() || row["id"].toString().isEmpty()) continue;
            if (!row.contains("executable")) windowEnumProcessInfo(row);
            windows.append(row);
        }
        value["windows"] = windows;
    }
    emit busyChanged(false);
    emit result(value);
}

void WindowEnumController::request(QJsonObject request)
{
    if (m_busy) return;
    const QString action = request["action"].toString();
    if (action != "list" && action != "details") {
        QString error;
        if (!windowEnumValidateAction(request, &error)) { complete({{"error", error}}); return; }
    }
    m_busy = true;
    emit busyChanged(true);
    m_timeout.start();
    m_token = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString backend = m_backend;
    if (backend == "auto") {
        const bool wayland = qEnvironmentVariable("XDG_SESSION_TYPE") == "wayland" ||
            !qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY");
        const QString desktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP").toLower();
        backend = !wayland ? "x11" : desktop.contains("kde") ? "kwin" :
            desktop.contains("gnome") ? "gnome" : "wayland";
    }
    if (m_activeBackend != backend && m_process) {
        // Only the foreign-toplevel helper persists between requests.
        auto *process = m_process;
        m_process = nullptr;
        process->disconnect(this);
        process->kill();
        process->deleteLater();
    }
    m_activeBackend = backend;
    if (backend == "kwin") kwin(request);
    else if (backend == "gnome") gnome(request);
    else helper(request, backend == "wayland");
}

void WindowEnumController::helper(const QJsonObject &request, bool wayland)
{
    if (wayland && m_process && m_process->state() == QProcess::Running) {
        m_process->write(QJsonDocument(request).toJson(QJsonDocument::Compact) + '\n');
        return;
    }
    if (wayland && request["action"] != "list") {
        complete({{"error", "The Wayland connection changed. Refresh and select the window again."}});
        return;
    }
    auto *process = new QProcess(this);
    m_process = process;
    m_buffer.clear();
    connect(process, &QProcess::readyReadStandardOutput, this, [this, process, wayland]() {
        if (m_process != process) return;
        m_buffer += process->readAllStandardOutput();
        if (!wayland) return;
        int newline;
        while ((newline = m_buffer.indexOf('\n')) >= 0) {
            const auto line = m_buffer.left(newline);
            m_buffer.remove(0, newline + 1);
            QJsonParseError error;
            const auto document = QJsonDocument::fromJson(line, &error);
            const auto value = error.error == QJsonParseError::NoError && document.isObject()
                ? document.object() : QJsonObject{{"error", "Invalid response from the window collector."}};
            process->setProperty("reportedError", value.contains("error"));
            complete(value);
        }
    });
    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
        if (m_process == process && error == QProcess::FailedToStart) {
            m_process = nullptr;
            complete({{"error", process->errorString()}});
            process->deleteLater();
        }
    });
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
        [this, process, wayland](int, QProcess::ExitStatus) {
        if (m_process != process) { process->deleteLater(); return; }
        m_process = nullptr;
        if (!wayland) {
            m_buffer += process->readAllStandardOutput();
            QJsonParseError error;
            const auto document = QJsonDocument::fromJson(m_buffer, &error);
            complete(error.error == QJsonParseError::NoError && document.isObject() ? document.object() :
                QJsonObject{{"error", "Window collector failed: " + QString::fromUtf8(process->readAllStandardError()).left(2000)}});
            m_buffer.clear();
        } else if (!process->property("reportedError").toBool())
            complete({{"error", "The Wayland collector stopped. Refresh to reconnect."}});
        process->deleteLater();
    });
    QStringList arguments;
    if (wayland) arguments << "--window-enum-wayland";
    else arguments << "--window-enum-x11" << QString::fromUtf8(QJsonDocument(request).toJson(QJsonDocument::Compact));
    process->start(QCoreApplication::applicationFilePath(), arguments);
}

void WindowEnumController::kwin(const QJsonObject &request)
{
    QFile source(":/window_enum/kwin.js");
    if (!source.open(QIODevice::ReadOnly)) { complete({{"error", "The KDE bridge is missing from this build."}}); return; }
    m_script.reset(new QTemporaryFile());
    if (!m_script->open()) { complete({{"error", "Could not create the temporary KDE script."}}); return; }
    m_scriptName = "cachyostools-window-enum-" + m_token;
    auto input = request;
    input["service"] = QDBusConnection::sessionBus().baseService();
    input["path"] = m_path;
    input["token"] = m_token;
    auto json = QJsonDocument(input).toJson(QJsonDocument::Compact);
    json.replace(QByteArray::fromHex("e280a8"), "\\u2028");
    json.replace(QByteArray::fromHex("e280a9"), "\\u2029");
    const auto contents = source.readAll() + "\nwindowEnumKWin(" + json + ");\n";
    if (m_script->write(contents) != contents.size() || !m_script->flush()) {
        complete({{"error", "Could not write the KDE bridge script."}}); return;
    }
    const QString token = m_token;
    auto *loaded = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(
        call("org.kde.KWin", "/Scripting", "org.kde.kwin.Scripting", "loadScript",
             {m_script->fileName(), m_scriptName}), 5000), this);
    connect(loaded, &QDBusPendingCallWatcher::finished, this, [this, loaded, token]() {
        QDBusPendingReply<int> reply = *loaded;
        loaded->deleteLater();
        if (token != m_token) return;
        if (reply.isError() || reply.value() < 0) {
            complete({{"error", "KWin scripting is unavailable: " + reply.error().message()}}); return;
        }
        const int id = reply.value();
        auto *started = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(
            call("org.kde.KWin", "/Scripting/Script" + QString::number(id), "org.kde.kwin.Script", "run"), 5000), this);
        connect(started, &QDBusPendingCallWatcher::finished, this, [this, started, id, token]() {
            QDBusPendingReply<> reply = *started;
            started->deleteLater();
            if (token != m_token || !m_busy || !reply.isError()) return;
            // Plasma 5 used /<script-id>; Plasma 6 uses /Scripting/Script<id>.
            if (reply.error().type() != QDBusError::UnknownObject && reply.error().type() != QDBusError::UnknownMethod) {
                complete({{"error", "KWin could not run the bridge: " + reply.error().message()}}); return;
            }
            auto *legacy = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(
                call("org.kde.KWin", "/" + QString::number(id), "org.kde.kwin.Script", "run"), 5000), this);
            connect(legacy, &QDBusPendingCallWatcher::finished, this, [this, legacy, token]() {
                QDBusPendingReply<> reply = *legacy;
                legacy->deleteLater();
                if (token == m_token && m_busy && reply.isError())
                    complete({{"error", "KWin could not run the bridge: " + reply.error().message()}});
            });
        });
    });
}

void WindowEnumController::Publish(const QString &token, const QString &json)
{
    if (!m_busy || token != m_token || m_activeBackend != "kwin") return;
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(json.toUtf8(), &error);
    complete(error.error == QJsonParseError::NoError && document.isObject()
        ? document.object() : QJsonObject{{"error", "Invalid KDE window response."}});
}

void WindowEnumController::gnome(const QJsonObject &request)
{
    const bool list = request["action"] == "list";
    const QString token = m_token;
    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(
        call("org.gnome.Shell", "/org/xetal/CachyOsTools/WindowEnum",
            "org.xetal.CachyOsTools.WindowEnum.Gnome", list ? "List" : "Act",
            list ? QVariantList() : QVariantList{QString::fromUtf8(QJsonDocument(request).toJson(QJsonDocument::Compact))}), 5000), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher, token]() {
        QDBusPendingReply<QString> reply = *watcher;
        watcher->deleteLater();
        if (token != m_token) return;
        if (reply.isError()) {
            complete({{"error", "GNOME access requires the Window Enum extension. Use Enable GNOME Bridge, then log out/in if GNOME has not discovered it yet. " + reply.error().message()}});
            return;
        }
        QJsonParseError error;
        const auto document = QJsonDocument::fromJson(reply.value().toUtf8(), &error);
        complete(error.error == QJsonParseError::NoError && document.isObject() ? document.object() :
            QJsonObject{{"error", "Invalid GNOME window response."}});
    });
}
