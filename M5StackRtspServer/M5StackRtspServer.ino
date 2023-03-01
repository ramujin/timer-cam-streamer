//    FRAMESIZE_96X96,    // 96x96
//    FRAMESIZE_QQVGA,    // 160x120
//    FRAMESIZE_QCIF,     // 176x144
//    FRAMESIZE_HQVGA,    // 240x176
//    FRAMESIZE_240X240,  // 240x240
//    FRAMESIZE_QVGA,     // 320x240
//    FRAMESIZE_CIF,      // 400x296
//    FRAMESIZE_HVGA,     // 480x320
//    FRAMESIZE_VGA,      // 640x480
//    FRAMESIZE_SVGA,     // 800x600
//    FRAMESIZE_XGA,      // 1024x768
//    FRAMESIZE_HD,       // 1280x720
//    FRAMESIZE_SXGA,     // 1280x1024
//    FRAMESIZE_UXGA,     // 1600x1200

#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include "CRtspSession.h"
#include "OV2640.h"
#include "OV2640Streamer.h"
//#include "SimStreamer.h"

#include "battery.h"
#include "led.h"
#include "camera_pins.h"

#include <esp_wifi.h>

#define ssid     "TimerCamRTSP"
#define password "PASSWORD"

OV2640 cam;

WiFiServer rtspServer(8554);
CStreamer *streamer;

const int VMAX = 4200; // 4.2V maximum
const int VMIN = 3300; // 3.3V minimum
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
  led_init(CAMERA_LED_GPIO);
//  pinMode(2, OUTPUT);
//  digitalWrite(2, HIGH);
  Serial.begin(115200);
  while (!Serial) {
    continue;
  }

  // camera_config_t timercam_config {
  //   .pin_pwdn     = -1,
  //   .pin_reset    = 15,
  //   .pin_xclk     = 27,
  //   .pin_sscb_sda = 25,
  //   .pin_sscb_scl = 23,
  //   .pin_d7       = 19,
  //   .pin_d6       = 36,
  //   .pin_d5       = 18,
  //   .pin_d4       = 39,
  //   .pin_d3       = 5,
  //   .pin_d2       = 34,
  //   .pin_d1       = 35,
  //   .pin_d0       = 32,
  //   .pin_vsync    = 22,
  //   .pin_href     = 26,
  //   .pin_pclk     = 21,
  //   .xclk_freq_hz = 20000000,
  //   .ledc_timer   = LEDC_TIMER_0,
  //   .ledc_channel = LEDC_CHANNEL_0,
  //   .pixel_format = PIXFORMAT_JPEG,
  // //  .frame_size   = FRAMESIZE_VGA,
  //   .frame_size   = FRAMESIZE_QVGA,
  // //  .frame_size   = FRAMESIZE_HQVGA,
  //   .jpeg_quality = 12,  // 0-63 lower numbers are higher quality
  // //  .jpeg_quality = 10,  // 0-63 lower numbers are higher quality
  //   .fb_count = 2  // if more than one i2s runs in continous mode. Use only with jpeg.
  // };
  // cam.init(timercam_config);
  camera_config_t ttgocam {
    .pin_pwdn     = -1,
    .pin_reset    = -1, 
    .pin_xclk     = 32,
    .pin_sscb_sda = 13,
    .pin_sscb_scl = 12,
    .pin_d7       = 39,
    .pin_d6       = 36,
    .pin_d5       = 23,
    .pin_d4       = 18,
    .pin_d3       = 15,
    .pin_d2       = 4,
    .pin_d1       = 14,
    .pin_d0       = 5,
    .pin_vsync    = 27,
    .pin_href     = 25,
    .pin_pclk     = 19,
    .xclk_freq_hz = 20000000,
    .ledc_timer   = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG,
    .frame_size   = FRAMESIZE_VGA,
    // .frame_size   = FRAMESIZE_QVGA,
  //  .frame_size   = FRAMESIZE_HQVGA,
  //  .jpeg_quality = 12,  // 0-63 lower numbers are higher quality
    .jpeg_quality = 10,  // 0-63 lower numbers are higher quality
    .fb_count = 2  // if more than one i2s runs in continous mode. Use only with jpeg.
  };
  cam.init(ttgocam);

  sensor_t *s = esp_camera_sensor_get();

//    s->set_vflip(s, 1);
   s->set_hmirror(s, 1); // TTGO has horizontal mirroring
  //initial sensors are flipped vertically and colors are a bit saturated
  s->set_vflip(s, 1);//flip it back
//  s->set_brightness(s, 1);//up the brightness just a bit
  s->set_brightness(s, -1);//up the brightness just a bit
//  s->set_saturation(s, -2);//lower the saturation
  s->set_saturation(s, 2);//lower the saturation

//    IPAddress ip;
//    WiFi.mode(WIFI_STA);
//    WiFi.begin(ssid, password);
//    while (WiFi.status() != WL_CONNECTED) {
//        delay(500);
//        Serial.print(F("."));
//    }
//    ip = WiFi.localIP();
  WiFi.softAP(ssid, password);
  IPAddress ip = WiFi.softAPIP();

  Serial.println(F("WiFi connected"));
  Serial.print("RTSP URL: rtsp://");
  Serial.print(ip);
  Serial.println(":8554/mjpeg/1");
  rtspServer.begin();

//  streamer = new SimStreamer(true); // Test JPEG image stream
  streamer = new OV2640Streamer(cam); // our streamer for UDP/TCP based RTP transport
}

void loop() {
  // If we have an active client connection, just service that until gone
  streamer->handleRequests(0);  // we don't use a timeout here,
  // instead we send only if we have new enough frames
  if (streamer->anySessions()) {
    uint32_t now = millis();
    streamer->streamImage(now);
  }

  WiFiClient rtspClient = rtspServer.accept();
  if (rtspClient) {
    Serial.print("client: ");
    Serial.print(rtspClient.remoteIP());
    Serial.println();
    streamer->addSession(rtspClient);
  }

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
           
//            // Begin pulsing LED for 1 minute
//            int count;
//            for(count=1; count<=60; count++) {
//              led_brightness( (count%2) * 128 );
//              delay(1000);
//            }
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
