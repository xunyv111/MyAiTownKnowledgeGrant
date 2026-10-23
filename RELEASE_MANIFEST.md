# 0.4.0 交付清单

交付日期：2026-10-23
目标引擎：Unreal Engine 5.7.4  
目标平台：Win64  
状态：AI-NPC ARPG 可集成候选（Beta）

## 新增能力

- 插件原生运行时 BYOK 面板与蓝图打开节点；
- GameInstance 级 OpenAI-compatible Provider 网关，Key 仅驻进程内存；
- 每 NPC 自动隔离 Session，本地 Transcript 默认不上传；
- 玩家明确同意后才发送角色说明、当轮授权事实和当前发言；
- Dialogue / Semantic Review 双 Provider 通道；
- 严格 JSON → 机械事实门 → 语义门 → `FinalApprovedText`。

## 构建与回归

- UHT `WarningsAsErrors`：通过；
- UnrealEditor、UnrealGame Development、UnrealGame Shipping：全部通过；
- 插件 Automation：12/12，0 警告，0 失败；
- 宿主 Automation：5/5，0 失败；网络用例保留 36 条 UE NetGUID 框架警告；
- 静态结构验证：通过。

Editor DLL SHA-256：`1D7A69D540DE084286263806FA9909FCCFE15E09C11676BDFC93C13592193121`

## 边界

未使用真实付费厂商 Key 发起网络请求，因此未验证具体厂商 SLA、配额、费用、内容政策和协议差异。当前协议为 OpenAI-compatible Chat Completions。玩家 BYOK 适合单机或本地 listen-server；正式专用服务器应使用项目自有服务端凭据。目标游戏仍需完成 Cook/Package、真实厂商沙盒、人工 UI/Play 和隐私合规验收。
