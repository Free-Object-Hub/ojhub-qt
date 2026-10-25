#ifndef OH_CORE_PROTOCOL_H
#define OH_CORE_PROTOCOL_H

#include <QByteArray>
#include <QString>
#include <QUrl>
#include <QJsonValue>
#include <QNetworkReply>
#include <QNetworkRequest>

class QHttpMultiPart;

// Shared protocol helpers: verbose [NET] logging, token lookup in JSON, ALTCHA solving.
void logNetReq(const char *tag, const QString &method, const QUrl &url, const QByteArray &body);
void logNetResp(const char *tag, QNetworkReply *reply, const QByteArray &data);
QString findTokenRecursive(const QJsonValue &v, int depth);
int solveAltchaFromJson(const QByteArray &jsonBytes, char *out_b64, int out_size);

// Server answers: the error code inside an answer, and the text shown to the user.
QString answerError(const QByteArray &b);
QString errorMessage(const QByteArray &data);
QString serverErrorText(const QString &code);

// Attaches the action, object id and channel to a reply, read back by the reply handler.
void tagReply(QNetworkReply *reply, int action, int id, int channel);

// Multipart form parts: a text field, and a file read from disk.
void addTextPart(QHttpMultiPart *mp, const QString &name, const QString &value);
void addFilePart(QHttpMultiPart *mp, const QString &name, const QString &path);

#endif
