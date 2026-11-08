// ДЗ 2.5 «Таймери»: витяжний вентилятор на ESP32-S3 (Arduino Framework)
//
// Вентилятор (мотор через 2N2222A на GPIO5) вмикається раз на FAN_PERIOD_MS
// і працює FAN_ON_TIME_MS, після чого вимикається сам.
//
// Як це працює:
//   periodTimer - періодичний таймер esp_timer. Раз на період вмикає
//                 вентилятор і запускає offTimer.
//   offTimer    - одноразовий таймер esp_timer. Через FAN_ON_TIME_MS
//                 вимикає вентилятор.
//
// esp_timer на ESP32-S3 працює на апаратному системному таймері (SYSTIMER).
// Колбеки викликає системна служба esp_timer, тому цикл повторюється сам:
// без loop(), без власних задач і без delay(). Щоб це показати, loop()
// навмисно зайнятий важкими обчисленнями, які ніяк не стосуються вентилятора.

#include <Arduino.h>
#include "esp_timer.h"

// ---------------- Налаштування ----------------
#define FAN_PIN        5
#define FAN_ON_LEVEL   HIGH   // HIGH відкриває транзисторний ключ

// 1 = режим налагодження: година -> 12 с, 15 хв -> 3 с (та сама пропорція 1/4)
#define DEBUG_FAST     1

#if DEBUG_FAST
  #define FAN_PERIOD_MS   (12UL * 1000UL)
  #define FAN_ON_TIME_MS  (3UL * 1000UL)
#else
  #define FAN_PERIOD_MS   (60UL * 60UL * 1000UL)   // 1 година
  #define FAN_ON_TIME_MS  (15UL * 60UL * 1000UL)   // 15 хвилин
#endif

// 1 = loop() виконує сторонню важку роботу (демонстрація незалежності таймерів)
#ifndef LOOP_BUSY_DEMO
#define LOOP_BUSY_DEMO 1
#endif

static_assert(FAN_ON_TIME_MS > 0, "FAN_ON_TIME_MS має бути більше 0");
static_assert(FAN_ON_TIME_MS < FAN_PERIOD_MS, "FAN_ON_TIME_MS має бути менше за FAN_PERIOD_MS");

// ---------------- Таймери ----------------
static esp_timer_handle_t periodTimer = nullptr;
static esp_timer_handle_t offTimer    = nullptr;
static volatile uint32_t  cycleCount  = 0;

static inline uint32_t nowMs() {
  return (uint32_t)(esp_timer_get_time() / 1000);
}

static void fanWrite(bool on) {
  digitalWrite(FAN_PIN, on ? FAN_ON_LEVEL : !FAN_ON_LEVEL);
}

// Через FAN_ON_TIME_MS після ввімкнення
static void onFanOffTimer(void * /*arg*/) {
  fanWrite(false);
  Serial.printf("[%8lu ms] вентилятор OFF, наступне ввімкнення через %lu с\n",
                (unsigned long)nowMs(),
                (unsigned long)((FAN_PERIOD_MS - FAN_ON_TIME_MS) / 1000));
}

// Раз на FAN_PERIOD_MS
static void onPeriodTimer(void * /*arg*/) {
  fanWrite(true);
  cycleCount++;
  esp_timer_stop(offTimer);   // про всяк випадок; за static_assert він уже відпрацював
  esp_timer_start_once(offTimer, (uint64_t)FAN_ON_TIME_MS * 1000ULL);
  Serial.printf("[%8lu ms] вентилятор ON, цикл %lu, вимкнеться через %lu с\n",
                (unsigned long)nowMs(), (unsigned long)cycleCount,
                (unsigned long)(FAN_ON_TIME_MS / 1000));
}

static void startFanTimers() {
  esp_timer_create_args_t offArgs = {};
  offArgs.callback = &onFanOffTimer;
  offArgs.name = "fan_off";
  ESP_ERROR_CHECK(esp_timer_create(&offArgs, &offTimer));

  esp_timer_create_args_t periodArgs = {};
  periodArgs.callback = &onPeriodTimer;
  periodArgs.name = "fan_period";
  ESP_ERROR_CHECK(esp_timer_create(&periodArgs, &periodTimer));

  // Перший цикл одразу після старту, далі кожні FAN_PERIOD_MS
  ESP_ERROR_CHECK(esp_timer_start_periodic(periodTimer, (uint64_t)FAN_PERIOD_MS * 1000ULL));
  onPeriodTimer(nullptr);
}

// ---------------- Стороння робота для loop() ----------------
#if LOOP_BUSY_DEMO
static uint32_t countPrimes(uint32_t limit) {
  uint32_t count = 0;
  for (uint32_t n = 2; n < limit; n++) {
    bool prime = true;
    for (uint32_t d = 2; d * d <= n; d++) {
      if (n % d == 0) { prime = false; break; }
    }
    if (prime) count++;
  }
  return count;
}
#endif

void setup() {
  pinMode(FAN_PIN, OUTPUT);
  fanWrite(false);

  Serial.begin(115200);
  while (!Serial && millis() < 2000) {
    // чекаємо Serial Monitor не довше 2 с
  }
  Serial.printf("\nВитяжний вентилятор: період %lu с, робота %lu с%s\n",
                (unsigned long)(FAN_PERIOD_MS / 1000), (unsigned long)(FAN_ON_TIME_MS / 1000),
                DEBUG_FAST ? " (режим налагодження)" : "");

  startFanTimers();
}

void loop() {
  // Вентилятором loop() не керує. Тут лише стороння робота.
#if LOOP_BUSY_DEMO
  uint32_t t0 = nowMs();
  uint32_t primes = countPrimes(400000);
  Serial.printf("[%8lu ms]   loop: прості числа до 400000 = %lu, рахував %lu мс\n",
                (unsigned long)nowMs(), (unsigned long)primes, (unsigned long)(nowMs() - t0));
#endif
}
