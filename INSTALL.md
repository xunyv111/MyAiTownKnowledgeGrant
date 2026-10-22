# 安装与接入检查单

## 兼容范围

- 已验证引擎：Unreal Engine 5.7.4。
- 已验证平台：Windows 64-bit。
- 插件类型：Runtime；不依赖模型 SDK、数据库或网络服务。
- 发布包包含 Win64 Editor 预编译 DLL、Development/Shipping 预编译对象和完整源码。

其他 UE 5.x 小版本或其他平台应从源码重新编译，并重新运行项目自己的 Automation、多人 PIE 与 Cook/Package。

## 安装

1. 关闭 Unreal Editor。
2. 将发布包中的 `MyAiTownKnowledgeGrant` 文件夹复制到项目的 `Plugins/`。
3. 确认项目启用了 `GameplayTags`；插件 Build.cs 已声明该模块依赖。
4. C++ 项目重新生成项目文件并编译；纯蓝图项目首次打开时允许 UE 编译插件。
5. 在 Plugins 面板确认 **My AI Town Knowledge Grant** 已启用并重启编辑器。

## 最小接入

1. 在服务器权威的玩家 Actor 或 `PlayerState` 上添加 `Knowledge Grant Component`。
2. 创建 `Knowledge Pack` Data Asset；每个包填写唯一 `Pack Id`、任务/世界事件来源、前置包和原子 Facts。
3. 任务事实真正发生后，只在 Authority 调用 `Grant Knowledge From Quest` 或 `Grant Knowledge From World Event`。
4. 在对话开始时用 `Knowledge Context Collector Component` 汇总场景、交互对象与专名 Tags。
5. 调用 `Build Dialogue Knowledge View`，仅把 `Allowed Documents/Facts` 交给模型。
6. 要求模型返回 README 中规定的严格 JSON；模型返回后把原始视图和实际 `Knowledge Context Collector` 组件交给 `Guard Knowledge Model Json Response For Collectors`，由插件内部读取实时语境 Tags。
7. 字幕、语音、行为树和正式日志只能消费 `Safe Text`，不得消费模型原文。
8. 需要撤销时调用 `Revoke Knowledge Pack` 并传入注册包；仍被依赖的包会返回 `pack_has_dependents`，应先撤销依赖者。
9. 需要限制内存时设置 `Max Processed Grant Records` / `Max Receipt Records`；超限会显式失败，不会静默丢历史。

## 发布前必须由宿主确认

- [ ] 每个 Pack/Document/Fact ID 唯一，前置链无悬空或循环。
- [ ] 未来事件和未来道具被拆进后续包，而非提前写入初始包。
- [ ] 所有正式模型出口都经过统一 Guard；没有原文旁路。
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
- 插件不是跨服授权账本，不实现换服加载、服务端正文存储或 NPC 全局持久身份；MMORPG 必须接入自己的后端权威层。

完整 API 与流程见 `README.md`，真实 UE 证据见 `VALIDATION.md`。
