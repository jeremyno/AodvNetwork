#include <EEPROM.h>

void pauseDigits(byte chars) {
  while (Serial.available() < chars) {
    delay(100);
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("What address? (3 digits, eg 001)");
  pauseDigits(3);
  byte num = Serial.parseInt();
  EEPROM.write(0,num);
  
  Serial.print("Saved address as: ");
  Serial.println(EEPROM.read(0));
  
  
}

void loop() {
  // put your main code here, to run repeatedly:

}
