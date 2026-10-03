#include <Clicker.h>
#include <esp_random.h>

#if !CONFIG_IDF_TARGET_ESP32C3
#error "Build the remote for ESP32-C3."
#endif

constexpr uint8_t kButtons[2] = {5, 6};
constexpr uint8_t kLed = 7;
clicker::Device device;
clicker::Debouncer buttons[2];
uint32_t session, sequence = 0, lastSent = 0, ledUntil = 0;
uint8_t lastMask = 0xff;

void setup() {
  Serial.begin(115200);
  for (auto pin : kButtons) pinMode(pin, INPUT_PULLUP);
  pinMode(kLed, OUTPUT);
  digitalWrite(kLed, LOW);
  device.begin("clicker-remote");
  session = esp_random();
  device.status();
}

void loop() {
  if (device.pollConsole(false)) lastMask = 0xff;
  uint32_t now = millis();
  uint8_t mask = 0;
  for (int i = 0; i < 2; ++i) {
    buttons[i].update(digitalRead(kButtons[i]) == LOW, now);
    if (buttons[i].pressed) mask |= 1 << i;
  }
  if (device.canSend() && (mask != lastMask || uint32_t(now - lastSent) >= clicker::kHeartbeatMs)) {
    clicker::Packet packet{clicker::kMagic, session, ++sequence, 1, mask, 0};
    if (esp_now_send(device.settings.peer, reinterpret_cast<uint8_t *>(&packet), sizeof(packet)) == ESP_OK) {
      lastMask = mask;
      ledUntil = now + 8;
    }
    lastSent = now;
  }
  // This indicates a queued transmission, not confirmation that the receiver got it.
  digitalWrite(kLed, int32_t(ledUntil - now) > 0 ? HIGH : LOW);
  delay(1);
}
