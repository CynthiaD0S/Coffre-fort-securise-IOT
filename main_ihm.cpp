#include <Arduino.h>
#include <ESP32Servo.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Bouton
#define PIN_BUTTON 4
int previousButtonState = LOW;
// Servomoteur
#define PIN_SG90 15
Servo sg90;

// Buzzer
#define BUZZER_PIN 2

// LED RGB
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

enum State {
    STATE_IDLE,
    STATE_ENTERING_ID,
    STATE_WAITING_BUTTON_AUTH,
    STATE_ENTERING_CODE,
    STATE_WAITING_CODE_AUTH,
    STATE_ACCESS_GRANTED,
    STATE_ACCESS_DENIED,
    STATE_ERROR
};
State currentState = STATE_IDLE;
State previousState = STATE_IDLE;

String inputCode = "";
String inputID = "";
unsigned long stateStartTime = 0;
const unsigned long TIMEOUT_DURATION = 50000;
const unsigned long DISPLAY_DURATION = 8000;

void setLED(int r, int g, int b) {
    analogWrite(PIN_RED, r);
    analogWrite(PIN_GREEN, g);
    analogWrite(PIN_BLUE, b);
}

void ledOff() {
    setLED(0, 0, 0);
}
void ledWaiting() {
    setLED(204, 102, 0);
}
void ledAuthorize() {
    setLED(0, 255, 0);
}
void ledDenied() {
    setLED(255, 0, 0);
}
void ledError() {
    setLED(0, 0, 255);
}

void changeState(State newState) {
    previousState = currentState;
    currentState = newState;
    stateStartTime = millis();
}

void stateIdle() {
    if (previousState != STATE_IDLE) {
        ledOff();
        lcd.backlight();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" Coffre Verrouille ");
        lcd.setCursor(0, 1);
        lcd.print("Appuyez bouton...");
        inputCode = "";
        previousState = STATE_IDLE;
    }

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
        inputID = "";
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Entrez ID:");
        lcd.setCursor(0, 1);
        lcd.print("C=OK E=Effacer");
        previousState = STATE_ENTERING_ID;
    }

    char key = keypad.getKey();
    if (key) {
        if (key == 'C') {
            if (inputID.length() == 4) {
                lcd.clear();
                lcd.print("Envoi ID");
                Serial.print("Envoi de la commande PUB_ID a ESP_Reseau: ");
                Serial.println(inputID);

                // envoie commande à ESP_Reseau via Serial2
                Serial2.println("PUB_ID," + inputID);

                lcd.setCursor(0, 1);
                lcd.print("Attente verif...");
                ledWaiting();
                changeState(STATE_WAITING_BUTTON_AUTH);
            } else {
                lcd.clear();
                lcd.print("ID doit avoir");
                lcd.setCursor(0, 1);
                lcd.print("4 chiffres !");
                delay(2000);
                previousState = STATE_IDLE;
                stateEnterID();
            }
        } else if (key == 'E') {  // Effacer
            inputID = "";
            lcd.clear();
            lcd.print("ID efface.");
            delay(1000);
            previousState = STATE_IDLE;
            stateEnterID();
        } else if (isDigit(key) && inputID.length() < 4) {
            inputID += key;
            lcd.setCursor(10, 0);
            lcd.print(inputID);
        }
    }

    if ((millis() - stateStartTime) > TIMEOUT_DURATION) {
        Serial.println("Timeout lors de la saisie ID.");
        changeState(STATE_ERROR);
    }
}

// Attend la réponse de l'ESP_Reseau concernant l'ID
void stateWaitingButtonAuth() {
    if (Serial2.available()) {
        String response = Serial2.readStringUntil('\n');
        response.trim();
        Serial.print("Reponse recue de ESP_Reseau: ");
        Serial.println(response);

        if (response == "AUTH_ID,true") {
            changeState(STATE_ENTERING_CODE);
        } else if (response == "AUTH_ID,false") {
            changeState(STATE_ACCESS_DENIED);
        } else {
            // Réponse inattendue
            changeState(STATE_ERROR);
        }
    }

    if ((millis() - stateStartTime) > TIMEOUT_DURATION) {
        Serial.println("Timeout attente reponse bouton.");
        changeState(STATE_ERROR);
    }
}

// Gère la saisie du code secret
void stateEnterCode() {
    if (previousState != STATE_ENTERING_CODE) {
        inputCode = "";
        lcd.clear();
        lcd.print("ID Autorise");
        lcd.setCursor(0, 1);
        lcd.print("Entrez le code:");
        delay(2000);

        lcd.clear();
        lcd.print("Code:");
        lcd.setCursor(0, 1);
        lcd.print("C=OK E=Effacer");
        previousState = STATE_ENTERING_CODE;
    }

    char key = keypad.getKey();
    if (key) {
        if (key == 'C') {  // Valider
            if (inputCode.length() == 4) {
                lcd.clear();
                lcd.print("Envoi Code...");
                Serial.print("Envoi de la commande PUB_CODE a ESP_Reseau: ");
                Serial.println(inputCode);

                // Envoie la commande à l'ESP_Reseau via Serial2
                Serial2.println("PUB_CODE," + inputCode);

                lcd.setCursor(0, 1);
                lcd.print("Attente verif...");
                ledWaiting();
                changeState(STATE_WAITING_CODE_AUTH);
            } else {
                lcd.clear();
                lcd.print("Code doit avoir");
                lcd.setCursor(0, 1);
                lcd.print("4 chiffres !");
                delay(2000);
                previousState = STATE_IDLE;
                stateEnterCode();
            }
        } else if (key == 'E') {  // Effacer
            inputCode = "";
            lcd.clear();
            lcd.print("Code efface.");
            delay(1000);
            previousState = STATE_IDLE;
            stateEnterCode();
        } else if (isDigit(key) && inputCode.length() < 4) {
            inputCode += key;
            lcd.setCursor(6, 0);
            // Montrer des astérisques pour la confidentialité
            for (int i = 0; i < inputCode.length(); i++) {
                lcd.print("*");
            }
        }
    }

    if ((millis() - stateStartTime) > TIMEOUT_DURATION) {
        Serial.println("Timeout lors de la saisie code.");
        changeState(STATE_ERROR);
    }
}

// Attend la réponse de l'ESP_Reseau concernant le code
void stateWaitingCodeAuth() {
    if (Serial2.available()) {
        String response = Serial2.readStringUntil('\n');
        response.trim();
        Serial.print("Reponse recue de ESP_Reseau: ");
        Serial.println(response);

        if (response == "AUTH_CODE,true") {
            changeState(STATE_ACCESS_GRANTED);
        } else if (response == "AUTH_CODE,false") {
            changeState(STATE_ACCESS_DENIED);
        } else {
            changeState(STATE_ERROR);
        }
    }

    if ((millis() - stateStartTime) > TIMEOUT_DURATION) {
        Serial.println("Timeout attente reponse code.");
        changeState(STATE_ERROR);
    }
}

void BuzzerandServo() {
    sg90.attach(PIN_SG90, 500, 2400);
    sg90.write(90);

    // laisse le coffre ouvert pendant 5sec
    delay(DISPLAY_DURATION);
    // Referme le coffre lentement avec des bips
    lcd.clear();
    lcd.print("Fermeture...");
    // La rotation lente se fait maintenant de 90° à 0°
    for (int angle = 90; angle >= 0; angle -= 2) {
        sg90.write(angle);
        // Petit bip à chaque pas
        digitalWrite(BUZZER_PIN, HIGH);
        delay(5);
        digitalWrite(BUZZER_PIN, LOW);
        delay(30);
    }
    sg90.detach();
}
// Ouvre le coffre et envoie un email de succès
void stateAccessGranted() {
    Serial.println("Acces Autorise.");
    lcd.clear();
    lcd.print("Acces Autorise");
    lcd.setCursor(0, 1);
    lcd.print("Ouverture...");
    ledAuthorize();

    // Commande à l'ESP_Reseau pour envoyer l'email de succès
    String mailCommand = "SEND_MAIL,Autorisé," + inputID;
    Serial2.println(mailCommand);
    Serial.println("Commande email envoyee: " + mailCommand);

    BuzzerandServo();
    changeState(STATE_IDLE);
}

// Refuse l'accès et envoie un email d'échec
void stateAccessDenied() {
    Serial.println("Acces Refuse.");
    lcd.clear();
    lcd.print("Acces Refuse");
    ledDenied();

    // Commande à l'ESP_Reseau pour envoyer l'email d'échec
    String mailCommand = "SEND_MAIL,Refusé," + inputID;
    Serial2.println(mailCommand);
    Serial.println("Commande email envoyee: " + mailCommand);

    digitalWrite(BUZZER_PIN, HIGH);
    delay(500);
    digitalWrite(BUZZER_PIN, LOW);

    delay(DISPLAY_DURATION);
    changeState(STATE_IDLE);
}

// Gère les erreurs (timeout, etc.)
void stateError() {
    Serial.println("Erreur systeme.");
    lcd.clear();
    lcd.print("ERREUR");
    lcd.setCursor(0, 1);
    lcd.print("Reessayez...");
    ledError();
    delay(DISPLAY_DURATION);
    changeState(STATE_IDLE);
}

// verifie la connexion au reseau
void ESP_connect_reseau() {
    const int numRetries = 3;
    bool networkReady = false;

    for (int i = 1; i <= numRetries; i++) {
        Serial.print("Tentative de contact Reseau ");
        Serial.println(i);

        // Envoye une commande PING pour demander le statut
        Serial2.println("PING");

        // Attendre une réponse pendant 5 secondes
        unsigned long startTime = millis();
        while (millis() - startTime < 5000) {
            if (Serial2.available()) {
                String response = Serial2.readStringUntil('\n');
                response.trim();
                if (response == "NET_READY") {
                    Serial.println("ESP_Reseau est pret.");
                    lcd.clear();
                    lcd.print("Systeme Pret.");
                    delay(1000);
                    networkReady = true;
                    break;  // Sort du while
                }
            }
        }
        if (networkReady) {
            break;
        }
    }

    if (!networkReady) {
        Serial.println("Erreur: Pas de reponse de ESP_Reseau. Redemarrage...");
        lcd.clear();
        lcd.print("Erreur Reseau !");
        lcd.setCursor(0, 1);
        lcd.print("Redemarrage...");
        delay(3000);
        ESP.restart();
    }
}

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200);  // Pour communication avec ESP_Reseau

    pinMode(PIN_BUTTON, INPUT);
    pinMode(PIN_RED, OUTPUT);
    pinMode(PIN_GREEN, OUTPUT);
    pinMode(PIN_BLUE, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    sg90.setPeriodHertz(50);

    lcd.init();
    lcd.backlight();
    Serial.println("Initialisation de l'ESP32 Interface...");

    ESP_connect_reseau();
}

void loop() {
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
            Serial.println("Etat inconnu ! Retour a IDLE.");
            changeState(STATE_IDLE);
            break;
    }
}
