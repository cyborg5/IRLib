/* Example program for from IRLib – an Arduino library for infrared encoding and decoding
 * Version 1.60, January 2016
 * Library: Copyright 2014-2016 by Chris Young http://cyborg5.com
 * This example: Copyright 2016 by Gabriel Staples -- http://www.ElectricRCAircraftGuy.com 
 * -written after doing a major rewrite of IRrecvPCI & IRrecv, and their associated Interrupt 
 * Service Routines (ISRs), buffers, & support functions 
 * 
 * Based on original example sketch for IRremuote library 
 * Version 0.11 September, 2009
 * Copyright 2009 Ken Shirriff
 * http://www.righto.com/
 */
 
/*
 * IRLib: IRrecvPCIDump - dump details of IR codes with IRrecvPCI ("IR Receiver Pin Change Interrupts")
 * -The advantage of using IRrecvPCI instead of IRrecv is that is REQUIRES NO TIMERS TO RECEIVE!
 * 
 * Circuit:
 *  An IR detector/demodulator must be connected to the proper External Interrupt pin in use, 
 *  based on your particular Arduino; see here: https://www.arduino.cc/en/Reference/AttachInterrupt
 * 
 * -TO STOP USING TIMERS: If you are *only* receiving IR data, and not sending IR data, you can 
 *  completely free your timer that this library would otherwise use by going to IRLib.h and 
 *  commenting out the line that says "#define USE_IRRECV". 
 * 
 * -TO SAVE PROGRAM SPACE: If not using either dumpResults methods of IRdecode nor IRfrequency you can
 *  comment out the "#define USE_DUMP" line in IRLib.h, to save considerable program space.
 * -See IRLib.h for more details, options, & info. 
 * 
 * Notes on Mark_Excess:
 * Try different values here for Mark_Excess. 50us is a good starting guess. Ken Shirriff originally used 100us. 
 * It is assumed that your IR receiver filters the modulated signal such that Marks (LOW periods
 * from the IR receiver) are several dozen microseconds too long and Spaces (HIGH periods from the
 * IR receiver) are the same amount too short. This is the case for most IR receivers.
 * If using the dirt-cheap (<$1 for 10) 1838 IR receivers from Ebay, however, 
 * I recommend setting Mark_Excess to -31us. If using the higher quality TSOP4838 ones, 
 * I recommend setting Mark_Excess to +45us. If Mark_Excess is off by too much, your IR receiver will 
 * appear not to work correctly at all, and will not properly decode IR signals. 
 */

#include <IRLib.h>

//specify External Interrupt number (which corresponds to an input pin)
//Ex: for Arduino Uno/Duemilanove/Nano/Pro Mini, etc (ATmega328), External Interrupt 0 is Pin 2, External Interrupt 1 is Pin 3
//-for more boards see here: https://www.arduino.cc/en/Reference/AttachInterrupt
const byte EXT_INTERRUPT_NUMBER = 0; //Pin 2 for ATmega328-based boards, like the Uno
IRrecvPCI My_Receiver(EXT_INTERRUPT_NUMBER);

/*
OPTIONAL: SINGLE VS DOUBLE-BUFFERING: Create another buffer to enable double-buffering for the IR 
receiver code. If double-buffered, your interrupts will continue to receive new data in the background 
even while you are decoding the last dataset in your main code. If you do *not* use double-buffering,
that is ok too, but you must call resume() after you are done using any decoder functions, such 
as decode() or dumpResults(), and ***any IR data that tries to come in while you are decoding/before
you call resume() will be lost.*** See the extensive notes on buffers in IRLibRData.h for more info.
Note: RAWBUF is defined in IRLib.h.
*/
// unsigned int extraBuffer[RAWBUF]; //uncomment to use
IRdecode My_Decoder;

void setup()
{
  Serial.begin(115200);
  delay(2000);while(!Serial);//delay for Leonardo
  Serial.println(F("begin"));  
  // My_Decoder.useDoubleBuffer(extraBuffer); //uncomment to use; requires the "extraBuffer" to be uncommented above 
  //Try different values here for Mark_Excess. 50us is a good starting guess. See detailed notes above for more info.
  My_Receiver.Mark_Excess=50; //us; mark vs space length correction factor
  //Optional: set LED to blink while IR codes are being received 
  // My_Receiver.blink13(true); //blinks whichever LED is connected to LED_BUILTIN on your board, usually pin 13
  //                            //-see here for info on LED_BUILTIN: https://www.arduino.cc/en/Reference/Constants
  My_Receiver.setBlinkLED(13,true); //same as above, except you can change the LED pin number if you like 
  My_Receiver.enableIRIn(); // Start the receiver
}

void loop() {
  if (My_Receiver.getResults(&My_Decoder)) //if IR data is ready to be decoded 
  {
    //1) decode it 
    My_Decoder.decode();
    Serial.println("decoding");
    
    //2) print results 
    //FOR BASIC OUTPUT ONLY:
    // Serial.println(My_Decoder.bits);
    // Serial.println(My_Decoder.value,HEX);
    // Serial.println();
    //FOR EXTENSIVE OUTPUT:
    My_Decoder.dumpResults();
    
    //3) resume receiver (ONLY CALL THIS FUNCTION IF SINGLE-BUFFERED); comment this line out if double-buffered 
    /*for single buffer use; do NOT resume until AFTER done calling all decoder
    functions that use the last data acquired, such as decode and dumpResults; if using a double 
    buffer, don't use resume() at all unless you called My_Receiver.detachInterrupt() previously 
    to completely stop receiving, and now want to resume IR receiving.*/
    My_Receiver.resume(); 
  }
}
