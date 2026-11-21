#pragma once
#include <QMap>
#include <QString>
#include <QXmlStreamReader>

namespace NetworkStatus {
// NetworkManager's device output contains active addresses, not ipv4.method.
// Read the method from the active connection profile, identified by UUID.
inline QString addressingLabel(const QString &method) {
    static const QMap<QString, QString> labels{{"auto", "Automatic"}, {"manual", "Manual"},
        {"link-local", "Link-local"}, {"shared", "Shared"}, {"disabled", "IPv4 off"}};
    return labels.value(method.trimmed(), "Not reported");
}
inline QString virtualNetworkMode(const QString &definition) {
    QXmlStreamReader xml(definition); QString mode = "isolated";
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name() == QLatin1String("forward")) {
            mode = xml.attributes().value("mode").toString();
            if (mode.isEmpty()) mode = "nat";
        }
    }
    if (xml.hasError()) return "Not reported";
    static const QMap<QString, QString> labels{{"isolated", "Isolated"}, {"nat", "Private (NAT)"},
        {"bridge", "Bridge"}, {"route", "Routed"}, {"open", "Open"}};
    return labels.value(mode, mode);
}
}
