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

#define LED_1 0
#define LED_2 2

#define DEVICE_NAME "Smart Library"

int stat;
unsigned long myTime;
int cnt=15;

int readsuccess;
byte readcard[4];
char str[32] = "";
String StrUID;
String serverurl = "http://192.168.100.10:3000/";

String studentUid;
int studentId;
// String bookUid[];

std::vector<int> bookUid;

// put function declarations here:
int getid();
void array_to_string(byte array[], unsigned int len, char buffer[]);
void post_data();

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

  wm.setConfigPortalTimeout(75);

  Serial.print("Connecting");
  //set custom ip for portal
  wm.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  if(wm.autoConnect(DEVICE_NAME)){
        Serial.println("WiFi Connected");
        digitalWrite(LED_2, HIGH); 
        
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
  if(WiFi.status() == WL_CONNECTED){
    stat = 1;
    readsuccess = getid();
    myTime = millis();

  
  if(readsuccess) { 
    stat = 0;
    digitalWrite(LED_2, LOW);
    HTTPClient http;    
    WiFiClient client;
 
    String UIDresultSend, postData;
    UIDresultSend = StrUID;
    String endpoint = serverurl;

    endpoint += "check_card?uid="+UIDresultSend;
  
    http.begin(client,endpoint);  //Request HTTP

    int httpCode = http.GET();
    if (httpCode > 0) {

    String payload = http.getString();    //Mengambil Repsonse
    http.end();  //Close connection

    Serial.println(UIDresultSend);
    Serial.println(httpCode);   //Print HTTP return code
    Serial.println(myTime);
    Serial.println(payload);    //Print request response payload

      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }
    if(httpCode == 200){

      if(doc["data"]["card_type"] == "student"){
        if(studentUid == ""){
          studentUid = UIDresultSend;
          studentId = doc["data"]["id"]; 
        } else {
          if(studentUid == UIDresultSend){
            post_data();
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
    Serial.println(httpCode);
    Serial.println(payload);
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

void post_data(){
  HTTPClient http;    
  WiFiClient client;
 
  String UIDresultSend, postData;
  UIDresultSend = StrUID;
  String endpoint = serverurl;

  DynamicJsonDocument doc(1024);  // Adjust size as necessary

  // Add vector elements to the JSON array
  JsonArray jsonArray = doc.to<JsonArray>();
  for (size_t i = 0; i < bookUid.size(); i++) {
    jsonArray.add(bookUid[i]);
  }

  // Serialize the JSON array to a string
  String jsonString;
  serializeJson(doc, jsonString);

  endpoint += "borrow";
    //Post Data
  postData = "student_id=" + UIDresultSend+"&book_id="+jsonString;
  
  http.begin(client,endpoint);  //Request HTTP
  http.addHeader("Content-Type", "application/x-www-form-urlencoded"); //Content Header
   
  int httpCode = http.POST(postData);   //Mengirim Request
  Serial.println("Post the Data");
  // int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();    //Mengambil Repsonse
    Serial.println(httpCode);
    Serial.println(payload);
    studentUid = "";
    http.end();  //Close connection
    return;
  }
  http.end();  //Close connection
  return;
}