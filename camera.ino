#include <M5Unified.h>

void setup() {
  Serial.begin(115200);
  delay(500);
}

void loop() {
  Serial.println("hello");

  delay(1000);
}
