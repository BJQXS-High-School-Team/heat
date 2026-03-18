#pragma once

#include <stdint.h>

#include "app_config.h"
#include "data_models.h"

namespace heatrisk {

class CommandCache {
 public:
  explicit CommandCache(uint8_t capacity);

  void setForDevice(const char* device_id, const OverrideCommand& command,
                    uint32_t now_ms);
  void setGlobal(const OverrideCommand& command, uint32_t now_ms);
  bool getForDevice(const char* device_id, uint32_t now_ms,
                    OverrideCommand* out_command) const;
  void clearExpired(uint32_t now_ms);

 private:
  struct Entry {
    bool occupied = false;
    char device_id[kMaxDeviceIdLen] = "";
    OverrideCommand command;
  };

  static constexpr uint8_t kMaxStorage = 16;

  int findIndex(const char* device_id) const;
  int findEmptyIndex() const;

  uint8_t capacity_ = 0;
  Entry entries_[kMaxStorage];
  OverrideCommand global_command_;
  bool global_occupied_ = false;
};

}  // namespace heatrisk
