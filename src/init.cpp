#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define pinButton 4
#define pinLed 2
// id pour se connecter au wifi de l'école
#define WLAN_SSID "user"
#define WLAN_PASS "mdp"

#define MQTT_SERVER "io.adafruit.com"
#define MQTT_SERVERPORT 1883

// serveur MQTT
#define MQTT_USERNAME "user"
#define MQTT_KEY "key"

#define USE_I2C

// Configuration MQTT
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_SERVERPORT, MQTT_USERNAME, MQTT_KEY);
// pour publier
Adafruit_MQTT_Publish appui_button = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "/feeds/iot.access_by_button");
// pour recevoir
Adafruit_MQTT_Subscribe authorization = Adafruit_MQTT_Subscribe(&mqtt, MQTT_USERNAME "/feeds/iot.authorization");

int appui = 0;
int currentState;
int previousState = LOW;
unsigned long timeout = 15000;

void MQTT_connect();

void setup() {
    Serial.begin(115200);
    pinMode(pinButton, INPUT);
    pinMode(pinLed, OUTPUT);
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
    // on s'inscrit au topic
}

void loop() {
    MQTT_connect();

    // si button appuyé
    currentState = digitalRead(pinButton);
    if (currentState == HIGH && previousState == LOW) {
        // Envoi
        Serial.print("Demande d'authentification... ");
        if (appui_button.publish(currentState)) {
            Serial.println("Demande envoyé. En attente de réponse ...");
        } else {
            Serial.println("La demande n'a pas pu être transmise.Réessayer...");
        }

        attenteReponse();
    }
    previousState = currentState;
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
    Serial.println("Connecté au serveur MQTT");
    mqtt.subscribe(&authorization);
}

// reponse du subscribe
void attenteReponse() {
    unsigned long start = millis();
    while (millis() - start < timeout) {
        MQTT_connect();
        Adafruit_MQTT_Subscribe *subscription;
        while ((subscription = mqtt.readSubscription(100))) {
            String message = (char *)authorization.lastread;
            Serial.print("Réponse reçue: ");
            Serial.println(message);
            if (message == "true") {
                Serial.println("Connexion acceptée par le serveur ...");
                // allumer led en orange , appel de la fonction pour mot de passe
                digitalWrite(pinLed, HIGH);
                return;
            } else {
                Serial.println("Connexion refusée par le serveur ...");
                return;
            }
        }
    }

    Serial.println("Timeout atteint.");
    // return false;
}
