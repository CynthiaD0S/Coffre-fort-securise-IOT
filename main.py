import paho.mqtt.client as mqtt
import sys  

# --- Configuration Adafruit IO ---
MQTT_SERVER = "io.adafruit.com"
MQTT_PORT = 1883
MQTT_USERNAME = "user"  
MQTT_KEY = "mdp"     

# --- Topics MQTT ---
TOPIC_BUTTON = f"{MQTT_USERNAME}/feeds/iot.access-by-button"
TOPIC_AUTHORIZATION = f"{MQTT_USERNAME}/feeds/iot.authorization"
TOPIC_CODE_RECEIVED = f"{MQTT_USERNAME}/feeds/iot.access-by-code"
TOPIC_AUTHORIZATION_CODE = f"{MQTT_USERNAME}/feeds/iot.authorization-code"

def charger_lignes_fichier_en_set(nom_fichier):
    """
    Ouvre un fichier, lit chaque ligne, enlève les espaces/sauts de ligne
    et retourne un 'set' (ensemble) pour une recherche rapide.
    """
    lignes_set = set()
    try:
        with open(nom_fichier, 'r') as f:
            for ligne in f:
                lignes_set.add(ligne.strip()) # .strip() est crucial
        
        print(f"Fichier {nom_fichier} chargé: {lignes_set}")
        return lignes_set
        
    except FileNotFoundError:
        print(f"ERREUR FATALE: Le fichier {nom_fichier} est introuvable.")
        return None # Retourne None si le fichier n'existe pas


IDS_AUTORISES = charger_lignes_fichier_en_set("liste_id_auto.txt")
CODES_AUTORISES = charger_lignes_fichier_en_set("code_auto.txt")

# Vérifier si les fichiers ont été chargés
if IDS_AUTORISES is None or CODES_AUTORISES is None:
    print("Un ou plusieurs fichiers de configuration sont manquants. Arrêt du script.")
    sys.exit(1) # Quitte le programme

# --- Callbacks MQTT ---
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connecté au serveur MQTT Adafruit IO")
        # S'abonner aux topics
        client.subscribe(TOPIC_BUTTON)
        print(f"Abonné à: {TOPIC_BUTTON}")
        client.subscribe(TOPIC_CODE_RECEIVED)
        print(f"Abonné à: {TOPIC_CODE_RECEIVED}")
    else:
        print(f"Échec de connexion. Code: {rc}")

def on_message(client, userdata, msg):
    message = msg.payload.decode().strip() 
    print(f"\nMessage reçu sur {msg.topic}: {message}")
    
    if msg.topic == TOPIC_BUTTON:
        id_recu = message
        print(f"Demande d'accès reçue avec l'ID: {id_recu}")
        
        if id_recu in IDS_AUTORISES:
            print(f"ID {id_recu} est AUTORISÉ.")
            client.publish(TOPIC_AUTHORIZATION, "true")
            print("Autorisation (ID) envoyée: true")
        else:
            print(f"ID {id_recu} est REFUSÉ.")
            client.publish(TOPIC_AUTHORIZATION, "false")
            print("Autorisation (ID) refusée: false")

    elif msg.topic == TOPIC_CODE_RECEIVED:
        code_recu = message
        print(f"Code reçu: {code_recu}")
        
        if code_recu in CODES_AUTORISES:
            print("Code correct!")
            client.publish(TOPIC_AUTHORIZATION_CODE, "true")
            print("Autorisation (code) envoyée: true")
        else:
            print("Code incorrect!")
            client.publish(TOPIC_AUTHORIZATION_CODE, "false")
            print("Autorisation (code) refusée: false")

def on_publish(client, userdata, mid):
    print("Message publié avec succès")

def on_disconnect(client, userdata, rc):
    if rc != 0:
        print("Déconnexion inattendue. Reconnexion...")

client = mqtt.Client()
client.username_pw_set(MQTT_USERNAME, MQTT_KEY)

client.on_connect = on_connect
client.on_message = on_message
client.on_publish = on_publish
client.on_disconnect = on_disconnect

print(f"Connexion à {MQTT_SERVER}...")
try:
    client.connect(MQTT_SERVER, MQTT_PORT, 60)
    print(f"\nEn attente de messages...")
    client.loop_forever()
    
except KeyboardInterrupt:
    print("\n\nArrêt du programme...")
    client.disconnect()
except Exception as e:
    print(f"Erreur: {e}")
