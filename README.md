**IRLib – an Arduino library for infrared encoding and decoding**  
**Version 1.6.0, 30 January 2016**  
Copyright 2013-2015 by Chris Young http://tech.cyborg5.com/irlib/  
-With additions by Gabriel Staples http://www.ElectricRCAircraftGuy.com, 2016  
 
# MAJOR UPDATES BY GABRIEL STAPLES, January 2016:  
**-I just put ~50+ hrs into this library to improve it, and beef it up for my own personal needs and use, and to make double-buffered features and external pin-change-interrupt-based IR receives rock-solid and reliable, so I can begin using this library with*out* taking up any timers, when in receive-only mode. I hope to get this whole thing merged back into Chris Young's main fork, so hopefully that works out...**  
##Version 1.6.0, 30 January 2016:  
  By Gabriel Staples (www.ElectricRCAircraftGuy.com):  
  -IR receiving now works better than ever! -- RECEIVE functions significantly improved!  
  This is a major rewrite of IRrecvPCI and IRrecv, causing them to be double-buffered and able to continually receive data regardless of what the user is doing. Rewrote all ISR routines, for both IRrecv and IRrecvPCI, to make them more robust, and to remove some extraneous/unused code. Added new example to demonstrate IRrecvPCI. Implemented atomic access guards for reading/writing volatile variables outside ISRs, using the ATOMIC_BLOCK(ATOMIC_RESTORESTATE) macro, in order to prevent data corruption when accessing multi-byte volatile variables. Added comments throughout code for additional clarity. Changed input pin from INPUT to INPUT_PULLUP, since many IR receiver datasheets recommend a pullup resistor of >10k~20k on the output line. Made Mark_Excess a 2-byte *signed* value to allow negatives, since some IR receivers (ex: dirt cheap 1838 ones for <$1 for 10 on Ebay) actually need a slightly *negative* Mark_Excess value, since they filter in such a way that *spaces* (output HIGH on the receiver) are slightly longer than marks (output LOW on the receiver), which is opposite the norm. Added in IRrecvBase::setBlinkLED, to be able to arbitrarily set the LED to blink. Also updated the blink code in general, leaving blink13(), however, as an option to use. Added simple filter to remove spurious, short, incoming Marks or Spaces when using IRrecv or IRrecvPCI. (User can modify the filter by changing "MINIMUM_TIME_GAP_PERMITTED" inside IRLibMatch.h). Changed buffer lengths to allow buffers with >255 values (using 2-byte buffer size now instead of 1-byte). Made significant changes and improvements to how buffers are done, allowing the user to single or double-buffer incoming data in a much more robust and reliable way. Removed the need for the user to ever call resume() when using a double buffer. Implemented a detachInterrupt function on the receiver objects utilizing ISRs (namely, IRrecv & IRrecvPCI), so the user can completely disable interrupts and stop receiving incoming IR data if desired. Note: If you do this, you must call resume() afterwards to continue receiving, whether single or double-buffered.  
  -Updated only a few of the examples to work with this new version, namely: IRrecvDump.ino, IRanalyze.ino, IRrecvPCIDump_UseNoTimers.ino. Other examples just need to be updated too; they will need only minor changes to be compatible.  
##Recommendations for future work:  
~By Gabriel Staples, 30 Jan. 2015:  
* Update all example files to work with this new library. Be sure to remove any delay() calls inside the examples, after receiving IR codes or after decoding or wherever, as they are no longer necessary.  
* Logically break out code segments into separate files; it is waaay too much to be in one .cpp file, or one main .h file, for instance.  
* Add the other protocols into this that are lacking here, but present in other IR libraries such as IRremote. IRLib has only 8 protocols, including the hash type, for instance, whereas IRremote has 14.  
* Figure out why it takes double the RAM of comparable IR libraries (ex: IRremote), even when only single-buffered, and with "#define USE_DUMP" and other memory-saving defines, as applicable, commented out in IRLib.h.  
* See if there's a way to import decode and send functions, classes, "routines," etc, for only those protocols you wish you use, to save memory during compilation--preferably without using defines that must be set in the header files. (Doing this from the main Arduino sketch somehow would be ideal).  
* Port code to/add functionality for more powerful 32-bit microcontrollers, such as the Arduion Zero, Due, newer Teensy's, etc.    
* Why so many classes? Consider consolidating some code while trying to take into account portability.  

# Notes from Chris Young:  

This library is a major rewrite of IRemote by Ken Shirriff which was covered 
by GNU LESSER GENERAL PUBLIC LICENSE which as I read it allows me to make 
modified versions. That same license applies to this modified version. See 
his original copyright below.  

The latest Ken Shirriff code can be found at  
https://github.com/shirriff/Arduino-IRremote  

My purpose was to reorganize the code to make it easier to add or remove 
protocols. As a result I have separated the act of receiving a set of raw timing 
codes from the act of decoding them by making them separate classes. That way 
the receiving aspect can be more black box and implementers of decoders and 
senders can just deal with the decoding of protocols.  

Also added provisions to make the classes base classes that could be extended 
with new protocols which would not require recompiling of the original library nor 
understanding of its detailed contents. Some of the changes were made to reduce 
code size such as unnecessary use of long versus bool. Some changes were just my 
weird programming style. Also extended debugging information added.  

IRremote  
Version 0.1 July, 2009  
Copyright 2009 Ken Shirriff  
For details, see http://arcfn.com/2009/08/multi-protocol-infrared-remote-library.htm  
http://arcfn.com  

Interrupt code based on NECIRrcv by Joe Knapp  
http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1210243556  
Also influenced by http://zovirl.com/2008/11/12/building-a-universal-remote-with-an-arduino/  

****************************************************  
###The package contains:  
IRLib.cpp	Code for the library written in object C++  
IRLib.h		Header file which you will include in your sketch  
IRLibMatch.h	Match macros used internally. Need not include this unless you implement  
		your own protocols  
iRLibTimer.h	Attempts to detect type of Arduino board and allows you to modify which   
		interrupt timer you will use. Defaults to timer 2 as did the original KS   
		library. Alternate board and timer information based on a fork of the  
		original KS library. That for can be found here.  
		https://github.com/TKJElectronics/Arduino-IRremote  
IRLibRData.h	Moved irparams structure and related data to this header to facilitate  
		user created extensions to IRrecvBase.  

Note: there is no "IRremoteInt.h" header as in the original library. Those values were 
	moved elsewhere.  

The examples directory contains:  
IRanalyze		Dumps detailed information about a recent signal. Useful for analyzing  
		unknown protocols  
IRfreq		Reports modulation frequency of IR signal. Requires TSMP58000 IR learner  
IRhashdecode	Demonstrates hash decoder.  
IRrecord		Recording incoming signal and play it back when a character is sent  
		through the serial console. By using the console you no longer need   
		to wire up a pushbutton to run this code.  
IRrecvDump	Receives a code, attempts to decode it, produces well formatted   
		output of the results using the new "dump" method.  
IRsendDemo	Simplistic demo to send a Sony DVD power signal every time a   
		character is received from the serial monitor.  
IRsendJVC		Demonstrates sending a code using JVC protocol which is tricky.  
IRservo		Demonstrates controlling a servo motor using an IR remote  
IRserial_remote	Demonstrates a Python application that runs on your PC and sends   
		serial data to Arduino which in turn sends IR remote signals.  
Samsung36	Demonstrates how to expand the library without recompiling it.   
		Also demonstrates how to handle codes that are longer than 32 bits.  
DirecTV		Demonstrates additional protocol for DirecTV  
GIcable		Demonstrates additional protocol for GIcable used by Motorola cable boxes  
rcmm		Additional protocol Phillips RCMM used by AT&T U-Verse boxes  
pinoccio		Demo sketches for Pinoccio Scout platform using ATmega256RFR2  
		Arduino compatible platform. See readme.txt in pinoccio folder for details.  
Note: I did not port any of the other demo sketches although I may add IRTest later.  
The manuals directory contains:  
IRLibReference.docx	Reference manual in Microsoft Word format    
IRLibReference.pdf	Reference manual in Adobe PDF format  
Online version of this manual is also available at:  
	http://tech.cyborg5.com/irlib/docs/  
****************************************************  
###The library handles the following protocols:  
NEC, Sony, RC5, RC6, Raw all of which were supported in the KS version.  
Additionally added Panasonic_Old, JVC, NECx.  
Also added KS hash code routines which he released separately.  
Example code included but not in the library: Samsung36, DirecTV, GIcable. rcmm (U-Verse)  
