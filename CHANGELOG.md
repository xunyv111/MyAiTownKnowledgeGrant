# Changelog

## Unreleased

- 新增 Claim 级语义审查：逐条判断事实/非事实、支持度、矛盾、越权、任务泄露、角色契合、置信度和 EvidenceFactIds，并保留最差 Claim 诊断。
- 新增 `ClientRequestId` 请求幂等、输入哈希、授权 epoch/作用域指纹校验、pending 查询、已完成结果复用和过期响应丢弃。
- 新增每 NPC `ProviderProfileId` 路由，允许不同 NPC 使用独立模型、地址和凭据。
- 新增严格 JSON 格式修复与有限网络重试；连接失败和 429 可重试，普通 400 与格式错误不盲目重试。
- 新增确定性授权事实兜底、受约束的非事实角色扮演和人格热切换。
- 保留 0.4.2 两项修复：任务核验始终经 `Execute_ResolveCompletedQuest` thunk；语义审查请求在调用点直接配对，不依赖临时成员。
- 自动化扩展到 20 组，新增 `QuestAuthorityBlueprintDispatch`、`SemanticRequestCorrelation`、`RequestIdempotency`、`StaleResponseRejection`、`MultiProviderRouting`、`DialogueFormatRepair`、`DeterministicAuthorizedFallback`、`SemanticClaimGates`。

## 0.4.2 - 2026-10-26

- 修复:`ARPG Knowledge Grant Adapter` 核验任务时改为恒定经接口 thunk(`Execute_ResolveCompletedQuest`)调度;此前原生类实现者会被直接调用 `ResolveCompletedQuest_Implementation`,导致蓝图子类对 `ResolveCompletedQuest` 的覆写被静默跳过,任务核验结果取自原生基线而非项目覆写。
- 修复:`AI NPC Dialogue Component` 发起语义审查后改为在调用点直接建立"审查请求 → 对话请求"映射并转发 Provider;此前依赖仅在同步广播窗口内有效的成员字段路由,多个对话组件共用一个语义审查组件时存在错误配对风险。`OnSemanticReviewRequested` 委托照常广播,自定义转发方不受影响。

## 0.4.1 - 2026-10-24

- 安全:HTTP loopback 豁免从字符串前缀改为精确主机匹配,只接受 `localhost`、`[::1]` 与 `127.0.0.0/8`;`http://localhost.com`、`http://127.0.0.1.evil.tld`、`http://127.0.0.1@evil.tld`、十进制 IP 等仿冒地址一律按远程处理并强制 HTTPS,API Key 不再可能经此类地址明文外发。
- 隐私:`Knowledge Grant Component` 的授权状态复制从 `COND_None` 收紧为 `COND_OwnerOnly`;`ProcessedGrants` 授予历史(内含稳定玩家 ID 与任务进度)不再广播给其他客户端,同时显著降低每次授予的复制流量。
- 实现此前无效的 `bRequireIndependentSemanticReview`(默认开启):语义审查通道与对话通道的 URL 与模型完全相同时,`ConfigureProvider` 拒绝配置并返回 `review_provider_must_be_independent`(双向检查,配置顺序无法绕过);设置面板在强制独立时禁用并取消“共用同一厂商”勾选。
- 自动化:`AiNpcProviderContract` 新增 loopback 前缀仿冒、userinfo 伪装、十进制 IP 拒绝与精确 loopback 放行用例,以及审查通道独立性(同 URL 同模型拒绝、换模型接受、清空凭据)用例。

## 0.4.0 - 2026-10-23

- 新增插件原生运行时 BYOK 设置面板和 `Show AI NPC Provider Settings` 蓝图入口。
- 新增 OpenAI-compatible Provider 网关；远程地址强制 HTTPS，明确同意前不允许发送请求。
- API Key 仅保存在当前 GameInstance 进程内存，不写入配置、存档或日志。
- 新增 `AI NPC Dialogue Component`；每个 NPC 在 BeginPlay 自动建立独立本地 Session，共享 Provider 客户端和凭据。
- 完整本地对话历史不发送给厂商；每轮仅发送公开角色说明、授权事实和当前玩家消息，面板明确披露。
- 新增第二条语义审查 Provider 通道；生成结果必须经过严格 JSON、机械事实守卫和 `FinalApprovedText` 语义门。
- 新增 Provider 合同自动化，覆盖同意、HTTPS、请求字段、密钥隔离和结构化审查响应。

## 0.3.0 - 2026-10-23

- 产品定位收缩为单机/联机 ARPG Runtime 插件，移除泛 RPG 与大型跨服游戏支持表述。
- 新增固定 `QuestId → KnowledgePack` 注册表与服务器 `ARPG Quest Authority Provider` 合同。
- 新增 `ARPG Knowledge Grant Adapter Component`：核验任务完成和玩家归属、拒绝调试/乱序事件、服务器生成 GrantEventId，并广播瞬态审计/持久化通知。
- 新增供应商无关的 `Knowledge Semantic Review Component`：外部 API/本地模型只返回批准、拦截或错误，不能注入替换台词。
- 语义审查等待期间重新验证 Revision、AuthorizationEpoch、实时 Context Tags 和 ScopeFingerprint；超时、错误、覆盖不完整和旧响应失败关闭。
- 明确 `SafeText` 是机械检查中间结果；启用语义审查的正式输出只能消费 `FinalApprovedText`。
- 新增语义审查合同与 ARPG 适配器宿主自动化测试。

## 0.2.1 - 2026-10-22

- Added collector-bound guard entries for raw claims and model JSON, so the live context is read from actual collectors instead of a caller-supplied tag container.
- Restore now ignores invalid but ungranted registry packs; an invalid granted pack still fails closed.
- Added `Revoke Knowledge Pack` with dependent check, event-history removal, and tag re-derivation.
- Added optional `Max Processed Grant Records` / `Max Receipt Records`; overflow fails closed instead of silently dropping history.
- Receiver snapshot restore now broadcasts `On Reaction Memory Restored`.
- Added regression coverage for collector-derived context, revocation, post-revoke snapshots, restore failure radius, and history caps.
- Rebuilt and revalidated against UE 5.7.4; real-level offscreen acceptance rerun 8/8.

## 0.2.0 - 2026-10-22

- 对话输出守卫必须接收当轮实时语境 Tags；旧场景视图以 `dialogue_view_context_changed` 失败关闭。
- 授权状态增加 `AuthorizationEpoch`；快照恢复强制递增 Revision 并换代，恢复前视图全部失效。
- 知识与 Receiver 快照升级到 Schema v3；旧 v1/v2 快照明确拒绝。
- 幂等历史升级为规范化的 `GrantEventId → PackId`；事件 ID 换包显式报错，快照验证事件—授权对应关系。
- 未授权的坏候选包不再拖垮整个对话视图。
- 数字解析保留正负号和小数，书写数字可由宿主词表声明；移除裸“ 不 / 未 ”否定判断，修复受保护短词被长词覆盖后的独立漏报。
- Receiver 快照验证反应范围、语气、行为意图、重复文档和规范化 ID；Receiver 事件换包显式失败。
- `FKnowledgeDialogueGuardResult::Outcome` 区分 Invalid、Allowed、PartiallyAllowed、Blocked；全拦截不再返回 `bOk=true`。
- CRC32 明确定义为一致性校验而非安全签名；“事实门”收紧为“证据引用与元数据一致性门”。
- 明确跨服后端、客户端反拆包、撤销/迁移、有界历史和开放域语义证明不在本插件实现范围。

## Open-source release metadata

- Declared xunyv as author and copyright holder.
- Published the project under the MIT License, with an informational NOTICE and public-repository ignore rules.

## 0.1.0 - 2026-10-22

- 首个可交付候选版本。
- 服务器权威的任务/世界事件知识授予、前置链和幂等事件。
- Gameplay Tag 场景、交互对象与专名语境收集。
- 统一 `FDialogueKnowledgeView`，带修订号与防篡改作用域指纹。
- 原子 Fact 证据门与严格模型 JSON 适配器；可信策略资产提供安全兜底。
- NPC 人格化确定性反应和按 `AudienceId × ReceiptEventId` 隔离的反应记忆。
- Schema v2 玩家知识与 Receiver 快照一致性校验。
- UE 5.7.4 Win64 Editor/Game Development/Shipping 官方 `BuildPlugin` 验证。
