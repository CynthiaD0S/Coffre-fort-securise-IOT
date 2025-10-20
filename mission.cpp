#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "time.h"

#define ENABLE_SMTP
#define ENABLE_DEBUG
#include <ReadyMail.h>

#define WLAN_SSID "iPhone de Cynthia"
#define WLAN_PASS "CynthiaLivai740."

WiFiClientSecure ssl_client;
SMTPClient smtp(ssl_client);

int i = 1;

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

void gererAcces(String status) {
    ssl_client.setInsecure();

    auto statusCallback = [](SMTPStatus status) {
        Serial.println(status.text);
    };

    smtp.connect("smtp.gmail.com", 465, statusCallback);

    if (smtp.isConnected()) {
        smtp.authenticate("t2224295@gmail.com", "pdao hstb zomy paii", readymail_auth_password);

        String dateTime = getDateTime();

        SMTPMessage msg;
        msg.headers.add(rfc822_from, "ReadyMail <t2224295@gmail.com>");
        msg.headers.add(rfc822_to, "Recipient <a22497752@gmail.com>");
        msg.headers.add(rfc822_subject, "Accès coffre fort");
        msg.text.body("This is a plain text message.");
        msg.html.body("<html><body><h1>Accès " + status + "</h1><p>Envoyé à : " + dateTime + "</p></body></html>");
        msg.html.body(
        "<html><body>"
        "<p><b>Envoyé à :</b> " "</p>"
        "<p><b>Accès :</b> " + status + "</p>"
        "<p><b>Heure :</b> " + dateTime + "</p>"
        "</body></html>"
        );
        configTime(0, 0, "pool.ntp.org");
        while (time(nullptr) < 100000) delay(100);
        msg.timestamp = time(nullptr);

        smtp.send(msg);
    }
}

void setup() {
    Serial.begin(115200);
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
}

void loop() {
    if (i == 1) {
        gererAcces("Accès autorisé");
    }
    i++;
}