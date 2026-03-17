# Architecture

## 1. System Layers

1. ESP32-C3 wearable/mobile terminal  
2. ESP32-S3 gateway/host/local web server  
3. Cloud MQTT broker  
4. Desktop client

## 2. Canonical Data Path

`C3 -> LAN HTTP -> S3 -> MQTT -> Cloud -> Desktop`

反向控制链路（预留）：
`Desktop/Cloud -> MQTT command -> S3 cache -> HTTP response -> C3`

## 3. Key Responsibilities

- C3:
  - 采集 GPS/体温/心率血氧/运动
  - 本地规则判定（normal/warning/danger）
  - 本地报警（震动+蜂鸣器）与OLED显示
  - HTTP JSON上报到S3
- S3:
  - 采集网关环境（SHT45/BH1750）
  - 接收可穿戴上报，维护设备快照
  - 提供本地Dashboard/API
  - 将网关+可穿戴数据桥接到云端MQTT
  - 接收云端命令并缓存，回传给C3

## 4. Ambient Data Limitation

SHT45/BH1750 接在 S3 网关侧，数据必须标注为 `gateway_ambient`，不得误标为 wearable ambient。

## 5. Risk Logic Strategy

- 当前阶段：C3 本地阈值规则引擎
- 未来阶段：可在云端或桌面侧增加更复杂AI评分，再通过MQTT命令回写
- 预留接口：S3命令缓存 + C3 override优先级处理
