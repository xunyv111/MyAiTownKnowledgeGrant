# AI-NPC 厂商连接与本地会话

## 玩家输入位置

在游戏的暂停菜单或设置菜单调用蓝图节点：

`Show AI NPC Provider Settings(PlayerController)`

插件会把原生 `AI NPC Provider Settings Widget` 加到本地玩家视口。玩家填写：

- 厂商名称；
- 完整 OpenAI-compatible `/chat/completions` URL；
- 模型名称；
- API Key；
- 是否同意向所选厂商发送当轮数据；
- 语义审查是否复用同一厂商，或使用第二个厂商/模型。项目默认开启 `bRequireIndependentSemanticReview`，强制审查通道与对话通道使用不同的 URL 或模型，面板会禁用“共用同一厂商”勾选；项目可在 Project Settings 中关闭该强制。

点击 `Apply for this game session` 后，Key 从密码输入框移入 `AI NPC Provider Subsystem` 的进程内存，并立即清空输入框。插件不调用 `SaveConfig`、`GConfig` 或 SaveGame 保存 Key，也不会记录 Key。退出游戏或点击 `Clear credentials` 后凭据消失。

`Project Settings → Plugins → AI NPC Roleplay` 只配置非敏感默认 URL、模型、超时、长度限制和玩家披露文本，绝不能填写密钥。

## 每 NPC 自动本地会话

给 NPC Actor 添加 `AI NPC Dialogue Component`：

1. 配置稳定 `Npc Id`、显示名和公开角色说明；
2. 绑定当前玩家的 `Knowledge Grant Component`；
3. 配置候选 Knowledge Packs 和实际 Context Collectors；
4. 配置机械 Guard Policy；
5. 配置 `Knowledge Semantic Review Component` 和 Review Policy；
6. 调用 `Send Player Message`。

组件在 `BeginPlay` 调用 `OpenNpcSession`，为每个 NPC 生成独立 SessionId。本地 Transcript 只记录已完成的最终文本，默认最多 24 轮；它不会进入 HTTP 请求。默认情况下 NPC 共享 GameInstance 级 Provider；也可以通过 Persona 的 `ProviderProfileId` 为单个 NPC 选择独立 Provider Profile，复用同一网关但使用不同模型、地址和凭据。

对话请求支持 `ClientRequestId` 幂等：相同 `ClientRequestId + 输入哈希 + AuthorizationEpoch/ScopeFingerprint` 不会重复调用 Provider。pending 重复返回原请求 ID，completed 重复会重放缓存回复；同一 ID 携带不同输入会失败关闭。

## 实际上传给厂商的数据

勾选同意并发送一轮对话时，厂商会收到：

- NPC 的公开显示名与 Role Instructions；
- 该轮 Knowledge View 中已授权、已按语境筛选的 FactId 与事实；
- 玩家当前输入；
- 严格 JSON 输出合同。

插件不会发送：

- API Key 之外的本地凭据（Key 只放在 HTTPS Authorization Header）；
- 完整本地对话 Transcript；
- 未授权或当前语境未激活的知识包；
- SaveGame、审计日志或其他 NPC 的 Session。

因此，“本地运行”指会话状态、密钥和安全门在本机/当前游戏进程中运行，不代表云推理离线。若要求任何文本都不离开设备，应把 URL 指向 `127.0.0.1` 或 `localhost` 的 OpenAI-compatible 本地服务。

## 安全链

`玩家输入 → Provider HTTPS → 严格 JSON 候选 → Mechanical Guard → 独立 Semantic Reviewer → FinalApprovedText`

- 远程厂商只允许 HTTPS；HTTP 仅允许显式启用的 loopback。
- 未勾选传输同意、未配置 Key、配置不完整或请求失败时不联网并返回明确错误。
- Provider 原文不会通过 Blueprint 正式出口；`AI NPC Dialogue Component` 只广播 `On Final Reply`。
- Semantic Reviewer 返回逐 Claim 的 `claimAssessments`：支持度、矛盾、越权、任务泄露、角色契合、置信度和 EvidenceFactIds；不能提供替换台词。
- 审查器证据必须属于源 Claim；缺失评分、覆盖不完整、证据不匹配或任一安全枚举失败都会失败关闭。
- 语义服务不可用时使用宿主可信策略的 ErrorText，不退回显示原始模型文本。
- 严格 JSON 失败可做一次受控格式修复；连接失败、429 和临时服务错误可有限重试。

## 联机边界

玩家 BYOK 适合单机或 listen-server 本地玩家。正式多人专用服务器不能从客户端索取或复制玩家 Key；应在服务器进程配置项目自有 Provider，并由服务器承担提示词、费用、审计和隐私告知。客户端提供的“已审查”标志一律不可信。
