/* Example program for from IRLib – an Arduino library for infrared encoding and decoding
 * Version 1.5   June 2014
 * Copyright 2014 by Chris Young http://tech.cyborg5.com
 * Based on Arduino firmware for use with AnalysIR IR signal analysis
 * software for Windows PCs. Many thanks to the people 
 * at http://analysir.com for their assistance In developing this program.
 */
/*
 * IRLib: IRFreqScout - measures frequency using IRfrequency. You must
 * connect an IR learner such as TSMP58000 to a hardware interrupt pin.
 */
/*
 * This is a version of IRfreq modified for use with Pinoccio Scout
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
#define FREQUENCY_INTERRUPT 6

IRfrequency My_Freq(FREQUENCY_INTERRUPT);

void setup()
{
  Scout.setup(SKETCH_NAME, SKETCH_REVISION, SKETCH_BUILD);
  Serial.print(F("\nInterrupt="));Serial.print(FREQUENCY_INTERRUPT,DEC);
  Serial.print(F(" Pin=")); Serial.println(My_Freq.getPinNum(),DEC);
  if(My_Freq.getPinNum()==255)
    Serial.println(F("Invalid interrupt number."));
  My_Freq.enableFreqDetect();//starts interrupt routine to compute frequency
}

void loop() {
  Scout.loop();
  if (My_Freq.HaveData()) {
    delay(500);  //It couldn't hurt to collect a little bit more just in case
    My_Freq.disableFreqDetect();
    My_Freq.DumpResults(false);//Change to "true" for more detail
    My_Freq.enableFreqDetect();//Zero out previous results and restart ISR
  }
}

