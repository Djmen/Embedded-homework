#include <Arduino.h>

#include "Button.h"
#include "Config.h"
#include "Led.h"
#include "LoopProfiler.h"

// The only global state: the flag shared between the ISR and loop().
volatile bool g_buttonPressed = false;

void IRAM_ATTR onButtonPress()
{
  g_buttonPressed = true;
}

enum class LedMode : uint8_t
{
  Blinking,
  AlwaysOn,
  AlwaysOff,
};

LedMode nextMode(LedMode mode)
{
  switch (mode)
  {
  case LedMode::Blinking:
    return LedMode::AlwaysOn;
  case LedMode::AlwaysOn:
    return LedMode::AlwaysOff;
  case LedMode::AlwaysOff:
    return LedMode::Blinking;
  }
  return LedMode::Blinking;
}

const char *modeName(LedMode mode)
{
  switch (mode)
  {
  case LedMode::Blinking:
    return "Blinking";
  case LedMode::AlwaysOn:
    return "Always ON";
  case LedMode::AlwaysOff:
    return "Always OFF";
  }
  return "Unknown";
}

class Application
{
public:
  void init()
  {
    Serial.begin(Config::Serial::kBaudRate);
    led_.init();
    button_.init(onButtonPress);
    Serial.printf("Started, mode: %s\n", modeName(mode_));
  }

  void update()
  {
    const uint32_t startUs = micros();
    const uint32_t nowMs = millis();

    if (button_.wasPressed(nowMs))
    {
      switchMode(nextMode(mode_), nowMs);
    }

    if (mode_ == LedMode::Blinking && nowMs - lastToggleMs_ >= Config::Led::kBlinkIntervalMs)
    {
      lastToggleMs_ = nowMs;
      led_.toggle();
    }

    profiler_.add(micros() - startUs);
  }

private:
  void switchMode(LedMode mode, uint32_t nowMs)
  {
    mode_ = mode;
    Serial.printf("Mode: %s\n", modeName(mode_));

    switch (mode_)
    {
    case LedMode::Blinking:
      lastToggleMs_ = nowMs;
      led_.set(LedState::On);
      break;
    case LedMode::AlwaysOn:
      led_.set(LedState::On);
      break;
    case LedMode::AlwaysOff:
      led_.set(LedState::Off);
      break;
    }
  }

  Led led_{Config::Led::kPin};
  Button button_{Config::Button::kPin, Config::Button::kDebounceMs, g_buttonPressed};
  LoopProfiler profiler_{Config::Profiling::kReportEveryIterations};
  LedMode mode_ = LedMode::Blinking;
  uint32_t lastToggleMs_ = 0;
};

// Function-local static instead of a global object.
Application &app()
{
  static Application instance;
  return instance;
}

void setup()
{
  app().init();
}

void loop()
{
  app().update();
}
