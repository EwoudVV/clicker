# Clicker

A two-button wireless remote for presentations. I wanted something small that I could use to change slides without staying next to my computer. The buttons default to left and right arrow, but they can be changed to other keys or shortcuts over USB.

![Remote and receiver cases](enclosures/preview.png)

## How it works

There are two boards. The battery-powered remote has two Kailh Choc V1 switches and an ESP32-C3. It sends the button states to an ESP32-S3 receiver over ESP-NOW. The receiver plugs into the computer through USB-C and acts as a USB keyboard, so it does not need a separate keyboard driver. The wireless link does not need a router.

Both boards have BOOT and RESET buttons, a programming header, and status LEDs. The remote also has a power switch, a battery connector, and a USB-C charging port. Both PCBs are two-layer boards.

### Remote PCB

<img width="250" alt="Remote PCB render" src="https://github.com/user-attachments/assets/7a14af57-7081-4eec-b738-5d1f520fa33b" />

### Receiver PCB

<img width="250" alt="Receiver PCB render" src="https://github.com/user-attachments/assets/d6dfd0ec-901f-4844-9c71-74fa7b48f5f0" />

## Using it

After flashing and pairing the boards:

1. Plug the receiver into the computer with a USB data cable.
2. Turn on the remote and open the presentation. Click the presentation window so it has keyboard focus.
3. Button 1 sends left arrow for the previous slide. Button 2 sends right arrow for the next slide. Use the remapping commands below if the presentation software expects different keys.
4. Turn off the remote when finished. The current firmware keeps the radio awake while it is on.

Holding a button holds the corresponding keyboard key. If the wireless link stops, the receiver releases the keys after 300 ms to avoid a stuck key.

### First setup

The [firmware guide](firmware/README.md) has the build commands, board settings, upload instructions, and pin assignments. The remote and receiver use different ESP32 targets, so each needs its own sketch.

Once both are flashed, open each board's USB serial console at 115200 baud with newline enabled. Send `status` to get its MAC address. Generate a shared pairing key on the computer:

```sh
python3 -c 'import secrets; print(secrets.token_hex(16))'
```

Send `pair <receiver-MAC> <key>` to the remote, then `pair <remote-MAC> <same-key>` to the receiver. Replace the placeholders with the actual addresses and the generated 32-character key. Pairing is saved through restarts. `unpair` clears it.

### Changing the buttons

Send these commands to the receiver's USB serial console to use page up and page down:

```text
map 1 pageup
map 2 pagedown
status
```

For the original slide controls, use `map 1 left` and `map 2 right`. Changes save immediately and do not need a firmware upload. Letters, other keyboard keys, and modifier combinations are supported too. The [firmware guide](firmware/README.md#remapping) lists the options. Media keys and sequences of multiple key presses are not implemented.

## Parts and assembly

The [full BOM](BOM.csv) covers one remote and one receiver, including the battery, case screws, and PCB orders. It includes supplier links, quantities used, and quantities bought. Some parts come in minimum packs, and each PCB quote is for five boards.

The submitted parts estimate is **$40.78**, plus a **$23.54 provisional shipping and tax allowance**, for **$64.32**. These are the submission quotes, not current checkout prices. Final shipping, taxes, and PCB discounts still need checking before ordering.

The board-level component lists are [remote BOM](remote/production/bom.csv) and [receiver BOM](receiver/production/bom.csv). The full BOM records the supplier substitutions selected during sourcing, including the LEDs and power switch that still need checking before purchase.

Solder the small surface-mount parts first, then the modules and connectors. Check orientation and inspect for bridges before powering either board. Bring up the boards on current-limited USB power and check the 3.3 V rails before flashing them.

The remote currently has a 2k charge-program resistor, which sets roughly 500 mA. Check this against the selected battery's charging limit and change the resistor if needed before connecting the battery. Also check the battery connector polarity. Charging, battery runtime, and operation as the battery discharges still need testing.

## Cases

Both boards sit on printed supports and are held down by the lids. Each case uses four M2 x 6 mm screws. The remote's battery sits beside the PCB to keep the case thinner.

The switch centres are 16.5 mm apart, so I made narrower printed keycaps with a 1.1 mm gap. Print the stem-fit sample before the keycaps and check that they seat without forcing the switch sockets.

- [Print plate](enclosures/clicker-print-plate.3mf)
- [Individual STL files](enclosures/stl)
- [Case download with STEP models and assembly instructions](enclosures/clicker-enclosures.zip)

Use PLA or PETG, around 0.16 to 0.20 mm layers, and at least three walls. Select the printer and material in the slicer. Check the USB openings, screw holes, and keycap stems in the slice preview before printing.

## Design files and progress

The editable KiCad projects are in [remote](remote) and [receiver](receiver). Ready-to-upload Gerber ZIPs are in [remote/production](remote/production/gerbers.zip) and [receiver/production](receiver/production/gerbers.zip). The [journal](journal.md) has the design progress.

The boards are routed, the firmware has compiled, and the cases have been checked in CAD. Nothing has been assembled or physically tested yet. The next step is to check the remaining part choices, build a pair, and test charging, button response, wireless range, and case fit.

Designed by EVV

*Made for Hack Club Half Life*
