#include "portfoliodomain.h"

#include "../../core/session.h"
#include "../../core/network.h"
#include "../../core/protocol.h"

#include <QUrlQuery>
#include <QHttpMultiPart>
#include <QNetworkReply>
#include <QDebug>

PortfolioDomain::PortfolioDomain(Session *session, Network *network, QObject *parent)
    : QObject(parent)
    , m_session(session)
    , m_network(network)
    , m_nam(network->nam())
{
}

QNetworkRequest PortfolioDomain::makeRequest(const QUrl &url) const
{
    return m_network->makeRequest(url);
}

// Texts for the plain-text errors of workAdd / workEdit (see zWorks.go)
QString workErrorText(const QByteArray &data)
{
    const QString t = QString::fromUtf8(data);
    if (t == QLatin1String("Bad title"))       return QString::fromUtf8("Title is required, at most 100 characters");
    if (t == QLatin1String("Bad text"))        return QString::fromUtf8("Text is required, at most 2000 characters");
    if (t == QLatin1String("Bad link"))        return QString::fromUtf8("Link is required, at most 255 characters");
    if (t == QLatin1String("Bad linkType"))    return QString::fromUtf8("Choose a link type");
    if (t == QLatin1String("Bad project id"))  return QString::fromUtf8("Project id must be a number");
    if (t == QLatin1String("Bad url"))         return QString::fromUtf8("The link must start with http:// or https://");
    if (t == QLatin1String("Project not found")) return QString::fromUtf8("Project not found");
    if (t == QLatin1String("Works limit reached")) return QString::fromUtf8("Portfolio limit reached (30 items)");
    if (t == QLatin1String("Internal error"))  return QString::fromUtf8("Server error, try again");
    if (t == QLatin1String("-1"))              return QString::fromUtf8("The server could not save the item");
    return errorMessage(data);
}

void PortfolioDomain::saveWork(const WorkForm &f)
{
    // workAdd / workEdit read multipart. Edit takes the id from the query.
    QUrl url(m_session->baseUrl() + QString::fromLatin1(f.id > 0 ? "/server/133/user/workEdit.php"
                                                      : "/server/133/user/workAdd.php"));
    if (f.id > 0) {
        QUrlQuery q;
        q.addQueryItem(QStringLiteral("id"), QString::number(f.id));
        url.setQuery(q);
    }
    QHttpMultiPart *mp = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    addTextPart(mp, QStringLiteral("title"), f.title);
    addTextPart(mp, QStringLiteral("text"), f.text);
    addTextPart(mp, QStringLiteral("gdpsId"), f.link);
    addTextPart(mp, QStringLiteral("linkType"), QString::number(f.linkType));
    QNetworkReply *reply = m_nam->post(makeRequest(url), mp);
    mp->setParent(reply);
    tagReply(reply, ActSaveWork, f.id, 0);
    connect(reply, SIGNAL(finished()), this, SLOT(onReplyFinished()));
}

void PortfolioDomain::deleteWork(int id)
{
    QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/user/workDelete.php"));
    QHttpMultiPart *mp = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    addTextPart(mp, QStringLiteral("id"), QString::number(id));
    QNetworkReply *reply = m_nam->post(makeRequest(url), mp);
    mp->setParent(reply);
    tagReply(reply, ActDeleteWork, id, 0);
    connect(reply, SIGNAL(finished()), this, SLOT(onReplyFinished()));
}

void PortfolioDomain::onReplyFinished()
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
    case ActSaveWork: {
        if (data.startsWith('[')) {
            emit actionFinished(action, true, QString::fromUtf8("Portfolio item saved"), id);
            return;
        }
        emit actionFinished(action, false, workErrorText(data), id);
        return;
    }
    case ActDeleteWork: {
        if (data == QByteArray::number(id)) {
            emit actionFinished(action, true, QString::fromUtf8("Portfolio item deleted"), id);
            return;
        }
        emit actionFinished(action, false, QString::fromUtf8("Could not delete the portfolio item"), id);
        return;
    }
    default:
        return;
    }
}
