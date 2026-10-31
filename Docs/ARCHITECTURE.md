# AI-NPC 插件架构与工作原理

## 总体架构

```text
玩家设置菜单
  └─ Show AI NPC Provider Settings
       └─ Provider Settings Widget（密码框、传输同意）
            └─ AI NPC Provider Subsystem（GameInstance，Key 仅驻内存）
                 ├─ Dialogue Provider HTTPS
                 └─ Semantic Reviewer HTTPS

每个 NPC Actor
  └─ AI NPC Dialogue Component
       ├─ 独立 SessionId
       ├─ Persona / Role Instructions
       ├─ 本地 Transcript（不随请求上传）
       └─ 当前玩家知识与语境引用
```

每个 NPC 并不启动一个重复的 HTTP 服务器，也不复制 API Key。它自动获得一个隔离的本地 Session，并可通过 `ProviderProfileId` 选择独立 Provider Profile；未指定时使用 GameInstance 级默认 Provider。每个 NPC 的 Session、请求和迟到响应都独立关联。

## 一轮对话

```text
玩家输入
  ↓
Dialogue Component 读取当前 PlayerKnowledge + Context Collectors
  ↓
BuildDialogueKnowledgeView
  ↓ 只留下：已授权 ∩ 当前语境激活
当轮 Prompt（角色说明 + 授权 FactId/事实 + 当前输入）
  ↓ 玩家已同意后，经 HTTPS
Dialogue Provider
  ↓ 严格 JSON 候选，仍不可信
Model Adapter + Mechanical Guard
  ↓ FactId / 专名 / 数字 / 否定 / 实时语境检查
Semantic Review Component
  ↓ 独立 Provider 返回每条 Claim 的支持度、矛盾、越权、任务泄露、角色契合和置信度
本地重新核验 Revision + AuthorizationEpoch + ScopeFingerprint
  ↓ 逐 Claim 门禁 + 证据绑定 + 覆盖检查
FinalApprovedText
  ↓
字幕 / 语音 / 行为
```

任何中间文本都不是正式输出。厂商原文只存在于 Provider 结果内部；NPC 的蓝图正式事件只广播 `OnFinalReply`。

## Provider 原理

`AI NPC Provider Subsystem` 是本地网关：

- 接受任意 OpenAI-compatible Chat Completions URL；
- 远程地址必须是 HTTPS，HTTP 只允许显式启用的 loopback；
- 未勾选传输同意、URL/模型/Key 缺失时拒绝请求；
- Key 只放入 HTTP `Authorization` Header，不进入 JSON、配置、存档或日志；
- 对话生成与语义审查是两个逻辑 Channel；默认要求审查通道与对话通道使用不同的 URL 或模型；
- 每个 NPC 可通过 `ProviderProfileId` 使用独立模型、地址和凭据；
- 对话请求支持 `ClientRequestId` 幂等、规范化输入、授权 epoch/作用域指纹校验和过期响应丢弃；
- 严格 JSON 失败可做一次受控格式修复；连接失败、429 和临时服务错误可有限重试；
- 请求和 Session 使用 GUID 关联，NPC 销毁后关闭 Session，迟到结果不会串给另一个 NPC。

## 本地与上传边界

始终留在当前游戏进程：

- API Key；
- 完整本地 Transcript；
- 其他 NPC 的 Session；
- 未授权知识包；
- 存档和审计记录。

每次云调用会发送：

- NPC 公共角色说明；
- 当前 Knowledge View 允许的事实；
- 玩家当前发言；
- 输出格式或审查合同。

因此该模式是“本地持有秘密和会话、云端执行当轮推理”，不是离线推理。完全零上传时应连接 `localhost` 的 OpenAI-compatible 本地模型。

## 失败策略

- Provider 未配置：不联网，返回 `provider_not_configured` 类错误；
- 未同意：拒绝配置；
- 云端失败或超时：不显示原始响应，使用可信 `ErrorText`；
- JSON 不合法或事实越界：机械守卫阻止；
- 语义审查失败：逐 Claim 安全门禁阻止，使用 `BlockedText` / `ErrorText`；
- 证据不属于源 Claim、Claim 覆盖不完整、缺少评分或分数越界：失败关闭；
- 等待期间玩家知识或语境变化：旧响应失效；
- 没有语义审查组件：自由生成文本失败关闭。

## 单机与联机

单机和本地 listen-server 可以使用玩家 BYOK。专用服务器模式不能把客户端 Key 传到服务器；应由项目在服务器进程配置项目自有 Provider。知识授予始终由服务器 Authority 控制，客户端的生成或“已审查”声明不能授予知识。

## 扩展点

- 增加不同厂商协议：在 Provider Subsystem 前增加适配器，不修改 NPC Session 和安全门；
- 本地模型：使用 loopback OpenAI-compatible URL；
- 语音：只消费 `FinalApprovedText`；
- 长期记忆：另建有界本地存储，并明确选择哪些摘要允许随请求发送；当前版本默认不上传 Transcript；
- 正式多人：服务器 Provider、费用限流、审计 Outbox 和隐私政策由目标游戏接管。
