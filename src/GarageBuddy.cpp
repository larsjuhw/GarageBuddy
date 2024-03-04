/*
@(#)File:            $RCSfile: GarageBuddy.cpp $
@(#)Version:         $Revision: 1.0 $
@(#)Last changed:    $Date: 2024/03/05 $
@(#)Purpose:         ESP8266 garage door controller
@(#)Author:          L. Wolter
*/

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <GarageBuddy.h>
#include <PubSubClient.h>

#include <functional>
#include <map>

#include "CommandHandler.h"
#include "DoorController.h"
#include <Settings.h>

WiFiClient espClient;
PubSubClient client(espClient);
DoorController doorController(&onDoorStateChange);
CommandHandler commandHandler(client, doorController);

unsigned long lastUpdate = 0;

void setup() {
    delay(10);

    #if DEBUG
        Serial.begin(115200);
        while (!Serial) {
            continue; // Wait for serial port to connect
        }
        Serial.println();
        Serial.println("Booting..");
    #endif

    wifiConnect();

    client.setServer(MQTT_IP, 1883);
    client.setCallback(mqttCallback);

    ArduinoOTA.setHostname("garagepoort-esp");

    commandHandler.setup();
    doorController.setup();
}

void loop() {
    if (!client.connected()) {
        mqttReconnect();
    }

    doorController.handlePinRead();
    client.loop();
    ArduinoOTA.handle();

    // Update the states and uptime every 10 minutes
    if (millis() - lastUpdate > 600000) {
        commandHandler.publishStates();
        lastUpdate = millis();
    }
}

void wifiConnect() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    debugPrint("(WIFI) Connecting to ");
    debugPrint(WIFI_SSID);

    while (WiFi.status() != WL_CONNECTED) {
        debugPrint(".");
        delay(500);
    }

    debugPrintln(" online!");
}

void mqttReconnect() {
    while (!client.connected()) {
        debugPrint("(MQTT) Attempting MQTT connection.. ");
        if (client.connect(MQTT_CLIENT, MQTT_USERNAME, MQTT_PASSWORD)) {
            debugPrintln("connected");
            client.publish(MQTT_TOPIC_DEBUG_OUT, "I am online!");
            commandHandler.publishStates();

            client.subscribe(MQTT_TOPIC_DEBUG_IN);
            if (commandHandler.getIsListening()) {
                client.subscribe(MQTT_TOPIC_IN);
            }
        } else {
            debugPrint("failed, rc=");
            debugPrintln(client.state());
            debugPrint("(MQTT) Next attempt in ");
            debugPrintln(MQTT_TIMEOUT);

            delay(MQTT_TIMEOUT);
        }
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // Convert byte* to const char*
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';

    #if DEBUG
        Serial.printf("(MQTT) [%s] '%s'\n", topic, message);
    #endif

    if (!strcmp(topic, MQTT_TOPIC_IN)) {
        if (strncmp(message, PAYLOAD_CLOSE, length) == 0) {
            doorController.setTargetDoorState(CLOSED);
        } else if (strncmp(message, PAYLOAD_OPEN, length) == 0) {
            doorController.setTargetDoorState(OPEN);
        } else {
            debugPrint("Unknown payload: ");
            debugPrintln(message);
        }
    } else if (!strcmp(topic, MQTT_TOPIC_DEBUG_IN)) {
        commandHandler.handleCommand(message);
    }
}

void onDoorStateChange() {
    // If shadow is enabled, don't send door state to MQTT
    if (commandHandler.getShadowEnabled()) {
        return;
    }

    publishDoorState();
}

void publishDoorState() {
    int state = doorController.getCurrentDoorState();
    if (state == OPEN) {
        client.publish(MQTT_TOPIC_STATE, STATE_OPEN, true);
    } else if (state == CLOSED) {
        client.publish(MQTT_TOPIC_STATE, STATE_CLOSED, true);
    }
}