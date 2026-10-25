#ifndef OH_CORE_NETWORK_H
#define OH_CORE_NETWORK_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QByteArray>
#include <QUrl>
#include <QString>

// Every request gets a watchdog: GETs abort after 30 s, writes after 120 s (uploads are slow).
class OhNetworkAccessManager : public QNetworkAccessManager
{
public:
    explicit OhNetworkAccessManager(QObject *parent = 0) : QNetworkAccessManager(parent) {}

protected:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &request,
                                 QIODevice *outgoingData = 0) override;
};

class Session;

// Shared HTTP entry: one NAM, auth headers from Session, optional blocking GET (ALTCHA).
class Network : public QObject
{
    Q_OBJECT
public:
    explicit Network(Session *session, QObject *parent = 0);

    Session *session() const;
    QNetworkAccessManager *nam() const;

    QNetworkRequest makeRequest(const QUrl &url) const;

    static void logReq(const char *tag, const QString &method, const QUrl &url, const QByteArray &body);
    static void logResp(const char *tag, QNetworkReply *reply, const QByteArray &data);

private:
    Session *m_session;
    QNetworkAccessManager *m_nam;
};

#endif
