#include "session.h"
#include "../settings.h"

#include <QUuid>

Session::Session(QObject *parent)
    : QObject(parent)
    , m_baseUrl(QString::fromLatin1("https://objecthub.xyz"))
    , m_userId(0)
{
}

void Session::setBaseUrl(const QString &url)
{
    m_baseUrl = url;
}

QString Session::baseUrl() const
{
    return m_baseUrl;
}

void Session::setUserToken(const QString &token)
{
    if (m_userToken == token)
        return;
    m_userToken = token;
    emit userChanged();
    emit loginStateChanged(isLoggedIn());
}

void Session::setDeviceToken(const QString &token)
{
    m_deviceToken = token;
}

QString Session::userToken() const
{
    return m_userToken;
}

QString Session::deviceToken() const
{
    return m_deviceToken;
}

void Session::setUserId(int id)
{
    if (m_userId == id)
        return;
    m_userId = id;
    emit userChanged();
}

int Session::userId() const
{
    return m_userId;
}

bool Session::isLoggedIn() const
{
    return !m_userToken.isEmpty() && m_userId > 0;
}

QString Session::ensureDeviceToken()
{
    if (!m_deviceToken.isEmpty())
        return m_deviceToken;
    m_deviceToken = AppSettings::deviceId();
    return m_deviceToken;
}

void Session::clearUser()
{
    m_userToken.clear();
    m_userId = 0;
    emit userChanged();
    emit loginStateChanged(false);
}
