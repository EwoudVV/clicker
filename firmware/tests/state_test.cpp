#include "ClickerState.h"
#include <assert.h>
#include <stdio.h>
using namespace clicker;

Packet packet(uint32_t seq, uint8_t buttons, uint32_t session = 123) {
  return {kMagic, session, seq, 1, buttons, 0};
}

int main() {
  Debouncer button;
  assert(!button.update(true, 100));
  assert(!button.update(false, 105));
  assert(!button.update(true, 110));
  assert(!button.update(true, 124));
  assert(button.update(true, 125) && button.pressed);
  assert(!button.update(false, 150));
  assert(button.update(false, 165) && !button.pressed);

  ReceiverState state;
  assert(state.accept(packet(1, 1), 100));
  assert(state.buttons == 1);
  assert(!state.accept(packet(1, 0), 200)); // Duplicate cannot change the keys.
  assert(!state.accept(packet(0, 0), 200)); // Nor can an older frame.
  assert(state.lastSeen == 100);
  assert(!state.expire(399));
  assert(state.expire(400) && state.buttons == 0 && !state.connected);
  assert(!state.accept(packet(1, 1), 410)); // No stale press after timeout.
  assert(state.accept(packet(3, 0), 420)); // Lost release recovered by a heartbeat.
  assert(state.accept(packet(1, 3, 456), 430)); // Remote restart, both buttons held.

  Packet invalid = packet(2, 0, 456);
  invalid.version = 2;
  assert(!state.accept(invalid, 440));
  invalid = packet(2, 4, 456);
  assert(!state.accept(invalid, 440));
  invalid = packet(2, 0, 456); invalid.magic = 0;
  assert(!state.accept(invalid, 440));
  invalid = packet(2, 0, 456); invalid.reserved = 1;
  assert(!state.accept(invalid, 440));
  assert(state.buttons == 3 && state.lastSeen == 430);

  state.reset();
  assert(state.accept(packet(0xffffffff, 1), 0xfffffff0));
  assert(state.accept(packet(0, 0), 0xfffffff8)); // Sequence wraps.
  assert(!state.accept(packet(0xffffffff, 1), 0xfffffff9));
  assert(!state.expire(0x123));
  assert(state.expire(0x124)); // millis() wraps too.

  button = Debouncer{};
  assert(!button.update(true, 0xfffffff8));
  assert(!button.update(true, 6));
  assert(button.update(true, 7));
  puts("PASS: debounce, duplicates, reordering, malformed packets, heartbeat recovery, radio timeout, reboot, timer and sequence wrap");
}
