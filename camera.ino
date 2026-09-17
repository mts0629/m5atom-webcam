#include <M5Unified.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <WebServer.h>

// AP info
const char *ssid = "***";
const char *pass = "***";

WebServer server(80);

// index.html
static char index_html[] = R"(
<!DOCTYPE html>
  <head>
    <meta charset="UTF-8">
    <title>Hello</title>
  </head>
  <body>
    <h1>hello world</h1>
  </body>
</html>
)";

// Handler
void handler_root() {
  server.send(200, "text/html", index_html);
}

void handler_not_found() {
  server.send(404, "text/plain", "404 Not Found");
}

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

  // Configure and start server
  server.on("/", handler_root);
  server.onNotFound(handler_not_found);
  server.begin();
}

void loop() {
  M5.update();

  server.handleClient();

  delay(1000);
}
