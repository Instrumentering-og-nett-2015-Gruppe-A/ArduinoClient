/*
 The LiquidCrystal library works with all LCD displays 
 that are compatible with the Hitachi HD44780 driver.

  The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)

 */

// include LCD library code:
#include <LiquidCrystal.h>

void setlcdinfo(String line1, String line2) {
  // initialize the library with the numbers of the interface pins
  LiquidCrystal lcd(7, 6, 5, 4, 3, 2);
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Remove characters that do not fit on LCD
  line1.remove(16);
  line2.remove(16);
  // Print to the first row.  
  lcd.print(line1);
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0)
  lcd.setCursor(0, 1);
  // Print to the second row
  lcd.print(line2);
}

void setup() {
  setlcdinfo("Dette er ikke en", "postboks");
}

void loop() {
  // Do nothing
}

