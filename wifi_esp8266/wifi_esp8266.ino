#include <SoftwareSerial.h>
#include <aJSON.h>
#define DEBUG true
#define dbg Serial  // USB local debug
#define RESPONSE_LENGTH 500 // Bytes to allocate for response strings
#define PUSH_BUTTON 44  // The number of the pin connected to button
#define RX 10  // The pin to recieve data from wifi module
#define TX 21  // The pin to transmit data to wifi module
#define IS_JSON true  // For returning Json from server response
#define SSID F("Eiriknett")
#define PASS F("safford001")

// DIGITAL PORT 11 and 12 on the UNO..
SoftwareSerial esp(RX, TX); // RX,TX

String wifi_ssid;
String wifi_pass;
String wifi_serverip;
String wifi_serverport;

  //////////////////////////////
 // Nå begynner JSON-stuff! //
////////////////////////////
namespace JSON{
  char* updateMailboxStatus(boolean hasMail){
    
    aJsonObject *body;
    body=aJson.createObject();
   
    if (hasMail == false){
      aJson.addBooleanToObject(body, "has_mail", false);
    }
    else {
      aJson.addBooleanToObject(body, "has_mail", true);
    }
    
    char *json_String = aJson.print(body);
    
    aJson.deleteItem(body);
    
    return json_String;
  }
  
  char* getLCDText(char* json_String){
  
    aJsonObject* jsonObject = aJson.parse(json_String);
    aJsonObject* display_text = aJson.getObjectItem(jsonObject, "display_text");
    
    //Printing json_string for testing (can be removed)
    Serial.println(json_String);
    //
    
    char *LCDText = display_text->valuestring;
    Serial.println(LCDText);
    
    //free(json_String);
    aJson.deleteItem(jsonObject);
    
    return LCDText;
  }
  
  char* getMailboxID(char* json_String){
  
    aJsonObject* jsonObject = aJson.parse(json_String);
    aJsonObject* id = aJson.getObjectItem(jsonObject, "id");
    
    char *mailboxID = id->valuestring;
    Serial.println(mailboxID);
  
    //free(json_String);
    aJson.deleteItem(jsonObject);
    
    return mailboxID;
  }
  
  char* setNewRfid(char* rfid){
  
    aJsonObject *body;
    body = aJson.createObject();
    aJson.addStringToObject(body, "rfid", rfid);
    
    char *json_String = aJson.print(body);
    Serial.println(json_String);
    
    aJson.deleteItem(body);
    
    return json_String;
  }
  
  char* checkRfid(char* json_String){
    
    aJsonObject* body = aJson.createArray();
    
    aJsonObject* jsonObject = aJson.parse(json_String);
   // aJsonArray* id = aJson.getObjectItem(jsonObject, "rfids");
  }
}

//////////////////////////////
// Resten er WiFi-relatert //
////////////////////////////

void wifi_init(String ssid, String pass) {
  wifi_ssid = ssid;
  wifi_pass = pass;
  wifi_serverip = "192.168.1.6";
  wifi_serverport = "4999";
  
  pinMode(PUSH_BUTTON, INPUT_PULLUP);
//  pinMode(3, OUTPUT);
//  pinMode(4, OUTPUT);
//  pinMode(5, OUTPUT);
//  pinMode(6, OUTPUT);
//  pinMode(7, OUTPUT);
//  pinMode(8, OUTPUT);
//  pinMode(9, OUTPUT);
//  pinMode(10, OUTPUT);
//  pinMode(13, OUTPUT); // For debugging purposes w/o USB
  // Set baud rates
 // digitalWrite(13, HIGH);
  esp.begin(9600);
  dbg.begin(9600);
  
  dbg.println(F("DEBUG: Running Setup"));
  
  sendData("AT+RST\r\n", 2000);  // reset module
  sendData("AT\r\n", 2000);  // reset module
  sendData("AT+CWMODE=1\r\n", 1000);  // configure as access point
  sendData("AT+CWJAP=\"" + ssid + "\",\"" + pass + "\"\r\n", 6000);  // configure as access point
  sendData("AT+CIPMUX=0\r\n", 2000);  // reset module
  sendData("AT+CIFSR\r\n", 1000);  // get ip address

  dbg.println(F("DEBUG: Setup complete\n\n"));
}

char *sendData(String command, const int timeout) {
  return sendData(command, timeout, false);
}

char *sendData(String command, const int timeout, bool getJson) {
  static char response[RESPONSE_LENGTH];
  esp.print(command); // send the read character to the esp8266
  long int time = millis();
  int i = 0;
  if (getJson) {
    memset(response, '\0', RESPONSE_LENGTH);
    while ( (time + timeout) > millis()) {
      while (esp.available() && i < RESPONSE_LENGTH) {
        // The esp has data and the buffer is not full
        char c = esp.read(); // read the next character.
        if (c == '{') {
          while ( (time + timeout) > millis()) {
            while (c != '}' && esp.available() && i < RESPONSE_LENGTH) {
              response[i] = c;
              i ++;
              c = esp.read();
            }
          }
          response[i] = c;
          return response;
        }
      }
    }
  }
  else {
    while ( (time + timeout) > millis()) {
      while (esp.available()) {
        // The esp has data and the buffer is not full
        esp.read(); // read the next character.
      }
    }
  }
  return NULL;
}

char *sendRequest(String method, String resource) {
  return sendRequest(method, resource, "\0");
}

char *sendRequest(String method, String resource, char *data) {
  sendData("AT+CIPSTART=\"TCP\",\"" + wifi_serverip + "\"," + wifi_serverport + "\r\n", 1000); // open TCP connection
  String httpRequest = method + " " + resource + " HTTP/1.1\r\nUser-Agent: curl/7.37.0\r\nHost: "
                              + wifi_serverip + ":" + wifi_serverport + "\r\n"
                              + "Accept: */*\r\n"
                              + "Content-Type: application/json\r\n"
                              + "Content-Length: " + strlen(data) + "\r\n\r\n"
                              + data + "\r\n";
  String cmd = "AT+CIPSEND=";
  dbg.println(F("This request is sent to server:"));
  dbg.println(httpRequest);
  cmd += httpRequest.length();
  cmd += "\r\n";  // important
  sendData(cmd, 1000);  // Make wifi-module ready for sending request
  char *httpResponse;
  httpResponse = sendData(httpRequest, 1000, IS_JSON);
  sendData("AT+CIPCLOSE\r\n", 1000);  // Close connection
  return httpResponse;
}

char *wifi_mailbox_by_id(String id) {
  return sendRequest("GET", "api/mailbox/" + id);  // Returns the json-string for the mailbox
}

char *wifi_register_mailbox() {
  return sendRequest("POST", "/api/mailbox");  // Returns the newly created json-string for the new mailbox
}

void update_mailbox_status(bool hasmail, String id) {
  char *status_json = JSON::updateMailboxStatus(hasmail);
  sendRequest("PUT","/api/mailbox/"+id, status_json);
  free(status_json);
  //dbg.println(status_json);
}
  
void setup() {
  wifi_init(SSID,PASS);
  dbg.println(wifi_mailbox_by_id("2"));
}

void loop() {
  if (digitalRead(PUSH_BUTTON) == 0) {
    dbg.println(F("DEBUG: Button push detected"));
    //listen_for_serverip();
    dbg.println(wifi_mailbox_by_id("2"));
    //dbg.println(wifi_register_mailbox());
    //update_mailbox_status(true, "2");
    dbg.println(F("end loop"));
  }
}


