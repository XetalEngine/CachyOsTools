#include "mainwindow.h"

#include <QApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>
#include <QTranslator>
#include <QJsonDocument>
#include "window_enum_backend.h"
#include <cstdio>
#include <cstring>

int main(int argc, char *argv[])
{
    if (argc > 1 && (std::strcmp(argv[1], "--window-enum-x11") == 0 ||
                    std::strcmp(argv[1], "--window-enum-wayland") == 0)) {
        QCoreApplication helper(argc, argv);
        if (std::strcmp(argv[1], "--window-enum-wayland") == 0) return windowEnumWayland();
        QJsonParseError error;
        const auto request = QJsonDocument::fromJson(argc > 2 ? QByteArray(argv[2]) : QByteArray(), &error);
        const auto result = error.error == QJsonParseError::NoError && request.isObject()
            ? windowEnumX11(request.object()) : QJsonObject{{"error", "Invalid window request."}};
        const auto output = QJsonDocument(result).toJson(QJsonDocument::Compact) + '\n';
        std::fwrite(output.constData(), 1, size_t(output.size()), stdout);
        return result.contains("error") ? 1 : 0;
    }
    QApplication a(argc, argv);

    // Language: Dashboard flag dropdown persists the choice; applied on startup.
    // CACHYOSTOOLS_LANG=pt_BR|de|en overrides the saved setting (handy for testing).
    QString lang = qEnvironmentVariable("CACHYOSTOOLS_LANG");
    if (lang.isEmpty()) {
        lang = QSettings("CachyOsTools", "CachyOsTools").value("language/code", "en").toString();
    }
    if (lang != "en") {
        QTranslator *qtTranslator = new QTranslator(&a);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        const QString qtTrDir = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
#else
        const QString qtTrDir = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#endif
        if (qtTranslator->load(QLocale(lang), "qtbase", "_", qtTrDir)) {
            a.installTranslator(qtTranslator);
        }
        QTranslator *appTranslator = new QTranslator(&a);
        if (appTranslator->load(":/i18n/cachyostools_" + lang + ".qm")) {
            a.installTranslator(appTranslator);
        }
    }

    MainWindow w;
    w.show();
    return a.exec();
}
