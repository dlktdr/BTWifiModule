# BTWifiModule
Frsky Bluetooth Emulator

This code is for use on a ESP Wroom 32, it emulates a FRSky Bluetooth modulw but with way more power and possibilities.

Update: 
Nov 9.Now can scan/connect/disconnect to a remote BLE device in Master/Bluetooth Mode. Still sends out simulated data on connection.
Apr 23 BT now works in Master Trainer Mode

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
- [ ] OTA Radio firmware update (Probably EdgeTX only)
- [ ] Mavlink Router Using a Wifi AP
- [ ] A2DP Audio Source, Mirror Radio Audio

On the ideas list.. 
-----------------
- Design jumper board to have available for installation in the radio. W/ESP WROOM32U (External Antenna)
