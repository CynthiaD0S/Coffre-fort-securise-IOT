#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <Wire.h>

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Wifi
#define WLAN_SSID "user"
#define WLAN_PASS "mdp"

// Adafruit
#define AIO_SERVER "io.adafruit.com"
#define AIO_SERVERPORT 1883
#define AIO_USERNAME "user"
#define AIO_KEY "mdp"

// boutton
#define PIN_BUTTON 4

// led rgb
const int PIN_RED = 19;
const int PIN_GREEN = 13;
const int PIN_BLUE = 23;

// Clavier
const int ROW_NUM = 4;
const int COLUMN_NUM = 4;

char keys[ROW_NUM][COLUMN_NUM] = {
    {'1', '2', '3', 'F'},
    {'4', '5', '6', 'E'},
    {'7', '8', '9', 'D'},
    {'A', '0', 'B', 'C'}};

byte pin_column[ROW_NUM] = {25, 26, 27, 14};
byte pin_rows[COLUMN_NUM] = {32, 33, 5, 18};
Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

// machine à états
enum State {
    STATE_IDLE,                 // état initial en attente d'authentification
    STATE_WAITING_BUTTON_AUTH,  // attente validation par le serveur
    STATE_ENTERING_CODE,        // attente code de l'user
    STATE_WAITING_CODE_AUTH,    // attente validation du code
    STATE_ACCESS_GRANTED,       // accès autorisé
    STATE_ACCESS_DENIED,        // Accès refusé
    STATE_ERROR                 // Erreur
};

State currentState = STATE_IDLE;
State previousState = STATE_IDLE;

// MQTT
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Publish buttonFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/iot.access-by-button");
Adafruit_MQTT_Publish codeFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/iot.access-by-code");

Adafruit_MQTT_Subscribe authorizationFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/iot.authorization");
Adafruit_MQTT_Subscribe authorizationCodeFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/iot.authorization-code");

int previousButtonState = LOW;
String currentCode = "";
String inputCode = "";

unsigned long stateStartTime = 0;
const unsigned long TIMEOUT_DURATION = 30000;
const unsigned long DISPLAY_DURATION = 5000;

// allumer la led
void setLED(int r, int g, int b) {
    analogWrite(PIN_RED, r);
    analogWrite(PIN_GREEN, g);
    analogWrite(PIN_BLUE, b);
}

void ledOff() {
    setLED(0, 0, 0);
}
void ledWaiting() {
    setLED(204, 102, 0);  // orange
}
void ledAuthorize() {
    setLED(0, 255, 0);  // vert
}
void ledDenied() {
    setLED(255, 0, 0);  // rouge
}
void ledError() {
    setLED(0, 0, 255);  // bleu
}

void changeState(State newState) {
    previousState = currentState;
    currentState = newState;
    stateStartTime = millis();
}

// CALLBACKS MQTT

void authButtonCallback(char* data, uint16_t len) {
    if (currentState != STATE_WAITING_BUTTON_AUTH) {
        Serial.println("Réponse bouton reçue hors contexte");
        return;
    }

    String message = String(data);
    message.trim();
    Serial.print("Réponse du serveur boutton: ");
    Serial.println(message);

    if (message == "true") {
        changeState(STATE_ENTERING_CODE);
    } else if (message == "false") {
        changeState(STATE_ACCESS_DENIED);
    } else {
        changeState(STATE_ERROR);
    }
}

void authCodeCallback(char* data, uint16_t len) {
    if (currentState != STATE_WAITING_CODE_AUTH) {
        Serial.println("Réponse code reçue hors contexte");
        return;
    }

    String message = String(data);
    message.trim();
    Serial.print("Réponse serveur code: ");
    Serial.println(message);

    if (message == "true") {
        changeState(STATE_ACCESS_GRANTED);
    } else if (message == "false") {
        changeState(STATE_ACCESS_DENIED);
    } else {
        changeState(STATE_ERROR);
    }
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
            Serial.println("Echec MQTT, redémarrage...");
            ESP.restart();
        }
    }
    Serial.println("Connecté au MQTT");
}

// IDLE
void stateIdle() {
    ledOff();
    lcd.clear();
    // Détecter appui bouton
    int currentButtonState = digitalRead(PIN_BUTTON);

    if (currentButtonState == HIGH && previousButtonState == LOW) {
        Serial.println("Bouton pressé");
        lcd.clear();
        lcd.setCursor(0, 0);
        if (buttonFeed.publish("1")) {
            Serial.println("Demande envoyée");
            lcd.print("Attente reponse...");
            ledWaiting();
            changeState(STATE_WAITING_BUTTON_AUTH);
        } else {
            Serial.println("Erreur envoi");
            lcd.print("Erreur d'envoie");
            changeState(STATE_ERROR);
        }
    }
    previousButtonState = currentButtonState;
}

// WAITING_BUTTON_AUTH
void stateWaitingButtonAuth() {
    if ((millis() - stateStartTime) > TIMEOUT_DURATION) {
        Serial.println("Timeout attente réponse bouton");
        changeState(STATE_ERROR);
    }
}

// state saisie code
void stateEnterCode() {
    if (previousState != STATE_ENTERING_CODE) {
        inputCode = "";
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Acces autorise");
        lcd.setCursor(0, 1);
        lcd.print("Entrez code");
        delay(DISPLAY_DURATION);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Code:");
        lcd.setCursor(0, 1);
        lcd.print("C=OK E=Effacer");
        previousState = STATE_ENTERING_CODE;
    }

    char key = keypad.getKey();

    if (key) {
        if (key == 'C') {  // Validation
            if (inputCode.length() == 4) {
                currentCode = inputCode;
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Envoi code...");

                if (codeFeed.publish(currentCode.c_str())) {
                    Serial.print("Code envoyé: ");
                    Serial.println(currentCode);
                    lcd.setCursor(0, 1);
                    lcd.print("Attente verif.");
                    ledWaiting();
                    inputCode = "";  // Vider le code après envoi
                    changeState(STATE_WAITING_CODE_AUTH);
                } else {
                    Serial.println("Erreur envoi code");
                    inputCode = "";  // Vider le code même si erreur
                    changeState(STATE_ERROR);
                }
            } else {
                // Afficher un message d'erreur court
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Code: ");
                for (int i = 0; i < inputCode.length(); i++) {
                    lcd.print("*");
                }
                lcd.setCursor(0, 1);
                lcd.print("Code de 4 chiffres");
                delay(2000);

                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Code:");
                lcd.setCursor(5, 0);
                for (int i = 0; i < inputCode.length(); i++) {
                    lcd.print("*");
                }
                lcd.setCursor(0, 1);
                lcd.print("C=OK E=Effacer");
            }

        } else if (key == 'E') {  // Effacer
            inputCode = "";
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Code efface:");
            lcd.setCursor(0, 1);
            lcd.print("C=OK E=Effacer");
            delay(1000);

            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Code:");
            lcd.setCursor(0, 1);
            lcd.print("C=OK E=Effacer");

        } else if ((key >= '0' && key <= '9') && (inputCode.length() < 4)) {
            inputCode += key;
            lcd.setCursor(5, 0);

            for (int i = 0; i < inputCode.length(); i++) {
                lcd.print("*");
            }
        }
    }

    if ((millis() - stateStartTime) > TIMEOUT_DURATION) {
        Serial.println("Timeout lors de lasaisie code");
        inputCode = "";  // Vider le code
        changeState(STATE_ERROR);
    }
}

// attente réponse code
void stateWaitingCodeAuth() {
    if ((millis() - stateStartTime) > TIMEOUT_DURATION) {
        Serial.println("Timeout attente réponse code");
        changeState(STATE_ERROR);
    }
}

// state accès autorisé
void stateAccessGranted() {
    Serial.println("Accès autorisé");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Acces autorise");
    lcd.setCursor(0, 1);
    lcd.print("Ouverture coffre");
    ledAuthorize();

    // envoi mail et servomoteur
    delay(DISPLAY_DURATION);
    changeState(STATE_IDLE);
}

// state accès refusé
void stateAccessDenied() {
    Serial.println("Accès refusé");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Acces refuse");
    lcd.setCursor(0, 1);
    lcd.print("Reessayez...");

    ledDenied();
    delay(DISPLAY_DURATION);
    changeState(STATE_IDLE);
}

// State Erreur
void stateError() {
    Serial.println("Erreur ");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ERREUR");
    lcd.setCursor(0, 1);
    lcd.print("Reessayez...");

    ledError();
    delay(DISPLAY_DURATION);
    changeState(STATE_IDLE);
}

void setup() {
    Serial.begin(115200);

    pinMode(PIN_BUTTON, INPUT);
    pinMode(PIN_RED, OUTPUT);
    pinMode(PIN_GREEN, OUTPUT);
    pinMode(PIN_BLUE, OUTPUT);

    lcd.init();
    lcd.backlight();
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

    authorizationFeed.setCallback(authButtonCallback);
    authorizationCodeFeed.setCallback(authCodeCallback);

    mqtt.subscribe(&authorizationFeed);
    mqtt.subscribe(&authorizationCodeFeed);

    stateStartTime = millis();
}

void loop() {
    MQTT_connect();

    // Traite les messages MQTT entrants
    mqtt.processPackets(10);

    // Ping pour garder la connexion active
    if (!mqtt.ping()) {
        mqtt.disconnect();
    }

    switch (currentState) {
        case STATE_IDLE:
            stateIdle();
            break;

        case STATE_WAITING_BUTTON_AUTH:
            stateWaitingButtonAuth();
            break;

        case STATE_ENTERING_CODE:
            stateEnterCode();
            break;

        case STATE_WAITING_CODE_AUTH:
            stateWaitingCodeAuth();
            break;

        case STATE_ACCESS_GRANTED:
            stateAccessGranted();
            break;

        case STATE_ACCESS_DENIED:
            stateAccessDenied();
            break;

        case STATE_ERROR:
            stateError();
            break;

        default:
            Serial.println("Erreur d'état");
            changeState(STATE_IDLE);
            break;
    }
}