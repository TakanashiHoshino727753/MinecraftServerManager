#pragma once
#include <QColor>

// Forward declaration
inline QString dynamicAppStyleSheet(const QString& accent, const QString& bgStyle, const QString& customBgPath);

// 辅助函数：混合两个颜色，ratio 为 accent 的占比 (0~1)
inline QString mixColor(const QString& accentHex, const QString& baseHex, float ratio) {
    QColor accent(accentHex);
    QColor base(baseHex);
    int r = qBound(0, (int)(base.red() + (accent.red() - base.red()) * ratio), 255);
    int g = qBound(0, (int)(base.green() + (accent.green() - base.green()) * ratio), 255);
    int b = qBound(0, (int)(base.blue() + (accent.blue() - base.blue()) * ratio), 255);
    return QString("#%1%2%3")
        .arg(r, 2, 16, QLatin1Char('0'))
        .arg(g, 2, 16, QLatin1Char('0'))
        .arg(b, 2, 16, QLatin1Char('0'));
}

// Default stylesheet (uses #6c5ce7 accent)
inline QString appStyleSheet() {
    return dynamicAppStyleSheet("#6c5ce7", "none", "");
}

// Dynamic stylesheet with configurable accent color and optional custom bg image
// bgStyle: "none" or "custom"
inline QString dynamicAppStyleSheet(const QString& accent, const QString& bgStyle, const QString& customBgPath) {
    // Background tones derived from accent (dark theme, tinted by accent)
    QString coreBg  = mixColor(accent, "#080812", 0.05f);
    QString panelBg = mixColor(accent, "#12121e", 0.08f);
    QString inputBg = mixColor(accent, "rgba(15,15,26,0.8)", 0.06f);
    QString textCol = "#e0e0f0";
    QString dimCol  = "#8888aa";
    QString borderCol = mixColor(accent, "#2a2a4a", 0.10f);
    QString sidebarBg = mixColor(accent, "rgba(26,26,46,0.8)", 0.08f);
    QString btnBg = mixColor(accent, "#1a1a2e", 0.08f);
    QString btnHoverBg = mixColor(accent, "#24244a", 0.10f);
    QString hoverAlpha = "0.06";
    QString consoleBg = mixColor(accent, "#050510", 0.04f);
    QString accentAlpha = accent;

    QString bgExtra = "";
    if (bgStyle == "custom" && !customBgPath.isEmpty()) {
        bgExtra = QString(
            "QMainWindow { background-image: url(%1); background-position: center; background-repeat: no-repeat; }"
        ).arg(customBgPath);
    }

    return QString(R"(
/* ── Global ── */
QWidget {
    background-color: %1;
    color: %2;
    font-family: "Microsoft YaHei", "PingFang SC", sans-serif;
    font-size: 13px;
}
QLabel {
    background: transparent;
    border: none;
    color: %2;
}
QScrollBar:vertical { background: transparent; width: 6px; margin: 0; }
QScrollBar::handle:vertical { background: %3; border-radius: 3px; min-height: 30px; }
QScrollBar::handle:vertical:hover { background: %4; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; border: none; }
QScrollBar:horizontal { background: transparent; height: 6px; }
QScrollBar::handle:horizontal { background: %3; border-radius: 3px; min-width: 30px; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; border: none; }
QScrollArea { border: none; background: transparent; }
QScrollArea > QWidget > QWidget { background: transparent; }
#titleBar { background: %5; border-bottom: 1px solid %6; min-height: 40px; max-height: 40px; }
#titleIcon { background: transparent; border-radius: 8px; min-width: 28px; max-width: 28px; min-height: 28px; max-height: 28px; }
#titleLabel { color: %2; font-size: 13px; font-weight: 600; }
#titleVersion { color: %8; font-size: 10px; }
#winBtn { background: transparent; border: none; color: %9; padding: 0; min-width: 44px; min-height: 40px; font-size: 16px; }
#winBtn:hover { background: rgba(255,255,255,%10); color: %2; }
#winBtnClose { background: transparent; border: none; color: %9; padding: 0; min-width: 48px; min-height: 40px; font-size: 16px; }
#winBtnClose:hover { background: rgba(255,71,87,0.8); color: white; }
#sidebar { background: %11; border-right: 1px solid %6; min-width: 180px; max-width: 220px; }
#sidebarSection { color: %9; font-size: 10px; font-weight: 500; letter-spacing: 1px; padding: 4px 0; }
#sidebarStatCard { background: %12; border: 1px solid %13; border-radius: 8px; }
#sidebarStatValue { font-size: 18px; font-weight: bold; }
#sidebarStatValueTotal { color: %14; }
#sidebarStatValueRunning { color: #00d2a0; }
#sidebarStatLabel { color: %9; font-size: 10px; }
#sidebarRunningDot { background: #00d2a0; border-radius: 3px; min-width: 6px; max-width: 6px; min-height: 6px; max-height: 6px; }
#sidebarNavBtn { background: transparent; border: none; text-align: left; padding: 10px 14px; border-radius: 8px; color: %9; font-size: 13px; }
#sidebarNavBtn:hover { background: rgba(255,255,255,%10); color: %2; }
#sidebarNavBtnActive { background: rgba(108,92,231,0.15); border: 1px solid rgba(108,92,231,0.2); border-radius: 8px; color: #a29bfe; font-size: 13px; font-weight: 500; }
#sidebarNavIcon { font-size: 16px; }
#sidebarNavActiveDot { background: %7; border-radius: 3px; min-width: 6px; max-width: 6px; min-height: 6px; max-height: 6px; }
#sidebarFooter { border-top: 1px solid %6; }
#sidebarFooterText { color: %9; font-size: 10px; }
#sidebarFooterIcon { color: #ffd700; font-size: 13px; }
QPushButton { background: %15; border: 1px solid %3; border-radius: 8px; padding: 8px 18px; color: %2; }
QPushButton:hover { background: %16; border-color: %4; }
QPushButton:pressed { background: %17; }
QPushButton:disabled { color: #555577; background: #12121e; }
QPushButton#primaryBtn { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 %7, stop:1 #a29bfe); border: none; color: white; font-weight: bold; border-radius: 8px; padding: 8px 18px; }
QPushButton#primaryBtn:hover { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #7d6ff0, stop:1 #b3a8ff); }
QPushButton#primaryBtn:disabled { background: #3a3a5a; color: #666688; }
QPushButton#greenBtn { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #00d2a0, stop:1 #00e5b0); border: none; color: white; font-weight: bold; border-radius: 8px; padding: 8px 18px; }
QPushButton#greenBtn:hover { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #00e5b0, stop:1 #00ffbe); }
QPushButton#dangerBtn { background: rgba(255,71,87,0.15); border: 1px solid rgba(255,71,87,0.2); color: #ff4757; border-radius: 8px; padding: 8px 18px; }
QPushButton#dangerBtn:hover { background: rgba(255,71,87,0.25); }
QPushButton#ghostBtn { background: transparent; border: 1px solid %6; color: %9; border-radius: 8px; padding: 8px 16px; }
QPushButton#ghostBtn:hover { background: rgba(255,255,255,%10); color: %2; }
QLineEdit, QSpinBox, QComboBox { background: %18; border: 1px solid %19; border-radius: 8px; padding: 8px 12px; color: %2; selection-background-color: %7; font-size: 13px; }
QLineEdit:focus, QSpinBox:focus, QComboBox:focus { border-color: %20; }
QComboBox::drop-down { border: none; width: 24px; }
QComboBox QAbstractItemView { background: %15; border: 1px solid %3; selection-background-color: %21; outline: none; color: %2; }
QComboBox QAbstractItemView::item { padding: 6px 12px; }
QComboBox::down-arrow { image: none; }
QLabel#sectionTitle { font-size: 18px; font-weight: bold; color: %2; }
QLabel#dimLabel { color: %9; font-size: 12px; }
QLabel#statusRunning { color: #00d2a0; font-weight: bold; }
QLabel#statusStopped { color: %9; font-weight: bold; }
QGroupBox { border: 1px solid %3; border-radius: 10px; margin-top: 12px; padding-top: 18px; font-weight: bold; color: #a29bfe; }
QGroupBox::title { subcontrol-origin: margin; left: 16px; padding: 0 6px; }
QProgressBar { background: rgba(18,18,30,0.8); border: 1px solid %6; border-radius: 4px; height: 8px; text-align: center; font-size: 0px; }
QProgressBar::chunk { background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 %7, stop:1 #00d2a0); border-radius: 3px; }
QTextEdit#consoleOutput { background: %22; border: 1px solid %3; border-radius: 8px; padding: 10px; font-family: "JetBrains Mono","Fira Code","Consolas",monospace; font-size: 12px; color: %23; selection-background-color: %21; }
QFrame#serverCard { background: %24; border: 1px solid %3; border-radius: 12px; padding: 16px; }
QFrame#serverCard:hover { border-color: %25; }
QFrame#serverCardRunning { background: %24; border: 1px solid rgba(0,210,160,0.3); border-radius: 12px; padding: 16px; }
QFrame#serverCardRunning:hover { border-color: rgba(0,210,160,0.5); }
QLabel#cardTitle { font-size: 15px; font-weight: 600; color: %2; }
QLabel#cardSubtitle { font-size: 12px; color: %9; }
QLabel#cardStatusDot { border-radius: 5px; min-width: 10px; max-width: 10px; min-height: 10px; max-height: 10px; }
QLabel#cardStatusDotRunning { background: #00d2a0; }
QLabel#cardStatusDotStopped { background: #555566; }
QLabel#cardStatusText { font-size: 12px; color: %9; }
QLabel#cardInfo { font-size: 11px; color: %9; }
QLabel#badgeVanilla { color: #facc15; }
QLabel#badgePaper   { color: #60a5fa; }
QLabel#badgeSpigot  { color: #fb923c; }
QLabel#badgeForge   { color: #c084fc; }
QLabel#badgeFabric  { color: #22d3ee; }
QPushButton#typeCardVanilla { background:%26; border:1px solid rgba(250,204,21,0.3); border-radius:12px; text-align:left; padding:24px; }
QPushButton#typeCardVanilla:hover { border-color: rgba(250,204,21,0.5); }
QPushButton#typeCardVanillaSelected { background:%26; border:2px solid #facc15; border-radius:12px; text-align:left; padding:24px; }
QPushButton#typeCardPaper { background:%26; border:1px solid rgba(96,165,250,0.3); border-radius:12px; text-align:left; padding:24px; }
QPushButton#typeCardPaper:hover { border-color: rgba(96,165,250,0.5); }
QPushButton#typeCardPaperSelected { background:%26; border:2px solid #60a5fa; border-radius:12px; text-align:left; padding:24px; }
QPushButton#typeCardFabric { background:%26; border:1px solid rgba(34,211,238,0.3); border-radius:12px; text-align:left; padding:24px; }
QPushButton#typeCardFabric:hover { border-color: rgba(34,211,238,0.5); }
QPushButton#typeCardFabricSelected { background:%26; border:2px solid #22d3ee; border-radius:12px; text-align:left; padding:24px; }
QLabel#stepCircle { border-radius: 14px; min-width: 28px; max-width: 28px; min-height: 28px; max-height: 28px; font-size: 13px; font-weight: bold; qproperty-alignment: AlignCenter; }
QLabel#stepCircleActive { background: %7; color: white; }
QLabel#stepCircleDone { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #00d2a0, stop:1 #00e5b0); color: white; }
QLabel#stepCirclePending { background: %15; color: #555577; border: 1px solid %3; }
QLabel#stepLine { background: %3; min-height: 2px; max-height: 2px; min-width: 40px; }
QLabel#stepLineDone { background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #00d2a0, stop:1 %7); min-height: 2px; max-height: 2px; min-width: 40px; }
QLabel#stepLabel { font-size: 12px; qproperty-alignment: AlignCenter; }
QLabel#stepLabelActive { color: #a29bfe; font-weight: bold; }
QLabel#stepLabelDone { color: #00d2a0; }
QLabel#stepLabelPending { color: #555577; }
QFrame#confirmCard { background: %24; border: 1px solid %3; border-radius: 12px; padding: 20px; }
QLabel#confirmKey { color: %9; font-size: 13px; }
QLabel#confirmVal { color: %2; font-weight: bold; font-size: 13px; }
QLabel#detailStatIcon { color: #a29bfe; font-size: 14px; }
QLabel#detailStatLabel { color: %9; font-size: 13px; }
QLabel#detailStatValue { color: %2; font-size: 13px; font-weight: 500; }
QLabel#detailStatValueGreen { color: #00d2a0; font-size: 13px; font-weight: 500; }
QWidget#consoleHeader { background: rgba(15,15,26,0.5); border-bottom: 1px solid %6; }
QWidget#cmdInputArea { background: rgba(15,15,26,0.5); border-top: 1px solid %6; }
QLineEdit#cmdInput { background: transparent; border: none; color: %2; font-family: "JetBrains Mono","Fira Code",monospace; font-size: 13px; padding: 0; }
QLineEdit#cmdInput:focus { border: none; }
QToolTip { background: %15; color: %2; border: 1px solid %3; border-radius: 6px; padding: 6px 10px; font-size: 12px; }
QCheckBox { color: %2; font-size: 13px; spacing: 8px; }
QCheckBox::indicator { width: 18px; height: 18px; border: 1px solid %3; border-radius: 4px; background: %26; }
QCheckBox::indicator:checked { background: %7; border-color: %7; }
QCheckBox::indicator:hover { border-color: %7; }
QFrame#hDivider { background: %6; min-height:1px; max-height:1px; }
QLabel#emptyIcon { color: rgba(108,92,231,0.4); font-size: 48px; qproperty-alignment: AlignCenter; }
QLabel#emptyTitle { color: %9; font-size: 16px; font-weight: 600; }
QLabel#emptyText { color: %8; font-size: 13px; }
%27
)")
    .arg(coreBg)           // %1
    .arg(textCol)          // %2
    .arg(borderCol)        // %3
    .arg("#3a3a5a")  // %4 - scrollbar hover (fixed dark)
    .arg(coreBg)           // %5 - titleBar bg
    .arg("rgba(42,42,74,0.5)")  // %6 - titleBar border (fixed dark)
    .arg(accent)           // %7 - accent color
    .arg("rgba(136,136,170,0.5)")  // %8 - version dim (fixed dark)
    .arg(dimCol)           // %9
    .arg(hoverAlpha)       // %10 - hover alpha
    .arg(sidebarBg)        // %11 - sidebar bg
    .arg("rgba(26,26,46,0.5)") // %12 - sidebar stat card (fixed dark)
    .arg("rgba(108,92,231,0.1)")  // %13 - stat card border (fixed dark)
    .arg("#a29bfe")        // %14 - stat value total
    .arg(btnBg)            // %15
    .arg(btnHoverBg)       // %16
    .arg("#1a1a3e")  // %17 - pressed bg (fixed dark)
    .arg(inputBg)          // %18
    .arg("rgba(108,92,231,0.2)")  // %19 - input border (fixed dark)
    .arg("rgba(108,92,231,0.6)")  // %20 - input focus border (fixed dark)
    .arg("rgba(108,92,231,0.4)")  // %21 - selection bg (fixed dark)
    .arg(consoleBg)        // %22
    .arg("#c0c0d0")  // %23 - console text (fixed dark)
    .arg(panelBg)          // %24 - server card bg
    .arg("rgba(108,92,231,0.4)")  // %25 - card hover (fixed dark)
    .arg(panelBg)          // %26 - re-use panelBg for typeCards
    .arg(bgExtra);         // %27 - custom bg image
}
