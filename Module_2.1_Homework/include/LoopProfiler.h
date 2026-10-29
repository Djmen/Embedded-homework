#pragma once

#include <Arduino.h>

// Collects per-iteration durations (in microseconds) and prints
// avg / min / max to Serial every `reportEvery` iterations.
class LoopProfiler
{
public:
  explicit constexpr LoopProfiler(uint32_t reportEvery) : reportEvery_(reportEvery) {}

  void add(uint32_t durationUs)
  {
    totalUs_ += durationUs;
    minUs_ = min(minUs_, durationUs);
    maxUs_ = max(maxUs_, durationUs);

    if (++iterations_ < reportEvery_)
    {
      return;
    }

    Serial.printf("loop(): avg %.2f us, min %lu us, max %lu us (%lu iterations)\n",
                  static_cast<double>(totalUs_) / iterations_,
                  static_cast<unsigned long>(minUs_),
                  static_cast<unsigned long>(maxUs_),
                  static_cast<unsigned long>(iterations_));
    reset();
  }

private:
  void reset()
  {
    iterations_ = 0;
    totalUs_ = 0;
    minUs_ = UINT32_MAX;
    maxUs_ = 0;
  }

  const uint32_t reportEvery_;
  uint32_t iterations_ = 0;
  uint64_t totalUs_ = 0;
  uint32_t minUs_ = UINT32_MAX;
  uint32_t maxUs_ = 0;
};
