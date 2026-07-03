#pragma once
#include <QString>

struct AppSettingsData {
    bool    autoStart      = false;
    bool    webApiEnabled  = true;
    int     webApiPort     = 25575;
    QString webApiToken    = "";
    bool    botEnabled     = true;
    int     botPluginPort  = 25576;

    // Language settings
    QString language       = "en";       // "en", "zh", "ja", "ko", "fr", "de", "ru", "es"
    QString webUILang      = "en";       // "en", "zh", "ja", ... or "auto"

    // Theme settings
    QString themeColor     = "#6c5ce7";  // 主色调 (accent color)
    QString bgStyle        = "none";     // "none" or "custom"
    QString customBgPath   = "";         // 自定义背景图路径
};

class AppSettings {
public:
    AppSettings();
    void load();
    void save() const;
    void setAutoStart(bool enable);

    AppSettingsData data;

private:
    QString m_filePath;
};
