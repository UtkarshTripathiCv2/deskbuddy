#include <esp_now.h>
#include <WiFi.h>

const int ledPin = 2; // change if needed

typedef struct {
  bool ledState;
} struct_message;

struct_message data;

// RECEIVE CALLBACK (UPDATED)
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  memcpy(&data, incomingData, sizeof(data));

  digitalWrite(ledPin, data.ledState ? HIGH : LOW);

  Serial.print("LED State: ");
  Serial.println(data.ledState);
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();  // 🔥 important

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
}
