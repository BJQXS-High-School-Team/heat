#include "max30102_algorithm.h"

#include <string.h>

namespace heatrisk {

namespace {

constexpr int32_t kFs = 100;
constexpr int32_t kBufferSize = kFs * 5;
constexpr int32_t kMa4Size = 4;
constexpr int32_t kHammingSize = 5;
constexpr uint16_t kHamm[kHammingSize] = {41, 276, 512, 276, 41};
constexpr uint8_t kSpo2Table[184] = {
    95, 95, 95, 96, 96, 96, 97, 97, 97, 97, 97, 98, 98, 98, 98, 98, 99, 99,
    99, 99, 99, 99, 99, 99, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
    100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 99, 99, 99, 99, 99, 99,
    99, 99, 98, 98, 98, 98, 98, 98, 97, 97, 97, 97, 96, 96, 96, 96, 95, 95,
    95, 94, 94, 94, 93, 93, 93, 92, 92, 92, 91, 91, 90, 90, 89, 89, 89, 88,
    88, 87, 87, 86, 86, 85, 85, 84, 84, 83, 82, 82, 81, 81, 80, 80, 79, 78,
    78, 77, 76, 76, 75, 74, 74, 73, 72, 72, 71, 70, 69, 69, 68, 67, 66, 66,
    65, 64, 63, 62, 62, 61, 60, 59, 58, 57, 56, 56, 55, 54, 53, 52, 51, 50,
    49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 31,
    30, 29, 28, 27, 26, 25, 23, 22, 21, 20, 19, 17, 16, 15, 14, 12, 11, 10,
    9,  7,  6,  5,  3,  2,  1};

static int32_t s_dx[kBufferSize - kMa4Size] = {0};
static int32_t s_x[kBufferSize] = {0};
static int32_t s_y[kBufferSize] = {0};

inline int32_t MinI32(int32_t a, int32_t b) { return (a < b) ? a : b; }

void SortAscend(int32_t* values, int32_t size) {
  for (int32_t i = 1; i < size; ++i) {
    const int32_t temp = values[i];
    int32_t j = i;
    while (j > 0 && temp < values[j - 1]) {
      values[j] = values[j - 1];
      --j;
    }
    values[j] = temp;
  }
}

void SortIndicesDescend(const int32_t* values, int32_t* indices, int32_t size) {
  for (int32_t i = 1; i < size; ++i) {
    const int32_t temp = indices[i];
    int32_t j = i;
    while (j > 0 && values[temp] > values[indices[j - 1]]) {
      indices[j] = indices[j - 1];
      --j;
    }
    indices[j] = temp;
  }
}

void PeaksAboveMinHeight(int32_t* peak_locs, int32_t* peak_count, int32_t* data,
                         int32_t size, int32_t min_height) {
  int32_t i = 1;
  *peak_count = 0;

  while (i < size - 1) {
    if (data[i] > min_height && data[i] > data[i - 1]) {
      int32_t width = 1;
      while ((i + width) < size && data[i] == data[i + width]) {
        ++width;
      }
      if (data[i] > data[i + width] && (*peak_count) < 15) {
        peak_locs[(*peak_count)++] = i;
        i += width + 1;
      } else {
        i += width;
      }
    } else {
      ++i;
    }
  }
}

void RemoveClosePeaks(int32_t* peak_locs, int32_t* peak_count, int32_t* data,
                      int32_t min_distance) {
  SortIndicesDescend(data, peak_locs, *peak_count);

  for (int32_t i = -1; i < *peak_count; ++i) {
    const int32_t old_count = *peak_count;
    *peak_count = i + 1;
    for (int32_t j = i + 1; j < old_count; ++j) {
      const int32_t dist = peak_locs[j] - ((i == -1) ? -1 : peak_locs[i]);
      if (dist > min_distance || dist < -min_distance) {
        peak_locs[(*peak_count)++] = peak_locs[j];
      }
    }
  }

  SortAscend(peak_locs, *peak_count);
}

void FindPeaks(int32_t* peak_locs, int32_t* peak_count, int32_t* data,
               int32_t size, int32_t min_height, int32_t min_distance,
               int32_t max_num) {
  PeaksAboveMinHeight(peak_locs, peak_count, data, size, min_height);
  RemoveClosePeaks(peak_locs, peak_count, data, min_distance);
  *peak_count = MinI32(*peak_count, max_num);
}

}  // namespace

bool ComputeMax30102Metrics(const uint32_t* ir_buffer, const uint32_t* red_buffer,
                            size_t length, Max30102AlgoResult* out_result) {
  if (ir_buffer == nullptr || red_buffer == nullptr || out_result == nullptr ||
      length != static_cast<size_t>(kBufferSize)) {
    return false;
  }

  memset(s_dx, 0, sizeof(s_dx));
  memset(s_x, 0, sizeof(s_x));
  memset(s_y, 0, sizeof(s_y));

  uint32_t ir_mean = 0;
  for (int32_t k = 0; k < kBufferSize; ++k) {
    ir_mean += ir_buffer[k];
  }
  ir_mean /= kBufferSize;

  for (int32_t k = 0; k < kBufferSize; ++k) {
    s_x[k] = static_cast<int32_t>(ir_buffer[k] - ir_mean);
  }

  for (int32_t k = 0; k < kBufferSize - kMa4Size; ++k) {
    s_x[k] = (s_x[k] + s_x[k + 1] + s_x[k + 2] + s_x[k + 3]) / 4;
  }

  for (int32_t k = 0; k < kBufferSize - kMa4Size - 1; ++k) {
    s_dx[k] = s_x[k + 1] - s_x[k];
  }

  for (int32_t k = 0; k < kBufferSize - kMa4Size - 2; ++k) {
    s_dx[k] = (s_dx[k] + s_dx[k + 1]) / 2;
  }

  for (int32_t i = 0; i < kBufferSize - kHammingSize - kMa4Size - 2; ++i) {
    int32_t sum = 0;
    for (int32_t k = i; k < i + kHammingSize; ++k) {
      sum -= s_dx[k] * kHamm[k - i];
    }
    s_dx[i] = sum / 1146;
  }

  int32_t threshold = 0;
  for (int32_t k = 0; k < kBufferSize - kHammingSize; ++k) {
    threshold += (s_dx[k] > 0) ? s_dx[k] : -s_dx[k];
  }
  threshold /= (kBufferSize - kHammingSize);

  int32_t dx_peak_locs[15] = {0};
  int32_t peak_count = 0;
  FindPeaks(dx_peak_locs, &peak_count, s_dx, kBufferSize - kHammingSize,
            threshold, 8, 5);

  out_result->heart_rate_valid = false;
  out_result->spo2_valid = false;

  if (peak_count >= 2) {
    int32_t interval_sum = 0;
    for (int32_t k = 1; k < peak_count; ++k) {
      interval_sum += dx_peak_locs[k] - dx_peak_locs[k - 1];
    }
    const int32_t mean_interval = interval_sum / (peak_count - 1);
    out_result->heart_rate_bpm = static_cast<float>(6000 / mean_interval);
    out_result->heart_rate_valid = true;
  }

  int32_t ir_valley_locs[15] = {0};
  for (int32_t k = 0; k < peak_count; ++k) {
    ir_valley_locs[k] = dx_peak_locs[k] + kHammingSize / 2;
  }

  for (int32_t k = 0; k < kBufferSize; ++k) {
    s_x[k] = static_cast<int32_t>(ir_buffer[k]);
    s_y[k] = static_cast<int32_t>(red_buffer[k]);
  }

  int32_t exact_ir_valley_locs[15] = {0};
  int32_t exact_valley_count = 0;
  for (int32_t k = 0; k < peak_count; ++k) {
    const int32_t center = ir_valley_locs[k];
    int32_t c_min = 16777216;
    bool found = false;
    if (center + 5 < (kBufferSize - kHammingSize) && center - 5 > 0) {
      for (int32_t i = center - 5; i < center + 5; ++i) {
        if (s_x[i] < c_min) {
          c_min = s_x[i];
          exact_ir_valley_locs[k] = i;
          found = true;
        }
      }
      if (found) {
        ++exact_valley_count;
      }
    }
  }

  if (exact_valley_count < 2) {
    return true;
  }

  for (int32_t k = 0; k < kBufferSize - kMa4Size; ++k) {
    s_x[k] = (s_x[k] + s_x[k + 1] + s_x[k + 2] + s_x[k + 3]) / 4;
    s_y[k] = (s_y[k] + s_y[k + 1] + s_y[k + 2] + s_y[k + 3]) / 4;
  }

  int32_t ratios[5] = {0};
  int32_t ratio_count = 0;
  for (int32_t k = 0; k < exact_valley_count - 1; ++k) {
    int32_t y_dc_max = -16777216;
    int32_t x_dc_max = -16777216;
    int32_t y_dc_max_idx = 0;
    int32_t x_dc_max_idx = 0;

    if (exact_ir_valley_locs[k + 1] - exact_ir_valley_locs[k] <= 10) {
      continue;
    }

    for (int32_t i = exact_ir_valley_locs[k];
         i < exact_ir_valley_locs[k + 1]; ++i) {
      if (s_x[i] > x_dc_max) {
        x_dc_max = s_x[i];
        x_dc_max_idx = i;
      }
      if (s_y[i] > y_dc_max) {
        y_dc_max = s_y[i];
        y_dc_max_idx = i;
      }
    }

    int32_t y_ac =
        (s_y[exact_ir_valley_locs[k + 1]] - s_y[exact_ir_valley_locs[k]]) *
        (y_dc_max_idx - exact_ir_valley_locs[k]);
    y_ac = s_y[exact_ir_valley_locs[k]] +
           y_ac / (exact_ir_valley_locs[k + 1] - exact_ir_valley_locs[k]);
    y_ac = s_y[y_dc_max_idx] - y_ac;

    int32_t x_ac =
        (s_x[exact_ir_valley_locs[k + 1]] - s_x[exact_ir_valley_locs[k]]) *
        (x_dc_max_idx - exact_ir_valley_locs[k]);
    x_ac = s_x[exact_ir_valley_locs[k]] +
           x_ac / (exact_ir_valley_locs[k + 1] - exact_ir_valley_locs[k]);
    x_ac = s_x[y_dc_max_idx] - x_ac;

    const int32_t numerator = (y_ac * x_dc_max) >> 7;
    const int32_t denominator = (x_ac * y_dc_max) >> 7;
    if (denominator > 0 && ratio_count < 5 && numerator != 0) {
      ratios[ratio_count++] = (numerator * 100) / denominator;
    }
  }

  if (ratio_count <= 0) {
    return true;
  }

  SortAscend(ratios, ratio_count);
  const int32_t middle = ratio_count / 2;
  int32_t ratio_average = ratios[middle];
  if (middle > 1) {
    ratio_average = (ratios[middle - 1] + ratios[middle]) / 2;
  }

  if (ratio_average > 2 && ratio_average < 184) {
    out_result->spo2_pct = static_cast<float>(kSpo2Table[ratio_average]);
    out_result->spo2_valid = true;
  }

  return true;
}

}  // namespace heatrisk
