#include <Arduino.h>
#include <ArduinoOTA.h>
#include "WiFi.h"
  #include <InfluxDbClient.h>
  #include <InfluxDbCloud.h>

/////////////////////////////////////////////////////////////////////////// Schleifen verwalten
unsigned long previousMillis_ota = 0; // OTA aufrufen
unsigned long interval_ota = 500; 

unsigned long previousMillis_fluxdb = 0; // Fluxdb schreiben
unsigned long interval_fluxdb = 2500; 

/////////////////////////////////////////////////////////////////////////// InfluxDB Setup
  #define INFLUXDB_URL "http://192.168.1.129:8086"
  #define INFLUXDB_TOKEN "KnjcF3qrD45EK3oFvI8OtCrSZ-vTckyASGBH62IArxGfOMBtaS4EEdD51zbhmOm-6l7tnQRI46prTwDdMFS76g=="
  #define INFLUXDB_ORG "1a413d9db39d66ff"
  #define INFLUXDB_BUCKET "growatt"

  // Time zone info
  #define TZ_INFO "UTC1"


//////////////////////////////////////////////////////////////////////////  Influxdb2
  // Declare InfluxDB client instance with preconfigured InfluxCloud certificate
  InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

  // Declare Data point
  Point esp32_wifi("ESP32-555");
  Point wechselrichter("Growatt_Wechselrichter_L1");

/////////////////////////////////////////////////////////////////////////// Funktionsprototypen
void loop                       ();
void wifi_setup                 ();
void OTA_update                 ();
void fluxdb_schreiben           ();

/////////////////////////////////////////////////////////////////////////// SETUP - OTA Update
void OTA_update(){

 ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";
      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
  ArduinoOTA.begin();

}

/////////////////////////////////////////////////////////////////////////// SETUP - Wifi
void wifi_setup() {

// WiFi Zugangsdaten
const char* WIFI_SSID = "GuggenbergerLinux";
const char* WIFI_PASS = "Isabelle2014samira";

// Static IP
IPAddress local_IP(192, 168, 55, 42);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 0, 0, 0);  
IPAddress dns(192, 168, 1, 1); 

// Verbindung zu SSID
Serial.print("Verbindung zu SSID - ");
Serial.println(WIFI_SSID); 

// IP zuweisen
if (!WiFi.config(local_IP, gateway, subnet, dns)) {
   Serial.println("STA fehlerhaft!");
  }

// WiFI Modus setzen
WiFi.mode(WIFI_OFF);
WiFi.disconnect();
delay(100);

WiFi.begin(WIFI_SSID, WIFI_PASS);
Serial.println("Verbindung aufbauen ...");

while (WiFi.status() != WL_CONNECTED) {

  if (WiFi.status() == WL_CONNECT_FAILED) {
     Serial.println("Keine Verbindung zum SSID möglich : ");
     Serial.println();
     Serial.print("SSID: ");
     Serial.println(WIFI_SSID);
     Serial.print("Passwort: ");
     Serial.println(WIFI_PASS);
     Serial.println();
    }
  delay(2000);
}
    Serial.println("");
    Serial.println("Mit Wifi verbunden");
    Serial.println("IP Adresse: ");
    Serial.println(WiFi.localIP());


    // Accurate time is necessary for certificate validation and writing in batches
    // We use the NTP servers in your area as provided by: https://www.pool.ntp.org/zone/
    // Syncing progress and the time will be printed to Serial.
    timeSync(TZ_INFO, "pool.ntp.org");


    // InfluxDB

    // Add tags to the data point
    esp32_wifi.addTag("SSID", WIFI_SSID);


        // Check server connection
    if (client.validateConnection()) {
      Serial.print("Connected to InfluxDB: ");
      Serial.println(client.getServerUrl());
    } else {
      Serial.print("InfluxDB connection failed: ");
      Serial.println(client.getLastErrorMessage());
    }    

}

/////////////////////////////////////////////////////////////////////////// SETUP
void setup() {

// Serielle Kommunikation starten
Serial.begin(115200);

// Wifi setup
wifi_setup();

// OTA update 
OTA_update();

}

/////////////////////////////////////////////////////////////////////////// Fluxdb schreiben
void fluxdb_schreiben() {

   // Löschen die Felder, um den Punkt wiederzuverwenden. Die Tags bleiben die gleichen.
    wechselrichter.clearFields();
    esp32_wifi.clearFields();
  
    // Store measured value into point
    // Report RSSI of currently connected network
  float h = 89.4;
  float t = 25.8;

  wechselrichter.addField("humidity", h);
  wechselrichter.addField("temperature", t);
  wechselrichter.addField("Millis", millis());
  
    // Schreibe Protokol seriell
    Serial.print("Schreibe fluxdb -> ");
    Serial.println(wechselrichter.toLineProtocol());
  
    // Punkte schreiben und bei Fehler ERROR ausgaben
    if (!client.writePoint(wechselrichter)) {
      Serial.print("ERROR!!! -------------- > InfluxDB write failed: ");
      Serial.println(client.getLastErrorMessage());
    }
  
}

/////////////////////////////////////////////////////////////////////////// LOOP
void loop() {

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ OTA aufrufen
  if (millis() - previousMillis_ota > interval_ota) {
      previousMillis_ota = millis(); 
      // OTA aufrufen
      ArduinoOTA.handle();  
    }   

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ Fluxdb aufrufen
  if (millis() - previousMillis_fluxdb > interval_fluxdb) {
      previousMillis_fluxdb = millis(); 
      fluxdb_schreiben();
    }     


    }