#include <AsyncMqttClient.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include "config.h"

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

Ticker unlockTimer;
Ticker ringingTimer;

bool isRinging = false;
bool isUnlocking = false;
bool isAutoUnlock = false;

enum unlockStage {
    ANSWER,
    UNLOCK,
    HANG_UP,
};

void unlock(unlockStage stage = ANSWER) {
    switch (stage) {
        case ANSWER:
            Serial.println("Answering...");

            isUnlocking = true;

            digitalWrite(config::ANSWER_PIN, 1);

            unlockTimer.once(config::UNLOCK_DELAY, unlock, UNLOCK);

            break;
        case UNLOCK:
            Serial.println("Unlocking...");

            digitalWrite(config::UNLOCK_PIN, 1);

            unlockTimer.once(config::UNLOCKING_DURATION, unlock, HANG_UP);

            break;
        case HANG_UP:
            Serial.println("Hanging up...");

            isUnlocking = false;

            digitalWrite(config::ANSWER_PIN, 1);
            digitalWrite(config::ANSWER_PIN, 0);
            digitalWrite(config::UNLOCK_PIN, 0);

            mqttClient.publish("buzzer/corridor-buzzer/unlocked", 0, false, isAutoUnlock ? "true" : "false");

            isAutoUnlock = false;

            break;
    }
}

unsigned long lastLineUpTime;

void ringingCheck() {
    if (isUnlocking) {
        return;
    }

    bool isLineUp = analogRead(A0) > 512;

    if (isLineUp) {
        if (!isRinging) {
            Serial.println("Call detected");

            isRinging = true;
            lastLineUpTime = millis();

            mqttClient.publish("buzzer/corridor-buzzer/ringing", 0, false, "true");

            if (isAutoUnlock) {
                Serial.println("Auto-unlocking...");
                unlockTimer.once(config::AUTO_UNLOCK_DELAY, unlock, ANSWER);
            }
        }
    } else {
        if (isRinging) {
            Serial.println("Call end detected");

            isRinging = false;

            mqttClient.publish("buzzer/corridor-buzzer/ringing", 0, false, "false");
        }
    }
}

void connectToWifi() {
    Serial.println(F("Connecting to Wi-Fi..."));
    WiFi.begin(config::WIFI_SSID, config::WIFI_PASSWORD);
}

void connectToMqtt() {
    Serial.println(F("Connecting to MQTT..."));
    mqttClient.connect();
}

void onWifiConnect(const WiFiEventStationModeGotIP &) {
    connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event) {
    Serial.print(F("Disconnected from Wi-Fi: "));
    Serial.println(event.reason);
    digitalWrite(LED_BUILTIN, LOW);

    mqttReconnectTimer.detach();
    wifiReconnectTimer.once(2, connectToWifi);
}

void onMqttConnect(bool) {
    Serial.println(F("Connected to MQTT."));
    digitalWrite(LED_BUILTIN, HIGH);

    // Subscribe to topics:
    mqttClient.subscribe("buzzer/corridor-buzzer/unlock", 0);
    mqttClient.subscribe("buzzer/corridor-buzzer/auto_unlock/set", 0);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
    Serial.print(F("Disconnected from MQTT. Reason: "));
    Serial.println((int) reason);

    digitalWrite(LED_BUILTIN, LOW);

    if (WiFi.isConnected()) {
        mqttReconnectTimer.once(2, connectToMqtt);
    }
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties, size_t, size_t, size_t) {
    if (strcmp(topic, "buzzer/corridor-buzzer/unlock") == 0) {
        if (strncmp(payload, "true", 4) == 0) {
            unlock();
        } else {
            unlock(HANG_UP);
        }
    } else if (strcmp(topic, "buzzer/corridor-buzzer/auto_unlock/set") == 0) {
        isAutoUnlock = strncmp(payload, "true", 4) == 0;

        Serial.print("Auto-unlock is now ");
        Serial.println(isAutoUnlock ? "enabled" : "disabled");

        mqttClient.publish("buzzer/corridor-buzzer/auto_unlock", 0, false, isAutoUnlock ? "true" : "false");
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println();

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(config::ANSWER_PIN, OUTPUT);
    pinMode(config::UNLOCK_PIN, OUTPUT);
    pinMode(config::BTN_PIN, INPUT_PULLUP);

    digitalWrite(LED_BUILTIN, LOW);

    wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
    wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.onMessage(onMqttMessage);
    mqttClient.setServer(config::MQTT_HOST, config::MQTT_PORT);
    mqttClient.setClientId(config::MQTT_ID);
    mqttClient.setCredentials("device", config::MQTT_PASSWORD);

    ringingTimer.attach_ms(100, ringingCheck);

    connectToWifi();
}

uint8_t lastBtnState = HIGH;

void loop() {
    uint8_t btnState = digitalRead(config::BTN_PIN);

    if (btnState != lastBtnState) {
        if (btnState == LOW) {
            Serial.println("Unlock btn pushed. Unlocking...");
            unlock();
        }

        lastBtnState = btnState;
    }
}