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
#include "SoftwareSerial.h"

// **** configure these values with your steup and credentials ***
const char* thingSpeakKey = "<yourThingspeakChannelKey>";
const char* ssid = "<youWifiSSID>";
const char* password = "<yourifiPassword>";

#define publishIntervalMins  1 // how often reading sent to Thingspeak

// ****              configure the above values                ***

#define sampleIntervalMs  3000

//int tempSamples[publishIntervalMins * 60 / (sampleIntervalMs / 1000)];
float tempSamples[20];
float humSamples[20];
uint16_t pm1Samples[20];
uint16_t pm25Samples[20];
uint16_t pm10Samples[20];

SoftwareSerial pmsSerial(D4, -1);

PMS pms(pmsSerial);
PMS::DATA data;

Adafruit_SHT31 sht31 = Adafruit_SHT31();

String temp;
String humidity = "3";
String pm1 = "5";
String pm25 = "9.5";
String pm10 = "9.5";


void setup() {
  Serial.begin(115200); Serial.println();
  Serial.println("setup - airqaulity");

  initWifi();

  Serial.println("Init SHT3x");
  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
//    while (1) delay(1);
  }

  pmsSerial.begin(9600, SWSERIAL_8N1, D4, -1, false, 128, 11);

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
    if (avgSamples()) {
       sendReading();
    }
    lastPublish = millis();
  }

  delay(2000);
}

bool avgSamples() {
  Serial.println("Averaging readings");
  if (samples == 0) return false;
  float ta = 0;
  float ha = 0;
  int pm1a = 0;
  int pm25a = 0;
  int pm10a = 0;
  for (int i=0; i<samples; i++) {
    ta += tempSamples[i];
    ha += humSamples[i];
    pm1a += pm1Samples[i];
    pm25a += pm25Samples[i];
    pm10a += pm10Samples[i];
  }

  temp = String(ta / samples, 1);
  humidity = String(ha / samples, 0);
  pm1 = String(pm1a / samples);
  pm25 = String(pm25a / samples);
  pm10 = String(pm10a / samples);

  Serial.print("Avg temp: "); Serial.println(temp) ;
  Serial.print("Avg humidity: "); Serial.println(humidity) ;
  Serial.print("Avg PM1: "); Serial.println(pm1) ;
  Serial.print("Avg PM2.5: "); Serial.println(pm25) ;
  Serial.print("Avg PM10: "); Serial.println(pm10) ;

  samples = 0;
  return true;
}

void takeSample() {
  Serial.println("Taking reading");
  if (pms.readUntil(data, 3000)) {
    pm1Samples[samples] = data.PM_SP_UG_1_0;
    pm25Samples[samples] = data.PM_SP_UG_2_5;
    pm10Samples[samples] = data.PM_SP_UG_10_0;
  }

    tempSamples[samples] = sht31.readTemperature();
    humSamples[samples] = sht31.readHumidity();

    Serial.print("temp: "); Serial.println(tempSamples[samples]);
    Serial.print("humidity: "); Serial.println(humSamples[samples]);
    Serial.print("PM 1.0 (ug/m3): "); Serial.println(pm1Samples[samples]);
    Serial.print("PM 2.5 (ug/m3): "); Serial.println(pm25Samples[samples]);
    Serial.print("PM 10.0 (ug/m3): "); Serial.println(pm10Samples[samples]);

    samples = samples + 1;
//  }

}

void sendReading() {
  String url = String("http://api.thingspeak.com/update?api_key=") + thingSpeakKey +
               "&field1=" + temp +
               "&field2=" + humidity +
               "&field3=" + pm1 +
               "&field4=" + pm25 +
               "&field5=" + pm10;

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
  Serial.print("connected, IP: "); Serial.println(WiFi.localIP());
}
