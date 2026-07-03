#pragma once
#include <QString>
#include <QMap>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

// ─── PropDef: describes a server.properties entry ────────────────────

struct PropDef {
    const char* key;
    const char* defaultVal;
    const char* category;   // "basic","world","player","network","rcon","security","advanced","performance","resource","other"
    const char* label;      // human-readable label (English)
    const char* type;       // "string","number","bool","enum"
    const char* options;    // for enum: comma-separated, for number: "min,max"
};

// ─── Category definitions for UI grouping ────────────────────────────

struct PropCategory {
    const char* id;         // matches PropDef::category
    const char* label;
    bool common;            // true = shown by default (Common section)
};

static const PropCategory kPropCategories[] = {
    {"basic",       "Basic Settings",         true},
    {"world",       "World Generation",       true},
    {"player",      "Player Settings",        true},
    {"network",     "Network",                false},
    {"rcon",        "RCON & Query",           false},
    {"security",    "Security & Permissions", false},
    {"advanced",    "World Advanced",         false},
    {"performance", "Performance",            false},
    {"resource",    "Resource Pack",          false},
    {"other",       "Other",                  false},
};
static const int kPropCategoryCount = sizeof(kPropCategories) / sizeof(kPropCategories[0]);

// ─── All known server.properties entries ─────────────────────────────

static const PropDef kPropDefs[] = {
    // ── Basic ──
    {"motd",               "A Minecraft Server", "basic",       "Server MOTD",                "string", ""},
    {"gamemode",           "survival",           "basic",       "Game Mode",                  "enum",   "survival,creative,adventure,spectator"},
    {"difficulty",         "easy",               "basic",       "Difficulty",                 "enum",   "peaceful,easy,normal,hard"},
    {"max-players",        "20",                 "basic",       "Max Players",                "number", "1,999"},
    {"server-port",        "25565",              "basic",       "Server Port",                "number", "1024,65535"},
    {"online-mode",        "true",               "basic",       "Online Mode",                "bool",   ""},
    {"pvp",                "true",               "basic",       "PvP",                        "bool",   ""},
    {"white-list",         "false",              "basic",       "White List",                 "bool",   ""},
    {"level-name",         "world",              "basic",       "World Name",                 "string", ""},
    {"level-seed",         "",                   "basic",       "World Seed",                 "string", ""},

    // ── World Generation ──
    {"level-type",         "minecraft:normal",   "world",       "World Type",                 "enum",   "minecraft:normal,minecraft:flat,minecraft:large_biomes,minecraft:amplified,minecraft:single_biome_surface"},
    {"generate-structures","true",               "world",       "Generate Structures",        "bool",   ""},
    {"allow-nether",       "true",               "world",       "Allow Nether",               "bool",   ""},
    {"view-distance",      "10",                 "world",       "View Distance",              "number", "3,32"},
    {"simulation-distance","10",                 "world",       "Simulation Distance",        "number", "3,32"},

    // ── Player ──
    {"spawn-protection",   "16",                 "player",      "Spawn Protection",           "number", "0,999"},
    {"max-build-height",   "256",                "player",      "Max Build Height",           "number", "64,2032"},
    {"allow-flight",       "false",              "player",      "Allow Flight",               "bool",   ""},
    {"force-gamemode",     "false",              "player",      "Force Gamemode",             "bool",   ""},

    // ── Network ──
    {"enable-status",      "true",               "network",     "Enable Status",              "bool",   ""},
    {"network-compression-threshold", "256",     "network",     "Compression Threshold",      "number", "-1,99999"},
    {"use-native-transport","true",              "network",     "Native Transport",           "bool",   ""},
    {"prevent-proxy-connections", "false",       "network",     "Prevent Proxy Connections",  "bool",   ""},

    // ── RCON & Query ──
    {"enable-rcon",        "false",              "rcon",        "Enable RCON",                "bool",   ""},
    {"rcon.password",      "",                   "rcon",        "RCON Password",              "string", ""},
    {"rcon.port",          "25575",              "rcon",        "RCON Port",                  "number", "1024,65535"},
    {"enable-query",       "false",              "rcon",        "Enable Query",               "bool",   ""},
    {"query.port",         "25565",              "rcon",        "Query Port",                 "number", "1024,65535"},

    // ── Security ──
    {"enforce-secure-profile", "true",           "security",    "Enforce Secure Profile",     "bool",   ""},
    {"enable-command-block","false",             "security",    "Command Blocks",             "bool",   ""},
    {"broadcast-console-to-ops", "true",         "security",    "Broadcast Console to Ops",   "bool",   ""},
    {"broadcast-rcon-to-ops", "true",            "security",    "Broadcast RCON to Ops",      "bool",   ""},
    {"op-permission-level", "4",                 "security",    "OP Permission Level",        "enum",   "1,2,3,4"},
    {"function-permission-level", "2",           "security",    "Function Permission Level",  "enum",   "1,2,3,4"},

    // ── World Advanced ──
    {"spawn-animals",      "true",               "advanced",    "Spawn Animals",              "bool",   ""},
    {"spawn-monsters",     "true",               "advanced",    "Spawn Monsters",             "bool",   ""},
    {"spawn-npcs",         "true",               "advanced",    "Spawn NPCs",                 "bool",   ""},
    {"max-world-size",     "29999984",           "advanced",    "Max World Size",             "number", "1,29999984"},
    {"hardcore",           "false",              "advanced",    "Hardcore Mode",              "bool",   ""},
    {"sync-chunk-writes",  "true",               "advanced",    "Sync Chunk Writes",          "bool",   ""},

    // ── Performance ──
    {"max-tick-time",      "60000",              "performance", "Max Tick Time (ms)",         "number", "0,99999"},
    {"entity-broadcast-range-percentage", "100", "performance","Entity Broadcast Range %",    "number", "10,1000"},
    {"rate-limit",         "0",                  "performance", "Rate Limit",                 "number", "0,9999"},

    // ── Resource Pack ──
    {"resource-pack",      "",                   "resource",    "Resource Pack URL",          "string", ""},
    {"resource-pack-sha1", "",                   "resource",    "Resource Pack SHA1",         "string", ""},
    {"resource-pack-prompt","",                  "resource",    "Resource Pack Prompt",       "string", ""},
    {"require-resource-pack","false",            "resource",    "Require Resource Pack",      "bool",   ""},

    // ── Other ──
    {"enable-jmx-monitoring","false",            "other",       "JMX Monitoring",             "bool",   ""},
    {"snooper-enabled",    "true",               "other",       "Snooper Enabled",            "bool",   ""},
};
static const int kPropDefCount = sizeof(kPropDefs) / sizeof(kPropDefs[0]);

// ─── File I/O ────────────────────────────────────────────────────────

inline QMap<QString, QString> readPropertiesFile(const QString& filePath) {
    QMap<QString, QString> props;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return props;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#') || line.startsWith('!'))
            continue;
        int eq = line.indexOf('=');
        if (eq < 0) {
            int colon = line.indexOf(':');
            if (colon >= 0) eq = colon;
        }
        if (eq >= 0) {
            QString key = line.left(eq).trimmed();
            QString val = line.mid(eq + 1).trimmed();
            val.replace("\\n", "\n").replace("\\t", "\t").replace("\\\\", "\\").replace("\\:", ":");
            props[key] = val;
        }
    }
    file.close();
    return props;
}

inline bool writePropertiesFile(const QString& filePath, const QMap<QString, QString>& props) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out << "# Minecraft server properties\n";
    out << "# " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n";

    QMapIterator<QString, QString> it(props);
    while (it.hasNext()) {
        it.next();
        QString val = it.value();
        val.replace("\\", "\\\\").replace("\n", "\\n").replace("\t", "\\t");
        out << it.key() << "=" << val << "\n";
    }
    file.close();
    return true;
}
