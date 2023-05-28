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
 
const char* ssid = "MORDOR";
const char* password =  "fernandahplg";
const int led = 2;
long accelX, accelY, accelZ;
float gForceX, gForceY, gForceZ;
long gyroX, gyroY, gyroZ;
float rotX, rotY, rotZ;


const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -10800;
const int daylightOffset_sec = 3600;

DynamicJsonDocument doc(1024);
char buffer[1024];
uint32_t timenow = 0;
 
AsyncWebServer server(5555);
AsyncWebSocket ws("/ws");
 
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
 
  if(type == WS_EVT_CONNECT){
 
    Serial.println("Websocket client connection received");
    digitalWrite(led,HIGH);
    delay(250);
    digitalWrite(led,LOW);
    delay(250);
    digitalWrite(led,HIGH);
    delay(250);
    digitalWrite(led,LOW);
    delay(250);
    digitalWrite(led,HIGH);
    delay(250);
    digitalWrite(led,LOW);
    delay(250);  
    client->text("Hello from ESP32 Server");
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


 
void setup(){
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.begin(115200);
  pinMode(led,OUTPUT);
  initWiFi();
  SPIFFSinit();  
  Wire.begin(); 
  setupMPU();
   
 
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Hello World");
    Serial.println("HTTP_GET request received from client");
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
 
  server.begin();
}
 
void loop(){

 

}