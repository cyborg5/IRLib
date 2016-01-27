/* Example program for from IRLib – an Arduino library for infrared encoding and decoding
 * Version 1.60   January 2016
 * Copyright 2014 by Chris Young http://cyborg5.com
 * Slight mods, & notes on Mark_Excess, external buffer, etc, added by Gabriel Staples (www.ElectricRCAircraftGuy.com) on 26 Jan 2016 
 * Based on original example sketch for IRremote library 
 * Version 0.11 September, 2009
 * Copyright 2009 Ken Shirriff
 * http://www.righto.com/
 */
/*
 * IRLib: IRrecvDump - dump details of IR codes with IRrecv
 * An IR detector/demodulator must be connected to the input RECV_PIN.
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

int RECV_PIN = 11;

IRrecv My_Receiver(RECV_PIN);

IRdecode My_Decoder;
unsigned int Buffer[RAWBUF]; //NB: you MUST use this external buffer if you want to resume the receiver before 
                             //decoding. Otherwise, you must decode and *then* resume. Search IRLib.cpp for 
                             //"external buffer" for more info.

void setup()
{
  Serial.begin(9600);
  delay(2000);while(!Serial);//delay for Leonardo
  My_Decoder.UseExtnBuf(Buffer);
  //Try different values here for Mark_Excess. 50us is a good starting guess. See detailed notes above for more info.
  My_Receiver.Mark_Excess=50; //us; mark/space correction factor
  My_Receiver.enableIRIn(); // Start the receiver
}

void loop() {
  if (My_Receiver.GetResults(&My_Decoder)) {
    //Restart the receiver so it can be capturing another code while we are working on decoding this one.
    //NB: you are ONLY allowed to resume before decoding if you are using an external buffer. See note above.
    My_Receiver.resume(); 
    My_Decoder.decode();
    My_Decoder.DumpResults();
  }
}

