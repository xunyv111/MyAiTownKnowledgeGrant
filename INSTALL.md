# 安装与接入检查单

> **English version:** [INSTALL_EN.md](INSTALL_EN.md)

## 兼容范围

- 已验证引擎：Unreal Engine 5.8.4。
- 已验证平台：Windows 64-bit。
- 插件类型：Runtime；使用 UE 内置 HTTP/JSON/UMG/Slate，不依赖第三方模型 SDK或数据库。
- Fab 提交包包含完整的人类可读源码，不包含 `Binaries`、`Build`、`Intermediate` 或 `Saved`；Unreal Engine 会在目标项目中重新生成这些目录。

其他 UE 5.x 小版本或其他平台应从源码重新编译，并重新运行项目自己的 Automation、多人 PIE 与 Cook/Package。

## 安装

1. 关闭 Unreal Editor。
2. 将发布包中的 `MyAiTownKnowledgeGrant` 文件夹复制到项目的 `Plugins/`。
3. 确认项目启用了 `GameplayTags`；插件 Build.cs 已声明该模块依赖。
4. C++ 项目重新生成项目文件并编译；纯蓝图项目首次打开时允许 UE 编译插件。
5. 在 Plugins 面板确认 **My AI Town Knowledge Grant** 已启用并重启编辑器。

## 最小接入

1. 在暂停菜单或设置菜单调用 `Show AI NPC Provider Settings`，让玩家输入 OpenAI-compatible URL、模型和 API Key。编辑器 `Project Settings → Plugins → AI NPC Roleplay` 只配置非敏感默认值。
2. 给每个需要自由角色扮演的 NPC 添加 `AI NPC Dialogue Component`，配置稳定 `Npc Id`、公开角色说明、知识组件、候选包、Context Collectors、机械 Guard 策略和语义审查组件/策略。组件会自动建立本地 Session。
3. 在服务器权威的玩家 Actor 或 `PlayerState` 上添加 `Knowledge Grant Component`。
4. 创建 `Knowledge Pack` Data Asset；每个包填写唯一 `Pack Id`、任务/世界事件来源、前置包和原子 Facts。
5. 任务知识通过 `ARPG Knowledge Grant Adapter Component` 授予：配置固定注册表，让服务器任务系统实现 `ARPG Quest Authority Provider`，再调用 `Grant Completed Quest Knowledge`。底层 `Grant Knowledge From Quest` 只保留给受控 C++ 适配层；观察型世界事件仍可在 Authority 调用 `Grant Knowledge From World Event`。
6. 在对话开始时用 `Knowledge Context Collector Component` 汇总场景、交互对象与专名 Tags。
7. 调用 `Build Dialogue Knowledge View`，仅把 `Allowed Documents/Facts` 交给模型。
8. 要求模型返回 README 中规定的严格 JSON；模型返回后把原始视图和实际 `Knowledge Context Collector` 组件交给 `Guard Knowledge Model Json Response For Collectors`，由插件内部读取实时语境 Tags。
9. 自由生成台词把机械结果交给 `Knowledge Semantic Review Component`；宿主监听 `On Semantic Review Requested` 调用自己的 API/本地模型，再把批准、拦截或错误响应交回 `Complete Semantic Review`。
10. 字幕、语音、行为树和正式日志只能消费 `Final Approved Text`，不得消费模型原文或机械 `Safe Text` 中间值。
11. 需要撤销时调用 `Revoke Knowledge Pack` 并传入注册包；仍被依赖的包会返回 `pack_has_dependents`，应先撤销依赖者。
12. 需要限制内存时设置 `Max Processed Grant Records` / `Max Receipt Records`；超限会显式失败，不会静默丢历史。

## 发布前必须由宿主确认

- [ ] 每个 Pack/Document/Fact ID 唯一，前置链无悬空或循环。
- [ ] 未来事件和未来道具被拆进后续包，而非提前写入初始包。
- [ ] 所有正式模型出口都经过统一 Guard；没有原文旁路。
- [ ] 自由生成出口经过独立语义审查，API 超时/错误使用可信兜底，并在响应后重验实时语境。
- [ ] 任务蓝图不直接传入玩家 ID、KnowledgePack 或 GrantEventId；正式授予只走 ARPG 适配器固定注册表。
- [ ] `On Grant Audited` 已接入宿主持久化/Outbox；`bPersistenceRequired=true` 不会被静默忽略。
- [ ] 多人项目使用服务器 Authority，并为 Receiver 提供稳定 `AudienceId`。
- [ ] SaveGame 使用 Schema v3 与稳定的 `Pack Registry Version`；v1/v2 已显式迁移或废弃。
- [ ] 客户端包中的明文 Data Asset 不包含必须防拆包的秘密正文。
- [ ] 目标 UE 版本、目标平台完成编译、Automation、Cook/Package 和项目级联机验收。

## 已知边界

- 输出门只做证据引用与元数据一致性检查，不证明开放域文本完整蕴含引用事实；规范事实模板或外部语义验证属于宿主职责。
- 宿主若绕开 Guard，插件无法阻止原始模型文本泄漏。
- v1/v2 玩家与 Receiver 快照不会自动迁移到 v3。
- Grant/Receipt 去重历史尚无内置滚动窗口，也没有 revoke/rollback；超长在线会话应由宿主制定保留、撤销和迁移策略。
- 插件控制“运行时允许进入对话上下文的资料”，不提供客户端资产反拆包保密。
- 插件只负责 ARPG 项目内授权，不实现跨服账本、服务器迁移、服务端正文存储或 NPC 全局持久身份。

完整 API 与流程见 `README.md`、`Docs/ARPG_INTEGRATION.md`、`Docs/SEMANTIC_REVIEW_API.md`，真实 UE 证据见 `VALIDATION.md`。
