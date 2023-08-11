:warning: UPDATE: This project has been [ported to CircuitPython](#) and upgraded with features like: intelligent auto-timing, WiFi, Web API and more.

# Photosensitive Power Relay (Arduino)

Built on Adafruit's [Feather M0 Bluefruit LE](https://www.adafruit.com/product/2995), this photosensitive power relay allows for controlling a string of AC lights (or any AC device) with BLE.

<img alt="Photosensitive Power Relay" src="/img/photo-pwr-relay.jpg" width=400><br/>

## Key Features

- Automatic power-on at dusk using sensible defaults. The light threshold value for turning on/off can be adjusted in volatile memrory.

- An automatic power-off timer during the night using a sensible default (avoids having the lights on all night). The value is customizable in volatile memory and resets on power cycle.

- Temporary override to toggle the relay. This feature will override the current power state until photostatic levels cross the next threshold. After this, the unit will automatically return to the pre-programmed schedule.

- Wireless command and control over BLE UART with a simple menu system (once connected send "0" to show the main menu). This unit requires a special BLE app and will not work with your phone without it. Here are some suggestions:
  - ANDROID: <https://play.google.com/store/apps/details?id=de.kai_morich.serial_bluetooth_terminal>
  - APPLE:   <https://apps.apple.com/app/adafruit-bluefruit-le-connect/id830125974>

## Security
Given that this is simply an AC power control relay with no IP connectivity and limited BLE range, security is not a top priority. That said, this project...
  - does not store sensitive information,
  - only works using a UART app,
  - does not advertise how to make changes when connected,
  - validates and silently ignores invalid input.

# Change Log

## 2022.10.05 - v1.0 RELEASE

- Fixed a bug where the lights would turn on briefly at dawn
- Finalized photoThreshold levels & updated the UART UI
- Minor UI performance improvements while BLE connected
- Minor tweaks & bugfixes

## 2022.09.21 - v1.0 BETA

- Added text-based menu system via UART
- Added override, photoresistor reporting over UART and system reset features
- Added input validation
- Replaced Adafruit's isConnected() feature with my own for more reliability
- Refactored for performance and stability
- Many, many bugfixes

## 2022.09.16 - v1.0 ALPHA

- Initial writing
