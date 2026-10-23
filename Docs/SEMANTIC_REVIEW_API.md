# 独立语义审查接口

## 定位

`Knowledge Semantic Review Component` 是供应商无关的异步审查边界。插件不发送 HTTP、不保存 API Key，也不依赖任何模型 SDK。宿主监听 `On Semantic Review Requested`，把请求交给云 API、本地模型或自建服务，再将结构化决定传回 `Complete Semantic Review`。

现有 `SafeText` 只表示 FactId、专名、数字、否定和 JSON 合同通过机械检查。启用语义审查后，字幕、语音、行为树和正式日志只能消费 `FinalApprovedText`。

## 接入流程

1. 使用 `Guard Knowledge Model Json Response For Collectors` 得到机械检查结果。
2. 创建可信的 `Knowledge Semantic Review Policy`，配置稳定 `Policy Version`、超时、拦截兜底和错误兜底。
3. 在 Authority 调用 `Begin Semantic Review`，传入同一玩家知识组件、知识视图、实际 Context Collectors、机械结果和策略。
4. 监听 `On Semantic Review Requested`。请求包含唯一 RequestId、授权 Revision/Epoch、作用域指纹、过期时间、机械文本、允许 ClaimId 和本轮允许 Facts。
5. 宿主只向自己的审查服务发送必要字段；密钥留在服务器配置，不进入 Data Asset、蓝图、日志或客户端包。
6. 审查服务只返回 `Approved`、`Blocked` 或 `Error`，以及已审 ClaimId、未支持 ClaimId、原因码、审查器名称和版本。响应不能携带替换台词。
7. Authority 调用 `Complete Semantic Review`。组件会重新读取实时语境，并重新验证 Revision、AuthorizationEpoch 和 ScopeFingerprint。
8. 只消费返回的 `FinalApprovedText`。批准时它是原机械文本；拦截、错误和超时时它是策略资产中的可信兜底。

## 失败关闭规则

以下情况不会放行模型文本：

- 缺少有效 Collector；
- 机械结果不是 Allowed/PartiallyAllowed；
- 策略版本无效；
- 超时、取消或未知 RequestId；
- 审查器名称/版本为空；
- 批准响应没有覆盖全部允许 ClaimId；
- 批准响应同时声明 Unsupported Claim；
- 等待期间授权、存档世代或对话语境改变；
- 审查器返回 Blocked/Error。

## 宿主 API 建议

- HTTPS、服务器侧认证、请求大小限制、速率限制和短超时；
- 以 `RequestId` 做响应关联，不接受重复完成；
- 记录审查器名称、版本、原因码和耗时，不记录 API Key；
- 缓存键至少绑定 Claims、AllowedFacts、PolicyVersion 和 ReviewerVersion；缓存命中仍必须经过组件的实时视图复验；
- 连续超时或服务错误应由宿主熔断，熔断期间返回 Error，让插件使用可信兜底；
- 关键剧情可在宿主聚合两个独立审查器后，再提交一次最终 Approved；组件本身不调用或信任特定厂商。

## 不允许的接法

- 审查服务直接返回一段新台词并跳过机械守卫；
- API 失败时退回显示机械 `SafeText`；
- 异步返回后不检查视图是否过期；
- 把客户端提供的“已审查”布尔值交给 Authority；
- 把模型原文、密钥或尚未授权事实写入正式日志。
