#include <Arduino.h>

// Світлофор за ПДР України (ESP32-S3, Arduino Framework).
// Регульований цикл: зелений → зелений миготливий → жовтий → червоний → червоний + жовтий → зелений.
// Кнопка вмикає жовтий миготливий; повторне натискання повертає цикл туди, де його перервали.
// Час відмірює апаратний таймер: переривання кожні 500 мс ("тік").

// ---------- Піни ----------
constexpr uint8_t PIN_RED    = 15;
constexpr uint8_t PIN_YELLOW = 16;
constexpr uint8_t PIN_GREEN  = 17;
constexpr uint8_t PIN_BUTTON = 18;  // кнопка до GND, внутрішня підтяжка до 3,3 В

// ---------- Час ----------
constexpr uint32_t TICK_MS     = 500;  // період таймера = половина періоду блимання
constexpr uint32_t DEBOUNCE_MS = 30;   // скільки рівень кнопки має бути стабільним

constexpr uint32_t msToTicks(uint32_t ms) { return ms / TICK_MS; }

// ---------- Стани ----------
enum class State : uint8_t {
  Green,        // рух дозволено
  GreenBlink,   // дозвіл скоро закінчиться
  Yellow,       // заборона, попередження про зміну
  Red,          // заборона
  RedYellow,    // заборона, скоро зелений
  YellowBlink,  // нерегульований режим (вмикається кнопкою)
};

// Тривалість стану в тіках. 0 = без обмеження (виходить лише кнопкою).
uint32_t durationTicks(State s) {
  switch (s) {
    case State::Green:       return msToTicks(10000);
    case State::GreenBlink:  return msToTicks(3000);  // 6 тіків = 3 блимання
    case State::Yellow:      return msToTicks(3000);
    case State::Red:         return msToTicks(10000);
    case State::RedYellow:   return msToTicks(2000);
    case State::YellowBlink: return 0;
  }
  return 0;
}

State nextState(State s) {
  switch (s) {
    case State::Green:       return State::GreenBlink;
    case State::GreenBlink:  return State::Yellow;
    case State::Yellow:      return State::Red;
    case State::Red:         return State::RedYellow;
    case State::RedYellow:   return State::Green;
    case State::YellowBlink: return State::YellowBlink;  // сам не виходить
  }
  return State::Red;
}

const char *stateName(State s) {
  switch (s) {
    case State::Green:       return "GREEN";
    case State::GreenBlink:  return "GREEN_BLINK";
    case State::Yellow:      return "YELLOW";
    case State::Red:         return "RED";
    case State::RedYellow:   return "RED_YELLOW";
    case State::YellowBlink: return "YELLOW_BLINK";
  }
  return "?";
}

// ---------- Апаратний таймер ----------
hw_timer_t *timer = nullptr;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t pendingTicks = 0;

// Переривання лише рахує тіки; вся логіка — у loop().
void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  pendingTicks++;
  portEXIT_CRITICAL_ISR(&timerMux);
}

void setupTimer() {
  timer = timerBegin(0, 80, true);              // таймер 0, 80 МГц / 80 = 1 МГц
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, TICK_MS * 1000, true); // кожні TICK_MS, автоперезапуск
  timerAlarmEnable(timer);
}

// ---------- Світлофор ----------
State state = State::Green;
uint32_t ticksInState = 0;

// Де був регульований цикл, коли ввімкнули жовтий миготливий.
State savedState = State::Green;
uint32_t savedTicks = 0;

void setLamps(bool red, bool yellow, bool green) {
  digitalWrite(PIN_RED, red);
  digitalWrite(PIN_YELLOW, yellow);
  digitalWrite(PIN_GREEN, green);
}

// Лампи повністю визначаються станом і кількістю тіків у ньому.
// Тому відновлення збереженого стану відновлює й лампи, разом із фазою блимання.
void applyLamps() {
  const bool oddTick = ticksInState % 2 == 1;

  switch (state) {
    case State::Green:       setLamps(false, false,    true);    break;
    case State::GreenBlink:  setLamps(false, false,    oddTick); break;  // починає з вимкненого
    case State::Yellow:      setLamps(false, true,     false);   break;
    case State::Red:         setLamps(true,  false,    false);   break;
    case State::RedYellow:   setLamps(true,  true,     false);   break;
    case State::YellowBlink: setLamps(false, !oddTick, false);   break;  // починає з увімкненого
  }
}

void enterState(State s) {
  state = s;
  ticksInState = 0;
  applyLamps();
  Serial.printf("[%7lu ms] %s\n", millis(), stateName(state));
}

void onTick() {
  ticksInState++;

  const uint32_t duration = durationTicks(state);
  if (duration != 0 && ticksInState >= duration) {
    enterState(nextState(state));
  } else {
    applyLamps();  // для миготливих станів перемикає лампу
  }
}

// Кнопка: вхід у жовтий миготливий зі збереженням циклу або повернення в нього.
void toggleYellowBlink() {
  if (state == State::YellowBlink) {
    state = savedState;
    ticksInState = savedTicks;
    applyLamps();
    Serial.printf("[%7lu ms] %s (resumed at %lu/%lu ticks)\n", millis(), stateName(state),
                  (unsigned long)ticksInState, (unsigned long)durationTicks(state));
  } else {
    savedState = state;
    savedTicks = ticksInState;
    state = State::YellowBlink;
    ticksInState = 0;
    applyLamps();
    Serial.printf("[%7lu ms] %s (saved %s at %lu/%lu ticks)\n", millis(), stateName(state),
                  stateName(savedState), (unsigned long)savedTicks,
                  (unsigned long)durationTicks(savedState));
  }
}

// ---------- Кнопка (опитування з антибрязкотом) ----------
bool lastReading = HIGH;   // останнє прочитане значення піна
bool stableLevel = HIGH;   // значення, що протрималося DEBOUNCE_MS
uint32_t lastChangeMs = 0;

// true рівно один раз на кожне натискання.
bool buttonPressed() {
  const bool reading = digitalRead(PIN_BUTTON);

  if (reading != lastReading) {  // рівень змінився: контакти ще брязкотять
    lastReading = reading;
    lastChangeMs = millis();
  }

  if (millis() - lastChangeMs >= DEBOUNCE_MS && reading != stableLevel) {
    stableLevel = reading;
    return stableLevel == LOW;  // з INPUT_PULLUP натиснута кнопка = LOW
  }
  return false;
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_YELLOW, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  enterState(State::Green);
  setupTimer();
}

void loop() {
  // Забираємо тіки, що накопичилися з переривання.
  portENTER_CRITICAL(&timerMux);
  uint32_t ticks = pendingTicks;
  pendingTicks = 0;
  portEXIT_CRITICAL(&timerMux);

  while (ticks--) {
    onTick();
  }

  if (buttonPressed()) {
    toggleYellowBlink();
  }
}