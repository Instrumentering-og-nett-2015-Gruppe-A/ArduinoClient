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


////////BUS, MASTER/SLAVE////////
#include <SPI.h>
const int SS_PIN = 53;
const int RST_PIN = 49;

////////SERVO////////
#include <Servo.h>
Servo servo;
const int open = 0;
const int locked = 70;

////////RFID&EEPROM////////
#include <MFRC522.h>
#include <EEPROM.h>
MFRC522 mfrc522(SS_PIN, RST_PIN);
const int button = 44;
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

////////ULTRALYDSENSOR////////
#include <TimedAction.h>

TimedAction checkMail = TimedAction(1000, mailDetected);

const int echoPin = 3;
const int trigPin = 2;
const int ledPin = 13;
int confirmMail = 0;
int maxRange = 22;
int minRange = 20;
float duration, distance;
boolean gotMail = false;

////////LCD DISPLAY////////
#include <LiquidCrystal.h>
LiquidCrystal lcd(30, 28, 9, 8, 7, 6);
unsigned long interval = 5000;
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;
boolean isClear = false;

void setup() {
  Serial.begin(38400);
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

  delay(5000);
  lcd.clear();
  lcd.print("EiT mailbox");
}

void loop() {
  clrLcd();
  registerCard();
  openBox();
  checkMail.check();
  if (gotMail) {
    checkMail.disable();
    digitalWrite(13, HIGH); //informer server om at det er post
  }
  else {
    digitalWrite(13, LOW);
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
        servo.write(open);
        delay(1000);
        checkMail.enable();
        wrongCard = true;
        gotMail = false;
      }
    }
  }
  if (wrongCard) {
    servo.write(locked);

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
      if (
        i == 1000 ||
        i == 2000 ||
        i == 3000 ||
        i == 4000 ||
        i == 5000 ||
        i == 6000 ||
        i == 7000 ||
        i == 8000 ||
        i == 9000 ||
        i == 10000
      ) {
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
    }
  }
  else {
    confirmMail = 0;
    gotMail = false;
    Serial.println(distance);
    Serial.println("No mail detected");
  }
}

void clrLcd() {
  currentMillis = millis();
  if ((currentMillis - previousMillis) >= interval && isClear == true) {
    lcd.clear();
    lcd.print("EiT mailbox");
    isClear = false;
  }
}

void rstLcd() {
  isClear = true;
  previousMillis = millis();
}

