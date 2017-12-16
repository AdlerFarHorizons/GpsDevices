/*
 * SimpleGPSEmu - GPS NMEA source emulator produces GGA and RMC
 * sentences.
 */

 /*
  * Syncing to the PC only works if Teensy is programmed. If it is
  * powered down it will come back up at approximately the time it
  * was powered down.
  * 
  */

#include <MsTimer2.h>
#include <TimeLib.h>
/*
 * $GPGGA,105506.00,4104.90288,N,08753.60184,W,1,5,03,1154.3,M,0.0,M,0.0,0000*45
 * $GPGGA,hhmmss.ss,llll.lllll,a,nnnnn.nnnnn,b,t,uu,v.v,w.w,M,x.x,M,y.y,zzzz*hh <CR><LF>
 * $GPRMC,105506.00,A,4104.90288,N,08753.60184,W,58.32,105.0,141017,0.0,E,A*22
 * $GPRMC,hhmmss.ss,A,llll.lllll,a,yyyyy.yyyyy,a,x.x,x.x,xxxxxx,x.x,a,i*hh<CR><LF>
 */

// Koerner Aviation coordinates:
const float startLat = 41.09; // degrees
const float startLon = -87.9167100; // degrees
const float startAlt = 434.3; // m

const float vAscent = 6; // m/sec
const float vDescent = -10; // m/sec
const float altBurst = 3e4; // m
const int tUpdate = 10; // seconds
const int speedup = 1;

const float earth_radius = 6.7E6; // m
const float latScale = 1.124E5; // m/deg
const float knots2mps = 0.51444444444;
const float ft2m = 0.3048;
const float deg2rad = 3.1415927 / 180.0;

boolean isBurst;
byte inByte;
char inChar;
boolean timesUp;
float vWind = 30; // m/sec
float vWindAngle = 105; // 0-360 deg east of north => tan( vx/vy )
float vx, vy, vz; // x=>lon, y=>lat, z=>alt
float position1[3] = {startLon, startLat, startAlt};
float position2[3] = {startLon, startLat, startAlt};
String inStr;
unsigned long timeNow, timeLast;

void setup() {
  MsTimer2::set( round( 1000.0 * tUpdate / ( 1.0 * speedup) ), sendString );
  // set the Time library to use Teensy 3.0's RTC to keep time
  setSyncProvider( getTeensy3Time );
  Serial1.begin(4800);
  Serial.begin(9600);
  delay( 100 );

  // Sync to PC time
  if (Serial.available()) {
    time_t t = processSyncMessage();
    if (t != 0) {
      Teensy3Clock.set(t); // set the RTC
      setTime(t);
    }
  }
  timeLast = millis();
  vx = vWind * sin( vWindAngle * deg2rad );
  vy = vWind * cos( vWindAngle * deg2rad );
  vz = vAscent;
  Serial.println( nmeaGgaString() );
  Serial.println( nmeaRmcString() );
  timesUp = false;
  isBurst = false;
  MsTimer2::start();
}

void loop() {

  if ( Serial.available() > 0 ) {

    inChar = (char)Serial.read();
    
    // Check for a CTRL-<x> and loop back something readable
    // unless it's a CR BS or LF. 
    // If CR add a LF in case the terminal isn't sending them.
    // If LF (^J), put it through. 
    // If BS, erase the character
    
    if (inChar == '\r') { //Terminal sent a CR, add LF
    
      Serial.write( '\n' );Serial.write( inChar );
      inStr = "";

     } else if ( inChar == '\n' ) { //Line Feed only, send it.
     
      Serial.write( inChar );
      inStr += inChar;
      
    } else if ( (byte)inChar == (char)0x08 ) { // BS, erase the char.

      Serial.write( inChar );Serial.write( '\x20' );Serial.write( inChar );
      inStr = inStr.substring( 0, inStr.length() - 1 );

    } else if ( (byte)inChar < 0x20 ) {

      inChar = (char)((byte)inChar + 0x40);
      Serial.write( '^' );Serial.write( inChar );
      inStr += '^';inStr += inChar;

    } else {

      Serial.write( inChar );
      inStr += inChar;

    }

    Serial.flush();
  
  }

  if ( timesUp ) {
    updatePosition();
    Serial.println( nmeaGgaString() );
    Serial1.println( nmeaGgaString() );
    Serial.println( nmeaRmcString() );
    Serial1.println( nmeaRmcString() );
    timesUp = false;
  }

  if ( !isBurst && position1[2] > altBurst ) {
    vz = vDescent;
    isBurst = true;
  } 

  if ( isBurst && position1[2] < startAlt ) {
    vx = 0.0;
    vy = 0.0;
    vz = 0.0;
  }

}

time_t getTeensy3Time()
{
  return Teensy3Clock.get();
}

String nmeaGgaString() {
  String tmpStr = "GPGGA," +
                  timeToNmeaString( now() ) + "," +
                  latToNmeaString( position1[1] ) + "," +
                  lonToNmeaString( position1[0] ) + "," + "1,5,03," +
                  altToNmeaString( position1[2] ) + ",M,0.0,M,0.0,0000";
  
  return  '$' + tmpStr + '*' + checkStr( tmpStr );
}

String nmeaRmcString() {
  
  String tmpStr = "GPRMC," +
                  timeToNmeaString( now() ) + ",A," +
                  latToNmeaString( position1[1] ) + ',' +
                  lonToNmeaString( position1[0] ) + ',' + 
                  gndSpeedToNmeaString( vx, vy ) + ',' +
                  headingToNmeaString( vx, vy ) + ',' +
                  dateToNmeaString( now() ) + ",0.0,E,A";
   
  return  '$' + tmpStr + '*' + checkStr( tmpStr );
}

String timeToNmeaString( time_t t ) {
  String tmpStr = "";
  int hr = hour(t); 
  if ( hr < 10 ) tmpStr += '0';
  tmpStr += String( hr );
  int min = minute(t); 
  if ( min < 10 ) tmpStr += '0';
  tmpStr += String( min );
  float sec = (float)( second(t) * 100 ) / 100.0; 
  if ( sec < 10 ) tmpStr += "0";
  tmpStr += String( sec );
  return tmpStr;
}

String dateToNmeaString( time_t t ) {
  return zeroPad( String( day(t) ), 2 ) + 
         zeroPad( String( month(t) ), 2 ) +
         String( year(t) ).substring( 2 ); 
}

String latToNmeaString( float lat ) {
  char dir = 'N';
  if ( lat < 0.0 ) {
    dir = 'S';
    lat *= -1.0;
  }
  float mins = 60.0 * ( lat - floor( lat ) );
  int deg = floor( lat );
  return  zeroPad( String( deg ), 2 ) + 
          zeroPad( String( (int)floor( mins ) ), 2 ) +
          zeroFill( String( mins - floor( mins ), 5 ).substring( 1 ), 6 ) +
          ',' + dir; 
}

String lonToNmeaString( float lon ) {
  char dir = 'E';
  if ( lon < 0.0 ) {
    dir = 'W';
    lon *= -1.0;
  }
  float mins = 60.0 * ( lon - floor( lon ) );
  int deg = floor( lon );
  return  zeroPad( String( deg ), 3 ) + 
          zeroPad( String( (int)floor( mins ) ), 2 ) +
          zeroFill( String( mins - floor( mins ), 5 ).substring( 1 ), 6 ) +
          ',' + dir; 
}

String altToNmeaString( float alt ) {
  return String( round( 10 * alt ) / 10.0, 1 );
}

String gndSpeedToNmeaString( float velx, float vely ) {
  return String( sqrt( velx*velx + vely*vely ) / knots2mps, 1 );  
}

String headingToNmeaString( float velx, float vely ) {
  float angle = ( atan2( velx, vely ) ) / deg2rad;
  if ( angle < 0 ) angle += 180.0;
  return String( angle, 1 );   
}

void updatePosition( ) {
  timeNow = millis();
  float deltaT = speedup * ( timeNow - timeLast ) / 1000.0;
  timeLast = timeNow;
  addToLatLonAlt( deltaT * vy, deltaT * vx, deltaT * vz );
}

void addToLatLonAlt( float dy, float dx, float dz ) {
  position1[0] += dx * cos( position1[1] * deg2rad ) / latScale;
  position1[1] += dy / latScale;
  position1[2] += dz;
}

String zeroPad( String str, int strLen ) {  
  int len = str.length();
  for ( int i = 0 ; i < strLen - len ; i++ ) {
    str = '0' + str;
  }
  return str;
}

String zeroFill( String str, int strLen ) {
  int len = str.length();
  for ( int i = 0 ; i < strLen - len ; i++ ) {
    str = str + '0';
  }
  return str;
}
void sendString() {
  timesUp = true;
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

/*  code to process time sync messages from the serial port   */
#define TIME_HEADER  "T"   // Header tag for serial time sync message

unsigned long processSyncMessage() {
  unsigned long pctime = 0L;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013 

  if(Serial.find(TIME_HEADER)) {
     pctime = Serial.parseInt();
     return pctime;
     if( pctime < DEFAULT_TIME) { // check the value is a valid time (greater than Jan 1 2013)
       pctime = 0L; // return 0 to indicate that the time is not valid
     }
  }
  return pctime;
}

