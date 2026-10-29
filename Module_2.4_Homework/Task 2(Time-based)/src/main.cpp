// Завдання 2. Software debounce через таймер (time-based)
//
// ISR лише ставить прапорець події. Уся логіка поза ISR, у loop():
// подія приймається, якщо з моменту попередньої ПРИЙНЯТОЇ події минуло >= 50 мс.
// Брязкіт натискання відкидається, але відпускання настає пізніше за 50 мс,
// тому його FALLING-фронти ще можуть дати хибне спрацювання.

#include <Arduino.h>

const uint8_t  BTN_PIN     = 16;   // GPIO16 -> кнопка -> GND, pull-up 10 кОм на 3V3
const uint32_t DEBOUNCE_MS = 50;
const uint32_t SUMMARY_QUIET_MS = 300;  // підсумок, коли кнопка 300 мс у спокої

volatile bool     eventFlag = false;
volatile uint32_t isrCount  = 0;   // «сирі» переривання, для порівняння

void IRAM_ATTR onButtonFalling() {
  isrCount++;
  eventFlag = true;                // ISR лише сигналізує
}

const uint32_t CLASSIFY_MS = 30;  // через 30 мс після прийнятої події дивимось, що це було

uint32_t accepted        = 0;
uint32_t lastAcceptedMs  = 0;
bool     classifyPending = false;
uint32_t lastIsrSeen     = 0;
uint32_t lastActivityMs  = 0;
bool     summaryPending  = false;

void setup() {
  Serial.begin(115200);
  pinMode(BTN_PIN, INPUT);         // підтяжка зовнішня 10 кОм
  attachInterrupt(digitalPinToInterrupt(BTN_PIN), onButtonFalling, FALLING);
  delay(1000);
  Serial.println("Завдання 2: debounce за часом, 50 мс");
}

void loop() {
  uint32_t now = millis();

  // ----- Debounce поза ISR -----
  if (eventFlag) {
    eventFlag = false;
    if (now - lastAcceptedMs >= DEBOUNCE_MS) {
      lastAcceptedMs = now;
      accepted++;
      classifyPending = true;
      Serial.printf("Прийнято #%lu\n", (unsigned long)accepted);
    }
    // інакше: менше 50 мс від попередньої прийнятої події - ігноруємо
  }

  // ----- Що це було: натискання чи відпускання (перевіряємо після брязкоту) -----
  if (classifyPending && now - lastAcceptedMs >= CLASSIFY_MS) {
    classifyPending = false;
    if (digitalRead(BTN_PIN) == LOW) {
      Serial.println("  це натискання");
    } else {
      Serial.println("  це ВІДПУСКАННЯ: хибне спрацювання");
    }
  }

  // ----- Підсумок після серії: скільки переривань відкинуто -----
  uint32_t isr = isrCount;
  if (isr != lastIsrSeen) {
    lastIsrSeen = isr;
    lastActivityMs = now;
    summaryPending = true;
  }
  if (summaryPending && now - lastActivityMs >= SUMMARY_QUIET_MS) {
    summaryPending = false;
    Serial.printf("  Разом: ISR %lu, прийнято %lu, відкинуто %lu\n",
                  (unsigned long)isr, (unsigned long)accepted, (unsigned long)(isr - accepted));
  }
}
