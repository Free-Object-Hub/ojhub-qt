#include "protocol.h"
#include "../altcha/altcha.h"
#include "../api/forms.h"

#include <QNetworkRequest>
#include <QVariant>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

int solveAltchaFromJson(const QByteArray &jsonBytes, char *out_b64, int out_size)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonBytes, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return -1;

    QJsonObject o = doc.object();
    QByteArray algo = o.value(QString::fromLatin1("algorithm")).toString().toUtf8();
    QByteArray challenge = o.value(QString::fromLatin1("challenge")).toString().toUtf8();
    QByteArray salt = o.value(QString::fromLatin1("salt")).toString().toUtf8();
    QByteArray signature = o.value(QString::fromLatin1("signature")).toString().toUtf8();
    int maxNumber = o.value(QString::fromLatin1("maxNumber")).toInt(100000);
    if (maxNumber <= 0)
        maxNumber = 100000;

    if (algo.isEmpty())
        algo = "SHA-256";
    if (challenge.isEmpty() || salt.isEmpty() || signature.isEmpty())
        return -1;

    unsigned int number = 0;
    if (!oh_altcha_solve(challenge.constData(), salt.constData(),
                         (unsigned int)maxNumber, &number, 0))
        return -2;

    return oh_altcha_make_payload_b64(
        algo.constData(),
        challenge.constData(),
        number,
        salt.constData(),
        signature.constData(),
        out_b64,
        out_size);
}

/* ---- verbose network log (stderr / qDebug) ---- */
void logNetReq(const char *tag, const QString &method, const QUrl &url, const QByteArray &body)
{
    qDebug().noquote() << QString::fromLatin1("[NET]") << tag
                       << method << url.toString();
    if (!url.query().isEmpty())
        qDebug().noquote() << QString::fromLatin1("[NET]  query:") << url.query();
    if (!body.isEmpty()) {
        QByteArray show = body;
        /* redacted password field for console */
        QByteArray red = show;
        {
            QUrlQuery q;
            q.setQuery(QString::fromUtf8(show));
            if (q.hasQueryItem(QString::fromLatin1("password"))) {
                q.removeAllQueryItems(QString::fromLatin1("password"));
                q.addQueryItem(QString::fromLatin1("password"), QString::fromLatin1("***"));
                red = q.query(QUrl::FullyEncoded).toUtf8();
            }
        }
        if (red.size() > 800)
            red = red.left(800) + "...";
        qDebug().noquote() << QString::fromLatin1("[NET]  body:") << QString::fromUtf8(red);
    }
}

void logNetResp(const char *tag, QNetworkReply *reply, const QByteArray &data)
{
    const int http = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug().noquote() << QString::fromLatin1("[NET]") << tag
                       << QString::fromLatin1("HTTP") << http
                       << QString::fromLatin1("qterr=") << (int)reply->error()
                       << reply->errorString();
    QByteArray show = data;
    if (show.size() > 2000)
        show = show.left(2000) + "\n...[truncated]...";
    qDebug().noquote() << QString::fromLatin1("[NET]  body:") << QString::fromUtf8(show);
}

/* Walk JSON for a string field named token / userToken / Token */
QString findTokenRecursive(const QJsonValue &v, int depth)
{
    if (depth > 6)
        return QString();
    if (v.isObject()) {
        QJsonObject o = v.toObject();
        const char *keys[] = { "token", "userToken", "user_token", "Token", "TOKEN", 0 };
        for (int i = 0; keys[i]; ++i) {
            QString k = QString::fromLatin1(keys[i]);
            if (o.contains(k) && o.value(k).isString()) {
                QString s = o.value(k).toString();
                if (!s.isEmpty())
                    return s;
            }
        }
        QStringList ks = o.keys();
        for (int i = 0; i < ks.size(); ++i) {
            QString found = findTokenRecursive(o.value(ks.at(i)), depth + 1);
            if (!found.isEmpty())
                return found;
        }
    } else if (v.isArray()) {
        QJsonArray a = v.toArray();
        for (int i = 0; i < a.size(); ++i) {
            QString found = findTokenRecursive(a.at(i), depth + 1);
            if (!found.isEmpty())
                return found;
        }
    }
    return QString();
}

// ---- shared answer and reply helpers ----

// Server answer for a write: "" = success, otherwise an error code or raw text.
// Success shapes: JSON array (InitOjhub after project save), a positive number (new id, or "1").
QString answerError(const QByteArray &b)
{
    if (b.isEmpty())
        return QStringLiteral("EMPTY");
    if (b.startsWith('['))
        return QString();
    if (b.startsWith('{')) {   // ApiError: {"error":"...","code":"-28"}
        return QJsonDocument::fromJson(b).object().value(QStringLiteral("code")).toString();
    }
    bool ok = false;
    const int v = b.toInt(&ok);
    if (ok && v > 0)
        return QString();
    return QString::fromUtf8(b);
}

QString errorMessage(const QByteArray &data)
{
    QString code = answerError(data);
    if (code.isEmpty())
        code = QStringLiteral("NONE");
    return serverErrorText(code);
}

QString serverErrorText(const QString &code)
{
    // LANGXXX: все сообщения ниже захардкожены, ключей в RU.json для них нет
    if (code == QLatin1String("-1"))  return QString::fromUtf8("The server could not save the data");
    if (code == QLatin1String("-2"))  return QString::fromUtf8("No access to this object");
    if (code == QLatin1String("-3"))  return QString::fromUtf8("Image rejected or object not found");
    if (code == QLatin1String("-4"))  return QString::fromUtf8("Fill in the fields");
    if (code == QLatin1String("-5"))  return QString::fromUtf8("Form data error");
    if (code == QLatin1String("-6"))  return QString::fromUtf8("Object not found");
    if (code == QLatin1String("-7"))  return QString::fromUtf8("Invalid id");
    if (code == QLatin1String("-8"))  return QString::fromUtf8("Database error");
    if (code == QLatin1String("-9"))  return QString::fromUtf8("Image rejected (PNG or JPEG up to 5 MB)");
    if (code == QLatin1String("-27")) return QString::fromUtf8("Fill in the title and description");
    if (code == QLatin1String("-28")) return QString::fromUtf8("Invalid description");
    if (code == QLatin1String("-29")) return QString::fromUtf8("Short description is longer than 200 characters");
    if (code == QLatin1String("Access denied")) return QString::fromUtf8("Access denied, please log in again");
    if (code == QLatin1String("NONE"))  return QString::fromUtf8("Object not found");
    if (code == QLatin1String("EMPTY")) return QString::fromUtf8("Empty server response");
    return QString::fromUtf8("Server error: %1").arg(code);
}

// Attaches the action, object id and channel to a reply, read back in onWriteReplyFinished().
void tagReply(QNetworkReply *reply, int action, int id, int channel)
{
    reply->setProperty("action", action);
    reply->setProperty("id", id);
    reply->setProperty("channel", channel);
}

// ---- multipart helpers ----

void addTextPart(QHttpMultiPart *mp, const QString &name, const QString &value)
{
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader,
                   QVariant(QString::fromLatin1("form-data; name=\"%1\"").arg(name)));
    part.setBody(value.toUtf8());
    mp->append(part);
}

void addFilePart(QHttpMultiPart *mp, const QString &name, const QString &path)
{
    QFile *file = new QFile(path, mp);   // owned by the multipart, which lives with the reply
    if (!file->open(QIODevice::ReadOnly))
        return;
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader,
                   QVariant(QString::fromLatin1("form-data; name=\"%1\"; filename=\"%2\"")
                                .arg(name, QFileInfo(path).fileName())));
    part.setHeader(QNetworkRequest::ContentTypeHeader,
                   QVariant(QString::fromLatin1("application/octet-stream")));
    part.setBodyDevice(file);
    mp->append(part);
}
