#include "AppSettings.h"
#include <QSettings>
#include <QCoreApplication>
#include <QUuid>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

AppSettings::AppSettings() {
    m_filePath = QCoreApplication::applicationDirPath() + "/settings.json";
}

void AppSettings::load() {
    QSettings settings("MCSM", "MCServerManager");
    data.autoStart     = settings.value("general/autoStart", false).toBool();
    data.webApiEnabled = settings.value("webapi/enabled", false).toBool();
    data.webApiPort    = settings.value("webapi/port", 25575).toInt();
    data.webApiToken   = settings.value("webapi/token", "").toString();
    data.botEnabled    = settings.value("bot/enabled", true).toBool();
    data.botPluginPort = settings.value("bot/port", 25576).toInt();
    data.language      = settings.value("general/language", "en").toString();
    data.webUILang     = settings.value("general/webUILanguage", "auto").toString();
    data.themeColor    = settings.value("theme/accentColor", "#6c5ce7").toString();
    data.bgStyle       = settings.value("theme/bgStyle", "none").toString();
    data.customBgPath  = settings.value("theme/customBgPath", "").toString();

    // Auto-generate token if empty
    if (data.webApiToken.isEmpty()) {
        data.webApiToken = QUuid::createUuid().toString(QUuid::WithoutBraces);
        save();
    }

    // Sync auto-start with Windows registry
#ifdef Q_OS_WIN
    {
        HKEY hKey;
        LONG result = RegOpenKeyExW(HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey);
        bool registryExists = false;
        if (result == ERROR_SUCCESS) {
            WCHAR buf[MAX_PATH];
            DWORD size = sizeof(buf);
            DWORD type = REG_SZ;
            if (RegQueryValueExW(hKey, L"MCServerManager", nullptr, &type, (LPBYTE)buf, &size) == ERROR_SUCCESS)
                registryExists = true;
            RegCloseKey(hKey);
        }
        if (data.autoStart && !registryExists) {
            setAutoStart(true);
        } else if (!data.autoStart && registryExists) {
            setAutoStart(false);
        }
    }
#endif
}

void AppSettings::save() const {
    QSettings settings("MCSM", "MCServerManager");
    settings.setValue("general/autoStart",    data.autoStart);
    settings.setValue("general/language",     data.language);
    settings.setValue("general/webUILanguage",data.webUILang);
    settings.setValue("webapi/enabled",       data.webApiEnabled);
    settings.setValue("webapi/port",          data.webApiPort);
    settings.setValue("webapi/token",         data.webApiToken);
    settings.setValue("bot/enabled",          data.botEnabled);
    settings.setValue("bot/port",             data.botPluginPort);
    settings.setValue("theme/accentColor",    data.themeColor);
    settings.setValue("theme/bgStyle",        data.bgStyle);
    settings.setValue("theme/customBgPath",   data.customBgPath);
    settings.sync();
}

// Windows auto-start helpers
#ifdef Q_OS_WIN
static bool winRegAutoStart(bool enable) {
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey);
    if (result != ERROR_SUCCESS) return false;

    if (enable) {
        QString exePath = QCoreApplication::applicationFilePath();
        exePath.replace('/', '\\');
        std::wstring wpath = exePath.toStdWString();
        result = RegSetValueExW(hKey, L"MCServerManager", 0, REG_SZ,
                                (const BYTE*)wpath.c_str(),
                                (DWORD)((wpath.size() + 1) * sizeof(wchar_t)));
    } else {
        RegDeleteValueW(hKey, L"MCServerManager");
        result = ERROR_SUCCESS;
    }
    RegCloseKey(hKey);
    return result == ERROR_SUCCESS;
}
#endif

void AppSettings::setAutoStart(bool enable) {
    data.autoStart = enable;
#ifdef Q_OS_WIN
    winRegAutoStart(enable);
#endif
    save();
}
