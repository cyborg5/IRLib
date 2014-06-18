IRLib – an Arduino library for infrared encoding and decoding
Version 1.5 June 2014
Copyright 2013, 2014 by Chris Young http://cyborg5.com
 
This library is covered by the GNU LESSER GENERAL PUBLIC LICENSE.

This readme is for the Pinoccio section only.
****************************************************
The Pinoccio Scout is a crowd sourced funded project from Indiegogo. It is in Arduino-like platform
based on the Atmel ATmega256RFR2 8-bit processor with 256K flash memory, 30 2K SRAM,
8K EEPROM and a built-in 2.4 GHz 802.15.4 mesh radio, LiPo battery, RGB LED, temperature sensor, and
an available Wi-Fi backpack. Is billed as "A Complete Ecosystem for Building the Internet of Things".
(By the way Pinoccio is not a typo. They really do spell it without the "h")

It was initially only available to the original backers but is now available as a public beta test.
For more information on the platform see https://pinocc.io/

This IRLib package automatically detects the ATmega256RFR2 processor in IRLibTimer.h. We have included
a Pinoccio Scout specific version of the IRrecvDump sample sketch for receiving IR signals and various 
other sample sketches. There is also sketch that adds an "ir.send(p,c,b)" command to the Scout Script 
language used by the system. You can use the hq.pinocc.io portal on a Google Chrome browser to send IR
codes using this command.

You should be able to adapt any other sample sketches or IRLib code of your own into the Pinoccio
infrastructure files by looking at those examples we provide.

We have also included a simple web-based application using the "ir.send" function. The IR codes in this 
app are set up for my Samsung TV and my Cisco/SA cable box. You can edit remote .js to enter 
your own codes. This web app is in simple HTML and JavaScript. It will run on a Windows-based PC 
on Internet Explorer just as a static file off of your hard drive without any web server at all. If you have 
a node JS web server on local host it will also run using Google Chrome and Mozilla Firefox. You can 
also hosts it on a public web server but your authentication token needs to be hardcoded in or you will 
have to write your own login front end.

The only available timer on this platform is TIMER 3. Output IR LED circuit should be connected to
digital pin D3. Any input pin can be used for receiving but we recommend D4.

****************************************************
The folder contains:
IRrecvDumpScout	Pinoccio Scout version of IRrecvDump sample sketch decoding codes. 
IRfreqScout	Frequency detection sketch ported to Pinoccio
IRrecvDumpFreqScout	Detect frequency and pattern simultaneously.
IRsendScout	Sketch adds "ir.send(p,c,b)" command to Scout Script
html		Folder contains a simple web application in HTML and JavaScript for a
		TV and cable box remote control use with IRsend Scout.
README.txt	This file.
