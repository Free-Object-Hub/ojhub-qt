#include "mainwindow.h"
#include "lang.h"
#include "version.h"
#include "api/searchresult.h"

#include <QApplication>
#include <QDateTime>
#include <QMetaType>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName(QStringLiteral("ObjectHubClient"));
    app.setOrganizationName(QStringLiteral("ObjectHub"));
    app.setApplicationVersion(QStringLiteral(OH_VERSION));

    qRegisterMetaType<SearchResultList>("SearchResultList");
    qRegisterMetaType<PublicProfile>("PublicProfile");
    qRegisterMetaType<DetailData>("DetailData");

    // Strings are compiled into the binary (Qt resource), so this works from any
    // cwd and on Android, where there is no filesystem path to RU.json.
    Lang::instance().loadFromJsonFile(QStringLiteral(":/lang/RU.json"));

#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
    qsrand(static_cast<uint>(QDateTime::currentMSecsSinceEpoch() & 0xffffffff));
#endif

    MainWindow w;
    w.show();

    return app.exec();
}
