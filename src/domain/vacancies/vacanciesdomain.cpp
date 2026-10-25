#include "vacanciesdomain.h"
#include "../authoring/formparse.h"

#include "../../core/session.h"
#include "../../core/network.h"
#include "../../core/protocol.h"

#include <QUrlQuery>
#include <QHttpMultiPart>
#include "../../lang.h"
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

VacanciesDomain::VacanciesDomain(Session *session, Network *network, QObject *parent)
    : QObject(parent)
    , m_session(session)
    , m_network(network)
    , m_nam(network->nam())
{
}

QNetworkRequest VacanciesDomain::makeRequest(const QUrl &url) const
{
    return m_network->makeRequest(url);
}

void VacanciesDomain::fetchVacancyForm(int id)
{
    QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/vacans/edit.php"));
    QUrlQuery q;
    q.addQueryItem(QString::fromLatin1("id"), QString::number(id));
    url.setQuery(q);
    QNetworkReply *reply = m_nam->get(makeRequest(url));
    tagReply(reply, ActLoadVacancy, id, 0);
    connect(reply, SIGNAL(finished()), this, SLOT(onReplyFinished()));
}

void VacanciesDomain::saveVacancy(const VacancyForm &f)
{
    const bool edit = f.id > 0;
    // Add: id = project id. Edit: id = vacancy id (vacsEdit reads PostFormValue("id")).
    QUrl url(m_session->baseUrl() + QString::fromLatin1(edit ? "/server/133/send/vacsEdit.php"
                                                  : "/server/133/send/vacsAdd.php"));
    QHttpMultiPart *mp = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    addTextPart(mp, QStringLiteral("id"), QString::number(edit ? f.id : f.gdpsId));
    addTextPart(mp, QStringLiteral("title"), f.title);
    addTextPart(mp, QStringLiteral("text"), f.text);
    addTextPart(mp, QStringLiteral("short"), f.shortText);
    for (int i = 0; i < f.tags.size(); ++i)
        addTextPart(mp, QStringLiteral("tags[]"), QString::number(f.tags.at(i)));

    QNetworkReply *reply = m_nam->post(makeRequest(url), mp);
    mp->setParent(reply);
    tagReply(reply, ActSaveVacancy, f.id, f.gdpsId);
    connect(reply, SIGNAL(finished()), this, SLOT(onReplyFinished()));
}

void VacanciesDomain::removeVacancy(int id)
{
    QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/vacans/removeVac.php"));
    QUrlQuery q;
    q.addQueryItem(QString::fromLatin1("id"), QString::number(id));
    url.setQuery(q);
    QNetworkReply *reply = m_nam->get(makeRequest(url));
    tagReply(reply, ActRemoveVacancy, id, 0);
    connect(reply, SIGNAL(finished()), this, SLOT(onReplyFinished()));
}

void VacanciesDomain::applyVacancy(int id)
{
    QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/vacans/apply.php"));
    QUrlQuery q;
    q.addQueryItem(QString::fromLatin1("id"), QString::number(id));
    url.setQuery(q);
    QNetworkReply *reply = m_nam->get(makeRequest(url));
    tagReply(reply, ActApply, id, 0);
    connect(reply, SIGNAL(finished()), this, SLOT(onReplyFinished()));
}

void VacanciesDomain::unapplyVacancy(int applyId)
{
    QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/vacans/removeApl.php"));
    QUrlQuery q;
    q.addQueryItem(QString::fromLatin1("id"), QString::number(applyId));
    url.setQuery(q);
    QNetworkReply *reply = m_nam->get(makeRequest(url));
    tagReply(reply, ActUnapply, applyId, 0);
    connect(reply, SIGNAL(finished()), this, SLOT(onReplyFinished()));
}

void VacanciesDomain::probeVacancy(int id)
{
    // vacans/edit: VacsEditPre checks the project owner, non-owners get -2
    QUrl url(m_session->baseUrl() + QString::fromLatin1("/server/133/vacans/edit.php"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("id"), QString::number(id));
    url.setQuery(q);
    QNetworkReply *reply = m_nam->get(makeRequest(url));
    tagReply(reply, ActProbeVacancy, id, 0);
    connect(reply, SIGNAL(finished()), this, SLOT(onReplyFinished()));
}

void VacanciesDomain::onReplyFinished()
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
    case ActLoadVacancy: {
        const QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject()) {
            emit actionFinished(action, false, errorMessage(data), id);
            return;
        }
        emit vacancyFormLoaded(parseVacancyForm(doc.object()));
        return;
    }
    case ActSaveVacancy: {
        const QString code = answerError(data);
        if (!code.isEmpty()) {
            emit actionFinished(action, false, serverErrorText(code), id);
            return;
        }
        const int newId = id > 0 ? id : data.toInt();   // add: answer is the new vacancy id
        emit actionFinished(action, true, QString::fromUtf8("Vacancy saved"), newId);   // LANGXXX
        return;
    }
    case ActRemoveVacancy: {
        const QString code = answerError(data);
        if (!code.isEmpty()) {
            emit actionFinished(action, false, serverErrorText(code), id);
            return;
        }
        emit actionFinished(action, true, QString::fromUtf8("Vacancy deleted"), id);   // LANGXXX
        return;
    }
    case ActApply: {
        if (data == "-1") {
            emit actionFinished(action, false, QString::fromUtf8("You have already applied"), id);   // LANGXXX
            return;
        }
        const QString code = answerError(data);
        if (!code.isEmpty()) {
            emit actionFinished(action, false, serverErrorText(code), id);
            return;
        }
        emit actionFinished(action, true, L("vacResponded"), data.toInt()); 
        return;
    }
    case ActUnapply: {
        const QString code = answerError(data);
        if (!code.isEmpty()) {
            emit actionFinished(action, false, serverErrorText(code), id);
            return;
        }
        emit actionFinished(action, true, QString::fromUtf8("Application withdrawn"), id);   // LANGXXX
        return;
    }
    case ActProbeVacancy:
        emit vacancyOwnership(id, QJsonDocument::fromJson(data).isObject());
        return;
    default:
        return;
    }
}
