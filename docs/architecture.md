# Architecture

## 1. 系统层次

1. ESP32-C3 可穿戴端（采集 + 本地规则 + 报警 + HTTP 上报）
2. ESP32-S3 网关端（采集 + HTTP API + Dashboard + MQTT 桥接）
3. Cloud MQTT Broker（消息中转）
4. Desktop Client（订阅监控/命令发布）

## 2. 固定链路

`ESP32-C3 -> LAN HTTP -> ESP32-S3 -> MQTT -> Cloud -> Desktop`

预留回控链路：

`Desktop/Cloud -> MQTT command -> S3 command cache -> HTTP response -> C3`

## 3. 关键职责

### C3

- 周期采集：GPS、DS18B20、MAX30102、MPU6050
- 本地规则判定：`normal | warning | danger`
- 本地告警：OLED + 震动 + 蜂鸣器（全部使用 `millis()` 非阻塞）
- HTTP JSON 上传到 S3
- 解析 S3 应答中的 override，并按优先级决定 effective state

### S3

- 周期采集：SHT35、BH1750
- 提供 HTTP API：`/api/health /api/host /api/devices /api/device /api/wearable/upload`
- 维护多可穿戴设备快照（固定容量）
- 本地 Dashboard 轮询展示
- MQTT 上行：网关遥测 + 可穿戴遥测/状态
- MQTT 下行：订阅 command，校验并缓存（含 TTL）
- 在 C3 下次上传时，通过 HTTP 响应下发最新 override

## 4. 数据完整性原则

- C3 不直连云端 MQTT，不直接上云。
- DS18B20 为接触式体表温度代理，不是核心体温。
- MAX30102 输出用于趋势预警，不宣称医疗诊断级。
- 读取失败保留最近有效值并更新健康标志。

## 5. 环境数据边界

SHT35/BH1750 挂在 S3 网关侧，因此只表示 `gateway_ambient`，不能标注为 wearable ambient，更不能替代用户实际暴晒时长。

## 6. 风险逻辑扩展点

- 当前：C3 本地阈值规则引擎
- 未来：云端/桌面侧 AI 评分（订阅 MQTT 后发布 override）
- 现有扩展接口：S3 command cache + C3 override 优先级决策
