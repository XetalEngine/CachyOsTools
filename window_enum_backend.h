#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

// Helpers run in separate processes so a stalled display server cannot freeze
// the GUI. Requests and replies are JSON; window metadata is never shell code.
QJsonObject windowEnumX11(const QJsonObject &request);
int windowEnumWayland();
void windowEnumProcessInfo(QJsonObject &window);
QString windowEnumCell(const QJsonValue &value);
bool windowEnumValidateAction(const QJsonObject &request, QString *error);
