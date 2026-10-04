# Journal

devlogs for progress

### Devlog 01

Date: Sep 19
Time spent: 1 hour
Lapse: No lapse

Initialized the repo, git, created the 2 kicad projects, created the project structure, wrote up a readme.

### Devlog 02

Date: Sep 20
Time spent: 30 minutes
Lapse: [lapse](https://lapse.hackclub.com/timelapse/JvdpDzcCZuD7)

Finished the remote schematic.
Placed the schematic symbols and wired everything up with labels.

### Devlog 03

Date: Sep 20
Time spent: 30 minutes
Lapse: [lapse](https://lapse.hackclub.com/timelapse/a2XuRAxTbL4d)

Finished the reciever schematic.
Placed the schematic symbols and wired everything up with labels.
Also fixed something from the remote schematics: the schottky diodes were backwards.
I also had to add power flags for both schematics, which i forgot about.

### Devlog 04

Date: Sep 20
Time spent: 1 hour
Lapse: [lapse](https://lapse.hackclub.com/timelapse/9pbpZWGdKg2V)

i found exact part numbers i needed for each symbol, and assigned all of them footprints, and updated both pcbs from both schematics. i routed the reciever pcb, but there are 200 errors that i have to work through, most of them are clearance violations that are probably fine and will go away once i adjust stuff.
i had to make my own footprint for the switch btw

### Devlog 05

Date: Sep 20
Time spent: 1 hour
Lapse: no lapse

i found the errors:
root causes all mechanical: tight outline, strict hole rule, one redundant via
expanded outline, corrected hole rule to fab capability, removed via
result: zero blocking errors, thermals stuff accepted, doing silkscreen cleanup now

### Devlog 06

Date: Sep 22
Time Spent: 30 minutes
Lapse: [lapse](https://lapse.hackclub.com/timelapse/fsMvkNjTQimd)

routed the remote board!
also added edge.cuts outline with rounded edges, cleaned up silkscreen, flipped some tht components to the back like the switch, debug header, and batt connector

### Devlog 07

Date: Sep 26
Time Spent: 1 hour
Lapse: no lapse

fixed a short in the remote board, make all production files, sourced parts for the boms

### Devlog 08

Date: Oct 3
Time Spent: 2 hours
Lapse: no lapse

made firmware for both boards: slide arrows by default, saved key remapping over USB, and configurable encrypted pairing.
both scripts compile
Fixed five thermal errors and the remot’s ERC error. Both PCBs now have zero DRC errors and unrouted connections, warnings are still there
refreshed Gerbers and wrote setup instructions

### Devlog 09

Date: Oct 3
Time Spent: 2 hours
Lapse: no lapse

I sourced exact links and prices for each part of the project. i tried to optimize it for cost, but it is ~$30 over budget. I can try to pay for the rest myself, but.. im kinda broke. Ill see if i can scrounge together some coupons before i order.

### Devlog 10

Date: Oct 3
Time Spent: 3.5 hours
Lapse: no lapse

made cases for the remote and receiver, with openings for USB-C, the LEDs, and the buttons. The boards sit on little supports and are held in place by the lids, so I didn't need to change either PCB.
The first remote case was way too thick. I had stacked the battery underneath the connectors, so I moved it beside the PCB and removed the extra power slider and feet. That brought the body down from 29 mm to 21.7 mm thick, although it is now wider.
I also made narrower printed keycaps because the original ones would have no gap between them. These leave a 1.1 mm gap, and I checked that they can move through the full switch travel without hitting the case.

### Devlog 11

Date: Oct 4
Time Spent: 2 hours
Lapse: no lapse

prepared this project for submission! i listed all items in the cart thing on this website, went through the order process on all the websites, took pictures, and made a simple reel video that shows the schematics, cad, and pcb renders.