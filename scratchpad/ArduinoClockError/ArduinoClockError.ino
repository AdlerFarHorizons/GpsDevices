volatile long startMicros;
volatile long endMicros;
volatile boolean isActive;

void setup() {
  Serial.begin( 9600 );
  isActive = false;
  attachInterrupt( 0, ppsInt, RISING );
}

void loop() {
  Serial.println( "starting" );
  while ( !isActive );
  Serial.println( "PPS detected..." );
  while ( isActive );
  Serial.println( "Count done" );
  float ppmErr = 1.0e6 - 1.0 * float( endMicros - startMicros );
  Serial.print( startMicros );Serial.print( " " );Serial.println( endMicros );
  //long temp = endMicros - startMicros;
  Serial.println( ppmErr );
  //while (true);
}

void ppsInt() {
  if ( !isActive ) {
    startMicros = micros();
    isActive = true;
  } else {
    endMicros = micros();
    isActive = false;
  }
}
