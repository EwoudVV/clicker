# Clicker firmware

The remote sends its two button states over ESP-NOW. The receiver turns them into USB keyboard reports. Button 1 is left arrow and button 2 is right arrow by default, for slides.

Both sketches compile with Espressif's Arduino core 3.3.12. The boards have not been built or tested yet.

## boards and pins

| | remote | receiver |
|---|---|---|
| module | ESP32-C3-WROOM-02-N4 | ESP32-S3-WROOM-1-N4 |
| button 1 | GPIO5, active low | — |
| button 2 | GPIO6, active low | — |
| LED | GPIO7, active high | GPIO4, active high |
| USB D-/D+ | GPIO18 / GPIO19 | GPIO19 / GPIO20 |
| debug RX / TX | GPIO20 / GPIO21 | GPIO44 / GPIO43 |

These are GPIO numbers, not the module's physical pad numbers. They come from the schematics. The battery divider is on remote GPIO4; this version does not use it for battery protection or sleep.

## build

Install Arduino CLI and the official ESP32 core:

```sh
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32@3.3.12 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

From the repo root:

```sh
./firmware/test.sh
./firmware/build.sh
```

`test.sh` runs the portable button/packet state tests on the computer, with address and undefined-behavior sanitizers. It does not emulate Wi-Fi, USB, flash storage, or the electronics. `build.sh` compiles both sketches and saves compiler results under `verification/2026-10-03/`. Builds go in the ignored `work/` folder.

In Arduino IDE, add `firmware/libraries` as a sketchbook library location or install its Clicker folder as a library. Select **ESP32C3 Dev Module** for the remote and **ESP32S3 Dev Module** for the receiver, with 4 MB flash, no PSRAM, and USB CDC on boot enabled. The receiver needs **USB-OTG (TinyUSB)** USB mode. Hardware CDC/JTAG mode cannot provide the keyboard interface.

For the first flash, hold BOOT, tap RESET, then release BOOT and select the board's serial port. Both boards have USB wired to the native pins. If that does not enumerate, use the debug header with a 3.3 V logic USB-UART adapter. The header is GND, RX, TX; it has no power pin. UART lines cross: adapter TX to board RX and adapter RX to board TX. Power the board through USB.

From the `firmware` folder, replacing `<port>` with the real port:

```sh
arduino-cli upload --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc,FlashSize=4M --input-dir ../work/build-remote --port '<port>' remote
arduino-cli upload --fqbn esp32:esp32:esp32s3:USBMode=default,CDCOnBoot=cdc,FlashSize=4M,PSRAM=disabled --input-dir ../work/build-receiver --port '<port>' receiver
```

These upload steps are instructions for later; no board has been flashed.

## pairing

Each board has a USB serial configuration console at 115200 baud. Send commands with a newline. Nothing transmits keyboard data until the pair is configured.

1. Run `status` on each board and note its MAC address.
2. Generate one random 16-byte link key on your computer:

   ```sh
   python3 -c 'import secrets; print(secrets.token_hex(16))'
   ```

3. On the remote, send `pair <receiver-MAC> <key>`.
4. On the receiver, send `pair <remote-MAC> <same-key>`.

For example, the command shape is `pair AA:BB:CC:DD:EE:02 <32 hex digits>`; use the actual other board's address and the generated key. Pairing survives restarts. `unpair` removes it. `status` shows addresses and mappings but does not print the key.

Both devices use Wi-Fi channel 1 and encrypted unicast ESP-NOW. No router or Wi-Fi password is needed. The receiver also checks that packets came from the configured remote address. If changing the radio channel in the source, rebuild both boards.

## remapping

Send these commands to the receiver's USB serial console. They save in flash immediately, with no new firmware upload needed:

```text
map 1 left
map 2 right
status
```

For page up/down:

```text
map 1 pageup
map 2 pagedown
```

Other names: `up`, `down`, `space`, `enter`, `escape`, `home`, `end`, `a` through `z`, or `disabled`. For other keys, use a USB HID keyboard usage number, such as `0x3A` for F1. These are HID usages, not ASCII or Arduino `KEY_*` values. Supported numeric usages are 0x04 through 0x65.

An optional modifier byte supports shortcuts:

```text
map 1 a 0x08
map 2 b 0x02
```

That maps button 1 to Command+A on macOS, and button 2 to Shift+B. Modifier bits are left Ctrl 0x01, Shift 0x02, Alt/Option 0x04, GUI/Command 0x08; right-side equivalents are 0x10, 0x20, 0x40, 0x80. Add the bits for combinations. Media consumer-control usages and multi-step macros are not implemented.

Open the serial console again and use `status` if the boot message was missed. On macOS a terminal console can be opened with `screen <serial-port> 115200`; leave it with Ctrl+A, then K. Close the console before uploading firmware.

## behavior

- Button debounce is 15 ms.
- Full state packets go out on changes and every 50 ms. A later packet can recover a dropped release.
- The receiver drops duplicate and older sequence numbers within a remote boot session.
- After 300 ms without a valid new packet, the receiver releases all keys.
- Both pressed buttons appear together in one HID report. Identical mappings appear once.
- Failed USB reports are retried; a USB reconnect sends the current state again.
- The remote LED indicates a queued send, not a delivery acknowledgement. The receiver LED flashes on accepted packets.

This version keeps the remote's radio awake. Battery runtime and low-power sleep need measurements before choosing a power-saving strategy.

## when the boards arrive

- Check assembly, connector polarity, and resistance between power and ground before powering.
- Start with current-limited USB power and measure both 3.3 V rails. Address the open power issues in `docs/hardware-review-2026-10-03.md` before connecting a battery.
- Flash each board, open its console, and check `status`.
- Pair the devices. Verify the receiver appears as a keyboard and serial device.
- Test short presses, holds, and both buttons in a text editor, then in slides.
- Hold a button and turn the remote off. Confirm the computer releases the key within about 300 ms.
- Change mappings, restart both boards, and verify the saved settings.
- Disconnect/reconnect the receiver USB while a button is held. Verify there are no stuck keys.
- Measure battery supply sag and current during transmission. Check charging and charge termination with the selected battery and both switch positions.
- Check range and latency with the final case and battery in place.

References: [Arduino ESP-NOW](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/espnow.html), [Arduino USB](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/usb.html), [Espressif USB keyboard source](https://github.com/espressif/arduino-esp32/blob/3.3.12/libraries/USB/src/USBHIDKeyboard.cpp).
