# Extreme-Heat Health Risk Early Warning System

本仓库是一个面向竞赛的单仓库（monorepo）工程，目标是实现：

- ESP32-C3 可穿戴端：本地采集 + 本地判定 + 本地报警 + 上报网关
- ESP32-S3 网关端：接收可穿戴上报 + 本地仪表盘 + 云端MQTT桥接
- 云端MQTT Broker：消息中转
- 桌面客户端：订阅与运维监控（后续阶段完善）

## Architecture Summary

强制链路：

`ESP32-C3 -> HTTP/LAN -> ESP32-S3 -> MQTT -> Cloud -> Desktop`

约束：

- C3 不直连云端MQTT
- S3 是唯一嵌入式MQTT上行节点
- S3 同时承载本地Web Dashboard
- C3 在 S3/云端离线时仍可独立本地报警

## Repository Layout

```text
docs/                     设计文档与契约
firmware/shared/          C3/S3共享模型与逻辑
firmware/wearable_c3/     C3固件
firmware/gateway_s3/      S3固件
desktop_client/           桌面端订阅工具（后续完善）
tests/                    非硬件逻辑测试
```

## Quick Start (Phase 1)

1. 复制并修改配置模板（后续 Phase 2/3 会完善）：
   - `firmware/wearable_c3/src/config.h`
   - `firmware/gateway_s3/src/config.h`
2. 安装 PlatformIO（VSCode 插件或 `pio` CLI）
3. 查看文档：
   - `docs/architecture.md`
   - `docs/api_contracts.md`
   - `docs/mqtt_topics.md`
   - `docs/pin_mapping.md`

## Current Phase Status

- Phase 1: 进行中/已落地基础骨架（共享模型、配置策略、文档骨架）
- Phase 2+: 待继续实现固件、MQTT桥接、桌面端与测试完善
