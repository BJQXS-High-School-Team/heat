# Extreme-Heat Health Risk Early Warning System

面向极端高温人群健康风险预警的竞赛项目单仓库（monorepo）。

## 1. 四层架构（固定）

1. ESP32-C3 可穿戴端
2. ESP32-S3 网关端（本地 Web + MQTT 上行）
3. 云端 MQTT Broker
4. 桌面订阅客户端

强制数据链路：

`ESP32-C3 -> Wi-Fi LAN(HTTP) -> ESP32-S3 -> MQTT -> Cloud -> Desktop`

约束：

- C3 不直连云端 MQTT。
- S3 是唯一嵌入式 MQTT 上行节点。
- C3 在 S3/云离线时仍要本地采集与报警。
- S3 侧 SHT35/BH1750 数据仅能标注为网关侧环境（gateway ambient）。

## 2. 当前阶段状态

- Phase 1: 已完成（共享模型、配置策略、文档骨架）
- Phase 2: 已完成（C3 采集/本地规则/报警/HTTP 上报/override 应答）
- Phase 3: 已完成（S3 采集/API/设备注册/Dashboard）
- Phase 4: 已完成（S3 MQTT 桥接/命令缓存/TTL 过期/C3 覆盖链路）
- Phase 5: 已完成（Python MQTT 桌面订阅与命令发布）
- Phase 6: 已完成（测试、样例 payload、构建与验收说明、自检）

## 3. 目录结构

```text
firmware/shared/       共享数据结构、状态逻辑、Topic 格式工具
firmware/wearable_c3/  C3 固件
firmware/gateway_s3/   S3 固件
desktop_client/        Python MQTT 订阅/命令客户端
docs/                  架构/API/Topic/Dashboard/验证文档
tests/                 非硬件逻辑测试
```

## 4. 构建环境

- PlatformIO Core（或 VSCode PlatformIO 扩展）
- Python 3.10+
- 桌面端依赖：`desktop_client/requirements.txt`
- 测试依赖：`pytest`

## 5. 固件构建

```bash
platformio run -e wearable_c3
platformio run -e gateway_s3
```

如果 `platformio` 不在 PATH，可用绝对路径（Windows 示例）：

```powershell
& 'C:\Users\fanxi\.platformio\penv\Scripts\platformio.exe' run -e wearable_c3 -e gateway_s3
```

## 6. 运行配置

先修改以下配置文件中的真实参数：

- `firmware/wearable_c3/src/config.h`
  - Wi-Fi SSID/Password
  - 网关地址（`kGatewayHost`）
  - 本地阈值（心率/血氧/体温/活动）
- `firmware/gateway_s3/src/config.h`
  - Wi-Fi SSID/Password
  - MQTT Host/Port/User/Password
  - `site_id` 与 `gateway_id`

## 7. 推荐引脚映射

详见 `docs/pin_mapping.md`。当前默认：

- C3: I2C(`GPIO8/9`), DS18B20(`GPIO4`), GPS UART(`GPIO20/21`), 蜂鸣器(`GPIO2`), 震动(`GPIO3`)
- S3: I2C(`GPIO8/9`), SHT35(`0x44`), BH1750(`0x23`)

## 8. 运行验收（建议顺序）

1. 上电 S3，观察串口：Wi-Fi/MQTT 连接、SHT35/BH1750 采样、HTTP Server 启动。
2. 浏览器访问 `http://<S3_IP>/`，检查 Dashboard 与 `/api/*`。
3. 上电 C3，观察串口：传感器初始化、状态转换、HTTP 上报。
4. 在云端订阅 topics，确认 S3 上行网关与可穿戴数据。
5. 用桌面客户端发布 override，确认 S3 缓存并通过 HTTP 应答回传 C3。

## 9. 典型串口日志（示例）

C3：

```text
[C3] boot
[C3][INIT] temp=1 gps=1 imu=1 max30102=1 oled=1
[C3][HTTP] upload ok, status=200
[C3][STATE] local=warning effective=danger alarm=1 override=1
```

S3：

```text
[S3] boot
[S3][WIFI] connected, ip=192.168.1.50
[S3][SENSOR] sht35_ok=1 bh1750_ok=1 temp=33.8 hum=61.2 lux=13200.0
[S3][MQTT] connected, subscribe=1 topic=heatrisk/site/site-a/command/+
[S3][MQTT] cmd set for wearable-c3-001 (from topic)
```

## 10. 桌面客户端

安装依赖：

```bash
pip install -r desktop_client/requirements.txt
```

订阅监控：

```bash
python desktop_client/main.py --host <broker_host> --port 1883 --site-id site-a
```

发布命令：

```bash
python desktop_client/main.py --host <broker_host> --site-id site-a --publish-command --target-device-id wearable-c3-001 --state danger --vibrate --buzzer --expires-in-s 30
```

## 11. 测试

```bash
pip install pytest
pytest -q tests
```

当前测试覆盖：

- 本地状态判定契约
- Payload Schema 约束
- MQTT Topic 格式
- 命令 TTL / 过期回退逻辑（契约测试）
- Dashboard API JSON 形状

## 12. 已知限制

- 当前无 NTP/RTC，`gateway_receive_epoch_ms` 默认 0（可后续接入 NTP）。
- MAX30102 与 DS18B20 数据用于趋势预警，不应宣称医疗诊断级。
- 网关环境数据不能代表移动用户的真实环境暴露。
