#include <M5Unified.h>
#include <WiFi.h>
#include <HTTPClient.h>

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

  Serial.print("IP=");
  Serial.println(WiFi.localIP());
}

const char *uri = "https://example.com/";

void loop() {
  M5.update();

  // Test access
  HTTPClient http;
  http.begin(uri);
  int http_code = http.GET();
  if (http_code == HTTP_CODE_OK) {
    auto payload = http.getString();
    Serial.println(payload);
  } else {
    Serial.println("HTTP GET failed");
  }

  delay(10000);
}
