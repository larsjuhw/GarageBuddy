/*
@(#)File:            $RCSfile: GarageBuddy.h $
@(#)Version:         $Revision: 1.0 $
@(#)Last changed:    $Date: 2024/03/05 $
@(#)Purpose:         ESP8266 garage door controller
@(#)Author:          L. Wolter
*/

#pragma once

#include <Settings.h>

#define DEBUG 0

#define debugPrintln(message)               \
    do {                                    \
        if (DEBUG) Serial.println(message); \
    } while (0)

#define debugPrint(message)               \
    do {                                  \
        if (DEBUG) Serial.print(message); \
    } while (0)

#define CLOSED 0x0
#define OPEN 0x1

#define MQTT_TIMEOUT            5000

#define MQTT_TOPIC_DEBUG_IN "debug/" MQTT_CLIENT "/in"
#define MQTT_TOPIC_DEBUG_OUT "debug/" MQTT_CLIENT "/out"
#define MQTT_TOPIC_SHADOW "garagedoor/shadow"
#define MQTT_TOPIC_RELAY "garagedoor/relay"
#define MQTT_TOPIC_LISTENING "garagedoor/listening"
#define MQTT_TOPIC_UPTIME "garagedoor/uptime"
#define MQTT_TOPIC_IN "garagedoor/set"
#define MQTT_TOPIC_STATE "garagedoor/state"

#define PAYLOAD_ON "ON"
#define PAYLOAD_OFF "OFF"
#define PAYLOAD_CLOSE "CLOSE"
#define PAYLOAD_OPEN "OPEN"
#define PAYLOAD_STOP "STOP"
#define STATE_OPEN "OPEN"
#define STATE_CLOSED "CLOSED"

void wifiConnect();
void mqttReconnect();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void onDoorStateChange();
void publishDoorState();