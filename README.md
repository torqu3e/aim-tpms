# Motivation

I was having a tough time dialing tire pressures with warmers, measuring before/after pressures and temperatures to determine how I was riding on the track. The instantaneous measurements are useful but only that, and they are tedious to complete so I got thinking. Since I had already added TPS and brake pressure logging to the AIM dash (MXM) on the Kramer Evo2R, and it technically supports some 60 odd extra channels over the ECU CANBUS, I figured I could use it to log tire pressure data. I am also training (physically) to ride better, and harder so heart rate data would also be useful to determine what training would work best.

It's a nice fun winter project to complete which hopefully will pay dividends on the track.

# Description

This project uses an ESP32 microcontroller that has a built in bluetooth stack to connect to TPMS sensors and a Heart Rate Monitor (HRM) chest strap. The ESP32 does some processing, converts the data into proper format for the AIM ECU protocol and outputs the data over serial to the CANBUS PHY which is logged by the AIM dash.

In theory this is highly extensible since the AIM ECU protocol supports 60+ channels. It should also work with mostly any AIM logger that allows an ECU CANBUS connection. The Solo2 DL does but then you can't use a secondary analog source like the ACC2 without a data expansion hub or CANBUS splitter.

# Research notes

It took me some effort getting this all together. There was a bunch of research first around TPMS sensors, then connecting them to the ESP32. Then there was the same work for the HRM chest strap. Figuring the AIM ECU protocol, and isolating hardware vs software issues was a bunch of evenings spent doing all this.

Significant parts of the code here is not mine, the fundamental parts are picked from the references below, some I may have even forgot where I found them (if its from you please let me know so I credit you).

The AIM protocol part is something I could not find prior art for, so I had to figure that myself. If it looks ugly and you have suggestions to make it more elegant, shoot em my way.

My contribution here would mostly be around bringing the various separate bits together, and making it work cohesively.


# Features
- Connects to TPMS sensors over BLE to read tire pressure, temperature, and sensor battery voltage
- Connects to an HRM chest strap over bluetooth to read heart rate
- Does necessary conversions and writes values to CANBUS using the Aim ECU protocol
- Configurable CANBUS speed (default: 1 Mbps)

# References
- [AIM ECU Protocol](https://support.aimshop.com/pdf/racing-ecus/AiM/CAN.pdf)
- [AIM MXM Pinout - Page 64](https://support.aimshop.com/downloads/product-documents/mxm/MXm_user_guide.pdf)
- [RA6070 BLE TPMS](https://github.com/ra6070/BLE-TPMS)
- [ANDI32 TPMS](https://github.com/andi38/TPMS)
- [Arduino CANBUS library](https://github.com/sandeepmistry/arduino-CAN) - this has some weird compilation issue with v3 of the ESP board definitions but works fine with the latest v2
- [CANBUS article](https://lastminuteengineers.com/esp32-can-bus-tutorial/) - great readup on the workings of CANBUS if you are new to it, helped me a bunch since this is my first project working with it
- Add reference for HR code here


# Hardware
- [TPMS sensor](https://www.aliexpress.com/item/1005007507568688.html) titled:"Motorcycle Bluetooth-Compatible Tire Pressure Monitoring System TPMS Tire Pressure Sensors External Sensor Android/IOS Car TMPS" in case that link calls it quits
- [Coospo H6M heart rate monitor chest strap](https://www.aliexpress.com/item/1005005059886054.html)
- [TJA1050 CANBUS transceiver](https://www.aliexpress.com/item/1005006832397047.html)
- [Autosport Size 22 pins](https://www.prowireusa.com/p-1552-size-22-contact-skt-deutsch-autosort.html)
- [Pin insert/remove tool](https://www.prowireusa.com/p-1608-removal-tool-size-22-23-plastic-autosport.html)
- [Rubber boot](https://www.prowireusa.com/p-532-90-lipped-boot.html)
- [Epoxy for rubber boot](https://www.prowireusa.com/resin-tech-rt125-ds-050-epoxy.html) This one is a bit pricey and sorta optional but I'd use it.

# Setup
**(assumes android phone)** Install nRF Connect app. Start it scanning and place the TPMS sensor on the tire valve. The sensors only transmit when they detect a change in pressure so it only works for a short while before you will need to do this again.

You are looking for a new device showing up that you can't connect to but looks like so

Note the ID of that device. Repeat with the other sensor. Both IDs are needed

Similarly so, set nrfConnect scanning, wear the HRM chest strap to get the service UUID (starts with 0x180d) and characteristic UUID (starts with 0x2a37). Note both IDs.

Wire the TJA1050 CANBUS transceiver to the ESP32 over pin 5 (TX) and pin 4 (RX). The CANBUS transceiver gets wired to the CANBUS lines on the AIM dash. For the MXM, that's pin 8 (CANH) and pin 9 (CANL).

The MXM wiring harness on the Evo2R does not expose all the pins so the connector boot had to be cut open, pins soldered to wires, and pins inserted in place (8 and 9). Would've been nice if Kramer had exposed all the pins on the MXM connector.

Set the values in the HRM_basic.ino file to match the IDs you found. Ref Line 15-18. Upload the code to the ESP32 and run it.

The AIM dash will need to be configured to read the ECU channels. Once configured, it should display 0.0 for the channels till the HRM or TPMS sensors are connected.

# Support

If you have any questions or need clarifications on any bit, please feel free to reach out to me and I can try to do my best. Having said that, this is mostly meant as a guide on how this can be done and not a product offering.
