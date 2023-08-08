
# Photosensitive Power Relay (Arduino)

Build on Adafruit's [Feather M0 Bluefruit LE](https://www.adafruit.com/product/2995), this photosensitive power relay allows for controlling a string of AC lights with BLE. Presently, the relay supports the following:

- Automatic power-on at dusk using sensible defaults. The light threshold value for turning
on/off can be adjusted in volatile memrory. This value resets to default on power cycle.

- An automatic power-off timer during the night using a sensible default. This avoids having
the lights on all night. The value is customizable in volitile memory and resets on power cycle.

- Temporary override to toggle the lights. This feature will override the current
power state until light levels cross the next threshold. After this, the unit will
automatically return to the pre-programmed schedule.

- Wireless command and control over BLE UART with a simple menu system (once connected
send "0" to show the main menu). This unit requires a special BLE app and will not
work with your phone without it. Here are some suggestions:
  - ANDROID: <https://play.google.com/store/apps/details?id=de.kai_morich.serial_bluetooth_terminal>
  - APPLE:   <https://apps.apple.com/app/adafruit-bluefruit-le-connect/id830125974>

- Given that this is simply a light control with no IP connectivity, security is not a top
  priority. That said, while this device can be controlled by anyone, it...
  - does not store sensitive information,
  - only works using a UART app,
  - does not advertise how to make changes when connected,
  - validates input and silently ignores invalid data.

# CHANGE LOG

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
