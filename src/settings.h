#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>

// Everything the client persists lives next to the binary, so the app stays portable
// (no registry, no ~/.config). Qt 5.0+.
class AppSettings
{
public:
    static QString iniPath();   // <exe dir>/ObjectHub.ini
    static QString cacheDir();  // <exe dir>/cache

    // Device id for the server's devices table (staticFp). Generated once, stored in the ini.
    static QString deviceId();

    // Saved login (raw token, same as the browser client keeps). Stored in the ini.
    static void saveLogin(const QString &token, int userId);
    static void loadLogin(QString &token, int &userId);
    static void clearLogin();
};

#endif // SETTINGS_H
