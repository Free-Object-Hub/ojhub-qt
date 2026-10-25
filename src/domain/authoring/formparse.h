#ifndef OH_DOMAIN_AUTHORING_FORMPARSE_H
#define OH_DOMAIN_AUTHORING_FORMPARSE_H

#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QMap>
#include <QList>
#include <QPair>
#include <QString>

#include "../../api/forms.h"

// Turns the server's edit-form answers into ProjectForm / VacancyForm. Shared by projects and vacancies.
QMap<QString, QString> parseMultiLang(const QString &raw, const QString &fallback);
QList<QPair<QString, QString> > parseLinks(const QJsonValue &v);
QList<int> idsFromJsonText(const QString &raw);
ProjectForm parseProjectForm(const QJsonArray &a, int channel, int id);
VacancyForm parseVacancyForm(const QJsonObject &o);

#endif
