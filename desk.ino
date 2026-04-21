#include <Wire.h>
#include <U8g2lib.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include "time.h"

// --- CREDENTIALS ---
const char* ssid     = "D_Office 2.4GHz";
const char* password = "ducat@2024";
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 19800; // India
const int   daylightOffset_sec = 0;

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
Servo legTilt, legSwing, leftHand, rightHand;

const int pinLegTilt = 18, pinLegSwing = 19, pinLeftHand = 27, pinRightHand = 14;
const int petTouchPin = 4;
const int clockTouchPin = 15;

// DEBUG TIP: Set threshold to 30-50 for standard wires. 500 is very high for ESP32.
int threshold = 500; 

void setup() {
  Serial.begin(115200);
  u8g2.begin();
  
  // 1. OLED WIFI STATUS
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(0, 15, "WIFI: Connecting...");
  u8g2.drawStr(0, 30, ssid);
  u8g2.sendBuffer();

  WiFi.begin(ssid, password);
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    u8g2.drawStr(attempt * 6, 45, "."); // Visual progress dots
    u8g2.sendBuffer();
    attempt++;
    if(attempt > 30) break; // Timeout after 15 seconds
  }

  if(WiFi.status() == WL_CONNECTED) {
    u8g2.clearBuffer();
    u8g2.drawStr(0, 15, "WIFI: CONNECTED!");
    u8g2.drawStr(0, 30, "IP:");
    u8g2.drawStr(25, 30, WiFi.localIP().toString().c_str());
    u8g2.sendBuffer();
    Serial.println("\nWiFi Connected!");
    delay(2000);
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  legTilt.attach(pinLegTilt); legSwing.attach(pinLegSwing);
  leftHand.attach(pinLeftHand); rightHand.attach(pinRightHand);
  
  centerAll();
}

void centerAll() {
  legTilt.write(90); legSwing.write(90);
  leftHand.write(90); rightHand.write(90);
}

void showDateTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Time Sync Failed");
    return;
  }

  Serial.println("Displaying Time...");
  unsigned long start = millis();
  while(millis() - start < 5000) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_logisoso24_tn); 
    char timeStr[10];
    strftime(timeStr, 10, "%H:%M", &timeinfo);
    u8g2.drawStr(30, 45, timeStr);
    u8g2.sendBuffer();
    getLocalTime(&timeinfo);
  }

  Serial.println("Displaying Date...");
  start = millis();
  while(millis() - start < 5000) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_helvB14_tr);
    char dayStr[20]; strftime(dayStr, 20, "%A", &timeinfo);
    u8g2.drawStr(25, 30, dayStr);
    char dateStr[20]; strftime(dateStr, 20, "%d %b", &timeinfo);
    u8g2.drawStr(35, 55, dateStr);
    u8g2.sendBuffer();
  }
}

void loop() {
  int petVal = touchRead(petTouchPin);
  int clockVal = touchRead(clockTouchPin);

  // LOG VALUES TO SERIAL FOR CALIBRATION
  Serial.printf("P:%d | C:%d\n", petVal, clockVal);

  if (clockVal < threshold && clockVal > 0) {
    Serial.println(">>> Clock Triggered");
    leftHand.write(140); rightHand.write(140);
    showDateTime();
    centerAll();
  } 
  else if (petVal < threshold && petVal > 0) {
    Serial.println(">>> Petting Triggered");
    u8g2.clearBuffer();
    u8g2.drawRBox(25, 25, 25, 15, 10); u8g2.drawRBox(78, 25, 25, 15, 10);
    u8g2.sendBuffer();
    leftHand.write(120); rightHand.write(60); delay(200);
    leftHand.write(60); rightHand.write(120); delay(200);
  } 
  else {
    // NORMAL MODE + DEBUG OVERLAY
    u8g2.clearBuffer();
    u8g2.drawRBox(25, 20, 25, 25, 5); u8g2.drawRBox(78, 20, 25, 25, 5);
    
    // TINY DEBUG INFO IN CORNER
    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.setCursor(0, 64);
    u8g2.print("P:"); u8g2.print(petVal); 
    u8g2.print(" C:"); u8g2.print(clockVal);
    
    u8g2.sendBuffer();
    centerAll();
    delay(200); // Reduced delay for faster touch response
  }
}
