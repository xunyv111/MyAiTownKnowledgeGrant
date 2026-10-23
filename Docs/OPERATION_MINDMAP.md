# 操作原理思维导图

这张图把插件从“装进项目”到“NPC 最终说出一句可用台词”的完整链路放在同一张画布上。阅读顺序是：先看中心目标，再沿编号 `1 → 8` 展开；底部是一次完整运行时流程。

![My AI Town Knowledge Grant 操作原理思维导图](images/operation-principle-mindmap.png)

[查看可缩放的 SVG 原图](images/operation-principle-mindmap.svg)

## 最短使用路径

1. 把插件放入项目 `Plugins/MyAiTownKnowledgeGrant/`，启用插件并编译。
2. 把 `Knowledge Grant Component` 放在持久玩家 Actor 或 PlayerState；联机项目只允许服务器 Authority 授予。
3. 用 `Knowledge Pack` Data Asset 拆分公共前提、现场发现和最终结论，通过 `Required Pack Ids` 串成多米诺依赖链。
4. 任务或世界事件真实完成后，调用 `Grant Knowledge From Quest` 或 `Grant Knowledge From World Event`；正式 ARPG 推荐通过固定 Registry 和 Adapter 授予。
5. 对话前由 `Knowledge Context Collector` 汇总场景、交互对象和明确提及的专名，再调用 `Build Dialogue Knowledge View`。
6. 只把视图中的允许事实发送给模型；模型必须返回带 `evidenceFactIds` 的严格 JSON。
7. 候选先经过机械 Guard，再经过独立 Semantic Review。字幕、语音、行为树和正式日志只能消费 `FinalApprovedText`。
8. NPC 的人格反应由 `Npc Knowledge Receiver Component` 延迟生成和保存；它不是知识授权门，不能让 NPC 绕过玩家授权获得事实。

## 多米诺骨牌原则

```text
公共前提
  └─现场观察（Requires: 公共前提）
      └─关联调查（Requires: 现场观察）
          └─最终确认（Requires: 关联调查）
```

每块骨牌只包含已经发生的原子事实。后续事实不能提前放进前置包；乱序授予返回 `knowledge_prerequisite_missing`。`GrantEventId` 必须稳定且唯一，同一事件换包会返回 `grant_event_id_conflict`。

## 两道知识门

- 输入门：`玩家已授权资料 ∩ 当前语境激活资料`，输出本轮唯一的 `FDialogueKnowledgeView`。
- 输出门：严格 JSON → 机械事实守卫 → 独立语义审查 → `FinalApprovedText`。
- 失败关闭：视图过期、语境变化、授权世代变化、证据不足、JSON 非法、审查超时或拒绝时，都不得显示模型原文。

## 单机与联机边界

- 单机或本地 listen-server：可以使用玩家 BYOK，Key 仅驻当前进程内存。
- 正式专用服务器：不要向客户端索取或复制玩家 Key；由服务器配置项目自己的 Provider、审计与费用策略。
- 存档：玩家知识和 NPC 反应分别使用 Schema v3 快照；恢复时校验 Registry、前置闭包、事件历史和授权世代。

## 对应蓝图节点

```text
任务完成 / 世界事件完成
  → Grant Knowledge From Quest / World Event
  → Build Dialogue Knowledge View
  → 模型返回严格 JSON
  → Guard Knowledge Model Json Response For Collectors
  → Begin Semantic Review
  → FinalApprovedText
  → 字幕 / 语音 / 行为 / 正式日志
```

第一次接入请从 [`BEGINNER_GUIDE.md`](BEGINNER_GUIDE.md) 开始。完整安装检查见 [`../INSTALL.md`](../INSTALL.md)，AI Provider 接入见 [`AI_NPC_PROVIDER.md`](AI_NPC_PROVIDER.md)，ARPG 任务适配见 [`ARPG_INTEGRATION.md`](ARPG_INTEGRATION.md)。

