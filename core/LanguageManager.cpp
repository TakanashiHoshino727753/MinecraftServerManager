#include "LanguageManager.h"
#include <QJsonArray>

LanguageManager* LanguageManager::s_instance = nullptr;

LanguageManager::LanguageManager() {
    loadTranslations();
}

LanguageManager* LanguageManager::instance() {
    if (!s_instance) s_instance = new LanguageManager();
    return s_instance;
}

// ── Language code → display name ──
QStringList LanguageManager::allLanguageCodes() {
    return {"en", "zh", "ja", "ko", "fr", "de", "ru", "es"};
}

QStringList LanguageManager::webUILanguageCodes() {
    return {"en", "zh", "ja", "ko", "fr", "de", "ru", "es"};
}

QString LanguageManager::languageName(const QString& code) {
    static QMap<QString, QString> names = {
        {"en", "English"},
        {"zh", "中文 (Chinese)"},
        {"ja", "日本語 (Japanese)"},
        {"ko", "한국어 (Korean)"},
        {"fr", "Français (French)"},
        {"de", "Deutsch (German)"},
        {"ru", "Русский (Russian)"},
        {"es", "Español (Spanish)"},
    };
    return names.value(code, code);
}

void LanguageManager::setLanguageCode(const QString& code) {
    m_currentCode = code;
}

void LanguageManager::setWebUILangCode(const QString& code) {
    m_webUICode = code;
}

// ── Translate ──
QString LanguageManager::t(const QString& key) const {
    if (m_trans.contains(m_currentCode)) {
        const auto& map = m_trans[m_currentCode];
        if (map.contains(key)) return map[key];
    }
    // Fallback to English
    if (m_currentCode != "en" && m_trans.contains("en")) {
        const auto& en = m_trans["en"];
        if (en.contains(key)) return en[key];
    }
    return key;
}

QJsonObject LanguageManager::translationsJson(const QString& langCode) const {
    QJsonObject obj;
    if (m_trans.contains(langCode)) {
        const auto& map = m_trans[langCode];
        for (auto it = map.begin(); it != map.end(); ++it)
            obj[it.key()] = it.value();
    }
    return obj;
}

// ══════════════════════════════════════════════════════════════════════
// Macro: auto-translate ("_en" → en, "_zh" → zh, ...)
// ══════════════════════════════════════════════════════════════════════
#define TKEY(key) QString::fromUtf8(key)

static void add(QMap<QString, QMap<QString, QString>>& t,
                const QString& key,
                const QString& en, const QString& zh="",
                const QString& ja="", const QString& ko="",
                const QString& fr="", const QString& de="",
                const QString& ru="", const QString& es="")
{
    t["en"][key] = en;
    if (!zh.isEmpty()) t["zh"][key] = zh; else t["zh"][key] = en;
    if (!ja.isEmpty()) t["ja"][key] = ja; else t["ja"][key] = en;
    if (!ko.isEmpty()) t["ko"][key] = ko; else t["ko"][key] = en;
    if (!fr.isEmpty()) t["fr"][key] = fr; else t["fr"][key] = en;
    if (!de.isEmpty()) t["de"][key] = de; else t["de"][key] = en;
    if (!ru.isEmpty()) t["ru"][key] = ru; else t["ru"][key] = en;
    if (!es.isEmpty()) t["es"][key] = es; else t["es"][key] = en;
}

void LanguageManager::loadTranslations() {
    auto& t = m_trans;

    // ── App / Global ──
    add(t, "app.title",
        "MC Server Manager",
        TKEY("MC 服务器管理器"),
        TKEY("MC サーバーマネージャー"),
        TKEY("MC 서버 매니저"),
        "MC Server Manager",
        "MC Server Manager",
        TKEY("Менеджер серверов MC"),
        "MC Server Manager");

    add(t, "app.version", "v1.0", "v1.0", "v1.0", "v1.0", "v1.0", "v1.0", "v1.0", "v1.0");

    // ── Sidebar ──
    add(t, "sidebar.section",
        "SERVER STATUS",
        TKEY("服务器状态"),
        TKEY("サーバー状態"),
        TKEY("서버 상태"),
        "ÉTAT DU SERVEUR",
        "SERVER-STATUS",
        TKEY("СТАТУС СЕРВЕРА"),
        "ESTADO DEL SERVIDOR");

    add(t, "sidebar.total",
        "Total Servers",
        TKEY("服务器总数"),
        TKEY("サーバー総数"),
        TKEY("전체 서버"),
        "Total des serveurs",
        "Server gesamt",
        TKEY("Всего серверов"),
        "Total de servidores");

    add(t, "sidebar.running",
        "Running",
        TKEY("运行中"),
        TKEY("実行中"),
        TKEY("실행 중"),
        "En cours",
        "Laufend",
        TKEY("Запущен"),
        "En ejecución");

    add(t, "sidebar.servers",
        "  Servers",
        TKEY("  服务器列表"),
        TKEY("  サーバー一覧"),
        TKEY("  서버 목록"),
        "  Serveurs",
        "  Server",
        TKEY("  Серверы"),
        "  Servidores");

    add(t, "sidebar.create",
        "  + New Server",
        TKEY("  + 新建服务器"),
        TKEY("  + 新規サーバー"),
        TKEY("  + 새 서버"),
        "  + Nouveau serveur",
        "  + Neuer Server",
        TKEY("  + Новый сервер"),
        "  + Nuevo servidor");

    add(t, "sidebar.settings",
        "  Settings",
        TKEY("  设置"),
        TKEY("  設定"),
        TKEY("  설정"),
        "  Paramètres",
        "  Einstellungen",
        TKEY("  Настройки"),
        "  Configuración");

    add(t, "sidebar.mods",
        "  Mods",
        TKEY("  模组管理"),
        TKEY("  モッド"),
        TKEY("  모드"),
        "  Mods", "  Mods", TKEY("  Моды"), "  Mods");

    add(t, "sidebar.modpack",
        "  Modpack",
        TKEY("  整合包"),
        TKEY("  統合パック"),
        TKEY("  모드팩"),
        "  Modpack", "  Modpack", TKEY("  Сборки"), "  Modpack");

    add(t, "sidebar.playerWorld",
        "  Players",
        TKEY("  玩家/世界"),
        TKEY("  プレイヤー"),
        TKEY("  플레이어"),
        "  Joueurs", "  Spieler", TKEY("  Игроки"), "  Jugadores");

    add(t, "sidebar.java",
        "  Java",
        TKEY("  Java 管理"),
        TKEY("  Java 管理"),
        TKEY("  Java 관리"),
        "  Java", "  Java", TKEY("  Java"), "  Java");

    add(t, "sidebar.footer",
        "Powered by Qt6", "Powered by Qt6", "Powered by Qt6", "Powered by Qt6",
        "Powered by Qt6", "Powered by Qt6", "Powered by Qt6", "Powered by Qt6");

    // ── System Tray ──
    add(t, "tray.show",
        "Show",
        TKEY("显示窗口"),
        TKEY("表示"),
        TKEY("표시"),
        "Afficher", "Anzeigen", TKEY("Показать"), "Mostrar");

    add(t, "tray.hide",
        "Hide",
        TKEY("隐藏到托盘"),
        TKEY("隠す"),
        TKEY("숨기기"),
        "Masquer", "Verstecken", TKEY("Скрыть"), "Ocultar");

    add(t, "tray.quit",
        "Quit",
        TKEY("退出"),
        TKEY("終了"),
        TKEY("종료"),
        "Quitter", "Beenden", TKEY("Выйти"), "Salir");

    // ── Server States ──
    add(t, "state.running",       "Running",       TKEY("运行中"),       TKEY("実行中"),       TKEY("실행 중"),       "En cours",   "Laufend",   TKEY("Запущен"),     "En ejecución");
    add(t, "state.stopped",       "Stopped",       TKEY("已停止"),       TKEY("停止"),          TKEY("중지됨"),          "Arrêté",     "Gestoppt",  TKEY("Остановлен"),  "Detenido");
    add(t, "state.starting",      "Starting...",   TKEY("启动中..."),     TKEY("起動中..."),      TKEY("시작 중..."),       "Démarrage...","Starte...", TKEY("Запуск..."),   "Iniciando...");
    add(t, "state.stopping",      "Stopping...",   TKEY("停止中..."),     TKEY("停止中..."),      TKEY("중지 중..."),       "Arrêt...",   "Stoppe...", TKEY("Остановка..."),"Deteniendo...");

    // ── Server List ──
    add(t, "list.emptyTitle",     "No Servers Yet",                    TKEY("暂无服务器"));
    add(t, "list.emptyDesc",      "Create your first Minecraft server to get started.", TKEY("创建你的第一个 Minecraft 服务器来开始吧。"));
    add(t, "list.createFirst",    "+ Create First Server",             TKEY("+ 创建第一个服务器"));
    add(t, "list.serverCount",    "%1 Servers",                        TKEY("%1 个服务器"));
    add(t, "list.start",          "Start",                             TKEY("启动"));
    add(t, "list.stop",           "Stop",                              TKEY("停止"));
    add(t, "list.config",         "Config",                            TKEY("配置"));
    add(t, "list.detail",         "Detail",                            TKEY("详情"));
    add(t, "list.running",        "Running",                           TKEY("运行中"));
    add(t, "list.stopped",        "Stopped",                           TKEY("已停止"));
    add(t, "list.version",        "Minecraft %1",                      "Minecraft %1");
    add(t, "list.memory",         "Mem: %1M-%2M",                      TKEY("内存: %1M-%2M"));
    add(t, "list.uptime",         "Up: --",                            TKEY("运行: --"));
    add(t, "list.path",           "Path: %1",                          TKEY("路径: %1"));
    add(t, "list.title",          "Servers",                           TKEY("服务器列表"));
    add(t, "list.createBtn",      "+ Create Server",                   TKEY("+ 创建服务器"));
    add(t, "list.editTooltip",    "Configure server",                  TKEY("配置服务器"));
    add(t, "list.detailTooltip",  "View details",                      TKEY("查看详情"));
    add(t, "list.refresh",        "Refresh list",                      TKEY("刷新列表"));

    // ── Detail ──
    add(t, "detail.back",         "\u2190");
    add(t, "detail.running",      "\u25CF Running",                    TKEY("\u25CF 运行中"));
    add(t, "detail.stopped",      "\u25CF Stopped",                    TKEY("\u25CF 已停止"));
    add(t, "detail.startServer",  "\u25B6  Start Server",              TKEY("\u25B6  启动服务器"));
    add(t, "detail.stopServer",   "\u25A0  Stop Server",               TKEY("\u25A0  停止服务器"));
    add(t, "detail.forceStop",    "Force Stop",                        TKEY("强制停止"));
    add(t, "detail.editConfig",   "Edit Config",                       TKEY("编辑配置"));
    add(t, "detail.properties",   "Properties",                        TKEY("属性"));
    add(t, "detail.delete",       "Delete",                            TKEY("删除"));
    add(t, "detail.status",       "Status:",                           TKEY("状态:"));
    add(t, "detail.uptime",       "Uptime:",                           TKEY("运行时间:"));
    add(t, "detail.memory",       "Memory:",                           TKEY("内存:"));
    add(t, "detail.java",         "Java:");
    add(t, "detail.console",      "Console",                           TKEY("控制台"));
    add(t, "detail.cmdPlaceholder","Enter command... (e.g. list, say, op)", TKEY("输入命令... (如 list, say, op)"));
    add(t, "detail.send",         "Send",                              TKEY("发送"));
    add(t, "detail.default",      "default",                           TKEY("默认"));
    add(t, "detail.javaDefault",  "java",                              TKEY("java"));
    add(t, "detail.cancel",       "Cancel",                            TKEY("取消"));
    add(t, "detail.save",         "Save",                              TKEY("保存"));
    add(t, "detail.error",        "Error",                             TKEY("错误"));

    // ── Detail tab labels ──
    add(t, "detail.tabConsole",     "Console",      TKEY("控制台"));
    add(t, "detail.tabMods",        "Mods",         TKEY("模组"));
    add(t, "detail.tabModpack",     "Modpack",      TKEY("整合包"));
    add(t, "detail.tabPlayerWorld", "Player/World", TKEY("玩家/世界"));

    // ── Detail (missing keys) ──
    add(t, "detail.deleteBtn",      "Delete",                            TKEY("删除"));
    add(t, "detail.editBtn",        "Edit",                              TKEY("编辑"));
    add(t, "detail.statusLabel",    "Status",                            TKEY("状态"));
    add(t, "detail.uptimeLabel",    "Uptime",                            TKEY("运行时间"));
    add(t, "detail.memLabel",       "Memory",                            TKEY("内存"));
    add(t, "detail.javaLabel",      "Java",                              TKEY("Java"));
    add(t, "detail.cheatSheet",     "Ref",                               TKEY("指令集"));
    add(t, "detail.confirm",        "Confirm",                           TKEY("确认"));
    add(t, "detail.forceStopMsg",   "Force stop this server immediately? Unsaved data may be lost!", TKEY("立即强制停止此服务器？未保存的数据可能会丢失！"));
    add(t, "detail.confirmDelete",  "Confirm Delete",                    TKEY("确认删除"));
    add(t, "detail.deleteMsg",      "Are you sure you want to delete '%1'?", TKEY("确定要删除 '%1' 吗？"));
    add(t, "detail.editConfigTitle", "Edit Server Config",               TKEY("编辑服务器配置"));
    add(t, "detail.serverName",     "Server Name",                       TKEY("服务器名称"));
    add(t, "detail.serverPath",     "Server Path",                       TKEY("服务器路径"));
    add(t, "detail.browse",         "Browse",                            TKEY("浏览"));
    add(t, "detail.selectDir",      "Select Directory",                  TKEY("选择目录"));
    add(t, "detail.minRam",         "Min: %1M",                          TKEY("最小: %1M"));
    add(t, "detail.maxRam",         "Max: %1M",                          TKEY("最大: %1M"));
    add(t, "detail.javaPath",       "Java Path",                         TKEY("Java 路径"));
    add(t, "detail.selectJava",     "Select Java",                       TKEY("选择 Java"));
    add(t, "detail.selectJavaFilter", "Java Executable (java.exe)",      TKEY("Java 可执行文件 (java.exe)"));
    add(t, "detail.jvmArgs",        "JVM Arguments",                     TKEY("JVM 参数"));
    add(t, "detail.jvmArgsHint",    "Additional JVM arguments (optional)", TKEY("额外的 JVM 参数（可选）"));
    add(t, "detail.nameEmpty",      "Server name cannot be empty",       TKEY("服务器名称不能为空"));
    add(t, "detail.pathEmpty",      "Server path cannot be empty",       TKEY("服务器路径不能为空"));
    add(t, "detail.lines",          "%1 lines",                          TKEY("%1 行"));
    add(t, "detail.serverProps",    "Server Properties",                 TKEY("服务器属性"));
    add(t, "detail.showAdvanced",   "Show Advanced",                     TKEY("显示高级选项"));
    add(t, "detail.advanced",       "Advanced Options",                  TKEY("高级选项"));
    add(t, "detail.saveProps",      "Save Properties",                   TKEY("保存属性"));
    add(t, "detail.serverNotRunning", "Server is not running",           TKEY("服务器未运行"));
    add(t, "detail.quickActions",   "Quick Actions",                     TKEY("快捷操作"));
    add(t, "detail.hideAdvanced",   "Hide Advanced",                     TKEY("隐藏高级选项"));

    // ── Settings ──
    add(t, "settings.title",      "\u2699  Settings",                  TKEY("\u2699  设置"));
    add(t, "settings.titleLabel", "\u2699  Settings",                  TKEY("\u2699  设置"));
    add(t, "settings.general",    "General",                           TKEY("常规"));
    add(t, "settings.language",   "Language",                          TKEY("语言"));
    add(t, "settings.webApi",     "Web API",                           "Web API");
    add(t, "settings.botPlugin",  "Bot Plugin",                        TKEY("机器人插件"));
    add(t, "settings.cancel",     "Cancel",                            TKEY("取消"));
    add(t, "settings.saveBtn",    "Save",                              TKEY("保存"));
    add(t, "settings.startup",    "Startup",                           TKEY("启动"));
    add(t, "settings.autoStart",  "Start with Windows",                TKEY("开机自动启动"));
    add(t, "settings.autoStartHint","Automatically launch MC Server Manager when you log into Windows.",
        TKEY("登录 Windows 时自动启动 MC 服务器管理器。"));
    add(t, "settings.langHint",   "Restart recommended for full effect.",
        TKEY("建议重启以获得完整效果。"));
    add(t, "settings.appLang",    "Application Language",
        TKEY("应用语言"), TKEY("アプリ言語"), TKEY("애플리케이션 언어"),
        "Langue de l'application", "Anwendungssprache",
        TKEY("Язык приложения"), "Idioma de la aplicación");
    add(t, "settings.webUILang",  "WebUI Language",
        TKEY("WebUI 语言"), TKEY("WebUI 言語"), TKEY("WebUI 언어"),
        "Langue WebUI", "WebUI-Sprache", TKEY("Язык WebUI"), "Idioma WebUI");
    add(t, "settings.followLauncher", "Follow Launcher (same language as the desktop app)",
        TKEY("跟随启动器 (与桌面应用使用相同语言)"));
    add(t, "settings.setManually", "Set Manually (WebUI can use a different language)",
        TKEY("手动设置 (WebUI 可使用不同语言)"));
    add(t, "settings.langLabel",  "Language:",
        TKEY("语言:"), TKEY("言語:"), TKEY("언어:"),
        "Langue :", "Sprache:", TKEY("Язык:"), "Idioma:");
    add(t, "settings.langChanged",     "Language Changed",             TKEY("语言已更改"));
    add(t, "settings.langChangedMsg",  "Language settings saved. Some elements may require a restart to fully update.",
        TKEY("语言设置已保存。部分元素可能需要重启才能完全更新。"));

    // ── WebUI basics ──
    add(t, "webui.title",         "MC Server Manager - WebUI",         TKEY("MC 服务器管理器 - WebUI"));
    add(t, "webui.servers",       "Servers",                           TKEY("服务器"));
    add(t, "webui.settings",      "Settings",                          TKEY("设置"));
    add(t, "webui.serverStatus",  "Server Status",                     TKEY("服务器状态"));
    add(t, "webui.total",         "Total Servers",                     TKEY("服务器总数"));
    add(t, "webui.start",         "Start",                             TKEY("启动"));
    add(t, "webui.stop",          "Stop",                              TKEY("停止"));
    add(t, "webui.kill",          "Force Stop",                        TKEY("强制停止"));
    add(t, "webui.console",       "Console",                           TKEY("控制台"));
    add(t, "webui.send",          "Send",                              TKEY("发送"));
    add(t, "webui.command",       "Enter command...",                  TKEY("输入命令..."));
    add(t, "webui.running",       "Running",                           TKEY("运行中"));
    add(t, "webui.stopped",       "Stopped",                           TKEY("已停止"));
    add(t, "webui.starting",      "Starting...",                       TKEY("启动中..."));
    add(t, "webui.stopping",      "Stopping...",                       TKEY("停止中..."));
    add(t, "webui.refresh",       "Refresh",                           TKEY("刷新"));
    add(t, "webui.detail",        "Detail",                            TKEY("详情"));
    add(t, "webui.properties",    "Properties",                        TKEY("属性"));
    add(t, "webui.close",         "Close",                             TKEY("关闭"));
    add(t, "webui.save",          "Save",                              TKEY("保存"));
    add(t, "webui.cancel",        "Cancel",                            TKEY("取消"));
    add(t, "webui.edit",          "Edit",                              TKEY("编辑"));
    add(t, "webui.delete",        "Delete",                            TKEY("删除"));
    add(t, "webui.quickActions",  "Quick",                             TKEY("快捷"));
    add(t, "webui.cheatSheet",    "Ref",                               TKEY("指令集"));
    add(t, "webui.langLabel",     "Language",                          TKEY("语言"));
    add(t, "webui.general",       "General",                           TKEY("常规"));
    add(t, "webui.enabled",       "Enabled",                           TKEY("已启用"));
    add(t, "webui.disabled",      "Disabled",                          TKEY("已禁用"));
    add(t, "webui.done",          "Done",                              TKEY("完成"));
    add(t, "webui.networkError",  "Network error",                     TKEY("网络错误"));

    add(t, "webui.addServer",       "+ Add Server",                      TKEY("+ 添加服务器"));
    add(t, "webui.apiConnected",        "API Connected",                              TKEY("API 已连接"));
    add(t, "webui.apiConnecting",        "API Connecting...",                              TKEY("API 连接中..."));
    add(t, "webui.apiDisconnected",        "API Disconnected",                              TKEY("API 已断开"));
    add(t, "webui.apiToken",        "API Token",                              TKEY("API 令牌"));
    add(t, "webui.autoStart",        "Auto-start",                              TKEY("自动启动"));
    add(t, "webui.botDesc",        "Enable bot plugin integration",                              TKEY("启用机器人插件集成"));
    add(t, "webui.botIntegration",        "Bot Integration",                              TKEY("机器人集成"));
    add(t, "webui.botPort",        "Bot Port",                              TKEY("机器人端口"));
    add(t, "webui.cheatCatAdmin",        "Admin",                              TKEY("管理员"));
    add(t, "webui.cheatCatAll",        "All",                              TKEY("全部"));
    add(t, "webui.cheatCatPlayer",        "Player",                              TKEY("玩家"));
    add(t, "webui.cheatCatServer",        "Server",                              TKEY("服务器"));
    add(t, "webui.cheatCatWorld",        "World",                              TKEY("世界"));
    add(t, "webui.cheatSearchPlaceholder",        "Search commands...",                              TKEY("搜索指令..."));
    add(t, "webui.cheatSheetHint",        "Click to fill",                              TKEY("点击填入"));
    add(t, "webui.cheatSheetTitle",        "Command Reference",                              TKEY("指令参考"));
    add(t, "webui.configSaved",        "Config saved",                              TKEY("配置已保存"));
    add(t, "webui.confirmDelete",        "Confirm Delete",                              TKEY("确认删除"));
    add(t, "webui.consoleOutput",        "Console Output",                              TKEY("控制台输出"));
    add(t, "webui.copy",        "Copy",                              TKEY("复制"));
    add(t, "webui.createNewServer",        "Create New Server",                              TKEY("创建新服务器"));
    add(t, "webui.deleteMsg",        "Delete server \"%1\"?",                              TKEY("确认删除服务器 \"%1\"？"));
    add(t, "webui.downloadComplete",        "Download complete",                              TKEY("下载完成"));
    add(t, "webui.downloading",        "Downloading...",                              TKEY("下载中..."));
    add(t, "webui.editConfig",        "Edit Configuration",                              TKEY("编辑配置"));
    add(t, "webui.enableBot",        "Enable Bot",                              TKEY("启用机器人"));
    add(t, "webui.failedToLoad",        "Failed to load settings",                              TKEY("加载设置失败"));
    add(t, "webui.failedToRegen",        "Failed to regenerate token",                              TKEY("重新生成令牌失败"));
    add(t, "webui.failedToSave",        "Failed to save",                              TKEY("保存失败"));
    add(t, "webui.forceKill",        "Force Kill",                              TKEY("强制终止"));
    add(t, "webui.forceKilled",        "Server force killed",                              TKEY("服务器已强制终止"));
    add(t, "webui.generate",        "Generate",                              TKEY("生成"));
    add(t, "webui.hideAdvanced",        "Hide Advanced",                              TKEY("隐藏高级设置"));
    add(t, "webui.java",        "Java",                              TKEY("Java"));
    add(t, "webui.javaPath",        "Java Path",                              TKEY("Java 路径"));
    add(t, "webui.jvmArgs",        "JVM Args",                              TKEY("JVM 参数"));
    add(t, "webui.langDesc",        "WebUI language",                              TKEY("WebUI 界面语言设置"));
    add(t, "webui.languageChanged",        "Language changed",                              TKEY("语言已更改"));
    add(t, "webui.lines",        "%1 lines",                              TKEY("%1 行"));
    add(t, "webui.loadingProperties",        "Loading properties...",                              TKEY("正在加载属性..."));
    add(t, "webui.loadingSettings",        "Loading settings...",                              TKEY("正在加载设置..."));
    add(t, "webui.maxRam",        "Max RAM",                              TKEY("最大内存"));
    add(t, "webui.memory",        "Memory",                              TKEY("内存"));
    add(t, "webui.minRam",        "Min RAM",                              TKEY("最小内存"));
    add(t, "webui.minecraftVersion",        "Minecraft Version",                              TKEY("Minecraft 版本"));
    add(t, "webui.noServers",        "No servers",                              TKEY("暂无服务器"));
    add(t, "webui.openInBrowser",        "Open %1 in browser",                              TKEY("在浏览器中打开 %1"));
    add(t, "webui.portChanged",        "Port changed to %1",                              TKEY("端口已更改为 %1"));
    add(t, "webui.portLabel",        "Port",                              TKEY("端口"));
    add(t, "webui.portRangeError",        "Port must be 1024-65535",                              TKEY("端口必须在 1024-65535 之间"));
    add(t, "webui.propsFailed",        "Failed to load properties",                              TKEY("加载属性失败"));
    add(t, "webui.propsNoProperties",        "No properties available",                              TKEY("无可用属性"));
    add(t, "webui.propsRestartNote",        "Changes require restart",                              TKEY("更改将在服务器重启后生效"));
    add(t, "webui.propsSaveBtn",        "Save Properties",                              TKEY("保存属性"));
    add(t, "webui.propsSaved",        "Properties saved",                              TKEY("属性已保存"));
    add(t, "webui.propsSaving",        "Saving...",                              TKEY("保存中..."));
    add(t, "webui.saveSettings",        "Save Settings",                              TKEY("保存设置"));
    add(t, "webui.saving",        "Saving...",                              TKEY("保存中..."));
    add(t, "webui.secBasic",        "Basic Settings",                              TKEY("基本设置"));
    add(t, "webui.secNetwork",        "Network",                              TKEY("网络"));
    add(t, "webui.secOther",        "Other",                              TKEY("其他"));
    add(t, "webui.secPerformance",        "Performance",                              TKEY("性能"));
    add(t, "webui.secPlayer",        "Player Settings",                              TKEY("玩家设置"));
    add(t, "webui.secRcon",        "RCON & Query",                              TKEY("RCON 与查询"));
    add(t, "webui.secResource",        "Resource Pack",                              TKEY("资源包"));
    add(t, "webui.secSecurity",        "Security & Permissions",                              TKEY("安全与权限"));
    add(t, "webui.secWorld",        "World Generation",                              TKEY("世界生成"));
    add(t, "webui.secWorldAdv",        "World Advanced",                              TKEY("世界高级"));
    add(t, "webui.serverAction",        "Server %1",                              TKEY("服务器 %1"));
    add(t, "webui.serverDeleted",        "Server deleted",                              TKEY("服务器已删除"));
    add(t, "webui.serverName",        "Server Name",                              TKEY("服务器名称"));
    add(t, "webui.serverProperties",        "Server Properties",                              TKEY("服务器属性"));
    add(t, "webui.serverStarting",        "Starting...",                              TKEY("启动中..."));
    add(t, "webui.serverStopping",        "Stopping...",                              TKEY("停止中..."));
    add(t, "webui.serverType",        "Server Type",                              TKEY("服务器类型"));
    add(t, "webui.settingsSaved",        "Settings saved",                              TKEY("设置已保存"));
    add(t, "webui.showAdvanced",        "Show Advanced",                              TKEY("显示高级设置"));
    add(t, "webui.status",        "Status",                              TKEY("状态"));
    add(t, "webui.tokenCopied",        "Token copied",                              TKEY("令牌已复制"));
    add(t, "webui.tokenRegenerated",        "Token regenerated",                              TKEY("令牌已重新生成"));
    add(t, "webui.uptime",        "Uptime",                              TKEY("运行时间"));
    add(t, "webui.urlCopied",        "URL copied",                              TKEY("地址已复制"));
    add(t, "webui.waitingOutput",        "Waiting for output...",                              TKEY("等待输出..."));
    add(t, "webui.webApiTitle",        "Web API",                              TKEY("Web 服务器"));
    add(t, "webui.webuiUrl",        "WebUI URL",                              TKEY("WebUI 地址"));
    add(t, "webui.welcomeSub",        "Select a server or create a new one",                              TKEY("选择服务器或创建新服务器"));
    add(t, "webui.welcomeTitle",        "Welcome to MC Server Manager",                              TKEY("欢迎使用 MC 服务器管理器"));
    add(t, "webui.wizardAgreeEula",        "I agree to the Minecraft EULA",                              TKEY("我同意 Minecraft EULA"));
    add(t, "webui.wizardAgreed",        "Agreed",                              TKEY("已同意"));
    add(t, "webui.wizardAutoDownload",        "Auto-download server JAR",                              TKEY("自动下载服务器 JAR"));
    add(t, "webui.wizardBuild",        "Build",                              TKEY("构建版本"));
    add(t, "webui.wizardCancel",        "Cancel",                              TKEY("取消"));
    add(t, "webui.wizardCreate",        "Create",                              TKEY("创建"));
    add(t, "webui.wizardCreated",        "Server created!",                              TKEY("服务器已创建！"));
    add(t, "webui.wizardCreating",        "Creating...",                              TKEY("创建中..."));
    add(t, "webui.wizardDownloadingJar",        "Downloading JAR...",                              TKEY("正在下载 JAR..."));
    add(t, "webui.wizardEula",        "EULA",                              TKEY("EULA"));
    add(t, "webui.wizardFabricRec",        "Fabric is recommended for modding",                              TKEY("Fabric 推荐用于模组"));
    add(t, "webui.wizardJava",        "Java",                              TKEY("Java"));
    add(t, "webui.wizardMemory",        "Memory",                              TKEY("内存"));
    add(t, "webui.wizardName",        "Name",                              TKEY("名称"));
    add(t, "webui.wizardNameRequired",        "Server name is required",                              TKEY("服务器名称不能为空"));
    add(t, "webui.wizardNext",        "Next",                              TKEY("下一步"));
    add(t, "webui.wizardNotAgreed",        "Not Agreed",                              TKEY("未同意"));
    add(t, "webui.wizardPaperRec",        "Paper is recommended for performance",                              TKEY("Paper 推荐用于性能优化"));
    add(t, "webui.wizardPath",        "Path",                              TKEY("路径"));
    add(t, "webui.wizardPathRequired",        "Server path is required",                              TKEY("服务器路径不能为空"));
    add(t, "webui.wizardPrevious",        "Previous",                              TKEY("上一步"));
    add(t, "webui.wizardSelectType",        "Please select a server type",                              TKEY("请选择服务器类型"));
    add(t, "webui.wizardSelectVersion",        "Please select a version",                              TKEY("请选择版本"));
    add(t, "webui.wizardType",        "Type",                              TKEY("类型"));
    add(t, "webui.wizardVanillaRec",        "Vanilla is the official experience",                              TKEY("Vanilla 是官方原版体验"));
    add(t, "webui.wizardVersion",        "Version",                              TKEY("版本"));

    // ── Mod Management (WebUI) ──
    add(t, "webui.mods",               "Mods",                             TKEY("模组管理"));
    add(t, "webui.loadingMods",        "Loading mods...",                  TKEY("正在加载模组..."));
    add(t, "webui.installedMods",      "Installed",                        TKEY("已安装"));
    add(t, "webui.marketMods",         "Market",                           TKEY("市场"));
    add(t, "webui.rescanMods",         "Rescan",                           TKEY("重新扫描"));
    add(t, "webui.noMods",            "No mods installed",                TKEY("没有安装模组"));
    add(t, "webui.noMarketMods",       "No mods available in market",      TKEY("市场中暂无可用模组"));
    add(t, "webui.enable",             "Enable",                           TKEY("启用"));
    add(t, "webui.disable",            "Disable",                          TKEY("禁用"));
    add(t, "webui.remove",             "Remove",                           TKEY("移除"));
    add(t, "webui.modEnabled",         "Mod enabled",                      TKEY("模组已启用"));
    add(t, "webui.modDisabled",        "Mod disabled",                     TKEY("模组已禁用"));
    add(t, "webui.modRemoved",         "Mod removed",                      TKEY("模组已移除"));
    add(t, "webui.modConflictLoader",  "Loader mismatch — mods require different mod loaders",
        TKEY("加载器不兼容 — 模组需要不同的模组加载器"));
    add(t, "webui.modConflictDeclared","Conflict declared between mods",   TKEY("模组之间已声明的冲突"));
    add(t, "webui.all",                "All",                              TKEY("全部"));
    // Player / World management (WebUI)
    add(t, "webui.playerWorld",        "Player & World",                   TKEY("玩家/世界"));
    add(t, "webui.playersTab",         "Players",                          TKEY("玩家"));
    add(t, "webui.worldsTab",          "Worlds",                           TKEY("世界"));
    add(t, "webui.noPlayers",          "No online players",                TKEY("没有在线玩家"));
    add(t, "webui.noWorlds",           "No worlds found",                  TKEY("未找到世界"));
    add(t, "webui.playerName",         "Player",                           TKEY("玩家"));
    add(t, "webui.world",              "World",                            TKEY("世界"));
    add(t, "webui.health",             "HP",                               TKEY("生命"));
    add(t, "webui.actions",            "Actions",                          TKEY("操作"));
    add(t, "webui.worldName",          "World",                            TKEY("世界"));
    add(t, "webui.size",               "Size",                             TKEY("大小"));
    add(t, "webui.kick",               "Kick",                             TKEY("踢出"));
    add(t, "webui.ban",                "Ban",                              TKEY("封禁"));
    add(t, "webui.op",                 "OP",                               TKEY("设为管理"));
    add(t, "webui.deop",               "De-OP",                            TKEY("取消管理"));
    add(t, "webui.backup",             "Backup",                           TKEY("备份"));
    add(t, "webui.kickConfirm",        "Kick %1?",                         TKEY("踢出 %1？"));
    add(t, "webui.banConfirm",         "Ban %1?",                          TKEY("封禁 %1？"));
    add(t, "webui.playerKicked",       "%1 kicked",                        TKEY("%1 已踢出"));
    add(t, "webui.playerBanned",       "%1 banned",                        TKEY("%1 已封禁"));
    add(t, "webui.playerOpped",        "%1 made operator",                 TKEY("%1 已设为管理员"));
    add(t, "webui.playerDeopped",      "%1 de-opped",                      TKEY("%1 已取消管理员"));
    add(t, "webui.worldBackedUp",      "%1 backed up",                     TKEY("%1 已备份"));
    add(t, "webui.deleteWorldConfirm", "Delete world \"%1\"?",             TKEY("删除世界 \"%1\"?"));
    add(t, "webui.worldDeleted",       "World \"%1\" deleted",             TKEY("世界 \"%1\" 已删除"));
    // Modpack install (WebUI)
    add(t, "webui.installModpack",     "Install Modpack",                  TKEY("安装整合包"));
    add(t, "webui.clickOrDrop",        "Click or drag .zip here",          TKEY("点击或拖拽 .zip 到此"));
    add(t, "webui.modpackHint",        "Supports .zip modpacks with mods/ folder or manifest.json",
                                                                           TKEY("支持包含 mods/ 目录或 manifest.json 的 .zip 整合包"));
    add(t, "webui.analyzing",          "Analyzing...",                     TKEY("正在分析..."));
    add(t, "webui.modCount",           "Mods",                             TKEY("模组数量"));
    add(t, "webui.targetPath",         "Target Path",                      TKEY("目标路径"));
    add(t, "webui.selectVersion",      "Select Version",                   TKEY("选择版本"));
    add(t, "webui.install",            "Install",                          TKEY("安装"));
    add(t, "webui.installingModpack",  "Installing modpack",               TKEY("正在安装整合包"));
    add(t, "webui.modpackInstalled",   "Modpack installed",                TKEY("整合包已安装"));
    // Modrinth search (WebUI)
    add(t, "webui.searchModrinth",     "Search Modrinth...",               TKEY("搜索 Modrinth..."));
    add(t, "webui.search",             "Search",                           TKEY("搜索"));
    add(t, "webui.download",           "Download",                         TKEY("下载"));
    add(t, "webui.noResults",          "No results",                       TKEY("无结果"));
    add(t, "webui.page",               "Page",                             TKEY("页"));
    add(t, "webui.prev",               "Prev",                             TKEY("上一页"));
    add(t, "webui.next",               "Next",                             TKEY("下一页"));
    add(t, "webui.selectServer",       "Please select a server first",     TKEY("请先选择一个服务器"));
    add(t, "webui.downloadingMod",     "Downloading...",                   TKEY("下载中..."));
    add(t, "webui.modDownloaded",      "Mod downloaded",                   TKEY("模组已下载"));

    // ── Watchdog ──
    add(t, "watchdog.armed",      "Watchdog: starting (60s timeout)",  TKEY("看门狗: 启动中 (60秒超时)"));
    add(t, "watchdog.fed",        "Server ready — watchdog disarmed",  TKEY("服务器就绪 — 看门狗已解除"));
    add(t, "watchdog.timeout",    "Watchdog timeout! Server startup may have failed.",
        TKEY("看门狗超时! 服务器启动可能失败。"));

    // ── Java Manager ──
    add(t, "java.autoDetect",     "Auto-detect Java",                  TKEY("自动检测 Java"));
    add(t, "java.download",       "Download Java",                     TKEY("下载 Java"));
    add(t, "java.notFound",       "Java not found. Please install Java %1 or higher.",
        TKEY("未找到 Java。请安装 Java %1 或更高版本。"));
    add(t, "java.downloading",    "Downloading Java %1...",            TKEY("正在下载 Java %1..."));
    add(t, "java.done",           "Java %1 installed successfully.",   TKEY("Java %1 安装成功。"));
    add(t, "java.recommended",    "Recommended: Java %1 for MC %2",    TKEY("推荐: Java %1 (适用于 MC %2)"));

    // ── Theme ──  
    add(t, "settings.theme",      "Theme",                              TKEY("主题"));
    add(t, "settings.accentColor","Accent Color",                       TKEY("主色调"));
    add(t, "settings.accentLabel","Accent:",                            TKEY("主题色:"));
    add(t, "settings.accentPurple","Purple",                            TKEY("紫色"));
    add(t, "settings.accentBlue",  "Blue",                              TKEY("蓝色"));
    add(t, "settings.accentCyan",  "Cyan",                              TKEY("青色"));
    add(t, "settings.accentGreen", "Green",                             TKEY("绿色"));
    add(t, "settings.accentOrange","Orange",                            TKEY("橙色"));
    add(t, "settings.accentRed",   "Red",                               TKEY("红色"));
    add(t, "settings.accentPink",  "Pink",                              TKEY("粉色"));
    add(t, "settings.accentMono",  "Gray",                              TKEY("灰色"));
    add(t, "settings.bgStyle",    "Background Style",                   TKEY("背景样式"));
    add(t, "settings.bgDark",     "Dark",                               TKEY("暗色"));
    add(t, "settings.bgLight",    "Light",                              TKEY("亮色"));
    add(t, "settings.bgCustom",   "Custom Image",                       TKEY("自定义图片"));
    add(t, "settings.chooseBg",   "Choose Image...",                    TKEY("选择图片..."));
    add(t, "settings.noBgSelected","No image selected",                 TKEY("未选择图片"));
    add(t, "settings.themeHint",  "Theme changes apply immediately. Accent color tints the background, and you can optionally use a custom background image.", 
        TKEY("主题更改立即生效。主色调会影响背景色调，您还可以选择使用自定义背景图片。"));

    // ── Theme (old compact keys, kept for compatibility) ──

    // ── Create Wizard ──
    add(t, "wizard.title",          "Create New Server",                 TKEY("创建新服务器"));
    add(t, "wizard.subtitle",       "Follow the steps to set up your Minecraft server", TKEY("按照步骤设置您的 Minecraft 服务器"));
    add(t, "wizard.cancelBtn",      "Cancel",                            TKEY("取消"));
    add(t, "wizard.prevBtn",        "Back",                              TKEY("上一步"));
    add(t, "wizard.step1Label",     "Type",                              TKEY("类型"));
    add(t, "wizard.step2Label",     "Version",                           TKEY("版本"));
    add(t, "wizard.step3Label",     "Config",                            TKEY("配置"));
    add(t, "wizard.step4Label",     "Confirm",                           TKEY("确认"));
    add(t, "wizard.step1Title",     "1. Choose Server Type",             TKEY("1. 选择服务器类型"));
    add(t, "wizard.step1Hint",      "Select the platform that best fits your needs", TKEY("选择最适合您需求的平台"));
    add(t, "wizard.vanillaName",    "Vanilla",                           TKEY("原版"));
    add(t, "wizard.vanillaDesc",    "Official Minecraft server",         TKEY("官方 Minecraft 服务器"));
    add(t, "wizard.vanillaFeat",    "Pure, stable, best for vanilla gameplay", TKEY("纯净、稳定，适合原版玩法"));
    add(t, "wizard.paperName",      "Paper",                             TKEY("Paper"));
    add(t, "wizard.paperDesc",      "High-performance fork",             TKEY("高性能分支"));
    add(t, "wizard.paperFeat",      "Plugins, optimizations, large servers", TKEY("插件、优化、大型服务器"));
    add(t, "wizard.fabricName",     "Fabric",                            TKEY("Fabric"));
    add(t, "wizard.fabricDesc",     "Lightweight mod loader",            TKEY("轻量模组加载器"));
    add(t, "wizard.fabricFeat",     "Mods, lightweight, community-driven", TKEY("模组、轻量、社区驱动"));
    add(t, "wizard.step2Title",     "2. Select Version",                 TKEY("2. 选择版本"));
    add(t, "wizard.step2Hint",      "Choose the Minecraft version for your server", TKEY("为您的服务器选择 Minecraft 版本"));
    add(t, "wizard.versionLabel",   "Minecraft Version:",                TKEY("Minecraft 版本:"));
    add(t, "wizard.buildLabel",     "Paper Build:",                      TKEY("Paper 构建版本:"));
    add(t, "wizard.step3Title",     "3. Server Configuration",           TKEY("3. 服务器配置"));
    add(t, "wizard.name",           "Server Name:",                      TKEY("服务器名称:"));
    add(t, "wizard.namePlaceholder","My Awesome Server",                 TKEY("我的服务器"));
    add(t, "wizard.path",           "Server Path:",                      TKEY("服务器路径:"));
    add(t, "wizard.pathPlaceholder","Choose a folder...",                TKEY("选择文件夹..."));
    add(t, "wizard.browse",         "Browse",                            TKEY("浏览"));
    add(t, "wizard.memoryGroup",    "Memory Allocation",                 TKEY("内存分配"));
    add(t, "wizard.minLabel",       "Min:",                              TKEY("最小:"));
    add(t, "wizard.maxLabel",       "Max:",                              TKEY("最大:"));
    add(t, "wizard.memUnit",        "%1 MB",                             TKEY("%1 MB"));
    add(t, "wizard.javaLabel",      "Java Path:",                        TKEY("Java 路径:"));
    add(t, "wizard.javaHint",       "Leave as 'java' to use system default", TKEY("保留 'java' 使用系统默认"));
    add(t, "wizard.eula",           "I agree to the Minecraft EULA",     TKEY("我同意 Minecraft 最终用户许可协议"));
    add(t, "wizard.step4Title",     "4. Confirm & Create",               TKEY("4. 确认并创建"));
    add(t, "wizard.step4Hint",      "Review your settings before creating the server", TKEY("创建前检查您的设置"));
    add(t, "wizard.keyType",        "Type:",                             TKEY("类型:"));
    add(t, "wizard.keyVersion",     "Version:",                          TKEY("版本:"));
    add(t, "wizard.keyBuild",       "Build:",                            TKEY("构建:"));
    add(t, "wizard.keyName",        "Name:",                             TKEY("名称:"));
    add(t, "wizard.keyPath",        "Path:",                             TKEY("路径:"));
    add(t, "wizard.keyMemory",      "Memory:",                           TKEY("内存:"));
    add(t, "wizard.keyJava",        "Java:",                             TKEY("Java:"));
    add(t, "wizard.keyEula",        "EULA:",                             TKEY("EULA:"));
    add(t, "wizard.notSelected",  "Not selected",                      TKEY("未选择"));
    add(t, "wizard.latest",         "Latest",                            TKEY("最新"));
    add(t, "wizard.notSet",         "Not set",                           TKEY("未设置"));
    add(t, "wizard.agreed",         "Agreed",                            TKEY("已同意"));
    add(t, "wizard.notAgreed",      "Not agreed",                        TKEY("未同意"));
    add(t, "wizard.errVersion",     "Please select a version",           TKEY("请选择版本"));
    add(t, "wizard.errName",        "Please enter a server name",        TKEY("请输入服务器名称"));
    add(t, "wizard.errPath",        "Please select a server path",       TKEY("请选择服务器路径"));
    add(t, "wizard.errEula",        "Please accept the EULA to continue", TKEY("请同意 EULA 以继续"));
    add(t, "wizard.errBuild",       "Please select a build number",      TKEY("请选择构建版本"));
    add(t, "wizard.errFill",        "Please fill in all required fields", TKEY("请填写所有必填项"));
    add(t, "wizard.errEulaAgree",   "You must agree to the EULA",        TKEY("您必须同意 EULA"));
    add(t, "wizard.dlFailed",       "Download failed: ",                 TKEY("下载失败: "));
    add(t, "wizard.createServer",   "Create Server",                     TKEY("创建服务器"));
    add(t, "wizard.nextStep",       "Next",                              TKEY("下一步"));
    add(t, "wizard.selectDir",      "Select Server Directory",           TKEY("选择服务器目录"));
    add(t, "wizard.selectJava",     "Select Java Executable",            TKEY("选择 Java 可执行文件"));
    add(t, "wizard.javaFilter",     "Java Executable (java.exe)",        TKEY("Java 可执行文件 (java.exe)"));
    add(t, "wizard.creating",       "Creating...",                       TKEY("创建中..."));

    // ── WebUI Wizard (missing keys) ──
    add(t, "wizard.create",        "Create",                              TKEY("创建"));
    add(t, "wizard.fabric",        "Fabric",                              TKEY("Fabric"));
    add(t, "wizard.loading",        "Loading...",                              TKEY("加载中..."));
    add(t, "wizard.paper",        "Paper",                              TKEY("Paper"));
    add(t, "wizard.serverPath",        "Server Path",                              TKEY("服务器路径"));
    add(t, "wizard.step1",        "1. Choose Type",                              TKEY("1. 选择类型"));
    add(t, "wizard.step2",        "2. Choose Version",                              TKEY("2. 选择版本"));
    add(t, "wizard.step3",        "3. Configuration",                              TKEY("3. 配置"));
    add(t, "wizard.step4",        "4. Confirm",                              TKEY("4. 确认"));
    add(t, "wizard.vanilla",        "Vanilla",                              TKEY("Vanilla"));

    // ── Server Properties labels ──
    add(t, "prop.allow-flight",        "Allow Flight",                              TKEY("允许飞行"));
    add(t, "prop.allow-nether",        "Allow Nether",                              TKEY("允许下界"));
    add(t, "prop.broadcast-console-to-ops",        "Broadcast Console to Ops",                              TKEY("向管理员广播控制台"));
    add(t, "prop.broadcast-rcon-to-ops",        "Broadcast RCON to Ops",                              TKEY("向管理员广播 RCON"));
    add(t, "prop.difficulty",        "Difficulty",                              TKEY("难度"));
    add(t, "prop.enable-command-block",        "Command Blocks",                              TKEY("命令方块"));
    add(t, "prop.enable-jmx-monitoring",        "JMX Monitoring",                              TKEY("JMX 监控"));
    add(t, "prop.enable-query",        "Enable Query",                              TKEY("启用查询"));
    add(t, "prop.enable-rcon",        "Enable RCON",                              TKEY("启用 RCON"));
    add(t, "prop.enable-status",        "Enable Status",                              TKEY("启用状态"));
    add(t, "prop.enforce-secure-profile",        "Enforce Secure Profile",                              TKEY("强制安全档案"));
    add(t, "prop.entity-broadcast-range-percentage",        "Entity Broadcast Range %",                              TKEY("实体广播范围 %"));
    add(t, "prop.force-gamemode",        "Force Gamemode",                              TKEY("强制游戏模式"));
    add(t, "prop.function-permission-level",        "Function Permission Level",                              TKEY("函数权限等级"));
    add(t, "prop.gamemode",        "Game Mode",                              TKEY("游戏模式"));
    add(t, "prop.generate-structures",        "Generate Structures",                              TKEY("生成结构"));
    add(t, "prop.hardcore",        "Hardcore Mode",                              TKEY("极限模式"));
    add(t, "prop.level-name",        "World Name",                              TKEY("世界名称"));
    add(t, "prop.level-seed",        "World Seed",                              TKEY("世界种子"));
    add(t, "prop.level-type",        "World Type",                              TKEY("世界类型"));
    add(t, "prop.max-build-height",        "Max Build Height",                              TKEY("最大建造高度"));
    add(t, "prop.max-players",        "Max Players",                              TKEY("最大玩家数"));
    add(t, "prop.max-tick-time",        "Max Tick Time (ms)",                              TKEY("最大 tick 时间 (ms)"));
    add(t, "prop.max-world-size",        "Max World Size",                              TKEY("最大世界大小"));
    add(t, "prop.motd",        "Server MOTD",                              TKEY("服务器 MOTD"));
    add(t, "prop.network-compression-threshold",        "Compression Threshold",                              TKEY("压缩阈值"));
    add(t, "prop.online-mode",        "Online Mode",                              TKEY("在线模式"));
    add(t, "prop.op-permission-level",        "OP Permission Level",                              TKEY("OP 权限等级"));
    add(t, "prop.prevent-proxy-connections",        "Prevent Proxy Connections",                              TKEY("阻止代理连接"));
    add(t, "prop.pvp",        "PvP",                              TKEY("PvP"));
    add(t, "prop.query.port",        "Query Port",                              TKEY("查询端口"));
    add(t, "prop.rate-limit",        "Rate Limit",                              TKEY("速率限制"));
    add(t, "prop.rcon.password",        "RCON Password",                              TKEY("RCON 密码"));
    add(t, "prop.rcon.port",        "RCON Port",                              TKEY("RCON 端口"));
    add(t, "prop.require-resource-pack",        "Require Resource Pack",                              TKEY("需要资源包"));
    add(t, "prop.resource-pack",        "Resource Pack URL",                              TKEY("资源包 URL"));
    add(t, "prop.resource-pack-prompt",        "Resource Pack Prompt",                              TKEY("资源包提示"));
    add(t, "prop.resource-pack-sha1",        "Resource Pack SHA1",                              TKEY("资源包 SHA1"));
    add(t, "prop.server-port",        "Server Port",                              TKEY("服务器端口"));
    add(t, "prop.simulation-distance",        "Simulation Distance",                              TKEY("模拟距离"));
    add(t, "prop.snooper-enabled",        "Snooper Enabled",                              TKEY("启用 Snooper"));
    add(t, "prop.spawn-animals",        "Spawn Animals",                              TKEY("生成动物"));
    add(t, "prop.spawn-monsters",        "Spawn Monsters",                              TKEY("生成怪物"));
    add(t, "prop.spawn-npcs",        "Spawn NPCs",                              TKEY("生成 NPC"));
    add(t, "prop.spawn-protection",        "Spawn Protection",                              TKEY("出生点保护"));
    add(t, "prop.sync-chunk-writes",        "Sync Chunk Writes",                              TKEY("同步区块写入"));
    add(t, "prop.use-native-transport",        "Native Transport",                              TKEY("原生传输"));
    add(t, "prop.view-distance",        "View Distance",                              TKEY("视距"));
    add(t, "prop.white-list",        "White List",                              TKEY("白名单"));

    // ── Settings (missing keys) ──
    add(t, "settings.webApiGroup",  "Web API Settings",                  TKEY("Web API 设置"));
    add(t, "settings.enableWeb",    "Enable Web API",                    TKEY("启用 Web API"));
    add(t, "settings.port",         "Port:",                             TKEY("端口:"));
    add(t, "settings.apiToken",     "API Token:",                        TKEY("API 令牌:"));
    add(t, "settings.tokenHint",    "Leave empty to auto-generate",      TKEY("留空以自动生成"));
    add(t, "settings.generate",     "Generate",                          TKEY("生成"));
    add(t, "settings.webApiUrl",    "API URL:",                          TKEY("API 地址:"));
    add(t, "settings.copy",         "Copy",                              TKEY("复制"));
    add(t, "settings.serverNotRunning", "Server not running",            TKEY("服务器未运行"));
    add(t, "settings.readyPort",    "Ready on port ",                    TKEY("已就绪，端口 "));
    add(t, "settings.disabled",     "Disabled",                          TKEY("已禁用"));
    add(t, "settings.apiDesc",      "Enable the Web API to control servers remotely via HTTP requests.", TKEY("启用 Web API 以通过 HTTP 请求远程控制服务器。"));
    add(t, "settings.botTitle",     "Bot Plugin",                        TKEY("机器人插件"));
    add(t, "settings.botDesc",      "Connect QQ bots to interact with your server via commands.", TKEY("连接 QQ 机器人，通过命令与服务器交互。"));
    add(t, "settings.cmdEndpoint",  "Command Endpoint",                  TKEY("命令接口"));
    add(t, "settings.botCmdInfo",   "POST /api/bot/cmd\nBody: {\"serverId\":\"...\", \"cmd\":\"say hello\"}", TKEY("POST /api/bot/cmd\nBody: {\"serverId\":\"...\", \"cmd\":\"say hello\"}"));
    add(t, "settings.napcatGroup",  "NapCat (QQ Bot)",                   TKEY("NapCat (QQ 机器人)"));
    add(t, "settings.napcatInfo",   "Configure NapCat to connect via the command endpoint above.", TKEY("配置 NapCat 通过上述命令接口连接。"));
    add(t, "settings.nonebotGroup", "NoneBot2",                          TKEY("NoneBot2"));
    add(t, "settings.nonebotInfo",  "Configure NoneBot2 plugin to use the command endpoint above.", TKEY("配置 NoneBot2 插件使用上述命令接口。"));

    // ── Theme bg image keys ──
    add(t, "settings.bgImage",      "Background Image",                  TKEY("背景图片"));
    add(t, "settings.bgNone",       "No Image",                          TKEY("无图片"));
    add(t, "settings.bgUseImage",   "Use Custom Image",                  TKEY("使用自定义图片"));


    // ── Server Properties (labels used as translation keys in onEditProperties) ──
    add(t, "Basic Settings",        "Basic Settings",                    TKEY("基本设置"));
    add(t, "World Generation",        "World Generation",                  TKEY("世界生成"));
    add(t, "Server MOTD",             "Server MOTD",                       TKEY("服务器 MOTD"));
    add(t, "Game Mode",               "Game Mode",                         TKEY("游戏模式"));
    add(t, "Difficulty",              "Difficulty",                        TKEY("难度"));
    add(t, "Max Players",             "Max Players",                       TKEY("最大玩家数"));
    add(t, "Server Port",             "Server Port",                       TKEY("服务器端口"));
    add(t, "Online Mode",             "Online Mode",                       TKEY("在线模式"));
    add(t, "PvP",                     "PvP",                               TKEY("PvP"));
    add(t, "White List",              "White List",                        TKEY("白名单"));
    add(t, "World Name",              "World Name",                        TKEY("世界名称"));
    add(t, "World Seed",              "World Seed",                        TKEY("世界种子"));
    add(t, "World Type",              "World Type",                        TKEY("世界类型"));
    add(t, "Generate Structures",     "Generate Structures",               TKEY("生成结构"));
    add(t, "Allow Nether",            "Allow Nether",                      TKEY("允许下界"));
    add(t, "View Distance",           "View Distance",                     TKEY("视距"));
    add(t, "Simulation Distance",     "Simulation Distance",               TKEY("模拟距离"));
    add(t, "Player Settings",         "Player Settings",                   TKEY("玩家设置"));
    add(t, "Spawn Protection",        "Spawn Protection",                  TKEY("出生点保护"));
    add(t, "Max Build Height",        "Max Build Height",                  TKEY("最大建筑高度"));
    add(t, "Allow Flight",            "Allow Flight",                      TKEY("允许飞行"));
    add(t, "Force Gamemode",          "Force Gamemode",                    TKEY("强制游戏模式"));
    add(t, "Network",                 "Network",                           TKEY("网络"));
    add(t, "Enable Status",           "Enable Status",                     TKEY("启用状态"));
    add(t, "Compression Threshold",   "Compression Threshold",             TKEY("压缩阈值"));
    add(t, "Native Transport",        "Native Transport",                  TKEY("原生传输"));
    add(t, "Prevent Proxy Connections","Prevent Proxy Connections",      TKEY("阻止代理连接"));
    add(t, "RCON & Query",            "RCON & Query",                      TKEY("RCON 与查询"));
    add(t, "Enable RCON",             "Enable RCON",                       TKEY("启用RCON"));
    add(t, "RCON Password",         "RCON Password",                     TKEY("RCON密码"));
    add(t, "RCON Port",               "RCON Port",                         TKEY("RCON端口"));
    add(t, "Enable Query",            "Enable Query",                      TKEY("启用查询"));
    add(t, "Query Port",              "Query Port",                        TKEY("查询端口"));
    add(t, "Security & Permissions",  "Security & Permissions",            TKEY("安全与权限"));
    add(t, "Enforce Secure Profile",  "Enforce Secure Profile",            TKEY("强制安全档案"));
    add(t, "Command Blocks",          "Command Blocks",                    TKEY("命令方块"));
    add(t, "Broadcast Console to Ops","Broadcast Console to Ops",         TKEY("广播控制台到管理员"));
    add(t, "Broadcast RCON to Ops",   "Broadcast RCON to Ops",             TKEY("广播RCON到管理员"));
    add(t, "OP Permission Level",     "OP Permission Level",               TKEY("管理员权限等级"));
    add(t, "Function Permission Level","Function Permission Level",        TKEY("函数权限等级"));
    add(t, "World Advanced",          "World Advanced",                    TKEY("世界高级"));
    add(t, "Spawn Animals",           "Spawn Animals",                     TKEY("生成动物"));
    add(t, "Spawn Monsters",          "Spawn Monsters",                    TKEY("生成怪物"));
    add(t, "Spawn NPCs",              "Spawn NPCs",                        TKEY("生成NPC"));
    add(t, "Max World Size",          "Max World Size",                    TKEY("最大世界大小"));
    add(t, "Hardcore Mode",           "Hardcore Mode",                     TKEY("极限模式"));
    add(t, "Sync Chunk Writes",       "Sync Chunk Writes",                 TKEY("同步区块写入"));
    add(t, "Performance",             "Performance",                       TKEY("性能"));
    add(t, "Max Tick Time (ms)",      "Max Tick Time (ms)",                TKEY("最大Tick时间 (ms)"));
    add(t, "Entity Broadcast Range %","Entity Broadcast Range %",            TKEY("实体广播范围 %"));
    add(t, "Rate Limit",              "Rate Limit",                        TKEY("速率限制"));
    add(t, "Resource Pack",           "Resource Pack",                     TKEY("资源包"));
    add(t, "Resource Pack URL",       "Resource Pack URL",                 TKEY("资源包URL"));
    add(t, "Resource Pack SHA1",      "Resource Pack SHA1",                TKEY("资源包SHA1"));
    add(t, "Resource Pack Prompt",    "Resource Pack Prompt",              TKEY("资源包提示"));
    add(t, "Require Resource Pack",   "Require Resource Pack",             TKEY("需要资源包"));
    add(t, "Other",                   "Other",                             TKEY("其他"));
    add(t, "JMX Monitoring",          "JMX Monitoring",                  TKEY("JMX监控"));
    add(t, "Snooper Enabled",         "Snooper Enabled",                   TKEY("启用Snooper"));
    add(t, "mod.title",           "Mods",                              TKEY("模组"));
    add(t, "mod.market",          "Mod Market",                        TKEY("模组市场"));
    add(t, "mod.installed",       "Installed Mods",                    TKEY("已安装模组"));
    add(t, "mod.install",         "Install Mod",                       TKEY("安装模组"));
    add(t, "mod.remove",          "Remove",                            TKEY("移除"));
    add(t, "mod.enable",          "Enable",                            TKEY("启用"));
    add(t, "mod.disable",         "Disable",                           TKEY("禁用"));
    add(t, "mod.conflict",        "Conflict detected!",                TKEY("检测到冲突!"));
    add(t, "mod.noMods",          "No mods installed",                 TKEY("没有已安装的模组"));

    // ── Modpack ──
    add(t, "modpack.title",       "Modpack Import",                    TKEY("整合包导入"));
    add(t, "modpack.select",      "Select Modpack File",               TKEY("选择整合包文件"));
    add(t, "modpack.importing",   "Importing modpack...",              TKEY("正在导入整合包..."));
    add(t, "modpack.done",        "Modpack imported successfully!",    TKEY("整合包导入成功!"));

    // ── Player / World Management ──
    add(t, "player.title",        "Player Management",                 TKEY("玩家管理"));
    add(t, "player.list",         "Online Players",                    TKEY("在线玩家"));
    add(t, "player.kick",         "Kick",                              TKEY("踢出"));
    add(t, "player.ban",          "Ban",                               TKEY("封禁"));
    add(t, "player.op",           "OP",                                TKEY("管理员"));
    add(t, "world.title",         "World Management",                  TKEY("世界管理"));
    add(t, "world.backup",        "Backup World",                      TKEY("备份世界"));
    add(t, "world.seed",          "Seed: %1",                          TKEY("种子: %1"));

    // ── Command Cheat Sheet ──
    add(t, "cmd.cheatSheetTitle", "Command Cheat Sheet",               TKEY("指令速查"));
    add(t, "cmd.clickToFill",     "Click to fill command",             TKEY("点击填入命令"));
    add(t, "cmd.searchPlaceholder", "Search commands...",              TKEY("搜索指令..."));
    add(t, "cmd.categoryAll",     "All",                               TKEY("全部"));
    add(t, "cmd.categoryPlayer",  "Player",                            TKEY("玩家"));
    add(t, "cmd.categoryWorld",   "World",                             TKEY("世界"));
    add(t, "cmd.categoryServer",  "Server",                            TKEY("服务器"));
    add(t, "cmd.categoryAdmin",   "Admin",                             TKEY("管理"));

    // ── Command Descriptions ──
    add(t, "cmdDesc.help",        "Show help",                         TKEY("显示帮助"));
    add(t, "cmdDesc.list",        "List online players",               TKEY("列出在线玩家"));
    add(t, "cmdDesc.op",          "Grant operator status",             TKEY("授予管理员权限"));
    add(t, "cmdDesc.deop",        "Revoke operator status",            TKEY("撤销管理员权限"));
    add(t, "cmdDesc.kick",        "Kick a player",                     TKEY("踢出玩家"));
    add(t, "cmdDesc.ban",         "Ban a player",                      TKEY("封禁玩家"));
    add(t, "cmdDesc.pardon",      "Unban a player",                    TKEY("解封玩家"));
    add(t, "cmdDesc.banip",       "Ban an IP address",                 TKEY("封禁 IP"));
    add(t, "cmdDesc.pardonip",    "Unban an IP address",               TKEY("解封 IP"));
    add(t, "cmdDesc.whitelist",   "Manage whitelist",                  TKEY("管理白名单"));
    add(t, "cmdDesc.gamemode",    "Change game mode",                  TKEY("更改游戏模式"));
    add(t, "cmdDesc.tp",          "Teleport players",                  TKEY("传送玩家"));
    add(t, "cmdDesc.give",        "Give items",                        TKEY("给予物品"));
    add(t, "cmdDesc.kill",        "Kill a player",                     TKEY("击杀玩家"));
    add(t, "cmdDesc.msg",         "Send private message",              TKEY("发送私聊"));
    add(t, "cmdDesc.xp",          "Add experience",                    TKEY("添加经验"));
    add(t, "cmdDesc.effect",      "Apply effect",                      TKEY("施加效果"));
    add(t, "cmdDesc.enchant",     "Enchant item",                      TKEY("附魔物品"));
    add(t, "cmdDesc.time",        "Set time",                          TKEY("设置时间"));
    add(t, "cmdDesc.weather",     "Set weather",                       TKEY("设置天气"));
    add(t, "cmdDesc.gamerule",    "Set game rule",                     TKEY("设置游戏规则"));
    add(t, "cmdDesc.seed",        "Show world seed",                   TKEY("显示世界种子"));
    add(t, "cmdDesc.difficulty",  "Set difficulty",                    TKEY("设置难度"));
    add(t, "cmdDesc.defaultgamemode", "Set default game mode",         TKEY("设置默认游戏模式"));
    add(t, "cmdDesc.spawnpoint",  "Set spawn point",                   TKEY("设置出生点"));
    add(t, "cmdDesc.worldborder", "Manage world border",               TKEY("管理世界边界"));
    add(t, "cmdDesc.fill",        "Fill blocks",                       TKEY("填充方块"));
    add(t, "cmdDesc.setblock",    "Set block",                         TKEY("设置方块"));
    add(t, "cmdDesc.summon",      "Summon entity",                     TKEY("召唤实体"));
    add(t, "cmdDesc.saveall",     "Save all data",                     TKEY("保存所有数据"));
    add(t, "cmdDesc.saveoff",     "Disable auto-save",                 TKEY("禁用自动保存"));
    add(t, "cmdDesc.saveon",      "Enable auto-save",                  TKEY("启用自动保存"));
    add(t, "cmdDesc.stop",        "Stop server",                       TKEY("停止服务器"));
    add(t, "cmdDesc.reload",      "Reload data packs",                 TKEY("重载数据包"));
    add(t, "cmdDesc.say",         "Broadcast message",                 TKEY("广播消息"));
    add(t, "cmdDesc.scoreboard",  "Manage scoreboard",                 TKEY("管理计分板"));
    add(t, "cmdDesc.banlist",     "Show banned players",               TKEY("显示封禁列表"));
    add(t, "cmdDesc.advancement", "Manage advancements",             TKEY("管理进度"));

    // ── Quick Actions ──
    add(t, "cmd.qTimeDay",        "Set Day",                           TKEY("设置为白天"));
    add(t, "cmd.qWeatherClear",   "Clear Weather",                     TKEY("清除天气"));
    add(t, "cmd.qSaveAll",        "Save All",                          TKEY("保存全部"));
    add(t, "cmd.qList",           "Player List",                       TKEY("玩家列表"));
    add(t, "cmd.qGameMode",       "Creative Mode",                     TKEY("创造模式"));
    add(t, "cmd.qSay",            "Say",                               TKEY("广播"));
    add(t, "cmd.qDifficulty",     "Peaceful",                          TKEY("和平模式"));
    add(t, "cmd.qKeepInventory",  "Keep Inventory",                    TKEY("死亡不掉落"));

    // ── Panels: Mod Manager ──
    add(t, "panel.mod.title",          "Mod Manager",             TKEY("模组管理"));
    add(t, "panel.mod.installed",      "Installed Mods",          TKEY("已安装模组"));
    add(t, "panel.mod.market",         "Mod Market",              TKEY("模组市场"));
    add(t, "panel.mod.marketNote",     "Select a popular mod to install", TKEY("选择常用模组安装"));
    add(t, "panel.mod.install",        "Install",                 TKEY("安装"));
    add(t, "panel.mod.installFile",    "Install from File",       TKEY("从文件安装"));
    add(t, "panel.mod.remove",         "Remove",                  TKEY("移除"));
    add(t, "panel.mod.refresh",        "Refresh",                 TKEY("刷新"));
    add(t, "panel.mod.checkConflicts", "Check Conflicts",         TKEY("检查冲突"));
    add(t, "panel.mod.colName",        "Name",                    TKEY("名称"));
    add(t, "panel.mod.colVersion",     "Version",                 TKEY("版本"));
    add(t, "panel.mod.colLoader",      "Loader",                  TKEY("加载器"));
    add(t, "panel.mod.colEnabled",     "Enabled",                 TKEY("启用"));
    add(t, "panel.mod.colActions",     "Actions",                 TKEY("操作"));
    add(t, "panel.mod.selectJar",      "Select JAR File",         TKEY("选择 JAR 文件"));
    add(t, "panel.mod.confirmRemove",  "Confirm Remove",          TKEY("确认移除"));
    add(t, "panel.mod.removeMsg",      "Are you sure you want to remove this mod?", TKEY("确定要移除此模组吗？"));
    add(t, "panel.mod.conflictTitle",  "Conflicts Detected",      TKEY("发现冲突"));
    add(t, "panel.mod.conflictsFound", "Conflicts found:",        TKEY("发现冲突:"));
    add(t, "panel.mod.noConflicts",    "No conflicts detected",  TKEY("未发现冲突"));
    add(t, "panel.mod.conflict",       "Conflict",                TKEY("冲突"));

    // ── Panels: Modrinth Market (Desktop) ──
    add(t, "panel.mod.search",         "Search",                 TKEY("搜索"));
    add(t, "panel.mod.searchHint",     "Search Modrinth for mods...", TKEY("在 Modrinth 搜索模组..."));
    add(t, "panel.mod.searching",      "Searching...",           TKEY("搜索中..."));
    add(t, "panel.mod.searchResults",  "Found %1 results",       TKEY("找到 %1 个结果"));
    add(t, "panel.mod.noResults",      "No results found",       TKEY("未找到结果"));
    add(t, "panel.mod.networkError",   "Network error",          TKEY("网络错误"));
    add(t, "panel.mod.page",           "Page %1 / %2 (%3 hits)", TKEY("第 %1 页 / 共 %2 页 (%3 项)"));
    add(t, "panel.mod.selectVersion",  "Select Version",         TKEY("选择版本"));
    add(t, "panel.mod.selectServer",   "No Server Selected",     TKEY("未选择服务器"));
    add(t, "panel.mod.selectServerMsg","Please select a server first.", TKEY("请先选择一个服务器。"));
    add(t, "panel.mod.loadingVersions","Loading versions...",    TKEY("加载版本列表中..."));
    add(t, "panel.mod.noVersions",     "No versions available",  TKEY("没有可用版本"));
    add(t, "panel.mod.downloadingMod", "Downloading %1...",      TKEY("正在下载 %1..."));
    add(t, "panel.mod.noDownload",     "No download file available", TKEY("没有可下载的文件"));
    add(t, "panel.mod.downloadFailed", "Download failed",        TKEY("下载失败"));
    add(t, "panel.mod.emptyFile",      "Downloaded empty file",  TKEY("下载的文件为空"));
    add(t, "panel.mod.writeFailed",    "Cannot write mod file",  TKEY("无法写入模组文件"));
    add(t, "panel.mod.installedMsg",   "%1 installed successfully!", TKEY("%1 安装成功！"));

    // ── Panels: Modpack Importer ──
    add(t, "panel.modpack.title",             "Modpack Import",              TKEY("整合包导入"));
    add(t, "panel.modpack.selectSource",      "Select Modpack",              TKEY("选择整合包"));
    add(t, "panel.modpack.selectSourceDir",   "Select Modpack Directory",    TKEY("选择整合包目录"));
    add(t, "panel.modpack.selectHint",        "Choose a modpack folder or archive", TKEY("选择整合包文件夹或压缩包"));
    add(t, "panel.modpack.sourcePlaceholder", "No modpack selected",         TKEY("未选择整合包"));
    add(t, "panel.modpack.analyze",           "Analyze",                     TKEY("分析"));
    add(t, "panel.modpack.analyzing",         "Analyzing...",                TKEY("分析中..."));
    add(t, "panel.modpack.analysisResult",    "Analysis Results",            TKEY("分析结果"));
    add(t, "panel.modpack.nameLabel",         "Name",                        TKEY("名称"));
    add(t, "panel.modpack.versionLabel",      "MC Version",                  TKEY("MC 版本"));
    add(t, "panel.modpack.loaderLabel",       "Loader",                      TKEY("加载器"));
    add(t, "panel.modpack.modCountLabel",     "Mod Count",                   TKEY("模组数量"));
    add(t, "panel.modpack.mods",              "mods",                        TKEY("个模组"));
    add(t, "panel.modpack.unknown",           "Unknown",                     TKEY("未知"));
    add(t, "panel.modpack.targetDir",         "Target Directory",            TKEY("目标目录"));
    add(t, "panel.modpack.selectTargetDir",   "Select Target Directory",     TKEY("选择目标目录"));
    add(t, "panel.modpack.targetPlaceholder", "No target directory",         TKEY("未选择目标目录"));
    add(t, "panel.modpack.importBtn",         "Import & Create Server",      TKEY("导入并创建服务器"));
    add(t, "panel.modpack.installing",        "Installing...",               TKEY("安装中..."));
    add(t, "panel.modpack.importDone",        "Modpack imported successfully!", TKEY("整合包导入完成！"));
    add(t, "panel.modpack.ready",             "Ready to import",             TKEY("可导入"));
    add(t, "panel.modpack.noPath",            "Please select a modpack",     TKEY("请选择整合包"));
    add(t, "panel.modpack.noTarget",          "Please select a target directory", TKEY("请选择目标目录"));
    add(t, "panel.modpack.browse",            "Browse",                      TKEY("浏览"));

    // ── Panels: Player & World Manager ──
    add(t, "panel.playerworld.title",   "Player & World",             TKEY("玩家与世界"));
    add(t, "panel.player.title",        "Players",                    TKEY("在线玩家"));
    add(t, "panel.player.onlineCount",  "Online Players",             TKEY("在线玩家"));
    add(t, "panel.player.actions",      "Player Actions",             TKEY("玩家操作"));
    add(t, "panel.player.kick",         "Kick",                       TKEY("踢出"));
    add(t, "panel.player.ban",          "Ban",                        TKEY("封禁"));
    add(t, "panel.player.op",           "OP",                         TKEY("设为 OP"));
    add(t, "panel.player.deop",         "DeOP",                       TKEY("取消 OP"));
    add(t, "panel.player.whitelist",    "Whitelist",                  TKEY("白名单"));
    add(t, "panel.player.gamemode",     "Game Mode",                  TKEY("游戏模式"));
    add(t, "panel.player.reason",       "Reason",                     TKEY("原因"));
    add(t, "panel.player.colName",      "Name",                       TKEY("名称"));
    add(t, "panel.player.colUUID",      "UUID",                       TKEY("UUID"));
    add(t, "panel.player.colWorld",     "World",                      TKEY("所在世界"));
    add(t, "panel.player.colHealth",    "Health",                     TKEY("生命值"));
    add(t, "panel.player.colStatus",    "Status",                     TKEY("状态"));
    add(t, "panel.player.online",       "Online",                     TKEY("在线"));
    add(t, "panel.player.offline",      "Offline",                    TKEY("离线"));
    add(t, "panel.world.title",         "Worlds",                     TKEY("世界管理"));
    add(t, "panel.world.actions",       "World Actions",              TKEY("世界操作"));
    add(t, "panel.world.backup",        "Backup",                     TKEY("备份"));
    add(t, "panel.world.restore",       "Restore",                    TKEY("恢复"));
    add(t, "panel.world.delete",        "Delete",                     TKEY("删除"));
    add(t, "panel.world.colName",       "Name",                       TKEY("名称"));
    add(t, "panel.world.colSize",       "Size",                       TKEY("大小"));
    add(t, "panel.world.colSeed",       "Seed",                       TKEY("种子"));
    add(t, "panel.world.colGameType",   "Game Type",                  TKEY("游戏类型"));
    add(t, "panel.world.colLastPlayed", "Last Played",                TKEY("最后游玩"));
    add(t, "panel.world.backupDir",     "Select Backup Directory",    TKEY("选择备份目录"));
    add(t, "panel.world.backupMsg",     "Creating backup...",         TKEY("备份中..."));
    add(t, "panel.world.backupDone",    "Backup completed",           TKEY("备份完成"));
    add(t, "panel.world.selectBackup",  "Select Backup File",         TKEY("选择备份文件"));
    add(t, "panel.world.confirmDelete", "Confirm Delete",             TKEY("确认删除"));
    add(t, "panel.world.deleteMsg",     "Delete this world? This cannot be undone!", TKEY("确定要删除此世界吗？此操作不可撤销！"));
    add(t, "panel.world.error",         "Error",                      TKEY("错误"));
    add(t, "panel.refresh",             "Refresh",                    TKEY("刷新"));
}
