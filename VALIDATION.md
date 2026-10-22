# 验证记录

> 本仓库只包含插件 10 项 Automation 测试。宿主演示与网络 PIE 测试由独立 Demo 仓库维护，下表中的相关结果作为 0.2.1 跨仓库证据保留。



验证日期：2026-10-22  
真实宿主：Unreal Engine 5.7.4，WindowsEditor / Win64 Development，NullRHI

## 本轮结论

0.2.1 / Schema v3 已通过真实 UE 编译与自动化：插件 10/10、演示宿主 2/2、专用服务器/双客户端 PIE 1/1。严格统计为 **13 个用例执行，12 个无警告成功，1 个带 36 条 NetGUID 框架警告成功，0 失败，0 未运行**。网络警告来自 UE 临时 PIE Level，业务断言错误为 0；数量与逐项结果由 UE JSON 报告生成。

0.2.1 已重新运行保存关卡 `/Game/Maps/KnowledgeAcceptance` 的纯后台离屏验收。`UnrealEditor-Cmd -game -RenderOffscreen` 实际载入该关卡，移动持久化碰撞 Probe 穿过场景 Trigger 与交互范围；8/8 检查通过，进程退出码 0，并生成新的 JSON、日志及锁定/解锁截图。该证据证明当前版本的真实接线回归，但不冒充人工 Play。

## 构建

- 官方插件打包：`RunUAT BuildPlugin` 使用纯 ASCII 临时 HostProject，从隔离副本完成 `UnrealEditor Win64 Development`、`UnrealGame Win64 Development` 与 `UnrealGame Win64 Shipping`，UHT 均启用 `WarningsAsErrors`；AutomationTool 输出 `BUILD SUCCESSFUL`，退出码 0。
- `KnowledgeGrantHostEditor Win64 Development`：在 ASCII 临时副本完成首次全量构建（32 个动作，其中一处测试编译错误修正后）；最终增量构建 5 个动作并链接成功，插件、宿主和全部测试源码均参与编译。
- 工作区直接构建先被同名旧 Release 插件重复发现阻止；临时禁用旧描述符后进入 UHT，又命中已知中文路径 PCH 问题。两次均未被当作代码失败，描述符已恢复，最终 ASCII 隔离构建成功。

原工作区含中文路径，真实编译使用 ASCII 临时验证副本；机器相关原始报告与日志不随公开仓库分发。没有修改用户的 UE 全局设置。

## 自动化测试

插件集合 `MyAiTown.KnowledgeGrant`：

| 测试 | 结果 | 警告 | 错误 |
|---|---|---:|---:|
| `Reaction.DelayedReceipt` | Success | 0 | 0 |
| `Reaction.PersonalityAndAffect` | Success | 0 | 0 |
| `Runtime.CausalChain` | Success | 0 | 0 |
| `Runtime.ContextCollector` | Success | 0 | 0 |
| `Runtime.ContextTagLibraries` | Success | 0 | 0 |
| `Runtime.DialogueGuard` | Success | 0 | 0 |
| `Runtime.ModelAdapterContract` | Success | 0 | 0 |
| `Runtime.QuestFlow` | Success | 0 | 0 |
| `Runtime.SnapshotConsistency` | Success | 0 | 0 |
| `Runtime.Validation` | Success | 0 | 0 |

宿主演示集合 `KnowledgeGrantHost.Demo`：

| 测试 | 结果 | 警告 | 错误 |
|---|---|---:|---:|
| `Demo.Flow` | Success | 0 | 0 |
| `Demo.StaggeredSpread` | Success | 0 | 0 |

网络 PIE 集合 `KnowledgeGrantHost.Network`：

| 测试 | 结果 | 警告 | 错误 |
|---|---|---:|---:|
| `AuthorityAndPlayerIsolation` | Success | 36 | 0 |

`Runtime.ModelAdapterContract` 明确覆盖：合法严格 JSON 放行；畸形 JSON、未知根字段、空 FactId、重复 FactId、遗漏受保护实体和超长文本失败关闭；额外数字与否定反转在解析后由 FactId 守卫拒绝；原始错误载荷不会进入 `SafeText`。适配器没有向 Blueprint 提供“只解析不守卫”的公开节点。

`Runtime.ContextCollector` 明确覆盖：多个重叠场景来源取并集；同一来源更新时替换旧标签；退出一个 Volume 不清除其他 Volume；切换交互对象后，旧对象迟到的 End 事件不能清除新对象；显式专名与场景/交互叠加；会话重置清空所有来源。收集器只选择信息库，不调用任何知识授予 API。

`Runtime.SnapshotConsistency` 明确覆盖：匹配注册表版本的合法 Schema v3 快照可恢复且授权标签由知识包重新派生；注册表版本不匹配、伪造额外标签、缺失前置包、未注册包、循环依赖、缺失事件历史和事件指向未授权包均失败关闭；无效但未授权的注册包不再阻塞合法恢复，非法已授权包仍失败关闭。失败恢复从已授予一个包的非默认状态开始，验证包、Revision 和事件历史均保持不变。恢复携带相同 Revision 的不同合法状态时，组件强制递增 Revision，旧视图失败关闭。撤销叶子包会移除其授权、历史并重派生 Tags；撤销后的快照可恢复；仍有依赖者时撤销失败。

`Runtime.ContextTagLibraries` 现在覆盖 Collector 绑定的输出守卫：从实际收集器读取实时 Tags 放行匹配视图；收集器切换场景后拒绝旧视图；没有有效收集器时返回 `context_collector_missing`。`Runtime.CausalChain` 覆盖依赖保护、撤销后事实减少、撤销后缺少前置不能重授予，以及可选 `MaxProcessedGrantRecords` 上限失败关闭。`Reaction.DelayedReceipt` 覆盖可选 `MaxReceiptRecords` 上限。

`AuthorityAndPlayerIsolation` 使用 UE CQTest 实际启动一台专用服务器和两个 PIE 客户端。服务端只向 Player A 的 `AKnowledgeDemoPlayerState` 授予知识，两个客户端复制收敛后均确认 A 有该包、B 没有；随后每个客户端在 Player B 组件上直接调用授予和快照恢复，两者都返回 `authority_required`，修订号不变。36 条警告均为 CQTest 临时 `/Temp/UEDPIE_*` Level 的 `FNetGUIDCache` / `RegisterNetGUID_Client` 日志，没有业务错误或断言失败。

`Demo.Flow` 新增可见状态回归：清空后两个公开语气 Getter 均返回 `None`，三名 NPC 观察器均显示 `memory=LOCKED | no reaction`；恢复后，授权、Receiver 记忆以及清空前两种人格语气全部返回。可见反应从已保存的 Receiver 快照重建，而不是依赖旧的 `LatestReactions` 残留。

`Reaction.DelayedReceipt` 覆盖 Receiver Authority 负向、受众隔离、ID 首尾空白规范化、同一受众事件换包冲突、Schema v3 恢复和伪造越界反应值失败关闭。玩家 A 与 B 使用相同 `ReceiptEventId` 时仍拥有各自记录；旧 Schema v2 快照被明确拒绝。

`Runtime.DialogueGuard` 覆盖锁定事实、错专名、独立短词被长词覆盖后的漏报、正负号反转、小数、宿主声明的中文书写数字、否定反转与“不错”误判回归、可信策略兜底、`AllOrNothing` / `PerClaim`、陈旧视图和一致性校验值改写。`Runtime.ContextTagLibraries` 另覆盖“同 Revision 下切换场景后复用旧视图”，返回 `dialogue_view_context_changed`。演示保留罗文的错误候选，统一出口将其替换为可信的“证据不足”文本。

## 机器证据

公开仓库只保留与机器无关的验证摘要：[Docs/VERIFICATION.md](Docs/VERIFICATION.md)。原始 UE JSON 报告、日志和截图包含本机路径与主机信息，未随公开仓库分发；发布者在 Release 资产中单独保存证据包。

本轮直接读取 UE JSON 报告并核对日志末尾 `TEST COMPLETE. EXIT CODE`：12 succeeded、1 succeeded with warnings、0 failed、0 not run，测试进程退出码为 0。

## 已证实的运行结果

- 玩家只获得已发生的因果事实；未来事件、道具和结果不会因场景标记提前授权。
- `FDialogueKnowledgeView` 固化玩家修订、授权世代、语境、允许文档及 CRC32 一致性校验；输出门精确比较当轮实时语境。CRC32 不作为安全签名。
- `UKnowledgeContextCollectorComponent` 可以从重叠场景、当前交互对象和显式专名汇总 Tags；使用弱引用跟踪来源，不会因已销毁 Actor 留下激活信息。
- 快照 Schema v3 绑定注册表版本与 `EventId → PackId` 历史；恢复时验证授权包、事件对应、前置闭包和循环依赖，并重新派生 Tags、递增 Revision、生成新授权世代。
- 真实专用服务器/双客户端 PIE 中，授权状态按 PlayerState 组件独立复制；玩家 A 的包不会串到玩家 B，客户端不能直接授予或恢复。
- 保存并清空会同时重置 Receiver 记忆、`LatestReactions` 和场景 NPC 文字；恢复成功后可见人格反应由 Receiver 快照重新构建。
- Receiver reaction receipt 以稳定 `(AudienceId, ReceiptEventId)` 为键；共享 NPC 面向不同玩家的反应和幂等状态彼此隔离，但仍不参与知识授权。
- 模型输出根对象只能包含 `claims`；claim 只能包含 `claimId`、`text`、`evidenceFactIds`、`claimedEntities`、`claimedNumbers`、`negated`，并受固定长度和数量上限约束。
- 模型 JSON 接入可直接调用整合入口取得 `FKnowledgeDialogueGuardResult`；字幕、语音、行为和日志使用 `SafeText`。插件没有提供“把 JSON 解析成原始 claims 后直接返回”的 parser-only 蓝图出口；已有结构化 claim 的宿主仍可显式调用底层事实守卫。
- 演示实测：罗文 `urgent / investigate`，米拉 `fearful / seek_safety`，芬恩 `fearful / investigate`；同一消息仍结合人格产生不同反应。
- Receiver 仍仅保存“NPC 收到消息后的反应记忆”，不参与知识授权；公式保持“玩家授权 ∩ 当前语境 ∩ 输出证据校验”。

## 仍未覆盖或未修复

- 输出门是“证据引用与元数据一致性门”，不证明开放域文本完整蕴含引用事实；正确证据后夹带词表外断言、谓词偷换、隐喻和代词指代仍可能通过。
- 插件无法阻止宿主故意绕开统一入口，把模型原文直接送到自建字幕、语音、行为或日志系统；接入审查仍是必要边界。
- 知识与 Receiver 的 v1/v2 快照均被明确拒绝；自动迁移尚未实现，宿主升级时需要显式迁移工具、重新生成存档或清空非授权反应记忆。
- `ProcessedGrants` / Receiver receipts 尚无滚动窗口，也没有 revoke/rollback；长会话仍需宿主有界保留与迁移策略。
- 未实现跨服授权账本、玩家换服加载、服务端正文存储、NPC 全局持久身份或客户端资产反拆包；当前是 MMO 宿主可集成的 Runtime 基础件，不是 MMO 后端。
- 尚未在人工 Play 中把真实关卡 Trigger Volume 蓝图连接到收集器；当前证据覆盖 Runtime 组件、来源切换契约和演示 C++ 接线。
- 已完成保存关卡中真实 Trigger/交互 Actor 的离屏后台接线验收；尚未做有人值守的人工键鼠体验验收。
- 1 万 NPC / 千包基准尚未完成。
- 网络测试当前使用 CQTest 的临时 PIE World，因此报告保留 36 条 UE NetGUID 框架警告；尚未在项目保存的正式多人关卡或外部独立进程专用服务器上复验。
- 未执行 Cook/Package，也未进行人工 Play 画面验收；当前新增证据是保存关卡、真实碰撞和 RenderOffscreen 截图，截图三维背景因关卡未布置正式灯光而为黑色，HUD 状态可见。






