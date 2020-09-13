#include "DHT.h"
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

// Temp Sensor
#define DHTPIN 4     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

DHT dht(DHTPIN, DHTTYPE);

// NTPClient
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
String ntpDateTime;
String ntpDate;
String ntpTime;

// Connect to WIFI
#ifndef STASSID
#define STASSID "xxxxx"
#define STAPSK  "xxxxx"
#endif

const char* ssid     = STASSID;
const char* password = STAPSK;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Temp Dash Sensor...");

  
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("");
  Serial.println("Starting DHT Sensor...");
  dht.begin();

  Serial.println("");
  Serial.println("Starting NTPClient...");
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  //timeClient.setTimeOffset(-14400);
}

void loop() {
  timeClient.update();
  
  // Wait a few seconds between measurements.
  delay(10000);

  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  ntpDateTime = timeClient.getFormattedDate();

  Serial.print("DATE: ");
  Serial.println(ntpDateTime);
  
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(f);
  Serial.print(F("°F  Heat index: "));
  Serial.print(hif);
  Serial.println(F("°F"));

  HTTPClient http;

  http.begin("http://10.0.1.4:8080/temp"); 
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<200> doc;
  doc["sensorName"] = "ESP8266";
  doc["tempF"] = f;
  doc["humidity"] = h;
  doc["heatIndex"] = hif;
  doc["readingDate"] = ntpDateTime;
  //serializeJson(doc,Serial);
  char json_str[200];
  serializeJsonPretty(doc, Serial);
  serializeJsonPretty(doc, json_str);
  int httpResponseCode = http.POST(json_str);
  if (httpResponseCode > 0)
  {
    String response = http.getString();
    Serial.println(httpResponseCode);
    Serial.println(response);
  }
  else
  {
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}
