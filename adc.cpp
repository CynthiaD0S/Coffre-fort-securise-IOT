#include <Arduino.h>
#include <driver/adc.h>

#define pinLed 2
#define uS_TO_mS_FACTOR 1000ULL
#define TIME_TO_SLEEP 1000
#define ldrPin 36

int ldrValue;
// These constants won't change:
const int sensorPin = 36;  // pin that the sensor is attached to

// variables:
int sensorValue = 0;   // the sensor value
int sensorMin = 4095;  // minimum sensor value
int sensorMax = 0;     // maximum sensor value

void setup() {
    Serial.begin(115200);
    // Configure ADC on GPIO 36 (ADC1_CH0)
    analogSetPinAttenuation(ldrPin, (adc_attenuation_t)ADC_ATTEN_DB_12);  // Set attenuation to 12dB (max ~3.1V)
    analogSetWidth(12);

    pinMode(pinLed, OUTPUT);
    //  ESP32 wakes after TIME_TO_SLEEP milliseconds
    //  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_mS_FACTOR);
    digitalWrite(pinLed, HIGH);

    // calibrate during the first five seconds
    while (millis() < 5000) {
        sensorValue = analogRead(ldrPin);
        // record the maximum sensor value
        if (sensorValue > sensorMax) {
            sensorMax = sensorValue;
        }
        // record the minimum sensor value
        if (sensorValue < sensorMin) {
            sensorMin = sensorValue;
        }
    }
    digitalWrite(pinLed, LOW);
    /* DeepSleep
    digitalWrite(pinLed, HIGH);
    delay(TIME_TO_SLEEP);
    digitalWrite(pinLed, LOW);
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_mS_FACTOR);
    esp_deep_sleep_start();*/
}

void loop() {
    ldrValue = analogRead(ldrPin);
    Serial.println("Value: " + String(ldrValue));
    while (ldrValue > 2000) {
        digitalWrite(pinLed, HIGH);
        ldrValue = analogRead(ldrPin);
        Serial.println("Value: " + String(ldrValue));
    }
    digitalWrite(pinLed, LOW);

    delay(2000);

    // DeepSleep Mode
}