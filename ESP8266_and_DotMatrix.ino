#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <WiFiClient.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN   14
#define DATA_PIN  13
#define CS_PIN    2

// Displaying Options
boolean showClock = true;
boolean showWeather = true;
boolean showCovidCounter = true;
boolean showCurrency = true;
uint8_t dotMatrixIndensity = 2;

// Time Options
uint8_t diplayingTimeClock = 15; // seconds
const long timeZone = 3; // default is 3 (+3)

// Weather Options
String weatherCity = "";
String weatherAPIKey = "";

// Currency Options
String base = "USD";
String baseTo = "TRY";

// Wi-Fi Credentials
char *ssid = "";
char *pass = "";

// HTTPS URLS
String covid19URL = "https://api.covid19api.com/summary";
String currencyURL = "https://api.exchangeratesapi.io/latest?base=" + base;

// HTTP URLS
String weatherURL = "http://api.openweathermap.org/data/2.5/weather?q=" + weatherCity + "&appid=" + weatherAPIKey;

WiFiUDP ntpUDP;
ESP8266WiFiMulti WiFiMulti;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600 * timeZone);
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
String currentTime = "";
boolean blinkClock;

void setup() {
  Serial.begin(115200);
  
  P.begin();
  P.displayText("", PA_CENTER, 0, 0, PA_SCROLL_DOWN, PA_SCROLL_DOWN);
  P.setIntensity(dotMatrixIndensity);
  P.print("Wi-Fi");

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid, pass);

  timeClient.begin();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }
}

void loop() {
  if (showClock) {
    for (uint8_t counter = 0; counter < diplayingTimeClock; counter++) {
      printClock();
    }
  }

  if (showWeather) {
    sendHTTPRequest(weatherURL);
  }

  if (showCovidCounter) {
    sendHTTPSRequest(covid19URL);
  }

  if (showCurrency) {
    sendHTTPSRequest(currencyURL);
  }
}

void sendHTTPRequest(String url) {
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    WiFiClient client;
    HTTPClient http;

    if (http.begin(client, url)) {
      int httpCode = http.GET();

      if (httpCode > 0) {

        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString();

          DynamicJsonDocument doc(1024);
          deserializeJson(doc, payload);

          if (url == weatherURL) {
            printWeather(doc);
          }
        }
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
    } else {
      Serial.printf("[HTTP} Unable to connect\n");
    }
  }
}

void sendHTTPSRequest(String url) {
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
    client->setInsecure();
    HTTPClient https;

    if (https.begin(*client, url)) {
      int httpCode = https.GET();
      if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = https.getString();

          DynamicJsonDocument doc(1024);
          deserializeJson(doc, payload);

          if (url == covid19URL) {
            printCovid19(doc);
          } else if (url == currencyURL) {
            printCurrency(doc);
          }
        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }
  }
}

void printWeather(DynamicJsonDocument doc) {
  JsonObject main = doc["main"];
  float main_temp = main["temp"];
  main_temp = main_temp - 273.15;
  int temp = floor(main_temp);
  String total;
  total += temp ;
  total += " C";
  P.print("Hava");
  delay(1000);
  P.print(total);
  delay(3000);
}

void printCovid19(DynamicJsonDocument doc) {
  JsonObject Global = doc["Global"];
  int Global_NewDeaths = Global["NewDeaths"];
  Serial.println(Global_NewDeaths);
  P.print("Cov-19");
  delay(1000);
  P.print(Global_NewDeaths);
  delay(3000);
}

void printClock() {
  timeClient.update();

  // HH
  if (timeClient.getHours() < 10) {
    currentTime = "0";
    currentTime += timeClient.getHours();
  } else {
    currentTime = timeClient.getHours();
  }

  // HH:
  if (!blinkClock) {
    currentTime += "   ";
    blinkClock = true;
  } else {
    currentTime += " : ";
    blinkClock = false;
  }

  // HH:MM
  if (timeClient.getMinutes() < 10) {
    currentTime += "0";
    currentTime += timeClient.getMinutes();
  } else {
    currentTime += timeClient.getMinutes();
  }

  P.print(currentTime);
  delay(1000);
}

void printCurrency(DynamicJsonDocument doc) {
  JsonObject rates = doc["rates"];
  float rate = rates[baseTo];
  P.print("1 " + base);
  delay(1000);
  P.print(rate);
  delay(3000);
}
