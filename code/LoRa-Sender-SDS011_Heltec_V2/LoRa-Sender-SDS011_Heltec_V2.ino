/*************************************************************************************************
                                      PROGRAMMINFO
**************************************************************************************************
Funktion: LORA Sender SDS011 (COM15)
**************************************************************************************************
Version: 14.06.2022
**************************************************************************************************
Board: HELTEC WLAN LORA 32 
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

//LoRa Libraries 
#include <SPI.h>
#include <LoRa.h>

//OLED Display Libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//PWM Sensoreingänge
int PM25 = 36;
int PM10 = 37;

float PM25Readin = 25;
float PM10Readin = 10;

//LoRa-Modul Pins
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

//Frequenz fuer Europa
#define BAND 866E6

//OLED Pins
#define OLED_SDA 4
#define OLED_SCL 15 
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED Breite
#define SCREEN_HEIGHT 64 // OLED Höhe

TwoWire I2Cone = TwoWire(1);

//Packet Counter
int readingID = 0;

int counter = 0;
String LoRaMessage = "";

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

//Initialisiere OLED Display
void startOLED(){
  //OLED Reset 
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  //Initialisiere OLED Display
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("LORA SENDER");
}

//Initialisiere LoRa
void startLoRA(){
  //SPI LoRa pins
  SPI.begin(SCK, MISO, MOSI, SS);
  //Setup LoRa Sende Modul
  LoRa.setPins(SS, RST, DIO0);

  while (!LoRa.begin(BAND) && counter < 10) {
    Serial.print(".");
    counter++;
    delay(500);
  }
  if (counter == 10) {
    // Erhöhe die Lese-ID für jede neue Lesung
    readingID++;
    Serial.println("Starting LoRa failed!"); 
  }
  Serial.println("LoRa-Initialisierung OK!");
  display.setCursor(0,10);
  display.clearDisplay();
  display.print("LoRa-Initialisierung OK!");
  display.display();
  delay(2000);
}

void getReadings(){
   PM25Readin = analogRead(36);
   PM25Readin = map(PM25Readin, 0, 4095, 0, 1000);
   PM10Readin = analogRead(37);
   PM10Readin = map(PM10Readin, 0, 4095, 0, 1000);
    
    //Kalibrierung:
    PM25Readin=PM25Readin/134,23;
    PM10Readin=PM10Readin/84,63;

   
   Serial.println("PM25Readin: "); 
   Serial.println(PM25Readin); 
   Serial.println("PM10Readin: "); 
   Serial.println(PM10Readin); 
}


void sendReadings() {
  LoRaMessage = String(readingID) + "/" + String(PM25Readin) + "&" + String(PM10Readin);
  //Send LoRa packet to receiver
  LoRa.beginPacket();
  LoRa.print(LoRaMessage);
  LoRa.endPacket();
  
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(1);
  display.print("LoRa Packet gesendet!");
  display.setCursor(0,20);
  display.print("PM25: ");
  display.setCursor(44,20);
  display.print(PM25Readin);
  display.setCursor(0,30);
  display.print("PM10: ");
  display.setCursor(44,30);
  display.print(PM10Readin);

  display.setCursor(0,40);
  display.print("Lese-ID:");
  display.setCursor(75,40);
  display.print(readingID);
  display.display();
  Serial.print("Paket senden: ");
  Serial.println(readingID);
  readingID++;
}

void setup() {
  //Intialisiere Serial Monitor
  Serial.begin(115200);
  startOLED();
  startLoRA();
}
void loop() {

  getReadings();
  sendReadings();
  delay(10000);
}
