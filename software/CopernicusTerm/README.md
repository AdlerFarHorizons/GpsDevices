TTY NMEA I/O for Trimble's Copernicus II GPS module.

Connected to Arduino per the schematic 
"CopernicusBasicTestCircuit".

Reference: 

"Copernicus II(R) GPS Receiver Reference Manual", Appendix C

**INTRODUCTION**

The program uses CopernicusII port B and will attempt to find the
baud rate that it is configured for.

The program has separate modes for sending commands and receiving
and displaying sentences to/from the device. Terminals don't 
necessarily have a command line buffer like the Arduino IDE. This 
feature allows the use of any terminal program and avoids having 
to compose a command while sentences are constantly being inserted 
between your keyboard strokes when there is no buffer.

**PROGRAM OPERATION**

The program starts by finding the baud rate setting within the range
of the SoftwareSerial limitations on an Arduino UNO. This limit is
a constant called "rateLimit" so it can be easily changed for porting
to a platform with different serial performance limits.

It may not find the baud rate (this is a bug). If not, perform an 
arduino reset or power cycle (to reset the Copernicus as well) and
try again.

The program then goes to receive mode and will display automatically-
generated NMEA sentences from the unit per the current configuration,
if any. The default factory setting is to send a GGA and VTG sentence
every second. If the unit has a backup battery, it will likely not
send any data if it has no valid fix. The unit is normally configured
to report the last fix if power has been cycled.

**NOTE:** Because of SoftwareSerial limitations, the output may not
keep up with the data stream from the Copernicus. The baud rate limit
is set based on writing commands successfully to configure the device,
not to display data correctly. That's the job of the platform where the
device is going to be installed. If you want to check the acquisition
and performance, configure it for less output data.

Typing a colon (:) will toggle between data display and command
modes.

In command mode, a "cmd:" prompt is presented and your input is
accepted and sent upon hitting "Enter".

The command format is a Trimble proprietary command sentence 
*exclusive of* '$PTNL' at the beginning and '\*cs' ('cs'=> checksum 
characters) at the end. *Upper and lower case are acceptable*

The full command sentence sent is displayed and then on the next line, 
the response preceeded with either "err:" or " ok:" is displayed. You
are then prompted for your next command.

To leave command mode, simply type a colon (:) and data display
from the device will resume from that point. Any data sent by the
device while in command mode, is lost.

**DEVICE OPERATION NOTES:**

If you're not seeing any output, the unit might have been
configured for no automatic sentences on power up. A test to 
see if the unit is functional and your wiring is correct is to
send a request for the current automatic sentence configuration
by entering 'QNM' (no quotes). You should receive a response of the form 
'$PTNLRNM,xxxx,yy,\*cc' regardless of having satellites in view
or even and antenna attached. If 'xxxx' is '0000', the unit
has in the past been configured for no auto output and that 
configuration had been saved with the RT message. So, it is
probably working just fine.

You can change the auto output to factory default for example with

SNM,0005,01

You should get a confirmation '$PTNLRNM,A\*3A' (the first 'A' 
means valid, if 'V', it's invalid and you may have entered the
command incorrectly.

A critical setting for high altitude balloons and such is the
dynamics mode, which must be set to 'air' or altitude is limited:

SCR,,,,,,,3,,

and yes, that's seven commas before the '3' for the seven fields
not being changed and there are two unchanged fields after the '3'.

This and any other of your configuration changes will be lost 
on power down unless saved to flash memory with: 

SRT,H,2,,

which should produce a response of '$PTNLRRTA*3F'. Again, the
'A' indicates success and if 'V', it failed - check that the
command is correct and try again.
