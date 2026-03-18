#pragma once

#include <stdint.h>

#include "app_config.h"
#include "data_models.h"

namespace heatrisk {

struct DeviceRecord {
  bool occupied = false;
  char device_id[kMaxDeviceIdLen] = "";
  WearableTelemetrySnapshot snapshot;
  uint32_t last_seen_ms = 0;
  bool online = false;
};

class DeviceRegistry {
 public:
  explicit DeviceRegistry(uint8_t capacity);

  bool upsert(const WearableTelemetrySnapshot& snapshot, uint32_t now_ms);
  void refreshOnlineStatus(uint32_t now_ms, uint32_t offline_timeout_ms);

  DeviceRecord* findMutable(const char* device_id);
  const DeviceRecord* find(const char* device_id) const;
  const DeviceRecord* at(uint8_t index) const;

  uint8_t count() const;
  uint8_t capacity() const;

 private:
  static constexpr uint8_t kMaxStorage = 16;

  int findIndex(const char* device_id) const;
  int findEmptyIndex() const;

  uint8_t capacity_ = 0;
  uint8_t count_ = 0;
  DeviceRecord records_[kMaxStorage];
};

}  // namespace heatrisk
