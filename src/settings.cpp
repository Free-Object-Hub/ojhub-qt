#include "settings.h"

#include <QCoreApplication>
#include <QSettings>
#include <QUuid>

QString AppSettings::iniPath()
{
    return QCoreApplication::applicationDirPath() + QLatin1String("/ObjectHub.ini");
}

QString AppSettings::cacheDir()
{
    return QCoreApplication::applicationDirPath() + QLatin1String("/cache");
}

QString AppSettings::deviceId()
{
    QSettings s(iniPath(), QSettings::IniFormat);
    QString id = s.value(QLatin1String("device/id")).toString();
    if (id.isEmpty()) {
        // 32 hex chars, same shape as before; QUuid works on XP
        id = QUuid::createUuid().toString();
        id.remove(QLatin1Char('{'));
        id.remove(QLatin1Char('}'));
        id.remove(QLatin1Char('-'));
        s.setValue(QLatin1String("device/id"), id);
        s.sync();
    }
    return id;
}

void AppSettings::saveLogin(const QString &token, int userId)
{
    QSettings s(iniPath(), QSettings::IniFormat);
    s.setValue(QLatin1String("auth/token"), token);
    s.setValue(QLatin1String("auth/userId"), userId);
    s.sync();
}

void AppSettings::loadLogin(QString &token, int &userId)
{
    QSettings s(iniPath(), QSettings::IniFormat);
    token = s.value(QLatin1String("auth/token")).toString();
    userId = s.value(QLatin1String("auth/userId")).toInt();
}

void AppSettings::clearLogin()
{
    QSettings s(iniPath(), QSettings::IniFormat);
    s.remove(QLatin1String("auth"));
    s.sync();
}
