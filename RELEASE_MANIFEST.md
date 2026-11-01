# 0.4.7 交付清单

- 交付日期：2026-11-01
- 目标引擎：Unreal Engine 5.8.4
- 目标平台：Win64
- 状态：单机 AI-NPC 知识授权安全验证版

## 本版内容

- 新增安全回归断言：
  - Policy 丢失时失败关闭为 `semantic_review_policy_unavailable`
  - Review Provider 重新配置/清除凭据推进 Provider Generation
  - 单请求取消只影响目标审查
  - Uncited Roleplay 独立长度/Claim 数量上限
  - 100 次同 `ClientRequestId` 幂等
- 单机 Demo HTTP 级幂等验证：100 次同 ID 仅产生 1 次 dialogue + 1 次 review POST。
- 宿主输出审计：`KnowledgeGroupChatHost` 无 `RawProviderPayload` 正式 UI 接线。
- 不新增玩法功能。

## 构建与回归

- UHT `WarningsAsErrors`：通过
- UnrealEditor Win64 Development：通过
- UnrealGame Win64 Development：通过
- UnrealGame Win64 Shipping：通过
- 插件 Automation：20/20，0 失败
- 静态结构验证与发布合同检查：通过

## 边界

- 本版按单机作品集验收；联机/专用服务器不在范围内。
- 真实 DeepSeek 并发请求计数未测量；HTTP 级幂等计数来自本地 Mock Provider。
- 尚未执行目标项目 Cook/Package、人工 Play、规模基准和外部宿主输出审计。
- 客户端明文资产不提供反拆包保密；需要保密的正文应由服务器或受控后端保存。
