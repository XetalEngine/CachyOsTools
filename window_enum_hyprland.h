#pragma once
#include <QByteArray>
#include <QJsonObject>
#include <QStringList>
#include <functional>

struct HyprlandReply { QByteArray output; QString error; };
using HyprlandCommand = std::function<HyprlandReply(const QStringList &)>;
QJsonObject windowEnumHyprland(const QJsonObject &request, HyprlandCommand command = {});
QString windowEnumAutomaticBackend(const QString &session, const QString &display,
                                   const QString &desktop, const QString &hyprland);
