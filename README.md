# BTWifiModule
Frsky Bluetooth Emulator

This code is for use on a ESP Wroom 32, it emulates a FRSky Bluetooth module but with way more future possibilities.

# Supported Boards

So far this has been tested on the PICO / WROOM32 and C3mini Boards. I would recommended to buy the listed boards in the external antenna versions if you plan on mounting inside a case, buy a adapter to RP-SMA mount this on your case and you can use a generic wifi antenna on the outside. These boards can be bought very cheaply, around $7 - $15 depending on the versions. The code isn't very large you can buy the cheapest 4Mb versions.

Choosing the ESP32-C3-DEVKITM-1U board would be my choice at the moment, as it will be able to support the newer features of BT5.0, when available. [Digikey Link](https://www.digikey.ca/en/products/detail/espressif-systems/ESP32-C3-DEVKITM-1U/15198974) See the Wiki Page for possible external antenna parts. [Boards and Hardware](https://github.com/dlktdr/BTWifiModule/wiki/Boards-and-Hardware)

**NOTE** This is still a work in progress, there will be some issues. If you have any questions please ask in the HeadTracker **#dev** [Discord](https://discord.gg/ux5hEaNSPQ) or add an [issue](https://github.com/dlktdr/BTWifiModule/issues) here. 

## RX/TX Pin Connections are Here
https://github.com/dlktdr/BTWifiModule/blob/main/src/defines.h

**This does require the Bluetooth option to be compiled/enabled in on the OpenTX or EdgeTX firmware.**

## Notes

* Currently won't connect to another PARA module, missing something here still.
* Haven't tested connecting to a CC2540 module yet.
* Works okay to the HeadTracker or to Another ESP module.

To Do (Plan is to work top down)
---------
- [x] At Modem Command Set to Radio via UART
- [x] Bluetooth Master/Bluetooth Scan and Device Connection
- [x] Bluetooth Trainer Receiver (Master / Bluetooth)
- [X] Bluetooth Trainer Transmitter (Slave / Bluetooth)
- [ ] Rework serial interface for extra modes including modifications to edgetx
- [ ] Bluetooth Joystick device
- [ ] A2DP Audio Source, Mirror Radio Audio
- [ ] Bluetooth Telemetry Transmitter
- [ ] ESPNow Trainer Master
- [ ] ESPNow Trainer Slave
- [ ] OTA Firmware updates for ESP (In AP mode via HTTP server)
- [ ] OTA Radio firmware update (Probably EdgeTX only)
- [ ] Mavlink Router Using a Wifi AP

