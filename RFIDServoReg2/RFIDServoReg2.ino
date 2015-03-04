#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <EEPROM.h>
#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);

Servo servo;
const int open = 70;
const int locked = 160;
const int button = 2;

byte readCard[4];
byte access[4];
boolean wrongCard = true;

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

void setup() {
  Serial.begin(38400);
  SPI.begin();
  mfrc522.PCD_Init();
  servo.attach(5);
  pinMode(button, INPUT_PULLUP);

  Serial.println("Scan RFID card");
  Serial.println();

  for (int i = 0; i < 4; i++) {
    access[i] = EEPROM.read(i);
  }
}

void loop() {
  registerCard();
  openBox();
}

/**
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
        Serial.println();
        Serial.println("Correct card. Opening mailbox!");
        Serial.println();
      } 
      else {
        wrongCard = true;
        Serial.println();
        Serial.println("Wrong RFID-card!");
        Serial.println();
      }
      if (!wrongCard) {
        servo.write(open);
        delay(2000);
        servo.write(locked);
      }
    }
  }
  if (wrongCard) {
    servo.write(locked);

  }
}
/**
 * Når man trykker inn knappen, vil man ha 10 sekunder på å
 * registrere inn nytt RFID kort. Dette vil bli lagret i EEPROM.
 */
void registerCard() {

  if (digitalRead(button) != HIGH) {
    Serial.println("Scan new card within 10 seconds");
    Serial.println();
    for (int i = 0; i < 10000; i += 200) {
      if (mfrc522.PICC_IsNewCardPresent()) {
        if (mfrc522.PICC_ReadCardSerial()) {
          Serial.println("Scanned RFID:");
          for (int j = 0; j < 4; j++) {
            EEPROM.write(j, mfrc522.uid.uidByte[j]);
            access[j] = mfrc522.uid.uidByte[j];
            Serial.print(mfrc522.uid.uidByte[j], HEX);
          }
          Serial.println();
          Serial.println();
          Serial.println("New card added to EEPROM");
          Serial.println();
          mfrc522.PICC_HaltA(); // Stop reading
          return;
        }
      }
      delay(200);
    }
    Serial.println("Card could not be read!");
  }
}




