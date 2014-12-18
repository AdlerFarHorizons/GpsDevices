#include <AltSoftSerial.h>

AltSoftSerial altSerial; // TX->Pin9 RX->Pin8
int baud = 4800;

void setup() {
  Serial.begin( baud );
  altSerial.begin( baud );
}

void loop() {
  int in = Serial.read();
  if ( in != -1 ) altSerial.write( (byte)in );
  int out = altSerial.read();
  if ( out != -1 ) Serial.write( (byte)out );
}
