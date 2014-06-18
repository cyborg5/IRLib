/* Example program for from IRLib – an Arduino library for infrared encoding and decoding
 * Version 1.5   June 2014
 * Copyright 2014 by Chris Young http://tech.cyborg5.com
 * Based on Arduino firmware for use with AnalysIR IR signal analysis
 * software for Windows PCs. Many thanks to the people 
 * at http://analysir.com for their assistance In developing this program.
 */
/*
 * IRLib: IRrecvDumpFreqScout - dump details of IR codes with IRrecvPCI and
 * measures frequency using IRfrequency. You must connect an IR receiver
 * such as TSOP58338 or equivalent to a hardware interrupt pin and connect
 * an IR learner such as TSMP58000 to an additional hardware interrupt pin.
 */
/*
 * This is a version of IRrecvFreqDump modified for use with Pinoccio Scout
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

#include <IRLib.h>
#define RECEIVER_INTERRUPT 0
#define FREQUENCY_INTERRUPT 6

//Using IRrecvPCI or IRrecvLoop for basic decoding when measuring frequency 
// simultaneously. It produces more accurate results because they do not interfere
// with frequency interrupt as much as IRrecv which samples every 50 microseconds.
IRrecvPCI My_Receiver(RECEIVER_INTERRUPT);
//IRrecvLoop My_Receiver(4); //comment out previous line and un-comment this for loop version
IRfrequency My_Freq(FREQUENCY_INTERRUPT);
IRdecode My_Decoder;

void setup()
{
  Scout.setup(SKETCH_NAME, SKETCH_REVISION, SKETCH_BUILD);
  Serial.print(F("Receiver interrupt ="));Serial.print(RECEIVER_INTERRUPT,DEC);
  Serial.print(F(" receiver pin =")); Serial.println(My_Receiver.getPinNum(),DEC);
  Serial.print(F("Frequency interrupt="));Serial.print(FREQUENCY_INTERRUPT,DEC);
  Serial.print(F(" frequency pin=")); Serial.println(My_Freq.getPinNum(),DEC);
  if(My_Receiver.getPinNum()==255 || My_Freq.getPinNum()==255)
    Serial.println(F("Invalid interrupt number."));
  My_Receiver.enableIRIn(); // Start the receiver
  My_Freq.enableFreqDetect();//starts interrupt routine to compute frequency
}

void loop() {
  Scout.loop();
  if (My_Receiver.GetResults(&My_Decoder)) {
    My_Freq.disableFreqDetect();//Stop interrupt as soon as possible
    My_Decoder.decode();
    My_Freq.DumpResults(false);//Change to "true" for more detail
    My_Decoder.DumpResults();
    delay(500);
    My_Receiver.resume(); 
    My_Freq.enableFreqDetect();//Zero out previous results and restart ISR
  }
}

