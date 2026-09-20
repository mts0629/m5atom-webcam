#include <M5Unified.h>
#include <WiFi.h>
#include <WebServer.h>

#include <esp_camera.h>

// AP info
const char *ssid = "***";
const char *pass = "***";

// Server instance
WiFiServer server(80);

void capture(WiFiClient *client) {
  // Get a frame
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("[ERROR] capture failed");
    return;
  }

  // Convert to JPEG
  uint8_t *jpg = NULL;
  size_t jpg_len = 0;
  frame2jpg(fb, 80, &jpg, &jpg_len);

  // Send the JPEG
  client->println("HTTP/1.1 200 OK");
  client->printf(
    "Content-Type: image/jpeg\r\nContent-Length: %u\r\n", jpg_len
  );
  client->println("Content-Disposition: inline; filename=capture.jpg");
  client->println();
  client->write(jpg, jpg_len);

  free(jpg);

  esp_camera_fb_return(fb);
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  // Start serial
  Serial.begin(115200);
  delay(500);

  // On POWER_N
  pinMode(18, OUTPUT);
  digitalWrite(18, LOW);
  delay(500);

  // Camera configuration
  camera_config_t cam_cfg = {
    .pin_pwdn = -1,
    .pin_reset = -1,
    .pin_xclk = 21,
    .pin_sccb_sda = 12,
    .pin_sccb_scl = 9,
    .pin_d7 = 13,
    .pin_d6 = 11,
    .pin_d5 = 17,
    .pin_d4 = 4,
    .pin_d3 = 48,
    .pin_d2 = 46,
    .pin_d1 = 42,
    .pin_d0 = 3,

    .pin_vsync = 10,
    .pin_href = 14,
    .pin_pclk = 40,

    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_RGB565,
    .frame_size = FRAMESIZE_240X240,

    .jpeg_quality = 12,
    .fb_count = 2,
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_LATEST,

    .sccb_i2c_port = 0,
  };

  // Release I2C port (when using M5Unified)
  M5.In_I2C.release();

  // Initialize the camera
  esp_err_t err = esp_camera_init(&cam_cfg);
  if (err != ESP_OK) {
    Serial.printf("[ERROR] camera init failed: error code=0x%x\n", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  // Flip vertically
  s->set_vflip(s, 1);

  // Establish WiFi connection
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  WiFi.setSleep(false);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.printf("Connected: SSID=%s\n", ssid);
  Serial.print("IP=");
  Serial.println(WiFi.localIP());

  // Start the server
  server.begin();
}

void loop() {
  M5.update();

  WiFiClient client = server.accept();
  if (client) {
    if (client.connected() && client.available()) {
      capture(&client);
    }

    client.stop();
  }
}
