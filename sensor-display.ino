#include "DHT.h"
#define DHTPIN 2
#define DHTTYPE DHT11

#define LEDPIN 12


#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>

#include <UUID.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#include "secrets.h" // please, create this file and put all wifi and aws secrets in order to connect to lambda function
//OR
// #define AUTH "Basic cafe123456=="
// #define WIFI_NAME "MY_WIFI"
// #define WIFI_PWD "my-super-secret"
// #define LAMBDA_SERVER "https://lambda-wpto.ws/table...""

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
const char *server = LAMBDA_SERVER;

boolean connectWifi() {
  // Let us connect to WiFi
  digitalWrite(LEDPIN, HIGH);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_NAME, WIFI_PWD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(2000);
    Serial.print(".");
  }
  Serial.println(".......");
  Serial.println("WiFi Connected....IP Address:");
  Serial.println(WiFi.localIP());
  digitalWrite(LEDPIN, LOW);
  return true;
}

DHT dht(DHTPIN, DHTTYPE);

boolean ledOn = false;

void setup() {
  Serial.begin(9600);
  pinMode(LEDPIN, OUTPUT);
  connectWifi();
  dht.begin();
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  

  
}

void loop() {
  showText();
}

int pos = 0;
int up = 0;

int aMinute = 0;

void showText() {
 
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println(F("Failed to read from DHT sensor!"));
  }
  
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(1000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  
  if(!isnan(temperature))
    display.print(temperature,1);
  else
    display.print(F("X")); 
  display.println(F("\370 C"));
  
  display.setCursor(0, 16);
  if(!isnan(humidity))
    display.print(humidity,0);
  else
    display.print(F("X"));  
  display.println(F("% umidade"));

  if(up == 0) {
    if(pos++ == 5)
      up = 1;
  } else {
    if(pos-- == 1)
      up = 0;
  }

  display.drawPixel(127, pos * 5, SSD1306_WHITE);
  
  display.display();
//  if(ledOn) {
//    digitalWrite(LEDPIN, HIGH);
//    ledOn = false;
//  } else {
//    digitalWrite(LEDPIN, LOW);
//    ledOn = true;
//  }
  
  delay(2000);
  aMinute += 2000;
  if(aMinute > 300000) {
    connectAndSend(humidity, temperature);
    aMinute = 0;
  }
  
}

UUID uuid;

void connectAndSend(float humidityPercentage, float tempCelsius) {
  digitalWrite(LEDPIN, HIGH);
  HTTPClient http;
  Serial.println("Connecting to the HTTP server....");
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  client->setInsecure();

  String json = "\{\"TableName\": \"sensors\", \"Item\": {\"temperatura\": ";
  json += tempCelsius;
  json += ",\"umidade\":";
  json += humidityPercentage;
  json += "}}";
  
  char apiURL[1024];
  sprintf(apiURL, "%s", server);
  
  Serial.printf("API URL=%s\r\n<<<%s>>>\r\n", apiURL, json.c_str());
  if (http.begin(*client, apiURL)) {
    Serial.println("Connected");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", AUTH);
    int code = http.POST(json);
    Serial.printf("HTTP Code [%d]", code);
    if (code > 0) {
      if (code == HTTP_CODE_OK || code == HTTP_CODE_MOVED_PERMANENTLY) {
        Serial.println("GET OK");
        String payload = http.getString();
        Serial.println(payload);
        Serial.println("...JSON..");
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s", http.errorToString(code).c_str());
    }
  }
  http.end();
  digitalWrite(LEDPIN, LOW);
}
