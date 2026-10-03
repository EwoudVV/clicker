#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "ClickerState.h"

namespace clicker {
struct Settings {
  uint32_t version = 1;
  uint8_t peer[6] = {};
  uint8_t key[16] = {};
  uint8_t usage[2] = {0x50, 0x4f}; // Left and right arrows.
  uint8_t modifiers[2] = {};
  bool paired = false;
};

class Device {
 public:
  Settings settings;
  bool radioReady = false;

  void begin(const char *role) {
    preferences.begin(role, false);
    if (preferences.getBytesLength("settings") == sizeof(Settings)) {
      Settings loaded;
      preferences.getBytes("settings", &loaded, sizeof(loaded));
      if (loaded.version == 1) settings = loaded;
    }
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    if (esp_wifi_set_channel(kChannel, WIFI_SECOND_CHAN_NONE) != ESP_OK ||
        esp_now_init() != ESP_OK) {
      Serial.println("Radio init failed. Reset the board to retry.");
      return;
    }
    radioReady = true;
    configurePeer();
  }

  bool configurePeer() {
    if (!radioReady) return false;
    if (hasRadioPeer) esp_now_del_peer(radioPeer);
    hasRadioPeer = false;
    if (!settings.paired) return true;
    if (esp_now_set_pmk(settings.key) != ESP_OK) return false;
    esp_now_peer_info_t peer{};
    memcpy(peer.peer_addr, settings.peer, 6);
    memcpy(peer.lmk, settings.key, 16);
    peer.channel = kChannel;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = true;
    if (esp_now_add_peer(&peer) != ESP_OK) {
      Serial.println("Couldn't add the paired device.");
      return false;
    }
    memcpy(radioPeer, settings.peer, 6);
    hasRadioPeer = true;
    return true;
  }

  bool canSend() const { return hasRadioPeer; }
  bool isPeer(const uint8_t *mac) const {
    return hasRadioPeer && memcmp(mac, settings.peer, 6) == 0;
  }

  void status() const {
    Serial.printf("MAC %s\n", WiFi.macAddress().c_str());
    Serial.printf("channel %u, radio %s, paired %s\n", kChannel,
                  radioReady ? "ready" : "failed", hasRadioPeer ? "yes" : "no");
    if (settings.paired) {
      Serial.printf("peer %02X:%02X:%02X:%02X:%02X:%02X\n",
                    settings.peer[0], settings.peer[1], settings.peer[2],
                    settings.peer[3], settings.peer[4], settings.peer[5]);
    }
    for (int i = 0; i < 2; ++i)
      Serial.printf("button %d: HID 0x%02X, modifiers 0x%02X\n",
                    i + 1, settings.usage[i], settings.modifiers[i]);
  }

  // Returns true when pairing or mapping changed. Call from loop(), never Wi-Fi callbacks.
  bool pollConsole(bool allowMapping) {
    bool changed = false;
    while (Serial.available()) {
      char c = Serial.read();
      if (c == '\r') continue;
      if (c == '\n') {
        if (!overflow) {
          line[length] = 0;
          changed |= command(allowMapping);
        } else Serial.println("Command too long.");
        length = 0;
        overflow = false;
      } else if (length < sizeof(line) - 1 && !overflow) line[length++] = c;
      else overflow = true;
    }
    return changed;
  }

 private:
  Preferences preferences;
  uint8_t radioPeer[6] = {};
  bool hasRadioPeer = false;
  char line[96] = {};
  size_t length = 0;
  bool overflow = false;

  bool save() {
    if (preferences.putBytes("settings", &settings, sizeof(settings)) != sizeof(settings)) {
      Serial.println("Saving settings failed.");
      return false;
    }
    Serial.println("Saved.");
    return true;
  }
  static bool hex(const char *s, uint8_t *out, size_t count, bool colon) {
    if (strlen(s) != (colon ? count * 3 - 1 : count * 2)) return false;
    for (size_t i = 0; i < count; ++i) {
      size_t j = i * (colon ? 3 : 2);
      if (!isxdigit(s[j]) || !isxdigit(s[j + 1])) return false;
      if (colon && i < count - 1 && s[j + 2] != ':') return false;
      char byte[] = {s[j], s[j + 1], 0};
      out[i] = strtoul(byte, nullptr, 16);
    }
    return true;
  }
  static int usageFor(const char *key) {
    const char *names[] = {"left", "right", "up", "down", "pageup", "pagedown",
                           "space", "enter", "escape", "home", "end", "disabled"};
    const uint8_t codes[] = {0x50,0x4f,0x52,0x51,0x4b,0x4e,0x2c,0x28,0x29,0x4a,0x4d,0};
    for (unsigned i = 0; i < sizeof(codes); ++i)
      if (strcmp(key, names[i]) == 0) return codes[i];
    if (strlen(key) == 1 && key[0] >= 'a' && key[0] <= 'z') return key[0] - 'a' + 4;
    char *end = nullptr;
    unsigned long n = strtoul(key, &end, 0);
    return end != key && *end == 0 && n >= 4 && n <= 0x65 ? int(n) : -1;
  }
  bool command(bool allowMapping) {
    char op[12], a[40], b[40], c[12], extra[2];
    int count = sscanf(line, "%11s %39s %39s %11s %1s", op, a, b, c, extra);
    if (count < 1) return false;
    if (strcmp(op, "status") == 0 && count == 1) { status(); return false; }
    if (strcmp(op, "pair") == 0 && count == 3) {
      Settings next = settings;
      if (!hex(a, next.peer, 6, true) || !hex(b, next.key, 16, false) ||
          (next.peer[0] & 1) || memcmp(next.peer, "\0\0\0\0\0\0", 6) == 0) {
        Serial.println("Use pair <unicast MAC> <32 hex digits>.");
        return false;
      }
      uint8_t own[6];
      esp_wifi_get_mac(WIFI_IF_STA, own);
      if (memcmp(next.peer, own, 6) == 0) { Serial.println("That is this board's MAC."); return false; }
      next.paired = true;
      settings = next;
      save();
      configurePeer();
      return true;
    }
    if (strcmp(op, "unpair") == 0 && count == 1) {
      settings.paired = false;
      memset(settings.peer, 0, 6);
      memset(settings.key, 0, 16);
      save(); configurePeer(); return true;
    }
    if (strcmp(op, "map") == 0 && allowMapping && (count == 3 || count == 4)) {
      int button = strcmp(a,"1") == 0 ? 0 : strcmp(a,"2") == 0 ? 1 : -1;
      int usage = usageFor(b);
      char *end = nullptr;
      unsigned long mods = count == 4 ? strtoul(c, &end, 0) : 0;
      if (button < 0 || usage < 0 || mods > 255 ||
          (count == 4 && (end == c || *end != 0))) {
        Serial.println("Use map <1|2> <key|HID usage> [modifier byte].");
        return false;
      }
      settings.usage[button] = usage;
      settings.modifiers[button] = mods;
      save(); return true;
    }
    Serial.println("Commands: status, pair <MAC> <32 hex key>, unpair");
    if (allowMapping) Serial.println("map <1|2> <left|right|pageup|pagedown|a..z|HID usage|disabled> [modifiers]");
    return false;
  }
};
}
