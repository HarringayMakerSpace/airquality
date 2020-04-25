/*
 * An Air Quality Monitoring IoT Device
 * 
 * Monitors: 
 *  - PM1 and PM2.5 particle concentration with a PMS7003 sensor
 *  - temperature and humidity with an SHT31. 
 * Runs on an ESP8266 MCU for Wifi connectivity
 * Publishes to Thingspeak for convenient and free storage 
 *  
 * Libraries used:
 * PMS - https://github.com/fu-hsi/pms - v1.1.0
 * SHT3x - Adafruit's SHT3x - v1.1.7
 *  
 * Author: (torntrousers) Anthony Elder 
 * License: MIT
 */
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "PMS.h"
#include "Adafruit_SHT31.h"

// **** configure these values with your steup and credentials ***
const char* thingSpeakKey = "N3MQ9PDUQY0RRAT9";
const char* ssid = "BTHub6-RX2F";
const char* password = "u3CA3bteMr4t";

#define publishIntervalMins  1 // how often reading sent to Thingspeak

// ****              configure the above values                ***

#define sampleIntervalMs  3000

//int tempSamples[publishIntervalMins * 60 / (sampleIntervalMs / 1000)];
int tempSamples[20];


PMS pms(Serial);
PMS::DATA data;

Adafruit_SHT31 sht31 = Adafruit_SHT31();

String temp; 
String humidity = "3"; 
String pm1 = "5"; 
String pm25 = "9.5"; 


void setup() {
  Serial.begin(115200); Serial.println();
  Serial.println("setup - airqaulity");  
  delay(5000);
  Serial.println("setup - airqaulity2");  
  initWifi();

//  Serial.println("Init SHT3x");
//  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
//    Serial.println("Couldn't find SHT31");
////    while (1) delay(1);
//  }

  
}

long lastPublish;
long lastSample;
int samples = 0;

void loop() {

  if (millis()-lastSample > sampleIntervalMs) {
    takeSample();
    lastSample = millis();
  }

  if (millis()-lastPublish > publishIntervalMins*60*1000) {
    avgSamples();
    sendReading(); 
    lastPublish = millis();
  }
  
}

void avgSamples() {
  int t = 0;
  for (int i=0; i<samples; i++) {
    t += tempSamples[i];
    Serial.print(tempSamples[i]); Serial.println(" ");
  }
  temp = String((float)t / samples, 1);
  Serial.print("Avg temp: "); Serial.println(temp) ;
  samples = 0;
}

void takeSample() {
  tempSamples[samples++] = samples;
}

void sendReading() {
  String url = String("http://api.thingspeak.com/update?api_key=") + thingSpeakKey +
               "&field1=" + temp +
               "&field2=" + humidity + 
               "&field3=" + pm1 + 
               "&field4=" + pm25; 

  Serial.print("Publishing: "); Serial.println(url);
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  Serial.print("response httpCode: ");  Serial.println(httpCode);
  http.end();
}


void initWifi() {
  Serial.print("Wifi connecting to "); Serial.print(ssid); Serial.print("...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.waitForConnectResult();
  Serial.print(", connected. IP: "); Serial.println(WiFi.localIP());
}
