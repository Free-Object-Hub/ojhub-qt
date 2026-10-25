#include "newsdomain.h"

#include "../../core/session.h"
#include "../../core/network.h"
#include "../../core/protocol.h"

#include <QUrlQuery>
#include <QHttpMultiPart>
#include <QNetworkReply>
#include <QDebug>

NewsDomain::NewsDomain(Session *session, Network *network, QObject *parent)
    : QObject(parent)
    , m_session(session)
    , m_network(network)
    , m_nam(network->nam())
{
}

QNetworkRequest NewsDomain::makeRequest(const QUrl &url) const
{
    return m_network->makeRequest(url);
}

void NewsDomain::saveNews(const NewsForm &f, const QString &filePath)
{
    if (f.id > 0) {
        // newsModify calls ParseForm before reading fields, so the body must be urlencoded.
        QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/send/newsModify.php"));
        QUrlQuery body;
        body.addQueryItem(QStringLiteral("id"), QString::number(f.id));
        body.addQueryItem(QStringLiteral("gdps"), QString::number(f.gdpsId));
        body.addQueryItem(QStringLiteral("title"), f.title);
        body.addQueryItem(QStringLiteral("text"), f.text);
        QNetworkRequest req = makeRequest(url);
        req.setHeader(QNetworkRequest::ContentTypeHeader,
                      QVariant(QString::fromLatin1("application/x-www-form-urlencoded")));
        QNetworkReply *reply = m_nam->post(req, body.query(QUrl::FullyEncoded).toUtf8());
        tagReply(reply, ActSaveNews, f.id, f.gdpsId);
        connect(reply, SIGNAL(finished()), this, SLOT(onReplyFinished()));
        return;
    }

    // AddNews reads multipart: gdps, title, text and an optional picture under "files".
    QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/send/newsPost.php"));
    QHttpMultiPart *mp = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    addTextPart(mp, QStringLiteral("gdps"), QString::number(f.gdpsId));
    addTextPart(mp, QStringLiteral("title"), f.title);
    addTextPart(mp, QStringLiteral("text"), f.text);
    if (!filePath.isEmpty())
        addFilePart(mp, QStringLiteral("files"), filePath);
    QNetworkReply *reply = m_nam->post(makeRequest(url), mp);
    mp->setParent(reply);
    tagReply(reply, ActSaveNews, 0, f.gdpsId);
    connect(reply, SIGNAL(finished()), this, SLOT(onReplyFinished()));
}

void NewsDomain::deleteNews(int id)
{
    QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/delete/newsPost.php"));
    QUrlQuery q;
    q.addQueryItem(QString::fromLatin1("ide"), QString::number(id));
    url.setQuery(q);
    QNetworkReply *reply = m_nam->get(makeRequest(url));
    tagReply(reply, ActDeleteNews, id, 0);
    connect(reply, SIGNAL(finished()), this, SLOT(onReplyFinished()));
}

void NewsDomain::probeNews(int id, int projectId)
{
    // NewsEdit checks access before the title, and returns nothing when the title is empty.
    // Owners therefore get an empty answer and nothing is changed; others get -2.
    QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/send/newsModify.php"));
    QUrlQuery body;
    body.addQueryItem(QStringLiteral("id"), QString::number(id));
    body.addQueryItem(QStringLiteral("gdps"), QString::number(projectId));
    body.addQueryItem(QStringLiteral("title"), QString());
    body.addQueryItem(QStringLiteral("text"), QString());
    QNetworkRequest req = makeRequest(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QVariant(QString::fromLatin1("application/x-www-form-urlencoded")));
    QNetworkReply *reply = m_nam->post(req, body.query(QUrl::FullyEncoded).toUtf8());
    tagReply(reply, ActProbeNews, id, 0);
    connect(reply, SIGNAL(finished()), this, SLOT(onReplyFinished()));
}

void NewsDomain::onReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;
    reply->deleteLater();

    const int action = reply->property("action").toInt();
    const int id = reply->property("id").toInt();
    const QByteArray data = reply->readAll().trimmed();
    qDebug().noquote() << "[NET] WRITE action" << action << "id" << id
                       << "HTTP" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
                       << QString::fromUtf8(data.left(300));

    if (reply->error() != QNetworkReply::NoError) {
        emit actionFinished(action, false, reply->errorString(), id);
        return;
    }

    switch (action) {
    case ActSaveNews: {
        const QString code = answerError(data);
        if (!code.isEmpty()) {
            emit actionFinished(action, false, serverErrorText(code), id);
            return;
        }
        const int newId = id > 0 ? id : data.toInt();   // add: answer is the new news id
        emit actionFinished(action, true, QString::fromUtf8("News saved"), newId);   // LANGXXX
        return;
    }
    case ActDeleteNews: {
        if (id > 0 && data == QByteArray::number(id)) {
            emit actionFinished(action, true, QString::fromUtf8("News deleted"), id);   // LANGXXX
            return;
        }
        emit actionFinished(action, false, serverErrorText(QStringLiteral("-2")), id);
        return;
    }
    case ActProbeNews:
        // Owners get an empty answer (nothing written, empty title); others get -2.
        emit newsOwnership(id, data.isEmpty());
        return;
    default:
        return;
    }
}
