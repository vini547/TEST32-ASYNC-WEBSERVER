#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <Wire.h>
#include <MPU6050.h>
#include <Arduinojson.h>
#include <string>
#include <sstream>
#include <iostream>
#include <FS.h>
#include <time.h>
#include <esp_log.h>
#include "soc/rtc_wdt.h"
#include <AsyncTCP.h>
#include <Arduino.h>
#include <AsyncWebSocket.h>
#include <Ticker.h>

 
const char* ssid = "MORDOR";
const char* password =  "fernandahplg";
const int led = 2;
long accelX, accelY, accelZ;
float gForceX, gForceY, gForceZ;
long gyroX, gyroY, gyroZ;
float rotX, rotY, rotZ;

int n = 0;
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -10800;
const int daylightOffset_sec = 3600;

DynamicJsonDocument doc(1024);
char buffer[1024];
uint32_t timenow = 0;

Ticker messageSender;
bool sendMessageFlag = false;
	
const char html[] = "<!DOCTYPE html>\n"
"<html lang=\"en\">\n"
"<head>\n"
"    <meta charset=\"UTF-8\">\n"
"    <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">\n"
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
"    <title>MPU6050 Micro Eletro Mechanical Sensor</title>\n"
"</head>\n"
"<script src=\"https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.9.4/Chart.js\"></script>\n"
"\n"
"<span style=\"color: blue;\">\n"
"    <h1>Giro</h1>\n"
"    <p><span id='GiroX'>'%STATE%'</span></p>\n"
"  <!--   <p><span id='GiroY'>0</span></p>\n"
"    <p><span id='GiroZ'>00</span></p>\n"
"    <h1>Accel</h1>\n"
"    <p><span id='AccelX'>0</span></p>\n"
"    <p><span id='AccelY'>0</span></p>\n"
"    <p><span id='AccelZ'>0</span></p> -->\n"
"\n"
"\n"
"\n"
"</span>\n"
"<body>\n"
" <canvas id=\"myChart\" style=\"width:100%;max-width:700px\"></canvas>\n"
"    <script>\n"
"        var Socket;\n"
"        function init() {\n"
"            Socket = new WebSocket('ws://192.168.0.64:5555/ws');\n"
"            Socket.onmessage = function (event) {\n"
"                processCommand(event);\n"
"            };    \n"
"        }\n"
"        function processCommand(event)  {\n"
"            document.getElementById('GiroX').innerHTML = event.data;\n"
"            console.log(event.data);            \n"
"        }\n"
"        window.onload = function (event) {\n"
"            init();\n"
"        }\n"
"    </script>\n"
"    <script>\n"
"        var xyValues = [\n"
"          {x:50, y:7},\n"
"          {x:60, y:8},\n"
"          {x:70, y:8},\n"
"          {x:80, y:9},\n"
"          {x:90, y:9},\n"
"          {x:100, y:9},\n"
"          {x:110, y:10},\n"
"          {x:120, y:11},\n"
"          {x:130, y:14},\n"
"          {x:140, y:14},\n"
"          {x:150, y:15}\n"
"        ];\n"
"        \n"
"        new Chart(\"myChart\", {\n"
"          type: \"scatter\",\n"
"          data: {\n"
"            datasets: [{\n"
"              pointRadius: 4,\n"
"              pointBackgroundColor: \"rgb(0,0,255)\",\n"
"              data: xyValues\n"
"            }]\n"
"          },\n"
"          options: {\n"
"            legend: {display: false},\n"
"            scales: {\n"
"              xAxes: [{ticks: {min: 40, max:160}}],\n"
"              yAxes: [{ticks: {min: 6, max:16}}],\n"
"            }\n"
"          }\n"
"        });\n"
"        </script>\n"
"\n"
"</body>\n"
"</html>";
 
AsyncWebServer server(5555); /* object of class AsyncWebServer needed to configure the HTTP asynchronous webserver. */
AsyncWebSocket ws("/ws");
 
void onWsEvent(AsyncWebSocket * server, 
               AsyncWebSocketClient * client, 
               AwsEventType type, 
               void * arg, 
               uint8_t *data, 
               size_t len) {
  
  if(type == WS_EVT_CONNECT){
    Serial.println("Client Connected"); 
      
  } else if(type == WS_EVT_DISCONNECT){
    Serial.println("Client disconnected"); 
  }
}

void setupMPU(){
  Wire.beginTransmission(0b1101000); //This is the I2C address of the MPU (b1101000/b1101001 for AC0 low/high datasheet sec. 9.2)
  Wire.write(0x6B); //Accessing the register 6B - Power Management (Sec. 4.28)
  Wire.write(0b00000000); //Setting SLEEP register to 0. (Required; see Note on p. 9)
  Wire.endTransmission();  
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x1B); //Accessing the register 1B - Gyroscope Configuration (Sec. 4.4) 
  Wire.write(0x00000000); //Setting the gyro to full scale +/- 250deg./s 
  Wire.endTransmission(); 
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x1C); //Accessing the register 1C - Acccelerometer Configuration (Sec. 4.5) 
  Wire.write(0b00000000); //Setting the accel to +/- 2g
  Wire.endTransmission(); 
  Serial.println("Done MPU Setup");
}

void recordAccelRegisters() {
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x3B); //Starting register for Accel Readings
  Wire.endTransmission();
  Wire.requestFrom(0b1101000,6); //Request Accel Registers (3B - 40)
  while(Wire.available() < 6);
  accelX = Wire.read()<<8|Wire.read(); //Store first two bytes into accelX
  accelY = Wire.read()<<8|Wire.read(); //Store middle two bytes into accelY
  accelZ = Wire.read()<<8|Wire.read(); //Store last two bytes into accelZ
 /* processAccelData();*/
}

void recordGyroRegisters() {
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x43); //Starting register for Gyro Readings
  Wire.endTransmission();
  Wire.requestFrom(0b1101000,6); //Request Gyro Registers (43 - 48)
  while(Wire.available() < 6);
  gyroX = Wire.read()<<8|Wire.read(); //Store first two bytes into accelX
  gyroY = Wire.read()<<8|Wire.read(); //Store middle two bytes into accelY
  gyroZ = Wire.read()<<8|Wire.read(); //Store last two bytes into accelZ
  
}

void SPIFFSinit() {

  if (!SPIFFS.begin(true)){
    Serial.println("Error mounting SPIFFS File System!");
    return;
  }
}

void FileWriter() {

  File FiletoWrite = SPIFFS.open("/test.txt", FILE_WRITE);
  

  if (!FiletoWrite) {

    Serial.println("Error while opening file for writing.");
    return;

  }
  
  if (FiletoWrite.print(buffer)) {
    digitalWrite(led,HIGH);
    delay(500);
    digitalWrite(led,LOW);
    delay(500);
    digitalWrite(led,HIGH);
    delay(500);
    digitalWrite(led,LOW);
    delay(500);
    digitalWrite(led,HIGH);
    delay(500);
    digitalWrite(led,LOW);
    delay(500);

    Serial.println("File successfully writen.");
    } else {
      Serial.println("Could not write to file.");
     }
  
  

  FiletoWrite.close();
  
}

void serializeJsonDocument() {  

    doc["Gyrox"] = gyroX;
    doc["Gyroy"] = gyroY;
    doc["Gyroz"] = gyroZ;
    doc["Accex"] = accelX;
    doc["Accey"] = accelY;
    doc["Accez"] = accelZ;
    doc["Time"] = esp_log_timestamp();
    
        

    serializeJsonPretty(doc, buffer);
        
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());
}

String processor(const String& var)
{

  Serial.println(var+" will be "+ buffer);

  if(var == "STATE"){
    return String(buffer);
  }

  return String();
}

void sendMessages() {
  sendMessageFlag = true;
}

void setup(){
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.begin(115200);
  pinMode(led,OUTPUT);
  initWiFi();
  SPIFFSinit();  
  Wire.begin(); 
  setupMPU();
   
 
  

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", html, processor );
    
    
    
  });

    server.on("/serve_txt", HTTP_GET, [](AsyncWebServerRequest *request){
    recordAccelRegisters();
    recordGyroRegisters();    
    serializeJsonDocument();
    FileWriter();
    request->send(SPIFFS, "/test.txt", "text/txt", true);
    Serial.println("SPIFFS file system served txt file");
    digitalWrite(led,HIGH);
    delay(500);
    digitalWrite(led,LOW);
    delay(500);
    digitalWrite(led,HIGH);
    delay(500);
    digitalWrite(led,LOW);
    delay(500);
    digitalWrite(led,HIGH);
    delay(500);
    digitalWrite(led,LOW);
    delay(500); 
  });



  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.begin();
  messageSender.attach(12.0, sendMessages);
  
  
}
 
void loop(){
 ws.cleanupClients(); 
 recordAccelRegisters();
 recordGyroRegisters();
 serializeJsonDocument();
 
 if (sendMessageFlag) {
    // Generate or retrieve the message you want to send
    String message = buffer;

    // Send the message to all connected clients
    ws.textAll(message);

    // Reset the flag
    sendMessageFlag = true;
  }
 
 
 delay(500);

}