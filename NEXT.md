---
handoff_schema: tavernweave/next/v1
project_id: ai-npc-knowledge-grant
status: delivery-candidate
updated: 2026-10-23
---

# NEXT · ARPG AI NPC 任务知识授权插件

## 当前权威

- 产品边界：仅面向 Unreal Engine 单机/联机 ARPG。
- 总设计案：`总设计案.md`
- 活动蓝图：`蓝图集/BP-04-简易AI小镇演示.md`
- 执行期持久权威蓝图预算：`0`
- 冻结概念：知识授权属于“当前玩家 × 当前语境”；NPC Receiver 只保存人格反应，不是第三个授权集合。

## 0.4.0 已实现

- 保留原有 Authority 授权、因果前置、语境收集、知识视图、Schema v3 快照、复制、撤销和人格反应能力。
- 新增 `UArpgKnowledgeGrantRegistry`，固定 `QuestId → KnowledgePack`，并绑定 RegistryVersion。
- 新增 `IArpgQuestAuthorityProvider`，由服务器任务系统按玩家 Owner 返回完成状态、稳定玩家 ID、完成修订和调试事件标记。
- 新增 `UArpgKnowledgeGrantAdapterComponent`：拒绝未完成、调试、非法身份和乱序事件；服务器生成幂等 GrantEventId；所有结果进入有界瞬态审计队列并广播持久化/Outbox 信号。
- 新增 `UKnowledgeSemanticReviewComponent`：宿主可连接独立语义 API、本地模型或自建服务，插件本身不引入网络或厂商 SDK。
- 外部审查器只能批准、拦截或报错，不能返回替换台词；批准必须覆盖全部机械允许 ClaimId。
- 异步响应返回后重新验证玩家 Revision、AuthorizationEpoch、实时 Context Tags 和 ScopeFingerprint；超时、取消、错误、覆盖不完整和旧响应全部失败关闭。
- `SafeText` 明确降级为机械中间结果；启用语义审查后正式出口只能消费 `FinalApprovedText`。
- 新增玩家运行时 BYOK 面板、进程内 Key、OpenAI-compatible Provider 网关和每 NPC 自动隔离 Session。
- 本地 Transcript 默认不上传；远程请求必须由玩家勾选同意，且只发送角色说明、当轮授权事实和当前发言。
- 新增 `Docs/ARPG_INTEGRATION.md` 与 `Docs/SEMANTIC_REVIEW_API.md`。

## 当前接入链

`服务器任务系统 → ARPG Quest Authority Provider → ARPG Knowledge Grant Adapter → 固定 Registry → Knowledge Grant Component`

`Volume/交互/专名 → Knowledge Context Collector → Build Dialogue Knowledge View → 模型严格 JSON → 机械 Guard → 独立语义审查 → 只消费 FinalApprovedText`

## 当前验证

- 已新增 `Docs/OPERATION_MINDMAP.md` 与 `Docs/images/operation-principle-mindmap.svg`，用一张思维导图串联安装、Authority 授予、多米诺前置、语境输入门、模型候选、双重输出门、NPC 反应和 Schema v3 存档边界。
- 已新增 `Docs/BEGINNER_GUIDE.md`，从安装工具链、Gameplay Tags、Knowledge Pack 和守卫策略资产开始，逐步覆盖玩家/NPC 组件、Blueprint 授予、Context Collector、Provider、严格 JSON、人格反应、Schema v3 存档、多人边界、首条验收流程和错误排查；README 已添加新手入口。
- 0.3.0 隔离 `RunUAT BuildPlugin`：UHT WarningsAsErrors、Editor Development、Game Development、Game Shipping 全部成功。
- 插件 Automation：11/11，0 警告、0 失败；新增 `Runtime.SemanticReviewContract` 通过。
- 隔离宿主增量编译成功；新增 `KnowledgeGrantHost.ARPG.AdapterFlow` 通过，覆盖任务未完成拒绝、固定注册表授予、玩家 Owner 绑定、幂等、修订乱序拒绝、调试事件拒绝和审计记录。
- 最终短路径隔离 `RunUAT BuildPlugin` 已从当前源码生成发布包，UHT WarningsAsErrors、Editor Development、Game Development 和 Game Shipping 全部通过。
- 最终发布包对应的插件 Automation 11/11；宿主 Automation 5/5，其中网络 PIE 1 项保留 36 条 UE NetGUID 框架警告，业务错误 0。
- 静态验证通过；0.2.1 保存关卡离屏验收 8/8 保留为历史基线，0.3.0 没有冒充重跑该人工/真实关卡门。

## 开放风险

- 语义接口已提供，但真实 API、本地模型、认证、熔断、费用和服务 SLA 属于目标 ARPG 宿主；当前自动化使用结构化模拟响应。
- 关键剧情仍推荐规范事实模板；独立语义模型不能提供数学意义上的绝对蕴含证明。
- `OnGrantAudited` 是持久化/Outbox 接口，不是内置事务数据库；目标 ARPG 必须接入自己的 SaveGame 或服务端持久层。
- 尚未在 0.3.0 执行目标游戏 Cook/Package、有人值守人工 Play、正式外部服务器和规模基准。
- 客户端明文资产不具备反拆包保密能力。

## 下一道门

进入目标 ARPG 的 Cook/Package、正式任务系统接入、审计 Outbox 持久化和真实语义服务验收；这些宿主门通过前不得标记生产就绪。

## 一句续接

0.3.0 已完成纯 ARPG 定位、服务器任务适配器、独立语义审查合同及最终回归；从目标 ARPG 的任务源、Outbox、真实审查服务与 Cook/Package 验收继续。
