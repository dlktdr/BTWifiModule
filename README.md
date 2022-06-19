# BTWifiModule
Frsky Bluetooth Emulator

This code is for use on a ESP Wroom 32, it emulates a FRSky Bluetooth module but with way more future possibilities.

# Supported Boards

So far this has been tested on the PICO / WROOM and C3 Boards. It is still a work in progress, if you have any questions please ask in the HeadTracker **#dev** [Discord](https://discord.gg/ux5hEaNSPQ) or add an [issue](https://github.com/dlktdr/BTWifiModule/issues) here. See the pin connections here for the supported boards. 
https://github.com/dlktdr/BTWifiModule/blob/57d511cb12d4203ff344db9a17360db8373ac9f0/src/defines.h#L5-L22 

Currently won't connect to another PARA module, missing something here still.
Haven't tested connecting to a CC2540 module yet.

To Do (Plan is to work top down)
---------
- [x] At Modem Command Set to Radio via UART
- [x] Bluetooth Master/Bluetooth Scan and Device Connection
- [x] Bluetooth Trainer Receiver (Master / Bluetooth)
- [X] Bluetooth Trainer Transmitter (Slave / Bluetooth)
- [ ] Bluetooth Telemetry Transmitter
- [ ] ESPNow Trainer Master
- [ ] ESPNow Trainer Slave
- [ ] OTA Firmware updates for ESP (In AP mode via HTTP server)
- [ ] Rework serial interface for more advanced features
- [ ] OTA Radio firmware update (Probably EdgeTX only)
- [ ] Mavlink Router Using a Wifi AP
- [ ] A2DP Audio Source, Mirror Radio Audio
