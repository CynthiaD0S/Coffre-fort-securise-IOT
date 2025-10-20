#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "time.h"

#define ENABLE_SMTP
#define ENABLE_DEBUG
#include <ReadyMail.h>

WiFiClientSecure ssl_client;
SMTPClient smtp(ssl_client);

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
    if (!smtp.isConnected()) return;}

void setup() {
    Serial.begin(115200);
    WiFi.begin("Groupe-esiea", "Kk5A2Ugmo");
    while (WiFi.status() != WL_CONNECTED) delay(500);

    ssl_client.setInsecure();

    auto statusCallback = [](SMTPStatus status) {
        Serial.println(status.text);
    };

    smtp.connect("smtp.gmail.com", 465, statusCallback);

    if (smtp.isConnected()) {
        smtp.authenticate("t2224295@gmail.com", "pdao hstb zomy paii", readymail_auth_password);

        SMTPMessage msg;
        msg.headers.add(rfc822_from, "ReadyMail <t2224295@gmail.com>");
        msg.headers.add(rfc822_to, "Recipient <a22497752@gmail.com>");
        msg.headers.add(rfc822_subject, "Accès coffre fort");
        msg.text.body("This is a plain text message.");
        msg.html.body("<html><body><h1>Hello!</h1><p>Message envoyé à : " + getDateTime() + "</p></body></html>");

        configTime(0, 0, "pool.ntp.org");
        while (time(nullptr) < 100000) delay(100);
        msg.timestamp = time(nullptr);

          bool accesAutorise = true;  // tu peux changer selon ton autre code
        if (accesAutorise) {
            sendEmail("autorisé");
        } else {
            sendEmail("refusé");
        }
    



    smtp.send(msg);
    }
}

void loop() {}