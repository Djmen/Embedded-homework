#pragma once

#include <Arduino.h>

// All tunable parameters live here, so the rest of the code has no magic numbers.
struct Config
{
  struct Serial
  {
    static constexpr uint32_t kBaudRate = 115200;
  };

  struct Led
  {
    static constexpr uint8_t kPin = 16;
    static constexpr uint32_t kBlinkIntervalMs = 500;
  };

  struct Button
  {
    // GPIO0 is the on-board BOOT button of the ESP32-S3-DevKitC-1 (active LOW).
    // An external button wired between any free GPIO and GND works the same way.
    static constexpr uint8_t kPin = 0;
    static constexpr uint32_t kDebounceMs = 50;
  };

  struct Profiling
  {
    static constexpr uint32_t kReportEveryIterations = 1000;
  };
};
