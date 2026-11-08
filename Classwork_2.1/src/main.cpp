#include <Arduino.h>

// Синій світлодіод на PC13 світиться, коли на піні LOW
void setup() {
  pinMode(PC13, OUTPUT);
}

void loop() {
  digitalWrite(PC13, LOW);   // увімкнути
  delay(100);
  digitalWrite(PC13, HIGH);  // вимкнути
  delay(900);
}