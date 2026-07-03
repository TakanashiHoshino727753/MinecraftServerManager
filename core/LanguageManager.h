#pragma once
#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QVector>
#include <QStringList>

// ─── Language Manager (singleton) ────────────────────────────────────
// 支持 en, zh, ja, ko, fr, de, ru, es

class LanguageManager {
public:
    explicit LanguageManager();

    static LanguageManager* instance();

    // Current language code: "en", "zh", "ja", etc.
    QString currentLangCode() const { return m_currentCode; }
    void setLanguageCode(const QString& code);

    // WebUI language
    QString webUILangCode() const { return m_webUICode; }
    void setWebUILangCode(const QString& code);

    // Translate a key → text for current language (fallback to EN)
    QString t(const QString& key) const;

    // Static helpers
    static QStringList allLanguageCodes();
    static QString languageName(const QString& code);
    static QStringList webUILanguageCodes();  // subset for WebUI

    // Get all translations as JSON
    QJsonObject translationsJson(const QString& langCode) const;

private:
    void loadTranslations();
    static LanguageManager* s_instance;

    QString m_currentCode = "en";
    QString m_webUICode   = "auto";  // "auto" = follow launcher

    // code → key → text
    QMap<QString, QMap<QString, QString>> m_trans;
};
