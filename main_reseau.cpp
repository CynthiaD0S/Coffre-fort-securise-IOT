#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "time.h"

#define ENABLE_SMTP
#define ENABLE_DEBUG
#include <ReadyMail.h>

// WIFI
#define WLAN_SSID "user"
#define WLAN_PASS "mdp"

// ADAFRUIT
#define AIO_SERVER "io.adafruit.com"
#define AIO_SERVERPORT 1883
#define AIO_USERNAME "user"
#define AIO_KEY "mdp"

// MAIL
WiFiClient client;
WiFiClientSecure ssl_client;
SMTPClient smtp(ssl_client);

// MQTT
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Feeds pour publier
Adafruit_MQTT_Publish buttonFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/iot.access-by-button");
Adafruit_MQTT_Publish codeFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/iot.access-by-code");

// Feeds pour s'abonner
Adafruit_MQTT_Subscribe authorizationFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/iot.authorization");
Adafruit_MQTT_Subscribe authorizationCodeFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/iot.authorization-code");

// Callback pour la réponse à l'authentification par ID
void authButtonCallback(char* data, uint16_t len) {
    String message = String(data);
    message.trim();
    Serial.print("Reponse MQTT (ID) recue: ");
    Serial.println(message);

    // On transfère la réponse à l'ESP_Interface via Serial2
    if (message == "true") {
        Serial2.println("AUTH_ID,true");
    } else {
        Serial2.println("AUTH_ID,false");
    }
}

// Callback pour la réponse à l'authentification par code
void authCodeCallback(char* data, uint16_t len) {
    String message = String(data);
    message.trim();
    Serial.print("Reponse MQTT (Code) recue: ");
    Serial.println(message);

    // On transfère la réponse à l'ESP_Interface via Serial2
    if (message == "true") {
        Serial2.println("AUTH_CODE,true");
    } else {
        Serial2.println("AUTH_CODE,false");
    }
}

void connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }
    Serial.print("Connexion WiFi à ");
    Serial.println(WLAN_SSID);

    WiFi.begin(WLAN_SSID, WLAN_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connecté !");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

void MQTT_connect() {
    if (mqtt.connected()) {
        return;
    }
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
    Serial.println("Connecté au MQTT !");
}

String getDateTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "Erreur heure";
    }
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
    return String(buffer);
}

void sendMailAcees(String status, String id_user) {
    ssl_client.setInsecure();

    auto statusCallback = [](SMTPStatus status) {
        Serial.println(status.text);
    };

    Serial.println("Connexion au serveur SMTP...");
    smtp.connect("smtp.gmail.com", 465, statusCallback);

    if (smtp.isConnected()) {
        Serial.println("Authentification...");
        smtp.authenticate("t2224295@gmail.com", "pdao hstb zomy paii", readymail_auth_password);
    } else {
        Serial.println("Echec de connexion SMTP.");
        return;
    }

    String dateTime = getDateTime();
    int espaceIndex = dateTime.indexOf(' ');
    String date = dateTime.substring(0, espaceIndex);
    String heure = dateTime.substring(espaceIndex + 1);

    SMTPMessage msg;
    msg.headers.add(rfc822_from, "Coffre Fort IoT <t2224295@gmail.com>");
    msg.headers.add(rfc822_to, "Recipient <a22497752@gmail.com>");
    msg.headers.add(rfc822_subject, "Alerte d'accès au coffre-fort");

    String htmlBody = "<html><body><p>Bonjour,</p>";
    if (status == "Autorisé") {
        htmlBody += "<p>Un accès au coffre-fort a été <b>autorisé</b>.</p>";
    } else {
        htmlBody += "<p>Une tentative d'accès au coffre-fort a été <b>refusée</b>.</p>";
    }
    htmlBody += "<p><b>Détails:</b></p><ul>";
    htmlBody += "<li>ID de l'utilisateur : " + id_user + "</li>";
    htmlBody += "<li>Date et heure : " + date + " à " + heure + "</li>";
    htmlBody += "</ul><p>Cordialement,<br>Le système de surveillance</p></body></html>";

    msg.html.body(htmlBody.c_str());
    msg.timestamp = time(nullptr);

    Serial.println("Envoi de l'email...");
    if (smtp.send(msg)) {
        Serial.println("Email envoyé avec succès.");
    } else {
        Serial.println("Erreur lors de l'envoi de l'email.");
    }
    // smtp.close();
}

void setup() {
    Serial.begin(115200);
    // Port Série pour la communication avec l'autre ESP32
    Serial2.begin(115200);
    Serial.println("Initialisation de l'ESP32 Reseau...");
    connectWiFi();

    configTime(0, 0, "pool.ntp.org");
    while (time(nullptr) < 100000) {
        delay(100);
    }
    Serial.println("Heure synchronisée.");

    authorizationFeed.setCallback(authButtonCallback);
    authorizationCodeFeed.setCallback(authCodeCallback);
    MQTT_connect();
    mqtt.subscribe(&authorizationFeed);
    mqtt.subscribe(&authorizationCodeFeed);

    // Prévenir l'autre ESP qu'on est prêt
    Serial.println("Systeme Reseau pret");
}

void loop() {
    MQTT_connect();

    // Traite les messages MQTT entrants
    mqtt.processPackets(10);

    // Ping MQTT pour garder la connexion active
    if (!mqtt.ping()) {
        mqtt.disconnect();
    }

    // Ecoute les ordres venant de l'ESP_Interface
    if (Serial2.available()) {
        String commande = Serial2.readStringUntil('\n');
        commande.trim();

        // Gérer la commande PING du setup de l'autre ESP
        if (commande == "PING") {
            Serial.println("PING recu, reponse NET_READY envoyee.");
            Serial2.println("NET_READY");
        }
        // Gérer les autres commandes qui contiennent des données
        else {
            Serial.print("Commande complexe recue: ");
            Serial.println(commande);

            int virguleIndex = commande.indexOf(',');
            if (virguleIndex > 0) {
                String cmd_type = commande.substring(0, virguleIndex);
                String cmd_data = commande.substring(virguleIndex + 1);

                if (cmd_type == "PUB_ID") {
                    Serial.print("Publication ID: ");
                    Serial.println(cmd_data);
                    buttonFeed.publish(cmd_data.c_str());
                } else if (cmd_type == "PUB_CODE") {
                    Serial.print("Publication Code: ");
                    Serial.println(cmd_data);
                    codeFeed.publish(cmd_data.c_str());
                } else if (cmd_type == "SEND_MAIL") {
                    int virguleIndex2 = cmd_data.indexOf(',');
                    if (virguleIndex2 > 0) {
                        String status = cmd_data.substring(0, virguleIndex2);
                        String id_user = cmd_data.substring(virguleIndex2 + 1);
                        sendMailAcees(status, id_user);
                    }
                }
            }
        }
    }
}
