/*
@(#)File:            $RCSfile: DoorController.h $
@(#)Version:         $Revision: 1.0 $
@(#)Last changed:    $Date: 2024/03/05 $
@(#)Purpose:         ESP8266 garage door controller
@(#)Author:          L. Wolter
*/

#pragma once

#include <Arduino.h>
#include <GarageBuddy.h>

class DoorController {
public:
    DoorController(void (*onStateChange)());
    void setup();
    void handlePinRead();
    bool blinkDoor();
    void toggleRelay();
    int getRelayState();
    void setRelayState(int state);
    int getCurrentDoorState();
    int getTargetDoorState();
    void setTargetDoorState(int state);
private:
    int targetDoorState = CLOSED;
    int currentDoorState = -1;
    int relayState = HIGH;
    int lastFlickerableState = LOW;
    unsigned long lastDebounceTime = 0;
    unsigned long lastBlinkTime = 0;
    bool callbackEnabled = true;
    void (*onStateChange)();
};