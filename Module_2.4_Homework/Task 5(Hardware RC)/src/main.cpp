// Завдання 5. Hardware debounce: порівняння з RC і без RC
//
// Цей скетч одночасно запускає всі чотири методи на одній кнопці і після кожного
// натискання друкує, скільки реакцій дав кожен:
//   ISR    - завдання 1, сирі переривання FALLING
//   ЧАС    - завдання 2, вікно 50 мс поза ISR
//   РІВЕНЬ - завдання 3, перевірка, що кнопка досі натиснута
//   ОПИТ.  - завдання 4, polling 5 мс + машина станів
// Ідеал: 1 реакція на 1 натискання.
//
// Схема з RC: 3V3 -> R1 10 кОм -> GPIO16; GPIO16 -> C1 100 нФ -> GND;
//             GPIO16 -> кнопка -> R2 100 Ом -> GND.
// Без RC: просто приберіть C1 (R2 100 Ом можна лишити або замінити перемичкою).
// Зробіть по 10 натискань у кожному варіанті і порівняйте підсумки.

#include <Arduino.h>

const uint8_t  BTN_PIN          = 16;
const bool     RC_INSTALLED     = true;  // false для серії без RC (лише підпис у лозі)
const uint32_t TIME_DEBOUNCE_MS = 50;    // метод 2
const uint32_t SETTLE_MS        = 20;    // метод 3
const uint32_t POLL_MS          = 5;     // метод 4
const uint8_t  STABLE_SAMPLES   = 4;     // метод 4: 4 x 5 мс = 20 мс
const uint32_t SESSION_QUIET_MS = 300;   // кінець натискання: 300 мс тиші й кнопка відпущена

// ---------- ISR: спільна для методів 1-3 ----------
volatile uint32_t rawCount = 0;
volatile bool     evTime   = false;   // окремий прапорець для методу 2
volatile bool     evState  = false;   // окремий прапорець для методу 3

void IRAM_ATTR onButtonFalling() {
  rawCount++;
  evTime  = true;
  evState = true;
}

// ---------- Метод 2: вікно за часом ----------
uint32_t cntTime = 0, lastTimeAcceptMs = 0;

void methodTime(uint32_t now) {
  if (!evTime) return;
  evTime = false;
  if (now - lastTimeAcceptMs >= TIME_DEBOUNCE_MS) {
    lastTimeAcceptMs = now;
    cntTime++;
  }
}

// ---------- Метод 3: перевірка рівня ----------
uint32_t cntState = 0, stEventMs = 0, stHighSinceMs = 0;
bool     stPending = false, stArmed = true, stHighTiming = false;

void methodState(uint32_t now, bool pressed) {
  if (evState) {
    evState = false;
    if (!stPending) {
      stPending = true;
      stEventMs = now;
    }
  }
  if (stPending && now - stEventMs >= SETTLE_MS) {
    stPending = false;
    if (pressed && stArmed) {
      stArmed = false;
      cntState++;
    }
  }
  if (!stArmed) {
    if (!pressed) {
      if (!stHighTiming) {
        stHighTiming = true;
        stHighSinceMs = now;
      } else if (now - stHighSinceMs >= SETTLE_MS) {
        stArmed = true;
        stHighTiming = false;
      }
    } else {
      stHighTiming = false;
    }
  }
}

// ---------- Метод 4: опитування + машина станів ----------
enum PollState { RELEASED, MAYBE_PRESSED, PRESSED, MAYBE_RELEASED };
PollState pollState = RELEASED;
uint8_t   pollStable = 0;
uint32_t  lastPollMs = 0, cntPoll = 0;

void methodPoll(uint32_t now, bool pressed) {
  if (now - lastPollMs < POLL_MS) return;
  lastPollMs += POLL_MS;
  switch (pollState) {
    case RELEASED:
      if (pressed) { pollState = MAYBE_PRESSED; pollStable = 1; }
      break;
    case MAYBE_PRESSED:
      if (!pressed) pollState = RELEASED;
      else if (++pollStable >= STABLE_SAMPLES) { pollState = PRESSED; cntPoll++; }
      break;
    case PRESSED:
      if (!pressed) { pollState = MAYBE_RELEASED; pollStable = 1; }
      break;
    case MAYBE_RELEASED:
      if (pressed) pollState = PRESSED;
      else if (++pollStable >= STABLE_SAMPLES) pollState = RELEASED;
      break;
  }
}

// ---------- Підсумок кожного натискання ----------
uint32_t sessionNo = 0, lastRawSeen = 0, lastActivityMs = 0;
uint32_t snapRaw = 0, snapTime = 0, snapState = 0, snapPoll = 0;
bool     sessionActive = false;

void setup() {
  Serial.begin(115200);
  pinMode(BTN_PIN, INPUT);   // підтяжка зовнішня 10 кОм
  attachInterrupt(digitalPinToInterrupt(BTN_PIN), onButtonFalling, FALLING);
  delay(1000);
  Serial.printf("Завдання 5: порівняння методів, RC-фільтр: %s\n", RC_INSTALLED ? "Є" : "НЕМАЄ");
  Serial.println(" #  |  ISR | ЧАС | РІВЕНЬ | ОПИТ.");
  lastPollMs = millis();
}

void loop() {
  uint32_t now = millis();
  bool pressed = (digitalRead(BTN_PIN) == LOW);

  methodTime(now);
  methodState(now, pressed);
  methodPoll(now, pressed);

  // Натискання триває, поки є переривання або кнопка натиснута
  uint32_t raw = rawCount;
  if (raw != lastRawSeen || pressed) {
    lastRawSeen = raw;
    lastActivityMs = now;
    sessionActive = true;
  }

  // 300 мс тиші після відпускання: друкуємо, скільки реакцій дав кожен метод
  if (sessionActive && !pressed && now - lastActivityMs >= SESSION_QUIET_MS) {
    sessionActive = false;
    sessionNo++;
    Serial.printf("%3lu | %4lu | %3lu | %6lu | %5lu\n", (unsigned long)sessionNo,
                  (unsigned long)(raw - snapRaw), (unsigned long)(cntTime - snapTime),
                  (unsigned long)(cntState - snapState), (unsigned long)(cntPoll - snapPoll));
    snapRaw = raw;
    snapTime = cntTime;
    snapState = cntState;
    snapPoll = cntPoll;

    if (sessionNo % 10 == 0) {
      Serial.printf("Разом за %lu натискань: ISR %lu, ЧАС %lu, РІВЕНЬ %lu, ОПИТ. %lu\n",
                    (unsigned long)sessionNo, (unsigned long)raw, (unsigned long)cntTime,
                    (unsigned long)cntState, (unsigned long)cntPoll);
    }
  }
}
