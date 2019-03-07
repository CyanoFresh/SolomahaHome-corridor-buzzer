#include <AsyncMqttClient.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>

#define WIFI_SSID "Solomaha"
#define WIFI_PASSWORD "solomakha21"

#define MQTT_HOST IPAddress(192, 168, 1, 76)
#define MQTT_PORT 1883
#define MQTT_ID "corridor-buzzer"
#define MQTT_PASSWORD "bnmdfgu534mbv7yadsu2hj34687tsd"

#define UNLOCK_DELAY 1
#define UNLOCKING_DURATION 1

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

Ticker unlockTimer;
Ticker ringingTimer;

const int answerPin = D1;
const int unlockPin = D2;
//const int buttonPin = D3;

bool isRinging = false;
bool isUnlocking = false;

void unlock(const int stage) {
    if (stage == 0) {
        Serial.println("Unlock called. Answering...");

        isUnlocking = true;

        digitalWrite(answerPin, 1);

        unlockTimer.once(UNLOCK_DELAY, unlock, 1);
    } else if (stage == 1) {
        Serial.println("Pressing unlock button...");

        digitalWrite(unlockPin, 1);

        unlockTimer.once(UNLOCKING_DURATION, unlock, 2);
    } else if (stage == 2) {
        Serial.println("Unlocked. Hanging up...");

        isUnlocking = false;

        digitalWrite(answerPin, 0);
        digitalWrite(unlockPin, 0);

        mqttClient.publish("buzzer/corridor-buzzer/unlocked", 0, false, "true");
    }
}

void ringingCheck() {
    if (isUnlocking) {
        return;
    }

//    Serial.println(analogRead(A0)); // debug

    if (analogRead(A0) > 512) {
        if (!isRinging) {
            Serial.println("Call detected");

            isRinging = true;

            mqttClient.publish("buzzer/corridor-buzzer/ringing", 0, false, "true");
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
    Serial.println("Connecting to Wi-Fi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt() {
    Serial.println("Connecting to MQTT...");
    mqttClient.connect();
}

void onWifiConnect(const WiFiEventStationModeGotIP &event) {
    Serial.println("Connected to Wi-Fi.");
    connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event) {
    Serial.println("Disconnected from Wi-Fi.");
    mqttReconnectTimer.detach();
    wifiReconnectTimer.once(2, connectToWifi);
}

void onMqttConnect(bool) {
    Serial.println("Connected to MQTT.");

    // Subscribe to topics:
    mqttClient.subscribe("buzzer/corridor-buzzer/unlock", 0);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
    Serial.print("Disconnected from MQTT. Reason: ");
    Serial.println((int) reason);

    if (WiFi.isConnected()) {
        mqttReconnectTimer.once(2, connectToMqtt);
    }
}

void onMqttMessage(char *topic, char *payload,
                   AsyncMqttClientMessageProperties properties, size_t len,
                   size_t index, size_t total) {
    if (strcmp(topic, "buzzer/corridor-buzzer/unlock") == 0) {
        unlock(0);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println();

    wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
    wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

    pinMode(answerPin, OUTPUT);
    pinMode(unlockPin, OUTPUT);

    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.onMessage(onMqttMessage);
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.setClientId(MQTT_ID);
    mqttClient.setCredentials("device", MQTT_PASSWORD);

    ringingTimer.attach_ms(100, ringingCheck);

    connectToWifi();
}

void loop() {}