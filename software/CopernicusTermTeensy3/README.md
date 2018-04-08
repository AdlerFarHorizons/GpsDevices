TTY NMEA I/O for Trimble's Copernicus II GPS module.

Copernicus Serial Port B connected to Teensy3+ Serial1 port,
or other port if "Serial1" is replaced with the appropriate
port throughout the code.

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

**IMPORTANT**: It is *crucial* that you're aware that no programming
changes will be effective after a power cycle unless you save your
changes as describe in **DEVICE OPERATION NOTES** below

**PROGRAM OPERATION**

The hardware terminal baud rate is set to 115000 baud, and the program 
starts by finding the Copernicus baud rate setting within the range 
of the standard rates fo the Copernicus II. This limit is a 
constant called "rateLimit".

It may not find the baud rate on the first try (this is a bug), but
will keep cycling through baud rate tests until it does, usually on
the second try. If it continues to fail, power cycle the
Teensy+Copernicus system and try again.

The program then goes to receive mode and will display automatically-
generated NMEA sentences from the unit per the current configuration,
if any. The default factory setting is to send a GGA and VTG sentence
every second. If the unit has a backup battery, it will likely not
send any data if it has no valid fix. The unit is normally configured
to report the last fix if power has been cycled.

Typing a colon (:) will toggle between data display and command
modes.

In command mode, GPS NMEA data display is suppressed and a "cmd:" prompt
is presented. Your input is accepted and sent upon hitting "Enter".

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
by entering 'qnm' (no quotes). You should receive a response of the form 
'$PTNLRNM,xxxx,yy,\*cc' regardless of having satellites in view
or even and antenna attached. If 'xxxx' is '0000', the unit
has in the past been configured for no auto output and that 
configuration had been saved with the RT message. So, it is
probably working just fine.

You can change the auto output to factory default for example with

snm,0005,01

You should get a confirmation '$PTNLRNM,A\*3A' (the first 'A' 
means valid, if 'V', it's invalid and you may have entered the
command incorrectly.

A critical setting for high altitude balloons and such is the
dynamics mode, which must be set to 'air' or altitude is limited:

scr,,,,,,,3,,

and yes, that's seven commas before the '3' for the seven fields
not being changed and there are two unchanged fields after the '3'.

This and any other of your configuration changes will be lost 
on power down unless saved to flash memory with: 

srt,H,2,,

which should produce a response of '$PTNLRRTA*3F'. Again, the
'A' indicates success and if 'V', it failed - check that the
command is correct and try again.
