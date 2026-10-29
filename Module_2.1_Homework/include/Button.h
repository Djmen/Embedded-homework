#pragma once

#include <Arduino.h>

// Interrupt-driven button. The ISR only raises a flag; debouncing happens in loop().
// Debounce strategy: after an edge, wait kDebounceMs and accept the press only if
// the pin is still LOW. This also rejects bounces generated while releasing.
class Button
{
public:
  using Isr = void (*)();

  Button(uint8_t pin, uint32_t debounceMs, volatile bool &pressedFlag)
      : pin_(pin), debounceMs_(debounceMs), pressedFlag_(pressedFlag) {}

  void init(Isr isr)
  {
    pinMode(pin_, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(pin_), isr, FALLING);
  }

  // Returns true once per debounced press.
  bool wasPressed(uint32_t nowMs)
  {
    if (pressedFlag_)
    {
      pressedFlag_ = false;
      if (!pending_)
      {
        pending_ = true;
        pendingSinceMs_ = nowMs;
      }
    }

    if (pending_ && nowMs - pendingSinceMs_ >= debounceMs_)
    {
      pending_ = false;
      return digitalRead(pin_) == LOW;
    }

    return false;
  }

private:
  const uint8_t pin_;
  const uint32_t debounceMs_;
  volatile bool &pressedFlag_;
  bool pending_ = false;
  uint32_t pendingSinceMs_ = 0;
};
