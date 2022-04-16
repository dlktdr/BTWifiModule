# BTWifiModule
Frsky Bluetooth Emulator (To Start)

This code is for use on a ESP Wroom 32, it emulates a FRSky Bluetooth modulw but with way more power and possibilities.

Update: Nov 9. Now can scan/connect/disconnect to a remote BLE device in Master/Bluetooth Mode. Still sends out simulated data on connection.

To Do (Plan is to work top down)
---------
- [x] At Modem Command Set to Radio via UART
- [x] Bluetooth Master/Bluetooth Scan and Device Connection
- [ ] Bluetooth Trainer Receiver (Master / Bluetooth)
- [ ] Bluetooth Trainer Transmitter (Slave / Bluetooth)
- [ ] Bluetooth Telewmetery Transmitter
- [ ] Wifi Mode Selection (Extra AT commands including settings) (Access Point / Device / Ad-Hoc)
   - Wifi Modes
     - Trainer Not sure on a good mode here yet, probably not infrastructure mode tho, point to point maybe in 11b LR mode. 
       **EDIT:** I think ESPnow is a perfect solution, point to point and still works in LR mode.
     - Telemetry
     - Mavlink Router (Eventually, with radio support)
- [ ] OTA Firmware updates for ESP (In AP mode via HTTP server)
- [ ] OTA Radio firmware update (Probably EdgeTX only)

On the ideas list.. 
-----------------
- Design jumper board to have available for installation in the radio. W/ESP WROOM32U (External Antenna)
