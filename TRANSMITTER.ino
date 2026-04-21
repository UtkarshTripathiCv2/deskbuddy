#include <Wire.h>
#include <U8g2lib.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include "time.h"
#include <esp_now.h>
#include <esp_wifi.h>

// -------- WIFI --------
const char* ssid     = "D_Office 2.4GHz";
const char* password = "ducat@2024";
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 19800;
const int   daylightOffset_sec = 0;

// -------- OLED --------
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// -------- SERVOS --------
Servo legTilt, legSwing, leftHand, rightHand;
const int pinLegTilt = 18, pinLegSwing = 19, pinLeftHand = 27, pinRightHand = 14;

// -------- TOUCH --------
const int petTouchPin = 4;
const int clockTouchPin = 15;
const int espTouchPin = 13;

int threshold = 40;

// -------- ESP-NOW --------
uint8_t receiverMAC[] = {0x24,0x6F,0x28,0xAA,0xBB,0xCC}; // PUT YOUR MAC

typedef struct {
  bool ledState;
} struct_message;

struct_message msg;
bool ledState = false;
bool lastTouchState = false;

// ✅ NEW CALLBACK (FIXED)
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "ESP SENT" : "ESP FAIL");
}

// -------- SETUP --------
void setup() {
  Serial.begin(115200);
  u8g2.begin();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(300);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // ESP NOW
  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  esp_now_add_peer(&peerInfo);

  // SERVOS
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  legTilt.attach(pinLegTilt);
  legSwing.attach(pinLegSwing);
  leftHand.attach(pinLeftHand);
  rightHand.attach(pinRightHand);

  centerAll();
}

// -------- FUNCTIONS --------
void centerAll() {
  legTilt.write(90);
  legSwing.write(90);
  leftHand.write(90);
  rightHand.write(90);
}

// ✅ FULL TIME + DAY + DATE
void showDateTime() {
  struct tm timeinfo;

  // TIME
  unsigned long t = millis();
  while (millis() - t < 3000) {
    if (!getLocalTime(&timeinfo)) return;

    char timeStr[10];
    strftime(timeStr, 10, "%H:%M", &timeinfo);

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_logisoso24_tn);
    u8g2.drawStr(30, 45, timeStr);
    u8g2.sendBuffer();
  }

  // DAY
  t = millis();
  while (millis() - t < 2000) {
    if (!getLocalTime(&timeinfo)) return;

    char dayStr[20];
    strftime(dayStr, 20, "%A", &timeinfo);

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_helvB14_tr);
    u8g2.drawStr(20, 40, dayStr);
    u8g2.sendBuffer();
  }

  // DATE
  t = millis();
  while (millis() - t < 2000) {
    if (!getLocalTime(&timeinfo)) return;

    char dateStr[20];
    strftime(dateStr, 20, "%d %b %Y", &timeinfo);

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_helvB14_tr);
    u8g2.drawStr(10, 40, dateStr);
    u8g2.sendBuffer();
  }
}

// -------- LOOP --------
void loop() {
  int petVal = touchRead(petTouchPin);
  int clockVal = touchRead(clockTouchPin);
  int espVal = touchRead(espTouchPin);

  Serial.printf("P:%d C:%d E:%d\n", petVal, clockVal, espVal);

  // 🔥 ESP-NOW (EDGE DETECTION FIX)
  bool currentTouch = (espVal < threshold && espVal > 0);

  if (currentTouch && !lastTouchState) {
    ledState = !ledState;
    msg.ledState = ledState;
    esp_now_send(receiverMAC, (uint8_t*)&msg, sizeof(msg));
    Serial.println("ESP TOGGLE SENT");
  }
  lastTouchState = currentTouch;

  // CLOCK
  if (clockVal < threshold && clockVal > 0) {
    leftHand.write(140);
    rightHand.write(140);
    showDateTime();
    centerAll();
  }

  // PETTING (FACE ANIMATION RESTORED)
  else if (petVal < threshold && petVal > 0) {
    u8g2.clearBuffer();
    u8g2.drawRBox(25, 25, 25, 15, 10);
    u8g2.drawRBox(78, 25, 25, 15, 10);
    u8g2.sendBuffer();

    leftHand.write(120); rightHand.write(60); delay(200);
    leftHand.write(60); rightHand.write(120); delay(200);
  }

  // NORMAL FACE
  else {
    u8g2.clearBuffer();
    u8g2.drawRBox(25, 20, 25, 25, 5);
    u8g2.drawRBox(78, 20, 25, 25, 5);
    u8g2.sendBuffer();

    centerAll();
    delay(120);
  }
}
