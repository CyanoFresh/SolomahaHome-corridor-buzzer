#ifndef CORRIDOR_BUZZER_CONFIG_H
#define CORRIDOR_BUZZER_CONFIG_H

#include <Arduino.h>

namespace config {
    const char WIFI_SSID[] = "Solomaha";
    const char WIFI_PASSWORD[] = "solomakha21";

    const auto MQTT_HOST = IPAddress(192, 168, 1, 230);
    const uint16_t MQTT_PORT = 1883;
    const char MQTT_ID[] = "corridor-buzzer";
    const char MQTT_PASSWORD[] = "bnmdfgu534mbv7yadsu2hj34687tsd";

    const uint8_t ANSWER_PIN = D1;
    const uint8_t UNLOCK_PIN = D2;
    const uint8_t BTN_PIN = D3;

    const uint8_t UNLOCK_DELAY = 1;     // TODO: can be lowered?
    const float AUTO_UNLOCK_DELAY = 0.8;
    const uint8_t UNLOCKING_DURATION = 1;
}

#endif
