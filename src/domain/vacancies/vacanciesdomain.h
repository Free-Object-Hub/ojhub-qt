#ifndef OH_DOMAIN_VACANCIES_VACANCIESDOMAIN_H
#define OH_DOMAIN_VACANCIES_VACANCIESDOMAIN_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

#include "../../api/forms.h"

class Session;
class Network;

// Vacancies domain: vacancy edit form, save, removal, applications and the owner check.
class VacanciesDomain : public QObject
{
    Q_OBJECT
public:
    VacanciesDomain(Session *session, Network *network, QObject *parent = 0);

    void fetchVacancyForm(int id);
    void saveVacancy(const VacancyForm &f);
    void removeVacancy(int id);
    void applyVacancy(int id);
    void unapplyVacancy(int applyId);
    void probeVacancy(int id);

signals:
    void vacancyFormLoaded(const VacancyForm &form);
    void vacancyOwnership(int id, bool owned);
    void actionFinished(int action, bool ok, const QString &message, int id);

private slots:
    void onReplyFinished();

private:
    QNetworkRequest makeRequest(const QUrl &url) const;

    Session *m_session;
    Network *m_network;
    QNetworkAccessManager *m_nam;
};

#endif
