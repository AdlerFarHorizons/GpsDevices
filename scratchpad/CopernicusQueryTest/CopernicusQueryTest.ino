/*
 
*/

#include <SoftwareSerial.h>
SoftwareSerial ss(6,5);
byte nDLE;
String sentence;
char modeChange = ':';
boolean sentencePending, sentenceReady;
char cmdBuf[82];
char sentenceBuf[82];
int cmdBufIndex = 0;
int sentenceBufIndex = 0;
boolean cmdMode, cmdModeChange, cmdRdy, sentenceRdy, gpsDataEnabled;
String cmdPrefix = "PTNL";

void setup() {
  Serial.begin( 115200 );
  ss.begin( 38400 );
  while ( Serial.available() > 0 ) Serial.read();
  while ( ss.available() > 0 ) ss.read();
  nDLE = 0;
  
  cmdMode = false;
  cmdModeChange =  false;
  cmdRdy = false;
  sentencePending = false;
  sentenceReady = false;
  sentence = String();
  gpsDataEnabled = true;
  Serial.println(freeRam());
}

void loop() {
//  ss.flush();
//  while (Serial.available()) {
//    byte c = Serial.read();
//    if ( c == 0x10 ) nDLE++;
//    Serial.print( c, HEX );
//    if ( c == 0x03 && nDLE % 2 == 1 ) {
//      Serial.println("");
//      nDLE = 0;
//    }
//  }
  getCharGPS();
  getCharTerm();
  if ( cmdMode ) {
    if ( cmdRdy ) {
      sendCmd();
      if ( cmdBuf[0] != 0 ) {
        getCmdResponse();
        Serial.print( "cmd:" );
      }
      cmdRdy = false;
      for ( int i = 0 ; i < cmdBufIndex ; i++ ) cmdBuf[i] = '\0';
      cmdBufIndex = 0;
    }
  } else {
    if (sentenceRdy) {
      Serial.print( sentence );//Serial.print(freeRam());
      sentenceRdy = false;
      sentence = String();
    }
  }  
}

void getCmdResponse() {
  sentenceRdy = false;
  sentencePending = false;
  long timer = millis() + 2000;
  sentence = String();
  while ( !sentenceRdy && millis() < timer ) {
    getCharGPS();
  }
  if ( sentenceRdy ) {
    if ( sentence.charAt(5) == 'R' ) {
      if ( sentence.charAt(9) == 'V' ) {
        Serial.print( "err:" );
      } else {
        Serial.print( " ok:" );
      }
      Serial.println( sentence);
    } else {
      Serial.println( "err:No Reply" );
      Serial.println( "" );
    }
    sentenceRdy = false; 
    sentence = String();
  }
}

void sendCmd() {
  int i = 0;
  
  // Strip CR + LF
  while ( cmdBuf[i] |= 0 ) {
    if ( cmdBuf[i] == 13 ) cmdBuf[i] = 0;
    if ( cmdBuf[i] == 10 ) cmdBuf[i] = 0;
    i++;
  }
  
  // Check for null command when switch into cmd mode.
  String tmpStr = cmdPrefix + (String)cmdBuf;
  tmpStr.toUpperCase();
  if ( cmdBuf[0] != 0 ) {
    Serial.print( "$" + tmpStr + "*" + checkStr( tmpStr ) );
    ss.print( "$" + tmpStr + "*" + checkStr( tmpStr ) +"\r\n");  
  }
  Serial.println( "" );
}

String checkStr( String str ) {
  char buf[80]; // Max length of NMEA excl. '$',CR,LF = 79, + null
  str.toCharArray(buf, 80);
  byte check = 0x00;
  for ( int i = 0 ; i < str.length() ; i++ ) {
    check ^= (byte)buf[i];
  }
  String chkStr = String( check, HEX );
  if ( check < 0x10 ) {
    chkStr = '0' + chkStr;
  }
  chkStr.toUpperCase();
  return chkStr;
}

void getCharTerm() {
  char c;
  if (Serial.available() ) {
    c = Serial.read();
    if ( c == ':' ) {
      cmdMode = !cmdMode;     
      while ( Serial.read() >= 0 );
      while ( ss.read() >= 0 );
      sentenceRdy = false;
      sentencePending = false;
      sentence = String();
      if ( !cmdMode ) {
        Serial.println("\n");
      } else {
        Serial.print( "\ncmd:" );
      }
    } else if ( cmdMode && !cmdRdy) {
      if ( c == 8 ) { // Backspace
        cmdBuf[cmdBufIndex] = 0;
        if ( cmdBufIndex != 0 ) cmdBufIndex -=  1;
      } else {
        if ( c == 10 ) {
          cmdRdy = true;
        }
        cmdBuf[cmdBufIndex] = c;
        cmdBufIndex +=1;
      }
    }
  }
}
void getCharGPS() {
  char c;
  if ( ss.available() ) {
    c = ss.read();
    if ( !sentenceRdy ) {
      if ( c == 36 ) {
        sentence = String();
        sentencePending = true;
      }
      if ( sentencePending && c == 10 ) {
        sentenceRdy = true;
        sentencePending = false;
      }
      sentence += c;
    }
  }
}

int freeRam() 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

String hexFill( byte inbyte ) {
    String chkStr = String( inbyte, HEX );
    if ( inbyte < 0x10 ) {
      chkStr = '0' + chkStr;
    }
    chkStr.toUpperCase();
    return chkStr;
}

