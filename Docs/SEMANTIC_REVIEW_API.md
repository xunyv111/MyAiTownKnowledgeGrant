# 独立语义审查接口

## 定位

`Knowledge Semantic Review Component` 是供应商无关的异步审查边界。插件不发送 HTTP、不保存 API Key，也不依赖任何模型 SDK。宿主监听 `On Semantic Review Requested`，把请求交给云 API、本地模型或自建服务，再把逐 Claim 的结构化评估传回 `Complete Semantic Review`。

`SafeText` 只表示 FactId、专名、数字、否定和 JSON 合同通过机械检查。启用语义审查后，字幕、语音、行为树和正式日志只能消费 `FinalApprovedText`。

## 接入流程

1. 使用 `Guard Knowledge Model Json Response For Collectors` 得到机械检查结果。
2. 创建 `Knowledge Semantic Review Policy`，配置稳定 `PolicyVersion`、超时和可信兜底。
3. 在 Authority 调用 `CreateSemanticReview`，只创建 pending 请求，不广播。
4. 调用方先建立“审查 RequestId → 对话请求 ID”的关联。
5. 调用 `DispatchSemanticReviewRequest` 广播 `OnSemanticReviewRequested`；此时宿主可以安全地同步或异步调用 `CompleteSemanticReview`。
6. 宿主监听事件时，由宿主发送审查请求；否则内置 Provider 发送。两者只能有一个发送方。绑定监听器后，内置 Provider 会自动让出；如无监听器且 `bUseBuiltInSemanticReviewProvider=false`，会失败关闭。
7. 只消费 `FinalApprovedText`。批准时它是原机械文本；拦截、错误和超时时它是策略资产中的可信兜底。

`BeginSemanticReview` 仅作为兼容便捷入口，内部执行 Create + Dispatch；需要自己建立关联的调用方应使用 Create → Correlate → Dispatch。

## 请求合同

`FKnowledgeSemanticReviewRequest` 包含：

- `RequestId`：本次审查的唯一 ID，响应必须原样回传；
- `PlayerRevision`、`AuthorizationEpoch`、`ScopeFingerprint`：请求所绑定的知识视图快照；
- `PolicyVersion`：审查策略版本；
- `ExpiresAtUtc`：超时时间；
- `MechanicallyValidatedText`：机械门允许的完整文本；
- `AllowedClaimIds`：插件分配的 Claim ID 集合；
- `AllowedClaims`：每个 Claim 的文本、`EvidenceFactIds`、来源元数据；
- `AllowedFacts`：本轮允许的所有 FactId、Statement、Entities、Numbers 和否定状态。

插件负责定义 Claim 边界；审查器不能自行增加、删除、重命名或合并 Claim。

## 响应合同

`FKnowledgeSemanticReviewResponse` 只包含：

- `RequestId`；
- `ClaimAssessments`；
- `ReviewerName`、`ReviewerVersion`；
- `Error`（仅本地传输/解析错误）；
- `ReviewDurationMs`。

每个 `FKnowledgeSemanticClaimAssessment` 必须包含：

| 字段 | 允许值 | 作用 |
|---|---|---|
| `ClaimId` | 请求中的 Claim ID | 必须与请求完全对应 |
| `ClaimType` | `Factual` / `NonFactual` | 必须与源 Claim 是否有 Evidence 一致 |
| `Support` | `Supported` / `Partial` / `Unsupported` | 事实支持；`Factual` 必须为 `Supported` |
| `Contradiction` | `None` / `Possible` / `Clear` | 必须为 `None` |
| `Overreach` | `None` / `Possible` / `Clear` | 必须为 `None` |
| `TaskLeak` | `None` / `Possible` / `Clear` | 必须为 `None` |
| `RoleFit` | `Good` / `Weak` / `Bad` | 不能为 `Bad`；按策略可要求 `Good` |
| `Confidence` | `High` / `Medium` / `Low` | 不能为 `Low`；按策略可要求 `High` |
| `EvidenceFactIds` | FactId 数组 | 必须是源 Claim `EvidenceFactIds` 的子集 |
| 六个 `*Score` | `0..1` 有限数字 | 仅诊断，不参与门禁补偿 |

约束：

- `ClaimAssessments` 必须与 `AllowedClaimIds` **数量相同、ID 集合完全相等**；
- 每条 Claim 恰好出现一次，不能缺失、重复或多余；
- `Factual` Claim 必须有非空 Evidence，且全部来自源 Claim 的证据；
- `NonFactual` Claim 必须使用空 Evidence；
- 所有分数必须是有限数字并且在 `[0,1]`；
- 审查器不能返回替代台词、批准/拒绝总布尔值或额外字段。

## 本地门禁

`Complete Semantic Review` 会在本地重新执行：

1. 检查 `RequestId` 是否存在且未过期；
2. 重新读取实时 Context Collectors；
3. 重新验证 Revision、AuthorizationEpoch、ScopeFingerprint 和实时语境；
4. 检查 Claim 覆盖、Claim 类型、Evidence 绑定、枚举、分数和置信度；
5. 对每条 Claim 执行安全门禁；
6. 只有全部 Claim 通过，才释放 `FinalApprovedText`。

任一 Claim 的 `Contradiction`、`Overreach` 或 `TaskLeak` 不是 `None`，都会导致整段失败关闭。总分不能补偿单条 Claim 的安全失败。

## 失败关闭规则

以下情况不会放行模型文本：

- 机械结果不是 `Allowed` / `PartiallyAllowed`；
- 缺少有效 Collector；
- 策略版本无效；
- 超时、取消或未知 `RequestId`；
- 审查器名称或版本为空；
- Claim 数量或 ID 集合不一致；
- Claim 类型与源 Claim 不匹配；
- Factual Claim 没有 Evidence，或 Evidence 不属于源 Claim；
- NonFactual Claim 携带 Evidence；
- 分数非有限数字、越界或缺少字段；
- 任一安全枚举不是允许值；
- 置信度不足；
- 等待期间授权、存档世代或对话语境改变；
- 审查器返回 `Error`。

## 诊断字段

`RejectedCandidateText`、`ClaimAssessments`、`ReviewedClaims`、`WorstClaimQualityScore`、`WorstClaimId` 和审查器元数据只在非 Shipping 构建中保留，用于定位问题。它们不是正式台词出口。

## 宿主 API 建议

- 使用 HTTPS、服务器侧认证、请求大小限制、速率限制和短超时；
- 以 `RequestId` 做响应关联，拒绝重复完成；
- 记录审查器名称、版本、策略版本、Claim 评估和耗时，不记录 API Key；
- 对相同输入缓存时必须绑定 `AllowedClaimIds`、`AllowedClaims`、`AllowedFacts`、`PolicyVersion` 和 `ReviewerVersion`；
- 缓存命中后仍必须经过组件的实时视图复验；
- 连续超时或服务错误应由宿主熔断，熔断期间返回 Error，让插件使用可信兜底；
- 关键剧情可在宿主聚合两个独立审查器后，再提交一次最终评估；组件本身不调用或信任特定厂商。

## 不允许的接法

- 审查服务直接返回一段新台词并跳过机械守卫；
- 审查器自行增加、删除或合并 Claim；
- 用无关的允许事实给 Claim 背书；
- API 失败时退回显示机械 `SafeText`；
- 异步返回后不检查视图是否过期；
- 把客户端提供的“已审查”布尔值交给 Authority；
- 把模型原文、密钥或尚未授权事实写入正式日志。
