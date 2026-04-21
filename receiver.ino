#include <esp_now.h>
#include <WiFi.h>

const int ledPin = 2;

typedef struct {
  bool ledState;
} struct_message;

struct_message data;

// UPDATED CALLBACK
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  memcpy(&data, incomingData, sizeof(data));

  digitalWrite(ledPin, data.ledState ? HIGH : LOW);

  Serial.print("LED: ");
  Serial.println(data.ledState);
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP FAIL");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {}
