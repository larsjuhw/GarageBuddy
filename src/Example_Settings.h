/*
This is an example file for Settings.h
All these constants are required to be defined, it likely will not compile otherwise.


#include <stdint.h>                                     // These two headers are only included to provide the ability
                                                        // to define GPIO pins as D1, D2 etc.
#include <pins_arduino.h>

#define DEBUG                   1                       // Whether debug mode should be enabled or not
                                                        // At the moment it only toggles the Serial connection

#define WIFI_SSID               "Example"               // The SSID that the controller connects to
#define WIFI_PASSWORD           "1234567890"            // The password of that SSID

#define MQTT_IP                 "192.168.0.200"         // The IP address of your mqtt broker
#define MQTT_CLIENT             "gd"                    // The MQTT client name that your device will use
                                                        // This is also used in the MQTT debug topics

#define ACCESSORY_NAME         "gd"                    // The name of your Homebridge accessory
#define DOOR_SERVICE_NAME      "Garage Door"           // The service name of your Homebridge accessory

#define PIN_REED                D7                      // The GPIO pin that the reed is connected to (the other end is connected to GND)
#define PIN_RELAY               D6                      // The GPIO pin that the relay is connected to

#define REED_VALIDATION_COUNT   50000                   // If the door state changes (open/closed), this new state will only be
                                                        // accepted after this number of consecutive reads of the reed pin.
                                                        // 50000 seems to be a pretty good value as it won't even take 2 seconds.
#define DOOR_OPENING_TIME       15000                   // Time it takes for the garage door to open in milliseconds
                                                        // The controller will wait this amount of time before it sends the message
                                                        // to Homebridge to change the current and target door states

#define MQTT_TIMEOUT            5000                    // The amount of milliseconds before retrying after failing to connect to MQTT 

#define BLINK_DURATION          150                     // Time the relay gets its voltage switched before reverting in milliseconds
#define BLINK_MIN_DELAY         750                     // Minimum milliseconds between two relay 'blinks'
#define LEFT_OPEN_TIME          30                      // Seconds of door being left open until a notification is sent

*/
