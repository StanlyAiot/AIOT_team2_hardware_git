
#include <Wire.h>
#include <ESP8266WiFi.h>       //wifi library
#include <ESP8266HTTPClient.h>  //Stan==> for send data to jsp
#include <WiFiClient.h>        //Stan==> for send data to jsp

#include "MAX30100_PulseOximeter.h"
#include "Wire.h"
#include "Adafruit_GFX.h"
#include "OakOLED.h"

#define REPORTING_PERIOD_MS 1000

OakOLED oled;

PulseOximeter pox;


//const char ssid[] = "XZ2 sasa";
//const char pwd[] = "0920231219";
//const char ssid[] = "chtti_NC";
//const char pwd[] = "chtti@chtti";
const char ssid[] = "SASAT";
const char pwd[] = "0920231219";

//float BPM, SpO2;
int BPM, SpO2;       //stan_ fix sql error
uint32_t tsLastReport = 0;

const unsigned char bitmap [] PROGMEM =
{
  0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x18, 0x00, 0x0f, 0xe0, 0x7f, 0x00, 0x3f, 0xf9, 0xff, 0xc0,
  0x7f, 0xf9, 0xff, 0xc0, 0x7f, 0xff, 0xff, 0xe0, 0x7f, 0xff, 0xff, 0xe0, 0xff, 0xff, 0xff, 0xf0,
  0xff, 0xf7, 0xff, 0xf0, 0xff, 0xe7, 0xff, 0xf0, 0xff, 0xe7, 0xff, 0xf0, 0x7f, 0xdb, 0xff, 0xe0,
  0x7f, 0x9b, 0xff, 0xe0, 0x00, 0x3b, 0xc0, 0x00, 0x3f, 0xf9, 0x9f, 0xc0, 0x3f, 0xfd, 0xbf, 0xc0,
  0x1f, 0xfd, 0xbf, 0x80, 0x0f, 0xfd, 0x7f, 0x00, 0x07, 0xfe, 0x7e, 0x00, 0x03, 0xfe, 0xfc, 0x00,
  0x01, 0xff, 0xf8, 0x00, 0x00, 0xff, 0xf0, 0x00, 0x00, 0x7f, 0xe0, 0x00, 0x00, 0x3f, 0xc0, 0x00,
  0x00, 0x0f, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void onBeatDetected()
{
  Serial.println("Beat!");
  oled.drawBitmap( 60, 20, bitmap, 28, 28, 1);
  oled.display();
}

void setup()
{
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pwd);
  while (WiFi.status() != WL_CONNECTED) {    //stan__ if no connect to wifi it won't leave this loop.
    Serial.print("try agin");
    delay(500);
  }

  Serial.print("\nIP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("WiFi RSSI: ");
  Serial.println(WiFi.RSSI());

  oled.begin();
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(1);
  oled.setCursor(0, 0);

  oled.println("Initializing pulse oximeter..");
  oled.display();

  pinMode(16, OUTPUT);
  Serial.print("Initializing pulse oximeter..");



  if (!pox.begin()) {
    Serial.println("FAILED");
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(1);
    oled.setCursor(0, 0);
    oled.println("FAILED");
    oled.display();
    for (;;);
  } else {
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(1);
    oled.setCursor(0, 0);
    oled.println("SUCCESS");
    oled.display();
    Serial.println("SUCCESS");
  }


  // The default current for the IR LED is 50mA and it could be changed
  //   by uncommenting the following line. Check MAX30100_Registers.h for all the
  //   available options.
  //pox.setIRLedCurrent(MAX30100_LED_CURR_14_2MA);    //rate too high
  pox.setIRLedCurrent(MAX30100_LED_CURR_17_4MA);   //stan__best current

  // Register a callback for the beat detection
  pox.setOnBeatDetectedCallback(onBeatDetected);

}


void loop() {
  pox.update();

  BPM = pox.getHeartRate();
  SpO2 = pox.getSpO2();

  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {

    Serial.print("Heart rate:");
    Serial.print(BPM);
    Serial.print(" bpm / SpO2:");
    Serial.print(SpO2);
    Serial.println(" %");



    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(1);
    oled.setCursor(0, 16);
    oled.println(pox.getHeartRate());

    oled.setTextSize(1);
    oled.setTextColor(1);
    oled.setCursor(0, 0);
    oled.println("Heart BPM");

    oled.setTextSize(1);
    oled.setTextColor(1);
    oled.setCursor(0, 30);
    oled.println("Spo2");

    oled.setTextSize(1);
    oled.setTextColor(1);
    oled.setCursor(0, 45);
    oled.println(pox.getSpO2());
    oled.display();
    tsLastReport = millis();

  }
  servletGo(BPM, SpO2);       //stan___ call my function to send data to servlet
}


void servletGo(int BPM, int SpO2) {

  // REPLACE with your Domain name and URL path or IP address with path
  //const char* serverName = "http://192.168.10.12:8080/FinalProject/bpminsert";        //Stan==> for send data to jsp===============================================
  const char* serverName = "http://192.168.0.3:8080/FinalProject/bpminsert";

  //Check WiFi connection status
  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;

    // Your Domain name with URL path or IP address with path
    http.begin(serverName);

    // Specify content-type header
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");


    // Prepare your HTTP POST request data
    String httpRequestData = "Pulse_Rate=" + String(BPM) + "&SpO2=" + String(SpO2) + "&Patno=20";

    Serial.print("BPM: ");
    Serial.println(BPM);
    Serial.print("SpO2: ");
    Serial.println(SpO2);

    Serial.print("httpRequestData: ");
    Serial.println(httpRequestData);


    // Send HTTP POST request
    int httpResponseCode = http.POST(httpRequestData);


    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();

  }
  /*else {
    Serial.println("WiFi Disconnected");    //Stan=====> display no WiFi
    BPM, SpO2 = 0;

    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(1);
    oled.setCursor(30, 5);
    oled.println("Sorry, WiFi ");
    oled.setCursor(30, 15);
    oled.println("Disconnected ");
    oled.display();

    }*/
  //Send an HTTP POST request every 15 seconds
  //delay(15000);


}
