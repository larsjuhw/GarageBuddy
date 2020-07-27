/*
@(#)File:            $RCSfile: GarageBuddy.h $
@(#)Version:         $Revision: 0.1 $
@(#)Last changed:    $Date: 2020/07/22 $
@(#)Purpose:         ESP8266 garage door controller
@(#)Author:          L. Wolter
*/

#include <Settings.h>

#define CLOSED 0x0
#define OPENED 0x1

#define MQTT_TOPIC_IN "debug/" MQTT_CLIENT "/in"
#define MQTT_TOPIC_OUT "debug/" MQTT_CLIENT "/out"
#define MQTT_FROM_HOMEBRIDGE "homebridge/from/set/" HOMEBRIDGE_NAME
#define MQTT_TO_HOMEBRIDGE "homebridge/to/set"

#define PAYLOAD_TARGET                          \
    "{\"value\":1,\"name\":\"" HOMEBRIDGE_NAME  \
    "\",\"service_name\":\"" HOMEBRIDGE_SERVICE \
    "\",\"characteristic\":\"TargetDoorState\"}"
#define PAYLOAD_CURRENT                         \
    "{\"value\":1,\"name\":\"" HOMEBRIDGE_NAME  \
    "\",\"service_name\":\"" HOMEBRIDGE_SERVICE \
    "\",\"characteristic\":\"CurrentDoorState\"}"

const char* strOn = "on";
const char* strOff = "off";

void handlePinRead();
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
};