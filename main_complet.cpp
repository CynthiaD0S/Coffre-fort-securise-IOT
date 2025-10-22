#include <Arduino.h>
#include <ESP32Servo.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "time.h"
// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Wifi
#define WLAN_SSID "user"
#define WLAN_PASS "mdp"

// mail
#define ENABLE_SMTP
#define ENABLE_DEBUG
#include <ReadyMail.h>

WiFiClientSecure ssl_client;
SMTPClient smtp(ssl_client);

// Adafruit
#define AIO_SERVER "io.adafruit.com"
#define AIO_SERVERPORT 1883
#define AIO_USERNAME "user"
#define AIO_KEY "mdp"

// boutton
#define PIN_BUTTON 4

// servomoteur
#define PIN_SG90 15  // Broche de signal
Servo sg90;

// buzzer
#define BUZZER_PIN 2
// led rgb
const int PIN_RED = 19;
const int PIN_GREEN = 13;
const int PIN_BLUE = 12;

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
    STATE_IDLE,  // état initial en attente d'authentification
    STATE_ENTERING_ID,
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
String inputID = "";

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
    setLED(0, 0, 255);
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
    inputID = "";  // Vider l'ID après envoi
    // Détecter appui bouton
    int currentButtonState = digitalRead(PIN_BUTTON);

    if (currentButtonState == HIGH && previousButtonState == LOW) {
        Serial.println("Bouton pressé");
        changeState(STATE_ENTERING_ID);
    }
    previousButtonState = currentButtonState;
}

void stateEnterID() {
    if (previousState != STATE_ENTERING_ID) {
        inputID = "";  // On utilise inputID
        lcd.clear();
        lcd.setCursor(0, 0);
        Serial.println("entrez ID");
        lcd.print("Entrez ID:");
        lcd.setCursor(0, 1);
        lcd.print("C=OK E=Effacer");
        previousState = STATE_ENTERING_ID;
    }

    char key = keypad.getKey();

    if (key) {
        Serial.print(key);
        if (key == 'C') {  // Validation
            if (inputID.length() == 4) {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Envoi ID...");
                Serial.println("ID: ");
                // On envoie l'ID sur le feed du bouton
                if (buttonFeed.publish(inputID.c_str())) {
                    Serial.print("ID envoyé: ");
                    Serial.println(inputID);
                    lcd.setCursor(0, 1);
                    lcd.print("Attente verif.");
                    ledWaiting();
                    changeState(STATE_WAITING_BUTTON_AUTH);  // On attend la réponse
                } else {
                    Serial.println("Erreur envoi ID");
                    inputID = "";
                    changeState(STATE_ERROR);
                }
            } else {
                lcd.clear();
                lcd.print("ID doit avoir");
                lcd.setCursor(0, 1);
                lcd.print("4 chiffres !");
                delay(2000);
                previousState = STATE_IDLE;
                stateEnterID();
                /*
                // Message d'erreur si l'ID n'a pas 4 chiffres
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("ID: ");
                lcd.setCursor(4, 0);
                for (int i = 0; i < inputID.length(); i++) {
                    lcd.print("*");
                }
                lcd.setCursor(0, 1);
                lcd.print("ID de 4 chiffres");
                delay(2000);

                // Ré-afficher l'écran de saisie
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("ID: ");
                lcd.setCursor(4, 0);
                for (int i = 0; i < inputID.length(); i++) {
                    lcd.print("*");
                }
                lcd.setCursor(0, 1);
                lcd.print("C=OK E=Effacer");*/
            }

        } else if (key == 'E') {  // Effacer
            inputID = "";
            inputID = "";
            lcd.clear();
            lcd.print("ID efface.");
            delay(1000);
            previousState = STATE_IDLE;
            stateEnterID();
            /*
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("ID efface:");
            lcd.setCursor(0, 1);
            lcd.print("C=OK E=Effacer");
            delay(1000);

            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("ID: ");
            lcd.setCursor(0, 1);
            lcd.print("C=OK E=Effacer");*/

        } else if ((key >= '0' && key <= '9') && (inputID.length() < 4)) {
            // Ajout d'un chiffre à l'ID
            inputID += key;
            Serial.println(inputID);
            lcd.setCursor(12, 0);
            for (int i = 0; i < inputID.length(); i++) {
                lcd.print("*");
            }
        }
    }

    // Gestion du timeout
    if ((millis() - stateStartTime) > TIMEOUT_DURATION) {
        Serial.println("Timeout lors de la saisie ID");
        inputID = "";
        changeState(STATE_ERROR);
    }
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

void buzzerAndServo() {
    // Rotation lente de 0° à 90°
    for (int angle = 0; angle <= 90; angle++) {
        Serial.println(angle);
        sg90.write(angle);
        delay(500);  // ralentit le mouvement
        digitalWrite(BUZZER_PIN, HIGH);
        delay(10);
        digitalWrite(BUZZER_PIN, LOW);
    }

    // Facultatif : détacher pour arrêter complètement le servo
    sg90.detach();
}

String getDateTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "Erreur récupération heure";
    }
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
    return String(buffer);
}

void sendEmail(String accesStatus) {
    if (!smtp.isConnected()) return;
}

// send mail
void sendMailAcees(String status, String id_user) {
    ssl_client.setInsecure();

    auto statusCallback = [](SMTPStatus status) {
        Serial.println(status.text);
    };

    smtp.connect("smtp.gmail.com", 465, statusCallback);

    if (smtp.isConnected()) {
        smtp.authenticate("t2224295@gmail.com", "pdao hstb zomy paii", readymail_auth_password);

        configTime(0, 0, "pool.ntp.org");
        while (time(nullptr) < 100000) delay(100);
    }

    String dateTime = getDateTime();
    int espaceIndex = dateTime.indexOf(' ');
    String date = dateTime.substring(0, espaceIndex);
    String heure = dateTime.substring(espaceIndex + 1);

    SMTPMessage msg;
    msg.headers.add(rfc822_from, "coffre fort IoT <t2224295@gmail.com>");
    msg.headers.add(rfc822_to, "Recipient <a22497752@gmail.com>");
    msg.headers.add(rfc822_subject, "Accès coffre fort");
    msg.text.body("This is a plain text message.");
    if (status == "Autorisé") {
        msg.html.body(
            "<html><body>"
            "<p>Bonjour,</p>"
            "<p>Un accès au coffre-fort a été enregistré.</p>"
            "<p>Détails: </p>"
            "<p> ID de l'utilisateur :  " +
            id_user +
            " </p>"
            "<p> Connexion le : " +
            date + " à " + heure +
            "</p>"
            "<p>Cordialement,</p>"
            "<p>Le système de surveillance du coffre-fort</p>"
            "</body></html>");
    } else {
        msg.html.body(
            "<html><body>"
            "<p>Bonjour,</p>"
            "<p>Une tentative d'accès au coffre-fort a été refusée.</p>"
            "<p>Détails: </p>"
            "<p> ID de l'utilisateur :  " +
            id_user +
            " </p>"
            "<p> Connexion le : " +
            date + " à " + heure +
            "</p>"
            "<p>Cordialement,</p>"
            "<p>Le système de surveillance du coffre-fort</p>"
            "</body></html>");
    }

    msg.timestamp = time(nullptr);
    smtp.send(msg);
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
    buzzerAndServo();
    sendMailAcees("Autorisé", inputID);
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
    sendMailAcees("Refusé", inputID);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(150);
    digitalWrite(BUZZER_PIN, LOW);
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

    // servomoteur
    sg90.setPeriodHertz(50);           // Fréquence PWM pour le SG90
    sg90.attach(PIN_SG90, 500, 2400);  // Impulsions min et max (µs)
    pinMode(BUZZER_PIN, OUTPUT);       // Configure la broche en sortie
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
        case STATE_ENTERING_ID:
            stateEnterID();
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
