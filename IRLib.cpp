/* IRLib.cpp from IRLib - an Arduino library for infrared encoding and decoding
 * Version 1.60   January 2016 
 * Copyright 2014-2016 by Chris Young http://cyborg5.com
 * With additions by Gabriel Staples (www.ElectricRCAircraftGuy.com); see CHANGELOG.txt 
 *
 * This library is a major rewrite of IRemote by Ken Shirriff which was covered by
 * GNU LESSER GENERAL PUBLIC LICENSE which as I read it allows me to make modified versions.
 * That same license applies to this modified version. See his original copyright below.
 * The latest Ken Shirriff code can be found at https://github.com/shirriff/Arduino-IRremote
 * My purpose was to reorganize the code to make it easier to add or remove protocols.
 * As a result I have separated the act of receiving a set of raw timing codes from the act of decoding them
 * by making them separate classes. That way the receiving aspect can be more black box and implementers
 * of decoders and senders can just deal with the decoding of protocols. It also allows for alternative
 * types of receivers independent of the decoding. This makes porting to different hardware platforms easier.
 * Also added provisions to make the classes base classes that could be extended with new protocols
 * which would not require recompiling of the original library nor understanding of its detailed contents.
 * Some of the changes were made to reduce code size such as unnecessary use of long versus bool.
 * Some changes were just my weird programming style. Also extended debugging information added.
 */
/*
 * IRremote
 * Version 0.1 July, 2009
 * Copyright 2009 Ken Shirriff
 * For details, see http://www.righto.com/2009/08/multi-protocol-infrared-remote-library.html http://www.righto.com/
 *
 * Interrupt code based on NECIRrcv by Joe Knapp
 * http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1210243556
 * Also influenced by http://zovirl.com/2008/11/12/building-a-universal-remote-with-an-arduino/
 */

#include "IRLib.h"
#include "IRLibMatch.h"
#include "IRLibRData.h"
#include <Arduino.h>
#include <util/atomic.h> //for ATOMIC_BLOCK macro (source: http://www.nongnu.org/avr-libc/user-manual/group__util__atomic.html)

volatile irparams_t irparams; //MUST be volatile since it is used both inside and outside ISRs

/*
 * Returns a pointer to a flash stored string that is the name of the protocol received. 
 */
const __FlashStringHelper *Pnames(IRTYPES Type) {
  if(Type>LAST_PROTOCOL) Type=UNKNOWN;
  // You can add additional strings before the entry for hash code.
  const __FlashStringHelper *Names[LAST_PROTOCOL+1]={F("Unknown"),F("NEC"),F("Sony"),F("RC5"),F("RC6"),F("Panasonic Old"),F("JVC"),F("NECx"),F("Hash Code")};
  return Names[Type];
};


#define TOPBIT 0x80000000

/*
 * The IRsend classes contain a series of methods for sending various protocols.
 * Each of these begin by calling enableIROut(unsigned char kHz) to set the carrier frequency.
 * It then calls mark(int usec) and space(inc usec) to transmit marks and
 * spaces of varying length of microseconds however the protocol defines.
 * Because we want to separate the hardware specific portions of the code from the general programming
 * portions of the code, the code for IRsendBase::IRsendBase, IRsendBase::enableIROut, 
 * IRsendBase::mark and IRsendBase::space can be found in the lower section of this file.
 */

/*
 * Most of the protocols have a header consisting of a mark/space of a particular length followed by 
 * a series of variable length mark/space signals.  Depending on the protocol they very the lengths of the 
 * mark or the space to indicate a data bit of "0" or "1". Most also end with a stop bit of "1".
 * The basic structure of the sending and decoding these protocols led to lots of redundant code. 
 * Therefore I have implemented generic sending and decoding routines. You just need to pass a bunch of customized 
 * parameters and it does the work. This reduces compiled code size with only minor speed degradation. 
 * You may be able to implement additional protocols by simply passing the proper values to these generic routines.
 * The decoding routines do not encode stop bits. So you have to tell this routine whether or not to send one.
 */
void IRsendBase::sendGeneric(unsigned long data, unsigned char Num_Bits, unsigned int Head_Mark, unsigned int Head_Space, 
                             unsigned int Mark_One, unsigned int Mark_Zero, unsigned int Space_One, unsigned int Space_Zero, 
							 unsigned char kHz, bool Use_Stop, unsigned long Max_Extent) {
  Extent=0;
  data = data << (32 - Num_Bits);
  enableIROut(kHz);
//Some protocols do not send a header when sending repeat codes. So we pass a zero value to indicate skipping this.
  if(Head_Mark) mark(Head_Mark); 
  if(Head_Space) space(Head_Space);
  for (int i = 0; i <Num_Bits; i++) {
    if (data & TOPBIT) {
      mark(Mark_One);  space(Space_One);
    } 
    else {
      mark(Mark_Zero);  space(Space_Zero);
    }
    data <<= 1;
  }
  if(Use_Stop) mark(Mark_One);   //stop bit of "1"
  if(Max_Extent) {
#ifdef IRLIB_TRACE
    Serial.print("Max_Extent="); Serial.println(Max_Extent);
	Serial.print("Extent="); Serial.println(Extent);
	Serial.print("Difference="); Serial.println(Max_Extent-Extent);
#endif
	space(Max_Extent-Extent); 
	}
	else space(Space_One);
};

void IRsendNEC::send(unsigned long data)
{
  if (data==REPEAT) {
    enableIROut(38);
    mark (564* 16); space(564*4); mark(564);space(56*173);
  }
  else {
    sendGeneric(data,32, 564*16, 564*8, 564, 564, 564*3, 564, 38, true);
  }
};

/*
 * Sony is backwards from most protocols. It uses a variable length mark and a fixed length space rather than
 * a fixed mark and a variable space. Our generic send will still work. According to the protocol you must send
 * Sony commands at least three times so we automatically do it here.
 */
void IRsendSony::send(unsigned long data, int nbits) {
  for(int i=0; i<3;i++){
     sendGeneric(data,nbits, 600*4, 600, 600*2, 600, 600, 600, 40, false,((nbits==8)? 22000:45000)); 
  }
};

/*
 * This next section of send routines were added by Chris Young. They all use the generic send.
 */
void IRsendNECx::send(unsigned long data)
{
  sendGeneric(data,32, 564*8, 564*8, 564, 564, 564*3, 564, 38, true, 108000);
};

void IRsendPanasonic_Old::send(unsigned long data)
{
  sendGeneric(data,22, 833*4, 833*4, 833, 833, 833*3, 833,57, true);
};

/*
 * JVC omits the mark/space header on repeat sending. Therefore we multiply it by 0 if it's a repeat.
 * The only device I had to test this protocol was an old JVC VCR. It would only work if at least
 * 2 frames are sent separated by 45us of "space". Therefore you should call this routine once with
 * "First=true" and it will send a first frame followed by one repeat frame. If First== false,
 * it will only send a single repeat frame.
 */
void IRsendJVC::send(unsigned long data, bool First)
{
  sendGeneric(data, 16,525*16*First, 525*8*First, 525, 525,525*3, 525, 38, true);
  space(525*45);
  if(First) sendGeneric(data, 16,0,0, 525, 525,525*3, 525, 38, true);
}

/*
 * The remaining protocols require special treatment. They were in the original IRremote library.
 */
void IRsendRaw::send(unsigned int buf[], unsigned char len, unsigned char hz)
{
  enableIROut(hz);
  for (unsigned char i = 0; i < len; i++) {
    if (i & 1) {
      space(buf[i]);
    } 
    else {
      mark(buf[i]);
    }
  }
  space(0); // Just to be sure
}

/*
 * The RC5 protocol uses a phase encoding of data bits. A space/mark pair indicates "1"
 * and a mark/space indicates a "0". It begins with a single "1" bit which is not encoded
 * in the data. The high order data bit is a toggle bit that indicates individual
 * keypresses. You must toggle this bit yourself when sending data.
 */

#define RC5_T1		889
#define RC5_RPT_LENGTH	46000
void IRsendRC5::send(unsigned long data)
{
  enableIROut(36);
  data = data << (32 - 13);
  Extent=0;
  mark(RC5_T1); // First start bit
//Note: Original IRremote library incorrectly assumed second bit was always a "1"
//bit patterns from this decoder are not backward compatible with patterns produced
//by original library. Uncomment the following two lines to maintain backward compatibility.
  //space(RC5_T1); // Second start bit
  //mark(RC5_T1); // Second start bit
  for (unsigned char i = 0; i < 13; i++) {
    if (data & TOPBIT) {
      space(RC5_T1); mark(RC5_T1);// 1 is space, then mark
    } 
    else {
      mark(RC5_T1);  space(RC5_T1);// 0 is mark, then space
    }
    data <<= 1;
  }
  space(114000-Extent); // Turn off at end
}

/*
 * The RC6 protocol also phase encodes databits although the phasing is opposite of RC5.
 */
#define RC6_HDR_MARK	2666
#define RC6_HDR_SPACE	889
#define RC6_T1		444
void IRsendRC6::send(unsigned long data, unsigned char nbits)
{
  enableIROut(36);
  data = data << (32 - nbits);
  Extent=0;
  mark(RC6_HDR_MARK); space(RC6_HDR_SPACE);
  mark(RC6_T1);  space(RC6_T1);// start bit "1"
  int t;
  for (int i = 0; i < nbits; i++) {
    if (i == 3) {
      t = 2 * RC6_T1;       // double-wide trailer bit
    } 
    else {
      t = RC6_T1;
    }
    if (data & TOPBIT) {
      mark(t); space(t);//"1" is a Mark/space
    } 
    else {
      space(t); mark(t);//"0" is a space/Mark
    }
    data <<= 1;
  }
  space(107000-Extent); // Turn off at end
}

/*
 * This method can be used to send any of the supported types except for raw and hash code.
 * There is no hash code send possible. You can call sendRaw directly if necessary.
 * Typically "data2" is the number of bits.
 */
void IRsend::send(IRTYPES Type, unsigned long data, unsigned int data2) {
  switch(Type) {
    case NEC:           IRsendNEC::send(data); break;
    case SONY:          IRsendSony::send(data,data2); break;
    case RC5:           IRsendRC5::send(data); break;
    case RC6:           IRsendRC6::send(data,data2); break;
    case PANASONIC_OLD: IRsendPanasonic_Old::send(data); break;
    case NECX:          IRsendNECx::send(data); break;    
    case JVC:           IRsendJVC::send(data,(bool)data2); break;
  //case ADDITIONAL:    IRsendADDITIONAL::send(data); break;//add additional protocols here
	//You should comment out protocols you will likely never use and/or add extra protocols here
  }
}

/*
 * The irparams definitions which were located here have been moved to IRLibRData.h
 */

 /*
 * We've chosen to separate the decoding routines from the receiving routines to isolate
 * the technical hardware and interrupt portion of the code which should never need modification
 * from the protocol decoding portion that will likely be extended and modified. It also allows for
 * creation of alternative receiver classes separate from the decoder classes.
 */
IRdecodeBase::IRdecodeBase(void) {
  //by default, configure for single buffer use (see extensive buffer notes in IRLibRData.h for more info)
  //atomic guards not needed for these single byte volatile variables 
  irparams.doubleBuffered = false; 
  irparams.rawbuf2 = rawbuf = irparams.rawbuf1;
  
  IgnoreHeader=false;
  Reset();
};

/*
 * Use External Buffer:
 * NB: The ISR always stores data directly into irparams.rawbuf2, which is *normally* the same buffer
 * as irparams.rawbuf1. However, if you want to use a double-buffer so you can continue to receive
 * new data while decoding the previous IR code then you can define a separate buffer in your 
 * Arduino sketch and and pass the address here.
 * -See IRrecvBase::GetResults, and the extensive buffer notes in IRLibRData.h, for more info.
 */
void IRdecodeBase::UseExtnBuf(volatile uint16_t *p_buffer){
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    irparams.rawbuf2 = p_buffer; //atomic guards required for volatile pointers since they are multi-byte 
    irparams.doubleBuffered = true;
  }
};

//GS note: 29 Jan 2016: DEPRECATED: copyBuf no longer necessary since the decoder's rawbuf is now the *same buffer* as irparams.rawbuf1. IRhashdecode will be updated to work without copyBuf. 
/*
 * Copies rawbuf and rawlen from one decoder to another. See IRhashdecode example
 * for usage.
 */
/* void IRdecodeBase::copyBuf (IRdecodeBase *source){
  //ensure atomic access in case you are NOT using an external buffer, in which case rawbuf is volatile 
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    memcpy((void *)rawbuf,(const void *)source->rawbuf,sizeof(irparams.rawbuf1));
  }
  rawlen=source->rawlen;
}; */

/*
 * This routine is actually quite useful. Allows extended classes to call their parent
 * if they fail to decode themselves.
 */
bool IRdecodeBase::decode(void) {
  return false;
};

void IRdecodeBase::Reset(void) {
  decode_type= UNKNOWN;
  value=0;
  bits=0;
  rawlen=0;
};
#ifndef USE_DUMP
void DumpUnavailable(void) {Serial.println(F("DumpResults unavailable"));}
#endif
/*
 * This method dumps useful information about the decoded values.
 */
void IRdecodeBase::DumpResults(void) {
#ifdef USE_DUMP
  int i;unsigned long Extent;int interval;
  if(decode_type<=LAST_PROTOCOL){
    Serial.print(F("Decoded ")); Serial.print(Pnames(decode_type));
	Serial.print(F("(")); Serial.print(decode_type,DEC);
    Serial.print(F("): Value:")); Serial.print(value, HEX);
  };
  Serial.print(F(" ("));  Serial.print(bits, DEC); Serial.println(F(" bits)"));
  Serial.print(F("Raw samples(")); Serial.print(rawlen, DEC);
  Serial.print(F("): Gap:")); Serial.println(rawbuf[0], DEC);
  Serial.print(F("  Head: m")); Serial.print(rawbuf[1], DEC);
  Serial.print(F("  s")); Serial.println(rawbuf[2], DEC);
  int LowSpace= 32767; int LowMark=  32767;
  int HiSpace=0; int HiMark=  0;
  Extent=rawbuf[1]+rawbuf[2];
  for (i = 3; i < rawlen; i++) {
    Extent+=(interval= rawbuf[i]);
    if (i % 2) {
      LowMark=min(LowMark, interval);  HiMark=max(HiMark, interval);
      Serial.print(i/2-1,DEC);  Serial.print(F(":m"));
    } 
    else {
       if(interval>0)LowSpace=min(LowSpace, interval);  HiSpace=max (HiSpace, interval);
       Serial.print(F(" s"));
    }
    Serial.print(interval, DEC);
    int j=i-1;
    if ((j % 2)==1)Serial.print(F("\t"));
    if ((j % 4)==1)Serial.print(F("\t "));
    if ((j % 8)==1)Serial.println();
    if ((j % 32)==1)Serial.println();
  }
  Serial.println();
  Serial.print(F("Extent="));  Serial.println(Extent,DEC);
  Serial.print(F("Mark  min:")); Serial.print(LowMark,DEC);Serial.print(F("\t max:")); Serial.println(HiMark,DEC);
  Serial.print(F("Space min:")); Serial.print(LowSpace,DEC);Serial.print(F("\t max:")); Serial.println(HiSpace,DEC);
  Serial.println();
#else
  DumpUnavailable();
#endif
}

/*
 * Again we use a generic routine because most protocols have the same basic structure. However we need to
 * indicate whether or not the protocol varies the length of the mark or the space to indicate a "0" or "1".
 * If "Mark_One" is zero. We assume that the length of the space varies. If "Mark_One" is not zero then
 * we assume that the length of Mark varies and the value passed as "Space_Zero" is ignored.
 * When using variable length Mark, assumes Head_Space==Space_One. If it doesn't, you need a specialized decoder.
 */
bool IRdecodeBase::decodeGeneric(unsigned char Raw_Count, unsigned int Head_Mark, unsigned int Head_Space, 
                                 unsigned int Mark_One, unsigned int Mark_Zero, unsigned int Space_One, unsigned int Space_Zero) {
// If raw samples count or head mark are zero then don't perform these tests.
// Some protocols need to do custom header work.
  unsigned long data = 0;  unsigned char Max; offset=1;
  if (Raw_Count) {if (rawlen != Raw_Count) return RAW_COUNT_ERROR;}
  if(!IgnoreHeader) {
    if (Head_Mark) {
	  if (!MATCH(rawbuf[offset],Head_Mark)) return HEADER_MARK_ERROR(Head_Mark);
	}
  }
  offset++;
  if (Head_Space) {if (!MATCH(rawbuf[offset],Head_Space)) return HEADER_SPACE_ERROR(Head_Space);}

  if (Mark_One) {//Length of a mark indicates data "0" or "1". Space_Zero is ignored.
    offset=2;//skip initial gap plus header Mark.
    Max=rawlen;
    while (offset < Max) {
      if (!MATCH(rawbuf[offset], Space_One)) return DATA_SPACE_ERROR(Space_One);
      offset++;
      if (MATCH(rawbuf[offset], Mark_One)) {
        data = (data << 1) | 1;
      } 
      else if (MATCH(rawbuf[offset], Mark_Zero)) {
        data <<= 1;
      } 
      else return DATA_MARK_ERROR(Mark_Zero);
      offset++;
    }
    bits = (offset - 1) / 2;
  }
  else {//Mark_One was 0 therefore length of a space indicates data "0" or "1".
    Max=rawlen-1; //ignore stop bit
    offset=3;//skip initial gap plus two header items
    while (offset < Max) {
      if (!MATCH (rawbuf[offset],Mark_Zero)) return DATA_MARK_ERROR(Mark_Zero);
      offset++;
      if (MATCH(rawbuf[offset],Space_One)) {
        data = (data << 1) | 1;
      } 
      else if (MATCH (rawbuf[offset],Space_Zero)) {
        data <<= 1;
      } 
      else return DATA_SPACE_ERROR(Space_Zero);
      offset++;
    }
    bits = (offset - 1) / 2 -1;//didn't encode stop bit
  }
  // Success
  value = data;
  return true;
}

/*
 * This routine has been modified significantly from the original IRremote.
 * It assumes you've already called IRrecvBase::GetResults and it was true.
 * The purpose of GetResults is to determine if a complete set of signals
 * has been received. It then copies the raw data into your decoder's rawbuf
 * By moving the test for completion and the copying of the buffer
 * outside of this "decode" method you can use the individual decode
 * methods or make your own custom "decode" without checking for
 * protocols you don't use.
 * Note: Don't forget to call IRrecvBase::resume(); after decoding is complete.
 */
bool IRdecode::decode(void) {
  if (IRdecodeNEC::decode()) return true;
  if (IRdecodeSony::decode()) return true;
  if (IRdecodeRC5::decode()) return true;
  if (IRdecodeRC6::decode()) return true;
  if (IRdecodePanasonic_Old::decode()) return true;
  if (IRdecodeNECx::decode()) return true;
  if (IRdecodeJVC::decode()) return true;
//if (IRdecodeADDITIONAL::decode()) return true;//add additional protocols here
//Deliberately did not add hash code decoding. If you get decode_type==UNKNOWN and
// you want to know a hash code you can call IRhash::decode() yourself.
// BTW This is another reason we separated IRrecv from IRdecode.
  return false;
}

#define NEC_RPT_SPACE	2250
bool IRdecodeNEC::decode(void) {
  IRLIB_ATTEMPT_MESSAGE(F("NEC"));
  // Check for repeat
  if (rawlen == 4 && MATCH(rawbuf[2], NEC_RPT_SPACE) &&
    MATCH(rawbuf[3],564)) {
    bits = 0;
    value = REPEAT;
    decode_type = NEC;
    return true;
  }
  if(!decodeGeneric(68, 564*16, 564*8, 0, 564, 564*3, 564)) return false;
  decode_type = NEC;
  return true;
}

// According to http://www.hifi-remote.com/johnsfine/DecodeIR.html#Sony8 
// Sony protocol can only be 8, 12, 15, or 20 bits in length.
bool IRdecodeSony::decode(void) {
  IRLIB_ATTEMPT_MESSAGE(F("Sony"));
  if(rawlen!=2*8+2 && rawlen!=2*12+2 && rawlen!=2*15+2 && rawlen!=2*20+2) return RAW_COUNT_ERROR;
  if(!decodeGeneric(0, 600*4, 600, 600*2, 600, 600,0)) return false;
  decode_type = SONY;
  return true;
}

/*
 * The next several decoders were added by Chris Young. They illustrate some of the special cases
 * that can come up when decoding using the generic decoder.
 */

/*
 * A very good source for protocol information is... http://www.hifi-remote.com/johnsfine/DecodeIR.html
 * I used that information to understand what they call the "Panasonic old" protocol which is used by
 * Scientific Atlanta cable boxes. That website uses a very strange notation called IRP notation.
 * For this protocol, the notation was:
 * {57.6k,833}<1,-1|1,-3>(4,-4,D:5,F:6,~D:5,~F:6,1,-???)+ 
 * This indicates that the frequency is 57.6, the base length for the pulse is 833
 * The first part of the <x,-x|x,-x> section tells you what a "0" is and the second part
 * tells you what a "1" is. That means "0" is 833 on, 833 off while an "1" is 833 on
 * followed by 833*3=2499 off. The section in parentheses tells you what data gets sent.
 * The protocol begins with header consisting of 4*833 on and 4*833 off. The other items 
 * describe what the remaining data bits are.
 * It reads as 5 device bits followed by 6 function bits. You then repeat those bits complemented.
 * It concludes with a single "1" bit followed by and an undetermined amount of blank space.
 * This makes the entire protocol 5+6+5+6= 22 bits long since we don't encode the stop bit.
 * The "+" at the end means you only need to send it once and it can repeat as many times as you want.
 */
bool IRdecodePanasonic_Old::decode(void) {
  IRLIB_ATTEMPT_MESSAGE(F("Panasonic_Old"));
  if(!decodeGeneric(48,833*4,833*4,0,833,833*3,833)) return false;
  /*
   * The protocol spec says that the first 11 bits described the device and function.
   * The next 11 bits are the same thing only it is the logical Bitwise complement.
   * Many protocols have such check features in their definition but our code typically doesn't
   * perform these checks. For example NEC's least significant 8 bits are the complement of 
   * of the next more significant 8 bits. While it's probably not necessary to error check this, 
   * you can un-comment the next 4 lines of code to do this extra checking.
   */
//  long S1= (value & 0x0007ff);       // 00 0000 0000 0111 1111 1111 //00000 000000 11111 111111
//  long S2= (value & 0x3ff800)>> 11;  // 11 1111 1111 1000 0000 0000 //11111 111111 00000 000000
//  S2= (~S2) & 0x0007ff;
//  if (S1!=S2) return IRLIB_REJECTION_MESSAGE(F("inverted bit redundancy"));
  // Success
  decode_type = PANASONIC_OLD;
  return true;
}

bool IRdecodeNECx::decode(void) {
  IRLIB_ATTEMPT_MESSAGE(F("NECx"));  
  if(!decodeGeneric(68,564*8,564*8,0,564,564*3,564)) return false;
  decode_type = NECX;
  return true;
}

// JVC does not send any header if there is a repeat.
bool IRdecodeJVC::decode(void) {
  IRLIB_ATTEMPT_MESSAGE(F("JVC"));
  if(!decodeGeneric(36,525*16,525*8,0,525,525*3,525)) 
  {
     IRLIB_ATTEMPT_MESSAGE(F("JVC Repeat"));
     if (rawlen==34) 
     {
        if(!decodeGeneric(0,525,0,0,525,525*3,525))
           {return IRLIB_REJECTION_MESSAGE(F("JVC repeat failed generic"));}
        else {
 //If this is a repeat code then IRdecodeBase::decode fails to add the most significant bit
           if (MATCH(rawbuf[4],(525*3))) 
           {
              value |= 0x8000;
           } 
           else
           {
             if (!MATCH(rawbuf[4],525)) return DATA_SPACE_ERROR(525);
           }
        }
        bits++;
     }
     else return RAW_COUNT_ERROR;
  } 
  decode_type =JVC;
  return true;
}

/*
 * The remaining protocols from the original IRremote library require special handling
 * This routine gets one undecoded level at a time from the raw buffer.
 * The RC5/6 decoding is easier if the data is broken into time intervals.
 * E.g. if the buffer has MARK for 2 time intervals and SPACE for 1,
 * successive calls to getRClevel will return MARK, MARK, SPACE.
 * offset and used are updated to keep track of the current position.
 * t1 is the time interval for a single bit in microseconds.
 * Returns ERROR if the measured time interval is not a multiple of t1.
 */
IRdecodeRC::RCLevel IRdecodeRC::getRClevel(unsigned char *used, const unsigned int t1) {
  if (offset >= rawlen) {
    // After end of recorded buffer, assume SPACE.
    return SPACE;
  }
  unsigned int width = rawbuf[offset];
  IRdecodeRC::RCLevel val;
  if ((offset) % 2) val=MARK; else val=SPACE;
  
  unsigned char avail;
  if (MATCH(width, t1)) {
    avail = 1;
  } 
  else if (MATCH(width, 2*t1)) {
    avail = 2;
  } 
  else if (MATCH(width, 3*t1)) {
    avail = 3;
  } 
  else {
    if((IgnoreHeader) && (offset==1) && (width<t1))
	  avail =1;
	else{
      return ERROR;}
  }
  (*used)++;
  if (*used >= avail) {
    *used = 0;
    (offset)++;
  }
  return val;   
}

#define MIN_RC5_SAMPLES 11
#define MIN_RC6_SAMPLES 1

bool IRdecodeRC5::decode(void) {
  IRLIB_ATTEMPT_MESSAGE(F("RC5"));
  if (rawlen < MIN_RC5_SAMPLES + 2) return RAW_COUNT_ERROR;
  offset = 1; // Skip gap space
  data = 0;
  used = 0;
  // Get start bits
  if (getRClevel(&used, RC5_T1) != MARK) return HEADER_MARK_ERROR(RC5_T1);
//Note: Original IRremote library incorrectly assumed second bit was always a "1"
//bit patterns from this decoder are not backward compatible with patterns produced
//by original library. Uncomment the following two lines to maintain backward compatibility.
  //if (getRClevel(&used, RC5_T1) != SPACE) return HEADER_SPACE_ERROR(RC5_T1);
  //if (getRClevel(&used, RC5_T1) != MARK) return HEADER_MARK_ERROR(RC5_T1);
  for (nbits = 0; offset < rawlen; nbits++) {
    RCLevel levelA = getRClevel(&used, RC5_T1); 
    RCLevel levelB = getRClevel(&used, RC5_T1);
    if (levelA == SPACE && levelB == MARK) {
      // 1 bit
      data = (data << 1) | 1;
    } 
    else if (levelA == MARK && levelB == SPACE) {
      // zero bit
      data <<= 1;
    } 
    else return DATA_MARK_ERROR(RC5_T1);
  }
  // Success
  bits = 13;
  value = data;
  decode_type = RC5;
  return true;
}

bool IRdecodeRC6::decode(void) {
  IRLIB_ATTEMPT_MESSAGE(F("RC6"));
  if (rawlen < MIN_RC6_SAMPLES) return RAW_COUNT_ERROR;
  // Initial mark
  if (!IgnoreHeader) {
    if (!MATCH(rawbuf[1], RC6_HDR_MARK)) return HEADER_MARK_ERROR(RC6_HDR_MARK);
  }
  if (!MATCH(rawbuf[2], RC6_HDR_SPACE)) return HEADER_SPACE_ERROR(RC6_HDR_SPACE);
  offset=3;//Skip gap and header
  data = 0;
  used = 0;
  // Get start bit (1)
  if (getRClevel(&used, RC6_T1) != MARK) return DATA_MARK_ERROR(RC6_T1);
  if (getRClevel(&used, RC6_T1) != SPACE) return DATA_SPACE_ERROR(RC6_T1);
  for (nbits = 0; offset < rawlen; nbits++) {
    RCLevel levelA, levelB; // Next two levels
    levelA = getRClevel(&used, RC6_T1); 
    if (nbits == 3) {
      // T bit is double wide; make sure second half matches
      if (levelA != getRClevel(&used, RC6_T1)) return TRAILER_BIT_ERROR(RC6_T1);
    } 
    levelB = getRClevel(&used, RC6_T1);
    if (nbits == 3) {
      // T bit is double wide; make sure second half matches
      if (levelB != getRClevel(&used, RC6_T1)) return TRAILER_BIT_ERROR(RC6_T1);
    } 
    if (levelA == MARK && levelB == SPACE) { // reversed compared to RC5
      // 1 bit
      data = (data << 1) | 1;
    } 
    else if (levelA == SPACE && levelB == MARK) {
      // zero bit
      data <<= 1;
    } 
    else {
      return DATA_MARK_ERROR(RC6_T1); 
    } 
  }
  // Success
  bits = nbits;
  value = data;
  decode_type = RC6;
  return true;
}

/*
 * This Hash decoder is based on IRhashcode
 * Copyright 2010 Ken Shirriff
 * For details see http://www.righto.com/2010/01/using-arbitrary-remotes-with-arduino.html
 * Use FNV hash algorithm: http://isthe.com/chongo/tech/comp/fnv/#FNV-param
 * Converts the raw code values into a 32-bit hash code.
 * Hopefully this code is unique for each button.
 */
#define FNV_PRIME_32 16777619UL
#define FNV_BASIS_32 2166136261UL
// Compare two tick values, returning 0 if newval is shorter,
// 1 if newval is equal, and 2 if newval is longer
int IRdecodeHash::compare(unsigned int oldval, unsigned int newval) {
  if (newval < oldval * .8) return 0;
  if (oldval < newval * .8) return 2;
  return 1;
}

bool IRdecodeHash::decode(void) {
  hash = FNV_BASIS_32;
  for (int i = 1; i+2 < rawlen; i++) {
    hash = (hash * FNV_PRIME_32) ^ compare(rawbuf[i], rawbuf[i+2]);
  }
//note: does not set decode_type=HASH_CODE nor "value" because you might not want to.
  return true;
}

/* We have created a new receiver base class so that we can use its code to implement
 * additional receiver classes in addition to the original IRremote code which used
 * 50us interrupt sampling of the input pin. See IRrecvLoop and IRrecvPCI classes
 * below. IRrecv is the original receiver class with the 50us sampling.
 */
IRrecvBase::IRrecvBase(unsigned char recvpin)
{
  irparams.recvpin = recvpin; //note: irparams.recvpin is atomically safe since it cannot be modified after object creation 
  Init();
}
void IRrecvBase::Init(void) {
  irparams.LEDblinkActive = false;
  Mark_Excess=100;
}

unsigned char IRrecvBase::getPinNum(void){
  return irparams.recvpin;
}

/* Any receiver class must implement a GetResults method that will return true when a complete code
 * has been received. At a successful end of your GetResults code you should then call IRrecvBase::GetResults
 * and it will manipulate the data inside irparams.rawbuf1 (same as decoder.rawbuf), which would have 
 * already been copied into irparams.rawbuf1 from irparams.rawbuf2, if double-buffered, by the ISR. 
 * Some receivers provide results in rawbuf1 measured in ticks on some number of microseconds while others
 * return results in actual microseconds. If you use ticks then you should pass a multiplier
 * value in Time_per_Ticks, in order to convert ticks to us.
 */
bool IRrecvBase::GetResults(IRdecodeBase *decoder, const unsigned int Time_per_Tick) {
  decoder->Reset();//clear out any old values.
/* Typically IR receivers over-report the length of a mark and under-report the length of a space.
 * This routine adjusts for that by subtracting Mark_Excess from recorded marks and
 * adding it to recorded spaces. The amount of adjustment used to be defined in IRLibMatch.h.
 * It is now user adjustable with the old default of 100;
 * NB: By copying the the values from irparams to decoder we can call IRrecvBase::resume 
 * immediately while decoding is still in progress, IF AND ONLY IF you are using an external buffer. 
 * See the function "IRdecodeBase::UseExtnBuf" for more info. If you are not using an external buffer,
 * you must wait until decoding is complete before resuming, or else you risk over-writing the very data
 * you are trying to decode.
 */
  //ensure atomic access to volatile variables; irparams is volatile  
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    decoder->rawlen = irparams.rawlen1;
    for(unsigned char i=0; i<decoder->rawlen; i++) 
    {
      //Note: even indices are marks, odd indices are spaces. Subtract Mark_Exces from marks and add it to spaces.
      //-GS UPDATE Note: 29 Jan 2016: decoder->rawbuf now points to the *same buffer* as irparams.rawbuf1, so they are actually interchangeable. 
      decoder->rawbuf[i]=decoder->rawbuf[i]*Time_per_Tick + ( (i % 2)? -Mark_Excess:Mark_Excess);
    }
  }
  return true;
}

void IRrecvBase::enableIRIn(void) { 
  pinMode(irparams.recvpin, INPUT_PULLUP); //many IR receiver datasheets recommend a >10~20K pullup resistor from the output line to 5V; using INPUT_PULLUP does just that
  resume();
}

//Note: resume is called by IRrecvBase::enableIRIn
void IRrecvBase::resume() {
  //ensure atomic access to volatile variables 
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {  
    irparams.rawlen1 = irparams.rawlen2 = 0;
    irparams.dataStateChangedToReady = false; //initialize, for use by IRrecvPCI 
  }
}

/* This receiver uses no interrupts or timers. Other interrupt driven receivers
 * allow you to do other things and call GetResults at your leisure to see if perhaps
 * a sequence has been received. Typically you would put GetResults in your loop
 * and it would return false until the sequence had been received. However because this
 * receiver uses no interrupts, it takes control of your program when you call GetResults
 * and doesn't let go until it's got something to show you. The advantage is you don't need
 * interrupts which would make it easier to use and nonstandard hardware and will allow you to
 * use any digital input pin. Timing of this routine is only as accurate as your "micros();"
 * GS Notes: double-buffer doesn't make sense for IRrecvLoop, so we will store data directly
 * into irparams.rawbuf1 directly, whereas an ISR would store it into irparams.rawbuf2 instead.
 */
bool IRrecvLoop::GetResults(IRdecodeBase *decoder) {
  bool Finished=false;
  byte OldState=HIGH;byte NewState;
  unsigned long StartTime, DeltaTime, EndTime;
  StartTime=micros();
  while(irparams.rawlen1<RAWBUF) {  //While the buffer not overflowing
    while(OldState==(NewState=digitalRead(irparams.recvpin))) { //While the pin hasn't changed
      if( (DeltaTime = (EndTime=micros()) - StartTime) > 10000) { //If it's a very long wait
        if((Finished=irparams.rawlen1)) break; //finished unless it's the opening gap
      }
    }
    if(Finished) break;
	do_Blink(!NewState);
    irparams.rawbuf1[irparams.rawlen1++]=DeltaTime;
    OldState=NewState;StartTime=EndTime;
  };
  IRrecvBase::GetResults(decoder);
  return true;
}
#ifdef USE_ATTACH_INTERRUPTS
/* This receiver uses the pin change hardware interrupt to detect when your input pin
 * changes state. It gives more detailed results than the 50us interrupts of IRrecv
 * and theoretically is more accurate than IRrecvLoop. However because it only detects
 * pin changes, it doesn't always know when it's finished. GetResults attempts to detect
 * a long gap of space but sometimes the next signal gets there before GetResults notices.
 * This means the second set of signals can get messed up unless there is a long gap.
 * This receiver is based in part on Arduino firmware for use with AnalysIR IR signal analysis
 * software for Windows PCs. Many thanks to the people at http://analysir.com for their 
 * assistance in developing this section of code.
 */

IRrecvPCI::IRrecvPCI(unsigned char inum) {
  Init();
  intrnum=inum;
  irparams.recvpin=Pin_from_Intr(inum);
}

//---------------------------------------------------------------------------------------
//checkForEndOfIRCode
//By Gabriel Staples (www.ElectricRCAircraftGuy.com) on 27 Jan 2016
//-a global function for use inside and outside an ISR, by IRrecvPCI
//-this is a non-reentrant function, since it contains static variables & is shared with an ISR, so 
// whenever you call it from outside an ISR, ***put atomic guards around the whole function call, AND
// around the portion of code just before that, where dt is determined.*** See IRrecvPCI::GetResults 
// for an example
//--ie: this function will also be called by IRrecPCI::GetResults, outside of ISRs, so be sure 
//  to use atomic access guards!
//-pass in the IR receiver input pin pinState, and the time elapsed (dt) in us since the last Mark or Space
// edge occurred. 
//-returns true if dataStateChangedToReady==true, which means dataStateisReady just transitioned 
// from false to true, so the user can now decode the results 
//---------------------------------------------------------------------------------------
//whoIsCalling defines:
#define CALLED_BY_USER (0)
#define CALLED_BY_ISR (1)
bool checkForEndOfIRCode(bool pinState, unsigned long dt, byte whoIsCalling)
{
  //local variables 
  static bool dataStateIsReady_old = true; //the previous data state last time this function was called; initialize to true
  bool dataStateIsReady; 
  bool dataStateChangedToReady = false;

  //Check for long Space to indicate end of IR code 
  //-if the USER is calling this function, we want the pinState to be HIGH (SPACE_START), and dt to be long, to consider this to be the end of the IR code; if pinState transitions from HIGH to LOW, and dt is long, we will let the ISR catch and handle it, rather than the user's call
  //-if the ISR is calling this function, we want the pinState to be LOW (MARK_START), and dt to be long , to consider this to be the end of the IR code, since the ISR is only called when pin state *transitions* occur 
  if ((whoIsCalling==CALLED_BY_ISR && pinState==MARK_START && dt >= 10000) || 
      (whoIsCalling==CALLED_BY_USER && pinState==HIGH && dt >= 10000)) //a long SPACE gap (10ms or more) just occurred; this indicates the end of a complete IR code 
  {
    dataStateIsReady = true; //the current data state; true since we just detected the end of the IR code 
    
    //check for a *change of state*; only copy the buffer if the change of state just went from false to true
    //-necessary since this function can be repeatedly called by a user via GetResults, regardless of whether or not any new data comes in 
    if (dataStateIsReady==true && dataStateIsReady_old==false)
    {
      //data is now ready to be decoded
      dataStateChangedToReady = true; 
      
      if (whoIsCalling==CALLED_BY_ISR)
        irparams.dataStateChangedToReady = true; //used to notify the user that data state just changed to ready, next time the user calls GetResults
      else if (whoIsCalling==CALLED_BY_USER)
        irparams.dataStateChangedToReady = false; //this whole function will return true, but since the user is reading this now (calling this whole function from within GetResults), and can choose to act on it to decode the data now, it gets immediately reset back to false; otherwise, the user would accidentally try to decode the same data more than once simply by repeatedly calling GetResults rapidly. 
      
      if (irparams.doubleBuffered==true)
      {
        //copy buffer from primary (rawlen2) to secondary (rawlen) buffer; the secondary buffer will be waiting for the user to decode it, while the primary buffer will be written in by this ISR as any new data comes in 
        for(unsigned char i=0; i<irparams.rawlen2; i++) 
          irparams.rawbuf1[i] = irparams.rawbuf2[i];
        irparams.rawlen1 = irparams.rawlen2;
        irparams.rawlen2 = 0; //start of a new IR code 
      }
      else //irparams.doubleBuffered==false; for single-buffering:
      {
        irparams.pauseISR = true; //since single-buffered only, we must pause the reception of data until decoding the current data is complete
        //no need to copy anything from irparams.rawbuf2 to irparams.rawbuf1, because when single-buffered, irparams.rawbuf2 points to irparams.rawbuf1 anyway, so they are the same buffer
      }
    }
  }
  else //end of IR code NOT found yet 
  {
    dataStateIsReady = false; 
  }
  dataStateIsReady_old = dataStateIsReady; //update 
  
  return dataStateChangedToReady;
} //end of checkForEndOfIRCode

//---------------------------------------------------------------------------------------
//Pin Change Interrupt handler/ISR 
//Completely rewritten by Gabriel Staples (www.ElectricRCAircraftGuy.com) on 26 Jan 2016
//-no state machine is used for this ISR anymore 
//-raw codes are optionally double-buffered, in which case raw code values are continually received, and never ignored, 
// even if the user has never called GetResults, or has not called it in a long time 
//-the secondary buffer is rawbuf2; a *pointer* to it is stored in irparams, and it is what 
// is used by the ISR to continually store new IR Mark and Space raw values as they come in.
//--the secondary buffer must be *externally created* by the user in their Arduino sketch,
//  and only a *pointer* to it is passed in to irparams; Refer to the 
//  example sketch called "IRrecvPCIDump_UseNoTimers.ino" for an example.
//-the primary buffer is rawbuf1; it is stored in irparams and passed to the decode 
// routines whenever a full sequence is ready to be decoded, and the user calls GetResults.
//-The ISR automatically copies the secondary buffer (rawbuf2) to the primary buffer (rawbuf1)
// whenever a full IR code has been received, which is noted by a long space (HIGH pd
// on the IR receiver output pin) of >10ms. 
//---------------------------------------------------------------------------------------
//-Note: HIGH times are either: A) dead-time between codes, or B) code Spaces 
//       LOW times are code Marks
//Defines: now defined in IRLibMatch.h 
//#define MARK_START (LOW) //this edge indicates the start of a mark, and the end of a space, in the IR code sequence
//                         //we are LOW now, so we were HIGH before 
//#define SPACE_START (HIGH) //this edge indicates the start of a space, and the end of a mark, in the IR code sequence
//                           //we are HIGH now, so we were LOW before
void IRrecvPCI_Handler()
{
  if (irparams.pauseISR==true)
    return; //don't process new data if the ISR reception of IR data is paused; pausing is necessary if single-buffered, until old data is decoded, so that it won't be overwritten 
  
  //local vars
  unsigned long t_now = micros(); //us; time stamp this edge
  bool pinState = digitalRead(irparams.recvpin);
  unsigned long t_old = irparams.timer; //us; time stamp last edge (previous time stamp)
  
  //blink LED 
  do_Blink(!pinState);
  
  //check time elapsed 
  unsigned long dt = t_now - t_old; //us; time elapsed ("delta time")
  //Note: if pinState==MARK_START, dt = lastSpaceTime
  //      if pinState==SPACE_START, dt = lastMarkTime
  if (dt < MINIMUM_TIME_GAP_PERMITTED) //consider this last pulse to be noise; ignore it
  {
    return;
  }
  checkForEndOfIRCode(pinState,dt,CALLED_BY_ISR);
  
  //else pinState==MARK_START && (MINIMUM_TIME_GAP_PERMITTED <= dt < 10000), OR pinState==SPACE_START && (dt >= MINIMUM_TIME_GAP_PERMITTED)
  //process the data by storing the time gap (dt) Mark or Space value 
  irparams.rawbuf2[irparams.rawlen2] = dt; 
  irparams.rawlen2++;
  if (irparams.rawlen2>=RAWBUF)
    irparams.rawlen2 = RAWBUF - 1; //constrain to just keep overwriting the last value, until the start of a new code can be identified again 
  
  irparams.timer = t_now; //us; update 
} //end of IRrecvPCI_Handler()

void IRrecvPCI::enableIRIn(void) {
  IRrecvBase::enableIRIn();
  //set up External interrupt 
  attachInterrupt(intrnum, IRrecvPCI_Handler, CHANGE);
}

//---------------------------------------------------------------------------------------
//IRrecvPCI::GetResults 
//Completely rewritten by Gabriel Staples (www.ElectricRCAircraftGuy.com) on 27 Jan 2016
//-returns true if data is received & ready to be decoded, false otherwise 
//-see the notes just above IRrecvPCI_Handler() & checkForEndOfIRCode for more info. 
//---------------------------------------------------------------------------------------
bool IRrecvPCI::GetResults(IRdecodeBase *decoder) 
{
  bool newDataJustIn = false; 
  
  //Order is important here ~GS:
  //1) first, check to see if irparams.dataStateChangedToReady==true, so we don't needlessly call checkForEndOfIRCode() if data is already ready 
  if (irparams.dataStateChangedToReady==true) //variable is a singe byte; already atomic; atomic guards not needed 
    newDataJustIn = true;
  else //2) manually check for long Space at end of IR code
  {
    //ensure atomic access to volatile variables
    //-NB: as a MINIMUM, all calcs for dt, *and* the entire checkForEndOfIRCode function call, must be inside the ATOMIC_BLOCK
    //-Note: since digitalRead is slow, I'd like to keep it *outside* the ATOMIC_BLOCK, *if possible*. Here, it *is* possible. Let's consider a case where a pin change interrupt occurs after reading the pinState: I read pinState, an interrupt occurs (pinState changes), I enter the ATOMIC_BLOCK, calculdate dt, and pass in the WRONG pinState but the RIGHT dt to the checkForEndOfIRCode function. What will happen?
    //--Answer: the ISR would have already correctly processed the whole thing, and since checkForEndOfIRCode checks pinState *and* dt, so long as one of those is correct, the same IR code data won't be accidentally processed twice. We should be ok. In this scenario, the dt calcs, *and* the checkForEndOfIRCode, however, *MUST* be inside the *same* ATOMIC_BLOCK for everything to work right. That's why I have done that below.
    bool pinState = digitalRead(irparams.recvpin); //already atomic since irparams.recvpin is one byte 
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
      unsigned long dt = micros() - irparams.timer; //us since last edge; note: irparams.timer contains the last time stamp for a Mark or Space edge 
      newDataJustIn = checkForEndOfIRCode(pinState,dt,CALLED_BY_USER);
    }
  }
  //3) if new data is ready, process it 
  if (newDataJustIn==true)
    IRrecvBase::GetResults(decoder); //mandatory to call whenever a new IR data packet is ready to be decoded; this copies volatile data from the secondary buffer into the decoder, while subtracting Mark_Exces from Marks, and adding it to Spaces, among other things
    
  return newDataJustIn;
};

 /* This class facilitates detection of frequency of an IR signal. Requires a TSMP58000
 * or equivalent device connected to the hardware interrupt pin.
 * Create an instance of the object passing the interrupt number.
 */
//These variables MUST be volatile since they are used both in and outside ISRs
volatile unsigned FREQUENCY_BUFFER_TYPE *IRfreqTimes; //non-volatile pointer to volatile data (http://www.barrgroup.com/Embedded-Systems/How-To/C-Volatile-Keyword)
volatile unsigned char IRfreqCount;
IRfrequency::IRfrequency(unsigned char inum) {  //Note this is interrupt number, not pin number
  intrnum=inum;
  pin= Pin_from_Intr(inum);
  //ISR cannot be passed parameters. If I declare the buffer global it would
  //always eat RAN even if this object was not declared. So we make global pointer
  //and copy the address to it. ISR still puts data in the object.
  IRfreqTimes = &(Time_Stamp[0]);
};

// Note ISR handler cannot be part of a class/object
void IRfreqISR(void) {
   IRfreqTimes[IRfreqCount++]=micros();
}

void IRfrequency::enableFreqDetect(void){
  attachInterrupt(intrnum,IRfreqISR, FALLING);
  //ensure atomic access to volatile variables 
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    for(i=0; i<256; i++) Time_Stamp[i]=0; //Time_Stamp is volatile
    IRfreqCount=0; //volatile variable 
  }
  Results=0.0;
  Samples=0;
};
/* Test to see if we have collected at least one full buffer of data.
 * Note values are always zeroed before beginning so any non-zero data
 * in the final elements means we have collected at least a buffer full.
 * By chance the final might be zero so we test two of them. Would be
 * nearly impossible for two consecutive elements to be zero unless
 * we had not yet collected data.
 */
bool IRfrequency::HaveData(void) {
  bool dataIsReceived;
  //ensure atomic access to volatile variables 
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    dataIsReceived = Time_Stamp[255] || Time_Stamp[254];
  }
  return dataIsReceived;
};

void IRfrequency::disableFreqDetect(void){
  detachInterrupt(intrnum);
 };

//compute the incoming frequency in kHz and store into public variable IRfrequency.Results
void IRfrequency::ComputeFreq(void){
  Samples=0; Sum=0;
  for(i=1; i<256; i++) {
    unsigned char Interval;
    //ensure atomic access to volatile variables; Time_Stamp is volatile here
    //UNNECESSARY HERE: Interval is a single byte and is therefore already atomic 
    // ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    // {
      Interval=Time_Stamp[i]-Time_Stamp[i-1];
    // }
    if(Interval>50 || Interval<10) continue;//ignore extraneous results where freq is outside the 20~100kHz range
                                            //Note: 1/50us = 20kHz; 1/10us = 100khz
    Sum+=Interval;//accumulate usable intervals
    Samples++;    //account usable intervals
  };
  if(Sum)
    Results=(double)Samples/(double)Sum*1000; //kHz
  else
    Results= 0.0;
 };
 
//Didn't need to be a method that we made one following example of IRrecvBase
unsigned char IRfrequency::getPinNum(void) {
  return pin;
}

void IRfrequency::DumpResults(bool Detail) {
  ComputeFreq();
#ifdef USE_DUMP
  Serial.print(F("Number of samples:")); Serial.print(Samples,DEC);
  Serial.print(F("\t  Total interval (us):")); Serial.println(Sum,DEC); 
  Serial.print(F("Avg. interval(us):")); Serial.print(1.0*Sum/Samples,2);
  Serial.print(F("\t Aprx. Frequency(kHz):")); Serial.print(Results,2);
  Serial.print(F(" (")); Serial.print(int(Results+0.5),DEC);
  Serial.println(F(")"));
  if(Detail) {
    for(i=1; i<256; i++) {
      unsigned int Interval;
      //ensure atomic access to volatile variables; Time_Stamp is volatile 
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
      {
        Interval=Time_Stamp[i]-Time_Stamp[i-1];
      }
      Serial.print(Interval,DEC); Serial.print("\t");
      if ((i % 4)==0)Serial.print(F("\t "));
      if ((i % 8)==0)Serial.println();
      if ((i % 32)==0)Serial.println();
    }
    Serial.println();
  }
#else
  DumpUnavailable(); 
#endif
};
#endif // ifdef USE_ATTACH_INTERRUPTS
 
/*
 * The remainder of this file is all related to interrupt handling and hardware issues. It has 
 * nothing to do with IR protocols. You need not understand this is all you're doing is adding 
 * new protocols or improving the receiving, decoding and sending of protocols.
 */

//See IRLib.h comment explaining this function
 unsigned char Pin_from_Intr(unsigned char inum) {
  static const PROGMEM uint8_t attach_to_pin[]= {
#if defined(__AVR_ATmega256RFR2__)//Assume Pinoccio Scout
	4,5,SCL,SDA,RX1,TX1,7
#elif defined(__AVR_ATmega32U4__) //Assume Arduino Leonardo
	3,2,0,1,7
#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)//Assume Arduino Mega 
	2,3, 21, 20, 1, 18
#else	//Assume Arduino Uno or other ATmega328
	2, 3
#endif
  };
#if defined(ARDUINO_SAM_DUE)
  return inum;
#endif
  if (inum<sizeof attach_to_pin) {//note this works because we know it's one byte per entry
	return (byte)pgm_read_byte(&(attach_to_pin[inum]));
  } else {
    return 255;
  }
}

// Provides ISR
#include <avr/interrupt.h>
// defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif
#define CLKFUDGE 5      // fudge factor for clock interrupt overhead
#ifdef F_CPU
#define SYSCLOCK F_CPU     // main Arduino clock
#else
#define SYSCLOCK 16000000  // main Arduino clock
#endif
#define PRESCALE 8      // timer clock prescale
#define CLKSPERUSEC (SYSCLOCK/PRESCALE/1000000)   // timer clocks per microsecond

#include <IRLibTimer.h>

/* 
 * This section contains the hardware specific portions of IRrecvBase
 */
/* If your hardware is set up to do both output and input but your particular sketch
 * doesn't do any output, this method will ensure that your output pin is low
 * and doesn't turn on your IR LED or any output circuit.
 */
void IRrecvBase::No_Output (void) {
#if defined(IR_SEND_PWM_PIN)
 pinMode(IR_SEND_PWM_PIN, OUTPUT);  
 digitalWrite(IR_SEND_PWM_PIN, LOW); // When not sending PWM, we want it low    
#endif
}

//enable/disable blinking of any arbitrary LED pin whenever IR data comes in 
void IRrecvBase::setBlinkLED(uint8_t pinNum, bool blinkActive)
{
  //atomic access guards not required since these LED parameters are all single bytes 
  //These masks, ports, etc, will be used for auto-mapped direct port access to blink the LED 
  //-this is *much* faster than digitalWrite 
  irparams.LEDpinNum = pinNum;
  irparams.LEDbitMask = digitalPinToBitMask(pinNum);
  irparams.LEDp_PORT_out = portOutputRegister(digitalPinToPort(pinNum));
  irparams.LEDblinkActive = blinkActive;
  if (blinkActive)
     pinMode(irparams.LEDpinNum,OUTPUT);
  else //LEDblinkActive==false 
  {
    pinMode(irparams.LEDpinNum,INPUT);
    fastDigitalWrite(irparams.LEDp_PORT_out, irparams.LEDbitMask, LOW); //digitalWrite to LOW to ensure INPUT_PULLUP is NOT on
  }
}

//kept for backwards compatibility
//-same as setBlinkLED, except the LED is forced to be LED_BUILTIN, which is usually LED 13 
//-see here for info on LED_BUILTIN: https://www.arduino.cc/en/Reference/Constants
void IRrecvBase::blink13(bool blinkActive)
{
  this->setBlinkLED(LED_BUILTIN, blinkActive);
}

//Do the actual blinking off and on
//This is not part of IRrecvBase because it may need to be inside an ISR
//and we cannot pass parameters to them.
void do_Blink(bool blinkState) {
  //atomic access guards not required since these LED parameters are all single bytes and hence, already atomic; also, if this method is called within an ISR, of course it is atomic, as interrupts are by default disabled inside ISRs.
  if (irparams.LEDblinkActive)
    fastDigitalWrite(irparams.LEDp_PORT_out, irparams.LEDbitMask, blinkState);
}

/* If not using the IRrecv class but only using IRrecvPCI or IRrecvLoop you can eliminate
 * some timer conflicts with the duplicate definition of ISR by turning off USE_IRRECV.
 * To do this, simply go to IRLib.h and comment out "#define USE_IRRECV". 
 */
//===========================================================================================
#ifdef USE_IRRECV
//===========================================================================================
/*
 * The original IRrecv which uses 50us timer driven interrupts to sample input pin.
 */
void IRrecv::resume() {
  // initialize state machine variables
  //ensure atomic access to volatile variables 
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    irparams.rcvstate = STATE_IDLE;
  }
  IRrecvBase::resume();
}

void IRrecv::enableIRIn(void) {
  IRrecvBase::enableIRIn();
  // setup pulse clock timer interrupt
  cli();
  IR_RECV_CONFIG_TICKS();
  IR_RECV_ENABLE_INTR;
  sei();
}

bool IRrecv::GetResults(IRdecodeBase *decoder) {
  rcvstate_t rcvstate;
  //ensure atomic access to volatile variables 
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    rcvstate = irparams.rcvstate;
  } 
  if (rcvstate != STATE_STOP) return false;
  //else:
  IRrecvBase::GetResults(decoder,USECPERTICK);
  return true;
}

#define _GAP 5000 // Minimum map between transmissions
#define GAP_TICKS (_GAP/USECPERTICK)
/*
 * This interrupt service routine is only used by IRrecv and may or may not be used by other
 * extensions of the IRrecBase. It is timer driven interrupt code to collect raw data.
 * Widths of alternating SPACE, MARK are recorded in rawbuf2. Recorded in ticks of 50 microseconds.
 * rawlen2 counts the number of entries recorded so far. First entry is the SPACE between transmissions.
 * As soon as a SPACE gets long, ready is set, state switches to IDLE, timing of SPACE continues.
 * As soon as first MARK arrives, gap width is recorded, ready is cleared, and new logging starts.
 */
ISR(IR_RECV_INTR_NAME)
{
  enum irdata_t {IR_MARK=0, IR_SPACE=1};
  irdata_t irdata = (irdata_t)digitalRead(irparams.recvpin);
  irparams.timer++; // One more 50us tick
  if (irparams.rawlen2 >= RAWBUF) {
    // Buffer overflow
    irparams.rcvstate = STATE_STOP;
  }
  switch(irparams.rcvstate) {
  case STATE_IDLE: // In the middle of a gap
    if (irdata == IR_MARK) {
      if (irparams.timer < GAP_TICKS) {
        // Not big enough to be a gap.
        irparams.timer = 0;
      } 
      else {
        // gap just ended, record duration and start recording transmission
        irparams.rawlen2 = 0;
        irparams.rawbuf2[irparams.rawlen2++] = irparams.timer;
        irparams.timer = 0;
        irparams.rcvstate = STATE_MARK;
      }
    }
    break;
  case STATE_MARK: // timing MARK
    if (irdata == IR_SPACE) {   // MARK ended, record time
      irparams.rawbuf2[irparams.rawlen2++] = irparams.timer;
      irparams.timer = 0;
      irparams.rcvstate = STATE_SPACE;
    }
    break;
  case STATE_SPACE: // timing SPACE
    if (irdata == IR_MARK) { // SPACE just ended, record it
      irparams.rawbuf2[irparams.rawlen2++] = irparams.timer;
      irparams.timer = 0;
      irparams.rcvstate = STATE_MARK;
    } 
    else { // SPACE
      if (irparams.timer > GAP_TICKS) {
        // big SPACE, indicates gap between codes
        // Mark current code as ready for processing
        // Switch to STOP
        // Don't reset timer; keep counting space width
        irparams.rcvstate = STATE_STOP;
      } 
    }
    break;
  case STATE_STOP: // waiting, measuring gap
    if (irdata == IR_MARK) { // reset gap timer
      irparams.timer = 0;
    }
    break;
  }
  do_Blink(!(bool)irdata);
}
#endif //end of ifdef USE_IRRECV
/*
 * The hardware specific portions of IRsendBase
 */
void IRsendBase::enableIROut(unsigned char khz) {
//NOTE: the comments on this routine accompanied the original early version of IRremote library
//which only used TIMER2. The parameters defined in IRLibTimer.h may or may not work this way.
  // Enables IR output.  The khz value controls the modulation frequency in kilohertz.
  // The IR output will be on pin 3 (OC2B).
  // This routine is designed for 36-40KHz; if you use it for other values, it's up to you
  // to make sure it gives reasonable results.  (Watch out for overflow / underflow / rounding.)
  // TIMER2 is used in phase-correct PWM mode, with OCR2A controlling the frequency and OCR2B
  // controlling the duty cycle.
  // There is no prescaling, so the output frequency is 16MHz / (2 * OCR2A)
  // To turn the output on and off, we leave the PWM running, but connect and disconnect the output pin.
  // A few hours staring at the ATmega documentation and this will all make sense.
  // See my Secrets of Arduino PWM at http://www.righto.com/2009/07/secrets-of-arduino-pwm.html for details.
  
  // Disable the Timer2 Interrupt (which is used for receiving IR)
 IR_RECV_DISABLE_INTR; //Timer2 Overflow Interrupt    
 pinMode(IR_SEND_PWM_PIN, OUTPUT);  
 digitalWrite(IR_SEND_PWM_PIN, LOW); // When not sending PWM, we want it low    
 IR_SEND_CONFIG_KHZ(khz);
 }

IRsendBase::IRsendBase () {
 pinMode(IR_SEND_PWM_PIN, OUTPUT);  
 digitalWrite(IR_SEND_PWM_PIN, LOW); // When not sending PWM, we want it low    
}

//The Arduino built in function delayMicroseconds has limits we wish to exceed
//Therefore we have created this alternative
void  My_delay_uSecs(unsigned int T) {
  if(T){if(T>16000) {delayMicroseconds(T % 1000); delay(T/1000); } else delayMicroseconds(T);};
}

void IRsendBase::mark(unsigned int time) {
 IR_SEND_PWM_START;
 IR_SEND_MARK_TIME(time);
 Extent+=time;
}

void IRsendBase::space(unsigned int time) {
 IR_SEND_PWM_STOP;
 My_delay_uSecs(time);
 Extent+=time;
}

/*
 * Various debugging routines
 */


#ifdef IRLIB_TRACE
void IRLIB_ATTEMPT_MESSAGE(const __FlashStringHelper * s) {Serial.print(F("Attempting ")); Serial.print(s); Serial.println(F(" decode:"));};
void IRLIB_TRACE_MESSAGE(const __FlashStringHelper * s) {Serial.print(F("Executing ")); Serial.println(s);};
byte IRLIB_REJECTION_MESSAGE(const __FlashStringHelper * s) { Serial.print(F(" Protocol failed because ")); Serial.print(s); Serial.println(F(" wrong.")); return false;};
byte IRLIB_DATA_ERROR_MESSAGE(const __FlashStringHelper * s, unsigned char index, unsigned int value, unsigned int expected) {  
 IRLIB_REJECTION_MESSAGE(s); Serial.print(F("Error occurred with rawbuf[")); Serial.print(index,DEC); Serial.print(F("]=")); Serial.print(value,DEC);
 Serial.print(F(" expected:")); Serial.println(expected,DEC); return false;
};
#endif
