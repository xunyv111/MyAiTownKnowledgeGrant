# ARPG 任务授权适配器

## 推荐入口

正式任务知识不要由分散的任务蓝图直接调用底层 `Grant Knowledge From Quest`。在玩家持久 Actor 或 PlayerState 上添加 `ARPG Knowledge Grant Adapter Component`，并配置：

- `ARPG Knowledge Grant Registry`：固定的 `QuestId → KnowledgePack` 映射和注册表版本；
- `Quest Authority Provider`：实现 `ARPG Quest Authority Provider` 的服务器任务系统或适配对象；
- `Knowledge Component`：玩家自己的知识组件；留空时从同一 Owner 查找。

## 权威提供者合同

`Resolve Completed Quest` 必须根据传入的玩家 Owner 在服务器任务状态中查询，并返回：

- 任务是否确实为 Completed；
- 该玩家的稳定 ID；
- 单调递增且可持久化的完成修订号；
- 本事件是否来自调试/作弊路径。

适配器不接受调用方手写玩家 ID、知识包或 GrantEventId。它会：

1. 检查 Authority、组件和注册表；
2. 从可信提供者核验任务属于当前 Owner；
3. 拒绝未完成任务、调试事件、非法玩家 ID 和无效修订号；
4. 从固定注册表解析唯一知识包；
5. 在服务器生成 `arpg|PlayerId|QuestId|CompletionRevision`；
6. 拒绝低于已记录修订号的乱序事件；
7. 调用核心授权组件并保留其幂等与因果前置检查；
8. 为每次成功或失败生成瞬态审计记录并广播 `On Grant Audited`。

## 持久化与审计

成功且实际改变授权时，结果中的 `bPersistenceRequired=true`。宿主应监听 `On Grant Audited`，使用自己的 SaveGame、数据库或 Outbox 持久化玩家知识快照和业务审计。

组件内 `AuditRecords` 只是有界瞬态诊断队列，默认保留 256 条，不是持久审计数据库。正式日志应记录玩家稳定 ID、QuestId、PackId、GrantEventId、Revision 和错误码，但不记录模型密钥或未授权正文。

## ARPG 发布前检查

- Quest Authority Provider 只能存在于服务器可信路径；
- 稳定玩家 ID 在存档、重连和地图切换后保持一致；
- 完成修订号不会倒退或复用给不同完成事务；
- RegistryVersion 随映射变化而更新，并与知识快照版本策略一致；
- 调试任务和作弊入口返回 `bOutDebugEvent=true`；
- 授权成功后持久化失败必须进入重试/Outbox，而不是静默丢失；
- 任务回滚、周目重置和旧存档迁移有明确项目策略；
- 字幕出口完成机械守卫与独立语义审查后，只消费 `FinalApprovedText`。
