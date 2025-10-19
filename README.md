# Coffre fort sécurisé

L’objectif est de simuler un **système d’accès intelligent** avec double validation à une coffre fort.  
Tout d'abord, un individu souhaite débloquer le coffre fort. Il appuie sur un bouton qui déclenche le système de vérification.
Lorsqu'on déclenche le bouton, on envoie au serveur une demande de reconnaise. Ensuite le serveur retourne l'autorisation de se connecter.
Si le serveur autorise, on notifie à l'indivdu qu'il doit entrer enfin un mot de passe.
Une fois que c'est validé on ouvre le coffre tout en notifant le gestionnaire du coffe. Il recoit un mail avec les infos de connexion et peut s'opposer.

Dans notre cas , comment ça se passe ?
Le serveur est géré par un script python qui autorise ou non l'authentification. c'est également le serveur qui valide le code pin de l'user. Ici, comme on a pas le coffre fort, on utilise un servomoteur. Le servomoteur tourne pour montrer que le coffre est ouvert.

## 1. Déclenchement et autorisation initiale (entrée réseau)

- L’utilisateur appuie sur un **bouton poussoir** connecté à l’ESP32.
- L’ESP32 publie un message MQTT sur `iot/demande`.
- Un **script Python** (sur PC ou serveur) écoute ce topic et décide :

  - Autorisation ou refus (ex. via reconnaissance faciale, base de données…)
  - Publie la réponse sur `iot/autorisation` (`{"autorise": true}` ou `false`).
    L’ESP32 reçoit la réponse :

- Si autorisé → LED orange allumée → passage à l’étape suivante
- Si refus → LED rouge ou buzzer → fin du processus.

## 2. Validation du code PIN

- Si autorisation donnée, l’ESP32 active un **clavier matriciel** pour la saisie du code PIN.
- Une fois le code saisi :
  - L’ESP32 publie le code sur `acces/code`
  - Le script Python vérifie le code et renvoie une validation sur `acces/validation` (`true` / `false`)

Code correct → accès autorisé  
Code incorrect → LED rouge + alarme sonore

## 3. Ouvir le coffre

- Si validation positive :
  - L’ESP32 actionne un **servomoteur** (ouverture porte/barrière)
  - Allume une **LED verte** (accès autorisé)
- En cas d’échec → buzzer ou LED rouge

## 4. Notification du gestionnaire

- Une fois le processus terminé (autorisé ou refusé), le système envoie :
  - Un **e-mail** via SMTP
  - Un message MQTT sur `acces/log` pour archivage

Contenu du message :

- Identifiant utilisateur (ou image si reconnaissance faciale)
- Horodatage
- Résultat (autorisé / refusé)

### Installation Python

Installer les dépendances :

```bash
pip install paho-mqtt
```

## Ressources

[MQTT Avec Arduino](https://arduino.blaisepascal.fr/mqtt-avec-arduino/)

[Adafruit MQTT](https://learn.adafruit.com/mqtt-adafruit-io-and-you/intro-to-adafruit-mqtt)

[Librairie Adafruit](https://github.com/adafruit/Adafruit_MQTT_Library/tree/master/examples)
[text](https://arduinogetstarted.com/tutorials/arduino-rgb-led)