// Завдання 4. Polling + debounce як машина станів (без переривань)
//
// Кнопку опитуємо кожні POLL_MS = 5 мс. Новий стан приймається лише тоді,
// коли STABLE_SAMPLES опитувань поспіль дали однаковий рівень (4 x 5 мс = 20 мс).
//
//   RELEASED --LOW--> MAYBE_PRESSED --4 x LOW--> PRESSED (дія!)
//   PRESSED --HIGH--> MAYBE_RELEASED --4 x HIGH--> RELEASED (без дії)
//   Будь-який «відскок» назад повертає у попередній стабільний стан.

#include <Arduino.h>

const uint8_t  BTN_PIN        = 16;  // GPIO16 -> кнопка -> GND, pull-up 10 кОм на 3V3
const uint32_t POLL_MS        = 5;   // період опитування
const uint8_t  STABLE_SAMPLES = 4;   // скільки однакових опитувань поспіль потрібно

enum ButtonState { RELEASED, MAYBE_PRESSED, PRESSED, MAYBE_RELEASED };

ButtonState state       = RELEASED;
uint8_t     stableCount = 0;
uint32_t    lastPollMs  = 0;
uint32_t    firstLowMs  = 0;   // для виміру затримки реакції
uint32_t    presses     = 0;

void setup() {
  Serial.begin(115200);
  pinMode(BTN_PIN, INPUT);      // підтяжка зовнішня 10 кОм; переривань немає
  delay(1000);
  Serial.println("Завдання 4: polling кожні 5 мс + машина станів");
  lastPollMs = millis();
}

void loop() {
  uint32_t now = millis();
  if (now - lastPollMs < POLL_MS) return;   // ще не час опитувати
  lastPollMs += POLL_MS;

  bool low = (digitalRead(BTN_PIN) == LOW);

  switch (state) {
    case RELEASED:
      if (low) {
        state = MAYBE_PRESSED;
        stableCount = 1;
        firstLowMs = now;
      }
      break;

    case MAYBE_PRESSED:
      if (!low) {
        state = RELEASED;                    // короткий сплеск, не натискання
      } else if (++stableCount >= STABLE_SAMPLES) {
        state = PRESSED;
        presses++;                           // ДІЯ: рівно один раз на натискання
        Serial.printf("Натискання #%lu, затримка реакції %lu мс\n",
                      (unsigned long)presses, (unsigned long)(now - firstLowMs));
      }
      break;

    case PRESSED:
      if (!low) {
        state = MAYBE_RELEASED;
        stableCount = 1;
      }
      break;

    case MAYBE_RELEASED:
      if (low) {
        state = PRESSED;                     // брязкіт при відпусканні
      } else if (++stableCount >= STABLE_SAMPLES) {
        state = RELEASED;
        Serial.println("  відпущено (без дії)");
      }
      break;
  }
}
