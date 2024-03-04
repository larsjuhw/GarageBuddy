/*
@(#)File:            $RCSfile: CommandHandler.cpp $
@(#)Version:         $Revision: 1.0 $
@(#)Last changed:    $Date: 2024/03/05 $
@(#)Purpose:         ESP8266 garage door controller
@(#)Author:          L. Wolter
*/

#include "CommandHandler.h"

CommandHandler::CommandHandler(PubSubClient& client, DoorController& doorController) : client(client), doorController(doorController) {
    // Initialize the map with string-to-handler-function mappings
    commandMap["ota"] = std::bind(&CommandHandler::handleOta, this);
    commandMap["restart"] = std::bind(&CommandHandler::handleRestart, this);
    commandMap["status"] = std::bind(&CommandHandler::handleStatus, this);
    commandMap["door"] = std::bind(&CommandHandler::handleDoor, this);
    commandMap["toggle"] = std::bind(&CommandHandler::handleToggle, this);
    commandMap["relay"] = std::bind(&CommandHandler::handleRelay, this);
    commandMap["shadow"] = std::bind(&CommandHandler::handleShadow, this);
    commandMap["ip"] = std::bind(&CommandHandler::handleIP, this);
    commandMap["save"] = std::bind(&CommandHandler::handleSave, this);
    commandMap["help"] = std::bind(&CommandHandler::handleHelp, this);
}

void CommandHandler::setup() {
    // Load settings from EEPROM if available
    debugPrintln("Loading settings from EEPROM");
    // Read the JSON string from the EEPROM
    EEPROM.begin(80);
    char json[80];
    for (int i = 0; i < sizeof(json); i++) {
        json[i] = EEPROM.read(i);
    }
#if DEBUG
    debugPrintln(json);
#endif

    // Parse the JSON string
    StaticJsonDocument<80> doc;
    DeserializationError error = deserializeJson(doc, json);

    // Check for errors in parsing
    if (error) {
        debugPrint("Failed to parse EEPROM settings with error: ");
        debugPrintln(error.c_str());
        return;
    }

    // Extract the settings from the JSON object
    shadowEnabled = doc["shadowEnabled"];
    isListening = doc["isListening"];
    doorController.setRelayState(doc["relayState"]);

    debugPrint("Settings loaded. Shadow: ");
    debugPrint(shadowEnabled);
    debugPrint(", Subscribed: ");
    debugPrint(isListening);
    debugPrint(", Relay: ");
    debugPrintln(doorController.getRelayState());
}

bool CommandHandler::getShadowEnabled() {
    return shadowEnabled;
}

bool CommandHandler::getIsListening() {
    return isListening;
}

void CommandHandler::handleCommand(const char* command) {
    // Find the handler function for the given command and call it
    auto handler = commandMap.find(command);
    if (handler != commandMap.end()) {
        handler->second();
    } else {
        client.publish(MQTT_TOPIC_DEBUG_OUT, "Unknown command");
    }
}

void CommandHandler::handleOta() {
    ArduinoOTA.onEnd([this]() {
        mqttReconnect();
        this->client.publish(MQTT_TOPIC_DEBUG_OUT, "OTA finished");
        this->client.loop();
    });

    ArduinoOTA.begin();
    otaEnabled = true;
    client.publish(MQTT_TOPIC_DEBUG_OUT, "OTA enabled");
}

void CommandHandler::handleRestart() {
    client.publish(MQTT_TOPIC_DEBUG_OUT, "Restarting now...");
    client.loop();
    delay(200);
    ESP.restart();
}

void CommandHandler::handleStatus() {
    char buffer[125];

    unsigned long uptimeMillis = millis();
    unsigned int uptime;
    const char* time;

    if (uptimeMillis < 1000000) { // Display seconds if uptime is less than 1000 seconds
        time = "seconds";
        uptime = uptimeMillis / 1000;
    } else if (uptimeMillis < 7200000) { // Display minutes if uptime is less than 120 minutes
        time = "minutes";
        uptime = uptimeMillis / 60000;
    } else if (uptimeMillis < 360000000) { // Display hours if uptime is less than 100 hours
        time = "hours";
        uptime = uptimeMillis / 3600000;
    } else { // Display days if uptime is more than 100 hours
        time = "days";
        uptime = uptimeMillis / 86400000;
    }

    const char* doorState = digitalRead(PIN_REED) == OPEN ? "open" : "closed";
    const char* otaState = otaEnabled ? "enabled" : "disabled";
    const char* relayState = doorController.getRelayState() ? "off" : "on";
    const char* listeningState = isListening ? "enabled" : "disabled";
    const char* shadowState = shadowEnabled ? "enabled" : "disabled";

    snprintf(buffer, sizeof(buffer), "Uptime: %d %s / door: %s / relay: %s / shadow: %s / listening: %s / ota: %s", uptime, time, doorState, relayState, shadowState, listeningState, otaState);
    
    client.publish(MQTT_TOPIC_DEBUG_OUT, buffer);
}

void CommandHandler::handleDoor() {
    bool blinked = doorController.blinkDoor();
    if (blinked) {
        client.publish(MQTT_TOPIC_DEBUG_OUT, "Blinking door");
    } else {
        client.publish(MQTT_TOPIC_DEBUG_OUT, "Too fast");
    }
}

void CommandHandler::handleToggle() {
    if (isListening) {
        if (client.unsubscribe(MQTT_TOPIC_IN)) {
            isListening = false;
            client.publish(MQTT_TOPIC_DEBUG_OUT, "Unsubscribed from " MQTT_TOPIC_IN);
        }
    } else {
        if (client.subscribe(MQTT_TOPIC_IN)) {
            isListening = true;
            client.publish(MQTT_TOPIC_DEBUG_OUT, "Subscribed to " MQTT_TOPIC_IN);
        }
    }
    publishListening();
}

void CommandHandler::handleRelay() {
    doorController.toggleRelay();
    int relayState = doorController.getRelayState();
    if (relayState) {
        client.publish(MQTT_TOPIC_DEBUG_OUT, "Relay on");
    } else {
        client.publish(MQTT_TOPIC_DEBUG_OUT, "Relay off");
    }
    publishRelay();
}

void CommandHandler::handleShadow() {
    if (shadowEnabled) {
        shadowEnabled = false;
        client.publish(MQTT_TOPIC_DEBUG_OUT, "Shadow disabled");
    } else {
        shadowEnabled = true;
        client.publish(MQTT_TOPIC_DEBUG_OUT, "Shadow enabled");
    }
    publishShadow();
}

void CommandHandler::handleIP() {
    // Reply with the IP address
    IPAddress local = WiFi.localIP();
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%d.%d.%d.%d", local[0], local[1], local[2], local[3]);
    client.publish(MQTT_TOPIC_DEBUG_OUT, buffer);
}

void CommandHandler::handleSave() {
    debugPrintln("Saving settings to EEPROM");
    client.publish(MQTT_TOPIC_DEBUG_OUT, "Saving settings");

    // Create a JSON object with the settings
    StaticJsonDocument<80> doc;
    doc["shadowEnabled"] = shadowEnabled;
    doc["isListening"] = isListening;
    doc["relayState"] = doorController.getRelayState();
    // Add more settings as needed

    // Convert the JSON object to a string
    char json[80];
    serializeJson(doc, json);

    debugPrintln(json);

    // Write the JSON string to the EEPROM
    EEPROM.begin(sizeof(json));
    for (int i = 0; i < sizeof(json); i++) {
        EEPROM.write(i, json[i]);
    }

    // Make sure to commit the changes to the EEPROM
    EEPROM.commit();

    // Calculate the amount of free space on EEPROM
    int freeSpace = EEPROM.length() - sizeof(json);

    // Publish the message with the amount of free space
    char message[50];
    snprintf(message, sizeof(message), "Settings saved. Free space: %d bytes", freeSpace);
    debugPrintln(message);
    client.publish(MQTT_TOPIC_DEBUG_OUT, message);

#if DEBUG
    // Debug print the first 200 bytes of the EEPROM
    debugPrintln("EEPROM content:");
    for (int i = 0; i < sizeof(json); i++) {
        char byte = EEPROM.read(i);
        // Stop on null terminator
        if (byte == '\0') {
            break;
        }
        debugPrint(byte);
    }
#endif
}

void CommandHandler::handleHelp() {
    client.publish(MQTT_TOPIC_DEBUG_OUT, "[ota, restart, status, door, toggle, relay, shadow, save, help]");
}

void CommandHandler::publishStates() {
    publishShadow();
    publishRelay();
    publishListening();
    publishUptime();
}

void CommandHandler::publishShadow() {
    if (shadowEnabled) {
        client.publish(MQTT_TOPIC_SHADOW, PAYLOAD_ON);
    } else {
        client.publish(MQTT_TOPIC_SHADOW, PAYLOAD_OFF);
    }
}

void CommandHandler::publishRelay() {
    if (doorController.getRelayState()) {
        client.publish(MQTT_TOPIC_RELAY, PAYLOAD_OFF);
    } else {
        client.publish(MQTT_TOPIC_RELAY, PAYLOAD_ON);
    }
}

void CommandHandler::publishListening() {
    if (isListening) {
        client.publish(MQTT_TOPIC_LISTENING, PAYLOAD_ON);
    } else {
        client.publish(MQTT_TOPIC_LISTENING, PAYLOAD_OFF);
    }
}

void CommandHandler::publishUptime() {
    unsigned long uptime = millis();
    char buffer[11];
    snprintf(buffer, sizeof(buffer), "%lu", uptime);
    client.publish(MQTT_TOPIC_UPTIME, buffer);
}
