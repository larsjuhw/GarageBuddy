/*
This is an example file for Settings.h
All these constants are required to be defined, it likely will not compile otherwise.


#include <stdint.h>                                     // These two headers are only included to provide the ability
                                                        // to define GPIO pins as D1, D2 etc.
#include <pins_arduino.h>

#define WIFI_SSID               "Example"               // The SSID that the controller connects to
#define WIFI_PASSWORD           "1234567890"            // The password of that SSID

#define MQTT_IP                 "192.168.0.200"         // The IP address of your mqtt broker
#define MQTT_CLIENT             "gd"                    // The MQTT client name that your device will use
                                                        // This is also used in the MQTT debug topics
#define MQTT_USERNAME           "username"              // Username to use for MQTT authentication
#define MQTT_PASSWORD           "password"              // Password to use for MQTT authentication

#define PIN_REED                D7                      // The GPIO pin that the reed is connected to (the other end is connected to GND)
#define PIN_RELAY               D6                      // The GPIO pin that the relay is connected to

#define DEBOUNCE_DELAY          500                     // The delay in milliseconds to debounce the reed switch
#define BLINK_DELAY             1000                    // The minimum delay between door activations. This is to prevent the door from being activated too quickly.
#define BLINK_DURATION          150                     // The duration in milliseconds that the relay is activated

*/
