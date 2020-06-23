
#include <Wire.h>
#include <ESP8266WiFi.h>       //wifi library
#include <ESP8266HTTPClient.h>  //Stan==> for send data to jsp
#include <WiFiClient.h>        //Stan==> for send data to jsp
#include "MAX30100_PulseOximeter.h"

#include <ESP8266WebServer.h>     //Stan_webserver

#include "Wire.h"
#include "Adafruit_GFX.h"
#include "OakOLED.h"
#define REPORTING_PERIOD_MS 1000


// Recommended settings for the MAX30100, DO NOT CHANGE!!!!,  refer to the datasheet for further info
//#define SAMPLING_RATE                       MAX30100_SAMPRATE_100HZ       // Max sample rate
//#define IR_LED_CURRENT                      MAX30100_LED_CURR_50MA        // The LEDs currents must be set to a level that
//#define IR_LED_CURRENT                      MAX30100_LED_CURR_17_4MA   //stan__best current
//#define RED_LED_CURRENT                     MAX30100_LED_CURR_27_1MA      // avoids clipping and maximises the dynamic range
#define PULSE_WIDTH                         MAX30100_SPC_PW_1600US_16BITS // The pulse width of the LEDs driving determines
#define HIGHRES_MODE                        true                          // the resolution of the ADC


// Create objects for the raw data from the sensor (used to make the trace) and the pulse and oxygen levels
MAX30100 sensor;            // Raw Data
PulseOximeter pox;          // Pulse and Oxygen

OakOLED oled;

ESP8266WebServer server(80);    //Stan_web object




//const char ssid[] = "XZ2 sasa";          //wifi  SSID & Password
//const char pwd[] = "0920231219";
//const char ssid[] = "chtti_NC";
//const char pwd[] = "chtti@chtti";
const char ssid[] = "SASAT";
const char pwd[] = "0920231219";

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
  //Serial.println("Beat!");
  oled.drawBitmap( 60, 20, bitmap, 28, 28, 1);
  oled.display();
}



void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pwd);
  while (WiFi.status() != WL_CONNECTED) {
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

  //pinMode(16, OUTPUT);
  Serial.print("Initializing pulse oximeter..");


  // Initialize the sensor. Failures are generally due to an improper I2C wiring, missing power supply
  // or wrong target chip. Occasionally fails on startup (very rare), just press reset on Arduino
  if (!sensor.begin()) {
    Serial.print("Could not initialise MAX30100");
    for (;;);                                               // End program in permanent loop
  }


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

  // Set up the parameters for the raw data object

  //sensor.setLedsCurrent(IR_LED_CURRENT, RED_LED_CURRENT);
  //sensor.setSamplingRate(SAMPLING_RATE);
  sensor.setLedsPulseWidth(PULSE_WIDTH);
  sensor.setMode(MAX30100_MODE_SPO2_HR);
  sensor.setHighresModeEnabled(HIGHRES_MODE);


  pox.setIRLedCurrent(MAX30100_LED_CURR_17_4MA);   //stan__best current


  // Register a callback for the beat detection
  pox.setOnBeatDetectedCallback(onBeatDetected);


}

void loop() {


  //int16_t Diff = 0;                           // The difference between the Infra Red (IR) and Red LED raw results
  uint16_t ir, red;                           // raw results returned in these
  /*
    static float lastx = 1;                     // Last x position of trace
    static int lasty = TRACE_MIDDLE_Y_POSITION; // Last y position of trace, default to middle
    static float x = 1;                         // current x position of trace
    int32_t y;                                  // current y position of trace
  */

  uint8_t BPM, SpO2;                            // BPM and O2 values
  static uint32_t tsLastReport = 0;           // Last time BMP/O2 were checked
  //static int32_t SensorOffset = 10000;        // Offset to lowest point that raw data does not go below, default 10000
  // Note that as sensors may be slightly different the code adjusts this
  // on the fly if the trace is off screen. The default was determined
  // By analysis of the raw data returned

  pox.update();                               // Request pulse and o2 data from sensor
  sensor.update();                            // request raw data from sensor


  BPM = pox.getHeartRate();
  SpO2 = pox.getSpO2();
  //bool stop = 1;


  if (sensor.getRawValues(&ir, &red)) {      // If raw data available for IR and Red

    /*Serial.print("ir:");
      Serial.println(ir);
      Serial.print("red:");
      Serial.println(red);*/


    if (red < 1000)  {
      //Serial.println("Please Place your finger");
      oled.clearDisplay();
      oled.setTextSize(1);
      oled.setTextColor(1);
      oled.setCursor(30, 5);
      oled.println("Please Place ");
      oled.setCursor(30, 25);
      oled.println("your finger ");
      oled.display();


    }


    if (millis() - tsLastReport > REPORTING_PERIOD_MS && red > 3000) {

      //Serial.println("I'm in ");
      /*
        Serial.print("Heart rate:");
        Serial.print(BPM);
        Serial.print(" bpm / SpO2:");
        Serial.print(SpO2);
        Serial.println(" %");
      */

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


      //poorWeb (BPM, SpO2);

    }

    if (red > 1500 && red < 3700 && BPM > 30 && SpO2 > 50) {    // post data to DB when finger leave
      /*
        Serial.print("Heart rate_post:");
        Serial.print(BPM);
        Serial.print(" bpm / SpO2_post:");
        Serial.print(SpO2);
        Serial.println(" %");
      */

      servletGo(BPM, SpO2);

      oled.clearDisplay();
      oled.setTextSize(1);
      oled.setTextColor(1);
      oled.setCursor(30, 5);
      oled.println("Upload data ");
      oled.setCursor(30, 25);
      oled.println("to DB ");
      oled.display();

      //delay(500);
      Serial.println("***** RESET ******");
      ESP.reset();                            // fix the frozen problem

      //Serial.print("red:");
      //Serial.println(red);


    }
  }

}




void servletGo(uint8_t BPM, uint8_t SpO2) {

  // REPLACE with your Domain name and URL path or IP address with path
  //const char* serverName = "http://192.168.10.12:8080/FinalProject/bpminsert";        //Stan==> for send data to jsp===============================================
  const char* serverName = "http://192.168.0.3:8080/FinalProject/bpminsert";
  //const char* serverName = "http://192.168.0.6:8080/


  //Check WiFi connection status
  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;


    // Your Domain name with URL path or IP address with path
    http.begin(serverName);

    // Specify content-type header
    http.addHeader("Content - Type", "application / x - www - form - urlencoded");


    // Prepare your HTTP POST request data
    String httpRequestData = "Pulse_Rate = " + String(BPM) + "&SpO2 = " + String(SpO2) + "&Patno = 20";


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
  else {
    Serial.println("WiFi Disconnected");    //Stan=====> display no WiFi
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(1);
    oled.setCursor(30, 5);
    oled.println("Sorry, WiFi ");
    oled.setCursor(30, 15);
    oled.println("Disconnected ");
    oled.display();

  }
  //Send an HTTP POST request every 15 seconds
  //delay(15000);



}



void poorWeb (uint8_t B, uint8_t S) {
  uint8_t BPM = B;
  uint8_t SpO2 = S;

  server.on("/", handleRoot(BPM, SpO2));
  server.on("/about", []() {
    server.send(200, "text/html" , "hello Donkey!");        //stan_if request sucess, response this.

  });

  server.onNotFound([]() {
    server.send(404, "text/html" , "File Not found!!!");
  });


  server.begin();      //stan_ webserver on



  server.handleClient();             // service for client

}



void handleRoot(uint8_t BPM, uint8_t SpO2) {
  String HTML = "<!DOCTYPE html>\
  <html><head><meta charset= 'utf-8' ></head>\
  <body><h2>快傳病人編號給我,笨蛋!</h2>\
  <P>心跳:" + String(BPM) + "</P>\
  <P>血氧:" + String(SpO2) + "</p>\
  </body></html>";
  server.send(200, "text/html", HTML);

}
