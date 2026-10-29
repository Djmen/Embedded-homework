// Завдання 3. Debounce через перевірку рівня (state-based)
//
// ISR лише сигналізує «була подія». У loop() через SETTLE_MS після події
// перевіряємо рівень: якщо кнопка досі натиснута (LOW) - це справжнє натискання.
// Події при відпусканні відкидаються, бо на момент перевірки кнопка вже HIGH.
// Після прийнятого натискання нова реакція можлива лише тоді, коли кнопку
// відпустили і вона стабільно HIGH. Результат: рівно 1 реакція на 1 натискання.

#include <Arduino.h>

const uint8_t  BTN_PIN   = 16;   // GPIO16 -> кнопка -> GND, pull-up 10 кОм на 3V3
const uint32_t SETTLE_MS = 20;   // через скільки після події перевіряти рівень

volatile bool     eventFlag = false;
volatile uint32_t isrCount  = 0;

void IRAM_ATTR onButtonFalling() {
  isrCount++;
  eventFlag = true;              // лише сигнал «подія»
}

bool     pending      = false;   // чекаємо SETTLE_MS, щоб перевірити рівень
uint32_t eventMs      = 0;
bool     armed        = true;    // чи готові прийняти нове натискання
bool     highTiming   = false;   // відлік стабільного HIGH для повторного «взведення»
uint32_t highSinceMs  = 0;
uint32_t presses      = 0;
uint32_t ignored      = 0;

void setup() {
  Serial.begin(115200);
  pinMode(BTN_PIN, INPUT);       // підтяжка зовнішня 10 кОм
  attachInterrupt(digitalPinToInterrupt(BTN_PIN), onButtonFalling, FALLING);
  delay(1000);
  Serial.println("Завдання 3: debounce через перевірку рівня");
}

void loop() {
  uint32_t now = millis();
  bool pressed = (digitalRead(BTN_PIN) == LOW);

  // 1. Подія від ISR: запускаємо відлік до перевірки (повторні події в цей час поглинаємо)
  if (eventFlag) {
    eventFlag = false;
    if (!pending) {
      pending = true;
      eventMs = now;
    }
  }

  // 2. Перевірка рівня через SETTLE_MS
  if (pending && now - eventMs >= SETTLE_MS) {
    pending = false;
    if (pressed && armed) {
      armed = false;
      presses++;
      Serial.printf("Натискання #%lu (ISR всього %lu)\n",
                    (unsigned long)presses, (unsigned long)isrCount);
    } else if (!pressed) {
      ignored++;
      Serial.printf("  подія при відпусканні - ігнорую (відкинуто %lu)\n", (unsigned long)ignored);
    }
    // pressed && !armed: брязкіт того самого натискання - нічого не робимо
  }

  // 3. Повторне «взведення»: кнопка має бути стабільно відпущена SETTLE_MS
  if (!armed) {
    if (!pressed) {
      if (!highTiming) {
        highTiming = true;
        highSinceMs = now;
      } else if (now - highSinceMs >= SETTLE_MS) {
        armed = true;
        highTiming = false;
      }
    } else {
      highTiming = false;
    }
  }
}
