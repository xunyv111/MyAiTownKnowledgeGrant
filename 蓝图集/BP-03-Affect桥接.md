---
blueprint_schema: tavernweave/domain-blueprint/v1
project_id: ai-npc-knowledge-grant
blueprint_id: BP-03
parent_id: BP-ROOT
status: candidate
updated: 2026-10-20
---

# BP-03 · Affect Core 可选桥接

## 输入与边界

人格反应输出的有界 `SuggestedValenceDelta` 与 `SuggestedArousalDelta`。本蓝图只负责可选适配，不让知识插件硬依赖 `AIaffect-core`。

## 输出与证据

候选输出为独立桥接组件及集成测试；当前 BP-02 已提供与 Affect Core 上限兼容的建议量，但尚未创建硬连接。

## 停止条件

目标项目使用其他情绪系统，或驾驶员不批准增加插件依赖。

## 下一道门

待 BP-02 通过并确认目标项目确实启用 `AIaffect-core` 后再决定是否激活。
