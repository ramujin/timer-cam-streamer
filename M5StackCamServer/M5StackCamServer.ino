#include "esp_camera.h"
#include <WiFi.h>
#include "led.h"
#include "battery.h"
//#include "bmm8563.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "camera_pins.h"

#include <esp_wifi.h>

const int VMAX = 4200; // 4.2V maximum
const int VMIN = 3300; // 3.3V minimum

const char* ssid = "SSID_USERNAME";
const char* password = "SSID_PASSWORD";

const char* ap_ssid = "TimerCamUDP";
const char* ap_password = "PASSWORD";

uint8_t wifi_mode = 0 ;   //1:station mode, 0:access point mode

void startCameraServer(); // defined in app_httpd.cpp
uint32_t previous_voltage = 0;
uint32_t current_voltage = 0;
int charging = 0;
int timeStart = 0;
int timeEnd = 0;
const int SAMPLE_TIME = 10000; // 1 second sampling

void setup() {
  // https://www.mischianti.org/2021/03/06/esp32-practical-power-saving-manage-wifi-and-cpu-1/
  WiFi.setSleep(false);
  esp_wifi_set_ps(WIFI_PS_NONE); // turn off WiFi power saving

  bat_init();
  bat_hold_output();
//  bmm8563_init();
  led_init(CAMERA_LED_GPIO);

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;

  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.xclk_freq_hz = 20000000;

  config.pixel_format = PIXFORMAT_JPEG;
//  config.frame_size = FRAMESIZE_UXGA;
//  config.jpeg_quality = 10;
//  config.frame_size   = FRAMESIZE_HQVGA;
  config.frame_size   = FRAMESIZE_QVGA;
  config.jpeg_quality = 10;
  config.fb_count     = 2;

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    bat_disable_output();
    return;
  }

  sensor_t *s = esp_camera_sensor_get();

  //initial sensors are flipped vertically and colors are a bit saturated
  s->set_vflip(s, 1);//flip it back
  s->set_brightness(s, 1);//up the brightness just a bit
  s->set_saturation(s, -2);//lower the saturation

  //drop down frame size for higher initial frame rate
//  s->set_framesize(s, FRAMESIZE_QVGA);
//  s->set_framesize(s, FRAMESIZE_VGA);

  if(wifi_mode) {
    Serial.printf("Connecting to SSID: %s\r\n", ssid);

    WiFi.begin(ssid, password);

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
  else {
    Serial.println("Configuring access point...");

    // You can remove the password parameter if you want the AP to be open.
    WiFi.softAP(ap_ssid, ap_password);
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    startCameraServer();

    Serial.println("Server started");
  }
}

void loop() {
  timeEnd = millis();
  if (timeEnd - timeStart >= SAMPLE_TIME) {
    timeStart = timeEnd;

    // Get current battery reading
    current_voltage = bat_get_voltage();
    Serial.print("Current voltage: ");
    Serial.println(current_voltage);

    // A significant voltage change of 20 mV can trigger a previous state update
    int delta_voltage = current_voltage - previous_voltage;
    if (abs(delta_voltage) > 20) {
      bool pluggedIn = (current_voltage > previous_voltage) ? true : false;
      previous_voltage = current_voltage;

      // If not charging
      if(charging == 0) {
        if(pluggedIn) { // If we got plugged in, now we are charging
          Serial.println("Battery is being charged.");
          led_brightness(128);
          charging = 1;
        }
        else if(current_voltage < VMIN) { // Shut down if voltage drops below VMIN
          Serial.println("Battery voltage critically low. Shutting off soon.");

          // Begin pulsing LED for 1 minute
          int count;
          for(count=1; count<=60; count++) {
            led_brightness( (count%2) * 128 );
            delay(1000);
          }
          bat_disable_output();
          esp_deep_sleep_start();
        }
      }

      // If charging
      if(charging == 1) {
        if(current_voltage > VMAX) { // Turn off LED when fully charged
          Serial.println("Battery is fully charged.");
          led_brightness(0);
          charging = 2;
        }
        else if(!pluggedIn) { // It was unplugged
          charging = 0;
        }
      }

      // If fully charged
      if(charging == 2) {
        if(current_voltage < VMAX) { // Go to not charging if the voltage falls below VMAX
          charging = 0;
        }
      }
    }
  }
}
