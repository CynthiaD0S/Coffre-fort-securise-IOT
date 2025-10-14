#include <Arduino.h>
#include <driver/adc.h>

#define pinLed 2
#define ldrPin 36  // pin VP

int ldrValue;
int sensorValue = 0;
int sensorMin = 4095;
int sensorMax = 0;

void setup() {
    Serial.begin(115200);
    analogSetPinAttenuation(ldrPin, (adc_attenuation_t)ADC_ATTEN_DB_12);
    analogSetWidth(12);

    pinMode(pinLed, OUTPUT);
    digitalWrite(pinLed, HIGH);
    Serial.println("Début calibration...");

    while (millis() < 5000) {
        sensorValue = analogRead(ldrPin);
        if (sensorValue > sensorMax) {
            sensorMax = sensorValue;
        }
        if (sensorValue < sensorMin) {
            sensorMin = sensorValue;
        }
    }
    digitalWrite(pinLed, LOW);
    // Afficher les valeurs de calibration
    Serial.printf("sensorMin: %d (%.3fV)\n", sensorMin, (sensorMin / 4095.0) * 3.1);
    Serial.printf("sensorMax: %d (%.3fV)\n", sensorMax, (sensorMax / 4095.0) * 3.1);
}

void loop() {
    sensorValue = analogRead(ldrPin);
    sensorValue = constrain(sensorValue, sensorMin, sensorMax);
    // apply the calibration to the sensor reading
    sensorValue = map(sensorValue, sensorMin, sensorMax, 0, 255);

    Serial.println("Value__: " + String(sensorValue));
    while (sensorValue > 220) {
        digitalWrite(pinLed, HIGH);
        sensorValue = analogRead(ldrPin);
        sensorValue = constrain(sensorValue, sensorMin, sensorMax);
        // apply the calibration to the sensor reading
        sensorValue = map(sensorValue, sensorMin, sensorMax, 0, 255);
        Serial.println("Value: " + String(sensorValue));
    }
    digitalWrite(pinLed, LOW);

    delay(1000);
}