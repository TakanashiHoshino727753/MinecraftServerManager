#pragma once
#include <QString>

// ─── Embedded Web UI Dashboard (v3 — multi-tab server details) ───
// Served at http://localhost:<port>/

static const char* kWebUiHtml = R"html(<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>MC Server Manager</title>
<style>
/* ── Reset & Base ── */
*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
:root {
  --bg: #080812; --panel: #0e0e1a; --card: #12122a;
  --border: #1e1e3a; --text: #d0d0e8; --dim: #6a6a8a;
  --accent: #6c5ce7; --accent2: #a29bfe; --green: #00d2a0;
  --red: #ff6b6b; --orange: #fb923c; --yellow: #facc15;
  --cyan: #22d3ee; --blue: #60a5fa; --purple: #c084fc;
  --radius: 10px; --radius-sm: 8px;
  --font: -apple-system, 'Segoe UI', Roboto, sans-serif;
  --mono: 'SF Mono', 'Cascadia Code', 'Consolas', monospace;
}
body { font-family: var(--font); background: var(--bg); color: var(--text); min-height: 100vh; overflow: hidden; }

/* ── Header ── */
header {
  display: flex; align-items: center; gap: 16px;
  padding: 12px 24px; background: var(--panel);
  border-bottom: 1px solid var(--border); z-index: 10;
}
.logo { font-size: 18px; font-weight: 700; color: var(--accent2); cursor: pointer; }
.header-stats { margin-left: auto; display: flex; gap: 18px; font-size: 12px; color: var(--dim); }
.header-stats span { color: var(--text); font-weight: 600; }
.header-stats .running { color: var(--green); }
.dl-indicator { display: none; align-items: center; gap: 8px; margin-left: 16px; font-size: 12px; }
.dl-indicator.show { display: flex; }
.dl-bar { width: 100px; height: 4px; background: #2a2a4a; border-radius: 2px; overflow: hidden; }
.dl-bar-fill { height: 100%; background: var(--accent2); transition: width .3s; }

/* ── Layout ── */
main { display: flex; height: calc(100vh - 51px); }
.sidebar {
  width: 280px; background: var(--panel); border-right: 1px solid var(--border);
  display: flex; flex-direction: column; flex-shrink: 0;
}
.sidebar-nav {
  display: flex; border-bottom: 1px solid var(--border);
}
.sidebar-nav .nav-btn {
  flex: 1; padding: 10px; text-align: center; font-size: 12px; font-weight: 600;
  color: var(--dim); cursor: pointer; border-bottom: 2px solid transparent; transition: .15s;
  background: none; border-top: none; border-left: none; border-right: none; font-family: var(--font);
}
.sidebar-nav .nav-btn.active { color: var(--accent2); border-bottom-color: var(--accent); }
.sidebar-nav .nav-btn:hover { color: var(--text); }
.server-list { flex: 1; overflow-y: auto; padding: 6px; }
.server-list::-webkit-scrollbar { width: 5px; }
.server-list::-webkit-scrollbar-thumb { background: #333; border-radius: 3px; }

/* ── Server Card ── */
.server-card {
  background: var(--card); border: 1px solid var(--border); border-radius: var(--radius);
  padding: 12px; margin-bottom: 6px; cursor: pointer; transition: .15s;
}
.server-card:hover { border-color: #3a3a5a; }
.server-card.active { border-color: var(--accent); background: rgba(108,92,231,.08); }
.server-card.selected { border-color: var(--accent2); background: rgba(162,155,254,.06); }
.server-card .sc-top { display: flex; align-items: center; gap: 8px; margin-bottom: 6px; }
.server-card .sc-dot { width: 8px; height: 8px; border-radius: 50%; flex-shrink: 0; }
.server-card .sc-dot.online { background: var(--green); box-shadow: 0 0 6px var(--green); }
.server-card .sc-dot.offline { background: #444; }
.server-card .sc-name { font-weight: 600; font-size: 13px; flex: 1; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
.server-card .sc-badge {
  display: inline-block; padding: 1px 8px; border-radius: 10px; font-size: 10px;
  font-weight: 600; text-transform: uppercase; flex-shrink: 0;
}
.badge-vanilla { background: rgba(250,204,21,.15); color: var(--yellow); }
.badge-paper { background: rgba(96,165,250,.15); color: var(--blue); }
.badge-fabric { background: rgba(34,211,238,.15); color: var(--cyan); }
.badge-spigot { background: rgba(251,146,60,.15); color: var(--orange); }
.badge-forge { background: rgba(192,132,252,.15); color: var(--purple); }
.server-card .sc-info { font-size: 11px; color: var(--dim); margin-bottom: 8px; display: flex; gap: 10px; flex-wrap: wrap; }
.server-card .sc-info span { white-space: nowrap; }
.server-card .sc-path { font-size: 10px; color: rgba(106,106,138,.5); margin-bottom: 8px; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
.server-card .sc-actions { display: flex; gap: 6px; }
.sc-btn {
  flex: 1; padding: 5px 0; border: none; border-radius: 6px; font-size: 11px; font-weight: 600;
  cursor: pointer; transition: .15s; font-family: var(--font); text-align: center;
}
.sc-btn-start { background: rgba(0,210,160,.12); color: var(--green); }
.sc-btn-start:hover { background: rgba(0,210,160,.22); }
.sc-btn-stop { background: rgba(255,107,107,.12); color: var(--red); }
.sc-btn-stop:hover { background: rgba(255,107,107,.22); }
.sc-btn-edit { background: transparent; border: 1px solid var(--border); color: var(--dim); flex: 0; padding: 5px 10px; border-radius: 6px; font-size: 14px; }
.sc-btn-edit:hover { border-color: var(--text); color: var(--text); }
.sc-btn-go { background: transparent; border: 1px solid var(--border); color: var(--dim); flex: 0; padding: 5px 10px; border-radius: 6px; font-size: 14px; }
.sc-btn-go:hover { border-color: var(--accent2); color: var(--accent2); }

/* ── Empty state ── */
.sidebar-empty { padding: 30px 16px; text-align: center; color: var(--dim); font-size: 12px; }
.add-server-btn {
  margin: 6px 8px; padding: 10px; border: 1px dashed var(--border); border-radius: var(--radius-sm);
  text-align: center; color: var(--dim); cursor: pointer; font-size: 13px; transition: .15s;
}
.add-server-btn:hover { border-color: var(--accent2); color: var(--accent2); }

.sidebar-footer {
  padding: 8px 16px; border-top: 1px solid var(--border);
  font-size: 11px; color: var(--dim); display: flex; align-items: center; gap: 8px; flex-shrink: 0;
}
.sidebar-footer .ind { width: 6px; height: 6px; border-radius: 50%; background: var(--green); }
.sidebar-footer .ind.off { background: #444; }

/* ── Content Area ── */
.content { flex: 1; display: flex; flex-direction: column; overflow: hidden; min-width: 0; }

/* ── Tab Bar ── */
.tab-bar {
  display: none; align-items: flex-end; gap: 0;
  padding: 0 16px; background: var(--panel); border-bottom: 1px solid var(--border);
  overflow-x: auto; overflow-y: hidden; flex-shrink: 0; min-height: 36px;
}
.tab-bar::-webkit-scrollbar { height: 3px; }
.tab-bar::-webkit-scrollbar-thumb { background: #333; border-radius: 2px; }
.tab-bar.show { display: flex; }
.tab-item {
  display: flex; align-items: center; gap: 6px;
  padding: 7px 12px; font-size: 12px; font-weight: 500;
  color: var(--dim); cursor: pointer; border: 1px solid transparent;
  border-bottom: none; border-radius: 6px 6px 0 0;
  white-space: nowrap; transition: .12s; max-width: 180px;
  position: relative; flex-shrink: 0;
}
.tab-item:hover { color: var(--text); background: rgba(255,255,255,.03); }
.tab-item.active {
  color: var(--text); background: var(--card);
  border-color: var(--border); font-weight: 600;
}
.tab-item .tab-name { overflow: hidden; text-overflow: ellipsis; }
.tab-item .tab-close {
  width: 16px; height: 16px; border-radius: 3px; border: none;
  background: transparent; color: var(--dim); font-size: 14px; line-height: 1;
  cursor: pointer; display: flex; align-items: center; justify-content: center;
  flex-shrink: 0; transition: .12s; padding: 0;
}
.tab-item .tab-close:hover { background: rgba(255,107,107,.2); color: var(--red); }

/* ── Tab Panels ── */
.tab-panels { display: none; flex: 1; overflow: hidden; }
.tab-panels.show { display: flex; }
.tab-panel { display: none; flex: 1; flex-direction: column; overflow: hidden; }
.tab-panel.active { display: flex; }

/* ── Welcome / Empty State ── */
.empty-state {
  flex: 1; display: flex; flex-direction: column; align-items: center; justify-content: center; gap: 14px; color: var(--dim);
}
.empty-state .icon { font-size: 52px; opacity: .3; }
.empty-state .title { font-size: 18px; font-weight: 700; color: var(--text); }
.empty-state .sub { font-size: 13px; }
.btn { padding: 10px 20px; border-radius: var(--radius-sm); border: none; font-size: 13px; font-weight: 600;
  cursor: pointer; transition: .15s; font-family: var(--font); }
.btn-primary { background: var(--accent); color: #fff; }
.btn-primary:hover { background: #7c6cf7; }
.btn-green { background: var(--green); color: #000; }
.btn-green:hover { opacity: .85; }
.btn-red { background: var(--red); color: #fff; }
.btn-red:hover { opacity: .85; }
.btn-orange { background: var(--orange); color: #000; }
.btn-orange:hover { opacity: .85; }
.btn-ghost { background: transparent; border: 1px solid var(--border); color: var(--dim); }
.btn-ghost:hover { border-color: var(--text); color: var(--text); }
.btn-sm { padding: 5px 12px; font-size: 11px; }

/* ── Detail View (inside tab panel) ── */
.detail { flex: 1; display: flex; flex-direction: column; overflow: hidden; }
.detail-top {
  padding: 12px 24px; display: flex; align-items: center; gap: 10px;
  border-bottom: 1px solid var(--border); flex-shrink: 0; flex-wrap: wrap;
}
.detail-top .sname { font-size: 17px; font-weight: 700; }
.detail-top .sstatus { font-size: 11px; padding: 2px 10px; border-radius: 12px; font-weight: 600; }
.detail-top .sstatus.online { background: rgba(0,210,160,.15); color: var(--green); }
.detail-top .sstatus.offline { background: rgba(106,106,138,.15); color: var(--dim); }
.detail-actions { margin-left: auto; display: flex; gap: 6px; flex-wrap: wrap; }

.stats-bar {
  display: grid; grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
  gap: 8px; padding: 10px 24px; flex-shrink: 0;
}
.stat-card {
  background: var(--card); border-radius: var(--radius-sm); padding: 10px 14px; border: 1px solid var(--border);
}
.stat-card .label { font-size: 10px; color: var(--dim); text-transform: uppercase; letter-spacing: .5px; }
.stat-card .value { font-size: 14px; font-weight: 700; color: var(--text); margin-top: 2px; word-break: break-all; }
.stat-card .value.green { color: var(--green); }

/* ── Console ── */
.console-wrap {
  flex: 1; display: flex; flex-direction: column; overflow: hidden;
  margin: 0 24px 14px 24px; background: #000; border-radius: var(--radius);
  border: 1px solid var(--border);
}
.console-header {
  display: flex; align-items: center; justify-content: space-between;
  padding: 6px 14px; border-bottom: 1px solid var(--border); font-size: 10px; color: var(--dim);
}
.console-out {
  flex: 1; overflow-y: auto; padding: 8px 14px;
  font-family: var(--mono); font-size: 11px; line-height: 1.6; color: #aaa;
}
.console-out .line { white-space: pre-wrap; word-break: break-all; }
.console-out .line.error { color: var(--red); }
.console-out .line.warn { color: var(--yellow); }
.console-out .line.join { color: var(--green); }
.console-out .line.leave { color: #888; }
.console-out .line.info { color: var(--cyan); }
.console-inp {
  display: flex; border-top: 1px solid var(--border); padding: 6px 10px; gap: 8px;
}
.console-inp .prompt { color: var(--accent2); font-family: var(--mono); font-size: 13px; line-height: 28px; }
.console-inp input {
  flex: 1; background: transparent; border: none; outline: none;
  color: var(--text); font-family: var(--mono); font-size: 12px;
}
.console-inp input::placeholder { color: #444; }
.console-inp input:disabled { opacity: .4; }

/* ── Quick Actions ── */
.quick-actions { display: flex; align-items: center; gap: 6px; flex-wrap: wrap; padding: 8px 24px; flex-shrink: 0; }
.quick-actions .qa-label { font-size: 11px; color: rgba(106,106,138,.5); letter-spacing: 0; margin-right: 8px; flex-shrink: 0; white-space: nowrap; min-width: 24px; }
.qa-btn { display: inline-flex; align-items: center; gap: 3px; padding: 3px 10px; border-radius: 20px; font-size: 10px; font-weight: 500; background: rgba(108,92,231,.08); border: 1px solid rgba(108,92,231,.15); color: rgba(162,155,254,.7); cursor: pointer; transition: .12s; font-family: var(--font); white-space: nowrap; }
.qa-btn:hover { background: rgba(108,92,231,.18); border-color: rgba(108,92,231,.3); color: var(--accent2); }
.qa-btn:disabled { opacity: .25; cursor: not-allowed; }

/* ── Cheat Sheet Button (console header) ── */
.cheat-btn { display: inline-flex; align-items: center; gap: 4px; padding: 3px 10px; border-radius: 6px; font-size: 10px; font-weight: 500; background: rgba(108,92,231,.08); border: 1px solid rgba(108,92,231,.15); color: rgba(162,155,254,.7); cursor: pointer; transition: .12s; font-family: var(--font); margin-right: 10px; }
.cheat-btn:hover { background: rgba(108,92,231,.18); border-color: rgba(108,92,231,.3); color: var(--accent2); }
.cheat-btn:disabled { opacity: .25; cursor: not-allowed; }

/* ── Cheat Sheet Modal ── */
.cheat-modal { width: 640px; max-width: 94vw; max-height: 80vh; overflow: hidden; display: flex; flex-direction: column; padding: 0 !important; }
.cheat-modal-hd { display: flex; align-items: center; justify-content: space-between; padding: 18px 22px 12px 22px; border-bottom: 1px solid var(--border); flex-shrink: 0; }
.cheat-modal-hd .cm-title { font-size: 18px; font-weight: 700; color: var(--accent2); }
.cheat-modal-hd .cm-hint { font-size: 10px; color: rgba(106,106,138,.3); margin-right: 12px; }
.cm-search { padding: 10px 22px; flex-shrink: 0; position: relative; }
.cm-search input { width: 100%; padding: 8px 12px 8px 32px; background: var(--card); border: 1px solid var(--border); border-radius: 8px; color: var(--text); font-size: 13px; font-family: var(--font); outline: none; }
.cm-search input:focus { border-color: var(--accent2); }
.cm-search .cm-search-icon { position: absolute; left: 30px; top: 50%; transform: translateY(-50%); color: rgba(106,106,138,.4); font-size: 13px; pointer-events: none; }
.cm-cats { display: flex; gap: 6px; padding: 0 22px 8px; flex-shrink: 0; flex-wrap: wrap; }
.cm-cat { padding: 4px 12px; border-radius: 20px; font-size: 11px; font-weight: 500; background: transparent; border: 1px solid transparent; color: var(--dim); cursor: pointer; transition: .12s; font-family: var(--font); }
.cm-cat:hover { border-color: var(--border); color: var(--text); }
.cm-cat.active { background: rgba(108,92,231,.15); border-color: rgba(108,92,231,.3); color: var(--accent2); }
.cm-list { flex: 1; overflow-y: auto; padding: 4px 16px 16px; min-height: 200px; }
.cm-item { display: flex; align-items: baseline; gap: 10px; padding: 7px 10px; border-radius: 6px; cursor: pointer; transition: .08s; }
.cm-item:hover { background: rgba(108,92,231,.08); }
.cm-item code { font-family: var(--mono); font-size: 13px; color: var(--accent2); min-width: 180px; flex-shrink: 0; }
.cm-item .cm-desc { font-size: 11px; color: var(--dim); }
.cm-empty { text-align: center; color: rgba(106,106,138,.3); padding: 30px; font-size: 12px; }

/* ── Command Suggestions ── */
.cmd-suggest-wrap { position: relative; flex: 1; display: flex; }
.cmd-suggest { display: none; position: absolute; left: 0; right: 0; bottom: 100%; margin-bottom: 4px; background: var(--card); border: 1px solid var(--border); border-radius: 6px; max-height: 180px; overflow-y: auto; z-index: 10; }
.cmd-suggest.show { display: block; }
.cmd-suggest-item { display: flex; align-items: center; gap: 8px; padding: 7px 12px; cursor: pointer; transition: .06s; }
.cmd-suggest-item:hover, .cmd-suggest-item.active { background: rgba(108,92,231,.12); }
.cmd-suggest-item code { font-family: var(--mono); font-size: 12px; color: var(--accent2); min-width: 130px; flex-shrink: 0; }
.cmd-suggest-item .sug-desc { font-size: 10px; color: var(--dim); }

/* ── Settings Page ── */
.settings-page { flex: 1; overflow-y: auto; padding: 24px; }
.settings-page h2 { font-size: 20px; font-weight: 700; color: var(--accent2); margin-bottom: 20px; }
.settings-group {
  background: var(--card); border: 1px solid var(--border); border-radius: var(--radius);
  padding: 18px 20px; margin-bottom: 14px;
}
.settings-group h3 { font-size: 14px; font-weight: 700; color: var(--accent2); margin-bottom: 12px; }
.settings-row { display: flex; align-items: center; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid rgba(30,30,58,.5); }
.settings-row:last-child { border-bottom: none; }
.settings-row .skey { font-size: 13px; color: var(--dim); }
.settings-row .sval { font-size: 13px; font-weight: 600; color: var(--text); font-family: var(--mono); }
.settings-row .sval.green { color: var(--green); }
.settings-row .sval.red { color: var(--red); }
.settings-copy { background: var(--bg); border: 1px solid var(--border); color: var(--text); font-family: var(--mono);
  padding: 6px 12px; border-radius: 6px; font-size: 12px; margin-right: 8px; min-width: 200px; text-align: center; }
.settings-input { background: var(--card); border: 1px solid var(--border); color: var(--text);
  padding: 6px 12px; border-radius: 6px; font-size: 13px; font-family: var(--mono); width: 120px; text-align: center; }
.settings-input:focus { border-color: var(--accent2); outline: none; }
.settings-desc { font-size: 11px; color: var(--dim); margin-top: 8px; line-height: 1.5; }
.token-row { display: flex; align-items: center; gap: 6px; }

/* ── Create Wizard ── */
.wizard { flex: 1; overflow-y: auto; padding: 24px; display: flex; flex-direction: column; }
.wizard-steps { display: flex; align-items: center; justify-content: center; gap: 0; margin-bottom: 24px; }
.wizard-step {
  display: flex; align-items: center; gap: 6px; font-size: 12px; font-weight: 600;
  color: var(--dim); padding: 6px 14px; border-radius: var(--radius-sm);
}
.wizard-step.active { color: var(--accent2); }
.wizard-step.done { color: var(--green); }
.wizard-step .num {
  width: 24px; height: 24px; border-radius: 50%; display: flex; align-items: center; justify-content: center;
  font-size: 11px; font-weight: 700; border: 2px solid var(--border);
}
.wizard-step.active .num { border-color: var(--accent2); background: rgba(162,155,254,.15); }
.wizard-step.done .num { border-color: var(--green); background: rgba(0,210,160,.15); }
.wizard-line { width: 36px; height: 2px; background: var(--border); margin: 0 2px; }
.wizard-line.done { background: var(--green); }

.type-cards { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 10px; margin-bottom: 20px; }
.type-card {
  background: var(--card); border: 2px solid var(--border); border-radius: var(--radius);
  padding: 18px; cursor: pointer; transition: .15s; text-align: center;
}
.type-card:hover { border-color: var(--dim); }
.type-card.selected { border-color: var(--accent); }
.type-card .t-icon { font-size: 32px; margin-bottom: 8px; }
.type-card .t-name { font-size: 15px; font-weight: 700; margin-bottom: 4px; }
.type-card .t-desc { font-size: 11px; color: var(--dim); }

.form-group { margin-bottom: 14px; }
.form-group label { display: block; font-size: 12px; color: var(--dim); margin-bottom: 5px; font-weight: 600; }
.form-group input, .form-group select {
  width: 100%; padding: 9px 12px; background: var(--card); border: 1px solid var(--border);
  border-radius: var(--radius-sm); color: var(--text); font-size: 13px; font-family: var(--font); outline: none;
}
.form-group input:focus, .form-group select:focus { border-color: var(--accent2); }
.form-row { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }

.check-row { display: flex; align-items: center; gap: 8px; margin: 14px 0; }
.check-row input[type=checkbox] { width: 16px; height: 16px; accent-color: var(--accent2); cursor: pointer; }
.check-row label { font-size: 12px; color: var(--text); cursor: pointer; }

.wizard-nav { display: flex; justify-content: space-between; margin-top: auto; padding-top: 16px; }

.confirm-card {
  background: var(--card); border: 1px solid var(--border); border-radius: var(--radius);
  padding: 20px; max-width: 480px; margin: 0 auto;
}
.confirm-card .row { display: flex; justify-content: space-between; padding: 7px 0; border-bottom: 1px solid var(--border); font-size: 13px; }
.confirm-card .row:last-child { border-bottom: none; }
.confirm-card .key { color: var(--dim); }
.confirm-card .val { color: var(--text); font-weight: 600; }

/* ── Modal ── */
.modal-overlay {
  display: none; position: fixed; inset: 0; background: rgba(0,0,0,.6); z-index: 100;
  align-items: center; justify-content: center;
}
.modal-overlay.show { display: flex; }
.modal {
  background: var(--panel); border: 1px solid var(--border); border-radius: var(--radius);
  padding: 22px; min-width: 380px; max-width: 480px; max-height: 80vh; overflow-y: auto;
}
.modal h3 { font-size: 16px; font-weight: 700; margin-bottom: 14px; color: var(--accent2); }
.modal .actions { display: flex; justify-content: flex-end; gap: 8px; margin-top: 18px; }

/* ── Notifications ── */
.notify {
  position: fixed; top: 60px; right: 20px; padding: 10px 18px;
  border-radius: var(--radius-sm); font-size: 12px; z-index: 200; animation: slideIn .3s ease;
  background: var(--card); border: 1px solid var(--border); max-width: 340px;
}
.notify.ok { border-color: var(--green); }
.notify.err { border-color: var(--red); }
@keyframes slideIn { from { transform: translateX(20px); opacity: 0; } to { transform: translateX(0); opacity: 1; } }

::-webkit-scrollbar { width: 5px; }
::-webkit-scrollbar-thumb { background: #333; border-radius: 3px; }

@media (max-width: 768px) {
  .sidebar { width: 100%; max-height: 180px; }
  main { flex-direction: column; }
  .stats-bar { grid-template-columns: repeat(2, 1fr); }
  .form-row { grid-template-columns: 1fr; }
  .type-cards { grid-template-columns: 1fr; }
  .modal { min-width: auto; margin: 0 12px; }
  .props-form { display: grid; grid-template-columns: 1fr 1fr; gap: 0 24px; }
  .props-group { grid-column: 1 / -1; }
  .props-single { grid-column: 1 / -1; }
}
@media (min-width: 769px) {
  .props-form { grid-template-columns: 1fr 1fr; }
  .props-group { grid-column: 1 / -1; }
  .props-half { }
}

/* ── Properties Modal ── */
.props-modal { width: 860px !important; max-width: 94vw !important; max-height: 85vh !important; overflow-y: auto; }
.props-section-title {
  font-size: 14px; font-weight: 700; color: var(--accent2);
  padding: 10px 0 6px 0; margin: 12px 0 4px 0;
  border-bottom: 1px solid var(--border); cursor: pointer; user-select: none;
}
.props-section-title .arrow { font-size: 10px; margin-right: 6px; transition: .2s; display: inline-block; }
.props-section-title.collapsed .arrow { transform: rotate(-90deg); }
.props-section-body { display: grid; grid-template-columns: 1fr 1fr; gap: 6px 16px; padding: 6px 0; }
.props-item { display: flex; flex-direction: column; gap: 3px; padding: 5px 0; }
.props-item label { font-size: 11px; color: var(--dim); font-weight: 600; }
.props-item input, .props-item select {
  background: var(--card); border: 1px solid var(--border); color: var(--text);
  padding: 6px 10px; border-radius: 6px; font-size: 12px; font-family: var(--font); outline: none; width: 100%;
}
.props-item input:focus, .props-item select:focus { border-color: var(--accent2); }
.props-item input[type=checkbox] { width: 16px; height: 16px; accent-color: var(--accent2); cursor: pointer; }
.props-check-row { display: flex; align-items: center; gap: 8px; padding: 5px 0; }
.props-toggle { text-align: center; padding: 10px 0; }
.props-toggle button { font-size: 12px; color: var(--dim); background: transparent; border: 1px solid var(--border); border-radius: 6px; padding: 6px 16px; cursor: pointer; font-family: var(--font); }
.props-toggle button:hover { border-color: var(--accent2); color: var(--accent2); }
.props-save-row { text-align: right; padding: 14px 0 4px 0; border-top: 1px solid var(--border); margin-top: 14px; }

/* ── Mod Management ── */
#modModal .modal { max-width: 720px; }
.mod-list { max-height: 320px; overflow-y: auto; margin-bottom: 10px; }
.mod-item { display: flex; align-items: center; gap: 12px; padding: 10px 12px; border: 1px solid var(--border); border-radius: 8px; margin-bottom: 6px; background: var(--card); transition: .2s; }
.mod-item.disabled { opacity: .45; }
.mod-item:hover { border-color: var(--accent2); }
.mod-item .mod-icon { width: 36px; height: 36px; border-radius: 6px; display: flex; align-items: center; justify-content: center; font-size: 16px; flex-shrink: 0; background: linear-gradient(135deg, var(--accent), var(--accent2)); color: #fff; }
.mod-item .mod-info { flex: 1; min-width: 0; }
.mod-item .mod-info .mod-name { font-weight: 600; font-size: 13px; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
.mod-item .mod-info .mod-meta { font-size: 11px; color: var(--dim); margin-top: 2px; }
.mod-item .mod-loader { font-size: 10px; padding: 2px 8px; border-radius: 10px; background: rgba(108,92,231,.15); color: var(--accent2); white-space: nowrap; }
.mod-item .mod-actions { display: flex; gap: 4px; flex-shrink: 0; }
.mod-item .mod-actions button { padding: 4px 10px; font-size: 11px; border-radius: 5px; border: 1px solid var(--border); background: transparent; color: var(--dim); cursor: pointer; font-family: var(--font); transition: .2s; }
.mod-item .mod-actions button:hover { border-color: var(--accent2); color: var(--accent2); }
.mod-item .mod-actions .mod-remove:hover { border-color: var(--red); color: var(--red); }
.mod-empty { text-align: center; padding: 30px; color: var(--dim); font-size: 13px; }
.mod-conflict { padding: 8px 12px; border: 1px solid rgba(251,146,60,.3); border-radius: 6px; background: rgba(251,146,60,.06); margin-bottom: 6px; font-size: 12px; color: var(--orange); }
.mod-market-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(200px,1fr)); gap: 8px; max-height: 260px; overflow-y: auto; margin-top: 8px; }
.mod-market-card { padding: 10px 12px; border: 1px solid var(--border); border-radius: 8px; background: var(--card); font-size: 12px; }
.mod-market-card .mmc-name { font-weight: 600; font-size: 13px; margin-bottom: 3px; }
.mod-market-card .mmc-desc { font-size: 11px; color: var(--dim); margin-bottom: 6px; }
.mod-market-card .mmc-loader { font-size: 10px; padding: 1px 6px; border-radius: 8px; background: rgba(108,92,231,.12); color: var(--accent2); }
.mod-market-card .mmc-versions { font-size: 10px; color: var(--dim); margin-top: 4px; }
.mod-tabs { display: flex; gap: 0; margin-bottom: 12px; }
.mod-tab { padding: 7px 18px; background: transparent; border: 1px solid var(--border); color: var(--dim); cursor: pointer; font-family: var(--font); font-size: 12px; border-radius: 0; }
.mod-tab:first-child { border-radius: 6px 0 0 6px; }
.mod-tab:last-child { border-radius: 0 6px 6px 0; }
.mod-tab.active { background: var(--accent); color: #fff; border-color: var(--accent); }

/* ── Player / World Modal ── */
#pwModal .modal { max-width: 780px; min-width: 560px; }
.pw-table { width: 100%; border-collapse: collapse; font-size: 12px; margin-bottom: 8px; }
.pw-table th, .pw-table td { padding: 8px 10px; text-align: left; border-bottom: 1px solid var(--border); }
.pw-table th { color: var(--dim); font-weight: 600; font-size: 11px; text-transform: uppercase; }
.pw-table td { color: var(--text); }
.pw-table .pw-online { color: var(--green); font-weight: 600; }
.pw-table .pw-offline { color: var(--dim); }
.pw-table .pw-actions { display: flex; gap: 4px; }
.pw-table .pw-actions button { padding: 3px 8px; font-size: 10px; border-radius: 4px; border: 1px solid var(--border); background: transparent; color: var(--dim); cursor: pointer; font-family: var(--font); transition: .15s; }
.pw-table .pw-actions button:hover { border-color: var(--accent2); color: var(--accent2); }
.pw-table .pw-actions .pw-danger:hover { border-color: var(--red); color: var(--red); }
.pw-tabs { display: flex; gap: 0; margin-bottom: 12px; }
.pw-tab { padding: 7px 18px; background: transparent; border: 1px solid var(--border); color: var(--dim); cursor: pointer; font-family: var(--font); font-size: 12px; border-radius: 0; }
.pw-tab:first-child { border-radius: 6px 0 0 6px; }
.pw-tab:last-child { border-radius: 0 6px 6px 0; }
.pw-tab.active { background: var(--accent); color: #fff; border-color: var(--accent); }
.pw-empty { text-align: center; padding: 24px; color: var(--dim); font-size: 13px; }

/* ── Modpack Modal ── */
#mpModal .modal { max-width: 680px; }
.mp-upload-zone { border: 2px dashed var(--border); border-radius: var(--radius); padding: 28px; text-align: center; cursor: pointer; transition: .2s; margin-bottom: 14px; }
.mp-upload-zone:hover, .mp-upload-zone.dragover { border-color: var(--accent2); background: rgba(108,92,231,.05); }
.mp-upload-zone .mp-upload-icon { font-size: 36px; margin-bottom: 8px; }
.mp-upload-zone .mp-upload-text { font-size: 13px; color: var(--dim); }
.mp-upload-zone .mp-upload-hint { font-size: 11px; color: rgba(106,106,138,.4); margin-top: 4px; }
.mp-upload-zone input[type=file] { display: none; }
.mp-analysis { background: var(--card); border: 1px solid var(--border); border-radius: var(--radius-sm); padding: 14px 18px; margin-bottom: 14px; }
.mp-analysis .mp-row { display: flex; justify-content: space-between; padding: 5px 0; border-bottom: 1px solid rgba(30,30,58,.4); font-size: 13px; }
.mp-analysis .mp-row:last-child { border-bottom: none; }
.mp-analysis .mp-key { color: var(--dim); }
.mp-analysis .mp-val { font-weight: 600; color: var(--text); }

/* ── Modrinth Search (Market) ── */
.modrinth-search { display: flex; gap: 8px; margin-bottom: 12px; }
.modrinth-search input { flex: 1; padding: 8px 12px; background: var(--card); border: 1px solid var(--border); border-radius: 8px; color: var(--text); font-size: 13px; font-family: var(--font); outline: none; }
.modrinth-search input:focus { border-color: var(--accent2); }
.modrinth-search .mr-loader-select { width: 100px; padding: 8px 6px; background: var(--card); border: 1px solid var(--border); border-radius: 8px; color: var(--text); font-size: 12px; font-family: var(--font); outline: none; }
.modrinth-results { max-height: 360px; overflow-y: auto; }
.mr-card { display: flex; gap: 12px; padding: 10px 12px; border: 1px solid var(--border); border-radius: 8px; margin-bottom: 6px; background: var(--card); cursor: pointer; transition: .15s; align-items: center; }
.mr-card:hover { border-color: var(--accent2); }
.mr-card .mr-icon { width: 40px; height: 40px; border-radius: 8px; background: rgba(108,92,231,.15); flex-shrink: 0; display: flex; align-items: center; justify-content: center; font-size: 18px; overflow: hidden; }
.mr-card .mr-icon img { width: 100%; height: 100%; object-fit: cover; }
.mr-card .mr-info { flex: 1; min-width: 0; }
.mr-card .mr-name { font-weight: 600; font-size: 13px; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
.mr-card .mr-desc { font-size: 11px; color: var(--dim); margin-top: 2px; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
.mr-card .mr-meta { font-size: 10px; color: rgba(106,106,138,.5); margin-top: 2px; }
.mr-card .mr-dl { font-size: 10px; color: var(--dim); white-space: nowrap; }
.mr-pagination { display: flex; align-items: center; justify-content: center; gap: 8px; margin-top: 10px; }
.mr-pagination button { padding: 4px 12px; font-size: 11px; border-radius: 6px; border: 1px solid var(--border); background: transparent; color: var(--dim); cursor: pointer; font-family: var(--font); }
.mr-pagination button:hover { border-color: var(--accent2); color: var(--accent2); }
.mr-pagination button:disabled { opacity: .3; cursor: default; }
.mr-pagination span { font-size: 11px; color: var(--dim); }

/* ── Version picker modal ── */
.mr-version-list { max-height: 280px; overflow-y: auto; }
.mr-version-item { display: flex; align-items: center; justify-content: space-between; padding: 8px 12px; border: 1px solid var(--border); border-radius: 6px; margin-bottom: 4px; transition: .12s; }
.mr-version-item:hover { border-color: var(--accent2); }
.mr-version-item .mr-ver-name { font-size: 13px; font-weight: 600; }
.mr-version-item .mr-ver-meta { font-size: 10px; color: var(--dim); }
.mr-version-item .mr-ver-dl { padding: 4px 12px; font-size: 11px; border-radius: 5px; border: 1px solid var(--green); background: transparent; color: var(--green); cursor: pointer; font-family: var(--font); transition: .15s; }
.mr-version-item .mr-ver-dl:hover { background: rgba(0,210,160,.12); }
.mr-version-item .mr-ver-dl:disabled { opacity: .4; cursor: wait; }

</style>
</head>
<body>

<header>
  <div class="logo" id="logoHome">MC Server Manager</div>
  <div class="header-stats">
    <span id="hdrServersLabel">Servers:</span> <span id="totalCount">0</span>
    &nbsp;|&nbsp; <span id="hdrRunningLabel">Running:</span> <span class="running" id="runningCount">0</span>
  </div>
  <div class="dl-indicator" id="dlIndicator">
    <span id="dlText">Downloading...</span>
    <div class="dl-bar"><div class="dl-bar-fill" id="dlBarFill" style="width:0%"></div></div>
  </div>
</header>

<main>
  <!-- Sidebar -->
  <aside class="sidebar">
    <div class="sidebar-nav">
      <button class="nav-btn active" id="navServers" onclick="navTo('servers')" data-i18n="webui.servers">Servers</button>
      <button class="nav-btn" id="navSettings" onclick="navTo('settings')" data-i18n="webui.settings">Settings</button>
    </div>
    <div class="server-list" id="serverList"></div>
    <div class="add-server-btn" id="btnCreateServer" data-i18n="webui.addServer">+ Add Server</div>
    <div class="sidebar-footer">
      <div class="ind" id="apiStatus"></div>
      <span id="apiText" data-i18n="webui.apiConnecting">API Connecting...</span>
    </div>
  </aside>

  <!-- Content Area -->
  <div class="content" id="contentArea">
    <!-- Tab Bar -->
    <div class="tab-bar" id="tabBar"></div>
    <!-- Tab Panels -->
    <div class="tab-panels" id="tabPanels"></div>
    <!-- Default empty state -->
    <div class="empty-state" id="emptyState">
      <div class="icon">&#x1F5A5;</div>
      <div class="title" data-i18n="webui.welcomeTitle">Welcome to MC Server Manager</div>
      <div class="sub" data-i18n="webui.welcomeSub">Select a server or create a new one</div>
      <button class="btn btn-primary" onclick="showWizard()" data-i18n="webui.createNewServer">+ Create New Server</button>
    </div>
  </div>
</main>

<!-- Edit Config Modal -->
<div class="modal-overlay" id="editModal">
  <div class="modal">
    <h3 data-i18n="webui.editConfig">&#x2699; Edit Server Configuration</h3>
    <div class="form-group"><label data-i18n="webui.serverName">Server Name</label><input type="text" id="editName"></div>
    <div class="form-group"><label data-i18n="webui.serverType">Server Type</label>
      <select id="editType"><option value="vanilla">Vanilla</option><option value="paper">Paper</option><option value="fabric">Fabric</option><option value="spigot">Spigot</option><option value="forge">Forge</option></select>
    </div>
    <div class="form-group"><label data-i18n="webui.minecraftVersion">Minecraft Version</label><input type="text" id="editVersion"></div>
    <div class="form-row">
      <div class="form-group"><label data-i18n="webui.minRam">Min RAM (MB)</label><select id="editMinRam"><option>512</option><option>1024</option><option>2048</option><option>4096</option><option>6144</option><option>8192</option><option>12288</option><option>16384</option></select></div>
      <div class="form-group"><label data-i18n="webui.maxRam">Max RAM (MB)</label><select id="editMaxRam"><option>1024</option><option>2048</option><option selected>4096</option><option>6144</option><option>8192</option><option>12288</option><option>16384</option></select></div>
    </div>
    <div class="form-group"><label data-i18n="webui.javaPath">Java Path</label><input type="text" id="editJavaPath" placeholder="java"></div>
    <div class="form-group"><label data-i18n="webui.jvmArgs">JVM Args</label><input type="text" id="editJavaArgs" placeholder="-XX:+UseG1GC"></div>
    <div class="actions">
      <button class="btn btn-ghost" onclick="closeEditModal()" data-i18n="webui.cancel">Cancel</button>
      <button class="btn btn-primary" onclick="saveEditConfig()" data-i18n="webui.save">Save</button>
    </div>
  </div>
</div>

<!-- Delete Confirm Modal -->
<div class="modal-overlay" id="deleteModal">
  <div class="modal">
    <h3 data-i18n="webui.confirmDelete">&#x26A0; Confirm Delete</h3>
    <p style="color:var(--dim);margin-bottom:8px" id="deleteMsg"></p>
    <div class="actions">
      <button class="btn btn-ghost" onclick="closeDeleteModal()" data-i18n="webui.cancel">Cancel</button>
      <button class="btn btn-red" onclick="confirmDelete()" data-i18n="webui.delete">Delete</button>
    </div>
  </div>
</div>

<!-- Cheat Sheet Modal -->
<div class="modal-overlay" id="cheatModal">
  <div class="modal cheat-modal">
    <div class="cheat-modal-hd">
      <span class="cm-title">&#x1F4D6; <span data-i18n="webui.cheatSheetTitle">Command Reference</span></span>
      <span class="cm-hint" data-i18n="webui.cheatSheetHint">Click to fill</span>
    </div>
    <div class="cm-search">
      <span class="cm-search-icon">&#x1F50D;</span>
      <input id="cheatSearch" type="text" placeholder="Search commands..." data-i18n-ph="webui.cheatSearchPlaceholder">
    </div>
    <div class="cm-cats" id="cheatCats">
      <button class="cm-cat active" data-cat="all" data-i18n="webui.cheatCatAll">All</button>
      <button class="cm-cat" data-cat="player" data-i18n="webui.cheatCatPlayer">Player</button>
      <button class="cm-cat" data-cat="world" data-i18n="webui.cheatCatWorld">World</button>
      <button class="cm-cat" data-cat="server" data-i18n="webui.cheatCatServer">Server</button>
      <button class="cm-cat" data-cat="admin" data-i18n="webui.cheatCatAdmin">Admin</button>
    </div>
    <div class="cm-list" id="cheatList"></div>
    <div class="actions" style="padding:8px 16px 16px;flex-shrink:0;border-top:1px solid var(--border)">
      <button class="btn btn-ghost btn-sm" onclick="closeCheatSheet()" data-i18n="webui.close">Close</button>
    </div>
  </div>
</div>

<!-- Server Properties Modal -->
<div class="modal-overlay" id="propsModal">
  <div class="modal props-modal">
    <h3>&#x2699; <span id="propsTitleLabel" data-i18n="webui.serverProperties">Server Properties</span> — <span id="propsServerName"></span></h3>
    <div class="props-toggle" style="margin-bottom:8px">
      <button id="propsToggleAdvanced" onclick="toggleAdvancedProps()" data-i18n="webui.showAdvanced">Show Advanced Settings</button>
    </div>
    <div class="props-form" id="propsForm">
      <div style="text-align:center;color:var(--dim);padding:30px;grid-column:1/-1" data-i18n="webui.loadingProperties">Loading properties...</div>
    </div>
    <div class="props-save-row">
      <span id="propsErr" style="color:var(--red);font-size:12px;margin-right:12px"></span>
      <button class="btn btn-ghost" onclick="closePropsModal()" data-i18n="webui.cancel">Cancel</button>
      <button class="btn btn-primary" id="btnSaveProps" onclick="saveProperties()" data-i18n="webui.propsSaveBtn">Save Properties</button>
    </div>
    <div style="font-size:10px;color:var(--dim);margin-top:8px" data-i18n="webui.propsRestartNote">Changes to server.properties only take effect after server restart.</div>
  </div>
</div>

<!-- Mod Management Modal -->
<div class="modal-overlay" id="modModal">
  <div class="modal">
    <h3 style="display:flex;justify-content:space-between;align-items:center">
      <span>&#x1F4E6; <span data-i18n="webui.mods">Mods</span> — <span id="modServerName"></span></span>
      <span style="display:flex;gap:6px">
        <button class="btn btn-ghost btn-sm" id="btnScanMods" onclick="scanMods()" data-i18n="webui.rescanMods">Rescan</button>
        <button class="btn btn-ghost btn-sm" onclick="closeModsModal()">&times;</button>
      </span>
    </h3>
    <div class="mod-tabs">
      <button class="mod-tab active" id="modTabInstalled" onclick="switchModTab('installed')" data-i18n="webui.installedMods">Installed</button>
      <button class="mod-tab" id="modTabMarket" onclick="switchModTab('market')" data-i18n="webui.marketMods">Market</button>
    </div>
    <div id="modMarketPanel" style="display:none">
      <div class="modrinth-search">
        <input type="text" id="mrSearchInput" placeholder="Search Modrinth..." data-i18n-ph="webui.searchModrinth" onkeydown="if(event.key==='Enter')searchModrinth(0)">
        <select class="mr-loader-select" id="mrLoaderSelect" onchange="searchModrinth(0)">
          <option value="" data-i18n="webui.all">All</option>
          <option value="fabric">Fabric</option>
          <option value="forge">Forge</option>
          <option value="quilt">Quilt</option>
          <option value="neoforge">NeoForge</option>
        </select>
        <button class="btn btn-primary btn-sm" onclick="searchModrinth(0)" data-i18n="webui.search">Search</button>
      </div>
      <div class="modrinth-results" id="mrResults"></div>
      <div class="mr-pagination" id="mrPagination" style="display:none"></div>
    </div>
    <div id="modInstalledPanel">
      <div id="modConflicts"></div>
      <div class="mod-list" id="modList"></div>
      <div id="modEmpty" class="mod-empty" style="display:none" data-i18n="webui.noMods">No mods installed</div>
    </div>
    <div id="modContent" style="text-align:center;color:var(--dim);padding:30px" data-i18n="webui.loadingMods">Loading mods...</div>
  </div>
</div>

<!-- Player / World Management Modal -->
<div class="modal-overlay" id="pwModal">
  <div class="modal">
    <h3 style="display:flex;justify-content:space-between;align-items:center">
      <span>&#x1F465; <span data-i18n="webui.playerWorld">Player & World</span> — <span id="pwServerName"></span></span>
      <button class="btn btn-ghost btn-sm" onclick="closePWModal()">&times;</button>
    </h3>
    <div class="pw-tabs">
      <button class="pw-tab active" id="pwTabPlayers" onclick="switchPWTab('players')" data-i18n="webui.playersTab">Players</button>
      <button class="pw-tab" id="pwTabWorlds" onclick="switchPWTab('worlds')" data-i18n="webui.worldsTab">Worlds</button>
    </div>
    <div id="pwPlayersPanel">
      <div class="pw-empty" id="pwPlayerEmpty" style="display:none" data-i18n="webui.noPlayers">No online players</div>
      <div style="max-height:260px;overflow-y:auto"><table class="pw-table"><thead><tr>
        <th data-i18n="webui.playerName">Player</th><th data-i18n="webui.world">World</th><th data-i18n="webui.health">HP</th><th data-i18n="webui.actions">Actions</th>
      </tr></thead><tbody id="pwPlayerList"></tbody></table></div>
    </div>
    <div id="pwWorldsPanel" style="display:none">
      <div class="pw-empty" id="pwWorldEmpty" style="display:none" data-i18n="webui.noWorlds">No worlds found</div>
      <div style="max-height:260px;overflow-y:auto"><table class="pw-table"><thead><tr>
        <th data-i18n="webui.worldName">World</th><th data-i18n="webui.size">Size</th><th data-i18n="webui.actions">Actions</th>
      </tr></thead><tbody id="pwWorldList"></tbody></table></div>
    </div>
    <div id="pwLoading" style="text-align:center;color:var(--dim);padding:30px" data-i18n="webui.loading">Loading...</div>
  </div>
</div>

<!-- Modpack Install Modal -->
<div class="modal-overlay" id="mpModal">
  <div class="modal">
    <h3 style="display:flex;justify-content:space-between;align-items:center">
      <span>&#x1F4E6; <span data-i18n="webui.installModpack">Install Modpack</span></span>
      <button class="btn btn-ghost btn-sm" onclick="closeMPModal()">&times;</button>
    </h3>
    <div class="mp-upload-zone" id="mpUploadZone" onclick="document.getElementById('mpFileInput').click()">
      <div class="mp-upload-icon">&#x1F4C1;</div>
      <div class="mp-upload-text" data-i18n="webui.clickOrDrop">Click or drag .zip here</div>
      <div class="mp-upload-hint" data-i18n="webui.modpackHint">Supports .zip modpacks with mods/ folder or manifest.json</div>
      <input type="file" id="mpFileInput" accept=".zip" onchange="onMPFilePicked(this.files[0])">
    </div>
    <div id="mpAnalyzing" style="text-align:center;color:var(--dim);padding:20px;display:none" data-i18n="webui.analyzing">Analyzing modpack...</div>
    <div id="mpAnalysis" class="mp-analysis" style="display:none">
      <div class="mp-row"><span class="mp-key" data-i18n="webui.wizardName">Name</span><span class="mp-val" id="mpName"></span></div>
      <div class="mp-row"><span class="mp-key" data-i18n="webui.wizardVersion">Version</span><span class="mp-val" id="mpVersion"></span></div>
      <div class="mp-row"><span class="mp-key">Loader</span><span class="mp-val" id="mpLoader"></span></div>
      <div class="mp-row"><span class="mp-key" data-i18n="webui.modCount">Mods</span><span class="mp-val" id="mpModCount"></span></div>
    </div>
    <div id="mpInstallForm" style="display:none">
      <div class="form-group"><label data-i18n="webui.serverName">Server Name</label><input type="text" id="mpServerName"></div>
      <div class="form-group"><label data-i18n="wizard.serverPath" data-i18n="webui.targetPath">Target Directory</label><input type="text" id="mpTargetPath" placeholder="C:/mc/modpack-server"></div>
      <div class="form-row">
        <div class="form-group"><label data-i18n="webui.minRam">Min RAM</label><select id="mpMinRam"><option>512</option><option selected>1024</option><option>2048</option><option>4096</option><option>6144</option><option>8192</option></select></div>
        <div class="form-group"><label data-i18n="webui.maxRam">Max RAM</label><select id="mpMaxRam"><option>1024</option><option>2048</option><option selected>4096</option><option>6144</option><option>8192</option><option>12288</option></select></div>
      </div>
      <div class="actions" style="margin-top:14px">
        <button class="btn btn-ghost" onclick="closeMPModal()" data-i18n="webui.cancel">Cancel</button>
        <button class="btn btn-primary" id="btnInstallMP" onclick="installModpack()" data-i18n="webui.install">Install</button>
      </div>
    </div>
    <div id="mpError" style="color:var(--red);font-size:12px;margin-top:8px;text-align:center"></div>
    <div id="mpStatus" style="color:var(--green);font-size:12px;margin-top:8px;text-align:center"></div>
  </div>
</div>

<!-- Modrinth Version Picker Modal -->
<div class="modal-overlay" id="mrVersionModal">
  <div class="modal" style="max-width:520px">
    <h3 style="display:flex;justify-content:space-between;align-items:center">
      <span>&#x2B07; <span data-i18n="webui.selectVersion">Select Version</span> — <span id="mrVersionModName"></span></span>
      <button class="btn btn-ghost btn-sm" onclick="closeMRVersionModal()">&times;</button>
    </h3>
    <div class="mr-version-list" id="mrVersionList"></div>
    <div id="mrVersionLoading" style="text-align:center;color:var(--dim);padding:30px" data-i18n="webui.loading">Loading versions...</div>
  </div>
</div>

<script>
// ═══════════════ STATE ═══════════════
const API = window.location.origin + '/api';
var TOKEN = "__TOKEN__";
if (TOKEN.startsWith("__")) TOKEN = new URLSearchParams(window.location.search).get('token') || '';
var USER_LANG = "__LANG__";
if (USER_LANG.startsWith("__")) USER_LANG = 'en';
// Override with localStorage preference (WebUI can have its own language)
var savedLang = localStorage.getItem('mcsm_webui_lang');
if(savedLang && (savedLang==='en'||savedLang==='zh')) USER_LANG = savedLang;
var I18N = __TRANSLATIONS__;
function t(key){ return (I18N && I18N[key]) ? I18N[key] : key; }
function tf(key){ var v = t(key); return v ? v : key; }
// Apply translations to all elements with data-i18n attribute
function applyI18n(){
  var els = document.querySelectorAll('[data-i18n]');
  els.forEach(function(el){ el.textContent = t(el.getAttribute('data-i18n')); });
  var phs = document.querySelectorAll('[data-i18n-ph]');
  phs.forEach(function(el){ el.placeholder = t(el.getAttribute('data-i18n-ph')); });
}
applyI18n();
let servers = [], sidebarView = 'servers', view = 'welcome';
let pollTimer = null;
let editServerId = null, pendingDeleteId = null, wizardStep = 0, wizardData = {};

// Player/World state
let g_pwServerId = null, g_pwTab = 'players';

// Modpack state
let g_mpFileData = null, g_mpAnalysis = null;

// Modrinth search state
let g_mrOffset = 0, g_mrTotal = 0, g_mrQuery = '', g_mrLoader = '';
let g_mrProjectId = null, g_mrServerId = null;

// Tab state
let tabs = [];          // [{id: serverId, name: serverName}]
let activeTabId = null; // current active server id
let tabTimers = {};     // serverId -> {consolePoll, uptimeTimer, consoleLineCount, uptimeSecs, uptimeRunning}

const TYPE_COLORS = {vanilla:'var(--yellow)',paper:'var(--blue)',fabric:'var(--cyan)',spigot:'var(--orange)',forge:'var(--purple)'};

// ── Command Reference ──
const CMD_LIST = [
  {cmd:'help', desc:'Show help', descZh:'显示帮助', cat:'player'},
  {cmd:'list', desc:'List online players', descZh:'列出在线玩家', cat:'player'},
  {cmd:'op <player>', desc:'Grant operator', descZh:'授予管理员', cat:'player'},
  {cmd:'deop <player>', desc:'Revoke operator', descZh:'撤销管理员', cat:'player'},
  {cmd:'kick <player>', desc:'Kick player', descZh:'踢出玩家', cat:'player'},
  {cmd:'ban <player>', desc:'Ban player', descZh:'封禁玩家', cat:'player'},
  {cmd:'pardon <player>', desc:'Unban player', descZh:'解封玩家', cat:'player'},
  {cmd:'ban-ip <addr>', desc:'IP ban', descZh:'IP 封禁', cat:'player'},
  {cmd:'pardon-ip <addr>', desc:'IP unban', descZh:'IP 解封', cat:'player'},
  {cmd:'whitelist add <player>', desc:'Add whitelist', descZh:'添加白名单', cat:'player'},
  {cmd:'gamemode <mode> <player>', desc:'Change gamemode', descZh:'切换游戏模式', cat:'player'},
  {cmd:'tp <from> <to>', desc:'Teleport player', descZh:'传送玩家', cat:'player'},
  {cmd:'give <player> <item>', desc:'Give item', descZh:'给予物品', cat:'player'},
  {cmd:'kill <player>', desc:'Kill entity', descZh:'杀死实体', cat:'player'},
  {cmd:'msg <player> <msg>', desc:'Private message', descZh:'私聊消息', cat:'player'},
  {cmd:'xp add <player> <n>', desc:'Add XP', descZh:'增加经验', cat:'player'},
  {cmd:'effect give @p <id>', desc:'Give effect', descZh:'给予效果', cat:'player'},
  {cmd:'enchant @p <id> <lvl>', desc:'Enchant item', descZh:'附魔物品', cat:'player'},
  {cmd:'time set day', desc:'Set daytime', descZh:'设为白天', cat:'world'},
  {cmd:'time set night', desc:'Set night', descZh:'设为夜晚', cat:'world'},
  {cmd:'time set noon', desc:'Set noon', descZh:'设为正午', cat:'world'},
  {cmd:'weather clear', desc:'Clear weather', descZh:'晴天', cat:'world'},
  {cmd:'weather rain', desc:'Set rain', descZh:'下雨', cat:'world'},
  {cmd:'weather thunder', desc:'Set thunder', descZh:'雷雨', cat:'world'},
  {cmd:'gamerule <rule> <val>', desc:'Modify gamerule', descZh:'修改游戏规则', cat:'world'},
  {cmd:'seed', desc:'Show world seed', descZh:'显示种子', cat:'world'},
  {cmd:'difficulty <level>', desc:'Set difficulty', descZh:'设置难度', cat:'world'},
  {cmd:'defaultgamemode <m>', desc:'Default gamemode', descZh:'默认游戏模式', cat:'world'},
  {cmd:'spawnpoint <player>', desc:'Set spawn point', descZh:'设置出生点', cat:'world'},
  {cmd:'worldborder <...>', desc:'Manage border', descZh:'管理边界', cat:'world'},
  {cmd:'fill <from> <to> <block>', desc:'Fill region', descZh:'填充区域', cat:'world'},
  {cmd:'setblock <x> <y> <z> <b>', desc:'Place block', descZh:'放置方块', cat:'world'},
  {cmd:'summon <entity>', desc:'Summon entity', descZh:'召唤实体', cat:'world'},
  {cmd:'save-all', desc:'Save all worlds', descZh:'保存所有世界', cat:'server'},
  {cmd:'save-off', desc:'Disable autosave', descZh:'关闭自动保存', cat:'server'},
  {cmd:'save-on', desc:'Enable autosave', descZh:'开启自动保存', cat:'server'},
  {cmd:'stop', desc:'Stop server', descZh:'停止服务器', cat:'server'},
  {cmd:'reload', desc:'Reload datapacks', descZh:'重载数据包', cat:'server'},
  {cmd:'say <msg>', desc:'Broadcast message', descZh:'广播消息', cat:'server'},
  {cmd:'scoreboard <...>', desc:'Manage scoreboard', descZh:'管理计分板', cat:'server'},
  {cmd:'banlist', desc:'Show ban list', descZh:'显示封禁列表', cat:'server'},
  {cmd:'advancement <...>', desc:'Manage advancements', descZh:'管理进度', cat:'server'}
];

const QUICK_CMDS = [
  {cmd:'time set day', icon:'\u2600', label:'time set day', labelZh:'设为白天'},
  {cmd:'weather clear', icon:'\u2601', label:'weather clear', labelZh:'晴天'},
  {cmd:'save-all', icon:'\uD83D\uDCBE', label:'save-all', labelZh:'保存'},
  {cmd:'list', icon:'\uD83D\uDCCB', label:'list', labelZh:'在线列表'},
  {cmd:'gamemode creative', icon:'\uD83D\uDCA1', label:'gamemode creative', labelZh:'创造模式'},
  {cmd:'say ', icon:'\uD83D\uDCE2', label:'say', labelZh:'广播'},
  {cmd:'difficulty peaceful', icon:'\uD83D\uDEE1', label:'difficulty peaceful', labelZh:'和平难度'},
  {cmd:'gamerule keepInventory true', icon:'\uD83D\uDC9A', label:'keepInventory', labelZh:'不掉落'}
];

let cheatActiveCat = 'all';

// ── Get translated cmd description ──
function cmdDesc(c){ return (USER_LANG==='zh' && c.descZh) ? c.descZh : c.desc; }

function authHeaders() { const h = {}; if (TOKEN) h['Authorization'] = 'Bearer ' + TOKEN; return h; }
const FETCH_TIMEOUT = 8000;
async function apiFetch(path, opts) {
  var ctrl = new AbortController(); var t = setTimeout(function(){ctrl.abort();}, FETCH_TIMEOUT);
  try { var r = await fetch(API+path, Object.assign({signal: ctrl.signal}, opts)); clearTimeout(t); return r.json(); }
  catch(e) { clearTimeout(t); throw e; }
}
async function apiGet(path) { return apiFetch(path, {headers:authHeaders()}); }
async function apiPost(path,body,method='POST') {
  return apiFetch(path, {method, headers:{'Content-Type':'application/json',...authHeaders()}, body:JSON.stringify(body)});
}

function notify(msg,type='ok'){
  const n=document.createElement('div'); n.className='notify '+type; n.textContent=msg;
  document.body.appendChild(n);
  setTimeout(function(){n.style.opacity='0';n.style.transition='.3s';setTimeout(function(){n.remove();},300);},2500);
}
function esc(s){const d=document.createElement('div');d.textContent=s;return d.innerHTML;}
function badgeHtml(t){return '<span class="sc-badge badge-'+t+'">'+t+'</span>';}

// ═══════════════ NAVIGATION ═══════════════
function navTo(page) {
  if (sidebarView === page) return;
  sidebarView = page;
  document.getElementById('navServers').className = 'nav-btn' + (page==='servers'?' active':'');
  document.getElementById('navSettings').className = 'nav-btn' + (page==='settings'?' active':'');
  document.getElementById('serverList').style.display = page==='servers'?'block':'none';
  document.getElementById('btnCreateServer').style.display = page==='servers'?'block':'none';
  if (page==='servers') { renderList(); showWelcome(); }
  else { showSettings(); }
}
document.getElementById('logoHome').onclick = function(){navTo('servers');showWelcome();};

// ═══════════════ SIDEBAR: Server List ═══════════════
function renderList(){
  const list = document.getElementById('serverList');
  if (sidebarView !== 'servers') return;
  if(servers.length===0){
    list.innerHTML = '<div class="sidebar-empty">'+t('webui.noServers')+'</div>';
  } else {
    list.innerHTML = servers.map(function(s){
      var st = s.state||'stopped';
      var dotCls = 'sc-dot '+(st==='running'?'online':'offline');
      var isActive = s.id===activeTabId && view==='detail';
      var isSelected = s.id===activeTabId;
      return '<div class="server-card'+(isActive?' active':'')+(isSelected&&!isActive?' selected':'')+'" id="card_'+s.id+'">'
        +'<div class="sc-top">'
        +'<div class="'+dotCls+'"></div>'
        +'<div class="sc-name" title="'+esc(s.name)+'">'+esc(s.name)+'</div>'
        +badgeHtml(s.type||'vanilla')
        +'</div>'
        +'<div class="sc-info"><span>v'+esc(s.version||'?')+'</span><span>'+ (s.minRamMB||1024)+'M-'+(s.maxRamMB||4096)+'M</span></div>'
        +'<div class="sc-path" title="'+esc(s.path||'')+'">'+esc(s.path||'')+'</div>'
        +'<div class="sc-actions">'
        +(st==='running'
          ?'<button class="sc-btn sc-btn-stop" onclick="event.stopPropagation();doCardAction(\''+s.id+'\',\'stop\')">&#x25A0; '+t('webui.stop')+'</button>'
          :'<button class="sc-btn sc-btn-start" onclick="event.stopPropagation();doCardAction(\''+s.id+'\',\'start\')">&#x25B6; '+t('webui.start')+'</button>')
        +'<button class="sc-btn-edit" title="'+t('webui.edit')+'" onclick="event.stopPropagation();openEditModal(\''+s.id+'\')">&#x2699;</button>'
        +'<button class="sc-btn-go" title="Open detail" onclick="event.stopPropagation();openTab(\''+s.id+'\')">&#x2192;</button>'
        +'</div></div>';
    }).join('');
    list.querySelectorAll('.server-card').forEach(function(el){
      var sid = el.id.replace('card_','');
      el.onclick = function(e){ if(!e.target.closest('button')) openTab(sid); };
    });
  }
  document.getElementById('totalCount').textContent = servers.length;
  document.getElementById('runningCount').textContent = servers.filter(function(s){return s.state==='running'}).length;
  document.getElementById('btnCreateServer').onclick = showWizard;
}

async function doCardAction(id, action) {
  var r = await apiPost('/servers/'+id+'/'+action,{});
  if (r.success) notify(action==='stop'?t('webui.serverStopping'):t('webui.serverStarting'));
  else notify(r.message||'Failed','err');
  setTimeout(refreshAll, 1500);
}

// ═══════════════ TAB SYSTEM ═══════════════
function getTabIndex(id){
  for(var i=0;i<tabs.length;i++){ if(tabs[i].id===id) return i; }
  return -1;
}

function openTab(id){
  var s = servers.find(function(x){return x.id===id;});
  if(!s) return;

  var idx = getTabIndex(id);
  if(idx >= 0){
    // Tab already exists — just switch to it
    switchTab(id);
    return;
  }

  // Create new tab
  tabs.push({id: id, name: s.name});
  tabTimers[id] = {consolePoll: null, uptimeTimer: null, consoleLineCount: 0, uptimeSecs: 0, uptimeRunning: false};

  // Build panel DOM
  buildTabPanel(id, s);

  // Switch to this tab
  switchTab(id);
  startTimerForTab(id);
}

function closeTab(id){
  var idx = getTabIndex(id);
  if(idx < 0) return;

  // Clean up timers
  clearTimerForTab(id);
  delete tabTimers[id];

  // Remove panel
  var panel = document.getElementById('panel_'+id);
  if(panel) panel.remove();

  // Remove from tabs array
  tabs.splice(idx, 1);

  // Switch to adjacent tab or show welcome
  if(activeTabId === id){
    if(tabs.length > 0){
      var nextIdx = Math.min(idx, tabs.length - 1);
      switchTab(tabs[nextIdx].id);
    } else {
      activeTabId = null;
      view = 'welcome';
      showWelcomeView();
    }
  }

  renderTabBar();
  renderList();
}

function switchTab(id){
  // Stop current tab timers
  if(activeTabId && activeTabId !== id){
    clearTimerForTab(activeTabId);
  }

  var prevActive = activeTabId;
  activeTabId = id;

  // Update panels visibility
  var allPanels = document.querySelectorAll('.tab-panel');
  allPanels.forEach(function(p){ p.classList.remove('active'); });
  var panel = document.getElementById('panel_'+id);
  if(panel) panel.classList.add('active');

  // Show tab bar and panels
  document.getElementById('tabBar').classList.add('show');
  document.getElementById('tabPanels').classList.add('show');
  document.getElementById('emptyState').style.display = 'none';

  // Start timers for new tab
  if(prevActive !== id){
    startTimerForTab(id);
  }

  view = 'detail';
  renderTabBar();
  renderList();
}

function renderTabBar(){
  var bar = document.getElementById('tabBar');
  if(tabs.length === 0){
    bar.classList.remove('show');
    bar.innerHTML = '';
    return;
  }
  bar.classList.add('show');
  bar.innerHTML = tabs.map(function(t){
    var isActive = (t.id === activeTabId);
    return '<div class="tab-item'+(isActive?' active':'')+'" onclick="switchTab(\''+t.id+'\')" title="'+esc(t.name)+'">'
      +'<span class="tab-name">'+esc(t.name)+'</span>'
      +'<span class="tab-close" onclick="event.stopPropagation();closeTab(\''+t.id+'\')" title="Close">&#x2715;</span>'
      +'</div>';
  }).join('');
}

function clearTimerForTab(id){
  var tt = tabTimers[id];
  if(!tt) return;
  if(tt.consolePoll){clearInterval(tt.consolePoll);tt.consolePoll=null;}
  if(tt.uptimeTimer){clearInterval(tt.uptimeTimer);tt.uptimeTimer=null;}
  tt.uptimeRunning = false;
}

function startTimerForTab(id){
  var tt = tabTimers[id];
  if(!tt) return;
  var s = servers.find(function(x){return x.id===id;});
  if(!s) return;

  // Start console poll
  if(!tt.consolePoll){
    tt.consoleLineCount = 0;
    pollTabConsole(id);
    tt.consolePoll = setInterval(function(){pollTabConsole(id);}, 1500);
  }

  // Start uptime tick if running
  if(s.state === 'running' && !tt.uptimeTimer && !tt.uptimeRunning){
    tt.uptimeRunning = true;
    tt.uptimeTimer = setInterval(function(){tickTabUptime(id);}, 2000);
  }
}

// ═══════════════ VIEW MANAGEMENT ═══════════════
function showWelcome(){
  view = 'welcome';
  // Close all tabs
  while(tabs.length > 0){ closeTabSilent(tabs[0].id); }
  activeTabId = null;
  showWelcomeView();
  renderList();
}

function closeTabSilent(id){
  // Same as closeTab but without switching (used when closing all tabs)
  var idx = getTabIndex(id);
  if(idx < 0) return;
  clearTimerForTab(id);
  delete tabTimers[id];
  var panel = document.getElementById('panel_'+id);
  if(panel) panel.remove();
  tabs.splice(idx, 1);
}

function showWelcomeView(){
  view = 'welcome';
  activeTabId = null;
  document.getElementById('tabBar').classList.remove('show');
  document.getElementById('tabPanels').classList.remove('show');
  document.getElementById('tabPanels').innerHTML = '';
  document.getElementById('emptyState').style.display = 'flex';
  // Hide Settings / Wizard wrappers so they don't bleed through
  var sw = document.getElementById('settingsWrapper'); if(sw) sw.style.display = 'none';
  var ww = document.getElementById('wizardWrapper'); if(ww) ww.style.display = 'none';
  renderList();
}

function showView(name){
  view = name;
  // Only clear tab timers if we're navigating away from detail view entirely
  if(name !== 'detail'){
    tabs.forEach(function(t){ clearTimerForTab(t.id); });
    activeTabId = null;
  }
}

// ═══════════════ TAB PANEL BUILDING ═══════════════
// ── Cheat Sheet ──
function openCheatSheet(){
  document.getElementById('cheatSearch').value = '';
  cheatActiveCat = 'all';
  updateCheatCats();
  renderCheatList();
  document.getElementById('cheatModal').classList.add('show');
  document.getElementById('cheatSearch').focus();
}
function closeCheatSheet(){
  document.getElementById('cheatModal').classList.remove('show');
}
function updateCheatCats(){
  var cats = document.getElementById('cheatCats').querySelectorAll('.cm-cat');
  cats.forEach(function(c){ c.classList.toggle('active', c.dataset.cat === cheatActiveCat); });
}
function renderCheatList(){
  var q = (document.getElementById('cheatSearch').value||'').toLowerCase();
  var list = CMD_LIST.filter(function(c){
    if(cheatActiveCat !== 'all' && c.cat !== cheatActiveCat) return false;
    if(q) return c.cmd.toLowerCase().indexOf(q) !== -1 || cmdDesc(c).toLowerCase().indexOf(q) !== -1;
    return true;
  });
  var el = document.getElementById('cheatList');
  if(list.length === 0){
    el.innerHTML = '<div class="cm-empty">'+(USER_LANG==='zh'?'没有匹配的命令':'No matching commands')+'</div>';
  } else {
    el.innerHTML = list.map(function(c){
      return '<div class="cm-item" onclick="fillTabCommand(activeTabId,\''+c.cmd.replace(/'/g,"\\'")+'\');closeCheatSheet();">'
        +'<code>'+esc(c.cmd)+'</code><span class="cm-desc">\u2014 '+esc(cmdDesc(c))+'</span></div>';
    }).join('');
  }
}

// ── Fill Command ──
function fillTabCommand(id, cmd){
  var inp = document.getElementById('cmdInput_'+id);
  if(inp){ inp.value = cmd; inp.focus(); }
}

// ── Command Auto-Complete ──
function cmdAutoComplete(id, val){
  var suggest = document.getElementById('cmdSuggest_'+id);
  if(!suggest) return;
  var v = val.trim();
  if(!v){
    suggest.classList.remove('show'); suggest.innerHTML = ''; return;
  }
  var matches = CMD_LIST.filter(function(c){ return c.cmd.toLowerCase().indexOf(v.toLowerCase()) !== -1; });
  if(matches.length === 0 || (matches.length === 1 && matches[0].cmd.toLowerCase() === v.toLowerCase())){
    suggest.classList.remove('show'); suggest.innerHTML = ''; return;
  }
  suggest.innerHTML = matches.slice(0,8).map(function(c){
    return '<div class="cmd-suggest-item" onclick="fillTabCommand(\''+id+'\',\''+c.cmd.replace(/'/g,"\\'")+'\');document.getElementById(\'cmdSuggest_'+id+'\').classList.remove(\'show\');">'
      +'<code>'+esc(c.cmd)+'</code><span class="sug-desc">'+esc(cmdDesc(c))+'</span></div>';
  }).join('');
  suggest.classList.add('show');
}

// ── build tab panel ──
async function buildTabPanel(id, s){
  var panels = document.getElementById('tabPanels');
  var panelEl = document.createElement('div');
  panelEl.className = 'tab-panel';
  panelEl.id = 'panel_'+id;
  panelEl.innerHTML =
    '<div class="detail">'
    +'<div class="detail-top">'
    +'<div style="width:10px;height:10px;border-radius:50%;'+(s.state==='running'?'background:var(--green);box-shadow:0 0 6px var(--green)':'background:#444')+'"></div>'
    +'<div class="sname">'+esc(s.name)+'</div>'
    +'<div class="sstatus '+(s.state==='running'?'online':'offline')+'">'+s.state.toUpperCase()+'</div>'
    +badgeHtml(s.type||'vanilla')
    +'<div class="detail-actions">'
    +(s.state==='running'
      ?'<button class="btn btn-red btn-sm" id="btnStop_'+id+'">&#x25A0; '+t('webui.stop')+'</button>'
       +'<button class="btn btn-orange btn-sm" id="btnKill_'+id+'">'+t('webui.forceKill')+'</button>'
      :'<button class="btn btn-green btn-sm" id="btnStart_'+id+'">&#x25B6; '+t('webui.start')+'</button>')
    +'<button class="btn btn-ghost btn-sm" id="btnEdit_'+id+'">&#x2699; '+t('webui.edit')+'</button>'
    +'<button class="btn btn-ghost btn-sm" id="btnDelete_'+id+'" style="color:var(--red);border-color:rgba(255,107,107,.3)">'+t('webui.delete')+'</button>'
    +'<button class="btn btn-ghost btn-sm" id="btnRefresh_'+id+'">&#x21BB;</button>'
    +'<button class="btn btn-primary btn-sm" id="btnProps_'+id+'">'+t('webui.properties')+'</button>'
    +'<button class="btn btn-primary btn-sm" id="btnMods_'+id+'">&#x1F4E6; '+t('webui.mods')+'</button>'
    +'<button class="btn btn-primary btn-sm" id="btnPW_'+id+'">&#x1F465; '+t('webui.playerWorld')+'</button>'
    +'<button class="btn btn-primary btn-sm" id="btnMP_'+id+'">&#x1F4E6; '+t('webui.installModpack')+'</button>'
    +'</div></div>'

    +'<div class="stats-bar">'
    +'<div class="stat-card"><div class="label">'+t('webui.status')+'</div><div class="value '+(s.state==='running'?'green':'')+'" id="statStatus_'+id+'">'+(s.state==='running'?t('webui.running'):t('webui.stopped'))+'</div></div>'
    +'<div class="stat-card"><div class="label">'+t('webui.uptime')+'</div><div class="value" id="uptimeVal_'+id+'">'+(s.state==='running'?'--':'N/A')+'</div></div>'
    +'<div class="stat-card"><div class="label">'+t('webui.memory')+'</div><div class="value">'+(s.minRamMB||1024)+'M - '+(s.maxRamMB||4096)+'M</div></div>'
    +'<div class="stat-card"><div class="label">'+t('webui.java')+'</div><div class="value">'+esc(s.javaPath||'java')+'</div></div>'
    +'</div>'

    // Quick actions bar
    +'<div class="quick-actions" id="quickActions_'+id+'">'
    +'<span class="qa-label">'+t('webui.quickActions')+'</span>'
    + QUICK_CMDS.map(function(q,i){ return '<button class="qa-btn" id="qaBtn_'+id+'_'+i+'" '+(s.state!=='running'?'disabled':'')+' onclick="fillTabCommand(\''+id+'\',\''+q.cmd.replace(/'/g,"\\'")+'\')">'+q.icon+' '+esc(USER_LANG==='zh'&&q.labelZh?q.labelZh:q.label)+'</button>'; }).join('')
    +'</div>'

    +'<div class="console-wrap">'
    +'<div class="console-header"><span>&#x25B6; '+t('webui.consoleOutput')+'</span>'
    +'<div style="display:flex;align-items:center;gap:8px">'
    +'<button class="cheat-btn" id="cheatBtn_'+id+'" onclick="openCheatSheet()">&#x1F4D6; '+t('webui.cheatSheet')+'</button>'
    +'<span id="consoleLineCount_'+id+'" style="color:var(--dim)">0 '+t('webui.lines').replace('%1','0')+'</span>'
    +'</div></div>'
    +'<div class="console-out" id="consoleOut_'+id+'"><div style="color:var(--dim)">'+t('webui.waitingOutput')+'</div></div>'
    +'<div class="console-inp">'
    +'<span class="prompt">&gt;</span>'
    +'<div class="cmd-suggest-wrap">'
    +'<div class="cmd-suggest" id="cmdSuggest_'+id+'"></div>'
    +'<input id="cmdInput_'+id+'" type="text" placeholder="'+t('webui.command')+'" '+(s.state!=='running'?'disabled':'')+'>'
    +'</div>'
    +'<button class="btn btn-primary btn-sm" id="btnSendCmd_'+id+'" '+(s.state!=='running'?'disabled':'')+'>'+t('webui.send')+'</button>'
    +'</div></div></div>';

  panels.appendChild(panelEl);

  // Bind buttons
  var bind = function(elId, fn){
    var e = document.getElementById(elId);
    if(e) e.onclick = fn;
  };
  bind('btnStart_'+id, function(){doTabAction(id,'start');});
  bind('btnStop_'+id, function(){doTabAction(id,'stop');});
  bind('btnKill_'+id, function(){doTabAction(id,'kill');});
  bind('btnEdit_'+id, function(){openEditModal(id);});
  bind('btnDelete_'+id, function(){openDeleteModal(id, s.name);});
  bind('btnRefresh_'+id, function(){refreshAll();});
  bind('btnProps_'+id, function(){openProperties(id);});
  bind('btnMods_'+id, function(){openModsModal(id);});
  bind('btnPW_'+id, function(){openPWModal(id);});
  bind('btnMP_'+id, function(){openMPModal();});
  bind('btnSendCmd_'+id, function(){sendTabCommand(id);});
  var inp = document.getElementById('cmdInput_'+id);
  if(inp){
    inp.onkeydown = function(e){if(e.key==='Enter'){ document.getElementById('cmdSuggest_'+id).classList.remove('show'); sendTabCommand(id); }};
    inp.oninput = function(){ cmdAutoComplete(id, this.value); };
    inp.onblur = function(){ setTimeout(function(){ var s=document.getElementById('cmdSuggest_'+id); if(s) s.classList.remove('show'); }, 150); };
  }

  // Fetch initial uptime
  if(s.state === 'running'){
    try{
      var r = await apiGet('/servers/'+id+'/status');
      tabTimers[id].uptimeSecs = r.uptimeSecs || 0;
      var uv = document.getElementById('uptimeVal_'+id);
      if(uv) uv.textContent = formatUptime(tabTimers[id].uptimeSecs);
    }catch(e){}
  }
}

async function doTabAction(id, action){
  var r;
  if(action==='kill') r=await apiPost('/servers/'+id+'/kill',{});
  else r=await apiPost('/servers/'+id+'/'+action,{});
  if(r.success) notify(action==='kill'?t('webui.forceKilled'):t('webui.serverAction').replace('%1',action));
  else notify(r.message||'Failed','err');
  setTimeout(refreshAll, 1500);
}

async function sendTabCommand(id){
  var inp = document.getElementById('cmdInput_'+id);
  if(!inp||!inp.value.trim()) return;
  var r = await apiPost('/servers/'+id+'/command',{command:inp.value.trim()});
  inp.value=''; inp.focus();
}

// ═══════════════ TAB CONSOLE POLL ═══════════════
async function pollTabConsole(id){
  var tt = tabTimers[id];
  if(!tt) return;
  try {
    var r = await apiGet('/servers/'+id+'/console?limit=300');
    var out = document.getElementById('consoleOut_'+id);
    if(!out) return;
    if(!r.lines||r.lines.length===0) return;
    if(r.total !== tt.consoleLineCount){
      out.innerHTML = r.lines.map(function(l){
        var cls='';
        if(/(ERROR|Exception|FATAL)/i.test(l)) cls='error';
        else if(/WARN/i.test(l)) cls='warn';
        else if(/joined the game/i.test(l)||/logged in/i.test(l)) cls='join';
        else if(/left the game/i.test(l)||/lost connection/i.test(l)) cls='leave';
        else if(/\[INFO\]|\[Server\]|Done/i.test(l)) cls='info';
        return '<div class="line '+cls+'">'+esc(l)+'</div>';
      }).join('');
      out.scrollTop = out.scrollHeight;
      tt.consoleLineCount = r.total;
      var lc = document.getElementById('consoleLineCount_'+id);
      if(lc) lc.textContent = t('webui.lines').replace('%1',r.total);
    }
  }catch(e){}
}

// ═══════════════ TAB UPTIME TICK ═══════════════
async function tickTabUptime(id){
  var tt = tabTimers[id];
  if(!tt) return;
  var s = servers.find(function(x){return x.id===id;});
  if(!s||s.state!=='running') return;
  try {
    var r = await apiGet('/servers/'+id+'/status');
    if(r.uptimeSecs !== undefined){
      tt.uptimeSecs = r.uptimeSecs;
      var uv = document.getElementById('uptimeVal_'+id);
      if(uv) uv.textContent = formatUptime(tt.uptimeSecs);
    }
  }catch(e){}
}

function formatUptime(s){
  if(s<60)return s+'s';
  if(s<3600)return Math.floor(s/60)+'m '+s%60+'s';
  return Math.floor(s/3600)+'h '+Math.floor((s%3600)/60)+'m';
}

// ═══════════════ Settings Page ═══════════════
var g_settingsData = null; // cached settings for rendering form

async function showSettings(){
  showView('settings');
  var area = document.getElementById('contentArea');
  // Hide tab system
  document.getElementById('tabBar').classList.remove('show');
  document.getElementById('tabPanels').classList.remove('show');
  document.getElementById('emptyState').style.display = 'none';

  // Use a wrapper div
  var wrapper = document.getElementById('settingsWrapper') || (function(){
    var d = document.createElement('div'); d.id = 'settingsWrapper'; area.appendChild(d); return d;
  })();
  var wizardW = document.getElementById('wizardWrapper');
  if(wizardW) wizardW.style.display = 'none';
  wrapper.style.display = 'block';
  wrapper.style.flex = '1';
  wrapper.style.overflow = 'auto';

  wrapper.innerHTML = '<div class="settings-page"><h2>&#x2699; '+t('webui.settings')+'</h2>'
    +'<div style="text-align:center;color:var(--dim);padding:40px">'+t('webui.loadingSettings')+'</div></div>';

  try {
    g_settingsData = await apiGet('/settings');
    renderSettingsForm(wrapper);
  } catch(e) {
    wrapper.innerHTML = '<div class="settings-page"><h2>&#x2699; '+t('webui.settings')+'</h2>'
      +'<div style="text-align:center;color:var(--red);padding:40px">'+t('webui.failedToLoad')+'</div></div>';
  }
}

function renderSettingsForm(wrapper){
  var s = g_settingsData;
  wrapper.innerHTML = '<div class="settings-page">'
    +'<h2>&#x2699; '+t('webui.settings')+'</h2>'

    // ── WebUI Server ──
    +'<div class="settings-group">'
    +'<h3>'+t('webui.webApiTitle')+'</h3>'
    +'<div class="settings-row"><span class="skey">'+t('webui.status')+'</span>'
    +'<span class="sval green">'+t('webui.running')+'</span></div>'
    +'<div class="settings-row"><span class="skey">'+t('webui.portLabel')+'</span>'
    +'<input type="number" id="sWebPort" class="settings-input" value="'+esc(String(s.webApiPort||25575))+'" min="1024" max="65535"></div>'
    +'<div class="settings-row">'
    +'<span class="skey">'+t('webui.webuiUrl')+'</span>'
    +'<div class="token-row"><span class="settings-copy" id="settingsUrl">'+esc(s.webApiUrl||'http://localhost:'+(s.webApiPort||25575))+'</span>'
    +'<button class="btn btn-ghost btn-sm" onclick="copySettingsUrl()">'+t('webui.copy')+'</button></div></div>'
    +'<div class="settings-row">'
    +'<span class="skey">'+t('webui.apiToken')+'</span>'
    +'<div class="token-row"><span class="settings-copy" id="settingsToken">'+esc(s.webApiToken||'(empty)')+'</span>'
    +'<button class="btn btn-ghost btn-sm" onclick="copySettingsToken()">'+t('webui.copy')+'</button>'
    +'<button class="btn btn-primary btn-sm" onclick="regenerateToken()">'+t('webui.generate')+'</button></div></div>'
    +'<div class="settings-desc" id="settingsUrlPreview">'+t('webui.openInBrowser').replace('%1','<b>'+esc(s.webApiUrl||'http://localhost:'+(s.webApiPort||25575))+'</b>')+'</div>'
    +'</div>'

    // ── General ──
    +'<div class="settings-group">'
    +'<h3>'+t('webui.general')+'</h3>'
    +'<div class="check-row">'
    +'<input type="checkbox" id="sAutoStart" '+(s.autoStart?'checked':'')+'>'
    +'<label for="sAutoStart">'+t('webui.autoStart')+'</label></div>'
    +'</div>'

    // ── Language (WebUI only) ──
    +'<div class="settings-group">'
    +'<h3>'+t('webui.langLabel')+' / '+t('webui.settings')+'</h3>'
    +'<div class="form-group" style="margin-top:6px"><label>'+t('webui.langLabel')+'</label>'
    +'<select id="sWebUILang" style="background:var(--bg);border:1px solid var(--border);color:var(--text);border-radius:8px;padding:10px 14px;font-size:13px;width:100%">'
    +'<option value="en" '+(USER_LANG==='en'?'selected':'')+'>English</option>'
    +'<option value="zh" '+(USER_LANG==='zh'?'selected':'')+'>中文 (Chinese)</option>'
    +'</select></div>'
    +'<div class="settings-desc">'+t('webui.langDesc')+'</div>'
    +'</div>'

    // ── Bot Plugin ──
    +'<div class="settings-group">'
    +'<h3>'+t('webui.botIntegration')+'</h3>'
    +'<div class="check-row">'
    +'<input type="checkbox" id="sBotEnabled" '+(s.botEnabled?'checked':'')+'>'
    +'<label for="sBotEnabled">'+t('webui.enableBot')+'</label></div>'
    +'<div class="form-group" style="margin-top:10px"><label>'+t('webui.botPort')+'</label>'
    +'<input type="number" id="sBotPort" value="'+esc(String(s.botPort||25576))+'" min="1024" max="65535"></div>'
    +'<div class="settings-desc">'+t('webui.botDesc')+'</div>'
    +'</div>'

    // ── Save ──
    +'<div style="text-align:right;margin-top:8px">'
    +'<span id="settingsErr" style="color:var(--red);font-size:12px;margin-right:12px"></span>'
    +'<button class="btn btn-primary" id="btnSaveSettings" onclick="saveSettings()">'+t('webui.saveSettings')+'</button>'
    +'</div>'

    +'</div>';

  // update URL preview when port input changes
  var portInp = document.getElementById('sWebPort');
  if(portInp) portInp.oninput = function(){
    var p = this.value;
    var urlEl = document.getElementById('settingsUrl');
    if(urlEl) urlEl.textContent = 'http://localhost:' + p;
    var prev = document.getElementById('settingsUrlPreview');
    if(prev) prev.innerHTML = t('webui.openInBrowser').replace('%1','<b>http://localhost:'+p+'</b>');
  };
}

async function saveSettings(){
  var btn = document.getElementById('btnSaveSettings');
  var err = document.getElementById('settingsErr');
  err.textContent = '';

  var portVal = parseInt(document.getElementById('sWebPort').value) || g_settingsData.webApiPort;
  if(portVal < 1024 || portVal > 65535){
    err.textContent = t('webui.portRangeError');
    return;
  }

  var body = {
    webApiPort: portVal,
    autoStart: document.getElementById('sAutoStart').checked,
    botEnabled: document.getElementById('sBotEnabled').checked,
    botPort: parseInt(document.getElementById('sBotPort').value) || 25576
  };

  // Check language change for WebUI
  var selLang = document.getElementById('sWebUILang');
  var langChanged = selLang && selLang.value !== USER_LANG;
  if(langChanged){
    localStorage.setItem('mcsm_webui_lang', selLang.value);
    body.webUILang = selLang.value;
    // Reload to apply new language (after save)
  }

  btn.disabled = true;
  btn.textContent = t('webui.saving');

  try {
    var r = await apiPost('/settings', body, 'PUT');
    if(r.success){
      if(r.portChanged){
        notify(t('webui.portChanged').replace('%1',r.newPort));
        setTimeout(function(){
          window.location.href = r.newUrl + (TOKEN ? '?token=' + TOKEN : '');
        }, 800);
      } else if(langChanged){
        notify(t('webui.languageChanged'));
        setTimeout(function(){ location.reload(); }, 500);
      } else {
        notify(t('webui.settingsSaved'));
      }
      g_settingsData.webApiPort = portVal;
      btn.disabled = false;
      btn.textContent = t('webui.saveSettings');
    } else {
      err.textContent = r.message || t('webui.failedToSave');
      btn.disabled = false;
      btn.textContent = t('webui.saveSettings');
    }
  } catch(e){
    err.textContent = t('webui.networkError');
    btn.disabled = false;
    btn.textContent = t('webui.saveSettings');
  }
}

function copySettingsUrl(){
  var el = document.getElementById('settingsUrl');
  if(!el) return;
  navigator.clipboard.writeText(el.textContent).then(function(){notify(t('webui.urlCopied'));});
}
function copySettingsToken(){
  var el = document.getElementById('settingsToken');
  if(!el) return;
  navigator.clipboard.writeText(el.textContent).then(function(){notify(t('webui.tokenCopied'));});
}
async function regenerateToken(){
  try {
    var r = await apiPost('/settings',{regenerateToken:true},'PUT');
    if(r.success){notify(t('webui.tokenRegenerated'));setTimeout(function(){location.reload();},1500);}
    else notify(t('webui.failedToRegen'),'err');
  }catch(e){notify(t('webui.networkError'),'err');}
}

// ═══════════════ CREATE WIZARD ═══════════════
function showWizard(){
  showView('wizard');
  activeTabId = null;

  // Hide tab system
  document.getElementById('tabBar').classList.remove('show');
  document.getElementById('tabPanels').classList.remove('show');
  document.getElementById('emptyState').style.display = 'none';

  var wrapper = document.getElementById('wizardWrapper');
  if(!wrapper){
    wrapper = document.createElement('div'); wrapper.id = 'wizardWrapper';
    document.getElementById('contentArea').appendChild(wrapper);
  }
  var sw = document.getElementById('settingsWrapper');
  if(sw) sw.style.display = 'none';
  wrapper.style.display = 'block';
  wrapper.style.flex = '1';
  wrapper.style.overflow = 'auto';

  wizardStep=0; wizardData={type:'paper'};
  renderWizard();
}

function renderWizard(){
  var steps = [t('wizard.step1'),t('wizard.step2'),t('wizard.step3'),t('wizard.step4')];
  var wrapper = document.getElementById('wizardWrapper');
  wrapper.innerHTML = '<div class="wizard"><div class="wizard-steps">'
    + steps.map(function(s,i){
        var cls=''; if(i===wizardStep) cls='active'; else if(i<wizardStep) cls='done';
        return (i>0?'<div class="wizard-line '+(i<=wizardStep?'done':'')+'"></div>':'')
          + '<div class="wizard-step '+cls+'"><div class="num">'+(i<wizardStep?'&#x2713;':i+1)+'</div>'+s+'</div>';
      }).join('')
    + '</div><div id="wizardBody"></div>'
    + '<div class="wizard-nav"><div>'+(wizardStep>0?'<button class="btn btn-ghost" onclick="prevStep()">&#x2190; '+t('webui.wizardPrevious')+'</button>':'')+'</div>'
    + '<div><button class="btn btn-ghost" onclick="showWelcome()" style="margin-right:8px">'+t('webui.wizardCancel')+'</button>'
    + '<button class="btn btn-primary" id="btnNext" onclick="nextStep()">'+(wizardStep===3?'&#x1F389; '+t('wizard.create'):t('webui.wizardNext')+' &#x2192;')+'</button></div></div>'
    + '<div id="wizardErr" style="color:var(--red);font-size:12px;margin-top:6px;text-align:center"></div></div>';
  renderStep();
}

function prevStep(){ if(wizardStep>0){wizardStep--;renderWizard();} }
function nextStep(){
  document.getElementById('wizardErr').textContent='';
  var err = validateStep();
  if(err){ document.getElementById('wizardErr').textContent=err; return; }
  if(wizardStep===3){ doCreate(); return; }
  wizardStep++; renderWizard();
}

function validateStep(){
  if(wizardStep===0){ if(!wizardData.type) return t('webui.wizardSelectType'); }
  if(wizardStep===1){ if(!wizardData.version) return t('webui.wizardSelectVersion'); }
  if(wizardStep===2){
    if(!wizardData.name||!wizardData.name.trim()) return t('webui.wizardNameRequired');
    if(!wizardData.path||!wizardData.path.trim()) return t('webui.wizardPathRequired');
  }
  return null;
}

function renderStep(){
  var body = document.getElementById('wizardBody');
  if(wizardStep===0){
    var types = [
      {id:'vanilla',name:t('wizard.vanilla'),icon:'&#x1F310;',desc:t('wizard.vanillaDesc')},
      {id:'paper',name:t('wizard.paper'),icon:'&#x1F680;',desc:t('wizard.paperDesc')},
      {id:'fabric',name:t('wizard.fabric'),icon:'&#x1F9F5;',desc:t('wizard.fabricDesc')}
    ];
    body.innerHTML = '<div style="max-width:640px;margin:0 auto"><div class="type-cards">'
      + types.map(function(t){return '<div class="type-card '+(wizardData.type===t.id?'selected':'')
        +'" style="border-left-color:'+TYPE_COLORS[t.id]+'" onclick="selType(\''+t.id+'\')">'
        +'<div class="t-icon">'+t.icon+'</div>'
        +'<div class="t-name" style="color:'+TYPE_COLORS[t.id]+'">'+t.name+'</div>'
        +'<div class="t-desc">'+t.desc+'</div></div>';}).join('')
      +'</div><p style="color:var(--dim);font-size:11px;text-align:center;margin-top:10px">'
      + (wizardData.type==='paper'?t('webui.wizardPaperRec')
        :wizardData.type==='fabric'?t('webui.wizardFabricRec')
        :t('webui.wizardVanillaRec'))
      +'</p></div>';
  } else if(wizardStep===1){
    body.innerHTML = '<div style="max-width:460px;margin:0 auto">'
      +'<div class="form-group"><label>'+t('webui.minecraftVersion')+'</label><select id="verSelect" onchange="wizardData.version=this.value"><option>'+t('wizard.loading')+'</option></select></div>'
      +'<p style="color:var(--dim);font-size:11px;margin-top:14px">'+t('webui.wizardAutoDownload')+'</p></div>';
    loadVersions();
  } else if(wizardStep===2){
    wizardData.name = wizardData.name || '';
    wizardData.path = wizardData.path || '';
    wizardData.javaPath = wizardData.javaPath || 'java';
    wizardData.minRam = wizardData.minRam || 1024;
    wizardData.maxRam = wizardData.maxRam || 4096;
    body.innerHTML = '<div style="max-width:520px;margin:0 auto">'
      +'<div class="form-group"><label>'+t('webui.serverName')+'</label><input type="text" id="cfgName" value="'+esc(wizardData.name)+'" placeholder="My Survival Server" oninput="wizardData.name=this.value"></div>'
      +'<div class="form-group"><label>'+t('wizard.serverPath')+'</label><input type="text" id="cfgPath" value="'+esc(wizardData.path)+'" placeholder="C:/mc/survival" oninput="wizardData.path=this.value"></div>'
      +'<div class="form-row">'
      +'<div class="form-group"><label>'+t('webui.minRam')+'</label><select id="cfgMinRam" onchange="wizardData.minRam=parseInt(this.value)">'
      +[512,1024,2048,4096,6144,8192,12288,16384].map(function(v){return '<option value="'+v+'" '+(v===wizardData.minRam?'selected':'')+'>'+v+' MB</option>';}).join('')+'</select></div>'
      +'<div class="form-group"><label>'+t('webui.maxRam')+'</label><select id="cfgMaxRam" onchange="wizardData.maxRam=parseInt(this.value)">'
      +[1024,2048,4096,6144,8192,12288,16384].map(function(v){return '<option value="'+v+'" '+(v===wizardData.maxRam?'selected':'')+'>'+v+' MB</option>';}).join('')+'</select></div>'
      +'</div>'
      +'<div class="form-group"><label>'+t('webui.javaPath')+'</label><input type="text" id="cfgJavaPath" value="'+esc(wizardData.javaPath)+'" placeholder="java" oninput="wizardData.javaPath=this.value"></div>'
      +'<div class="form-group"><label>'+t('webui.jvmArgs')+'</label><input type="text" id="cfgJavaArgs" value="'+esc(wizardData.javaArgs||'')+'" placeholder="-XX:+UseG1GC" oninput="wizardData.javaArgs=this.value"></div>'
      +'<div class="check-row"><input type="checkbox" id="cfgEula" onchange="wizardData.eula=this.checked"><label>'+t('webui.wizardAgreeEula')+'</label></div>'
      +'</div>';
  } else if(wizardStep===3){
    body.innerHTML = '<div class="confirm-card">'
      +'<div class="row"><span class="key">'+t('webui.wizardType')+'</span><span class="val" style="color:'+TYPE_COLORS[wizardData.type]+'">'+wizardData.type.toUpperCase()+'</span></div>'
      +'<div class="row"><span class="key">'+t('webui.wizardVersion')+'</span><span class="val">'+esc(wizardData.version)+'</span></div>'
      +(wizardData.build?'<div class="row"><span class="key">'+t('webui.wizardBuild')+'</span><span class="val">'+esc(wizardData.build)+'</span></div>':'')
      +'<div class="row"><span class="key">'+t('webui.wizardName')+'</span><span class="val">'+esc(wizardData.name)+'</span></div>'
      +'<div class="row"><span class="key">'+t('webui.wizardPath')+'</span><span class="val">'+esc(wizardData.path)+'</span></div>'
      +'<div class="row"><span class="key">'+t('webui.wizardMemory')+'</span><span class="val">'+wizardData.minRam+'M - '+wizardData.maxRam+'M</span></div>'
      +'<div class="row"><span class="key">'+t('webui.wizardJava')+'</span><span class="val">'+esc(wizardData.javaPath||'java')+'</span></div>'
      +'<div class="row"><span class="key">'+t('webui.wizardEula')+'</span><span class="val" style="color:'+(wizardData.eula?'var(--green)':'var(--red)')+'">'+(wizardData.eula?t('webui.wizardAgreed'):t('webui.wizardNotAgreed'))+'</span></div>'
      +'</div>';
  }
}

function selType(t){ wizardData.type=t; renderWizard(); }

async function loadVersions(){
  var vs = document.getElementById('verSelect'); if(!vs) return;
  try {
    var r = await apiGet('/versions');
    if(r.versions&&r.versions.length){
      vs.innerHTML = r.versions.map(function(v){return '<option value="'+v+'" '+(v===wizardData.version?'selected':'')+'>'+v+'</option>';}).join('');
      if(!wizardData.version&&r.versions.length) wizardData.version=r.versions[0];
    }
    if(r.cached===false) setTimeout(loadVersions,2000);
  }catch(e){ vs.innerHTML='<option>Failed</option>'; }
}

async function doCreate(){
  var btn = document.getElementById('btnNext');
  btn.disabled=true; btn.textContent=t('webui.wizardCreating');
  try {
    var body = {
      type:wizardData.type, version:wizardData.version, build:wizardData.build||'',
      name:wizardData.name, path:wizardData.path, javaPath:wizardData.javaPath||'java',
      minRamMB:wizardData.minRam, maxRamMB:wizardData.maxRam, javaArgs:wizardData.javaArgs||'',
      download:true
    };
    var r = await apiPost('/servers', body);
    if(r.success){
      notify(t('webui.wizardCreated')+' '+(r.downloading?t('webui.wizardDownloadingJar'):''));
      await refreshAll();
      if(r.server) openTab(r.server.id);
    } else { notify(r.message||'Failed','err'); btn.disabled=false; btn.textContent=t('webui.wizardCreate'); }
  } catch(e){ notify(t('webui.networkError'),'err'); btn.disabled=false; btn.textContent=t('webui.wizardCreate'); }
}

// ═══════════════ EDIT MODAL ═══════════════
function openEditModal(id){
  editServerId=id;
  var s=servers.find(function(x){return x.id===id;});
  if(!s) return;
  document.getElementById('editName').value=s.name||'';
  document.getElementById('editType').value=s.type||'vanilla';
  document.getElementById('editVersion').value=s.version||'';
  document.getElementById('editMinRam').value=s.minRamMB||1024;
  document.getElementById('editMaxRam').value=s.maxRamMB||4096;
  document.getElementById('editJavaPath').value=s.javaPath||'java';
  document.getElementById('editJavaArgs').value=s.javaArgs||'';
  document.getElementById('editModal').classList.add('show');
}
function closeEditModal(){ document.getElementById('editModal').classList.remove('show'); }
async function saveEditConfig(){
  var body={
    name:document.getElementById('editName').value,
    type:document.getElementById('editType').value,
    version:document.getElementById('editVersion').value,
    minRamMB:parseInt(document.getElementById('editMinRam').value),
    maxRamMB:parseInt(document.getElementById('editMaxRam').value),
    javaPath:document.getElementById('editJavaPath').value,
    javaArgs:document.getElementById('editJavaArgs').value
  };
  var r=await apiPost('/servers/'+editServerId,body,'PUT');
  if(r.success){ closeEditModal(); notify(t('webui.configSaved')); refreshAll(); }
  else notify(r.message||'Failed','err');
}

// ═══════════════ DELETE MODAL ═══════════════
function openDeleteModal(id,name){
  pendingDeleteId=id;
  document.getElementById('deleteMsg').textContent=t('webui.deleteMsg').replace('%1',name);
  document.getElementById('deleteModal').classList.add('show');
}
function closeDeleteModal(){ document.getElementById('deleteModal').classList.remove('show'); pendingDeleteId=null; }
async function confirmDelete(){
  if(!pendingDeleteId) return;
  var r=await apiPost('/servers/'+pendingDeleteId,{},'DELETE');
  closeDeleteModal();
  if(r.success){
    // Close tab if open
    closeTabSilent(pendingDeleteId);
    if(activeTabId === pendingDeleteId) activeTabId = null;
    notify(t('webui.serverDeleted'));
    showWelcome();
    refreshAll();
  }
  else notify(r.message||'Failed','err');
}

// ═══════════════ GLOBAL REFRESH (lightweight) ═══════════════
async function refreshAll(){
  try {
    var r=await apiGet('/servers');
    if(r.servers){
      var prev = servers;
      servers=r.servers;
      renderList();
      // Light-update active tab's detail if we're in detail view
      if(activeTabId && view==='detail'){
        var s = servers.find(function(x){return x.id===activeTabId;});
        var ps = prev.find(function(x){return x.id===activeTabId;});
        if(s) refreshTabPanelLight(activeTabId, s, ps);
      }
    }
    document.getElementById('apiStatus').className='ind';
    document.getElementById('apiText').textContent=t('webui.apiConnected');
  }catch(e){
    document.getElementById('apiStatus').className='ind off';
    document.getElementById('apiText').textContent=t('webui.apiDisconnected');
  }
}

function refreshTabPanelLight(id, s, prevS){
  var changed = !prevS || prevS.state !== s.state;
  if(changed){
    // Update status dot
    var dot = document.querySelector('#panel_'+id+' .detail-top div[style]');
    if(dot){
      dot.style.cssText = s.state==='running'
        ? 'width:10px;height:10px;border-radius:50%;background:var(--green);box-shadow:0 0 6px var(--green)'
        : 'width:10px;height:10px;border-radius:50%;background:#444';
    }
    // Update status badge
    var st = document.querySelector('#panel_'+id+' .sstatus');
    if(st){
      st.className = 'sstatus '+(s.state==='running'?'online':'offline');
      st.textContent = s.state.toUpperCase();
    }
    // Update stat card
    var statStatus = document.getElementById('statStatus_'+id);
    if(statStatus){
      statStatus.className = 'value '+(s.state==='running'?'green':'');
      statStatus.textContent = s.state.charAt(0).toUpperCase()+s.state.slice(1);
    }
    // Update action buttons
    var act = document.querySelector('#panel_'+id+' .detail-actions');
    if(act){
      act.innerHTML = s.state==='running'
        ? '<button class="btn btn-red btn-sm" id="btnStop_'+id+'">&#x25A0; '+t('webui.stop')+'</button>'
         +'<button class="btn btn-orange btn-sm" id="btnKill_'+id+'">'+t('webui.forceKill')+'</button>'
        : '<button class="btn btn-green btn-sm" id="btnStart_'+id+'">&#x25B6; '+t('webui.start')+'</button>';
      act.innerHTML +=
        '<button class="btn btn-ghost btn-sm" id="btnEdit_'+id+'">&#x2699; '+t('webui.edit')+'</button>'
       +'<button class="btn btn-ghost btn-sm" id="btnDelete_'+id+'" style="color:var(--red);border-color:rgba(255,107,107,.3)">'+t('webui.delete')+'</button>'
       +'<button class="btn btn-ghost btn-sm" id="btnRefresh_'+id+'">&#x21BB;</button>'
       +'<button class="btn btn-primary btn-sm" id="btnProps_'+id+'">'+t('webui.properties')+'</button>'
       +'<button class="btn btn-primary btn-sm" id="btnMods_'+id+'">&#x1F4E6; '+t('webui.mods')+'</button>'
      +'<button class="btn btn-primary btn-sm" id="btnPW_'+id+'">&#x1F465; '+t('webui.playerWorld')+'</button>'
      +'<button class="btn btn-primary btn-sm" id="btnMP_'+id+'">&#x1F4E6; '+t('webui.installModpack')+'</button>';
      var be=document.getElementById('btnEdit_'+id); if(be) be.onclick=function(){openEditModal(id);};
      var bd=document.getElementById('btnDelete_'+id); if(bd) bd.onclick=function(){openDeleteModal(id,s.name);};
      var br=document.getElementById('btnRefresh_'+id); if(br) br.onclick=refreshAll;
      var bpr=document.getElementById('btnProps_'+id); if(bpr) bpr.onclick=function(){openProperties(id);};
      var bmo=document.getElementById('btnMods_'+id); if(bmo) bmo.onclick=function(){openModsModal(id);};
      var bpw=document.getElementById('btnPW_'+id); if(bpw) bpw.onclick=function(){openPWModal(id);};
      var bmp=document.getElementById('btnMP_'+id); if(bmp) bmp.onclick=function(){openMPModal();};
      var bst=document.getElementById('btnStart_'+id); if(bst) bst.onclick=function(){doTabAction(id,'start');};
      var bp=document.getElementById('btnStop_'+id); if(bp) bp.onclick=function(){doTabAction(id,'stop');};
      var bk=document.getElementById('btnKill_'+id); if(bk) bk.onclick=function(){doTabAction(id,'kill');};
    }
    // Enable/disable command input & quick actions & cheat sheet
    var ci = document.getElementById('cmdInput_'+id);
    var cb = document.getElementById('btnSendCmd_'+id);
    if(ci) ci.disabled = (s.state!=='running');
    if(cb) cb.disabled = (s.state!=='running');
    var cht = document.getElementById('cheatBtn_'+id);
    if(cht) cht.disabled = (s.state!=='running');
    var qaBtns = document.querySelectorAll('#quickActions_'+id+' .qa-btn');
    qaBtns.forEach(function(b){ b.disabled = (s.state!=='running'); });

    // Restart timers if state changed
    if(changed && s.state==='running'){
      var tt = tabTimers[id];
      if(tt && !tt.uptimeTimer){
        startTimerForTab(id);
      }
    }
  }
  // Update uptime
  var uv = document.getElementById('uptimeVal_'+id);
  var tt = tabTimers[id];
  if(uv && s.state==='running' && tt && tt.uptimeSecs>0) uv.textContent = formatUptime(tt.uptimeSecs);
  if(uv && s.state!=='running') uv.textContent = 'N/A';
}

// ── Download polling ──
let dlPoll=null, dlDoneNotified=false;
function pollDownload(){
  if(dlPoll) return;
  dlPoll=setInterval(async function(){
    try{var r=await apiGet('/download/status');
      var ind=document.getElementById('dlIndicator');
      if(!ind) return;
      if(r.downloading){
        dlDoneNotified=false;
        ind.classList.add('show');
        document.getElementById('dlText').textContent=r.status||t('webui.downloading');
        document.getElementById('dlBarFill').style.width=(r.percent||0)+'%';
      } else if(r.percent===100){
        if(!dlDoneNotified){
          dlDoneNotified=true;
          ind.classList.add('show');
          document.getElementById('dlText').textContent=t('webui.downloadComplete');
          document.getElementById('dlBarFill').style.width='100%';
          setTimeout(function(){ind.classList.remove('show');refreshAll();},4000);
        }
      } else { ind.classList.remove('show'); }
    }catch(e){}
  },2000);
}

// ═══════════════ Server Properties Editor ═══════════════
let g_propsServerId = null;
let g_propsData = null;       // { properties: {...}, definitions: [...] }
let g_propsShowAdvanced = false;

// Category → section metadata
const PROP_SECTIONS = [
  {key:'basic',    title:t('webui.secBasic'),     cat:'common'},
  {key:'world',    title:t('webui.secWorld'),     cat:'common'},
  {key:'player',   title:t('webui.secPlayer'),    cat:'common'},
  {key:'network',  title:t('webui.secNetwork'),   cat:'advanced'},
  {key:'rcon',     title:t('webui.secRcon'),      cat:'advanced'},
  {key:'security', title:t('webui.secSecurity'),  cat:'advanced'},
  {key:'advanced', title:t('webui.secWorldAdv'),  cat:'advanced'},
  {key:'performance',title:t('webui.secPerformance'),cat:'advanced'},
  {key:'resource', title:t('webui.secResource'),  cat:'advanced'},
  {key:'other',    title:t('webui.secOther'),     cat:'advanced'},
];

function openProperties(id){
  var s = servers.find(function(x){return x.id===id;});
  if(!s) return;
  g_propsServerId = id;
  g_propsShowAdvanced = false;
  document.getElementById('propsServerName').textContent = s.name;
  document.getElementById('propsToggleAdvanced').textContent = t('webui.showAdvanced');
  document.getElementById('propsForm').innerHTML = '<div style="text-align:center;color:var(--dim);padding:30px;grid-column:1/-1">'+t('webui.loadingProperties')+'</div>';
  document.getElementById('propsErr').textContent = '';
  document.getElementById('propsModal').classList.add('show');

  // Fetch
  (async function(){
    try {
      var r = await apiGet('/servers/'+id+'/properties');
      if(!r.success){ document.getElementById('propsForm').innerHTML='<div style="color:var(--red);grid-column:1/-1">'+t('webui.propsFailed')+'</div>'; return; }
      g_propsData = r;
      renderPropertiesForm();
    } catch(e){ document.getElementById('propsForm').innerHTML='<div style="color:var(--red);grid-column:1/-1">'+t('webui.propsFailed')+'</div>'; }
  })();
}

function closePropsModal(){
  document.getElementById('propsModal').classList.remove('show');
  g_propsServerId = null;
  g_propsData = null;
}

function toggleAdvancedProps(){
  g_propsShowAdvanced = !g_propsShowAdvanced;
  document.getElementById('propsToggleAdvanced').textContent = g_propsShowAdvanced ? t('webui.hideAdvanced') : t('webui.showAdvanced');
  renderPropertiesForm();
}

function togglePropsSection(secKey){
  var el = document.getElementById('propsSec_'+secKey);
  var hd = document.getElementById('propsSecHd_'+secKey);
  if(!el||!hd) return;
  if(el.style.display==='none'){
    el.style.display = 'grid';
    hd.classList.remove('collapsed');
  } else {
    el.style.display = 'none';
    hd.classList.add('collapsed');
  }
}

function renderPropertiesForm(){
  if(!g_propsData) return;
  var defs = g_propsData.definitions || [];
  var props = g_propsData.properties || {};

  // Group definitions by section
  var sections = {};
  defs.forEach(function(d){
    if(!sections[d.category]) sections[d.category] = [];
    sections[d.category].push(d);
  });

  // Build groups for display
  var cats = PROP_SECTIONS.filter(function(sec){ return sections[sec.key] && sections[sec.key].length > 0; });

  var html = '';
  cats.forEach(function(sec){
    var isAdvanced = sec.cat === 'advanced';
    var showBody = !isAdvanced || g_propsShowAdvanced;
    var cls = isAdvanced ? (g_propsShowAdvanced ? 'props-group' : 'props-group') : 'props-group';

    html += '<div class="props-group">';
    html += '<div class="props-section-title'+(showBody?'':' collapsed')+'" id="propsSecHd_'+sec.key+'" onclick="togglePropsSection(\''+sec.key+'\')">'
      +'<span class="arrow">&#x25BC;</span>'+esc(sec.title)+'</div>';
    html += '<div class="props-section-body" id="propsSec_'+sec.key+'" style="display:'+(showBody?'grid':'none')+'">';

    var items = sections[sec.key];
    items.forEach(function(d){
      var val = props[d.key]!==undefined ? props[d.key] : d.default;
      html += '<div class="props-item"><label>'+esc(d.label)+'</label>';
      if(d.type === 'bool'){
        var chk = (val==='true'||val===true)?' checked':'';
        html += '<div class="props-check-row"><input type="checkbox" id="prop_'+d.key+'" '+chk+' onchange="this.dataset.val=this.checked?\'true\':\'false\'" data-val="'+(val==='true'||val===true?'true':'false')+'">'
          +'<label for="prop_'+d.key+'" style="font-weight:400;color:var(--text)">'+(val==='true'||val===true?t('webui.enabled'):t('webui.disabled'))+'</label></div>';
      } else if(d.type === 'enum'){
        var opts = (d.options||'').split(',');
        html += '<select id="prop_'+d.key+'">'+opts.map(function(o){return '<option value="'+esc(o)+'" '+(o===val?'selected':'')+'>'+esc(o)+'</option>';}).join('')+'</select>';
      } else if(d.type === 'number'){
        html += '<input type="number" id="prop_'+d.key+'" value="'+esc(String(val))+'"'+(d.options?(' min="'+d.options.split(',')[0]+'" max="'+d.options.split(',')[1]+'"'):'')+'>';
      } else {
        html += '<input type="text" id="prop_'+d.key+'" value="'+esc(String(val||''))+'">';
      }
      html += '</div>';
    });

    html += '</div></div>';
  });

  if(cats.length===0){
    html = '<div style="text-align:center;color:var(--dim);padding:20px;grid-column:1/-1">'+t('webui.propsNoProperties')+'</div>';
  }

  document.getElementById('propsForm').innerHTML = html;
}

async function saveProperties(){
  if(!g_propsServerId||!g_propsData) return;
  var btn = document.getElementById('btnSaveProps');
  var err = document.getElementById('propsErr');
  err.textContent = '';

  var props = {};
  var defs = g_propsData.definitions || [];
  defs.forEach(function(d){
    var el = document.getElementById('prop_'+d.key);
    if(!el) return;
    if(d.type === 'bool'){
      props[d.key] = el.checked ? 'true' : 'false';
    } else {
      props[d.key] = el.value;
    }
  });

  btn.disabled = true;
  btn.textContent = t('webui.propsSaving');

  try {
    var r = await apiPost('/servers/'+g_propsServerId+'/properties', {properties:props}, 'PUT');
    if(r.success){
      notify(t('webui.propsSaved'));
      closePropsModal();
    } else {
      err.textContent = r.message || t('webui.failedToSave');
    }
  } catch(e){
    err.textContent = t('webui.networkError');
  }
  btn.disabled = false;
  btn.textContent = t('webui.propsSaveBtn');
}

// ═══════════════ MOD MANAGEMENT ═══════════════
var g_modsServerId = null;
var g_modsData = null;
var g_modsTab = 'installed';

function openModsModal(id){
  g_modsServerId = id;
  g_modsTab = 'installed';
  var s = servers.find(function(x){return x.id===id;});
  if(s) document.getElementById('modServerName').textContent = s.name;
  document.getElementById('modInstalledPanel').style.display = 'block';
  document.getElementById('modMarketPanel').style.display = 'none';
  document.getElementById('modContent').style.display = 'block';
  document.getElementById('modContent').innerHTML = '<div style="text-align:center;color:var(--dim);padding:30px">'+t('webui.loadingMods')+'</div>';
  document.getElementById('modConflicts').innerHTML = '';
  document.getElementById('modModal').classList.add('show');
  loadMods(id);
}

function closeModsModal(){
  document.getElementById('modModal').classList.remove('show');
  g_modsServerId = null;
  g_modsData = null;
}

async function loadMods(id){
  try {
    var r = await apiGet('/servers/'+id+'/mods');
    if(!r.success){ document.getElementById('modContent').innerHTML='<div style="color:var(--red);text-align:center;padding:30px">'+t('webui.failedToLoad')+'</div>'; return; }
    g_modsData = r;
    document.getElementById('modContent').style.display = 'none';
    renderModsList();
    renderModConflicts();
  } catch(e){
    document.getElementById('modContent').innerHTML='<div style="color:var(--red);text-align:center;padding:30px">'+t('webui.networkError')+'</div>';
  }
}

async function scanMods(){
  if(!g_modsServerId) return;
  document.getElementById('modContent').style.display = 'block';
  document.getElementById('modContent').innerHTML = '<div style="text-align:center;color:var(--dim);padding:30px">'+t('webui.loadingMods')+'</div>';
  document.getElementById('modConflicts').innerHTML = '';
  await loadMods(g_modsServerId);
}

function switchModTab(tab){
  g_modsTab = tab;
  document.getElementById('modTabInstalled').className = 'mod-tab' + (tab==='installed'?' active':'');
  document.getElementById('modTabMarket').className = 'mod-tab' + (tab==='market'?' active':'');
  if(tab==='market'){
    document.getElementById('modInstalledPanel').style.display = 'none';
    document.getElementById('modMarketPanel').style.display = 'block';
    document.getElementById('modContent').style.display = 'none';
    initModrinthMarket();
  } else {
    document.getElementById('modInstalledPanel').style.display = 'block';
    document.getElementById('modMarketPanel').style.display = 'none';
    document.getElementById('modContent').style.display = 'none';
    if(g_modsData) renderModsList();
  }
}

function renderModsList(){
  var mods = g_modsData ? g_modsData.mods : [];
  var listEl = document.getElementById('modList');
  var emptyEl = document.getElementById('modEmpty');
  if(!mods||mods.length===0){
    if(listEl) listEl.innerHTML = '';
    if(emptyEl) emptyEl.style.display = 'block';
    return;
  }
  if(emptyEl) emptyEl.style.display = 'none';
  if(listEl) listEl.innerHTML = mods.map(function(m){
    return '<div class="mod-item'+(m.enabled===false?' disabled':'')+'">'
      +'<div class="mod-icon">&#x1F4E6;</div>'
      +'<div class="mod-info">'
      +'<div class="mod-name" title="'+esc(m.name||m.fileName||'')+'">'+esc(m.name||m.fileName||'')+'</div>'
      +'<div class="mod-meta">v'+esc(m.version||'?')+' &mdash; '+esc(m.fileName||'')+'</div>'
      +'</div>'
      +(m.loader&&m.loader!=='any'?'<span class="mod-loader">'+esc(m.loader)+'</span>':'')
      +'<div class="mod-actions">'
      +(m.enabled===false
        ?'<button onclick="toggleMod(event,\''+m.fileName.replace(/'/g,"\\'")+'\',true)">'+t('webui.enable')+'</button>'
        :'<button onclick="toggleMod(event,\''+m.fileName.replace(/'/g,"\\'")+'\',false)">'+t('webui.disable')+'</button>')
      +'<button class="mod-remove" onclick="removeMod(event,\''+m.fileName.replace(/'/g,"\\'")+'\')">'+t('webui.remove')+'</button>'
      +'</div></div>';
  }).join('');
}

function renderModConflicts(){
  var conflicts = g_modsData ? g_modsData.conflicts : [];
  var el = document.getElementById('modConflicts');
  if(!el) return;
  if(!conflicts||conflicts.length===0){ el.innerHTML=''; return; }
  el.innerHTML = conflicts.map(function(c){
    var nameA = (c.modA&&c.modA.name) || (c.modA&&c.modA.fileName) || '?';
    var nameB = (c.modB&&c.modB.name) || (c.modB&&c.modB.fileName) || '?';
    var reason = c.reason==='loader_mismatch' ? t('webui.modConflictLoader') : t('webui.modConflictDeclared');
    return '<div class="mod-conflict">&#x26A0; '+esc(nameA)+' &harr; '+esc(nameB)+' &mdash; '+esc(reason)+'</div>';
  }).join('');
}

async function toggleMod(evt, fileName, enable){
  if(evt) evt.stopPropagation();
  if(!g_modsServerId) return;
  var r = await apiPost('/servers/'+g_modsServerId+'/mods/toggle',{fileName:fileName, enabled:enable});
  if(r.success){
    notify(enable?t('webui.modEnabled'):t('webui.modDisabled'));
    await loadMods(g_modsServerId);
  } else notify(r.message||'Failed','err');
}

async function removeMod(evt, fileName){
  if(evt) evt.stopPropagation();
  if(!g_modsServerId) return;
  var r = await apiPost('/servers/'+g_modsServerId+'/mods/remove',{fileName:fileName});
  if(r.success){
    notify(t('webui.modRemoved'));
    await loadMods(g_modsServerId);
  } else notify(r.message||'Failed','err');
}

// ═══════════════ PLAYER / WORLD MANAGEMENT ═══════════════
function openPWModal(id){
  g_pwServerId = id; g_pwTab = 'players';
  var s = servers.find(function(x){return x.id===id;});
  document.getElementById('pwServerName').textContent = s?s.name:id;
  document.getElementById('pwModal').classList.add('show');
  switchPWTab('players');
}

function closePWModal(){
  document.getElementById('pwModal').classList.remove('show');
  g_pwServerId = null;
}

function switchPWTab(tab){
  g_pwTab = tab;
  document.getElementById('pwTabPlayers').className = 'pw-tab' + (tab==='players'?' active':'');
  document.getElementById('pwTabWorlds').className = 'pw-tab' + (tab==='worlds'?' active':'');
  document.getElementById('pwPlayersPanel').style.display = (tab==='players'?'block':'none');
  document.getElementById('pwWorldsPanel').style.display = (tab==='worlds'?'block':'none');
  var loadEl = document.getElementById('pwLoading');
  if(loadEl) loadEl.style.display = 'block';
  if(tab==='players') loadPlayers(g_pwServerId); else loadWorlds(g_pwServerId);
}

async function loadPlayers(id){
  var listEl = document.getElementById('pwPlayerList');
  var emptyEl = document.getElementById('pwPlayerEmpty');
  var loadEl = document.getElementById('pwLoading');
  try {
    var r = await apiGet('/servers/'+id+'/players');
    if(loadEl) loadEl.style.display = 'none';
    if(!r.success||!r.players||r.players.length===0){
      if(listEl) listEl.innerHTML = '';
      if(emptyEl) emptyEl.style.display = 'block';
      return;
    }
    if(emptyEl) emptyEl.style.display = 'none';
    if(listEl) listEl.innerHTML = r.players.map(function(p){
      return '<tr>'
        +'<td class="pw-online">'+esc(p.name||'?')+'</td>'
        +'<td>'+esc(p.world||'-')+'</td>'
        +'<td>'+esc(p.health||'?')+'</td>'
        +'<td class="pw-actions">'
        +'<button onclick="kickPlayer(\''+p.name.replace(/'/g,"\\'")+'\')">'+t('webui.kick')+'</button>'
        +'<button onclick="banPlayer(\''+p.name.replace(/'/g,"\\'")+'\')">'+t('webui.ban')+'</button>'
        +'<button onclick="opPlayer(\''+p.name.replace(/'/g,"\\'")+'\')">'+t('webui.op')+'</button>'
        +'<button onclick="deopPlayer(\''+p.name.replace(/'/g,"\\'")+'\')">'+t('webui.deop')+'</button>'
        +'</td></tr>';
    }).join('');
  } catch(e){
    if(loadEl) loadEl.style.display = 'none';
    if(listEl) listEl.innerHTML = '<tr><td colspan="4" style="color:var(--red);text-align:center">'+t('webui.networkError')+'</td></tr>';
  }
}

async function loadWorlds(id){
  var listEl = document.getElementById('pwWorldList');
  var emptyEl = document.getElementById('pwWorldEmpty');
  var loadEl = document.getElementById('pwLoading');
  try {
    var r = await apiGet('/servers/'+id+'/worlds');
    if(loadEl) loadEl.style.display = 'none';
    if(!r.success||!r.worlds||r.worlds.length===0){
      if(listEl) listEl.innerHTML = '';
      if(emptyEl) emptyEl.style.display = 'block';
      return;
    }
    if(emptyEl) emptyEl.style.display = 'none';
    if(listEl) listEl.innerHTML = r.worlds.map(function(w){
      return '<tr>'
        +'<td>'+esc(w.name||'?')+'</td>'
        +'<td>'+esc(w.size||'?')+'</td>'
        +'<td class="pw-actions">'
        +'<button onclick="backupWorld(\''+w.name.replace(/'/g,"\\'")+'\')">'+t('webui.backup')+'</button>'
        +'<button class="pw-danger" onclick="deleteWorld(\''+w.name.replace(/'/g,"\\'")+'\')">'+t('webui.delete')+'</button>'
        +'</td></tr>';
    }).join('');
  } catch(e){
    if(loadEl) loadEl.style.display = 'none';
    if(listEl) listEl.innerHTML = '<tr><td colspan="3" style="color:var(--red);text-align:center">'+t('webui.networkError')+'</td></tr>';
  }
}

async function kickPlayer(name){
  if(!confirm(t('webui.kickConfirm').replace('%1',name))) return;
  var r = await apiPost('/servers/'+g_pwServerId+'/players/kick',{player:name});
  if(r.success){ notify(t('webui.playerKicked').replace('%1',name)); loadPlayers(g_pwServerId); }
  else notify(r.message||'Failed','err');
}

async function banPlayer(name){
  if(!confirm(t('webui.banConfirm').replace('%1',name))) return;
  var r = await apiPost('/servers/'+g_pwServerId+'/players/ban',{player:name});
  if(r.success){ notify(t('webui.playerBanned').replace('%1',name)); loadPlayers(g_pwServerId); }
  else notify(r.message||'Failed','err');
}

async function opPlayer(name){
  var r = await apiPost('/servers/'+g_pwServerId+'/players/op',{player:name});
  if(r.success) notify(t('webui.playerOpped').replace('%1',name));
  else notify(r.message||'Failed','err');
}

async function deopPlayer(name){
  var r = await apiPost('/servers/'+g_pwServerId+'/players/deop',{player:name});
  if(r.success) notify(t('webui.playerDeopped').replace('%1',name));
  else notify(r.message||'Failed','err');
}

async function backupWorld(name){
  var r = await apiPost('/servers/'+g_pwServerId+'/worlds/backup',{world:name});
  if(r.success){ notify(t('webui.worldBackedUp').replace('%1',name)); loadWorlds(g_pwServerId); }
  else notify(r.message||'Failed','err');
}

async function deleteWorld(name){
  if(!confirm(t('webui.deleteWorldConfirm').replace('%1',name))) return;
  var r = await apiPost('/servers/'+g_pwServerId+'/worlds/delete',{world:name});
  if(r.success){ notify(t('webui.worldDeleted').replace('%1',name)); loadWorlds(g_pwServerId); }
  else notify(r.message||'Failed','err');
}

// ═══════════════ MODPACK INSTALL ═══════════════
function openMPModal(){
  g_mpFileData = null; g_mpAnalysis = null;
  document.getElementById('mpModal').classList.add('show');
  document.getElementById('mpUploadZone').style.display = 'block';
  document.getElementById('mpAnalyzing').style.display = 'none';
  document.getElementById('mpAnalysis').style.display = 'none';
  document.getElementById('mpInstallForm').style.display = 'none';
  document.getElementById('mpError').textContent = '';
  document.getElementById('mpStatus').textContent = '';
  document.getElementById('mpFileInput').value = '';
}

function closeMPModal(){
  document.getElementById('mpModal').classList.remove('show');
  g_mpFileData = null; g_mpAnalysis = null;
}

function onMPFilePicked(file){
  if(!file) return;
  document.getElementById('mpUploadZone').style.display = 'none';
  document.getElementById('mpAnalyzing').style.display = 'block';
  document.getElementById('mpAnalysis').style.display = 'none';
  document.getElementById('mpInstallForm').style.display = 'none';
  document.getElementById('mpError').textContent = '';
  var reader = new FileReader();
  reader.onload = async function(){
    var base64 = reader.result.split(',')[1];
    g_mpFileData = base64;
    try {
      var r = await apiPost('/api/modpack/analyze',{file:base64, fileName:file.name});
      document.getElementById('mpAnalyzing').style.display = 'none';
      if(!r.success){ document.getElementById('mpError').textContent = r.message||t('webui.failedToLoad'); return; }
      g_mpAnalysis = r;
      document.getElementById('mpName').textContent = r.name||'?';
      document.getElementById('mpVersion').textContent = r.mcVersion||'?';
      document.getElementById('mpLoader').textContent = r.loader||'?';
      document.getElementById('mpModCount').textContent = (r.modCount||0)+' mods';
      document.getElementById('mpAnalysis').style.display = 'block';
      document.getElementById('mpInstallForm').style.display = 'block';
      document.getElementById('mpServerName').value = r.name||'';
    } catch(e){ document.getElementById('mpAnalyzing').style.display = 'none'; document.getElementById('mpError').textContent = t('webui.networkError'); }
  };
  reader.readAsDataURL(file);
}

async function installModpack(){
  if(!g_mpFileData){ document.getElementById('mpError').textContent = t('webui.analyzing'); return; }
  var btn = document.getElementById('btnInstallMP');
  btn.disabled = true;
  document.getElementById('mpStatus').textContent = t('webui.installingModpack')+'...';
  document.getElementById('mpError').textContent = '';
  try {
    var r = await apiPost('/api/modpack/install',{
      file: g_mpFileData,
      name: document.getElementById('mpServerName').value||'',
      targetPath: document.getElementById('mpTargetPath').value||'',
      minRam: document.getElementById('mpMinRam').value,
      maxRam: document.getElementById('mpMaxRam').value,
      loader: g_mpAnalysis?g_mpAnalysis.loader:'',
      mcVersion: g_mpAnalysis?g_mpAnalysis.mcVersion:''
    });
    if(r.success){
      document.getElementById('mpStatus').textContent = t('webui.modpackInstalled');
      notify(t('webui.modpackInstalled'));
      setTimeout(function(){ closeMPModal(); refreshAll(); }, 2000);
    } else {
      document.getElementById('mpError').textContent = r.message||t('webui.failedToLoad');
    }
  } catch(e){ document.getElementById('mpError').textContent = t('webui.networkError'); }
  btn.disabled = false;
}

// ── MP drag & drop ──
(function setupMPDrop(){
  var zone = document.getElementById('mpUploadZone');
  if(zone){
    zone.addEventListener('dragover',function(e){ e.preventDefault(); zone.classList.add('dragover'); });
    zone.addEventListener('dragleave',function(e){ e.preventDefault(); zone.classList.remove('dragover'); });
    zone.addEventListener('drop',function(e){
      e.preventDefault(); zone.classList.remove('dragover');
      var file = e.dataTransfer.files[0];
      if(file&&file.name.toLowerCase().endsWith('.zip')) onMPFilePicked(file);
    });
  }
})();

// ═══════════════ MODRINTH MARKET ═══════════════
function initModrinthMarket(){
  g_mrServerId = g_modsServerId;
  g_mrOffset = 0; g_mrQuery = ''; g_mrLoader = '';
  var inp = document.getElementById('mrSearchInput');
  var sel = document.getElementById('mrLoaderSelect');
  var res = document.getElementById('mrResults');
  var pag = document.getElementById('mrPagination');
  if(inp) inp.value = '';
  if(sel) sel.value = '';
  if(res) res.innerHTML = '<div style="color:var(--dim);padding:30px;text-align:center">'+t('webui.searchModrinth')+'</div>';
  if(pag) pag.style.display = 'none';
}

async function searchModrinth(offset){
  offset = offset||0;
  if(offset<0) offset=0;
  g_mrOffset = offset;
  var query = document.getElementById('mrSearchInput');
  var loader = document.getElementById('mrLoaderSelect');
  g_mrQuery = query?query.value:'';
  g_mrLoader = loader?loader.value:'';
  g_mrServerId = g_modsServerId;
  var params = [];
  if(g_mrQuery) params.push('query='+encodeURIComponent(g_mrQuery));
  if(g_mrLoader) params.push('loader='+encodeURIComponent(g_mrLoader));
  params.push('offset='+offset);
  params.push('limit=20');
  var res = document.getElementById('mrResults');
  var pag = document.getElementById('mrPagination');
  if(res) res.innerHTML = '<div style="color:var(--dim);padding:30px;text-align:center">'+t('webui.loadingMods')+'</div>';
  if(pag) pag.style.display = 'none';
  try {
    var r = await apiGet('/mods/search?'+params.join('&'));
    if(!r.success||!r.results||r.results.length===0){
      if(res) res.innerHTML = '<div style="color:var(--dim);padding:30px;text-align:center">'+t('webui.noResults')+'</div>';
      return;
    }
    g_mrTotal = r.total||r.totalHits||r.results.length;
    if(res) res.innerHTML = r.results.map(function(m){
      var pid = m.project_id||m.slug||'';
      var title = m.name||m.slug||'?';
      var icon = m.iconUrl||'';
      var catStr = (m.categories&&Array.isArray(m.categories)?m.categories.join(', '):'');
      return '<div class="mr-card" onclick="openMRVersionModal(\''+esc(pid)+'\',\''+esc(title).replace(/'/g,"\\'")+'\')">'
        +(icon?'<div class="mr-icon"><img src="'+esc(icon)+'" alt=""></div>':'<div class="mr-icon">&#x1F4E6;</div>')
        +'<div class="mr-info">'
        +'<div class="mr-name">'+esc(title)+'</div>'
        +'<div class="mr-desc">'+esc(m.description||'')+'</div>'
        +'<div class="mr-meta">'+esc(m.author||'')+(catStr?' &mdash; '+esc(catStr):'')+'</div>'
        +'</div>'
        +'<div class="mr-dl">&#x2B07; '+esc(m.downloads||0)+'</div>'
        +'</div>';
    }).join('');
    if(pag){
      var totalPages = Math.ceil(g_mrTotal/20);
      var curPage = Math.floor(offset/20)+1;
      pag.style.display = 'flex';
      pag.innerHTML = ''
        +'<button '+(offset<=0?'disabled':'')+' onclick="searchModrinth('+(offset-20)+')">'+t('webui.prev')+'</button>'
        +'<span>'+t('webui.page')+' '+curPage+' / '+totalPages+' ('+g_mrTotal+' hits)</span>'
        +'<button '+(offset+20>=g_mrTotal?'disabled':'')+' onclick="searchModrinth('+(offset+20)+')">'+t('webui.next')+'</button>';
    }
  } catch(e){
    if(res) res.innerHTML = '<div style="color:var(--red);padding:30px;text-align:center">'+t('webui.networkError')+'</div>';
  }
}

// ═══════════════ MODRINTH VERSION PICKER ═══════════════
function openMRVersionModal(projectId, modName){
  g_mrProjectId = projectId;
  document.getElementById('mrVersionModName').textContent = modName||projectId;
  document.getElementById('mrVersionModal').classList.add('show');
  document.getElementById('mrVersionList').innerHTML = '';
  document.getElementById('mrVersionLoading').style.display = 'block';
  loadMRVersions(projectId);
}

function closeMRVersionModal(){
  document.getElementById('mrVersionModal').classList.remove('show');
  g_mrProjectId = null;
}

async function loadMRVersions(projectId){
  var listEl = document.getElementById('mrVersionList');
  var loadEl = document.getElementById('mrVersionLoading');
  try {
    var r = await apiGet('/mods/modrinth/'+encodeURIComponent(projectId||g_mrProjectId)+'/versions');
    if(loadEl) loadEl.style.display = 'none';
    if(!r.success||!r.versions||r.versions.length===0){
      if(listEl) listEl.innerHTML = '<div style="color:var(--dim);text-align:center;padding:20px">'+t('webui.noResults')+'</div>';
      return;
    }
    if(listEl) listEl.innerHTML = r.versions.map(function(v){
      return '<div class="mr-version-item">'
        +'<div><div class="mr-ver-name">'+esc(v.name||v.version_number||'?')+'</div>'
        +'<div class="mr-ver-meta">MC '+esc(v.game_versions?v.game_versions.join(', '):'?')+' &mdash; '+esc(v.loaders?v.loaders.join(', '):'?')+' &mdash; '+esc(v.version_type||'')+'</div></div>'
        +'<button class="mr-ver-dl" onclick="downloadModrinthMod(event,\''+esc(v.id||'')+'\',\''+esc(v.name||v.version_number||'').replace(/'/g,"\\'")+'\')">'
        +'&#x2B07; '+t('webui.download')+'</button>'
        +'</div>';
    }).join('');
  } catch(e){
    if(loadEl) loadEl.style.display = 'none';
    if(listEl) listEl.innerHTML = '<div style="color:var(--red);text-align:center;padding:20px">'+t('webui.networkError')+'</div>';
  }
}

async function downloadModrinthMod(evt, versionId, versionName){
  if(evt) evt.stopPropagation();
  if(!g_mrServerId){ notify(t('webui.selectServer'),'err'); return; }
  var btn = evt?evt.target:null;
  if(btn){ btn.disabled = true; btn.textContent = t('webui.downloadingMod'); }
  try {
    var r = await apiPost('/servers/'+g_mrServerId+'/mods/install-modrinth',{versionId:versionId});
    if(r.success){
      notify(t('webui.modDownloaded'));
      closeMRVersionModal();
    } else notify(r.message||'Failed','err');
  } catch(e){ notify(t('webui.networkError'),'err'); }
  if(btn){ btn.disabled = false; btn.textContent = '\\u2B07 '+t('webui.download'); }
}

// ═══════════════ INIT ═══════════════
refreshAll();
pollTimer=setInterval(refreshAll,5000);
pollDownload();
document.getElementById('editModal').onclick=function(e){if(e.target===this)closeEditModal();};
document.getElementById('deleteModal').onclick=function(e){if(e.target===this)closeDeleteModal();};
document.getElementById('propsModal').onclick=function(e){if(e.target===this)closePropsModal();};
document.getElementById('cheatModal').onclick=function(e){if(e.target===this)closeCheatSheet();};
document.getElementById('modModal').onclick=function(e){if(e.target===this)closeModsModal();};
document.getElementById('pwModal').onclick=function(e){if(e.target===this)closePWModal();};
document.getElementById('mpModal').onclick=function(e){if(e.target===this)closeMPModal();};
document.getElementById('mrVersionModal').onclick=function(e){if(e.target===this)closeMRVersionModal();};
// Cheat sheet search & category events
document.getElementById('cheatSearch').oninput = renderCheatList;
document.getElementById('cheatCats').onclick = function(e){
  var btn = e.target.closest('.cm-cat');
  if(!btn) return;
  cheatActiveCat = btn.dataset.cat;
  updateCheatCats();
  renderCheatList();
};
</script>
</body>
</html>
)html";
