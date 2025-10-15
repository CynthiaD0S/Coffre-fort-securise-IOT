# 🚪 ESP32 + MQTT + Python – Contrôle d’accès connecté

Ce projet est un **Proof of Concept (POC)** de contrôle d’accès basé sur **ESP32**, **MQTT** et **un script Python côté serveur**.  
L’objectif est de simuler un **système d’accès intelligent** avec double validation :  
1️⃣ une autorisation initiale depuis un serveur,  
2️⃣ une validation par code PIN local,  
3️⃣ une action physique (servo) et  
4️⃣ une notification réseau.

---

## 🧭 Fonctionnement global

```
[ Bouton ESP32 ]  →  [ MQTT Broker ]  →  [ Script Python ]
        ↓                                    ↑
  [ Clavier PIN ] → MQTT aller/retour ← [ Validation code ]
        ↓
 [ Servo + LED ]  →  [ Email de notification ]
```

---

## ✅ 1. Déclenchement et autorisation initiale (entrée réseau)

- L’utilisateur appuie sur un **bouton poussoir** connecté à l’ESP32.
- L’ESP32 publie un message MQTT sur `acces/demande`.
- Un **script Python** (sur PC ou serveur) écoute ce topic et décide :
  - Autorisation ou refus (ex. via reconnaissance faciale, base de données…)
  - Publie la réponse sur `acces/autorisation` (`{"autorise": true}` ou `false`).

📡 L’ESP32 reçoit la réponse :

- ✅ Si autorisé → LED orange allumée → passage à l’étape suivante
- ❌ Si refus → LED rouge ou buzzer → fin du processus.

---

## ✅ 2. Validation locale du code PIN (entrée physique)

- Si autorisation donnée, l’ESP32 active un **clavier matriciel** pour la saisie du code PIN.
- Une fois le code saisi :
  - L’ESP32 publie le code sur `acces/code`
  - Le script Python vérifie le code et renvoie une validation sur `acces/validation` (`true` / `false`)

🟡 Code correct → accès autorisé  
🔴 Code incorrect → LED rouge + alarme sonore

---

## ✅ 3. Action physique sur site (sortie physique)

- Si validation positive :
  - L’ESP32 actionne un **servomoteur** (ouverture porte/barrière)
  - Allume une **LED verte** (accès autorisé)
- En cas d’échec → buzzer ou LED rouge

⚙️ Cette partie est exécutée localement sur l’ESP32.

---

## ✅ 4. Notification externe (sortie réseau)

- Une fois le processus terminé (autorisé ou refusé), le système envoie :
  - Un **e-mail** via SMTP
  - Ou un message MQTT sur `acces/log` pour archivage

Contenu du message :

- 📇 Identifiant utilisateur (ou image si reconnaissance faciale)
- 🕒 Horodatage
- 🟢 Résultat (autorisé / refusé)

---

## 🧾 Récapitulatif

| Étape                     | Type                   | Technologie                           |
| ------------------------- | ---------------------- | ------------------------------------- |
| 1. Déclenchement          | Entrée réseau          | Bouton ESP32 → MQTT                   |
| 2. Validation code PIN    | Entrée physique + MQTT | Clavier matriciel + MQTT aller/retour |
| 3. Action physique locale | Sortie physique        | Servomoteur + LED                     |
| 4. Notification           | Sortie réseau          | Email ou MQTT log                     |

---

## 🧰 Matériel utilisé

- 🧠 ESP32 DevKit
- 🟢 Bouton poussoir
- ⌨️ Clavier matriciel 4x4
- 🔧 Servomoteur SG90 (ou équivalent)
- 🟡 LED (orange, verte, rouge)
- 📡 Broker MQTT (ex. Mosquitto ou Adafruit IO)
- 🐍 Python pour autorisation / validation / notification
- 📧 Service SMTP (Gmail ou autre)

---

## 🕹️ Exemple de topics MQTT

| Topic                | Direction                  | Message Exemple                |
| -------------------- | -------------------------- | ------------------------------ |
| `acces/demande`      | ESP32 → Serveur            | `{ "id": "porte1" }`           |
| `acces/autorisation` | Serveur → ESP32            | `{ "autorise": true }`         |
| `acces/code`         | ESP32 → Serveur            | `{ "code": "1234" }`           |
| `acces/validation`   | Serveur → ESP32            | `{ "valide": true }`           |
| `acces/log`          | ESP32 ou Serveur → Gestion | `{"user":"ID1","statut":"ok"}` |

---

## 🧑‍💻 Installation rapide

### 📌 Côté ESP32

- Programmer avec Arduino IDE ou PlatformIO
- Connecter bouton, clavier, LED, servo
- Configurer Wi-Fi + MQTT broker
- Publier/s’abonner aux topics ci-dessus.

### 🐍 Côté Python

- Installer les dépendances :
  ```bash
  pip install paho-mqtt
  ```
- Lancer le script Python d’autorisation :
  ```bash
  python main.py
  ```

---

## ✨ Améliorations possibles

- 🔐 Gestion des tentatives et verrouillage temporaire
- 🕒 Horodatage NTP
- 📲 Ajout de Bluetooth ou badge RFID
- 🌐 Dashboard de supervision des accès
- 📥 Logs stockés en base de données

---

## 🧑‍🔬 Auteurs

- 💻 Projet POC IoT ESP32 – Contrôle d’accès
- 🌐 MQTT + Python + ESP32
- 🛠️ Démo académique pour systèmes distribués IoT

## Ressources

[MQTT Avec Arduino](https://arduino.blaisepascal.fr/mqtt-avec-arduino/)
[Adafruit MQTT](https://learn.adafruit.com/mqtt-adafruit-io-and-you/intro-to-adafruit-mqtt)
[Librairie Adafruit](https://github.com/adafruit/Adafruit_MQTT_Library/tree/master/examples)
