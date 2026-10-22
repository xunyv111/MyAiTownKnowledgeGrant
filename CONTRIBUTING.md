# Contributing

感谢你为 My AI Town Knowledge Grant 做出贡献。

## 开发约定

1. 从公开仓库创建分支或 Fork。
2. 保持插件运行时层不依赖具体模型 SDK、数据库或线上服务。
3. 新增知识授权、输出守卫或存档行为时，同时补充 Unreal Automation 覆盖。
4. 运行 `node Scripts/validate-plugin.mjs`，并在可用的目标 UE 版本中完成编译和 Automation Test。
5. Pull Request 应说明问题、行为变化、验证证据与尚未验证的边界。

贡献者提交的代码默认按项目的 Apache License 2.0 授权。不要提交 API Key、第三方私有资产、未获授权的 Unreal Engine 源码或其他机密信息。
