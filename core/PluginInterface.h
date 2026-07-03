#pragma once
// ───────────────────────────────────────────────────────────
//  Plugin / Bot Integration Interface
//  MC Server Manager — NapCat / NoneBot 插件命令接口
// ───────────────────────────────────────────────────────────
//
//  架构:
//    用户发送 QQ 消息 → 插件(NapCat/NoneBot)读取 → 自行解析意图
//    → POST 结构化指令到开服器 → 开服器执行 → 返回结果 → 插件回复用户
//
//    开服器只负责执行命令，消息解析和 NLP 逻辑完全在插件侧。
//
// ── 核心端点 ──────────────────────────────────────────────
//
//  POST /api/plugin/execute
//    Body: {
//      "action":   "list"|"status"|"start"|"stop"|"command"|"console",
//      "serverId": "...",     // 除 list 外必填
//      "command":  "...",     // command 动作必填
//      "limit":    20         // console 动作可选，默认 20
//    }
//    Response: {
//      "success":  true,
//      "action":   "list",
//      "data":     { ... 动作特定数据 ... },
//      "message":  "Found 3 server(s)"   // 可直接回复用户
//    }
//
// ── 动作说明 ───────────────────────────────────────────────
//
//  1. list     — 列出所有服务器
//     Body: { "action": "list" }
//     data.servers: [{id, name, type, version, state, ...}]
//     message: "Found 3 server(s)"
//
//  2. status   — 查询服务器状态
//     Body: { "action": "status", "serverId": "xxx" }
//     data: { serverId, serverName, running, state, uptimeSecs }
//     message: "Server '生存服' is running"
//
//  3. start    — 启动服务器
//     Body: { "action": "start", "serverId": "xxx" }
//     data: { success, message, serverId }
//     message: "Server '生存服' is starting..."
//
//  4. stop     — 停止服务器
//     Body: { "action": "stop", "serverId": "xxx" }
//     data: { success, message, serverId }
//     message: "Server '生存服' is stopping..."
//
//  5. command  — 发送控制台命令
//     Body: { "action": "command", "serverId": "xxx", "command": "list" }
//     data: { success, serverId, command }
//     message: "Command 'list' sent to '生存服'"
//
//  6. console  — 获取最近控制台输出
//     Body: { "action": "console", "serverId": "xxx", "limit": 20 }
//     data: { serverId, lines: [...], total: N }
//     message: "Last 20 lines of console output"
//
// ── 认证 ──────────────────────────────────────────────────
//
//  方式 1: Authorization: Bearer <token>
//  方式 2: X-Auth-Token: <token>
//  方式 3: ?token=<token> 查询参数
//
// ── NapCat 集成 ──────────────────────────────────────────
//
//  QQ群消息 → NapCat 收到 → 解析用户意图 → POST execute → 回复
//
//  async function handleMcCommand(msg) {
//    const text = msg.trim();
//    let action, serverId = 'default';
//
//    // 插件自行解析意图（可接入 AI/NLP）
//    if (text.includes('列表')) action = 'list';
//    else if (text.includes('状态')) action = 'status';
//    else if (text.includes('启动')) action = 'start';
//    else if (text.includes('停止')) action = 'stop';
//    else if (text.startsWith('/')) {
//      action = 'command';  // 以 / 开头的当作 MC 命令
//      // 提取 serverId 和 command ...
//    } else return '无法识别';
//
//    const resp = await fetch('http://localhost:25575/api/plugin/execute', {
//      method: 'POST',
//      headers: { 'Content-Type': 'application/json',
//                  'Authorization': 'Bearer YOUR_TOKEN' },
//      body: JSON.stringify({ action, serverId })
//    });
//    const data = await resp.json();
//    return data.message;  // 发给 QQ 群
//  }
//
// ── NoneBot 集成 ──────────────────────────────────────────
//
//  from nonebot import on_command
//  import httpx
//
//  mc = on_command("mc", priority=5)
//
//  @mc.handle()
//  async def handle_mc():
//      args = str(await mc.get_arg("args")).strip().split()
//      if not args:
//          await mc.finish("用法: /mc <list|status|start|stop|cmd> [ID]")
//
//      action = args[0]
//      server_id = args[1] if len(args) > 1 else "default"
//
//      body = {"action": action, "serverId": server_id}
//      # 如果是 command 动作，从消息中提取命令
//      if action == "command" and len(args) > 2:
//          body["command"] = " ".join(args[2:])
//
//      async with httpx.AsyncClient() as client:
//          resp = await client.post(
//              "http://localhost:25575/api/plugin/execute",
//              json=body,
//              headers={"Authorization": "Bearer YOUR_TOKEN"}
//          )
//          data = resp.json()
//          await mc.finish(data.get("message", "完成"))
//
// ── WebUI 网页面板 ────────────────────────────────────────
//
//  GET /  → 完整的浏览器端开服器管理面板
//    支持服务器列表、启动/停止、控制台、发送命令
//    WebUI 通过 /api/* 端点与后端交互
//
// ───────────────────────────────────────────────────────────
