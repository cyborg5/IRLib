/* Example program for from IRLib – an Arduino library for infrared encoding and decoding
 * Version 1.60, January 2016
 * Library Copyright 2014-2016 by Chris Young http://cyborg5.com
 * This example written by Gabriel Staples (www.ElectricRCAircraftGuy.com), Copyright 2016, after doing a major rewrite of IRrecvPCI, and its associated Interrupt Service Routine (ISR)
 * 
 * Based on original example sketch for IRremuote library 
 * Version 0.11 September, 2009
 * Copyright 2009 Ken Shirriff
 * http://www.righto.com/
 */
 
/*
 * IRLib: IRrecvPCIDump - dump details of IR codes with IRrecvPCI
 * -The advantage of using IRrecvPCI instead of IRrecv is that is REQUIRES NO TIMERS TO RECEIVE!
 * -TO STOP USING TIMERS: If you are *only* receiving IR data, and not sending IR data, you can 
 *  completely free your timer that this library would otherwise use by going to IRLib.h and 
 *  commenting out the line that says "#define USE_IRRECV". 
 * 
 * Circuit:
 * An IR detector/demodulator must be connected to the proper External Interrupt pin in use, based on your particular Arduino; see here: https://www.arduino.cc/en/Reference/AttachInterrupt
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

// unsigned int Buffer[RAWBUF]; ///////////////////////////NB: you MUST use this external buffer if you want to resume the receiver before 
                             //decoding. Otherwise, you must decode and *then* resume. Search IRLib.cpp for 
                             //"external buffer" for more info.
IRdecode My_Decoder;

void setup()
{
  Serial.begin(115200);
  delay(2000);while(!Serial);//delay for Leonardo
  Serial.println(F("begin"));
  // My_Decoder.useDoubleBuffer(Buffer);
  //Try different values here for Mark_Excess. 50us is a good starting guess. See detailed notes above for more info.
  My_Receiver.Mark_Excess=50; //us; mark/space correction factor
  // My_Receiver.blink13(true); //blinks whichever LED is connected to LED_BUILTIN on your board, usually pin 13
  //                            //-see here for info on LED_BUILTIN: https://www.arduino.cc/en/Reference/Constants
  My_Receiver.setBlinkLED(13,true); //same as above, except you can change the LED pin number if you like 
  My_Receiver.enableIRIn(); // Start the receiver
}

void loop() {
  if (My_Receiver.getResults(&My_Decoder)) {
    //Restart the receiver so it can be capturing another code while we are working on decoding this one.
    //NB: you are ONLY allowed to resume before decoding if you are using an external buffer. See note above.
    // My_Receiver.resume(); 
    My_Decoder.decode();
    Serial.println("decoding");
    
    //FOR BASIC OUTPUT ONLY:
    // Serial.println(My_Decoder.bits);
    // Serial.println(My_Decoder.value,HEX);
    // Serial.println();
    
    //FOR EXTENSIVE OUTPUT:
    My_Decoder.dumpResults();
    My_Receiver.resume(); //for single buffer use; do NOT resume until AFTER done calling all decoder functions that use the last data acquired, such as decode and dumpResults; if using a double buffer, don't use resume() at all unless you called My_Receiver.detachInterrupt first, and now want to resume IR receiving. 
  }
  // My_Receiver.detachInterrupt();
}
