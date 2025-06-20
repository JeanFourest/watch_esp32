#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include "rgb_lcd.h"

const char* ssid = "VOTRE_RESEAU_WIFI";
const char* password = "VOTRE_MOT_DE_PASSE";
const char* serverName = "http://192.168.X.X:5000/notification";
const int buttonPin = 2;
const int buzzerPin = 25; 
const int ledPin = 18;

rgb_lcd lcd;

String lastMessage = "";
String lastApp = "";
bool modeHeure = false;
bool lastButtonState = HIGH;
bool buttonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 20;

int scrollPosition = 0;
unsigned long lastScrollTime = 0;
unsigned long scrollDelay = 300;
bool isScrolling = false;
unsigned long scrollStartTime = 0;
unsigned long initialDisplayTime = 2000;

String lastTimeString = "";
String lastDateString = "";
unsigned long lastTimeUpdate = 0;
unsigned long timeUpdateInterval = 1000;

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

void setup() {
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  Serial.begin(115200);
  lcd.begin(16, 2);
  lcd.setRGB(0, 0, 255);
  lcd.print("Connexion WiFi...");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  lcd.clear();
  lcd.setRGB(0, 255, 0);
  lcd.print("Connecte !");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Synchronisation de l'heure...");

  delay(100);
  lcd.clear();
  afficherModeActuel();
}

void afficherModeActuel() {
  if (modeHeure) {
    lcd.setRGB(0, 255, 255);
  } else {
    lcd.setRGB(0, 255, 0);
  }
}

void afficherHeure() {
  unsigned long currentTime = millis();
  if (currentTime - lastTimeUpdate >= timeUpdateInterval) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      lcd.clear();
      lcd.setRGB(255, 128, 0);
      lcd.print("Erreur temps");
      return;
    }

    char timeString[17];
    char dateString[17];
    strftime(timeString, sizeof(timeString), "%H:%M:%S", &timeinfo);
    strftime(dateString, sizeof(dateString), "%d/%m/%Y", &timeinfo);

    String currentTimeStr = String(timeString);
    String currentDateStr = String(dateString);

    if (currentTimeStr != lastTimeString || currentDateStr != lastDateString) {
      lcd.setCursor(0, 0);
      lcd.print("                ");
      lcd.setCursor(0, 0);
      lcd.print(currentTimeStr);
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print(currentDateStr);

      lastTimeString = currentTimeStr;
      lastDateString = currentDateStr;
    }

    lastTimeUpdate = currentTime;
  }
}

void afficherNotification() {
  if (lastMessage != "" && lastApp != "") {
    lcd.setRGB(255, 255, 255);
    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 0);
    lcd.print("[" + lastApp + "]");

    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    if (lastMessage.length() <= 16) {
      lcd.print(lastMessage);
      isScrolling = false;
    } else {
      lcd.print(lastMessage.substring(0, 16));
      isScrolling = false;
      scrollStartTime = millis();
      scrollPosition = 0;
    }
  }
}

void gererBouton() {
  int reading = digitalRead(buttonPin);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
        modeHeure = !modeHeure;
        Serial.println(modeHeure ? "Mode heure activé" : "Mode notification activé");

        isScrolling = false;
        scrollPosition = 0;

        lastTimeString = "";
        lastDateString = "";
        lastTimeUpdate = 0;

        afficherModeActuel();

        if (modeHeure) {
          lcd.clear();
          afficherHeure();
        } else {
          afficherNotification();
        }
      }
    }
  }

  lastButtonState = reading;
}

void gererDefilementNotification() {
  if (lastMessage.length() > 16 && !modeHeure) {
    unsigned long currentTime = millis();

    if (!isScrolling) {
      if (currentTime - scrollStartTime >= initialDisplayTime) {
        isScrolling = true;
        scrollPosition = 0;
        lastScrollTime = currentTime;
      }
    } else {
      if (currentTime - lastScrollTime >= scrollDelay) {
        scrollPosition++;
        if (scrollPosition > lastMessage.length() - 16) {
          scrollPosition = 0;
        }

        lcd.setCursor(0, 1);
        lcd.print(lastMessage.substring(scrollPosition, scrollPosition + 16));
        lastScrollTime = currentTime;
      }
    }
  }
}

void gererNotifications() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName);
    int httpCode = http.GET();

    if (httpCode == 200) {
      String payload = http.getString();
      StaticJsonDocument<256> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        String app = doc["app"] | "Inconnue";
        String msg = doc["message"] | "Aucun message";

        bool isNew = (msg != lastMessage || app != lastApp);

        // Allumer la LED uniquement pour Discord ou WhatsApp
        if (app == "Discord" || app == "WhatsApp") {
          digitalWrite(ledPin, HIGH);
        } else {
          digitalWrite(ledPin, LOW);
        }

        if (isNew) {
          lastMessage = msg;
          lastApp = app;

          Serial.println("Nouvelle notif :");
          Serial.println("[" + app + "] " + msg);

          afficherNotification();

          // Buzzer sonne 200ms
          digitalWrite(buzzerPin, HIGH);
          delay(200);
          digitalWrite(buzzerPin, LOW);
        }
      } else {
        lcd.setRGB(255, 128, 0);
        lcd.setCursor(0, 0);
        lcd.print("Erreur JSON     ");
        Serial.println("Erreur JSON: " + String(error.c_str()));
      }
    } else {
      lcd.setRGB(255, 0, 0);
      lcd.setCursor(0, 0);
      lcd.print("Erreur HTTP     ");
      Serial.println("Code HTTP : " + String(httpCode));
    }

    http.end();
  }
}

void loop() {
  gererBouton();

  if (modeHeure) {
    afficherHeure();
    delay(10);
  } else {
    static unsigned long lastNotificationCheck = 0;
    unsigned long currentTime = millis();

    if (currentTime - lastNotificationCheck >= 500) {
      gererNotifications();
      lastNotificationCheck = currentTime;
    }

    gererDefilementNotification();
    delay(10);
  }
}