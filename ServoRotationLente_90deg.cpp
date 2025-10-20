#include <ESP32Servo.h>
#define PIN_SG90 22  // Broche de signal

Servo sg90;
#define BUZZER_PIN 23  // Broche où le buzzer est connecté

void setup() {
    sg90.setPeriodHertz(50);           // Fréquence PWM pour le SG90
    sg90.attach(PIN_SG90, 500, 2400);  // Impulsions min et max (µs)
    pinMode(BUZZER_PIN, OUTPUT);       // Configure la broche en sortie

    Serial.begin(115200);
    // Rotation lente de 0° à 90°
    for (int angle = 0; angle <= 90; angle++) {
        Serial.println(angle);
        sg90.write(angle);
        delay(500);  // ralentit le mouvement (20–40 ms = fluide)
        digitalWrite(BUZZER_PIN, HIGH);
        delay(10);
        digitalWrite(BUZZER_PIN, LOW);
    }

    // Facultatif : détacher pour arrêter complètement le servo
    sg90.detach();
}

void loop() {
    // Ne rien faire : le servo reste à 90°
}