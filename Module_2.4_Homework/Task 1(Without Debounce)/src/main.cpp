// Завдання 1. Базова реалізація без debounce
//
// Схема: GPIO16 -> кнопка -> GND, зовнішній pull-up 10 кОм з GPIO16 на 3V3.
// Відпущена кнопка = HIGH, натиснута = LOW.
//
// Переривання FALLING спрацьовує на кожен перехід HIGH -> LOW. Через брязкіт контактів
// одне натискання дає кілька переривань, а при відпусканні контакт теж «тремтить»
// і дає нові FALLING-фронти.

#include <Arduino.h>

const uint8_t BTN_PIN = 16;

volatile uint32_t isrCount = 0;   // змінюється в ISR, тому volatile

void IRAM_ATTR onButtonFalling() {
  isrCount++;                     // в ISR лише інкремент
}

void setup() {
  Serial.begin(115200);
  pinMode(BTN_PIN, INPUT);        // підтяжка зовнішня 10 кОм (без неї: INPUT_PULLUP)
  attachInterrupt(digitalPinToInterrupt(BTN_PIN), onButtonFalling, FALLING);
  delay(1000);
  Serial.println("Завдання 1: interrupt FALLING без debounce");
}

const uint32_t BURST_QUIET_MS = 100;   // серія переривань закінчилась, якщо 100 мс тиші

uint32_t shown       = 0;   // останнє виведене значення лічильника
uint32_t burstStart  = 0;   // лічильник на початку поточної серії
uint32_t lastIsrMs   = 0;
bool     burstActive = false;

void loop() {
  uint32_t now = millis();
  uint32_t count = isrCount;      // одна копія: значення не зміниться між рядками нижче

  // Лічильник у лог при кожній зміні
  if (count != shown) {
    if (!burstActive) {
      burstActive = true;
      burstStart = shown;
    }
    Serial.printf("ISR count = %lu\n", (unsigned long)count);
    shown = count;
    lastIsrMs = now;
  }

  // Після серії: скільки переривань дала одна дія і що це було, натискання чи відпускання.
  // Рівень перевіряємо вже після брязкоту, тому він правдивий.
  if (burstActive && now - lastIsrMs >= BURST_QUIET_MS) {
    burstActive = false;
    Serial.printf("  серія: +%lu переривань, це було %s\n",
                  (unsigned long)(shown - burstStart),
                  digitalRead(BTN_PIN) == LOW ? "НАТИСКАННЯ" : "ВІДПУСКАННЯ");
  }
}
