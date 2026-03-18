#include "command_cache.h"

#include <string.h>

#include "status_logic.h"

namespace heatrisk {

CommandCache::CommandCache(uint8_t capacity) {
  capacity_ = (capacity > kMaxStorage) ? kMaxStorage : capacity;
}

int CommandCache::findIndex(const char* device_id) const {
  if (device_id == nullptr || device_id[0] == '\0') {
    return -1;
  }
  for (uint8_t i = 0; i < capacity_; ++i) {
    if (entries_[i].occupied && strcmp(entries_[i].device_id, device_id) == 0) {
      return i;
    }
  }
  return -1;
}

int CommandCache::findEmptyIndex() const {
  for (uint8_t i = 0; i < capacity_; ++i) {
    if (!entries_[i].occupied) {
      return i;
    }
  }
  return -1;
}

void CommandCache::setForDevice(const char* device_id,
                                const OverrideCommand& command, uint32_t now_ms) {
  if (device_id == nullptr || device_id[0] == '\0') {
    return;
  }

  int idx = findIndex(device_id);
  if (idx < 0) {
    idx = findEmptyIndex();
    if (idx < 0) {
      return;
    }
    entries_[idx].occupied = true;
    strncpy(entries_[idx].device_id, device_id,
            sizeof(entries_[idx].device_id) - 1);
    entries_[idx].device_id[sizeof(entries_[idx].device_id) - 1] = '\0';
  }
  entries_[idx].command = command;
  entries_[idx].command.issued_at_ms = now_ms;
}

void CommandCache::setGlobal(const OverrideCommand& command, uint32_t now_ms) {
  global_command_ = command;
  global_command_.issued_at_ms = now_ms;
  global_occupied_ = true;
}

bool CommandCache::getForDevice(const char* device_id, uint32_t now_ms,
                                OverrideCommand* out_command) const {
  if (out_command == nullptr) {
    return false;
  }

  const int idx = findIndex(device_id);
  if (idx >= 0) {
    const OverrideCommand& cmd = entries_[idx].command;
    if (IsOverrideActive(cmd, now_ms)) {
      *out_command = cmd;
      return true;
    }
  }

  if (global_occupied_ && IsOverrideActive(global_command_, now_ms)) {
    *out_command = global_command_;
    return true;
  }
  return false;
}

void CommandCache::clearExpired(uint32_t now_ms) {
  for (uint8_t i = 0; i < capacity_; ++i) {
    if (!entries_[i].occupied) {
      continue;
    }
    if (!IsOverrideActive(entries_[i].command, now_ms)) {
      entries_[i].occupied = false;
      entries_[i].device_id[0] = '\0';
    }
  }

  if (global_occupied_ && !IsOverrideActive(global_command_, now_ms)) {
    global_occupied_ = false;
  }
}

}  // namespace heatrisk
