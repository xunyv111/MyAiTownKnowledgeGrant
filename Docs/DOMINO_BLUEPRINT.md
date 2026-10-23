# 多米诺知识系统蓝图

这份蓝图把插件看成一套“不会提前倒下的多米诺系统”。它不要求你先懂 C++：先按编号看总览，再阅读四张分解图。

## 30 秒总览

```mermaid
flowchart TD
    A["① 任务或世界事件真实完成"] --> B["② 服务器 Authority 授予知识包"]
    B --> C["③ Required Pack Ids 推进多米诺知识链"]
    C --> D["④ Context Collector 收集当前语境"]
    D --> E["⑤ Build Dialogue Knowledge View"]
    E --> F["⑥ AI 返回严格 JSON 候选"]
    F --> G["⑦ 机械 Guard"]
    G --> H["⑧ 独立 Semantic Review"]
    H --> I["⑨ FinalApprovedText"]
    I --> J["字幕 · 语音 · 行为 · 正式日志"]

    B -. "旁路：已授权包" .-> K["NPC 人格反应"]
    B -. "持久化" .-> L["Schema v3 Snapshot"]
```

一句话理解：**服务器决定哪块骨牌可以倒下，语境决定本轮照亮哪几块，双重守卫决定最终哪句话可以离开系统。**

## 图例

| 图形或连线 | 含义 |
|---|---|
| 圆角节点 | 一次事件、一个组件或一项结果 |
| 菱形节点 | 必须通过的判断门 |
| 实线箭头 | 正常执行顺序或数据流 |
| 虚线箭头 | 旁路通知、人格反应或存档，不会反向授予知识 |
| 红色失败节点 | 失败关闭：停止流程，不泄漏模型原文或未来事实 |

## 第一张蓝图：服务器如何推倒知识骨牌

对应 Unreal 中的任务完成事件、`ARPG Knowledge Grant Adapter Component`、固定 Registry 和玩家的 `Knowledge Grant Component`。

```mermaid
flowchart TD
    Q(["任务完成 / 世界事件完成"]) --> A{"Owner 有 Authority?"}

    A -- "否" --> F1["停止：authority_required"]
    A -- "是" --> B["ARPG Knowledge Grant Adapter<br/>核验任务完成、玩家归属和完成修订"]

    B --> C{"任务状态有效?"}
    C -- "否" --> F2["停止：quest_not_completed"]
    C -- "是" --> D["Knowledge Grant Registry<br/>QuestId → KnowledgePack"]

    D --> E{"Registry 与知识包匹配?"}
    E -- "否" --> F3["停止：registry_pack_mismatch"]
    E -- "是" --> G["生成稳定 Grant Event Id<br/>PlayerId : QuestId : CompletionRevision"]

    G --> H["Knowledge Grant Component<br/>Grant Knowledge From Quest / World Event"]
    H --> I{"Required Pack Ids 都已授权?"}

    I -- "否" --> F4["停止：knowledge_prerequisite_missing"]
    I -- "是" --> J{"Grant Event Id 是否用过?"}

    J -- "同事件、同知识包" --> K["幂等成功：不重复授予"]
    J -- "同事件、不同知识包" --> F5["停止：grant_event_id_conflict"]
    J -- "全新事件" --> L["写入 Granted Packs<br/>Revision + 1"]

    classDef fail fill:#3b1015,stroke:#ef4444,color:#fecaca;
    class F1,F2,F3,F4,F5 fail;
```

### 这一段每个部位做什么

| 多米诺比喻 | 插件部位 | 作用 |
|---|---|---|
| 推骨牌的手 | 服务器 Authority | 只有服务器能改变玩家知识状态 |
| 开始推动 | 任务/世界事件完成 | 表示事实已经在游戏世界中真实发生 |
| 固定轨道 | Knowledge Registry | 固定任务与知识包的对应关系 |
| 每次推动的编号 | Grant Event Id | 防止网络重试造成重复授予或事件换包 |
| 轨道管理员 | Knowledge Grant Component | 保存已授权知识，并检查前置、幂等和冲突 |

## 第二张蓝图：多米诺前置链如何阻止剧透

每个 `Knowledge Pack` 只保存当前阶段已经发生的原子事实。未来结论必须放进后续知识包。

```mermaid
flowchart TD
    P1["Pack.PublicRumor<br/>公共传闻"]
    P2["Pack.HarborFootprint<br/>港口脚印"]
    P3["Pack.ManorConnection<br/>庄园关联"]
    P4["Pack.WitnessStatement<br/>证人证词"]
    P5["Pack.FinalConfirmation<br/>最终确认"]

    P1 -- "Required Pack Id" --> P2
    P2 -- "Required Pack Id" --> P3
    P1 -- "Required Pack Id" --> P4
    P3 -- "Required Pack Id" --> P5
    P4 -- "Required Pack Id" --> P5

    X(["尝试直接授予最终确认"]) --> CHECK{"P3 与 P4 都已授权?"}
    CHECK -- "否" --> BLOCK["拒绝授予<br/>不会提前泄漏最终真相"]
    CHECK -- "是" --> P5

    classDef fail fill:#3b1015,stroke:#ef4444,color:#fecaca;
    class BLOCK fail;
```

这张图包含两种关系：

- **直线推进**：公共传闻 → 港口脚印 → 庄园关联。
- **分支汇合**：只有庄园关联和证人证词都获得后，最终确认才能倒下。

## 第三张蓝图：NPC 对话为什么不会直接相信 AI

知识已经授权，不代表任何场景都能使用。插件先求“已授权知识 ∩ 当前语境”，再把模型候选送过两道输出门。

```mermaid
flowchart TD
    S1(["进入场景 Trigger Volume"]) --> C["Knowledge Context Collector"]
    S2(["开始与 Actor 交互"]) --> C
    S3(["对话明确提到专名"]) --> C

    G["玩家 Granted Packs"] --> V["Build Dialogue Knowledge View"]
    C -- "Active Context Tags" --> V
    P["候选 Knowledge Packs"] --> V

    V --> VIEW["FDialogueKnowledgeView<br/>Allowed Documents + Facts<br/>Revision + Epoch + Fingerprint"]
    VIEW --> M["AI NPC Provider<br/>只发送允许事实、角色说明和当前发言"]
    M --> JSON["严格 JSON<br/>claims[] + evidenceFactIds[]"]

    JSON --> GUARD["Guard Knowledge Model Json Response For Collectors"]
    VIEW --> GUARD
    C -- "重新读取实时语境" --> GUARD

    GUARD --> M1{"JSON、FactId、人物、数字、否定<br/>全部机械一致?"}
    M1 -- "否" --> F1["失败关闭<br/>不显示模型原文"]
    M1 -- "是" --> SAFE["机械结果 SafeText<br/>仍不是自由生成台词的正式出口"]

    SAFE --> REVIEW["Begin Semantic Review"]
    REVIEW --> M2{"语义批准且 Revision、Epoch、Context<br/>仍与原视图一致?"}
    M2 -- "否" --> F2["失败关闭<br/>拒绝超时、旧视图或语境变化"]
    M2 -- "是" --> FINAL["FinalApprovedText"]

    FINAL --> OUT["字幕 · 语音 · 行为树 · 正式日志"]

    classDef fail fill:#3b1015,stroke:#ef4444,color:#fecaca;
    class F1,F2 fail;
```

### 两道门的区别

| 输出门 | 检查内容 | 不能替代什么 |
|---|---|---|
| Mechanical Guard | JSON 结构、证据 FactId、专名、数字、否定关系、实时语境 | 不能完整理解开放域自然语言含义 |
| Semantic Review | 候选句子是否真正得到允许事实支持 | 不能授予新知识，也不能返回替换台词 |

自由生成台词的正式消费端只能读取 `FinalApprovedText`。`SafeText` 是机械检查中间结果。

## 第四张蓝图：NPC 反应和存档为什么是旁路

NPC Receiver 保存的是“NPC 听到消息后的反应”，不是第三套知识授权。

```mermaid
flowchart TD
    PACK["已授权 Knowledge Pack"] --> PLAYER["玩家 Granted Packs"]
    PACK -. "延迟接收" .-> RECEIVER["Npc Knowledge Receiver Component"]

    PROFILE["Npc Personality Profile"] --> REACT["Evaluate Knowledge Reaction"]
    AFFECT["当前 Affect Context"] --> REACT
    RECEIVER --> REACT

    REACT --> RESULT["兴趣 · 威胁感 · 台词语气<br/>行为意图 · 情绪建议"]
    RESULT --> MEMORY["AudienceId × ReceiptEventId<br/>反应记忆"]

    PLAYER --> PSNAP["Player Snapshot<br/>Schema v3"]
    MEMORY --> RSNAP["Receiver Snapshot<br/>Schema v3"]

    PSNAP --> RESTORE{"Registry、前置闭包、事件历史<br/>和 Schema 全部有效?"}
    RESTORE -- "是" --> NEWSTATE["恢复知识状态<br/>Revision 递增<br/>生成新 AuthorizationEpoch"]
    RESTORE -- "否" --> FAIL["拒绝恢复<br/>保留恢复前状态"]

    MEMORY -. "绝不反向授予事实" .-> PLAYER

    classDef fail fill:#3b1015,stroke:#ef4444,color:#fecaca;
    class FAIL fail;
```

虚线“绝不反向授予事实”是关键边界：NPC 可以表现得惊讶、谨慎或愤怒，但这些反应不能让玩家或其他 NPC 凭空获得知识。

## 新手照着接的最短顺序

1. 在玩家 Actor 或 PlayerState 上挂 `Knowledge Grant Component`。
2. 创建最少两个 `Knowledge Pack`：公共前提和后续发现。
3. 在后续包的 `Required Pack Ids` 中填写前置包 ID。
4. 任务完成后只在 Authority 侧调用授予节点。
5. 在玩家或对话协调 Actor 上挂 `Knowledge Context Collector Component`。
6. 对话前构建 `FDialogueKnowledgeView`。
7. 只把 `Allowed Documents` 和 `Facts` 发送给模型。
8. 模型返回严格 JSON 后先过 Mechanical Guard，再过 Semantic Review。
9. 字幕、语音和行为系统只消费 `FinalApprovedText`。
10. 最后再接 NPC Receiver 和 Schema v3 存档，不要把它们误当知识授权门。

## 常见接错位置

| 接错方式 | 为什么错 | 正确做法 |
|---|---|---|
| 把 Knowledge Grant Component 挂在每个 NPC 上 | 会把玩家知识拆成大量重复副本 | 挂在持久玩家 Actor 或 PlayerState |
| 客户端自己调用授予 | 客户端不能作为任务完成权威 | 由服务器任务系统或 Adapter 调用 |
| 一个包同时写公共前提和最终凶手 | 第一块倒下时就会剧透 | 按发生顺序拆成多个包 |
| 只根据 Context Tags 解锁知识 | 场景标签只能选择，不能授予 | 必须先完成任务授权，再进行语境筛选 |
| 直接显示模型文本 | 模型候选仍不可信 | 只显示 `FinalApprovedText` |
| 把 NPC 反应记忆当知识状态 | Receiver 只记录人格反应 | 玩家授权仍以 Knowledge Grant Component 为准 |

继续操作请阅读[新手从零接入教程](BEGINNER_GUIDE.md)；完整组件边界见[架构说明](ARCHITECTURE.md)。
