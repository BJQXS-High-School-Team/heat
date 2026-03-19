# AGENTS Notes

本文件用于说明协作约定，便于多人并行开发与竞赛演示。

## Roles

- `firmware/wearable_c3`: 可穿戴端采集、状态判定、报警、HTTP 上报
- `firmware/gateway_s3`: 网关端采集、HTTP 服务、设备注册、Dashboard、MQTT 桥接
- `firmware/shared`: 共享数据结构、状态逻辑、Topic 工具
- `desktop_client`: 云端 MQTT 订阅与运维命令工具
- `docs`: 架构/API/Topic/验证流程文档
- `tests`: 非硬件逻辑测试（状态机、Topic、Schema 等）

## Engineering Rules

- 不修改既定四层架构，不合并 C3/S3 设备角色。
- C3 不得直接连接云端 MQTT。
- 不在生产代码中伪造传感器值。
- 传感器读取失败时使用最近有效值并记录健康状态。
- 所有周期任务使用 `millis()` 非阻塞调度。

## Commit Style (建议)

- `feat(wearable): ...`
- `feat(gateway): ...`
- `docs: ...`
- `test: ...`
- `refactor(shared): ...`
