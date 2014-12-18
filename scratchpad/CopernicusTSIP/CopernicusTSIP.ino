/*
	TTY TSIP I/O for Trimble's Copernicus II GPS module.

	Connected to Arduino per the schematic 
	"CopernicusBasicTestCircuit".

	Far Horizons Lab
	June 2014
*/


/*
  NOTES:
  
  Serial baud rate change is limited to 38400 for sake of AltSoftwareSerial on Arduino UNO.
  It can't handle anything faster. Not tested on 8MHz 3V Arduino Mini, may be worse. 
  
  GGA is 78 chars, GSV is 52. Serial B delivers this in (130*10 bits)/baudRate sec, or
  271ms at 4800 baud with default configuration. This would be the minimum latency for
  the fix.
  
  NMEA sentences are 82 chars maximum including CR and LF, so maximum time per sentence
  is 171ms at 4800 baud. 
*/


#include <AltSoftSerial.h>
AltSoftSerial altSerial; // TX Pin 9, RX Pin 8
const long rateLimit = 38400; //Set for Arduino UNO AltSoftSerial limitations.

char modeChange = ':';
boolean sentencePending, sentenceRdy;
byte cmdBuf[82];
byte sentenceBuf[100];
int sentenceBufIndex = 0;
int cmdBufIndex = 0;
boolean cmdMode, cmdRdy, rateChangeFlag, rateChangePending;
long rate;
byte dle = 0x10;
byte etx = 0x03;
char* baudRates[] = { "004800", "009600", "019200", "038400", "057600", "115200" };
int baudRatesLen = 6;

void setup() {
  
  Serial.begin( 115200 );
  while( !Serial );
  findBaudRate();
  
  // clear input buffer
  while ( Serial.available() > 0 ) Serial.read();
  while ( altSerial.available() > 0 ) altSerial.read();

  cmdMode = false;
  cmdRdy = false;
  sentencePending = false;
  sentenceRdy = false;
  rateChangeFlag = false;
  rateChangePending = false;
  sentenceBuf[0] = 0;
  sentenceBufIndex = 0;
  cmdBuf[0] = 0;
  cmdBufIndex = 0;
}

void loop() {
  
  getCharGPS();
  getCharTerm();
  if ( cmdMode ) {
    if ( cmdRdy ) {
      sendCmd();
      if ( cmdBuf[0] != 0 ) {
        getCmdResponse();
        if ( rateChangeFlag ) {
          rateChangeFlag = false;
        }
        Serial.print( "cmd:" );
      }
      cmdRdy = false;
      for ( int i = 0 ; i < cmdBufIndex ; i++ ) cmdBuf[i] = '\0';
      cmdBufIndex = 0;
    }
  } else {
    if (sentenceRdy) {
      for ( int i = 0 ; i < sentenceBufIndex ; i++ ) {
        Serial.print( hexFill( sentenceBuf[sentenceBufIndex] ) );
      }
      //Serial.print( sentenceBuf );//Serial.print(freeRam());
      sentenceRdy = false;
      sentenceBuf[0] = 0;
      sentenceBufIndex = 0;
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

void findBaudRate() {
  cmdMode = true;
  Serial.println( "Finding baud rate..." );
  boolean baudFound = false;
  int baudIndex = 0;
  char temp[7];
  rate = 0;
  while ( !baudFound && rate < rateLimit ) {
    rate = atol( baudRates[baudIndex] );
    Serial.print( "Trying " );Serial.print( rate );Serial.println( "..." );
    altSerial.begin( rate );
    delay(2000);
    while ( altSerial.available() > 0 ) altSerial.read();
    //String tmpStr = "PTNLQPT";
    cmdBuf[0] = dle;
    cmdBuf[1] = 0xFF;
    cmdBuf[2] = dle;
    cmdBuf[3] = dle;
    cmdBuf[4] = etx;
    //altSerial.print( "$" + tmpStr + "*" + checkStr( tmpStr ) +"\r\n" );
    altSerial.write( cmdBuf, 5 );
    delay(1000);
    if ( getCmdResponse() == 0 ) {
      baudFound = true;
    } else {
      Serial.println( " no joy." );
    }
    baudIndex++;
  }
  if ( !baudFound ) Serial.println( "couldn't find baud rate" );
  cmdMode = false; 
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
  byte c;
  if ( ss.available() ) {
    
    c = ss.read();
    
    if ( !sentenceRdy ) {
      
      if ( c == dle ) {
        if ( dleLast ) {
          dleOdd = !dleOdd;
        else {
          dleOdd = true;
        }
        sentencePending = true;
      }
      
      if ( c == etx && sentencePending && dleLast && dleOdd ) {
        sentenceRdy = true;
        sentencePending = false;
        dleOdd = false;
      }
      
      // Write only if it's not the second of a pair of dle's
      if ( !( c == dle && !dleOdd ) ) {
        sentenceBufIndex++;
        sentenceBuf[sentencBufIndex] = c;
      }
      dleLast = ( c == dle );
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
