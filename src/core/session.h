#ifndef OH_CORE_SESSION_H
#define OH_CORE_SESSION_H

#include <QObject>
#include <QString>

// Identity and tokens only. No network, no domain data.
class Session : public QObject
{
    Q_OBJECT
public:
    explicit Session(QObject *parent = 0);

    void setBaseUrl(const QString &url);
    QString baseUrl() const;

    void setUserToken(const QString &token);
    void setDeviceToken(const QString &token);
    QString userToken() const;
    QString deviceToken() const;

    void setUserId(int id);
    int userId() const;

    bool isLoggedIn() const;

    // Persistent device id next to the binary (ObjectHub.ini via settings).
    QString ensureDeviceToken();

    void clearUser();

signals:
    void userChanged();
    void loginStateChanged(bool loggedIn);

private:
    QString m_baseUrl;
    QString m_userToken;
    QString m_deviceToken;
    int m_userId;
};

#endif
