/* IRLib.h from IRLib – an Arduino library for infrared encoding and decoding
 * Version 1.1   April 2013
 * Copyright 2013 by Chris Young http://cyborg5.com
 *
 * This library is a major rewrite of IRemote by Ken Shirriff which was covered by
 * GNU LESSER GENERAL PUBLIC LICENSE which as I read it allows me to make modified versions.
 * That same license applies to this modified version. See his original copyright below.
 * The latest Ken Shirriff code can be found at https://github.com/shirriff/Arduino-IRremote
 * My purpose was to reorganize the code to make it easier to add or remove protocols.
 * As a result I have separated the act of receiving a set of raw timing codes from the act of decoding them
 * by making them separate classes. That way the receiving aspect can be more black box and implementers
 * of decoders and senders can just deal with the decoding of protocols.
 * Also added provisions to make the classes base classes that could be extended with new protocols
 * which would not require recompiling of the original library nor understanding of its detailed contents.
 * Some of the changes were made to reduce code size such as unnecessary use of long versus bool.
 * Some changes were just my weird programming style. Also extended debugging information added.
 */
/*
 * IRremote
 * Version 0.1 July, 2009
 * Copyright 2009 Ken Shirriff
 * For details, see http://arcfn.com/2009/08/multi-protocol-infrared-remote-library.htm http://arcfn.com
 *
 * Interrupt code based on NECIRrcv by Joe Knapp
 * http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1210243556
 * Also influenced by http://zovirl.com/2008/11/12/building-a-universal-remote-with-an-arduino/
 */

#ifndef IRLib_h
#define IRLib_h
#include <Arduino.h>

// The following are compile-time library options.
// If you change them, recompile the library.
// If DEBUG is defined, a lot of debugging output will be printed during decoding.
// If TRACE is defined, some debugging information about the decode will be printed
// TEST must be defined for the IRtest unittests to work.  It will make some
// methods virtual, which will be slightly slower, which is why it is optional.
// #define DEBUG
// #define TRACE
// #define TEST

// Only used for testing; can remove virtual for shorter code
#ifdef TEST
#define VIRTUAL virtual
#else
#define VIRTUAL
#endif

#define RAWBUF 100 // Length of raw duration buffer
#define USE_TIMER1 1  //should be "1" for timer 1, should be "0" for timer 2

enum IRTYPES {UNKNOWN, NEC, SONY, RC5, RC6, PANASONIC_OLD, JVC, NECX, HASH_CODE, LAST_PROTOCOL=HASH_CODE};

const __FlashStringHelper *Pnames(IRTYPES Type); //Returns a character string that is name of protocol.

// Base class for decoding raw results
class IRdecodeBase
{
public:
  IRdecodeBase(void);
  IRTYPES decode_type;           // NEC, SONY, RC5, UNKNOWN etc.
  unsigned long value;           // Decoded value
  int bits;                      // Number of bits in decoded value
  volatile unsigned int *rawbuf; // Raw intervals in .5 us ticks
  int rawlen;                    // Number of records in rawbuf.
  virtual void Reset(void);      // Initializes the decoder
  virtual bool decode(void);     // This base routine always returns false override with your routine
  bool decodeGeneric(int Raw_Count,int Head_Mark,int Head_Space, int Mark_One, int Mark_Zero, int Space_One,int Space_Zero);
  unsigned long Interval_uSec(int index);
  virtual void DumpResults (void);
  void UseExtnBuf(void *P); //Normally uses same rawbuf as IRrecv. Use this to define your own buffer.
  void copyBuf (IRdecodeBase *source);//copies rawbuf and rawlen from one decoder to another
};

class IRdecodeHash: public virtual IRdecodeBase
{
public:
  unsigned long hash;
  virtual bool decode(void);//made virtual in case you want to substitute your own hash code
protected:
  int compare(unsigned int oldval, unsigned int newval);//used by decodeHash
};


class IRdecodeNEC: public virtual IRdecodeBase 
{
public:
  virtual bool decode(void);
};

class IRdecodeSony: public virtual IRdecodeBase 
{
public:
  virtual bool decode(void);
};

class IRdecodeRC: public virtual IRdecodeBase 
{
public:
  enum RCLevel {MARK, SPACE, ERROR};//used by decoders for RC5/RC6
  // These are called by decode
  RCLevel getRClevel(int *offset, int *used, int t1);
};

class IRdecodeRC5: public virtual IRdecodeRC 
{
public:
  virtual bool decode(void);
};

class IRdecodeRC6: public virtual IRdecodeRC
{
public:
  virtual bool decode(void);
};

class IRdecodePanasonic_Old: public virtual IRdecodeBase 
{
public:
  virtual bool decode(void);
};

class IRdecodeJVC: public virtual IRdecodeBase 
{
public:
  virtual bool decode(void);
};

class IRdecodeNECx: public virtual IRdecodeBase 
{
public:
  virtual bool decode(void);
};

// main class for decoding all supported protocols
class IRdecode: 
public virtual IRdecodeNEC,
public virtual IRdecodeSony,
public virtual IRdecodeRC5,
public virtual IRdecodeRC6,
public virtual IRdecodePanasonic_Old,
public virtual IRdecodeJVC,
public virtual IRdecodeNECx
{
public:
  virtual bool decode(void);    // Calls each decode routine individually
};

//Base class for sending signals
class IRsendBase
{
public:
  IRsendBase() ;
  void sendGeneric(unsigned long data, int Num_Bits, int Head_Mark, int Head_Space, int Mark_One, int Mark_Zero, int Space_One, int Space_Zero, int mHz, bool Stop_Bits);
protected:
  void enableIROut(int khz);
  VIRTUAL void mark(int usec);
  VIRTUAL void space(int usec);
};

class IRsendNEC: public virtual IRsendBase
{
public:
  void send(unsigned long data);
};

class IRsendSony: public virtual IRsendBase
{
public:
  void send(unsigned long data, int nbits);
};

class IRsendRaw: public virtual IRsendBase
{
public:
  void send(unsigned int buf[], int len, int hz);
};

class IRsendRC5: public virtual IRsendBase
{
public:
  void send(unsigned long data);
};

class IRsendRC6: public virtual IRsendBase
{
public:
  void send(unsigned long data, int nbits);
};

class IRsendPanasonic_Old: public virtual IRsendBase
{
public:
  void send(unsigned long data);
};

class IRsendJVC: public virtual IRsendBase
{
public:
  void send(unsigned long data, bool First);
};

class IRsendNECx: public virtual IRsendBase
{
public:
  void send(unsigned long data);
};

class IRsend: 
public virtual IRsendNEC,
public virtual IRsendSony,
public virtual IRsendRaw,
public virtual IRsendRC5,
public virtual IRsendRC6,
public virtual IRsendPanasonic_Old,
public virtual IRsendJVC,
public virtual IRsendNECx
{
public:
  void send(IRTYPES Type, unsigned long data, int nbits);
};


// main class for receiving IR
class IRrecv
{
public:
  IRrecv(int recvpin);
  void No_Output(void);
  void blink13(int blinkflag);
  bool GetResults(IRdecodeBase *decoder);
  void enableIRIn();
  void resume();
};

// Some useful constants
// Decoded value for NEC when a repeat code is received
#define REPEAT 0xffffffff


#endif //IRLib_h
