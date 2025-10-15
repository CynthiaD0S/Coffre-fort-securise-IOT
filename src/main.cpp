#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>

#include "Adafruit_BME680.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#define WLAN_SSID "Meteor-29EB4F93"
#define WLAN_PASS "LhvCRc765sTFHn5K"
#define MQTT_SERVER "io.adafruit.com"
#define MQTT_SERVERPORT 1883
#define MQTT_USERNAME "cynthiadossa"
#define MQTT_KEY "aio_BQZZ33SKnIu091tTYyht3x056Ycf"
Adafruit_BME680 bme;
#define USE_I2C
// Configuration MQTT
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_SERVERPORT, MQTT_USERNAME, MQTT_KEY);
Adafruit_MQTT_Publish temperature = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME
                                                          "/feeds/temperature");
Adafruit_MQTT_Publish humidity = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME
                                                       "/feeds/humidity");

void MQTT_connect();

void setup() {
    Serial.begin(115200);
    if (!bme.begin()) {
        Serial.println("BME680 non détecté!");
        while (1);
    }
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    // connection wifi
    Serial.print("Connecting to ");
    Serial.println(WLAN_SSID);
    WiFi.begin(WLAN_SSID, WLAN_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void loop() {
    MQTT_connect();

    if (!bme.performReading()) {
        Serial.println("ERREUR: Échec lecture BME680");
        return;
    }

    float temp = bme.temperature;
    float humid = bme.humidity;

    // Envoi de la température
    Serial.print("Envoi température... ");
    if (temperature.publish(bme.temperature)) {
        Serial.println("OK");
    } else {
        Serial.println("FAILED");
    }

    // Envoi de l'humidité
    Serial.print("Envoi humidité... ");
    if (humidity.publish(bme.humidity)) {
        Serial.println("OK");
    } else {
        Serial.println("FAILED");
    }

    Serial.println("");
    delay(5000);
}
void MQTT_connect() {
    if (mqtt.connected()) return;
    Serial.print("Connexion MQTT... ");
    uint8_t retries = 3;
    int8_t ret;
    while ((ret = mqtt.connect()) != 0) {
        Serial.println(mqtt.connectErrorString(ret));
        Serial.println("Nouvelle tentative dans 5s...");
        mqtt.disconnect();
        delay(5000);
        retries--;
        if (retries == 0) {
            Serial.println("Redémarrage...");
            ESP.restart();
        }
    }
    Serial.println("Connecté!");
}