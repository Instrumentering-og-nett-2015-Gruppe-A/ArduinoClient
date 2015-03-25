#include <aJSON.h>

char *json_String;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(2, INPUT_PULLUP);
  
  aJsonObject* body;
  body=aJson.createObject();
  
  aJson.addStringToObject(body, "display_text", "This will go on LCDpanel");
  aJson.addStringToObject(body, "id", "2");  
  json_String = aJson.print(body);
}
void loop() {

  // put your main code here, to run repeatedly: 
  //running code if pin 2 connected to ground. For testing, can be removed.
  int sensorVal = digitalRead(2);
  
  char* rfid = "1234";

  if (sensorVal == HIGH){
    updateMailboxStatus(true);
    getLCDText(json_String);
    getMailboxID(json_String);
    setNewRfid(rfid);
  }
}

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
  Serial.println(json_String);
  
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
  aJsonArray* id = aJson.getObjectItem(jsonObject, "rfids");
}
