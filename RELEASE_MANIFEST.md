# 0.2.1 交付清单

交付日期：2026-10-22  
目标引擎：Unreal Engine 5.7.4  
目标平台：Win64  
状态：可集成候选（Beta；非 MMORPG 后端）

作者与版权人：xunyv  
开源许可证：Apache License 2.0

## 官方构建结果

使用 UE `RunUAT BuildPlugin` 从隔离的临时 HostProject 构建，全部通过：

- UnrealHeaderTool：`-WarningsAsErrors` 通过。
- `UnrealEditor Win64 Development`：通过。
- `UnrealGame Win64 Development`：通过。
- `UnrealGame Win64 Shipping`：通过。
- AutomationTool：`BUILD SUCCESSFUL`，退出码 0。

原工作区路径包含中文；UE/MSVC 自动生成 PCH 在该路径下会报找不到文件。因此官方构建使用纯 ASCII 临时打包路径，成品再复制回交付目录。插件源码无需为此修改。

## 回归证据

- 插件 Automation：10/10。
- 宿主演示 Automation：2/2。
- 专用服务器/双客户端 PIE：1/1，业务错误 0。
- 保存关卡离屏验收：8/8。
- 静态插件验证：通过。

详细测试名、报告与未覆盖边界见 `VALIDATION.md`。

## 成品应包含

- `MyAiTownKnowledgeGrant.uplugin`
- `Binaries/Win64/UnrealEditor-MyAiTownKnowledgeGrant.dll`
- `Intermediate/Build/Win64/` 的 UE 预编译元数据与目标对象
- `Source/` 完整插件源码
- `Config/DefaultGameplayTags.ini`
- `README.md`、`INSTALL.md`、`VALIDATION.md`、`CHANGELOG.md`
- `LICENSE`、`NOTICE`、`CONTRIBUTING.md`（Apache License 2.0、版权声明与贡献规则）

Editor DLL SHA-256：`3900F3C3AAAA515FF2363CD734D3ABE794AE6D1C582A83776899BA7A8787C1D5`

## 不随插件包提供

- `KnowledgeGrantHost` 演示宿主及其地图、截图、日志和 Automation 报告。
- 任何模型 SDK、API 密钥、数据库或线上服务。
- 其他 UE 版本或其他平台的预编译二进制。

## 交付边界

该包可以直接复制到 UE 5.7.4 Win64 项目的 `Plugins/MyAiTownKnowledgeGrant/`。宿主仍必须完成 `INSTALL.md` 中的项目级出口审查、Cook/Package 和目标网络拓扑验收；插件包的成功构建不能替代宿主游戏的发布验收。

本包只提供 UE Runtime 授权与对话元数据一致性门，不提供开放域语义蕴含证明、客户端资产反拆包、跨服授权账本、服务端正文存储、撤销/迁移或无限期历史压缩。

