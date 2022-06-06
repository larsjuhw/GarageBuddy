/*
@(#)File:            $RCSfile: GarageBuddy.cpp $
@(#)Version:         $Revision: 0.1 $
@(#)Last changed:    $Date: 2020/07/27 $
@(#)Purpose:         ESP8266 garage door controller
@(#)Author:          L. Wolter
*/

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <GarageBuddy.h>
#include <PubSubClient.h>
#include <Settings.h>

#define debugPrintln(message)               \
    do {                                    \
        if (DEBUG) Serial.println(message); \
    } while (0)

#define debugPrint(message)               \
    do {                                  \
        if (DEBUG) Serial.print(message); \
    } while (0)

char payloadTargetState[] = PAYLOAD_TARGET;
char payloadCurrentState[] = PAYLOAD_CURRENT;

WiFiClient espClient;
PubSubClient client(espClient);

int readCount = 0;   // handlePinRead() will only validate the new state if
                     // readCount exceeds REED_VALIDATION_COUNT
int reedState = -1;  // Init as -1 so handlePinRead() will update it

bool reportStateEnabled = true;  // state reporting means telling Homebridge
                                 // whether the door is opened or closed

bool listenToHomebridge = true;  // listenToHomebridge decides if the controller
                                 // should subscribe to homebridge's updates

bool otaEnabled = false;
int relayState = HIGH;

unsigned long lastBlinkMillis = 0;
unsigned long readDelayMillis = 0;

void setup() {
    delay(10);

    #if DEBUG
        Serial.begin(115200);
        Serial.println();
        Serial.println("Booting..");
    #endif

    wifiConnect();

    client.setServer(MQTT_IP, 1883);
    client.setCallback(mqttCallback);

    pinMode(PIN_REED, INPUT_PULLUP);
    pinMode(PIN_RELAY, OUTPUT);

    digitalWrite(PIN_RELAY, HIGH);
}

void loop() {
    if (!client.connected()) {
        mqttReconnect();
    }

    handlePinRead();
    client.loop();
    ArduinoOTA.handle();
}

void handlePinRead() {

    int reedStateNew = digitalRead(PIN_REED);

    if (reedStateNew == reedState) {
        readCount = 0;
    } else if ((++readCount >= REED_VALIDATION_COUNT) &&
               ((reedStateNew == CLOSED) || (millis() > readDelayMillis))) {
        reedState = reedStateNew;

        // The payload starts with {"value":x}, where x is 9th in the char array
        payloadTargetState[9] = !reedStateNew + 48;
        payloadCurrentState[9] = !reedStateNew + 48;

        if (reportStateEnabled) {
            client.publish(MQTT_TO_HOMEBRIDGE, payloadTargetState);
            client.publish(MQTT_TO_HOMEBRIDGE, payloadCurrentState);
        }

        debugPrint("(REED) New reed state: garage door ");
        debugPrintln(reedStateNew ? "opened" : "closed");
    }
}

const char* boolToCstr(bool boolValue) {
    return boolValue ? strOn : strOff;
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
        if (client.connect(MQTT_CLIENT)) {
            debugPrintln("connected");
            client.publish(MQTT_TOPIC_OUT, "I am online!");

            client.subscribe(MQTT_TOPIC_IN);
            if (listenToHomebridge) {
                client.subscribe(MQTT_FROM_HOMEBRIDGE);
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
    char command[length + 1];
    memcpy(command, payload, length);
    command[length] = '\0';

    #if DEBUG
        Serial.printf("(MQTT) [%s] '%s'\n", topic, command);
    #endif

    if (!strcmp(topic, MQTT_TOPIC_IN)) {
        if (!handleInput(command)) {
            debugPrint("(CMND) No such command: [");
            debugPrint(command);
            debugPrintln("]");
        }
    } else if (!strcmp(topic, MQTT_FROM_HOMEBRIDGE)) {
        handleHomebridge(command);
    }

}

void relayBlink() {
    // If the previous blink was less than BLINK_MIN_DELAY ago, wait.
    // This is to prevent the relay from being rapidly switched.
    while ((millis() - lastBlinkMillis) < BLINK_MIN_DELAY) {
        delay(100);
    }
    lastBlinkMillis = millis();
    readDelayMillis = lastBlinkMillis + DOOR_OPENING_TIME;

    digitalWrite(PIN_RELAY, !relayState);
    delay(BLINK_DURATION);
    digitalWrite(PIN_RELAY, relayState);
}

void handleHomebridge(char* command) {
    debugPrintln("(MQTT) Handling the homebridge message");
    relayBlink();
}

bool handleInput(char* command) {
    for (int i = 0; i < COMMANDS_AMOUNT; i++) {
        if (!strcmp(command, commandNames[i])) {
            debugPrint("(CMND) Running command: [");
            debugPrint(commandNames[i]);
            debugPrintln("]");
            client.publish(MQTT_TOPIC_OUT, "OK");

            commandFunctions[i]();
            return true;
        }
    }
    client.publish(MQTT_TOPIC_OUT, "invalid");
    return false;
}

void commandOTA() {
    debugPrint("(OTA)  Initializing ArduinoOTA.. ");
    client.publish(MQTT_TOPIC_OUT, "Starting OTA");

    ArduinoOTA.onEnd([]() {
        mqttReconnect();
        client.publish(MQTT_TOPIC_OUT, "Updated complete");
        client.loop();
    });

    ArduinoOTA.begin();
    otaEnabled = true;

    debugPrintln("done");
    client.publish(MQTT_TOPIC_OUT, "OTA ready");
}

void commandStatus() {
    char buffer[100];

    unsigned long uptimeSeconds = millis() / 1000;
    const char* doorState = reedState ? "opened" : "closed";
    const char* otaString = boolToCstr(otaEnabled);
    const char* relayStateString = boolToCstr(!relayState);
    const char* reportStateString = boolToCstr(reportStateEnabled);
    const char* listeningString = boolToCstr(listenToHomebridge);

    sprintf(buffer, "uptime: %lu secs / door: %s / ota: %s / relay: %s / reporting: %s / listening: %s",
            uptimeSeconds, doorState, otaString, relayStateString, reportStateString, listeningString);

    client.publish(MQTT_TOPIC_OUT, buffer);
    debugPrint("(STAT) ");
    debugPrintln(buffer);
}

void commandRestart() {
    const char* restartMessage = "Restarting in 3 seconds";

    client.publish(MQTT_TOPIC_OUT, restartMessage);
    debugPrint("(RST)  ");
    debugPrintln(restartMessage);

    delay(3000);
    ESP.restart();
}

void commandShadow() {
    reportStateEnabled = !reportStateEnabled;

    char buffer[30];
    sprintf(buffer, "Reporting reed state %s", boolToCstr(reportStateEnabled));
    
    debugPrint("(SHDW) ");
    debugPrintln(buffer);
    client.publish(MQTT_TOPIC_OUT, buffer);
}

void commandBlink() {
    char blinkMessage[] = "Blinking the relay (GPIO %d)";
    sprintf(blinkMessage, blinkMessage, PIN_RELAY);
    
    relayBlink();

    debugPrint("(BLNK) ");
    debugPrintln(blinkMessage);
    client.publish(MQTT_TOPIC_OUT, blinkMessage);
}

void commandRelay() {
    relayState = !relayState;
    digitalWrite(PIN_RELAY, relayState);

    char buffer[17];
    sprintf(buffer, "Relay is now %s", boolToCstr(relayState));

    debugPrint("(RLAY) ");
    debugPrintln(buffer);
    client.publish(MQTT_TOPIC_OUT, buffer);
}

void commandToggle() {
    listenToHomebridge = !listenToHomebridge;
    char toggleString[18];

    if (listenToHomebridge) {
        client.subscribe(MQTT_FROM_HOMEBRIDGE);
        strncpy(toggleString, "Subscribed to", 14);
    } else {
        client.unsubscribe(MQTT_FROM_HOMEBRIDGE);
        strncpy(toggleString, "Unsubscribed from", 18);
    }

    char buffer[34];
    sprintf(buffer, "%s homebridge topic", toggleString);

    debugPrint("(TGL)  ");
    debugPrintln(buffer);
    client.publish(MQTT_TOPIC_OUT, buffer);
}

void commandHelp() {
    const char* helpMessage = "[ota/status/restart/shadow/blink/relay/toggle/help]";

    debugPrint("(HELP) ");
    debugPrintln(helpMessage);
    client.publish(MQTT_TOPIC_OUT, helpMessage);
}