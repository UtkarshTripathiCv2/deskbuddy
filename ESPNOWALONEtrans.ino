#include <esp_now.h>
#include <WiFi.h>

// TOUCH PIN
const int touchPin = 13;
int threshold = 40;

// RECEIVER MAC (PUT YOURS)
uint8_t receiverMAC[] = {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC};

typedef struct {
  bool ledState;
} struct_message;

struct_message msg;

bool ledState = false;
bool lastTouch = false;

// NEW CALLBACK (ESP32 CORE 3.x)
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sent" : "Fail");
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();  // 🔥 important

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Peer Add Failed");
  }
}

void loop() {
  int val = touchRead(touchPin);

  bool touch = (val < threshold && val > 0);

  // EDGE DETECTION (VERY IMPORTANT)
  if (touch && !lastTouch) {
    ledState = !ledState;

    msg.ledState = ledState;
    esp_now_send(receiverMAC, (uint8_t *)&msg, sizeof(msg));

    Serial.print("Sent LED: ");
    Serial.println(ledState);
  }

  lastTouch = touch;

  delay(50);
}
