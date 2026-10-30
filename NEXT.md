---
handoff_schema: tavernweave/next/v1
project_id: ai-npc-knowledge-grant
status: 0.4.3-release-candidate
updated: 2026-10-31
---

# NEXT · ARPG AI NPC 任务知识授权插件

## 当前权威

- 产品边界：仅面向 Unreal Engine 单机/联机 ARPG。
- 当前版本：`0.4.3`，UE 5.8 / Win64。
- 总设计案：`总设计案.md`
- 活动蓝图：`蓝图集/BP-04-简易AI小镇演示.md`
- 执行期持久权威蓝图预算：`0`
- 冻结概念：知识授权属于“当前玩家 × 当前语境”；NPC Receiver 只保存人格反应，不是第三个授权集合。

## 0.4.3 已实现

- Claim 级语义审查：逐条判断事实/非事实、支持度、矛盾、越权、任务泄露、角色契合、置信度和 EvidenceFactIds。
- 审查器证据必须绑定到源 Claim；缺失 Claim、重复 Claim、证据不匹配和分数越界全部失败关闭。
- 对话请求 `ClientRequestId` 幂等、输入哈希、授权 epoch/作用域指纹校验、pending 查询和完成结果回放。
- 每个 NPC 可通过 `ProviderProfileId` 使用独立模型、地址和凭据。
- 严格 JSON 一次格式修复，以及连接失败、429 和临时服务错误的有限重试。
- 确定性授权事实兜底、受约束的非事实角色扮演和人格热切换。
- 删除 Demo 专用共享身份绕过；审查器与对话 Provider 必须独立。
- 自动化扩展到 20 组。

## 保留的 0.4.2 修复

- ARPG 任务核验始终经 `Execute_ResolveCompletedQuest` thunk 调度。
- 对话组件在调用点直接建立语义审查请求与对话请求映射，不依赖临时成员。

## 保留的 0.4.1 安全修复

- loopback 精确主机匹配，拒绝前缀仿冒、userinfo 伪装和十进制 IP 明文 HTTP。
- 授权 State 复制使用 `COND_OwnerOnly`。
- 默认强制独立语义审查。

## 当前验证

- UE 5.8 `BuildPlugin`：Editor、Game Development、Game Shipping 全部通过。
- 插件 Automation：20/20，0 失败。
- 真实 DeepSeek 7 用例：21 条回复，19 条通过，2 条本地守卫兜底，0 条语义审查误拦；未观察到越权泄露或未知事实编造。
- 真实 Provider 测试发现：第三阶段两名非知识接收者进入通用兜底，安全但缺少任务上下文。

## 开放风险

- Demo 宿主级兜底仍可能返回通用地点介绍，需改为“明确不知道该细节”或引用授权事实。
- 真实 Provider 流程尚未复测重复发送和幂等回放；该部分目前由 Automation 覆盖。
- 真实 DeepSeek 测试使用的是本地 Demo 凭据，不随仓库分发。
- 关键剧情仍推荐规范事实模板；语义模型不能提供数学意义上的绝对蕴含证明。
- 尚未执行目标游戏 Cook/Package、人工 Play、正式外部服务器和规模基准。

## 下一道门

修复 Demo 宿主级兜底后复跑同一组 7 条真实 Provider 用例，再进入目标 ARPG 的 Cook/Package、审计持久化和真实部署验收。
