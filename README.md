﻿# GarageBuddy

Firmware for ESP8266 that controls a garage door opener and connects to MQTT to allow smart home integration.

## Explanation

This IoT garage door controller works by using a relay to connect the two wires used by your garage door controller's physical button to start the opener on demand. The firmware subscribes to an MQTT topic (garagedoor/set) and will briefly trigger the relay on command (no matter what the current door position is), which moves the garage door.

The firmware will read the value of the reed switch on every loop. The reed switch state is then debounced and the door position is updated.

## Requirements

* ESP8266 device
* Relay
* Reed switch (or more, they are fragile)
* Dupont cables (female-female)
* Cables for the relay and reed switch
* Soldering iron and solder (optional)


## Setup

This only works with garage door openers that can be controlled using a physical button.

### Home Assistant

I personally use Home Assistant, but other platforms should also work. This is my configuration:

```yaml
mqtt:
  - cover:
    - command_topic: "garagedoor/set"
      state_topic: "garagedoor/state"
      optimistic: false
      qos: 0
      retain: false
      payload_open: "OPEN"
      payload_close: "CLOSE"
      payload_stop: "STOP"
      state_open: "OPEN"
      state_closed: "CLOSED"
      device_class: "garage"
      unique_id: "garagedoor-esp"
      device:
        sw_version: "1.0.1"
        manufacturer: "..."
        name: "Garage Door"
        connections:
          - ["mac", "ff:ff:ff:ff:ff:ff"]
  - switch:
    - name: "Toggle Service"
      command_topic: "debug/gd/in"
      state_topic: "garagedoor/listening"
      payload_on: "toggle"
      payload_off: "toggle"
      state_on: "ON"
      state_off: "OFF"
      optimistic: false
      unique_id: "garagedoor-esp-toggle"
      device:
        sw_version: "1.0.1"
        manufacturer: "..."
        name: "Garage Door"
        connections:
          - ["mac", "ff:ff:ff:ff:ff:ff"]
    - name: "Relay Service"
      command_topic: "debug/gd/in"
      state_topic: "garagedoor/relay"
      payload_on: "relay"
      payload_off: "relay"
      state_on: "ON"
      state_off: "OFF"
      optimistic: false
      unique_id: "garagedoor-esp-relay"
      device:
        sw_version: "1.0.1"
        manufacturer: "..."
        name: "Garage Door"
        connections:
          - ["mac", "ff:ff:ff:ff:ff:ff"]
    - name: "Shadow Service"
      command_topic: "debug/gd/in"
      state_topic: "garagedoor/shadow"
      payload_on: "shadow"
      payload_off: "shadow"
      state_on: "ON"
      state_off: "OFF"
      optimistic: false
      unique_id: "garagedoor-esp-shadow"
      device:
        sw_version: "1.0.1"
        manufacturer: "..."
        name: "Garage Door"
        connections:
          - ["mac", "ff:ff:ff:ff:ff:ff"]
```

### Relay

First, connect your relay to your ESP8266 and specify the GPIO port in `Settings.h`. The button connected to your opener is connected using two wires. When you press this button, the two wires will make contact and the opener will open or close your door. We need to connect these wires to the relay, so that the relay can short them together. Make sure that you connect them to the correct ports, so that the two wires are not connected when the relay is off.

### Reed switch

Reed switches will allow electricity to travel when they are being affected by a sufficient magnetic field. Therefore, find a place where you can mount this switch and place a magnet on a moving part of your garage door, in a way that the magnet will affect this reed switch only when the door is fully closed. Now, connect one end of the reed switch to a GND pin and the other to the GPIO pin that you should also specify in `Settings.h`, preferably by soldering the wires to the reed switch.

### Settings

Configure your settings variables by defining them in the `Settings.h` header file. See `Example_Settings.h` for an explanation of the settings. All fields that are in the example file should also be in your settings file, otherwise the firmware can fail to compile.

### Flashing the firmware

Make sure that the Espressif 8266 platform is installed before compiling. Compile the firmware and flash it on your device. Read [this](https://docs.platformio.org/en/latest/platforms/espressif8266.html) if you are using PlatformIO, otherwise read [this](https://nodemcu.readthedocs.io/en/master/flash/).

## Debug commands

This firmware supports a few commands over MQTT. The topic to send these commands to is defined as `debug/MQTT_CLIENT/in` where `MQTT_CLIENT` is defined in `Settings.h`. The commands will output in `debug/MQTT_CLIENT/out`. None of the commands have parameters, they just toggle settings on and off. These are the supported commands:

| Command   |                                                               |
|:---------:|---------------------------------------------------------------|
| `help`    | Outputs the list of available commands                        |
| `status`  | Outputs a status message with the state of some settings      |
| `restart` | Restarts the device                                           |
| `shadow`  | Stops the sending of door states to Homebridge                |
| `door`    | 'Blinks' the relay as if Homebridge told it to                |
| `relay`   | Toggles the state of the relay                                |
| `toggle`  | Stops listening to Homebridge commands to open/close the door |
| `ota`     | Initialize ArduinoOTA to allow you to update the device OTA   |
| `ip`      | Outputs the current local IP address                          |
| `save`    | Save the current shadow, toggle, and relay state to EEPROM    |
