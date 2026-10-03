#include <Clicker.h>
#include <USB.h>
#include <USBHIDKeyboard.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#if !CONFIG_IDF_TARGET_ESP32S3 || ARDUINO_USB_MODE != 0
#error "Build for ESP32-S3 with USB-OTG (TinyUSB), not hardware JTAG/CDC."
#endif

constexpr uint8_t kLed = 4;
USBHIDKeyboard keyboard;
USBHID hid;
clicker::Device device;
clicker::ReceiverState state;
struct Incoming { uint8_t source[6]; clicker::Packet packet; };
QueueHandle_t incoming;
hid_keyboard_report_t appliedReport{};
bool reportApplied = false;
bool wasUsbReady = false;
uint32_t ledUntil = 0;

void receive(const esp_now_recv_info_t *info, const uint8_t *data, int length) {
  if (!info || length != sizeof(clicker::Packet)) return;
  Incoming item;
  memcpy(item.source, info->src_addr, 6);
  memcpy(&item.packet, data, sizeof(item.packet));
  // ESP-NOW runs this on the Wi-Fi task. USB and settings stay in loop().
  xQueueSend(incoming, &item, 0);
}

bool applyButtons(uint8_t mask) {
  hid_keyboard_report_t report{};
  unsigned count = 0;
  for (int i = 0; i < 2; ++i) {
    uint8_t usage = device.settings.usage[i];
    if (!(mask & (1 << i)) || !usage) continue;
    report.modifier |= device.settings.modifiers[i];
    if (count == 0 || report.keycode[0] != usage) report.keycode[count++] = usage;
  }
  if (reportApplied && memcmp(&report, &appliedReport, sizeof(report)) == 0) return true;
  if (!hid.SendReport(HID_REPORT_ID_KEYBOARD, &report, sizeof(report), 10)) return false;
  appliedReport = report;
  reportApplied = true;
  return true;
}

void setup() {
  Serial.begin(115200);
  pinMode(kLed, OUTPUT);
  digitalWrite(kLed, LOW);
  incoming = xQueueCreate(16, sizeof(Incoming));
  if (!incoming) { Serial.println("Receive queue failed."); return; }
  keyboard.begin();
  USB.productName("Clicker");
  USB.begin();
  device.begin("clicker-recv");
  if (device.radioReady && esp_now_register_recv_cb(receive) != ESP_OK)
    Serial.println("Receive callback failed.");
  device.status();
}

void loop() {
  if (!incoming) { delay(100); return; }
  if (device.pollConsole(true)) {
    state.reset();
    reportApplied = false;
    xQueueReset(incoming);
  }
  Incoming item;
  uint32_t now = millis();
  while (xQueueReceive(incoming, &item, 0) == pdTRUE) {
    if (device.isPeer(item.source) && state.accept(item.packet, now)) ledUntil = now + 8;
  }
  state.expire(now);
  // Reapply after USB reconnect; release keys after a radio timeout.
  bool ready = hid.ready();
  if (!ready || !wasUsbReady) reportApplied = false;
  if (ready) applyButtons(state.buttons);
  wasUsbReady = ready;
  digitalWrite(kLed, int32_t(ledUntil - now) > 0 ? HIGH : LOW);
  delay(1);
}
