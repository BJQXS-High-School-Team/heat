#include "device_registry.h"

#include <string.h>

namespace heatrisk {

DeviceRegistry::DeviceRegistry(uint8_t capacity) {
  capacity_ = (capacity > kMaxStorage) ? kMaxStorage : capacity;
}

int DeviceRegistry::findIndex(const char* device_id) const {
  if (device_id == nullptr || device_id[0] == '\0') {
    return -1;
  }
  for (uint8_t i = 0; i < capacity_; ++i) {
    if (records_[i].occupied && strcmp(records_[i].device_id, device_id) == 0) {
      return i;
    }
  }
  return -1;
}

int DeviceRegistry::findEmptyIndex() const {
  for (uint8_t i = 0; i < capacity_; ++i) {
    if (!records_[i].occupied) {
      return i;
    }
  }
  return -1;
}

bool DeviceRegistry::upsert(const WearableTelemetrySnapshot& snapshot, uint32_t now_ms) {
  int index = findIndex(snapshot.device_id);
  if (index < 0) {
    index = findEmptyIndex();
    if (index < 0) {
      return false;
    }
    records_[index].occupied = true;
    strncpy(records_[index].device_id, snapshot.device_id,
            sizeof(records_[index].device_id) - 1);
    records_[index].device_id[sizeof(records_[index].device_id) - 1] = '\0';
    ++count_;
  }

  records_[index].snapshot = snapshot;
  records_[index].last_seen_ms = now_ms;
  records_[index].online = true;
  return true;
}

void DeviceRegistry::refreshOnlineStatus(uint32_t now_ms, uint32_t offline_timeout_ms) {
  for (uint8_t i = 0; i < capacity_; ++i) {
    if (!records_[i].occupied) {
      continue;
    }
    const uint32_t age_ms = now_ms - records_[i].last_seen_ms;
    records_[i].online = (age_ms <= offline_timeout_ms);
  }
}

DeviceRecord* DeviceRegistry::findMutable(const char* device_id) {
  const int idx = findIndex(device_id);
  return (idx >= 0) ? &records_[idx] : nullptr;
}

const DeviceRecord* DeviceRegistry::find(const char* device_id) const {
  const int idx = findIndex(device_id);
  return (idx >= 0) ? &records_[idx] : nullptr;
}

const DeviceRecord* DeviceRegistry::at(uint8_t index) const {
  if (index >= capacity_ || !records_[index].occupied) {
    return nullptr;
  }
  return &records_[index];
}

uint8_t DeviceRegistry::count() const { return count_; }

uint8_t DeviceRegistry::capacity() const { return capacity_; }

}  // namespace heatrisk
