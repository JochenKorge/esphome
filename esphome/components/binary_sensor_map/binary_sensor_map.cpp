#include "binary_sensor_map.h"
#include "esphome/core/log.h"

namespace esphome {
namespace binary_sensor_map {

static const char *const TAG = "binary_sensor_map";

void BinarySensorMap::dump_config() { LOG_SENSOR("  ", "binary_sensor_map", this); }

void BinarySensorMap::loop() {
  switch (this->sensor_type_) {
    case BINARY_SENSOR_MAP_TYPE_GROUP:
      this->process_group_();
      break;
    case BINARY_SENSOR_MAP_TYPE_SUM:
      this->process_sum_();
      break;
  }
}

void BinarySensorMap::process_group_() {
  float total_current_value = 0.0;
  uint8_t num_active_sensors = 0;
  uint64_t mask = 0x00;
  // check all binary_sensors for its state. when active add its value to total_current_value.
  // create a bitmask for the binary_sensor status on all channels
  for (size_t i = 0; i < this->channels_.size(); i++) {
    auto bs = this->channels_[i];
    if (bs.binary_sensor->state) {
      num_active_sensors++;
      total_current_value += bs.sensor_value;
      mask |= 1ULL << i;
    }
  }
  // check if the sensor map was touched
  if (mask != 0ULL) {
    // did the bit_mask change or is it a new sensor touch
    if (this->last_mask_ != mask) {
      float publish_value = total_current_value / num_active_sensors;
      this->publish_state(publish_value);
    }
  } else if (this->last_mask_ != 0ULL) {
    // is this a new sensor release
    ESP_LOGV(TAG, "'%s' - No binary sensor active, publishing NAN", this->name_.c_str());
    this->publish_state(NAN);
  }
  this->last_mask_ = mask;
}

void BinarySensorMap::process_sum_() {
  float total_current_value = 0.0;
  uint64_t mask = 0x00;
  // - check all binary_sensor states
  // - if active, add its value to total_current_value
  // - creates a bitmask for the binary_sensor status on all channels
  for (size_t i = 0; i < this->channels_.size(); i++) {
    auto bs = this->channels_[i];
    if (bs.binary_sensor->state) {
      total_current_value += bs.sensor_value;
      mask |= 1ULL << i;
    }
  }

  // update state only if the binary sensor states have changed or if no state has ever been sent on boot
  if ((this->last_mask_ != mask) || (!this->has_state())) {
    this->publish_state(total_current_value);
  }

  this->last_mask_ = mask;
}

void BinarySensorMap::add_channel(binary_sensor::BinarySensor *sensor, float value) {
  BinarySensorMapChannel sensor_channel{
      .binary_sensor = sensor,
      .sensor_value = value,
  };
  this->channels_.push_back(sensor_channel);
}

void BinarySensorMap::set_sensor_type(BinarySensorMapType sensor_type) { this->sensor_type_ = sensor_type; }

}  // namespace binary_sensor_map
}  // namespace esphome
