#ifndef OH_DOMAIN_PROJECTS_PROJECTSDOMAIN_H
#define OH_DOMAIN_PROJECTS_PROJECTSDOMAIN_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

#include "../../api/forms.h"

class Session;
class Network;

// Projects domain: edit form and save of a project (add or edit, with optional images).
class ProjectsDomain : public QObject
{
    Q_OBJECT
public:
    ProjectsDomain(Session *session, Network *network, QObject *parent = 0);

    void fetchProjectForm(int channel, int id);
    void saveProject(const ProjectForm &f, const QString &imgFile, const QString &banFile);

signals:
    void projectFormLoaded(const ProjectForm &form);
    void actionFinished(int action, bool ok, const QString &message, int id);
    // The owned-projects list may have changed: the ApiClient reloads the account.
    void accountRefreshRequested();

private slots:
    void onReplyFinished();

private:
    QNetworkRequest makeRequest(const QUrl &url) const;

    Session *m_session;
    Network *m_network;
    QNetworkAccessManager *m_nam;
};

#endif
