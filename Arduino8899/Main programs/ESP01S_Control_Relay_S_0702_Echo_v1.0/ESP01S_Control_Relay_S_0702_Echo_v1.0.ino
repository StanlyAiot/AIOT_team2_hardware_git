#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#ifndef STASSID
#define STASSID "SASAT"  //無線分享器的名稱
#define STAPSK  "0920231219"    //密碼
#define RELAY 0 // relay connected to  GPIO0
#endif

const char* ssid = STASSID;
const char* password = STAPSK;


ESP8266WebServer server(80);

void handleRoot() {  //訪客進入主網頁時顯示的內容
  server.send(200, "text/plain", "Hello From ESP01S Relay!");
}

void handleNotFound() {  //找不到網頁時顯示的內容
  server.send(404, "text/plain", "File Not Found");
}

void setup(void) {

  pinMode(RELAY,OUTPUT);
  digitalWrite(RELAY, HIGH);
  
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");


  // 等待WiFi連線
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //在監控視窗顯示取得的IP

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);  //綁定主網頁會觸發的副程式

  server.on("/on", []() {      //網頁 /on 會執行的程式
    server.send(200, "text/plain", "Relay Turn ON");
    digitalWrite(RELAY,LOW);
    Serial.print("Relay Turn On");
  });

  server.on("/off", []() {      //網頁 /off 會執行的程式
    server.send(200, "text/plain", "Relay Turn Off");
    digitalWrite(RELAY,HIGH);
    Serial.print("Relay Turn Off");
        
  });


  server.onNotFound(handleNotFound);  //綁定找不到網頁時會觸發的副程式

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
  MDNS.update();
}
