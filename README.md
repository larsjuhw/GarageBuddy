# GarageBuddy

This is one of the first things I made after I started learning C. Don't expect high quality code, but honestly it works really well for me. I might add some more compatibility in the future.

Currently, the compatibility of this project is very limited. It was made in PlatformIO and designed to specifically work with [Homebridge](https://github.com/homebridge/homebridge) and the [homebridge-mqtt](https://www.npmjs.com/package/homebridge-mqtt) plugin, although support for other services that use MQTT should not be difficult to add. Perhaps this is still helpful for someone who is new to C and/or microcontrollers.

The security largely depends on your MQTT broker's security and the security of the network that it is connected to. Only use this on private WiFi networks that are protected using a strong password. Make sure that your MQTT broker is password protect as well.

## Explanation

This IoT garage door controller works by using a relay to connect the two wires used by your garage door controller's physical button to start the opener on demand. Using Homebridge and the homebridge-mqtt plugin, the plugin will send an MQTT message when the target state of the garage door changes. The firmware will then briefly trigger the relay (no matter what the current and target door states are) to open or close the door.

The firmware will read the value of the reed switch on every loop. After detecting the same reed state x consecutive times (x defined by `REED_VALIDATION_COUNT`), it will accept this new state and send a message to the homebridge-mqtt plugin to change the current state AND the target state. However, when you open the door it will wait at least x milliseconds (x defined by `DOOR_OPENING_TIME`) before sending these messages. The reason for this is that the iOS Home app will show the garage door as '_opening_' or '_closing_' when the target state is not equal to the current state. Closing the door will not have this delay, as we know for sure that the door is closed when the reed detects it.

## Requirements

* ESP8266 device
* Relay
* Reed switch (or more, they are fragile)
* Dupont cables (female-female)
* Cables for the relay and reed switch
* Soldering iron and solder (optional)


## Setup

This only works with garage door openers that can be controlled using a physical button.

### Homebridge

Make sure that the homebridge-mqtt plugin is installed and configured. Add a new accessory by sending a message to `homebridge/to/add` like such:
```json
{
    "name": "your-accessory-name",
    "service_name": "your-service-name",
    "service": "GarageDoorOpener",
    "manufacturer": "example",
    "model": "example",
    "serialnumber": "0000",
    "firmwarerevision": "0.1"
}
```

This accessory should now be visible for HomeKit and Homebridge UI.

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
| `blink`   | 'Blinks' the relay as if Homebridge told it to                |
| `relay`   | Toggles the state of the relay                                |
| `toggle`  | Stops listening to Homebridge commands to open/close the door |
| `ota`     | Initialize ArduinoOTA to allow you to update the device OTA   |