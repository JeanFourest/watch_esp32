# 🔔 Projet Notification ESP32 + Raspberry Pi

Ce projet affiche sur un écran **LCD RGB Grove** les **notifications reçues sur votre téléphone** (Android ou iPhone), en temps réel, grâce à un serveur hébergé sur un **Raspberry Pi** et un module **ESP32**.

## 📦 Contenu du projet

- 🔌 Branchement d'un écran LCD, LED, buzzer et bouton sur un ESP32
- 🌐 Serveur Node.js qui reçoit les notifications
- 📱 Téléphone qui envoie les notifications (MacroDroid sur Android ou Raccourcis Apple)
- 🖥️ ESP32 qui interroge le serveur et affiche les infos

---

## 🧠 Fonctionnement général

1. Le **téléphone** envoie les notifications à un **serveur Node.js** sur le Raspberry Pi.
2. L'**ESP32** récupère la dernière notification toutes les 500ms.
3. L'**écran LCD** affiche soit la **notification** soit **l'heure**, selon le mode actif.
4. La **LED** s'allume pour WhatsApp/Discord, et un **buzzer** sonne brièvement.
5. Un **bouton** permet de **switcher entre les deux modes** : `notification` ↔ `horloge`.

---

## 🖥️ Installation du serveur sur Raspberry Pi

### 🔐 Connexion SSH

```bash
ssh pi@192.168.X.X
```

(Remplacez `192.168.X.X` par l'adresse IP de votre Raspberry Pi)

### ⚙️ Installation de Node.js

```bash
sudo apt update
sudo apt install nodejs npm -y
```

### 📝 Création du fichier serveur

```bash
mkdir notif-server
cd notif-server
nano server.js
```

Copiez-collez ce code :

```js
const express = require('express');
const app = express();
const port = 5000;

let lastNotification = { app: 'Aucune', message: 'Aucune notification' };

app.use(express.json());

app.post('/notification', (req, res) => {
  const { message, app } = req.body;
  if (!message && !app) {
    return res.status(400).json({ status: 'erreur', message: 'Données manquantes' });
  }
  lastNotification = { app, message };
  console.log(`🔔 [${app}] ${message}`);
  res.status(200).json();
});

app.get('/notification', (req, res) => {
  res.json(lastNotification);
});

app.listen(port, () => {
  console.log(`🚀 Serveur actif sur http://localhost:${port}`);
});
```

### 🚀 Lancement du serveur

```bash
npm init -y
npm install express
node server.js
```

---

## 📲 Configuration du téléphone

### ✅ Android (MacroDroid)

- **Application** : [MacroDroid sur Play Store](https://play.google.com/store/apps/details?id=com.arlosoft.macrodroid)
- **Macro à créer** : `Notification vers ESP32`

#### Déclencheur :
- **Notification reçue** (toutes applications ou ciblées)

#### Action :
- **Requête HTTP**
  - URL : `http://192.168.X.X:5000/notification`
  - Type : `POST`
  - Contenu JSON :
    ```json
    {
      "app": "{not_app_name}",
      "message": "{notification}"
    }
    ```

---

### 🍏 iPhone (ShortCut)

#### Étapes :
1. Créer un raccourci "Envoyer Notification"
2. Action 1 : Récupérer `Nom de l'app` et `Texte de la notification`
3. Action 2 : Faire une requête HTTP POST vers :
   - `http://192.168.X.X:5000/notification`
   - Corps :
     ```json
     {
       "app": "App Name",
       "message": "Message"
     }
     ```
---

## 🔌 Schéma de branchements

| Composant         | ESP32 Pin    |
|-------------------|--------------|
| LCD RGB Grove     | D21/D22      |
| Bouton Grove       | D2          |
| LED               | D18          |
| Buzzer            | D25          |

### 📸 Photos du montage

> 📷 **Ajoutez ici vos photos du câblage et du montage physique.**
>  
> Par exemple :
> - `montage.jpg`
> - `esp32_lcd_buzzer.jpg`

*(Vous pouvez intégrer vos images dans GitHub ou les lier en ligne)*

---

## 📚 Librairies utilisées (Arduino)

- `WiFi.h`
- `HTTPClient.h`
- `ArduinoJson.h`
- `time.h`
- `rgb_lcd.h` (Librairie officielle Seeed Studio Grove)

---

## 🔐 Configuration Wi-Fi et serveur dans le code ESP32

Pour connecter votre ESP32 au réseau Wi-Fi et interroger le serveur Node.js hébergé sur le Raspberry Pi, il faut définir dans le code les informations suivantes :

```cpp
const char* ssid = "VOTRE_RESEAU_WIFI";
const char* password = "VOTRE_MOT_DE_PASSE";
const char* serverName = "http://192.168.X.X:5000/notification";
```

### 📋 Variables à modifier :

- **`ssid`** : le nom du réseau Wi-Fi auquel l'ESP32 doit se connecter
- **`password`** : le mot de passe du réseau Wi-Fi
- **`serverName`** : l'URL complète du serveur qui reçoit les notifications (adresse IP locale du Raspberry Pi avec le port et la route `/notification`)

### ⚠️ Important

- Remplacez ces valeurs par vos propres informations réseau et l'adresse IP de votre Raspberry Pi
- Cette configuration est essentielle pour que l'ESP32 puisse se connecter à votre réseau local et récupérer les notifications via HTTP
- Assurez-vous que l'ESP32 et le Raspberry Pi sont sur le même réseau local

---

## ✅ Résultat attendu

- Une notification arrive → s'affiche sur l'écran avec l'app et le message
- Si le message > 16 caractères, il défile automatiquement
- La LED s'allume si app = WhatsApp ou Discord
- Le buzzer émet un bip
- Le bouton permet de switcher entre :
  - **🔔 Mode notification**
  - **🕒 Mode horloge synchronisée (NTP)**
