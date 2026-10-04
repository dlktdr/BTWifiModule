# BTWifiModule

FrSky Bluetooth emulator for ESP32, to be used inside an OpenTX/EdgeTX radio.
It emulates a FrSky Bluetooth module and adds **Bluetooth telemetry**
(S.Port and CRSF), **auto-connect** and a **selectable BLE host**
(Bluedroid or NimBLE).

Based on the community project by **dlktdr**
(https://github.com/dlktdr/BTWifiModule).
Documentation: https://btwifimod.gitbook.io/

## Features

| Feature | Status |
|---|---|
| Bluetooth Trainer — Master Mode (connect to a BLE trainer / headtracker) | Supported |
| Bluetooth Trainer — Slave Mode | Supported |
| Bluetooth Telemetry over BLE | Supported |
| — S.Port (FrSky models) | Supported |
| — CRSF (ELRS models) | Supported |
| Auto-connect to the last known device | Supported |
| Device name from `AT+NAME` (persisted and advertised) | Supported |
| Selectable BLE host: Bluedroid or NimBLE | Supported |

## How telemetry works

In *Telemetry* mode EdgeTX forwards the radio's telemetry over the BT serial
link. The module passes those bytes to the connected BLE central unchanged:

- FrSky models: S.Port frames, `0x7E`-delimited and byte-stuffed.
- ELRS/CRSF models: raw CRSF frames.

The client app auto-detects the protocol, so both work over the same GATT
service (`0xFFF0`, characteristic `0xFFF6`, NOTIFY).

## Telemetry app

- S.Port: **INAV / Telemetry Viewer**.
- CRSF: the maintained fork by **RomanLut**
  (https://github.com/RomanLut/android-taranis-smartport-telemetry/releases),
  which publishes ready-made APKs with CRSF support.

## Hardware

- **ESP32-C3 SuperMini** (primary target); other boards are listed in
  `platformio.ini`.
- UART to the radio: **GPIO20 (RX) / GPIO21 (TX)** on the C3 SuperMini.
- **Radiomaster TX16S MK3** with **EdgeTX** (a recent nightly is recommended,
  see below).

> **TX16S MK3 note.** EdgeTX needs PR #7855
> (`BT_PWR_GPIO = PC13` in `targets/tx16smk3/hal.h`) to power the AUX2/BT
> connector. It is included in current nightlies.

## Building

PlatformIO (ESP-IDF). Two BLE hosts are selectable:

| Environment | BLE host | Flash (C3, 1 MB app) | Notes |
|---|---|---|---|
| `C3SuperMini` | Bluedroid (default) | ~84 % | |
| `C3SuperMini_nimble` | NimBLE | ~52 % | smaller footprint, recommended |

```bash
pio run -e C3SuperMini -t upload          # Bluedroid
pio run -e C3SuperMini_nimble -t upload   # NimBLE
```

The Bluedroid path is kept behind `#if !defined(USE_NIMBLE)`, so both hosts
share the same application code.

## AT command set

The module speaks the same AT protocol EdgeTX uses for the FrSky Bluetooth
module:

| Command | Meaning |
|---|---|
| `AT+BAUD4` | baud rate (this firmware stays at 115200) |
| `AT+NAME<name>` | set the Bluetooth name (persisted and advertised) |
| `AT+TXPW<n>` | TX power (acknowledged) |
| `AT+ROLE0` | Peripheral (telemetry / trainer slave) |
| `AT+ROLE1` | Central (trainer master) |
| `AT+DISC?` | start a scan (central) |
| `AT+CON<addr>` | connect to a device (central) |
| `AT+CLEAR` | forget the saved device |
| `AT+HTRESET` | reset the headtracker (BTWifiModule extension) |

Responses: `OK+...`, `Central:<addr>`, `Peripheral:<addr>`, `Connected:<addr>`,
`DisConnected`, `ERROR`.

## Branches

- `1.0-dev` — consolidated development branch (this one).

## Credits

Original project: **dlktdr/BTWifiModule**. Bluetooth telemetry, auto-connect,
the CRSF passthrough and the NimBLE port by this fork.
