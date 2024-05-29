#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <FS.h>
#include <WiFiManager.h> // Library WiFi Manager
#include <ArduinoJson.h>
#include <vector>


WiFiManager wm;

#define SS_PIN 4 //D2 
#define RST_PIN 5 //D1
MFRC522 mfrc522(SS_PIN, RST_PIN);  

#define LED_1 16
#define LED_2 2
#define buttonPin 0

#define DEVICE_NAME "Smart Library"

int stat;
unsigned long myTime;
int cnt=15;

int readsuccess;
byte readcard[4];
char str[32] = "";
String StrUID;
String serverurl = "http://kompres.faizrashid.my.id:3000/";

String studentUid;
int studentId;
// String bookUid[];

const unsigned long debounceDelay = 50; // Delay to debounce button
unsigned long lastDebounceTime = 0;
int buttonState = HIGH;
int lastButtonState = HIGH;
unsigned long pressStartTime = 0;
unsigned long longPressDuration = 3000; // Long press duration in milliseconds

bool adminState = false;

std::vector<int> bookUid;

// put function declarations here:
int getid();
void array_to_string(byte array[], unsigned int len, char buffer[]);
void post_data(int studentId);
void adminStateAction();

void setup() {
  WiFi.mode(WIFI_STA); // Mode WiFi STA
  Serial.begin(115200); 
  SPI.begin(); 
  mfrc522.PCD_Init(); 

  delay(500);

  //  WiFi.begin(ssid, password); 
  Serial.println("");

  pinMode(LED_1,OUTPUT); 
  pinMode(LED_2,OUTPUT); 
  pinMode(buttonPin,INPUT_PULLUP); 

  wm.setConfigPortalTimeout(75);

  Serial.print("Connecting");
  //set custom ip for portal
  wm.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  if(wm.autoConnect(DEVICE_NAME)){
        Serial.println("WiFi Connected");
        digitalWrite(LED_2, LOW); 
        
        Serial.println("");
        Serial.print("Successfully connected to : ");
        Serial.println(WiFi.SSID());
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        Serial.println("Please tag a card or keychain to see the UID !");
        Serial.println("");
        stat = 1;
  } else {
        ESP.restart();
        Serial.println("Configportal running");
  }

}

void loop() {
  // Read the state of the button
  int reading = digitalRead(buttonPin);

  // Check if the button state has changed
  if (reading != lastButtonState) {
    // Reset the debounce timer
    lastDebounceTime = millis();
  }

  // Check if the debounce delay has passed
  if ((millis() - lastDebounceTime) > debounceDelay) {

    // If the button state has changed
    if (reading != buttonState) {
      buttonState = reading;

      // If the button is pressed
      if (buttonState == LOW) {
        pressStartTime = millis();
        Serial.println("BUTTON DIPENCET");
      } else { // Button is released
        // Calculate the duration of the press
        unsigned long pressDuration = millis() - pressStartTime;

        // Perform actions based on the press duration
        if (pressDuration < longPressDuration) {
          // Short press action
          Serial.println("Short press");

          adminState = !adminState;
          Serial.println("Admin State : "+ adminState);
          // Perform task for short press
        } else {
          // Long press action
          Serial.println("Long press");
          ESP.restart();
          // Perform task for long press
        }
      }
    }
  }

  // Save the current button state for the next iteration
  lastButtonState = reading;


  if(WiFi.status() == WL_CONNECTED){
    stat = 1;
    readsuccess = getid();
    myTime = millis();
  
  if(adminState){
    digitalWrite(LED_2,HIGH);
    adminStateAction();
  }else {
  digitalWrite(LED_2,LOW);
  if(readsuccess) { 
    stat = 0;
    HTTPClient http;    
    WiFiClient client;
 
    String UIDresultSend;
    UIDresultSend = StrUID;
    String endpoint = serverurl;

    endpoint += "cards/check_card?uid="+UIDresultSend;
  
    http.begin(client,endpoint);  //Request HTTP

    int httpCode = http.GET();
    if (httpCode > 0) {

    String payload = http.getString();    //Mengambil Repsonse
    http.end();  //Close connection

    Serial.print("UID : ");
    Serial.println(UIDresultSend);
    Serial.print("HTTP Code : ");
    Serial.println(httpCode);   //Print HTTP return code
    // Serial.println(payload);    //Print request response payload

      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }
    if(httpCode == 200){
      Serial.println(doc["data"]["card_type"].as<String>());
      Serial.println(doc["data"]["id"].as<String>());
      if(doc["data"]["card_type"] == "student"){
        if(studentUid == ""){
          digitalWrite(LED_2, HIGH);
          studentUid = UIDresultSend;
          studentId = doc["data"]["id"]; 
        } else {
          if(studentUid == UIDresultSend){
            studentUid = "";
            post_data(doc["data"]["id"]);
            digitalWrite(LED_2, LOW);
            return;
          } else {
            return;
          }
        }
      } else if(doc["data"]["card_type"] == "book") {
        if(studentUid != ""){
          bookUid.push_back((int) doc["data"]["id"]);
          return;
        }
      }

  } else {
    Serial.println("Error");
    Serial.print("HTTP Code : ");
    Serial.println(httpCode);
    Serial.print("Payload : ");
    Serial.println(payload);
  }
  }
  }
  }
  } else if(WiFi.status()!= WL_CONNECTED){
     Serial.println("Connection lost, Reseting & try to reconnect in 3 second");
     delay(5000);
     Serial.println("Reset..");
     ESP.restart();
  } else {
    
  }
  
}
//--- Presedur Pembacaan RFID ---//
int getid() {  
  if(!mfrc522.PICC_IsNewCardPresent()) {
    return 0;
  }
  if(!mfrc522.PICC_ReadCardSerial()) {
    return 0;
  }
 
  
  Serial.print("THE UID OF THE SCANNED CARD IS : ");
  
  for(int i=0;i<4;i++){
    readcard[i]=mfrc522.uid.uidByte[i]; //storing the UID of the tag in readcard
    array_to_string(readcard, 4, str);
    StrUID = str;
  }
  Serial.println(StrUID);
  mfrc522.PICC_HaltA();
  return 1;
}
//--- Prosedur mengubah array UID ke string ---//
void array_to_string(byte array[], unsigned int len, char buffer[]) {
    for (unsigned int i = 0; i < len; i++)
    {
        byte nib1 = (array[i] >> 4) & 0x0F;
        byte nib2 = (array[i] >> 0) & 0x0F;
        buffer[i*2+0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
        buffer[i*2+1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
    }
    buffer[len*2] = '\0';
}

void post_data(int studentId) {
  HTTPClient http;    
  WiFiClient client;

  String endpoint = serverurl + "borrows";

  DynamicJsonDocument doc(1024); 

  doc["student_id"] = studentId;

  JsonArray jsonArray = doc.createNestedArray("book_ids");
  for (const auto& book : bookUid) {
    jsonArray.add(book);
  }

  bookUid.clear();
  String postData;
  serializeJson(doc, postData);

  Serial.println(postData);

  // Begin HTTP request
  http.begin(client, endpoint);  
  http.addHeader("Content-Type", "application/json"); 

  // Send POST request
  int httpCode = http.POST(postData);

  // Check the response code
  if (httpCode > 0) {
    String payload = http.getString(); // Get the response
    Serial.println(httpCode);
    Serial.println(payload);
  } else {
    Serial.println("Error in HTTP request");
    Serial.println(http.errorToString(httpCode));
  }

  // End HTTP connection
  http.end(); 
}

void adminStateAction(){
  if(readsuccess) { 
    stat = 0;
    HTTPClient http;    
    WiFiClient client;

    String endpoint = serverurl + "cards/container_card";
    String UIDresultSend;
    UIDresultSend = StrUID;

    DynamicJsonDocument doc(1024); 

    doc["uid"] = UIDresultSend;

    String postData;
    serializeJson(doc, postData);

    Serial.println(postData);

    // Begin HTTP request
    http.begin(client, endpoint);  
    http.addHeader("Content-Type", "application/json"); 

    // Send POST request
    int httpCode = http.POST(postData);

    // Check the response code
    if (httpCode > 0) {
      String payload = http.getString(); // Get the response
      Serial.println(httpCode);
      Serial.println(payload);
    } else {
      Serial.println("Error in HTTP request");
      Serial.println(http.errorToString(httpCode));
    }

    // End HTTP connection
    http.end(); 
    }
  }
