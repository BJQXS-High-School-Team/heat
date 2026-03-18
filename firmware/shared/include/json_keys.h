#pragma once

namespace heatrisk::jsonkeys {

// Common fields
static constexpr const char* kSchemaVersion = "schema_version";
static constexpr const char* kDeviceId = "device_id";
static constexpr const char* kDeviceType = "device_type";
static constexpr const char* kSeq = "seq";
static constexpr const char* kDeviceUptimeMs = "device_uptime_ms";
static constexpr const char* kGatewayTimeMs = "gateway_time_ms";
static constexpr const char* kGatewayReceiveEpochMs = "gateway_receive_epoch_ms";
static constexpr const char* kSiteId = "site_id";
static constexpr const char* kGatewayId = "gateway_id";
static constexpr const char* kOk = "ok";

// Nested objects
static constexpr const char* kGps = "gps";
static constexpr const char* kPhysiology = "physiology";
static constexpr const char* kMotion = "motion";
static constexpr const char* kStatus = "status";
static constexpr const char* kSensorHealth = "sensor_health";
static constexpr const char* kNetwork = "network";
static constexpr const char* kGatewayAmbient = "gateway_ambient";
static constexpr const char* kOverride = "override";
static constexpr const char* kHealth = "health";

// GPS
static constexpr const char* kLat = "lat";
static constexpr const char* kLng = "lng";
static constexpr const char* kHasFix = "has_fix";
static constexpr const char* kSatellites = "satellites";

// Physiology
static constexpr const char* kBodyTempC = "body_temp_c";
static constexpr const char* kHeartRateBpm = "heart_rate_bpm";
static constexpr const char* kSpo2Pct = "spo2_pct";

// Motion
static constexpr const char* kActivityScore = "activity_score";
static constexpr const char* kActivityLevel = "activity_level";

// Status
static constexpr const char* kLocalState = "local_state";
static constexpr const char* kEffectiveState = "effective_state";
static constexpr const char* kAlarmActive = "alarm_active";

// Sensor health
static constexpr const char* kGpsOk = "gps_ok";
static constexpr const char* kTempOk = "temp_ok";
static constexpr const char* kMax30102Ok = "max30102_ok";
static constexpr const char* kImuOk = "imu_ok";
static constexpr const char* kOledOk = "oled_ok";
static constexpr const char* kSht35Ok = "sht35_ok";
static constexpr const char* kBh1750Ok = "bh1750_ok";

// Network
static constexpr const char* kRssi = "rssi";
static constexpr const char* kWifiConnected = "wifi_connected";
static constexpr const char* kMqttConnected = "mqtt_connected";

// Command
static constexpr const char* kTargetDeviceId = "target_device_id";
static constexpr const char* kEnabled = "enabled";
static constexpr const char* kState = "state";
static constexpr const char* kReason = "reason";
static constexpr const char* kVibrate = "vibrate";
static constexpr const char* kBuzzer = "buzzer";
static constexpr const char* kExpiresInS = "expires_in_s";

// Gateway ambient
static constexpr const char* kTemperatureC = "temperature_c";
static constexpr const char* kHumidityPct = "humidity_pct";
static constexpr const char* kLightLux = "light_lux";

}  // namespace heatrisk::jsonkeys
