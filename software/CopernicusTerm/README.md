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
at the beginning and '\*cs' ('\*' + checksum characters) at the
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
