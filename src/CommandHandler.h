/*
@(#)File:            $RCSfile: CommandHandler.h $
@(#)Version:         $Revision: 1.0 $
@(#)Last changed:    $Date: 2024/03/05 $
@(#)Purpose:         ESP8266 garage door controller
@(#)Author:          L. Wolter
*/

#pragma once

#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <PubSubClient.h>

#include <functional>
#include <map>

#include "DoorController.h"
#include "GarageBuddy.h"

struct CStringComparator {
    bool operator()(const char* a, const char* b) const {
        return strcmp(a, b) < 0;
    }
};

class CommandHandler {
public:
    CommandHandler(PubSubClient& client, DoorController& doorController);
    void setup();
    void handleCommand(const char* command);
    bool getShadowEnabled();
    bool getIsListening();
    void publishStates();

private:
    PubSubClient& client;
    DoorController& doorController;

    std::map<const char*, std::function<void()>, CStringComparator> commandMap;
    bool otaEnabled = false;
    bool isListening = true;
    bool shadowEnabled = false;
    void handleOta();
    void handleRestart();
    void handleStatus();
    void handleDoor();
    void handleToggle();
    void handleRelay();
    void handleShadow();
    void handleIP();
    void handleSave();
    void handleHelp();
    void publishShadow();
    void publishRelay();
    void publishListening();
    void publishUptime();
};