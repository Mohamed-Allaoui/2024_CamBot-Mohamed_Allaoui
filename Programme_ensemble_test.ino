#include "esp_camera.h"
#include <WiFi.h>
#include <M5Stack.h>
#include <Wire.h>
#include <TFLI2C.h>
#include "FaBoPWM_PCA9685.h"

#define CAMERA_MODEL_M5STACK_UNITCAM
#include "camera_pins.h"

#define I2C_SDA 4
#define I2C_SCL 13

const char* ssid = "#LaboD5";
const char* password = "0123456789";
int16_t tfDist;
int16_t tfAddr = TFL_DEF_ADR;

FaBoPWM faboPWM;
TFLI2C tflI2C;
esp_timer_handle_t timer;

void setupLedFlash(int pin);
void startCameraServer();
void IRAM_ATTR onTimer(void* param) {
static int cpt = 0;
}

void setup() {
  M5.begin();
  Wire.begin();
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  faboPWM.begin();
  faboPWM.init(300);
  faboPWM.set_hz(50);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t* s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);
    s->set_brightness(s, 1);
    s->set_saturation(s, -2);
  }
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

#if defined(LED_GPIO_NUM)
  setupLedFlash(LED_GPIO_NUM);
#endif

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    switch (command) 
    {
      case 'S':
        faboPWM.set_channel_value(0, 87);  // min 87 ; max 559 ; stable à 330
        delay(1000);
        faboPWM.set_channel_value(0, 330);
        delay(1000);
        break;
      case 'L':
        if (tflI2C.getData(tfDist, tfAddr))  // If read okay...
        {
          Serial.print("Dist: ");
          Serial.println(tfDist);     // print the data...
        } else tflI2C.printStatus();  // else, print error.
        delay(50);
    }
    Serial.println("Commande non reconnue");
  }
}