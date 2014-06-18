/* IRpattern: Send a test pattern of data for various protocols
 * Version 1.5  June 2014
 * Copyright 2014 by Chris Young http://tech.cyborg5.com
 */
#include <IRLib.h>
IRsend My_Sender;
void setup() {
  Serial.begin(9600);
  Serial.println(F("Type the protocol number:1-7"));
}

void loop() {
  if (Serial.available ()>0) {
    int i = Serial.parseInt();
    int bits;
    switch (i) {
        case 2: bits= 20; break;
        case 4: bits = 13; break;
        default: bits = 0;
    };
    Serial.print(F("Sending:"));
    Serial.println(Pnames(i)); 
    My_Sender.send(IRTYPES(i),0x12345678,bits); 
  }
}
