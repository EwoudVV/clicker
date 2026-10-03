#pragma once
#include <stdint.h>
#include <string.h>

namespace clicker {
constexpr uint32_t kMagic = 0x434c4b52;
constexpr uint32_t kHeartbeatMs = 50;
constexpr uint32_t kTimeoutMs = 300;
constexpr uint32_t kDebounceMs = 15;
constexpr uint8_t kChannel = 1;

struct __attribute__((packed)) Packet {
  uint32_t magic;
  uint32_t session;
  uint32_t sequence;
  uint8_t version;
  uint8_t buttons;
  uint16_t reserved;
};
static_assert(sizeof(Packet) == 16, "Both boards must use the same wire format");

inline bool validPacket(const Packet &p) {
  return p.magic == kMagic && p.version == 1 && p.buttons <= 3 && p.reserved == 0;
}

struct Debouncer {
  bool candidate = false;
  bool pressed = false;
  uint32_t changedAt = 0;
  bool update(bool input, uint32_t now) {
    if (input != candidate) { candidate = input; changedAt = now; }
    if (candidate != pressed && uint32_t(now - changedAt) >= kDebounceMs) {
      pressed = candidate;
      return true;
    }
    return false;
  }
};

struct ReceiverState {
  uint32_t session = 0, sequence = 0, lastSeen = 0;
  uint8_t buttons = 0;
  bool connected = false;
  bool seenSession = false;
  bool accept(const Packet &p, uint32_t now) {
    if (!validPacket(p)) return false;
    if (seenSession && p.session == session && int32_t(p.sequence - sequence) <= 0)
      return false;
    session = p.session;
    sequence = p.sequence;
    seenSession = connected = true;
    lastSeen = now;
    buttons = p.buttons;
    return true;
  }
  bool expire(uint32_t now) {
    if (connected && uint32_t(now - lastSeen) >= kTimeoutMs) {
      connected = false;
      buttons = 0;
      return true;
    }
    return false;
  }
  void reset() { *this = ReceiverState{}; }
};
}
