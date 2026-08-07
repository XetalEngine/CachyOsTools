#include "mainwindow.h"

#include <QApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>
#include <QTranslator>

int main(int argc, char *argv[])
{
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
