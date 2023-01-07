/*
@(#)File:            $RCSfile: GarageBuddy.cpp $
@(#)Version:         $Revision: 0.2 $
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
int lastReedState = -1;
int reedState = -1;  // Init as -1 so handlePinRead() will update it

bool reportStateEnabled = true;  // state reporting means telling Homebridge
                                 // whether the door is opened or closed

bool listenToHomebridge = true;  // listenToHomebridge decides if the controller
                                 // should subscribe to homebridge's updates

bool otaEnabled = false;
bool msWarningEnabled = true;
int relayState = HIGH;

unsigned long lastBlinkMillis = 0;
unsigned long readDelayMillis = 0;
unsigned long lastDoorChangeMillis = 0;

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
    unsigned long currentMillis = millis();

    if (lastReedState != reedStateNew) {
        readCount = 0;
    } else {
        readCount++;
    }

    lastReedState = reedStateNew;

    if ((++readCount >= REED_VALIDATION_COUNT) &&
               ((reedStateNew == CLOSED) || (currentMillis > readDelayMillis))) {
        // At this point, the new reed state is validated to be consistent.
        if (reedState != reedStateNew) {
            debugPrint("(REED) New reed state: garage door ");
            debugPrintln(reedStateNew ? "opened" : "closed");
            lastDoorChangeMillis = currentMillis;
            reedState = reedStateNew;
            publish_current_state();
        } else {
            if ( msWarningEnabled && reedState == OPENED &&
                (currentMillis - lastDoorChangeMillis >= (LEFT_OPEN_TIME * 1000))) {
                lastDoorChangeMillis = currentMillis;
                debugPrintln("Garage door left open.. blipping sensor state");
                blip_sensor_state();
            }
        }
    }
}

void publish_current_state() {
    // The payload starts with {"value":x}, where x is 9th in the char array
    payloadTargetState[9] = !reedState + 48;
    payloadCurrentState[9] = !reedState + 48;

    if (reportStateEnabled) {
        client.publish(MQTT_TO_HOMEBRIDGE, payloadTargetState);
        delay(50);
        client.publish(MQTT_TO_HOMEBRIDGE, payloadCurrentState);
    }
}

void blip_sensor_state() {
    client.publish(MQTT_TO_HOMEBRIDGE, PAYLOAD_SENSOR_DETECTED);
    delay(200);
    client.publish(MQTT_TO_HOMEBRIDGE, PAYLOAD_SENSOR_UNDETECTED);
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
        if (client.connect(MQTT_CLIENT, MQTT_USERNAME, MQTT_PASSWORD)) {
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
        client.publish(MQTT_TOPIC_OUT, "Update complete");
        client.loop();
    });

    ArduinoOTA.begin();
    otaEnabled = true;

    debugPrintln("done");
    client.publish(MQTT_TOPIC_OUT, "OTA ready");
}

void commandStatus() {
    char buffer[125];

    unsigned long uptimeSeconds = millis() / 1000;
    const char* doorState = reedState ? "opened" : "closed";
    const char* otaString = boolToCstr(otaEnabled);
    const char* relayStateString = boolToCstr(!relayState);
    const char* reportStateString = boolToCstr(reportStateEnabled);
    const char* listeningString = boolToCstr(listenToHomebridge);
    const char* msWarningEnabledString = boolToCstr(msWarningEnabled);

    sprintf(buffer, "uptime: %lu secs / door: %s / ota: %s / relay: %s / reporting: %s / listening: %s / msenabled: %s",
            uptimeSeconds, doorState, otaString, relayStateString, reportStateString, listeningString, msWarningEnabledString);

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

    char buffer[35];
    sprintf(buffer, "%s homebridge topic", toggleString);

    debugPrint("(TGL)  ");
    debugPrintln(buffer);
    client.publish(MQTT_TOPIC_OUT, buffer);
}

void commandMsToggle() {
    msWarningEnabled = !msWarningEnabled;
    if (msWarningEnabled) {
        client.publish(MQTT_TOPIC_OUT, "Motion sensor warnings enabled");
        debugPrintln("(TGL) Motion sensor warnings enabled");
    } else {
        client.publish(MQTT_TOPIC_OUT, "Motion sensor warnings disabled");
        debugPrintln("(TGL) Motion sensor warnings disabled");
    }
}

void commandMsTest() {
    blip_sensor_state();
}

void commandHelp() {
    const char* helpMessage = "[ota/status/restart/shadow/blink/relay/toggle/mstoggle/mstest/help]";

    debugPrint("(HELP) ");
    debugPrintln(helpMessage);
    client.publish(MQTT_TOPIC_OUT, helpMessage);
}