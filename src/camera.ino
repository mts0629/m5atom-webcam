#include <M5Unified.h>
#include <WiFi.h>

#include <esp_camera.h>

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
    frame2jpg(fb, 50, &jpg, &jpg_len);

    // Send the JPEG
    client->println("HTTP/1.1 200 OK");
    client->printf(
        "Content-Type: image/jpeg\r\nContent-Length: %u\r\n", jpg_len
    );
    client->println("Content-Disposition: inline; filename=capture.jpg");
    client->println();
    client->write(jpg, jpg_len);

    // Wait finish of data transfer
    delay(100);

    free(jpg);
    jpg = NULL;

    esp_camera_fb_return(fb);
    fb = NULL;
}

#define FRAME_BOUNDARY "__boundary__"

// Capture a stream
void capture_stream(WiFiClient *client) {
    client->println("HTTP/1.1 200 OK");
    // MotionJPEG stream
    client->printf(
        "Content-Type: multipart/x-mixed-replace; boundary=%s\r\n",
        FRAME_BOUNDARY
    );
    client->println("Content-Disposition: inline; filename=capture.jpg");
    client->println("Access-Control-Allow-Origin: *"); // Allow all origins
    client->println();

    camera_fb_t *fb;
    uint8_t *jpg = NULL;
    size_t jpg_len = 0;
    constexpr size_t PACKET_LEN = 8192;
    while (client->available()) {
        // Get a frame
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("[ERROR] capture failed");
            continue;
        }

        // Convert to JPEG
        frame2jpg(fb, 50, &jpg, &jpg_len);

        client->print("--" FRAME_BOUNDARY "\r\n");
        client->printf(
            "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", jpg_len
        );

        uint8_t *p = jpg;
        while (true) {
            // Transfer the frame by packets
            size_t send_len = (jpg_len > PACKET_LEN) ? PACKET_LEN : jpg_len;
            size_t sent_len = client->write(p, send_len);
            if (sent_len == 0) {
                break;
            }
            p += sent_len;
            jpg_len -= sent_len;
        }

        if (fb) {
            esp_camera_fb_return(fb);
            fb = NULL;
        }

        if (jpg) {
            free(jpg);
            jpg = NULL;
            jpg_len = 0;
        }
    }

    if (fb) {
        esp_camera_fb_return(fb);
        fb = NULL;
    }

    free(jpg);
    jpg = NULL;
}

// #define STA_MODE
#define AP_MODE

// Connect to an AP with specified SSID and password
void connect_to_ap(const char *ssid, const char *password, const int try_sec) {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(false, true);

    WiFi.begin(ssid, password);

    int i = 0;
    Serial.print("Connecting");
    auto status = WiFi.status();
    while ((status != WL_CONNECTED) || (i > try_sec)) {
        Serial.print(".");
        i++;
        delay(1000);
        status = WiFi.status();
    }

    if (status != WL_CONNECTED) {
        Serial.printf("\nConnection failed: SSID=%s\n", ssid);
        return;
    }

    Serial.printf("\nConnected: SSID=%s, ", ssid);
    Serial.print("IP=");
    Serial.println(WiFi.localIP());

    server.begin();
}

// Start an AP with specified SSID and password
void start_ap(const char *ssid, const char *password) {
    if (!WiFi.softAP(ssid, password)) {
        Serial.println("[ERROR] soft AP creation failed");
        return;
    }

    Serial.printf("\nStarted: SSID=%s, password=%s\n", ssid, password);
    Serial.print("IP=");
    Serial.println(WiFi.softAPIP());

    server.begin();
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

    // Set I2C pins for AtomS3R
    M5.In_I2C.release();
    M5.In_I2C.begin(I2C_NUM_0, 12, 9);

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

        .xclk_freq_hz = 16000000,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format = PIXFORMAT_RGB565,
        .frame_size = FRAMESIZE_QVGA,

        .jpeg_quality = 64 / 2,
        .fb_count = 2,
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_LATEST,

        .sccb_i2c_port = 0,
    };

    // Initialize the camera
    esp_err_t err = esp_camera_init(&cam_cfg);
    if (err != ESP_OK) {
        Serial.printf("[ERROR] camera init failed: error code=0x%x\n", err);
        return;
    }

    Serial.println("Init camera");

    sensor_t *s = esp_camera_sensor_get();
    // Flip an image for display
    s->set_vflip(s, 1);
    s->set_hmirror(s, 0);

    // WiFI setting: use SSID and password defined by config/secret.ini
#ifdef STA_MODE
    connect_to_ap(AP_SSID, AP_PASS, 60);
#endif

#ifdef AP_MODE
    start_ap(AP_SSID, AP_PASS);
#endif
}

void loop() {
    M5.update();

#ifdef STA_MODE
    if (!WiFi.isConnected()) {
        continue;
    }
#endif
    WiFiClient client = server.accept();

    if (client) {
#ifdef AP_MODE
        Serial.println("Connected");
#endif

        if (client.connected() && client.available()) {
            // capture(&client);
            capture_stream(&client);
        }
        client.stop();

#ifdef AP_MODE
        Serial.println("Disconnected");
#endif
    }

    delay(100);
}
