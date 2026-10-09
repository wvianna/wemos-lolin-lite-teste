#include "battery_logic.h"

#include <cstdio>

namespace battery {
namespace {

// Formato exigido pela especificação (docs/memorial.txt §4.1).
// O caractere 'ã' é UTF-8 (2 bytes) e é emitido literalmente.
constexpr char kMessageFormat[] = "Tensão da Bateria: %.2f V";

}  // namespace

float batteryVolts(std::int32_t pinMilliVolts) {
  if (pinMilliVolts <= 0) {
    return 0.0f;
  }
  return (static_cast<float>(pinMilliVolts) / 1000.0f) * kDividerScaleFactor;
}

void MilliVoltAverager::reset() {
  for (std::size_t i = 0; i < kAveragingWindow; ++i) {
    samples_[i] = 0;
  }
  next_ = 0;
  count_ = 0;
}

void MilliVoltAverager::add(std::int32_t pinMilliVolts) {
  samples_[next_] = pinMilliVolts;
  next_ = (next_ + 1) % kAveragingWindow;
  if (count_ < kAveragingWindow) {
    ++count_;
  }
}

std::int32_t MilliVoltAverager::averageMilliVolts() const {
  if (count_ == 0) {
    return 0;
  }
  std::int32_t sum = 0;
  for (std::size_t i = 0; i < count_; ++i) {
    sum += samples_[i];
  }
  return sum / static_cast<std::int32_t>(count_);
}

std::size_t MilliVoltAverager::count() const { return count_; }

int formatBatteryMessage(float volts, char* out, std::size_t outSize) {
  if (out == nullptr || outSize == 0) {
    return -1;
  }
  const int written =
      std::snprintf(out, outSize, kMessageFormat, static_cast<double>(volts));
  if (written < 0 || static_cast<std::size_t>(written) >= outSize) {
    out[outSize - 1] = '\0';
    return -1;
  }
  return written;
}

}  // namespace battery
