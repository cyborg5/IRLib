/* IRsendScout: adds "ir.send(p,c,b)" scout script command
 * Version 1.5 June 2014
 * Copyright 2014 by Chris Young http://tech.cyborg5.com
 * Based upon "Bootstrap.ino" by the Pinoccio team
 * https://pinocc.io/
 * Parameters are from protocol number 0-7, Up to 32 bit data,
 *  and number of bits for the protocols that required.
 * You must use 3 parameters therefore number of bits 
 * should be 0 if not needed. 
 */
#include <SPI.h>
#include <Wire.h>
#include <Scout.h>
#include <GS.h>
#include <bitlash.h>
#include <lwm.h>
#include <js0n.h>

#include "version.h"

#include <IRLib.h> //Add IRLib library

IRsend My_Sender; //Create a sending object

numvar irSend (void) {
  unsigned char protocol = getarg(1);
  unsigned long code     = getarg(2);
  unsigned char bits     = getarg(3);
  Led.setColor(255,0,0);      //just to confirm it works, turn on red LED
  My_Sender.send((IRTYPES)protocol,code,bits); //Send it
  Led.turnOff();              //turn it off
}

void setup() {
  addBitlashFunction("ir.send", (bitlash_function)irSend);
  Scout.setup(SKETCH_NAME, SKETCH_REVISION, SKETCH_BUILD);
}

void loop() {
  Scout.loop();
}
