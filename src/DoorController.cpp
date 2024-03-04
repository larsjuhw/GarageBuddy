/*
@(#)File:            $RCSfile: DoorController.cpp $
@(#)Version:         $Revision: 1.0 $
@(#)Last changed:    $Date: 2024/03/05 $
@(#)Purpose:         ESP8266 garage door controller
@(#)Author:          L. Wolter
*/

#include "DoorController.h"

DoorController::DoorController(void (*onStateChange)()) {
    this->onStateChange = onStateChange;
}

void DoorController::setup() {
    pinMode(PIN_REED, INPUT_PULLUP);
    pinMode(PIN_RELAY, OUTPUT);

    digitalWrite(PIN_RELAY, HIGH);
}

void DoorController::handlePinRead() {
    int reedState = digitalRead(PIN_REED);
    if (reedState != lastFlickerableState) {
        lastDebounceTime = millis();
        lastFlickerableState = reedState;
    } 

    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
        if (reedState != currentDoorState) {
            currentDoorState = reedState;
            if (callbackEnabled) {
                onStateChange();
            }
        }
    }
}

bool DoorController::blinkDoor() {
    if (millis() - lastBlinkTime > BLINK_DELAY) {
        debugPrintln("Blinking door");
        lastBlinkTime = millis();
        digitalWrite(PIN_RELAY, !relayState);
        delay(BLINK_DURATION);
        digitalWrite(PIN_RELAY, relayState);
        return true;
    } else {
        debugPrintln("Too fast");
        return false;
    }
}

void DoorController::toggleRelay() {
    relayState = !relayState;
    digitalWrite(PIN_RELAY, relayState);
}

int DoorController::getRelayState() {
    return relayState;
}

void DoorController::setRelayState(int state) {
    if (state != relayState) {
        toggleRelay();
    }
}

int DoorController::getCurrentDoorState() {
    return currentDoorState;
}

int DoorController::getTargetDoorState() {
    return targetDoorState;
}

void DoorController::setTargetDoorState(int state) {
    targetDoorState = state;
    blinkDoor();
}
