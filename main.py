import paho.mqtt.client as mqtt
import time

# Configuration Adafruit IO
MQTT_SERVER = "io.adafruit.com"
MQTT_PORT = 1883
MQTT_USERNAME = "user"
MQTT_KEY = "key"

# Topics MQTT
TOPIC_BUTTON = f"{MQTT_USERNAME}/feeds/iot.access-by-button"
TOPIC_AUTHORIZATION = f"{MQTT_USERNAME}/feeds/iot.authorization"

# Callbacks MQTT
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connecté au serveur MQTT Adafruit IO")
        # S'abonner au topic du bouton
        client.subscribe(TOPIC_BUTTON)
        print(f"Abonné à: {TOPIC_BUTTON}")
    else:
        print(f"Échec de connexion. Code: {rc}")

def on_message(client, userdata, msg):
    message = msg.payload.decode()
    print(f"\nMessage reçu sur {msg.topic}: {message}")
    
    # Si on reçoit un appui sur le bouton
    if msg.topic == TOPIC_BUTTON and message == "1":
        print("Bouton appuyé détecté!")
        
        # Demander l'autorisation à l'utilisateur
        reponse = input("Autoriser l'accès? (o/n): ").lower()
        
        if reponse == 'o' or reponse == 'oui':
            client.publish(TOPIC_AUTHORIZATION, "true")
            print("Autorisation envoyée: true")
        else:
            client.publish(TOPIC_AUTHORIZATION, "false")
            print("Autorisation refusée: false")

def on_publish(client, userdata, mid):
    print("Message publié avec succès")

def on_disconnect(client, userdata, rc):
    if rc != 0:
        print("Déconnexion inattendue. Reconnexion...")

# Créer le client MQTT
client = mqtt.Client()
client.username_pw_set(MQTT_USERNAME, MQTT_KEY)

# Assigner les callbacks
client.on_connect = on_connect
client.on_message = on_message
client.on_publish = on_publish
client.on_disconnect = on_disconnect

# Connexion au serveur
print(f"Connexion à {MQTT_SERVER}...")
try:
    client.connect(MQTT_SERVER, MQTT_PORT, 60)
    
    # Boucle principale
    print("\nEn attente de messages... (Ctrl+C pour quitter)\n")
    client.loop_forever()
    
except KeyboardInterrupt:
    print("\n\nArrêt du programme...")
    client.disconnect()
except Exception as e:
    print(f"Erreur: {e}")