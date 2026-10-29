#include <Arduino.h>

// Піни й періоди (мс)
const uint8_t  LED1_PIN = 15;   const uint32_t LED1_PERIOD = 200;
const uint8_t  LED2_PIN = 16;   const uint32_t LED2_PERIOD = 500;
const uint8_t  LED3_PIN = 17;   const uint32_t LED3_PERIOD = 1000;
const uint32_t STATS_PERIOD = 1000;  // як часто друкувати кількість проходів loop()

// Кожен LED має свій незалежний таймер: момент останньої зміни + поточний стан
uint32_t led1Last = 0;  bool led1On = false;
uint32_t led2Last = 0;  bool led2On = false;
uint32_t led3Last = 0;  bool led3On = false;

uint32_t statsLast = 0;
uint32_t loopCount = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);

  uint32_t now = millis();
  led1Last = led2Last = led3Last = statsLast = now;
}

void loop() {
  uint32_t now = millis();
  loopCount++;

  // LED1: кожні 200 мс
  if (now - led1Last >= LED1_PERIOD) {
    led1Last += LED1_PERIOD;   // += замість = now: похибка не накопичується, ритм не «пливе»
    led1On = !led1On;
    digitalWrite(LED1_PIN, led1On);
  }

  // LED2: кожні 500 мс
  if (now - led2Last >= LED2_PERIOD) {
    led2Last += LED2_PERIOD;
    led2On = !led2On;
    digitalWrite(LED2_PIN, led2On);
  }

  // LED3: кожні 1000 мс
  if (now - led3Last >= LED3_PERIOD) {
    led3Last += LED3_PERIOD;
    led3On = !led3On;
    digitalWrite(LED3_PIN, led3On);
  }

  // Раз на секунду: скільки разів встиг прокрутитися loop()
  if (now - statsLast >= STATS_PERIOD) {
    statsLast += STATS_PERIOD;
    Serial.printf("Проходів loop() за секунду: %lu\n", (unsigned long)loopCount);
    loopCount = 0;
  }
}