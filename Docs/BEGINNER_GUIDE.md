# 新手从零接入教程

这份教程面向第一次接触 Unreal Engine 插件、Gameplay Tags、Data Asset 或 AI-NPC 的使用者。你不需要先读 C++ 源码。按顺序完成每一节，并在每节末尾核对“成功标志”。

建议先打开[操作原理思维导图](OPERATION_MINDMAP.md)，再看[多米诺知识系统蓝图](../DOMINO_BLUEPRINT.md)。思维导图负责建立整体印象，蓝图负责拆解节点和失败分支，本教程负责告诉你在编辑器里点哪里、填什么、怎样判断接对了。

## 1. 先理解插件负责什么

插件解决三件事：

1. 玩家完成任务后，服务器给该玩家解锁一组已经发生的事实。
2. NPC 对话开始时，插件按当前场景和交互对象筛选本轮允许使用的事实。
3. AI 返回台词后，插件检查事实引用，再交给独立语义审查。通过后的 `FinalApprovedText` 才能进入字幕、语音和行为系统。

插件不替你制作任务系统、存档界面、对话 UI、模型服务或正式服务器。它提供可由 Blueprint 调用的知识授权与输出守卫。

### 1.1 你会接触的五类对象

| 对象 | 放在哪里 | 用途 |
|---|---|---|
| `Knowledge Pack` | Content Browser 中的 Data Asset | 保存一组已经发生的事实，以及前置知识包 |
| `Knowledge Grant Component` | 玩家 Actor 或 PlayerState | 记录当前玩家已经解锁哪些知识包 |
| `Knowledge Context Collector Component` | 玩家、PlayerState 或对话协调 Actor | 收集场景、交互对象和专名 Tags |
| `AI NPC Dialogue Component` | 每个需要 AI 对话的 NPC | 建立独立本地会话并驱动完整安全链 |
| `Npc Knowledge Receiver Component` | 需要人格化反应的 NPC | 保存该 NPC 第一次得知消息时的确定性反应 |

`Knowledge Grant Component` 决定玩家能给对话提供哪些事实。`Npc Knowledge Receiver Component` 只记录 NPC 的反应，不会授予知识。

## 2. 按三个阶段学习

新手先分阶段验证，不要第一天就接入任务、模型、存档和联机。

### 阶段 A：知识授权

你需要完成：

- 创建两个或三个 `Knowledge Pack`；
- 把它们串成前置链；
- 在 Authority 端依次授予；
- 用 `Has Knowledge Pack` 或构建知识视图检查结果。

这一阶段不需要 API Key，也不需要联网。

### 阶段 B：NPC 人格反应

你需要完成：

- 创建 `Npc Personality Profile`；
- 给 NPC 添加 `Npc Knowledge Receiver Component`；
- 调用 `Receive Unlocked Knowledge Pack`；
- 显示返回的语气、行为意图和反应强度。

这一阶段也不需要模型。

### 阶段 C：AI 对话

你需要完成：

- 配置 Dialogue Provider；
- 配置 Semantic Reviewer；
- 给 NPC 添加 `AI NPC Dialogue Component` 和 `Knowledge Semantic Review Component`；
- 调用 `Send Player Message`；
- 只从 `On Final Reply` 读取最终台词。

这一阶段需要兼容 OpenAI Chat Completions 请求与响应格式的服务。你可以使用云端服务，也可以连接本机服务。

## 3. 安装插件

### 3.1 环境要求

- Unreal Engine 5.8；仓库中的 0.4.3 已在 UE 5.8.4 / Win64 验证。
- Windows 源码编译需要 Visual Studio 2022、Desktop development with C++ 工作负载，以及与 UE 对应的 Windows SDK。
- 纯 Blueprint 项目第一次加载源码插件时仍可能要求本机具备 C++ 编译工具链。

其他引擎版本可以尝试源码编译，但仓库没有为它们提供完成证据。

### 3.2 复制目录

你的项目结构应为：

```text
YourGame/
  YourGame.uproject
  Content/
  Plugins/
    MyAiTownKnowledgeGrant/
      MyAiTownKnowledgeGrant.uplugin
      Source/
      Config/
```

不要只复制 `Source`，也不要把插件多套一层目录。下面这种结构会导致 UE 找不到 `.uplugin`：

```text
Plugins/MyAiTownKnowledgeGrant/MyAiTownKnowledgeGrant/MyAiTownKnowledgeGrant.uplugin
```

### 3.3 启用并编译

1. 关闭 Unreal Editor。
2. 把整个 `MyAiTownKnowledgeGrant` 目录复制到项目的 `Plugins/`。
3. C++ 项目右键 `.uproject`，选择 **Generate Visual Studio project files**。
4. 打开项目。UE 提示重新构建模块时，选择 **Yes**。
5. 进入 **Edit → Plugins**，搜索 `My AI Town Knowledge Grant`，确认插件处于启用状态。
6. 重启编辑器。

成功标志：

- **Add Component** 搜索结果中出现 `Knowledge Grant`、`Knowledge Context Collector`、`AI NPC Dialogue` 等组件；
- Blueprint 事件图右键搜索时能找到 `Grant Knowledge From Quest` 和 `Build Dialogue Knowledge View`。

编译失败时先检查 Visual Studio 工作负载和 UE 版本。不要把编译失败当成 Blueprint 连线问题。

## 4. 注册 Gameplay Tags

插件用 Gameplay Tags 选择当前相关的知识库。Tag 只负责筛选，不能代替任务授权。

### 4.1 打开设置

进入：

```text
Edit → Project Settings → Project → Gameplay Tags
```

添加一组适合你项目的稳定标签，例如：

```text
Knowledge.Context.Scene.NorthGate
Knowledge.Context.Scene.Tavern
Knowledge.Context.Entity.Caravan
Knowledge.Context.Entity.MerchantGuild
Knowledge.Context.Item.BronzeFragment
```

### 4.2 命名建议

使用下面的结构：

```text
Knowledge.Context.Scene.<场景ID>
Knowledge.Context.Entity.<人物或组织ID>
Knowledge.Context.Item.<道具ID>
```

ID 在发布后尽量保持稳定。显示名称可以本地化，Tag 负责程序匹配，不适合随文案一起改名。

成功标志：创建 Knowledge Pack 时，`Activation Tags` 的下拉列表能找到刚才注册的标签。

## 5. 先在纸面上拆一条多米诺知识链

下面用“失踪商队”作为示例：

| 顺序 | PackId | 已经发生的事实 | Required Pack Ids |
|---:|---|---|---|
| 1 | `caravan.public` | 商队失踪，城市正在调查 | 空 |
| 2 | `caravan.wreckage` | 玩家在北门外发现车辆残骸 | `caravan.public` |
| 3 | `caravan.tracks` | 足迹通往旧矿井 | `caravan.wreckage` |
| 4 | `caravan.resolved` | 玩家确认袭击者身份 | `caravan.tracks` |

每个包只写该事件发生后已经成立的事实。不要把“袭击者身份”提前写进公共前提包，再期待模型自己保密。插件只能保护你正确拆分过的资料。

### 5.1 骨牌规则

- 第一块骨牌没有前置。
- 后续包通过 `Required Pack Ids` 指向已经授予的包。
- 乱序授予返回 `knowledge_prerequisite_missing`。
- 同一个 `GrantEventId` 重放不会重复授予。
- 同一个 `GrantEventId` 改授另一个包会返回 `grant_event_id_conflict`。

## 6. 创建 Knowledge Pack Data Asset

### 6.1 创建第一块骨牌

1. 在 Content Browser 新建目录，例如 `/Game/Knowledge/Caravan/`。
2. 在空白处右键，选择 **Miscellaneous → Data Asset**。
3. 在类选择器中搜索 `Knowledge Pack`。
4. 创建资产，命名为 `DA_Knowledge_Caravan_Public`。
5. 打开资产并填写字段。

推荐填写：

| 字段 | 示例 | 说明 |
|---|---|---|
| `Pack Id` | `caravan.public` | 全项目唯一且稳定的包 ID |
| `Source Quest Id` | `quest.caravan.start` | 由任务完成授予时填写 |
| `Source World Event Id` | 空 | 与 `Source Quest Id` 二选一 |
| `Required Pack Ids` | 空 | 第一块骨牌没有前置 |
| `Unlock Tag` | 项目自定义 | 可供其他系统查询的授权标签 |
| `Activation Tags` | 空或 `Knowledge.Context.Entity.Caravan` | 空表示全局可用；有值时需匹配任意当前语境 Tag |

`Source Quest Id` 与 `Source World Event Id` 不要同时填写。任务结算使用 Quest，现场发现或世界变化使用 World Event。

### 6.2 添加文档和原子事实

在 `Documents` 中添加一个元素：

```text
Document Id: caravan.public.document
Content: 北门商队未按时返回，城市已经发起调查。
```

在该 Document 的 `Facts` 中添加一个元素：

```text
Fact Id: caravan.public.missing
Statement: 北门商队未按时返回。
Entities: 北门商队
Numbers: 空
Negated: false
```

字段含义：

- `Document Id` 标识整段提示词资料。
- `Fact Id` 标识一条可以被机械校验的原子事实。模型必须引用它，不能用 Document Id 代替。
- `Statement` 只写一件事。一个句子同时包含地点、人数、时间和结论时，应拆成多个 Fact。
- `Entities` 填台词里必须按规范出现的专名。
- `Numbers` 填规范数字，例如 `3`、`三` 或 `12.5`。写法要与项目文案规范一致。
- `Negated` 表示该原子事实本身是否为否定句。

### 6.3 创建后续骨牌

复制第一份资产，分别创建：

```text
DA_Knowledge_Caravan_Wreckage
DA_Knowledge_Caravan_Tracks
DA_Knowledge_Caravan_Resolved
```

修改每个资产的 `Pack Id`、事件来源、前置包和事实。复制资产后最容易漏改 `Pack Id`，请逐个核对。

成功标志：四个资产使用不同 `Pack Id` 和不同 `Fact Id`，后续三个资产分别指向上一块骨牌。

## 7. 创建守卫策略

### 7.1 机械守卫策略

1. 右键 **Miscellaneous → Data Asset**。
2. 选择 `Knowledge Dialogue Guard Policy`。
3. 命名为 `DA_KnowledgeGuard_Default`。

新手建议配置：

```text
Mode: AllOrNothing
Insufficient Evidence Text: 现有证据不足，我不能确认这件事。
Invalid View Text: 当前信息已经变化，请重新发起对话。
Malformed Output Text: 我暂时无法整理出可靠回答。
```

`AllOrNothing` 会在一条候选中出现非法 Claim 时拦截整段。`PerClaim` 会保留通过的 Claim，适合你已经理解 Claim 拆分与 UI 拼接后再启用。

把剧情专名加入 `Protected Entity Lexicon`。把中文数字词等项目需要保护的数字写法加入 `Protected Number Lexicon`。阿拉伯数字由插件自动检查。

### 7.2 语义审查策略

1. 创建 `Knowledge Semantic Review Policy` Data Asset。
2. 命名为 `DA_SemanticReview_Default`。
3. 填写：

```text
Policy Version: semantic.review.v1
Timeout Seconds: 5
Blocked Text: 我不能依据现有资料确认这个说法。
Error Text: 对话服务暂时不可用，请稍后再试。
```

语义审查器只能批准、拦截或返回错误。它不能提供替换台词，防止第二个模型绕过机械守卫注入新内容。

## 8. 给玩家添加组件

### 8.1 单机项目

把组件加到跨关卡持续存在并能参与存档的玩家 Actor 或 PlayerState：

- `Knowledge Grant Component`
- `Knowledge Context Collector Component`

把授权组件挂在 NPC 上会让每个 NPC 拥有一份独立玩家授权账本，后续存档和联机会很难维护。

### 8.2 联机项目

把 `Knowledge Grant Component` 放在服务器权威、可复制的 PlayerState。授予、撤销和恢复只在服务器执行。

客户端可以读取复制后的状态，但客户端调用写入节点会返回 `authority_required`。

成功标志：运行游戏后，玩家对应 Actor 的 Details 或调试输出能看到这两个组件；服务端调用授予节点时 `bOk=true`。

## 9. 授予第一块知识骨牌

### 9.1 学习用直接接法

在服务器任务完成事件中调用：

```text
任务完成事件
  → Grant Knowledge From Quest
      Grant Event Id = PlayerId:QuestId:CompletionRevision
      Completed Quest Id = Knowledge Pack.SourceQuestId
      Knowledge Pack = DA_Knowledge_Caravan_Public
```

世界观察事件改用：

```text
Grant Knowledge From World Event
```

`Grant Event Id` 示例：

```text
player-42:quest.caravan.start:1
player-42:event.caravan.wreckage:1
```

不要使用随机 GUID 作为可重放任务的事件 ID。服务器重试后生成新的随机值，会破坏幂等判断。

### 9.2 正式 ARPG 推荐接法

正式任务系统建议使用：

- `ARPG Knowledge Grant Registry`：固定维护 `QuestId → KnowledgePack`；
- `ARPG Knowledge Grant Adapter Component`：从可信任务提供者读取玩家、完成状态和完成修订；
- `ARPG Quest Authority Provider`：由你的服务器任务系统实现。

任务结算只把 `QuestId` 交给 `Grant Completed Quest Knowledge`。不要从客户端或任意 Blueprint 临时传入玩家 ID、Knowledge Pack 和完成修订。

详细合同见 [ARPG_INTEGRATION.md](ARPG_INTEGRATION.md)。

### 9.3 读取授予结果

拆分 `Knowledge Grant Result`，检查：

- `bOk`：调用是否成功；
- `bApplied`：本次是否改变了授权状态；
- `bDuplicate`：是否属于幂等重放；
- `Error`：失败原因；
- `Revision`：玩家授权修订号。

调试时把这些字段输出到日志。只打印“任务完成”无法判断知识是否授予成功。

## 10. 收集当前语境

`Knowledge Context Collector Component` 汇总三类来源。

### 10.1 场景语境

在 Trigger Volume 中连接：

```text
On Component Begin Overlap
  → Enter Scene Context
      Source = 当前 Volume
      Context Tags = Knowledge.Context.Scene.NorthGate

On Component End Overlap
  → Exit Scene Context
      Source = 当前 Volume
```

多个重叠 Volume 可以同时贡献 Tags。销毁的来源不会继续污染语境。

### 10.2 交互对象语境

玩家开始与 NPC、道具或公告板交互时调用：

```text
Set Interaction Context
```

交互结束时调用：

```text
Clear Interaction Context
```

`Set Interaction Context` 会替换旧交互对象，不会把旧目标的 Tags 一直累积下去。

### 10.3 明确提及的专名

对话解析器识别到玩家明确提及“北门商队”后，调用：

```text
Set Explicit Mention Context
```

本轮结束后调用：

```text
Clear Explicit Mention Context
```

关卡切换、玩家重生或强制结束对话时调用 `Reset All Contexts`。

成功标志：`Get Active Context Tags` 返回当前场景、当前交互目标和本轮专名的并集，离开场景或结束交互后对应 Tag 消失。

## 11. 构建本轮知识视图

在每次对话生成前调用：

```text
Get Active Context Tags
  → Build Dialogue Knowledge View
      Player Knowledge = 当前玩家的 Knowledge Grant Component
      Candidate Packs = 本次 NPC 可能用到的知识包
      Active Context Tags = Collector 输出
```

`FDialogueKnowledgeView` 包含：

- `Allowed Documents`：已授权且当前语境激活的资料；
- `Included Pack Ids`：本轮进入视图的知识包；
- `Excluded Reasons`：未授权、语境不匹配或无效的包；
- `Player Revision` 和 `Authorization Epoch`：绑定当前授权状态；
- `Scope Fingerprint`：发现意外改写的一致性校验值。

你每次开始新的模型请求都要重新构建 View。切换场景、改变交互目标或授予新知识后，不要复用旧 View。

成功标志：

- 任务前，未来包出现在 `Excluded Reasons`，原因是 `NotGranted`；
- 任务后但场景不匹配时，原因是 `ContextMismatch`；
- 授权和语境都满足时，文档进入 `Allowed Documents`。

## 12. 给 NPC 添加 AI 对话组件

给每个需要 AI 对话的 NPC 添加：

- `AI NPC Dialogue Component`
- `Knowledge Semantic Review Component`
- 可选的 `Npc Knowledge Receiver Component`

在 `AI NPC Dialogue Component` 的 Details 中配置：

| 字段 | 填写内容 |
|---|---|
| `Persona.Npc Id` | 稳定 NPC ID，例如 `npc.guard.rowan` |
| `Persona.Display Name` | 玩家看到的名字 |
| `Persona.Role Instructions` | 公开角色说明，不要在这里写未授权剧情秘密 |
| `Player Knowledge` | 当前对话玩家的授权组件 |
| `Candidate Packs` | 该 NPC 可能引用的知识包列表 |
| `Context Collectors` | 当前玩家或对话协调器的 Collector |
| `Guard Policy` | `DA_KnowledgeGuard_Default` |
| `Semantic Review Component` | 同一 NPC 上的审查组件 |
| `Semantic Review Policy` | `DA_SemanticReview_Default` |
| `Max Local Transcript Turns` | 新手先保留默认 24 |

多人游戏中，`Player Knowledge` 不能在 Class Defaults 中固定指向某个玩家。对话开始时根据当前交互玩家设置引用。

## 13. 配置 AI Provider

### 13.1 项目默认值

进入：

```text
Edit → Project Settings → Plugins → AI NPC Roleplay
```

这里可以填写非敏感默认值：

- Dialogue Provider URL；
- Dialogue 模型名称；
- Semantic Reviewer URL 和模型；
- 超时、输出长度与玩家披露文本。

不要在 Project Settings、ini、Data Asset 或 Blueprint 默认值中写 API Key。

### 13.2 给玩家打开运行时设置面板

在暂停菜单或 AI 设置页调用：

```text
Show AI NPC Provider Settings
  Player Controller = 本地 PlayerController
  ZOrder = 100
```

玩家在面板中填写：

- 厂商名称；
- 完整 `/chat/completions` URL；
- 模型名称；
- API Key；
- 是否同意发送当轮数据；
- 语义审查是否复用同一 Provider。

点击 `Apply for this game session` 后，Key 只保存在当前游戏进程内存。退出游戏或点击 `Clear credentials` 后，插件清除凭据。

### 13.3 URL 规则

- 远程服务必须使用 HTTPS。
- HTTP 只允许显式启用的 loopback 地址，例如 `http://127.0.0.1:1234/v1/chat/completions`。
- URL 需要包含完整 Chat Completions 路径，不能只填域名。

详细上传边界见 [AI_NPC_PROVIDER.md](AI_NPC_PROVIDER.md)。

## 14. 发起一轮 AI 对话

### 14.1 推荐的新手接法

在 NPC 对话 Blueprint 中：

```text
玩家点击“发送”
  → Send Player Message
      Player Text = 输入框文字
  → 保存返回的 RequestId，用于调试

AI NPC Dialogue Component.On Final Reply
  → Branch: bSuccess
      True  → 把 FinalApprovedText 显示到字幕或对话框
      False → 显示可信错误或兜底文本
```

`AI NPC Dialogue Component` 会自动完成：

1. 读取玩家授权与实时语境；
2. 构建知识视图；
3. 把本轮允许事实和玩家输入发送给 Dialogue Provider；
4. 解析严格 JSON；
5. 执行机械事实守卫；
6. 调用 Semantic Reviewer；
7. 在审查返回后重新核验授权和语境；
8. 通过 `On Final Reply` 返回正式文本。

不要把 Provider 原始响应或机械阶段的 `SafeText` 直接接到 UI。自由生成台词只消费 `FinalApprovedText`。

### 14.2 模型必须返回的 JSON

示例：

```json
{
  "claims": [
    {
      "claimId": "caravan.reply.001",
      "text": "北门商队尚未返回，城里已经派人调查。",
      "evidenceFactIds": ["caravan.public.missing"],
      "claimedEntities": ["北门商队"],
      "claimedNumbers": [],
      "negated": false
    }
  ]
}
```

根对象和每个 Claim 都不能带额外字段。每个 Claim 必须使用唯一且非空的 `claimId`，并引用存在于当前 View 的 `FactId`。`claimedEntities` 和 `claimedNumbers` 是可选的不可信诊断字段；插件会从 `text` 重新提取有效专名/数字，不会把它们当作授权来源。

### 14.3 机械 Guard 检查什么

- JSON 结构和字段类型；
- Claim ID 是否为空或重复；
- `evidenceFactIds` 是否属于当前允许事实；
- 台词中的受保护专名、地点、组织、任务物品是否属于当前允许事实；
- 数字、正负号、小数和项目数字词是否一致；
- 否定状态是否与事实一致；
- 授权修订、授权世代和当前语境是否仍与 View 一致。

`claimedEntities` / `claimedNumbers` 只作为不可信诊断输出，不能授权任何 Claim。模型说得通顺也不能替代这些检查。

### 14.4 语义审查的 Claim 合同

语义审查器必须为每条 Claim 返回一条 `claimAssessments`，包含：

- `claimType`：`factual` 或 `non_factual`；
- `support`：`supported` / `partial` / `unsupported`；
- `contradiction`、`overreach`、`taskLeak`：`none` / `possible` / `clear`；
- `roleFit`：`good` / `weak` / `bad`；
- `confidence`：`high` / `medium` / `low`；
- `evidenceFactIds`：必须是该 Claim 原始证据的子集；
- 六个评分字段（事实支持、矛盾、越权、任务泄露、角色契合、置信度）：仅诊断，不能用总分补偿安全失败。

缺失 Claim、重复 Claim、额外 Claim、证据不匹配、分数越界或任一安全枚举失败，整段都会失败关闭。

## 15. 配置 NPC 人格反应

### 15.1 创建人格资产

创建 `Npc Personality Profile` Data Asset，配置 0 到 100 的人格维度：

- Empathy：共情；
- Curiosity：好奇；
- Caution：谨慎；
- Order Loyalty：秩序忠诚；
- Aggression：攻击性。

这些数值描述稳定人格。一次事件只产生反应，不会永久改写人格资产。

### 15.2 给知识文档补充反应信息

在每个 `Knowledge Document` 中填写：

- `Threat Level`：即时危险程度；
- `Novelty`：新奇或信息量；
- `Moral Violation`：残酷、背叛或不公程度；
- `Order Impact`：对秩序的正负影响；
- `Topics`：主题标签。

### 15.3 延迟接收

NPC 即将参与相关对话或活动时，在 Authority 端调用：

```text
Receive Unlocked Knowledge Pack
  Audience Id = 当前玩家稳定 ID
  Receipt Event Id = 稳定接收事件 ID
  Player Knowledge = 当前玩家授权组件
  Knowledge Pack = 已解锁包
  Affect Context = 可选当前情绪
```

不要在任务完成时遍历地图上的全部 NPC。延迟接收减少无用计算，也让你能控制消息传播时机。

返回结果包含兴趣、认可、感知威胁、强度、台词语气、行为意图和有界情绪建议量。

## 16. 保存与恢复

### 16.1 玩家知识

保存：

```text
Create Snapshot
  Pack Registry Version = 你的稳定注册表版本
```

把返回的 `Knowledge Grant Snapshot` 写入你的 SaveGame 或服务器数据库。

恢复：

```text
Restore Snapshot
  Snapshot = 存档中的快照
  Registered Packs = 当前版本全部注册包
  Expected Pack Registry Version = 当前注册表版本
```

### 16.2 NPC 反应

使用：

```text
Create Reaction Snapshot
Restore Reaction Snapshot
```

玩家知识快照和 NPC 反应快照是两份不同数据。保存其中一份不会自动保存另一份。

### 16.3 版本边界

当前 Schema 为 v3。插件拒绝旧 Schema、错误 Registry 版本、缺少前置包、循环依赖、伪造 Tags 和没有事件历史的授权包。

如果你需要保留 v1 或 v2 存档，请在宿主项目中编写显式迁移。插件不会猜测旧数据如何转换。

## 17. 单机、Listen Server 与专用服务器

| 场景 | Knowledge Authority | Provider 凭据 |
|---|---|---|
| 单机 | 本地 Authority | 玩家可在运行时输入 BYOK |
| Listen Server | 主机 Authority | 本地主机玩家可使用 BYOK；项目需说明费用和隐私 |
| Dedicated Server | 服务器 Authority | 服务器配置项目自有 Provider；不要复制客户端 Key |

客户端提交的“任务已完成”“已经审查”或“这个知识包应该解锁”都不能直接改变服务器状态。

## 18. 第一条可验收流程

先做一条两骨牌流程：

```text
Public Pack → Discovery Pack
```

按下面的顺序测试：

1. 启动游戏，确认两个包都未授权。
2. 授予 Public Pack，检查 `bOk=true`、`bApplied=true`。
3. 在错误顺序下尝试先授予另一个后续包，确认收到 `knowledge_prerequisite_missing`。
4. 授予 Discovery Pack，构建 View，确认文档数量增加。
5. 切换到不匹配的场景，确认 Discovery Pack 因 `ContextMismatch` 被排除。
6. 返回正确场景并发送一轮对话。
7. 让测试候选引用不存在的 FactId，确认最终输出使用可信兜底。
8. 保存、清空运行时状态、恢复，确认两个包和 Revision 合法恢复。

完成这条流程后再扩展到四骨牌、多个 NPC 和正式任务系统。

## 19. 常见错误与排查

| 错误或现象 | 原因 | 检查方法 |
|---|---|---|
| 找不到插件节点 | 插件未启用或模块未编译 | 检查 Plugins 面板和编译日志 |
| `authority_required` | 客户端调用了授予、撤销或恢复 | 把调用移到服务器 Authority |
| `knowledge_prerequisite_missing` | 后续包的前置尚未授予 | 核对 `Required Pack Ids` 和授予顺序 |
| `grant_event_id_conflict` | 同一个事件 ID 被用于不同 Pack | 修正稳定事件 ID 生成规则 |
| View 中 `docs=0` | 未授权、语境不匹配或 Pack 无效 | 查看 `Excluded Reasons` |
| `dialogue_view_context_changed` | 模型等待期间场景或交互对象变化 | 重新构建 View 并发起新请求 |
| `dialogue_view_stale` | 模型等待期间玩家授权 Revision 变化 | 丢弃旧响应并重试 |
| `dialogue_view_authorization_changed` | 授权世代变化，例如恢复存档 | 丢弃恢复前创建的 View |
| `dialogue_view_tampered` | View 内容或指纹不一致 | 不要修改 View，重新构建 |
| `provider_not_configured` | URL、模型、Key 或同意项缺失 | 打开运行时 Provider 面板检查 |
| Provider 请求失败 | URL 不完整、远程 HTTP、认证失败或超时 | 检查完整 `/chat/completions` URL、HTTPS 和厂商日志 |
| Guard 返回 `Blocked` | FactId、专名、数字或否定不一致 | 记录模型 JSON，并逐项与 Fact 对照 |
| 只有 `SafeText` 没有最终台词 | 尚未经过语义审查 | 绑定审查组件和策略，读取 `FinalApprovedText` |
| 审查超时 | Reviewer 不可用或超过 Policy 超时 | 检查审查 Provider，显示 `ErrorText` |
| NPC 反应重复 | AudienceId 或 ReceiptEventId 不稳定 | 使用项目定义的稳定玩家与事件标识 |
| 恢复快照失败 | Schema、Registry 或前置闭包不一致 | 打印 Restore 的 `Error`，不要强行信任旧数据 |

## 20. 新手发布前检查单

### 插件与资产

- [ ] `.uplugin` 位于 `Plugins/MyAiTownKnowledgeGrant/` 第一层。
- [ ] 项目可以编译并启动。
- [ ] 每个 PackId、DocumentId、FactId 在自己的作用域内保持稳定。
- [ ] 后续事实位于后续 Pack，没有提前塞入公共包。
- [ ] Gameplay Tags 已在项目中注册。

### 授权与语境

- [ ] `Knowledge Grant Component` 位于玩家或 PlayerState，而不是每个 NPC。
- [ ] 联机写操作只在 Authority 执行。
- [ ] GrantEventId 可重复生成，并能识别任务完成修订。
- [ ] 场景、交互和专名 Context 在进入与退出时成对更新。
- [ ] 每轮模型请求前重新构建 View。

### 模型与输出

- [ ] 模型输出使用严格 JSON 和 `evidenceFactIds`。
- [ ] 机械 Guard 使用可信宿主策略资产。
- [ ] 自由生成台词经过独立语义审查。
- [ ] UI、语音和行为树只消费 `FinalApprovedText`。
- [ ] 超时、错误和拦截使用宿主预先配置的兜底文本。

### 隐私与联机

- [ ] API Key 没有写入 ini、SaveGame、Data Asset 或日志。
- [ ] 玩家看到数据传输披露，并主动勾选同意。
- [ ] 远程 Provider 使用 HTTPS。
- [ ] Dedicated Server 使用项目凭据，不索取客户端 Key。
- [ ] 正式项目完成 Cook/Package、真实任务系统、持久化和服务器验收。

## 21. 继续阅读

- [操作原理思维导图](OPERATION_MINDMAP.md)：一张图理解整体链路。
- [安装与接入检查单](../INSTALL.md)：安装和发布前硬检查。
- [AI-NPC 厂商连接与本地会话](AI_NPC_PROVIDER.md)：BYOK、上传边界和会话隔离。
- [ARPG 任务授权适配器](ARPG_INTEGRATION.md)：正式任务系统接入。
- [独立语义审查接口](SEMANTIC_REVIEW_API.md)：自建审查服务合同。
- [架构与工作原理](ARCHITECTURE.md)：组件、数据流和失败策略。
- [验证结果](VERIFICATION.md)：仓库已有证据及尚未覆盖的范围。


