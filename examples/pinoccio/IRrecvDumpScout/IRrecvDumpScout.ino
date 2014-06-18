/* Example program for from IRLib – an Arduino library for infrared encoding and decoding
 * Version 1.5  June 2014
 * Copyright 2014 by Chris Young http://cyborg5.com
 * Based on original example sketch for IRremote library 
 * Version 0.11 September, 2009
 * Copyright 2009 Ken Shirriff
 * http://www.righto.com/
 */
/*
 * IRLib: IRrecvDumpScout - dump details of IR codes with IRrecv
 * An IR detector/demodulator must be connected to the input RECV_PIN.
 */
/*
 * This is a version of IRrecvDump modified for use with Pinoccio Scout
 * Based upon "Bootstrap.ino" by the Pinoccio team
 * https://pinocc.io/
 */
#include <SPI.h>
#include <Wire.h>
#include <Scout.h>
#include <GS.h>
#include <bitlash.h>
#include <lwm.h>
#include <js0n.h>

#include "version.h"

#include <IRLib.h>  //Include the library
int RECV_PIN = 4;    //Any digital I/O pin would work. We recommend 4
IRrecv My_Receiver(RECV_PIN);
IRdecode My_Decoder;
unsigned int Buffer[RAWBUF];

void setup()
{
  Scout.setup(SKETCH_NAME, SKETCH_REVISION, SKETCH_BUILD);
  My_Receiver.enableIRIn(); // Start the receiver
  My_Decoder.UseExtnBuf(Buffer);
}

void loop() {
  Scout.loop();
  if (My_Receiver.GetResults(&My_Decoder)) {
    //Restart the receiver so it can be capturing another code
    //while we are working on decoding this one.
    My_Receiver.resume(); 
    My_Decoder.decode();
    My_Decoder.DumpResults();
  }
}

