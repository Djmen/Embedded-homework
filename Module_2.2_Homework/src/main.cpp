// ТЗ-Б, етап 4: хронометр реле (режим Б: COM -> 3V3, NO -> GPIO11)
//
// Затримка = момент замикання контакту NO мінус момент команди, у мілісекундах.
// Момент замикання ловить переривання RISING на GPIO11 (INPUT_PULLDOWN).

#include <Arduino.h>

// ВП-8: піни й активний рівень винесені в константи
const uint8_t RELAY_PIN    = 10;    // ВА-3: GPIO10 -> IN модуля реле
const uint8_t CONTACT_PIN  = 11;    // ВА-5: NO -> GPIO11, COM -> 3V3
const uint8_t RELAY_ACTIVE = LOW;   // ВП-1: активний рівень IN (з етапу 1)
const uint8_t RELAY_IDLE   = (RELAY_ACTIVE == LOW) ? HIGH : LOW;

const int      N_MEASUREMENTS = 10;   // ВП-7
const uint32_t PAUSE_MS       = 500;  // ВП-5
const uint32_t TIMEOUT_MS     = 100;  // ВП-6

// ВП-3: ISR лише записує millis() і ставить прапорець volatile, виводу в ISR немає
volatile bool     contactFlag = false;
volatile uint32_t contactMs   = 0;

void IRAM_ATTR onContact() {
  if (!contactFlag) {        // перший фронт = замикання; наступні (брязкіт) ігноруємо
    contactMs   = millis();
    contactFlag = true;
  }
}

// ВП-5, ВП-6: один вимір. Повертає затримку в мс або -1, якщо контакту немає
long measureOnce() {
  digitalWrite(RELAY_PIN, RELAY_IDLE);    // вимкнути реле
  delay(PAUSE_MS);                        // пауза 500 мс
  contactFlag = false;                    // скинути прапорець

  uint32_t commandMs = millis();          // момент команди
  digitalWrite(RELAY_PIN, RELAY_ACTIVE);  // увімкнути реле

  while (!contactFlag) {                  // дочекатися контакту
    if (millis() - commandMs > TIMEOUT_MS) {
      return -1;                          // тайм-аут
    }
  }
  return (long)(contactMs - commandMs);   // ВП-4
}

// ВП-7: 10 вимірювань підряд, кожне в Serial, після них середнє
void runSeries() {
  Serial.println();
  Serial.printf("Хронометр реле: %d вимірювань\n", N_MEASUREMENTS);

  long sum = 0;
  int  ok  = 0;
  for (int i = 1; i <= N_MEASUREMENTS; i++) {
    long d = measureOnce();
    if (d < 0) {
      Serial.printf("Вимір %2d: ПОМИЛКА, контакт не замкнувся за %lu мс\n",
                    i, (unsigned long)TIMEOUT_MS);
    } else {
      Serial.printf("Вимір %2d: %ld мс\n", i, d);
      sum += d;
      ok++;
    }
  }
  digitalWrite(RELAY_PIN, RELAY_IDLE);

  if (ok > 0) {
    Serial.printf("Середнє: %.1f мс (вдалих %d з %d)\n", (float)sum / ok, ok, N_MEASUREMENTS);
  } else {
    Serial.println("Середнє: немає вдалих вимірів. Перевірте COM -> 3V3, NO -> GPIO11 і RELAY_ACTIVE.");
  }
  Serial.println("Надішліть 'r' у Serial Monitor, щоб повторити.");
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_IDLE);
  pinMode(CONTACT_PIN, INPUT_PULLDOWN);                                    // ВП-2
  attachInterrupt(digitalPinToInterrupt(CONTACT_PIN), onContact, RISING);  // ВП-2
  delay(2000);  // час відкрити Serial Monitor
  runSeries();
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'r' || c == 'R') runSeries();
  }
}