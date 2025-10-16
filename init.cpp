#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <Wire.h>

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
// Initialisation de l'écran LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);
#define USE_I2C

#define WLAN_SSID "user"
#define WLAN_PASS "mdp"

#define AIO_SERVER "io.adafruit.com"
#define AIO_SERVERPORT 1883

#define AIO_USERNAME "user"
#define AIO_KEY "key"

#define PIN_BUTTON 4
#define PIN_LED 2

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Feed pour publier la demande
Adafruit_MQTT_Publish buttonFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/iot.access-by-button");

// Feed pour recevoir l'autorisation
Adafruit_MQTT_Subscribe authorizationFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/iot.authorization");

// clavier
const int ROW_NUM = 4;
const int COLUMN_NUM = 4;

char keys[ROW_NUM][COLUMN_NUM] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

byte pin_rows[ROW_NUM] = {9, 8, 7, 6};
byte pin_column[COLUMN_NUM] = {5, 4, 3, 2};
Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

int previousState = LOW;

void authCallback(char *data, uint16_t len) {
    String message = String(data);
    message.trim();
    if (message == "true") {
        Serial.println("Accès autorisé...");
        lcd.setCursor(0, 0);
        lcd.print("Accès autorisé");
        lcd.setCursor(0, 1);
        lcd.print("Entrer le code pin...");
        digitalWrite(PIN_LED, HIGH);
    } else if (message == "false") {
        Serial.println("Accès refusé...");
        lcd.setCursor(0, 0);
        lcd.print("Accès refusé");
        lcd.setCursor(0, 1);
        lcd.print("Réessayer...");
        digitalWrite(PIN_LED, LOW);
    } else {
        Serial.println("Erreur du serveur.... ");
        lcd.setCursor(0, 0);
        lcd.print("Erreur du serveur");
        lcd.setCursor(0, 1);
        lcd.print("Réessayer...");
    }
}

void MQTT_connect() {
    int8_t ret;
    if (mqtt.connected()) return;

    Serial.print("Connexion MQTT... ");
    uint8_t retries = 3;
    while ((ret = mqtt.connect()) != 0) {
        Serial.println(mqtt.connectErrorString(ret));
        Serial.println("Nouvelle tentative dans 5s...");
        mqtt.disconnect();
        delay(5000);
        retries--;
        if (retries == 0) {
            Serial.println("Echec, redémarrage...");
            ESP.restart();
        }
    }

    Serial.println("Connecté au MQTT");
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_BUTTON, INPUT);
    pinMode(PIN_LED, OUTPUT);

    Serial.print("Connexion WiFi à ");
    Serial.println(WLAN_SSID);
    WiFi.begin(WLAN_SSID, WLAN_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi connecté !");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    // Callback pour le topic d'autorisation
    authorizationFeed.setCallback(authCallback);
    mqtt.subscribe(&authorizationFeed);

    // lcd
    lcd.init();
    lcd.backlight();
}

void loop() {
    MQTT_connect();

    // Traitement des messages entrants
    mqtt.processPackets(1000);
    mqtt.ping();

    // Lecture bouton
    int currentState = digitalRead(PIN_BUTTON);
    if (currentState == HIGH && previousState == LOW) {
        Serial.println("Envoi de la demande d'authentification...");
        if (buttonFeed.publish("1")) {
            lcd.setCursor(0, 0);
            lcd.print("Attente reponse serveur...");
            Serial.println("Demande envoyée");
        } else {
            Serial.println("Erreur d'envoi");
        }
    }
    previousState = currentState;
}

String saisirCode() {
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Entrez code :");

    String code = "";
    while (true) {
        char key = keypad.getKey();
        if (key) {
            if (key == '#') {  // validation
                break;
            } else if (key == '*') {  // annulation
                code = "";
                lcd.clear();
                lcd.print("Code efface");
                delay(500);
                lcd.clear();
                lcd.print("Entrez code :");
            } else if (code.length() < 4) {
                code += key;
                lcd.setCursor(code.length(), 1);
                lcd.print("*");  // on masque la saisie
            }
        }
    }

    lcd.clear();
    lcd.print("Code: ");
    lcd.print(code);
    delay(1000);
    return code;
}