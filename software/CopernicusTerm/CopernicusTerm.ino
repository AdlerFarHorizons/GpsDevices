/*
  CopernicusTerm.ino
  
  TTY NMEA I/O for Trimble's Copernicus II GPS module.
  
  Connected to Arduino per the schematic 
  "CopernicusBasicTestCircuit".
  
  Reference: 
  
  "Copernicus II(R) GPS Receiver Reference Manual", Appendix C
  
  Uses CopernicusII port B and assumes it is programmed for the
  default 4800 baud rate.
  
  Terminal will display automatically sent NMEA sentences from 
  the unit per the current configuration. The default factory 
  setting is to send a GGA and VTG sentence every second regardless
  of the unit getting an actual fix.
  
  Enter a Trimble proprietary command sentence exclusive of '$PTNL'
  at the beginning and '*cs' ('*' + checksum characters) at the
  end. The terminal will immediately display the entire sentence
  as sent ('Sending: <full sentence>') amid the incoming NMEA
  sentences. If the unit sends a response, it will appear shortly
  after.
  
  NOTE: Since sending is asynchronous, the display sometimes gets
  garbled temporarily and the message probably got through ok.
  Re-send the message if you're not sure it was received properly.
  
  If you're not seeing any output, the unit might have been
  configured for no automatic sentences on power up. A test to 
  see if the unit is functional and your wiring is correct is to
  send a request for the current automatic sentence configuration
  by entering 'QNM' (no quotes). You should receive a response of the form 
  '$PTNLRNM,xxxx,yy,*cc' regardless of having satellites in view
  or even and antenna attached. If 'xxxx' is '0000', the unit
  has in the past been configured for no auto output and that 
  configuration had been saved with the RT message. So, it is
  probably working just fine. Change the auto output to factory
  default with 'SNM,0005,01'. You should get a confirmation
  '$PTNLRNM,A*3A' (the 'A' means valid, if 'V', it's invalid and
  you may have entered the command incorrectly.
  
*/

#include <SoftwareSerial.h>
SoftwareSerial ss(6,5);
const byte CHK_INIT = 'P' ^ 'T' ^ 'N' ^ 'L';
boolean echoOn;
String cmdStart = "$PTNL";
String echoToggleStr = "echo";

String sentence;
String command;
byte check;
boolean done;
void setup() {
  Serial.begin( 115200 );
  ss.begin(4800);
  // clear input buffer
  while ( Serial.available() > 0 ) Serial.read();
  while ( ss.available() > 0 ) ss.read();
  check = CHK_INIT;
}

void loop() {
  Serial.flush();
  while (ss.available()) {
    char c = ss.read();
    Serial.write(c); 
  }
  while ( Serial.available() > 0 ) {
    char s = (char) Serial.read();
    if (s == '\r' ) { // if done with entry
      while( Serial.available() > 0 ) {
        Serial.read();
      }
      if (echoOn) {
        Serial.write('\r');Serial.write('\n');
      }
      done = true;
    } else {
      command += s;
      if (echoOn) Serial.print(s);
      check ^= (byte)s;
      
    }
  }
  if ( done ) {
    if ( command == echoToggleStr ) {
      echoOn = !echoOn;
    } else {
      String chkStr = String( check, HEX );
      if ( check < 0x10 ) {
        chkStr = '0' + chkStr;
      }
      chkStr.toUpperCase();
      sentence = "$PTNL" + command + '*' + chkStr + '\r' + '\n';
      Serial.print("Sending: ");Serial.print( sentence );
      ss.print( sentence );
    }
    done = false;
    sentence = "$PTNL";
    check = CHK_INIT;
    command = String();
  }
  delay(100);
}
