/* PINNEOVERSIKT:
 * 1:  button, knapp for å registrere RFID-kort
 * 2:  echoPin
 * 3:  trigPin
 * 4:
 * 5:  servo
 * 6:  LCD_D7
 * 7:  LCD_D6
 * 8:  LCD_D5
 * 9:  LCD_D4
 * 20: WIFI_Tx
 * 21: WIFI_Rx
 * 28: LCD_E
 * 29: LCD_RS
 * 49: SPI_RST
 * 53: SPI_SS
 * 51: SPI_MOSI
 * 50: SPI_MISO
 * 52: SPI SCK
 *
 * SPENNINGSOVERSIKT:
 * RFID:     3.3V
 * WIFI:     3.3V
 * Servo:    5.0V
 * LCD:      5.0V
 * Ultralyd: 5.0V
 */
////////WIFI///////////////
#include <SoftwareSerial.h>
#include <aJSON.h>
#define DEBUG true
#define dbg Serial  // USB local debug
#define RESPONSE_LENGTH 1000 // Bytes to allocate for response strings
#define RX 10  // The pin to recieve data from wifi module
#define TX 21  // The pin to transmit data to wifi module
#define IS_JSON true  // For returning Json from server response
#define SSID F("Eiriknett")
#define PASS F("safford001")
#define SERVERIP "192.168.1.10"
#define SERVERPORT "4999"

////////BUS, MASTER/SLAVE////////
#include <SPI.h>
const int SS_PIN = 53;
const int RST_PIN = 49;

////////SERVO////////
#include <Servo.h>
Servo servo;
const int open = -10;
const int locked = 70;

////////RFID&EEPROM////////
#include <MFRC522.h>
#include <EEPROM.h>
MFRC522 mfrc522(SS_PIN, RST_PIN);
const int button = 44;
byte readCard[4];
byte access[4];
boolean wrongCard = true;

// Global variables for wifi
String box_id;
String wifi_ssid;
String wifi_pass;
String wifi_serverip;
String wifi_serverport;

// Funksjon for sammenligning av serienummer på RFID-kort.
boolean isArraysEqual(byte array_1[], byte array_2[]) {
  if (sizeof(array_1) != sizeof(array_2)) {
    return false;
  }
  for (int i = 0; i < sizeof(array_1); i++) {
    if (array_1[i] != array_2[i]) {
      return false;
    }
  }
  return true;
}

////////ULTRALYDSENSOR////////
#include <TimedAction.h>

TimedAction checkMail = TimedAction(1000, mailDetected);
TimedAction refreshLCD = TimedAction(60000, update_lcd);
const int echoPin = 3;
const int trigPin = 2;
const int ledPin = 13;
int confirmMail = 0;
int maxRange = 22;
int minRange = 18;
float duration, distance;
boolean gotMail = false;

////////LCD DISPLAY////////
#include <LiquidCrystal.h>
LiquidCrystal lcd(30, 28, 9, 8, 7, 6);
unsigned long interval = 5000;
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;
boolean isClear = false;
char display_text[33];

void setup() {
  display_text[32] = '0';
  Serial.begin(9600);
  SPI.begin();

  ////////Setup LCD////////
  lcd.begin(16, 2);
  lcd.setCursor(3, 0);
  lcd.print("Welcome to");
  lcd.setCursor(4, 1);
  lcd.print("SmartBOX");
  

  ////////Setup for Servo////////
  servo.attach(5);
  servo.write(locked);
  delay(1000);
  servo.detach();
  
  wifi_init(SSID, PASS);
  
  ////////Setup for registerCard og openBox////////
  mfrc522.PCD_Init();
  pinMode(button, INPUT_PULLUP);
  for (int i = 0; i < 4; i++) {   //Leser inn serienummer som er
    access[i] = EEPROM.read(i);   //lagret i EEPROM til access.
  }

  ///////Setup for ultralyd///////
  pinMode(echoPin, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  rstLcd();
}

void loop() {
  clrLcd();
  registerCard();
  openBox();
  checkMail.check();
  refreshLCD.check();
  if (gotMail) {
    checkMail.disable();
    
  }
  
}

/**
 * LESE RFID-KORT:
 * Leser RFID kortet. Hvis serienummeret på kortet stemmer
 * med korrekt adgangskort vil låsen åpnes og lukkes i løpet av
 * to sekunder
 */
void openBox() {
  if (mfrc522.PICC_IsNewCardPresent()) {
    if (mfrc522.PICC_ReadCardSerial()) {
      for (int i = 0; i < 4; i++) {
        readCard[i] = mfrc522.uid.uidByte[i];
      }
      mfrc522.PICC_HaltA(); // Stop reading

      if (isArraysEqual(readCard, access)) {
        wrongCard = false;
        lcd.clear();
        lcd.print("Correct card.");
        lcd.setCursor(0, 1);
        lcd.print("Opening mailbox!");
        rstLcd();
      }
      else {
        wrongCard = true;
        lcd.clear();
        lcd.print("Wrong RFID-card!");
        rstLcd();
      }
      if (!wrongCard) {
        servo.attach(5);
        servo.write(open);
        delay(1000);
        servo.write(locked);
        delay(500);
        servo.detach();
        wrongCard = true;
        gotMail = false;
        wifi_update_mailbox_status(false, box_id);
        checkMail.enable();
      }
    }
  }
  if (wrongCard) {
    servo.attach(5);
    servo.write(locked);
    delay(1000);
    servo.detach();
  }
}
/**
 * REGISTRERE NYTT RFID-KORT
 *
 * Når man trykker inn knappen, vil man ha 10 sekunder på å
 * registrere inn nytt RFID kort. Dette vil bli lagret i EEPROM.
 */
void registerCard() {
  if (digitalRead(44) != HIGH) {
    for (int i = 0; i < 10000; i += 200) {
      if (i % 1000 == 0) {
        Serial.print(i / 1000 % 1);
        lcd.clear();
        lcd.print("Scan new card");
        lcd.setCursor(0, 1);
        lcd.print("within ");
        lcd.print(10 - (i / 1000));
        lcd.print(" second");
      }
      if (mfrc522.PICC_IsNewCardPresent()) {
        if (mfrc522.PICC_ReadCardSerial()) {
          for (int j = 0; j < 4; j++) {
            EEPROM.write(j, mfrc522.uid.uidByte[j]);
            access[j] = mfrc522.uid.uidByte[j];
          }
          lcd.clear();
          lcd.print("New card added");
          lcd.setCursor(0, 1);
          lcd.print("to EEPROM");
          rstLcd();
          mfrc522.PICC_HaltA(); // Stop reading
          return;
        }
      }
      delay(200);
    }
    lcd.clear();
    lcd.print("Card could not");
    lcd.setCursor(0, 1);
    lcd.print("be read");
    rstLcd();
  }
}

/**
 * ULTRALYDSENSOR
 * Sjekker om det er kommet post hvert sekund. Hvis det blir detektert
 * post 10 ganger på rad, vil man få beskjed om at det er kommet post.
 * Bruker 10 ganger som "failsafe" hvis det skulle være falske signaler.
 */
void mailDetected()
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);

  duration = pulseIn(echoPin, HIGH); //i microsekunder
  distance = duration / 58.2;        //lyden beveger seg med 29.1 µm/s, så vi må dele tiden på 2*v

  if (distance >= maxRange || distance <= minRange && gotMail != true) {
    confirmMail += 1;
    //Serial.println(distance);
    if (confirmMail > 9) {
      gotMail = true;
      Serial.println(distance);
      Serial.println("You got mail!");
      confirmMail = 0;
      wifi_update_mailbox_status(true, box_id);
    }
  }
  else {
    confirmMail = 0;
    gotMail = false;
    Serial.println(distance);
    Serial.println("No mail detected for mailbox:" + box_id);
  }
}

void clrLcd() {
  currentMillis = millis();
  if ((currentMillis - previousMillis) >= interval && isClear == true) {
    lcd.clear();
   
    int index = 0;
    for(int i = 0; i < strlen(display_text); i++){
       if(display_text[i] == '\n'){
         index = i;
         break;
       }
      lcd.setCursor(i,0);
      lcd.print(display_text[i]); 
     
    }
    dbg.print("\n display:");
    dbg.print(display_text);
    lcd.setCursor(0, 1);
    for(int i = index +1; i < strlen(display_text); i++){
      if(display_text[i] == '\0') break;
      
      lcd.setCursor(i- index -1,1);
      lcd.print(display_text[i]);
    
    }    
    isClear = false;
  }
}

void rstLcd() {
  isClear = true;
  previousMillis = millis();
}

// DIGITAL PORT 11 and 12 on the UNO..
SoftwareSerial esp(RX, TX); // RX,TX

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
  
  char* getLCDText(char* json_String, char* buff){
  
    aJsonObject* jsonObject = aJson.parse(json_String);
    aJsonObject* display_text = aJson.getObjectItem(jsonObject, "display_text");
    
    dbg.print(json_String);
    char *LCDText = display_text->valuestring;
    dbg.print("here");
    dbg.print(LCDText);
    
    for(int i = 0; i < strlen(LCDText); i++){
      buff[i] = LCDText[i];
    }    
    //free(json_String);
    aJson.deleteItem(jsonObject);
    
    return buff;
  }
  
  char* getMailboxID(char* json_String, char*buff){
  
    aJsonObject* jsonObject = aJson.parse(json_String);
    aJsonObject* id = aJson.getObjectItem(jsonObject, "id");
    
    char *mailboxID = id->valuestring;
    buff[0] = mailboxID[0];
    buff[1] = mailboxID[1];
    //free(json_String);
    aJson.deleteItem(jsonObject);
    
    return buff;
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
// WiFi                    //
////////////////////////////
void update_lcd(){
  if(box_id == "00") return;
  refreshLCD.disable();
  char * mailbox = wifi_mailbox_by_id(box_id);
  char buff[33];
  memset(display_text, '\0', 33);
  memset(buff, '\0', 33);
  char*lcd_txt = JSON::getLCDText(mailbox, buff);
  for(int i = 0; i < strlen(lcd_txt); i++){
    display_text[i] = lcd_txt[i];
  }
  rstLcd();
  refreshLCD.enable();
}
void wifi_init(String ssid, String pass) {
  box_id = "00";
  wifi_ssid = ssid;
  wifi_pass = pass;
  wifi_serverip = SERVERIP;
  wifi_serverport = SERVERPORT;
  esp.begin(9600);
  dbg.begin(9600);
  
  dbg.println(F("DEBUG: Running Setup"));
  
  sendData("AT+RST\r\n", 2000);  // reset module
  sendData("AT\r\n", 2000);  // reset module
  sendData("AT+CWMODE=1\r\n", 1000);  // configure as access point
  sendData("AT+CWJAP=\"" + ssid + "\",\"" + pass + "\"\r\n", 6000);  // configure as access point
  sendData("AT+CIPMUX=0\r\n", 2000);  // reset module
  sendData("AT+CIFSR\r\n", 1000);  // get ip address
  read_id();
  if(box_id == "00") {
    char buff[3] = "00";
    box_id = String(JSON::getMailboxID(wifi_register_mailbox(), buff));
    if(box_id.length() == 1) {
      box_id = "0" + box_id; 
    }
    write_id();
  }
  update_lcd();
  dbg.println("DEBUG:" + box_id +  "Setup complete\n\n");
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

void wifi_update_mailbox_status(bool hasmail, String id) {
  char *status_json = JSON::updateMailboxStatus(hasmail);
  sendRequest("PUT","/api/mailbox/"+id, status_json);
  free(status_json);
  //dbg.println(status_json);
}

void read_id() {
    int id = EEPROM.read(4);
    int check = EEPROM.read(5);
    if(id > 99) {
      box_id = "00";  // Not initialized
      return;
    }
    if(id != check) {
      box_id = "00"; // Assume garbage values in EEPROM
      return;
    }
    box_id = String(id); 
    if(box_id.length() == 1)
      box_id = "0" + box_id;
    dbg.print("\nreading "+ box_id + " from EEPROM\n");
  }
  
void write_id() {
  if (box_id == "00") {
    return;
  }
  EEPROM.write(4, box_id.toInt());
  EEPROM.write(5, box_id.toInt());
  dbg.print("\nwriting "+box_id+" to EEPROM\n");
}
