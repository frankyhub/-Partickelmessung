/*************************************************************************************************
                                      PROGRAMMINFO
**************************************************************************************************
Funktion: LoRa WAN WEB-Server SDS011 A169 COM6
**************************************************************************************************
Version: 14.06.022
**************************************************************************************************
Board: TTGO Lora32-OLED V1
**************************************************************************************************
*PM2.5 steht beispielsweise für Partikel mit einem aerodynamischen Durchmesser von 2,5 Mikrometer oder weniger, PM10 für 10 Mikrometern

  SDS011 (Laser Dust/Air Quality Sensor Module)
  Specification:
  Measured values: PM2.5, PM10
  Range: 0 – 999.9 μg /m³
  Supply voltage: 5V (4.7 – 5.3V)
  Power consumption (work): 70mA±10mA
  Power consumption (sleep mode laser & fan): < 4mA
  Storage temperature: -20 to +60C
  Work temperature: -10 to +50C
  Humidity (storage): Max. 90%
  Humidity (work): Max. 70% (condensation of water vapor falsify readings)
  Accuracy:
  70% for 0.3μm
  98% for 0.5μm
  Size: 71x70x23 mm
  Certification: CE, FCC, RoHS
  Pinout:
  Pin 1 – not connected
  Pin 2 – PM2.5: 0-999μg/m³; PWM output
  Pin 3 – 5V
  Pin 4 – PM10: 0-999 μg/m³; PWM output
  Pin 5 – GND
  Pin 6 – RX UART (TTL) 3.3V
  Pin 7 – TX UART (TTL) 3.3V
**************************************************************************************************
Libraries:
https://github.com/espressif/arduino-esp32/tree/master/libraries
**************************************************************************************************
C++ Arduino IDE V1.8.19
**************************************************************************************************
Einstellungen:
https://dl.espressif.com/dl/package_esp32_index.json
http://dan.drown.org/stm32duino/package_STM32duino_index.json
http://arduino.esp8266.com/stable/package_esp8266com_index.json
**************************************************************************************************/
// Import Wi-Fi Library
#include <WiFi.h>
#include "ESPAsyncWebServer.h"

#include <SPIFFS.h>

//LoRa Libraries
#include <SPI.h>
#include <LoRa.h>

//OLED Display Libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//NTP Server Libraries
#include <NTPClient.h>
#include <WiFiUdp.h>

//Pins des LoRa-Transceiver-Moduls
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

//Europa Frequenz 866MHz
#define BAND 866E6

//TTGO OLED Pins
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_RST 16

/*HELTEC Pins
#define OLED_SDA 4
#define OLED_SCL 15 
#define OLED_RST 16
*/
#define SCREEN_WIDTH 128 // Breite OLED Display 128 Pixel
#define SCREEN_HEIGHT 64 // Höhe OLED Display 64 Pixel

// WiFi Zugang
const char* ssid     = "R2-D2";
const char* password = "xxx";

// NTP Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variablen fue Datum und Zeit
String formattedDate;
String day;
String hour;
String timestamp;


// Initialisiere LoRa Variablen
int rssi;
String loRaMessage;
String PM25Readin;
String PM10Readin;
String readingID;

// AsyncWebServer Port 80
AsyncWebServer server(80);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

// Ersetzt Platzhalter der PWM EIngaenge
/*Die Received Signal Strength Indication (RSSI) ist die empfangene Signalleistung 
 * in Milliwatt und wird in dBm gemessen. 
 */
String processor(const String& var){
  //Serial.println(var);
  if(var == "PM25"){
    return PM25Readin;
  }
    else if(var == "PM10"){
    return PM10Readin;
  }
  else if(var == "TIMESTAMP"){
    return timestamp;
  }
  else if (var == "RRSI"){
    return String(rssi);
  }
  return String();
}

//Initialisiere OLED Display
void startOLED(){
  //Reset OLED Display
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  //Initialisiere OLED Display
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Adresse 0x3C
    Serial.println(F("SSD1306-Initialisierung fehlgeschlagen"));
    for(;;); // Schleife beenden
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("LORA SENDER");
}

//Initialisiere LoRa Modul
void startLoRA(){
  int counter;
  //SPI LoRa Pins
  SPI.begin(SCK, MISO, MOSI, SS);
  //LoRa-Empfaenger-Modul einrichten
  LoRa.setPins(SS, RST, DIO0);

  while (!LoRa.begin(BAND) && counter < 10) {
    Serial.print(".");
    counter++;
    delay(500);
  }
  if (counter == 10) {
    // Erhöhe die Lese-ID bei jedem neuen Messwert
    Serial.println("LoRa Start fehlgeschlagen!"); 
  }
  Serial.println("LoRa Init. OK!");
  display.setCursor(0,10);
  display.clearDisplay();
  display.print("LoRa Init. OK!");
  display.display();
  delay(2000);
}

void connectWiFi(){
  // Verbindung dem Wi-Fi-Netzwerk mit SSID und Passwort
  Serial.print("Verbinde mit ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Lokale IP-Adresse ausgeben und Webserver starten
  Serial.println("");
  Serial.println("WiFi verbunden.");
  Serial.println("IP Adresse: ");
  Serial.println(WiFi.localIP());
  display.setCursor(0,25);
  display.print("Web-Server Adresse: ");
  display.setCursor(0,40);
  display.print(WiFi.localIP());
  display.display();
}

// Sensormesswerte vom LoRa-Paket lesen
void getLoRaData() {
  Serial.print("Lora-Paket empfangen: ");
  // Lese Packet
  while (LoRa.available()) {
    String LoRaData = LoRa.readString();
    // LoRaData-Format: LeseID/PM25&PM10#xx
    // String example: 1/2.5&10#xx
    Serial.print(LoRaData); 
    
    // Holen die ReadingID, PM25 und PM10
    int pos1 = LoRaData.indexOf('/');
    int pos2 = LoRaData.indexOf('&');
    int pos3 = LoRaData.indexOf('#');
    readingID = LoRaData.substring(0, pos1);
    PM25Readin = LoRaData.substring(pos1 +1, pos2);
    PM10Readin = LoRaData.substring(pos2 +1, pos3);
//    xx = LoRaData.substring(pos3+1, LoRaData.length());    
  }
  // Hole RSSI Received Signal Strength Indication
  rssi = LoRa.packetRssi();
  Serial.print(" Signalstärke: ");    
  Serial.println(rssi);
}

// Datum und Zeit vom NTPClient
void getTimeStamp() {
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  // Das formattedDate hat das folgendes Format:
  // 2018-05-28T16:00:13Z
  // Datum und Uhrzeit extrahieren
  formattedDate = timeClient.getFormattedDate();
  Serial.println(formattedDate);

  // Datum
  int splitT = formattedDate.indexOf("T");
  day = formattedDate.substring(0, splitT);
  Serial.println(day);
  // Zeit
  hour = formattedDate.substring(splitT+1, formattedDate.length()-1);
  Serial.println(hour);
  timestamp = day + " " + hour;
}

void setup() { 
  // Initialisiere Seriellen Monitor
  Serial.begin(115200);
  startOLED();
  startLoRA();
  connectWiFi();
  
  if(!SPIFFS.begin()){
    Serial.println("Beim Mounten von SPIFFS ist ein Fehler aufgetreten");
    return;
  }
  // Starte Index.html
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  server.on("/PM25", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", PM25Readin.c_str());
  });
  server.on("/PM10", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", PM10Readin.c_str());
  });
  server.on("/timestamp", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", timestamp.c_str());
  });
  server.on("/rssi", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(rssi).c_str());
  });
  server.on("/winter", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/winter.jpg", "image/jpg");
  });
  // Starte Server
  server.begin();
  
  // Initialisiere NTPClient
  timeClient.begin();
  // Zeit-Offset GMT +2 Stunden = 7200;
  timeClient.setTimeOffset(7200);
}

void loop() {
  // LoRa Pakete verfügbar?
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    getLoRaData();
    getTimeStamp();
  }
}
