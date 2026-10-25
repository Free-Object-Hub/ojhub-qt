#include "resultsmodel.h"
#include "tagnames.h"
#include "lang.h"
#include "version.h"

#include <QNetworkRequest>
#include <QUrl>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QPalette>
#include <QStyle>
#include <QtGlobal>
#include <QNetworkDiskCache>

#include "settings.h"

static const int kAvatarSize = 48;
static const int kAvatarPad = 8;
static const qint64 kAvatarCacheBytes = 50LL * 1024 * 1024;  // disk cache limit
static const qint64 kMaxAvatarBytes = 2LL * 1024 * 1024;     // abort bigger images

static QString absoluteImgUrl(const QString &raw, const QString &base)
{
    if (raw.isEmpty())
        return QString();
    if (raw.startsWith(QStringLiteral("http://"))
        || raw.startsWith(QStringLiteral("https://")))
        return raw;
    if (raw.startsWith(QLatin1Char('/')))
        return base + raw;
    return base + QLatin1Char('/') + raw;
}

/* ---------------- ResultsModel ---------------- */

ResultsModel::ResultsModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    QNetworkDiskCache *cache = new QNetworkDiskCache(this);
    cache->setCacheDirectory(AppSettings::cacheDir());
    cache->setMaximumCacheSize(kAvatarCacheBytes);
    m_nam->setCache(cache);
}

ResultsModel::~ResultsModel()
{
    abortPending();
}

int ResultsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_results.size();
}

QVariant ResultsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    const int row = index.row();
    if (row < 0 || row >= m_results.size())
        return QVariant();

    switch (role) {
    case ResultRole:
        return QVariant::fromValue(m_results.at(row));
    case AvatarRole:
        return QVariant::fromValue(m_avatars.at(row));
    case Qt::DisplayRole:
        return m_results.at(row).title;
    default:
        return QVariant();
    }
}

// Why beginReset/endReset and not a fresh model per search: the view keeps its
// scroll and selection bookkeeping, and no per-card objects are allocated.
void ResultsModel::setResults(const SearchResultList &results, const QString &baseUrl)
{
    beginResetModel();
    abortPending();
    m_results = results;
    m_avatars.clear();
    for (int i = 0; i < m_results.size(); ++i)
        m_avatars.append(QPixmap());
    endResetModel();
    fetchAvatars(0, baseUrl);
}

void ResultsModel::appendResults(const SearchResultList &results, const QString &baseUrl)
{
    if (results.isEmpty())
        return;
    const int first = m_results.size();
    beginInsertRows(QModelIndex(), first, first + results.size() - 1);
    for (int i = 0; i < results.size(); ++i) {
        m_results.append(results.at(i));
        m_avatars.append(QPixmap());
    }
    endInsertRows();
    fetchAvatars(first, baseUrl);
}

void ResultsModel::fetchAvatars(int fromRow, const QString &baseUrl)
{
    for (int i = fromRow; i < m_results.size(); ++i) {
        const QString img = absoluteImgUrl(m_results.at(i).imageUrl, baseUrl);
        if (img.isEmpty())
            continue;
        QNetworkRequest req = QNetworkRequest(QUrl(img));
        req.setRawHeader("User-Agent", "ObjectHubNativeClient/" OH_VERSION);
        QNetworkReply *reply = m_nam->get(req);
        m_pending.insert(reply, QPersistentModelIndex(index(i, 0)));
        connect(reply, SIGNAL(finished()), this, SLOT(onAvatarFinished()));
        connect(reply, SIGNAL(readyRead()), this, SLOT(onAvatarReadyRead()));
    }
}

void ResultsModel::clear()
{
    beginResetModel();
    abortPending();
    m_results.clear();
    m_avatars.clear();
    endResetModel();
}

void ResultsModel::abortPending()
{
    const QList<QNetworkReply *> replies = m_pending.keys();
    for (int i = 0; i < replies.size(); ++i) {
        QNetworkReply *r = replies.at(i);
        r->disconnect(this);
        r->abort();
        r->deleteLater();
    }
    m_pending.clear();
}

void ResultsModel::onAvatarReadyRead()
{
    // Bytes are not read until finished, so bytesAvailable() is the running total
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (reply && reply->bytesAvailable() > kMaxAvatarBytes)
        reply->abort();
}

void ResultsModel::onAvatarFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;

    const QPersistentModelIndex idx = m_pending.take(reply);
    reply->deleteLater();

    if (!idx.isValid() || reply->error() != QNetworkReply::NoError)
        return;

    QPixmap pm;
    if (!pm.loadFromData(reply->readAll()))
        return;

    const int row = idx.row();
    if (row < 0 || row >= m_avatars.size())
        return;

    m_avatars[row] = pm.scaled(kAvatarSize, kAvatarSize,
                               Qt::KeepAspectRatio, Qt::SmoothTransformation);
    emit dataChanged(idx, idx);
}

/* ---------------- ResultDelegate ---------------- */

namespace {
struct Line {
    QString text;
    bool bold;
    bool dim;
};
}

static QList<Line> linesFor(const SearchResult &r)
{
    QList<Line> out;

    Line title = { r.title.isEmpty() ? L("noTitle") : r.title, true, false };
    out.append(title);

    QString by;
    if (!r.author.isEmpty())
        by = L("addedBy") + QStringLiteral(": ") + r.author;
    if (r.id > 0) {
        if (!by.isEmpty())
            by += QStringLiteral("   ");
        by += QStringLiteral("id: ") + QString::number(r.id);
    }
    if (!by.isEmpty()) {
        Line l = { by, false, true };
        out.append(l);
    }

    if (!r.shortDesc.isEmpty()) {
        QString t = r.shortDesc;
        if (t.size() > 300)
            t = t.left(300) + QString::fromUtf8("…");
        Line l = { t, false, false };
        out.append(l);
    }

    // Votes line is always shown (zeros included) so the card layout does not jump
    // between items with and without votes.
    QString votes = L("likesLine");
    votes.replace(QStringLiteral("%1"), QString::number(r.likes));
    votes.replace(QStringLiteral("%2"), QString::number(r.dislikes));
    votes.replace(QStringLiteral("%3"), QString::number(r.comments));
    Line vl = { votes, false, true };
    out.append(vl);

    if (!r.tags.isEmpty()) {
        QString tags = L("tagsLine");
        tags.replace(QStringLiteral("%1"), tagsText(r.channel, r.tags));
        Line tl = { tags, false, true };
        out.append(tl);
    }

    return out;
}

// Paints the lines when p is non-null; always returns the height they take.
static int layoutLines(QPainter *p, const QStyleOptionViewItem &opt,
                       const QList<Line> &lines, const QRect &area,
                       const QColor &normal, const QColor &dim)
{
    int y = area.top();
    for (int i = 0; i < lines.size(); ++i) {
        const Line &ln = lines.at(i);

        QFont f = opt.font;
        f.setBold(ln.bold);
        if (ln.bold && f.pointSize() > 0)
            f.setPointSize(f.pointSize() + 1);

        const QFontMetrics fm(f);
        const QRect slot(area.left(), y, area.width(), 10000);
        const QRect br = fm.boundingRect(slot, Qt::TextWordWrap, ln.text);

        if (p) {
            p->setFont(f);
            p->setPen(ln.dim ? dim : normal);
            p->drawText(QRect(area.left(), y, area.width(), br.height()),
                        Qt::TextWordWrap, ln.text);
        }
        y += br.height() + 2;
    }
    return y - area.top();
}

ResultDelegate::ResultDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void ResultDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const
{
    painter->save();

    const bool selected = option.state.testFlag(QStyle::State_Selected);
    if (selected)
        painter->fillRect(option.rect, option.palette.highlight());

    // Colours come from the system palette, so secondary text stays native
    // on every platform (no hard-coded stylesheet greys).
    const QColor normal = selected ? option.palette.highlightedText().color()
                                   : option.palette.text().color();
    const QColor dim = selected ? normal
                                : option.palette.color(QPalette::Disabled, QPalette::Text);

    const SearchResult r = index.data(ResultsModel::ResultRole).value<SearchResult>();

    const QRect av(option.rect.left() + kAvatarPad, option.rect.top() + kAvatarPad,
                   kAvatarSize, kAvatarSize);
    const QPixmap pm = index.data(ResultsModel::AvatarRole).value<QPixmap>();
    if (pm.isNull()) {
        painter->fillRect(av, option.palette.alternateBase());
        painter->setPen(dim);
        painter->drawText(av, Qt::AlignCenter, QStringLiteral("?"));
    } else {
        painter->drawPixmap(av, pm);
    }

    const int textLeft = av.right() + 10;
    const QRect area(textLeft, option.rect.top() + kAvatarPad,
                     option.rect.right() - textLeft - 8,
                     option.rect.height() - 2 * kAvatarPad);
    layoutLines(painter, option, linesFor(r), area, normal, dim);

    painter->restore();
}

QSize ResultDelegate::sizeHint(const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
    const SearchResult r = index.data(ResultsModel::ResultRole).value<SearchResult>();
    const int width = option.rect.width() > 0 ? option.rect.width() : 400;
    const int textLeft = kAvatarPad + kAvatarSize + 10;
    const QRect area(0, 0, width - textLeft - 8, 10000);
    const int textH = layoutLines(0, option, linesFor(r), area, QColor(), QColor());
    const int h = qMax(kAvatarSize + 2 * kAvatarPad, textH + 2 * kAvatarPad);
    return QSize(width, h);
}
