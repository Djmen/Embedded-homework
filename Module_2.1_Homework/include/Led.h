#pragma once

#include <Arduino.h>

enum class LedState : uint8_t
{
  Off,
  On,
};

class Led
{
public:
  explicit constexpr Led(uint8_t pin) : pin_(pin) {}

  void init()
  {
    pinMode(pin_, OUTPUT);
    set(LedState::Off);
  }

  void set(LedState state)
  {
    state_ = state;
    digitalWrite(pin_, state_ == LedState::On ? HIGH : LOW);
  }

  void toggle() { set(state_ == LedState::On ? LedState::Off : LedState::On); }

  LedState state() const { return state_; }

private:
  const uint8_t pin_;
  LedState state_ = LedState::Off;
};
