/* Example program for from IRLib – an Arduino library for infrared encoding and decoding
 * Version 1.60   January 2016
 * Copyright 2014 by Chris Young http://cyborg5.com
 * Updated 30 Jan 2016 by by Gabriel Staples -- http://www.ElectricRCAircraftGuy.com 
 * after doing a major rewrite of IRrecvPCI & IRrecv, and their associated Interrupt 
 * Service Routines (ISRs), buffers, & support functions 
 * 
 * Based on original example sketch for IRremote library 
 * Version 0.11 September, 2009
 * Copyright 2009 Ken Shirriff
 * http://www.righto.com/
 */
/*
 * IRLib: IRrecvDump - dump details of IR codes with IRrecv ("IR Receiver" based on 50us timer interrupts)
 * An IR detector/demodulator must be connected to the input RECV_PIN.
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

int RECV_PIN = 2;

IRrecv My_Receiver(RECV_PIN);

IRdecode My_Decoder;

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

void setup()
{
  Serial.begin(115200);
  delay(2000);while(!Serial);//delay for Leonardo
  Serial.println(F("begin"));
  // My_Decoder.useDoubleBuffer(Buffer); //uncomment to use; requires the "extraBuffer" to be uncommented above 
  //Try different values here for Mark_Excess. 50us is a good starting guess. See detailed notes above for more info.
  My_Receiver.Mark_Excess=50; //us; mark/space correction factor
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

