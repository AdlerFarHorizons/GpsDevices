/*
 
*/

#include <SoftwareSerial.h>
SoftwareSerial ss(5,6);
byte nDLE;

void setup() {
  Serial.begin( 38400 );
  while ( Serial.available() > 0 ) Serial.read();
  nDLE = 0;
}

void loop() {
  ss.flush();
  while (Serial.available()) {
    byte c = Serial.read();
    if ( c == 0x10 ) nDLE++;
    Serial.print( c, HEX );
    if ( c == 0x03 && nDLE % 2 == 1 ) {
      Serial.println("");
      nDLE = 0;
    }
  }
}

String hexFill( byte inbyte ) {
    String chkStr = String( inbyte, HEX );
    if ( inbyte < 0x10 ) {
      chkStr = '0' + chkStr;
    }
    chkStr.toUpperCase();
    return chkStr;
}

