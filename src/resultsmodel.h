#ifndef RESULTSMODEL_H
#define RESULTSMODEL_H

#include <QAbstractListModel>
#include <QStyledItemDelegate>
#include <QList>
#include <QMap>
#include <QPixmap>
#include <QPersistentModelIndex>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "api/searchresult.h"

// Search results as one list model. A new search rewrites the rows in place
// (beginResetModel/endResetModel) instead of creating a widget per card.
// Avatars are stored in the model and painted by ResultDelegate, so there is
// no QLabel that can die while a reply is still in flight.
class ResultsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        ResultRole = Qt::UserRole + 1,
        AvatarRole
    };

    explicit ResultsModel(QObject *parent = 0);
    ~ResultsModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    void setResults(const SearchResultList &results, const QString &baseUrl);
    void appendResults(const SearchResultList &results, const QString &baseUrl);
    void clear();

private slots:
    void onAvatarFinished();
    void onAvatarReadyRead();

private:
    void abortPending();
    void fetchAvatars(int fromRow, const QString &baseUrl);

    QNetworkAccessManager *m_nam;
    SearchResultList m_results;
    QList<QPixmap> m_avatars;
    // reply -> row it belongs to; persistent so a reset invalidates it
    QMap<QNetworkReply *, QPersistentModelIndex> m_pending;
};

class ResultDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit ResultDelegate(QObject *parent = 0);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const;
};

#endif // RESULTSMODEL_H
