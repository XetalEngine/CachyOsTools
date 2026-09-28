#include "window_enum_backend.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSet>
#include <pwd.h>
#include <sys/stat.h>
#include <cmath>

void windowEnumProcessInfo(QJsonObject &window)
{
    const int pid = window.value("pid").toInt();
    if (pid <= 0 || window.value("remote").toBool()) return;
    const QString path = "/proc/" + QString::number(pid);
    const QString executable = QFileInfo(path + "/exe").symLinkTarget();
    if (!executable.isEmpty()) window["executable"] = executable;
    struct stat info {};
    if (::stat(QFile::encodeName(path).constData(), &info) == 0) {
        window["uid"] = int(info.st_uid);
        if (const passwd *entry = getpwuid(info.st_uid))
            window["user"] = QString::fromLocal8Bit(entry->pw_name);
    }
    QFile command(path + "/cmdline");
    if (command.open(QIODevice::ReadOnly)) {
        QByteArray bytes = command.read(65536);
        bytes.replace('\0', ' ');
        window["command"] = QString::fromLocal8Bit(bytes).trimmed();
    }
}

QString windowEnumCell(const QJsonValue &value)
{
    if (value.isNull() || value.isUndefined()) return QStringLiteral("—");
    if (value.isBool()) return value.toBool() ? QStringLiteral("Yes") : QStringLiteral("No");
    if (value.isDouble()) return QString::number(value.toDouble(), 'g', 12);
    if (value.isString()) return value.toString();
    if (value.isArray()) {
        QStringList values;
        for (const auto &item : value.toArray()) values << windowEnumCell(item);
        return values.join(", ");
    }
    return QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
}

bool windowEnumValidateAction(const QJsonObject &request, QString *error)
{
    static const QSet<QString> actions = {
        "geometry", "activate", "raise", "lower", "above", "below", "minimized",
        "maximized", "fullscreen", "opacity", "desktop", "close", "title", "decorated"
    };
    const QString action = request.value("action").toString();
    auto fail = [error](const QString &message) { *error = message; return false; };
    if (!actions.contains(action) || request.value("id").toString().isEmpty())
        return fail("Unknown action or missing window ID.");
    auto number = [&request](const char *key, double low, double high) {
        const auto value = request.value(key);
        const double n = value.toDouble();
        return value.isDouble() && std::isfinite(n) && n >= low && n <= high && n == std::floor(n);
    };
    if (action == "geometry" &&
        (!number("x", -32768, 32767) || !number("y", -32768, 32767) ||
         !number("width", 1, 65535) || !number("height", 1, 65535)))
        return fail("Position must be -32768…32767; dimensions must be 1…65535.");
    if (action == "geometry") {
        for (const char *key : {"move", "resize"})
            if (request.contains(key) && !request.value(key).isBool())
                return fail("Invalid geometry operation.");
        if (!request.value("move").toBool(true) && !request.value("resize").toBool(true))
            return fail("Choose a move or resize operation.");
    }
    if (action == "opacity" && !number("value", 0, 100))
        return fail("Opacity must be 0…100.");
    if (action == "desktop" && !number("value", -1, 999))
        return fail("Workspace must be -1 (all) or a zero-based index up to 999.");
    if (QSet<QString>{"above", "below", "minimized", "maximized", "fullscreen", "decorated"}.contains(action)
        && !request.value("value").isBool())
        return fail("This action needs a boolean value.");
    if (action == "title" && (!request.value("value").isString()
        || request.value("value").toString().contains(QChar('\0'))
        || request.value("value").toString().size() > 4096))
        return fail("Invalid window title.");
    return true;
}
