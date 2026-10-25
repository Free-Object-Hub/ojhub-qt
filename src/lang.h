#ifndef LANG_H
#define LANG_H

#include <QString>
#include <QHash>

class Lang
{
public:
    static Lang &instance();

    QString tr(const QString &key) const;
    void set(const QString &key, const QString &value);
    bool loadFromJsonFile(const QString &path);

private:
    Lang();
    void loadDefaults();

    QHash<QString, QString> m_map;
};

inline QString L(const char *key)
{
    return Lang::instance().tr(QString::fromUtf8(key));
}

#endif
