#include <Arduino.h>

#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>

// rc522----------------------------------------------------------------------
/* wiring the MFRC522 to ESP8266 (ESP-12)
RST     = GPIO0
SDA(SS) = GPIO4
MOSI    = GPIO13
MISO    = GPIO12
SCK     = GPIO14
GND     = GND
3.3V    = 3.3V
*/
/* LCD TO ESSP8266
SDA = GPIO4
SCL = GPIO5
*/
#define RST_PIN 0 // RST-PIN für RC522 - RFID - SPI - Modul GPIO5
#define SS_PIN 2  // SDA-PIN für RC522 - RFID - SPI - Modul GPIO4

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance
// Helper routine to dump a byte array as hex values to Serial
int startUID = 0;
String UID = "";

String dump_byte_array(byte *buffer, byte bufferSize)
{
  String Id;
  for (byte i = 0; i < bufferSize; i++)
  {
    Id = Id + (buffer[i] < 0x10 ? "0" : "");
    Id = Id + (buffer[i] + HEX);
  }
  return Id;
}

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "NTP"
#define WIFI_PASSWORD "01112002"

// Insert Firebase project API Key
#define API_KEY "AIzaSyAAJiIK1ddvIbu5bNXjVB1bCUlRzVf0IQw"

// Insert Authorized Username and Corresponding Password
#define USER_EMAIL "tuanphong01112002@gmail.com"
#define USER_PASSWORD "01112002"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://health-monitoring-c53df-default-rtdb.firebaseio.com/"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
// Variable to save USER UID
String uid;
// Variables to save database paths
String databasePath;
// Parent Node (to be updated in every loop)
String parentPath;

FirebaseJson json;
// Initialize WiFi
void initWiFi()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

void initDataBase()
{
  //------------------------------------------------------------------------------------------
  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;
  //------------------------------------------------------------------------------------------
  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  //------------------------------------------------------------------------------------------
  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "")
  {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);
}

void initLCD()
{
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  Serial.println(F("SSD1306 allocation succes"));
  display.setTextSize(1);
  display.setTextColor(WHITE);
}

void setup()
{
  // put your setup code here, to run once:
  pinMode(16, OUTPUT);

  Serial.begin(9600);
  Serial.print(F("beggin"));
  initLCD();
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Connecting to WiFi ..");
  display.println("SSID: NTP");
  display.println("Pass: 01112002");
  display.display();
  initWiFi();
  initDataBase();

  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522
}

void loop()
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("READY TO SCAN ..");
  display.display();
  if (!startUID)
  {
    if (!mfrc522.PICC_IsNewCardPresent())
    {
      delay(50);
      return;
    }
    if (!mfrc522.PICC_ReadCardSerial())
    {
      delay(50);
      return;
    }
    //-----------------------------------------------------------------------------------------------
    Serial.print(F("Card UID:"));
    UID = dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println(UID);
    digitalWrite(16, 1);
    delay(500);
    digitalWrite(16, 0);
    startUID = 1;
  }

  if (Firebase.ready() && (startUID == 1))
  {
    Serial.print(F("Send to database:"));
    parentPath = databasePath;
    json.set("mode", "on");
    json.set("id", UID);
    databasePath = "/data/" + UID;
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("UID CARD");
    display.println(UID);
    display.display();
    Serial.println(parentPath);
    Serial.printf("Set json... %s\n", Firebase.RTDB.updateNode(&fbdo, databasePath.c_str(), &json) ? "on" : fbdo.errorReason().c_str());
    // databasePath = "/data/" + UID + "/id";
    // Serial.printf("Set json... %s\n", Firebase.RTDB.setString(&fbdo, parentPath.c_str(), UID) ? "on" : fbdo.errorReason().c_str());
    startUID = 0;
  }
}
