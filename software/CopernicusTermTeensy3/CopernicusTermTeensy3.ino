/*
	TTY NMEA I/O for Trimble's Copernicus II GPS module.

  Adpated from "CopernicusTerm" for Teensy 3.x from Arduino UNO.
	Non-serial I/O connected per the schematic 
	"CopernicusBasicTestCircuit" (Arduino UNO). Serial I/O connected
	directly to Serial1, so baud rate can go up to 115000.

	Far Horizons Lab
	June 2016
*/


/*
  NOTES:
  
  Serial baud rate change is limited to 115000 due to hardware serial port used on Teensy.
  
  GGA is 78 chars, GSV is 52. Serial B delivers this in (130*10 bits)/baudRate sec, or
  271ms at 4800 baud with default configuration. This would be the minimum latency for
  the fix.
  
  NMEA sentences are 82 chars maximum including CR and LF, so maximum time per sentence
  is 171ms at 4800 baud. 
*/

const long rateLimit = 115200;

char modeChange = ':';
boolean sentencePending, sentenceRdy;
char cmdBuf[82];
char sentenceBuf[100];
int sentenceBufIndex = 0;
int cmdBufIndex = 0;
boolean cmdMode, cmdRdy, rateChangeFlag, rateChangePending;
long rate;
String cmdPrefix = "PTNL";
const char* baudRates[] = { "004800", "009600", "019200", "038400", "057600", "115200" };
int baudRatesLen = 6;

void setup() {
  
  Serial.begin( 115200 );
  while( !Serial );
  delay( 1000 );
  boolean tmp = false;
  while ( !tmp ) {
    tmp = findBaudRate();
  }
  
  // clear input buffer
  while ( Serial.available() > 0 ) Serial.read();
  while ( Serial1.available() > 0 ) Serial1.read();

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
      Serial.print( sentenceBuf );//Serial.print(freeRam());
      sentenceRdy = false;
      sentenceBuf[0] = 0;
      sentenceBufIndex = 0;
    }
  }  
}

int getCmdResponse() {
  
  int result = 1;
  sentenceRdy = false;
  sentencePending = false;
  ulong timer = millis() + 2000;
  sentenceBuf[0] = 0;
  sentenceBufIndex = 0;
  while ( !sentenceRdy && millis() < timer ) {
    getCharGPS();
  }
  if ( sentenceRdy ) {
    if ( sentenceBuf[5] == 'R' ) {
      if ( sentenceBuf[9] == 'V' ) {
        result = 1;
        Serial.print( "err:" );
      } else {
        Serial.print( " ok:" );
        
        // Check for baud rate change
        if ( rateChangePending && sentenceBuf[6] == 'P' && sentenceBuf[7] == 'T' ) {
          rateChangeFlag = true;
          Serial1.begin( rate );
          Serial.print("rate changed:");Serial.println( rate );
        }
        result = 0;
      }
      Serial.println( sentenceBuf);
    } else {
      Serial.println( "err:No Reply" );
      Serial.println( "" );
      result = 2;
    }
    rateChangePending = false;
    sentenceRdy = false; 
    sentenceBuf[0] = 0;
    sentenceBufIndex = 0;
  }
  return(result);
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
    if ( tmpStr.substring(4,7) == "SPT" ) {
      rateChangePending = true;
      char temp[7];
      String rateStr = tmpStr.substring(8,14);
      rateStr.toCharArray( temp, 7 );
      Serial.print("rateStr:");Serial.println(temp);
      rate = atol( temp );
    }
    if ( rate > rateLimit ) {
      Serial.println( "");
      Serial.print( "Command not sent: SoftwareSerial rate limit <= " );Serial.println( rateLimit );
    } else {
      Serial.print( "$" + tmpStr + "*" + checkStr( tmpStr ) );
      Serial1.print( "$" + tmpStr + "*" + checkStr( tmpStr ) +"\r\n");
    }
    Serial.println( "" );
  }
}

String checkStr( String str ) {
  
  char buf[80]; // Max length of NMEA excl. '$',CR,LF = 79, + null
  str.toCharArray(buf, 80);
  byte check = 0x00;
  for ( uint i = 0 ; i < str.length() ; i++ ) {
    check ^= (byte)buf[i];
  }
  String chkStr = String( check, HEX );
  if ( check < 0x10 ) {
    chkStr = '0' + chkStr;
  }
  chkStr.toUpperCase();
  return chkStr;
}

boolean findBaudRate() {
  cmdMode = true;
  Serial.println( "Finding baud rate..." );
  boolean baudFound = false;
  int baudIndex = 0;
  rate = 0;
  while ( !baudFound && rate < rateLimit ) {
    rate = atol( baudRates[baudIndex] );
    Serial.print( "Trying " );Serial.print( rate );Serial.println( "..." );
    Serial1.begin( rate );
    delay(2000);
    while ( Serial1.available() > 0 ) Serial1.read();
    String tmpStr = "PTNLQPT";
    Serial1.print( "$" + tmpStr + "*" + checkStr( tmpStr ) +"\r\n" );
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
  return baudFound;
}

void getCharTerm() {
  
  char c;
  if (Serial.available() ) {
    c = Serial.read();
    if ( c == ':' ) {
      cmdMode = !cmdMode;     
      while ( Serial.read() >= 0 );
      while ( Serial1.read() >= 0 );
      sentenceRdy = false;
      sentencePending = false;
      sentenceBuf[0] = 0;
      sentenceBufIndex = 0;
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
  if ( Serial1.available() ) {
    c = Serial1.read();
    if ( !sentenceRdy ) {
      if ( c == 36 ) {
//        sentence = String();
        sentenceBufIndex = 0;
        sentencePending = true;
      }
      if ( sentencePending && c == 10 ) {
        sentenceRdy = true;
        sentencePending = false;
      }
//      sentence += c;
      sentenceBuf[sentenceBufIndex] = c;
      sentenceBufIndex++;
      sentenceBuf[sentenceBufIndex] = 0;
    }
  }
}

int freeRam() 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void printCmdBuf() {
  
  Serial.print( "buffer:" );
  for ( int i = 0 ; cmdBuf[i] != 0 ; i++ ) {
    Serial.print( (int)cmdBuf[i] );Serial.print( " " );
  }
  Serial.println( " " );
}
