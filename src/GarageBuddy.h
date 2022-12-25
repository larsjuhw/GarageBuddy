/*
@(#)File:            $RCSfile: GarageBuddy.h $
@(#)Version:         $Revision: 0.2 $
@(#)Last changed:    $Date: 2020/07/22 $
@(#)Purpose:         ESP8266 garage door controller
@(#)Author:          L. Wolter
*/

#include <Settings.h>

#define CLOSED 0x0
#define OPENED 0x1

#define MQTT_TOPIC_IN "debug/" MQTT_CLIENT "/in"
#define MQTT_TOPIC_OUT "debug/" MQTT_CLIENT "/out"
#define MQTT_FROM_HOMEBRIDGE "homebridge/from/set/" ACCESSORY_NAME
#define MQTT_TO_HOMEBRIDGE "homebridge/to/set"

#define PAYLOAD_TARGET                                    \
    "{\"value\":1,\"name\":\"" ACCESSORY_NAME             \
    "\",\"service_name\":\"" DOOR_SERVICE_NAME            \
    "\",\"characteristic\":\"TargetDoorState\"}"
#define PAYLOAD_CURRENT                                   \
    "{\"value\":1,\"name\":\"" ACCESSORY_NAME             \
    "\",\"service_name\":\"" DOOR_SERVICE_NAME            \
    "\",\"characteristic\":\"CurrentDoorState\"}"

#define PAYLOAD_SENSOR_DETECTED                           \
    "{\"value\":true,\"name\":\"" ACCESSORY_NAME          \
    "\",\"service_name\":\"" SENSOR_SERVICE_NAME          \
    "\",\"characteristic\":\"MotionDetected\"}"

#define PAYLOAD_SENSOR_UNDETECTED                         \
    "{\"value\":false,\"name\":\"" ACCESSORY_NAME  \
    "\",\"service_name\":\"" SENSOR_SERVICE_NAME          \
    "\",\"characteristic\":\"MotionDetected\"}"

const char* strOn = "on";
const char* strOff = "off";

void handlePinRead();
void publish_current_state();
void blip_sensor_state();
const char* boolToCstr(bool boolValue);
void wifiConnect();
void mqttReconnect();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void relayBlink();
void handleHomebridge(char* command);
bool handleInput(char* command);

void commandOTA();
void commandStatus();
void commandRestart();
void commandShadow();
void commandBlink();
void commandRelay();
void commandToggle();
void commandHelp();
void commandMsToggle();

typedef void (*func_type)(void);

#define COMMANDS_AMOUNT 10
func_type commandFunctions[COMMANDS_AMOUNT] = {
    commandOTA,
    commandStatus,
    commandRestart,
    commandShadow,
    commandBlink,
    commandRelay,
    commandToggle,
    commandHelp,
    commandMsToggle,
};

char commandNames[COMMANDS_AMOUNT][10] = {
    "ota",
    "status",
    "restart",
    "shadow",
    "blink",
    "relay",
    "toggle",
    "help",
    "mstoggle",
};