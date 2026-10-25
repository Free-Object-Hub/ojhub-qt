#include "projectsdomain.h"
#include "../authoring/formparse.h"
#include "../../lang.h"

#include "../../core/session.h"
#include "../../core/network.h"
#include "../../core/protocol.h"

#include <QUrlQuery>
#include <QHttpMultiPart>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

namespace {
const char *kLangCodes[] = { "RU", "EN", "UA" };

// Project save endpoint: send/{camp,show,pere,tele}{Add,Edit}.php
QString projectSendPath(int channel, bool edit)
{
    static const char *names[] = { "camp", "show", "pere", "tele" };
    const int ch = (channel >= 0 && channel <= 3) ? channel : 0;
    return QString::fromLatin1("/server/133/send/") + QString::fromLatin1(names[ch])
         + QString::fromLatin1(edit ? "Edit.php" : "Add.php");
}

}

ProjectsDomain::ProjectsDomain(Session *session, Network *network, QObject *parent)
    : QObject(parent)
    , m_session(session)
    , m_network(network)
    , m_nam(network->nam())
{
}

QNetworkRequest ProjectsDomain::makeRequest(const QUrl &url) const
{
    return m_network->makeRequest(url);
}

void ProjectsDomain::fetchProjectForm(int channel, int id)
{
    QUrl url(m_session->baseUrl() + projectSendPath(channel, true));
    QUrlQuery q;
    q.addQueryItem(QString::fromLatin1("id"), QString::number(id));
    url.setQuery(q);
    QNetworkReply *reply = m_nam->get(makeRequest(url));
    tagReply(reply, ActLoadProject, id, channel);
    connect(reply, SIGNAL(finished()), this, SLOT(onReplyFinished()));
}

void ProjectsDomain::saveProject(const ProjectForm &f, const QString &imgFile, const QString &banFile)
{
    const bool edit = f.id > 0;
    QUrl url(m_session->baseUrl() + projectSendPath(f.channel, edit));
    if (edit) {
        QUrlQuery q;
        q.addQueryItem(QString::fromLatin1("id"), QString::number(f.id));
        url.setQuery(q);
    }

    QHttpMultiPart *mp = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    addTextPart(mp, QStringLiteral("title"), f.title);
    // Empty language = multi-language form (description/short as "LANG\ttext" parts)
    addTextPart(mp, QStringLiteral("language"), QString());
    for (unsigned i = 0; i < sizeof(kLangCodes) / sizeof(kLangCodes[0]); ++i) {
        const QString lang = QString::fromLatin1(kLangCodes[i]);
        const QString d = f.description.value(lang).trimmed();
        if (!d.isEmpty())
            addTextPart(mp, QStringLiteral("description"), lang + QLatin1Char('\t') + d);
        const QString s = f.shortText.value(lang).trimmed();
        if (!s.isEmpty())
            addTextPart(mp, QStringLiteral("short"), lang + QLatin1Char('\t') + s);
    }
    for (int i = 0; i < f.links.size(); ++i) {
        addTextPart(mp, QStringLiteral("links[]"), f.links.at(i).first);
        addTextPart(mp, QStringLiteral("links[]"), f.links.at(i).second);
    }
    for (int i = 0; i < f.tags.size(); ++i)
        addTextPart(mp, QStringLiteral("tags[]"), QString::number(f.tags.at(i)));
    for (int i = 0; i < f.os.size(); ++i)
        addTextPart(mp, QStringLiteral("os[]"), QString::number(f.os.at(i)));

    // A picked file wins over the url field. On edit an empty url keeps the stored image.
    if (!imgFile.isEmpty()) addFilePart(mp, QStringLiteral("img"), imgFile);
    else                    addTextPart(mp, QStringLiteral("img"), f.img);
    if (!banFile.isEmpty()) addFilePart(mp, QStringLiteral("ban"), banFile);
    else                    addTextPart(mp, QStringLiteral("ban"), f.ban);

    QNetworkReply *reply = m_nam->post(makeRequest(url), mp);
    mp->setParent(reply);
    tagReply(reply, ActSaveProject, f.id, f.channel);
    connect(reply, SIGNAL(finished()), this, SLOT(onReplyFinished()));
}

void ProjectsDomain::onReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;
    reply->deleteLater();

    const int action = reply->property("action").toInt();
    const int id = reply->property("id").toInt();
    const int channel = reply->property("channel").toInt();
    const QByteArray data = reply->readAll().trimmed();
    qDebug().noquote() << "[NET] WRITE action" << action << "id" << id
                       << "HTTP" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
                       << QString::fromUtf8(data.left(300));

    if (reply->error() != QNetworkReply::NoError) {
        emit actionFinished(action, false, reply->errorString(), id);
        return;
    }

    switch (action) {
    case ActLoadProject: {
        const QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isArray() || doc.array().size() < 9) {
            emit actionFinished(action, false, errorMessage(data), id);
            return;
        }
        emit projectFormLoaded(parseProjectForm(doc.array(), channel, id));
        return;
    }
    case ActSaveProject: {
        const QString code = answerError(data);
        if (!code.isEmpty()) {
            emit actionFinished(action, false, serverErrorText(code), id);
            return;
        }
        emit accountRefreshRequested();   // ownership of the new or edited project may have changed
        // Add returns InitOjhub, not the new id: the dialog layer opens the user's list instead.
        emit actionFinished(action, true, QString::fromUtf8("Project saved"), id);   // LANGXXX
        return;
    }
    default:
        return;
    }
}
