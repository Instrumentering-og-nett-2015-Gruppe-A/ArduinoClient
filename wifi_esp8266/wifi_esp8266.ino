#include <SoftwareSerial.h>
#include <aJSON.h>
#define DEBUG true
#define dbg Serial  // USB local debug
#define RESPONSE_LENGTH 500 // Bytes to allocate for response strings
#define JSON_LENGTH 200 // Bytes to allocate for Json string from http request
#define PUSH_BUTTON 2  // The number of the pin connected to button
#define RX 11  // The pin to recieve data from wifi module
#define TX 12  // The pin to transmit data to wifi module

// DIGITAL PORT 11 and 12 on the UNO..
SoftwareSerial esp(RX, TX); // RX,TX

String wifi_ssid;
String wifi_pass;
String wifi_serverip;
String wifi_serverport;

//char ex_response[] = "HTTP/1.0 200 OK\nDate: Mon, 23 Mar 2015 23:59:59 GMT\nContent-Type: text/html\nContent-Length: 97\n\n{\"display_text\": \"\", \"has_mail\": false, \"keys\": [], \"opens_in\": 0, \"id\": \"1\", \"is_closed\": false}";

void wifi_init(String ssid, String pass) {
  wifi_ssid = ssid;
  wifi_pass = pass;
  wifi_serverip = "192.168.1.6";
  wifi_serverport = "4999";
  
  pinMode(PUSH_BUTTON, INPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(13, OUTPUT); // For debugging purposes w/o USB
  // Set baud rates
  digitalWrite(13, HIGH);
  esp.begin(9600);
  dbg.begin(9600);
  
  dbg.println("DEBUG: Running Setup");
  
  sendData("AT+RST\r\n", 2000); // reset module
  sendData("AT\r\n", 2000); // reset module
  sendData("AT+CWMODE=1\r\n", 1000); // configure as access point
  sendData("AT+CWJAP=\"" + ssid + "\",\"" + pass + "\"\r\n", 6000); // configure as access point
  sendData("AT+CIPMUX=0\r\n", 2000); // reset module
  sendData("AT+CIFSR\r\n", 1000); // get ip address

  dbg.println("DEBUG: Setup complete\n\n");
}

//void sendData(String command, const int timeout) {
//  esp.print(command); // send the read character to the esp8266
//  long int time = millis();
//  while (((time + timeout) > millis()) && esp.available()) {
//    // Wait for module
//    esp.read();
//  }
//}
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
      while (esp.available() && i < RESPONSE_LENGTH) {
        // The esp has data and the buffer is not full
        char c = esp.read(); // read the next character.
      }
    }
    return response;
  }
  return NULL;
}

char *sendRequest(String method, String resource) {
  sendData("AT+CIPSTART=\"TCP\",\"" + wifi_serverip + "\"," + wifi_serverport + "\r\n", 1000); // open TCP connection
  String httpRequest = method + " " + resource + " HTTP/1.1\r\nUser-Agent: curl/7.37.0\r\nHost: " + wifi_serverip + ":" + wifi_serverport + " \r\nAccept: */*\r\n\r\n";
  String cmd = "AT+CIPSEND=";
  dbg.println("Theis request is sent to server:");
  dbg.println(httpRequest);
  cmd += httpRequest.length();  
  cmd += "\r\n";
  sendData(cmd, 1000);  // Make wifi-module ready for sending request
  char *httpResponse = sendData(httpRequest + "\r\n", 1000, true);
  sendData("AT+CIPCLOSE\r\n", 1000);  // Close connection
  return httpResponse;
}

char *wifi_MailboxbyID(String id) {
  return sendRequest("GET", "api/mailbox/" + id);  // Send http request
}

void setup() {
  wifi_init("Eiriknett","safford001");
}

void loop() {
  if (digitalRead(PUSH_BUTTON) == 1) {
    dbg.println("DEBUG: Button push detected");
    dbg.println(wifi_MailboxbyID("1"));
    dbg.println("end loop");
  }
}


