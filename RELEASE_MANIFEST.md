# 0.4.8 交付清单

- 交付日期：2026-11-02
- 目标引擎：Unreal Engine 5.8.4
- 目标平台：Win64
- 状态：单机 AI-NPC 知识授权安全修复版

## 本版内容

- 修复 `ApplyPersona` 调用 `CancelAllPendingReviews()` 导致共享语义审查组件上其他 NPC 的 pending review 被误取消的问题。
- `ApplyPersona` 现在只取消本 NPC 的 ReviewId，并取消对应 Provider HTTP 请求。
- 保留 0.4.7 的安全回归断言和 0.4.6 的单请求取消 API。

## 构建与回归

- UHT `WarningsAsErrors`：通过
- UnrealEditor Win64 Development：通过
- UnrealGame Win64 Development：通过
- UnrealGame Win64 Shipping：通过
- 插件 Automation：20/20，0 失败

## 边界

- 本版按单机作品集验收；联机/专用服务器不在范围内。
- 真实 DeepSeek 并发请求计数未测量；HTTP 级幂等计数来自本地 Mock Provider。
- 尚未执行目标项目 Cook/Package、人工 Play、规模基准和外部宿主输出审计。
- 客户端明文资产不提供反拆包保密；需要保密的正文应由服务器或受控后端保存。
