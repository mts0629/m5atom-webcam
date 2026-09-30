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

    WiFi.mode(WIFI_STA);
    WiFi.disconnect(false, true);
}

bool connect_to_ap(const char *ssid, const char *password) {
    WiFi.begin(ssid, password);

    int i = 0;
    const int max_try = 60;
    Serial.print("Connecting");
    auto status = WiFi.status();
    while ((status != WL_CONNECTED) || (i > max_try)) {
        Serial.print(".");
        i++;
        delay(1000);
        status = WiFi.status();
    }

    if (status != WL_CONNECTED) {
        Serial.printf("\nConnection failed: SSID=%s\n", ssid);
        return false;
    }

    Serial.printf("\nConnected: SSID=%s, ", ssid);
    Serial.print("IP=");
    Serial.println(WiFi.localIP());

    return true;
}

void loop() {
    M5.update();

    WiFiClient client = server.accept();

    if (client) {
        if (client.connected() && client.available()) {
            capture(&client);
        }
        client.stop();
    } else {
        // Input SSID and password from serial console
        Serial.print("SSID: ");
        auto ssid_str = Serial.readStringUntil('\n');
        ssid_str.trim();
        Serial.print("Password: ");
        auto pass_str = Serial.readStringUntil('\n');
        pass_str.trim();

        // Establish WiFi connection
        if (connect_to_ap(ssid_str.c_str(), pass_str.c_str())) {
            server.begin();
        }
    }
}
