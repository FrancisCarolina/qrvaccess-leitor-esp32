#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "Informatica";
const char* password = "informatica453";

const char* serverName = "http://10.20.16.151:8000/codigo/validar";

unsigned long lastTime = 0;
unsigned long timerDelay = 5000;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.setDebugOutput(true);
  Serial.println();

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // put your main code here, to run repeatedly:
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
      if(WiFi.status()== WL_CONNECTED){
        WiFiClient client;
        HTTPClient http;
      
        // Your Domain name with URL path or IP address with path
        http.begin(client, serverName);
        
        // If you need Node-RED/server authentication, insert user and password below
        //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");
        
        // Specify content-type header
        //http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        // Data to send with HTTP POST
        //String httpRequestData = "api_key=tPmAT5Ab3j7F9&sensor=BME280&value1=24.25&value2=49.54&value3=1005.14";           
        // Send HTTP POST request
        //int httpResponseCode = http.POST(httpRequestData);
        
        // If you need an HTTP request with a content type: application/json, use the following:
        http.addHeader("Content-Type", "application/json");
        int httpResponseCode = http.POST("{\"codigo\":\"$2a$10$tw2R6xF/6F6ceiunSdlxjOCbIuTM264fQ4/YbJ6Jq.xxHVc9QN/7q\"}");

        // If you need an HTTP request with a content type: text/plain
        //http.addHeader("Content-Type", "text/plain");
        //int httpResponseCode = http.POST("Hello, World!");
        Serial.print("Testando getString(): ");
        Serial.println(http.getString());
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
          
        // Free resources
        http.end();
      }
      else {
        Serial.println("WiFi Disconnected");
      }
      lastTime = millis();
    }
  delay(10000);
}
