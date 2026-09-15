#include <M5Unified.h>
#include <WiFi.h>

// AP info
const char *ssid = "*";
const char *pass = "*";

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  Serial.begin(115200);
  delay(500);

  WiFi.begin(ssid, pass);
  
  Serial.println("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }
  Serial.printf("Connected: SSID=%s\n", ssid);

  Serial.printf("IP=");
  Serial.println(WiFi.localIP());
}

void loop() {
  delay(1000);
}
