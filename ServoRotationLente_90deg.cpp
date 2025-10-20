#include <ESP32Servo.h>
#define PIN_SG90 22  // Broche de signal

Servo sg90;

void setup() {
  sg90.setPeriodHertz(50);            // Fréquence PWM pour le SG90
  sg90.attach(PIN_SG90, 500, 2400);   // Impulsions min et max (µs)

  // Position initiale
  sg90.write(0);
  delay(500);

  // Rotation lente de 0° à 90°
  for (int angle = 0; angle <= 90; angle++) {
    sg90.write(angle);
    delay(30); // ralentit le mouvement (20–40 ms = fluide)
  }

  // Facultatif : détacher pour arrêter complètement le servo
  sg90.detach();
}

void loop() {
  // Ne rien faire : le servo reste à 90°
}
