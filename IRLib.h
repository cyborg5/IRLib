/* IRLib.h from IRLib – an Arduino library for infrared encoding and decoding
 * Version 1.51   March 2015
 * Copyright 2014 by Chris Young http://cyborg5.com
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

#ifndef IRLib_h
#define IRLib_h
#include <Arduino.h>

// The following are compile-time library options.
// If you change them, recompile the library.
// If IRLIB_TRACE is defined, some debugging information about the decode will be printed
// IRLIB_TEST must be defined for the IRtest unittests to work.  It will make some
// methods virtual, which will be slightly slower, which is why it is optional.
//#define IRLIB_TRACE
// #define IRLIB_TEST

/* If not using the IRrecv class but only using IRrecvPCI or IRrecvLoop you can eliminate
 * some conflicts with the duplicate definition of ISR by turning this feature off.
 * Comment out the following define to eliminate the conflicts.
 */
#define USE_IRRECV
/* Similarly some other libraries have conflicts with the built in Arduino functions
 * "attachInterrupt()" and "detachInterrupt()" which are used by the IRrecvPCI and
 * IRfrequency classes. If you're not using either of those classes and get conflicts
 * related to INT0_vect then comment out the following line to eliminate the conflicts.
 */
 #define USE_ATTACH_INTERRUPTS
/* If not using either DumpResults methods of IRdecode nor IRfrequency you can
 * comment out the following define to eliminate considerable program space.
 */
#define USE_DUMP

// Only used for testing; can remove virtual for shorter code
#ifdef IRLIB_TEST
#define VIRTUAL virtual
#else
#define VIRTUAL
#endif

#define RAWBUF 100 // Length of raw duration buffer (cannot exceed 255)

typedef char IRTYPES; //formerly was an enum
#define UNKNOWN 0
#define NEC 1
#define SONY 2
#define RC5 3
#define RC6 4
#define PANASONIC_OLD 5
#define JVC 6
#define NECX 7
//#define ADDITIONAL (number) //make additional protocol 8 and change HASH_CODE to 9
#define HASH_CODE 8
#define LAST_PROTOCOL HASH_CODE

const __FlashStringHelper *Pnames(IRTYPES Type); //Returns a character string that is name of protocol.

// Base class for decoding raw results
class IRdecodeBase
{
public:
  IRdecodeBase(void);
  IRTYPES decode_type;           // NEC, SONY, RC5, UNKNOWN etc.
  unsigned long value;           // Decoded value
  unsigned char bits;            // Number of bits in decoded value
  volatile unsigned int *rawbuf; // Raw intervals in microseconds
  unsigned char rawlen;          // Number of records in rawbuf.
  bool IgnoreHeader;             // Relaxed header detection allows AGC to settle
  virtual void Reset(void);      // Initializes the decoder
  virtual bool decode(void);     // This base routine always returns false override with your routine
  bool decodeGeneric(unsigned char Raw_Count, unsigned int Head_Mark, unsigned int Head_Space, 
                     unsigned int Mark_One, unsigned int Mark_Zero, unsigned int Space_One, unsigned int Space_Zero);
  virtual void DumpResults (void);
  void UseExtnBuf(void *P); //Normally uses same rawbuf as IRrecv. Use this to define your own buffer.
  void copyBuf (IRdecodeBase *source);//copies rawbuf and rawlen from one decoder to another
protected:
  unsigned char offset;           // Index into rawbuf used various places
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
  RCLevel getRClevel(unsigned char *used, const unsigned int t1);
protected:
  unsigned char nbits;
  unsigned char used;
  long data;
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
// , public virtual IRdecodeADDITIONAL //add additional protocols here
{
public:
  virtual bool decode(void);    // Calls each decode routine individually
};

//Base class for sending signals
class IRsendBase
{
public:
  IRsendBase();
  void sendGeneric(unsigned long data,  unsigned char Num_Bits, unsigned int Head_Mark, unsigned int Head_Space, 
                   unsigned int Mark_One, unsigned int Mark_Zero, unsigned int Space_One, unsigned int Space_Zero, 
				   unsigned char kHz, bool Stop_Bits, unsigned long Max_Extent=0);
protected:
  void enableIROut(unsigned char khz);
  VIRTUAL void mark(unsigned int usec);
  VIRTUAL void space(unsigned int usec);
  unsigned long Extent;
  unsigned char OnTime,OffTime,iLength;//used by bit-bang output.
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
  void send(unsigned int buf[], unsigned char len, unsigned char khz);
};

class IRsendRC5: public virtual IRsendBase
{
public:
  void send(unsigned long data);
};

class IRsendRC6: public virtual IRsendBase
{
public:
  void send(unsigned long data, unsigned char nbits);
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
// , public virtual IRsendADDITIONAL //add additional protocols here
{
public:
  void send(IRTYPES Type, unsigned long data, unsigned int data2);
};

// Changed this to a base class so it can be extended
class IRrecvBase
{
public:
  IRrecvBase(void) {};
  IRrecvBase(unsigned char recvpin);
  void No_Output(void);
  void blink13(bool blinkflag);
  bool GetResults(IRdecodeBase *decoder, const unsigned int Time_per_Ticks=1);
  void enableIRIn(void);
  virtual void resume(void);
  unsigned char getPinNum(void);
  unsigned char Mark_Excess;
protected:
  void Init(void);
};

/* Original IRrecv class uses 50µs interrupts to sample input. While this is generally
 * accurate enough for everyday purposes, it may be difficult to port to other
 * hardware unless you know a lot about hardware timers and interrupts. Also
 * when trying to analyze unknown protocols, the 50µs granularity may not be sufficient.
 * In that case use either the IRrecvLoop or the IRrecvPCI class.
 */
#ifdef USE_IRRECV
class IRrecv: public IRrecvBase
{
public:
  IRrecv(unsigned char recvpin):IRrecvBase(recvpin){};
  bool GetResults(IRdecodeBase *decoder);
  void enableIRIn(void);
  void resume(void);
};
#endif
/* This receiver uses no interrupts or timers. Other interrupt driven receivers
 * allow you to do other things and call GetResults at your leisure to see if perhaps
 * a sequence has been received. Typically you would put GetResults in your loop
 * and it would return false until the sequence had been received. However because this
 * receiver uses no interrupts, it takes control of your program when you call GetResults
 * and doesn't let go until it's got something to show you. The advantage is you don't need
 * interrupts which would make it easier to use and nonstandard hardware and will allow you to
 * use any digital input pin. Timing of this routine is only as accurate as your "micros();"
 */
class IRrecvLoop: public IRrecvBase
{
public:
  IRrecvLoop(unsigned char recvpin):IRrecvBase(recvpin){};
  bool GetResults(IRdecodeBase *decoder);
};

/* This receiver uses the pin change hardware interrupt to detect when your input pin
 * changes state. It gives more detailed results than the 50µs interrupts of IRrecv
 * and theoretically is more accurate than IRrecvLoop. However because it only detects
 * pin changes, it doesn't always know when it's finished. GetResults attempts to detect
 * a long gap of space but sometimes the next signal gets there before GetResults notices.
 * This means the second set of signals can get messed up unless there is a long gap.
 * This receiver is based in part on Arduino firmware for use with AnalysIR IR signal analysis
 * software for Windows PCs. Many thanks to the people at http://analysir.com for their 
 * assistance in developing this section of code.
 */
#ifdef USE_ATTACH_INTERRUPTS
class IRrecvPCI: public IRrecvBase
{
public:
  //Note this is interrupt number not pin number
  IRrecvPCI(unsigned char inum);
  bool GetResults(IRdecodeBase *decoder);
  void resume(void);
private:
  unsigned char intrnum;
};

/* This class facilitates detection of frequency of an IR signal. Requires a TSMP58000
 * or equivalent device connected to the hardware interrupt pin.
 * Create an instance of the object passing the interrupt number.
 */
//Un-comment only one of the following three lines depending on available RAM
//#define FREQUENCY_BUFFER_TYPE unsigned char
#define FREQUENCY_BUFFER_TYPE int
//#define FREQUENCY_BUFFER_TYPE long
class IRfrequency
{
public:
  //Note this is interrupt number, not pin number
  IRfrequency(unsigned char inum);
  void enableFreqDetect(void);
  bool HaveData(void);      //detective data received
  void disableFreqDetect(void);
  void ComputeFreq(void);	//computes but does not print results
  void DumpResults(bool Detail);	//computes and prints result
  unsigned char getPinNum(void);//get value computed from interrupt number
  double Results; //results in kHz
  unsigned char Samples; //number of samples used in computation
private:
  volatile unsigned FREQUENCY_BUFFER_TYPE Time_Stamp[256];
  unsigned char intrnum, pin;
  unsigned int i;
  unsigned long Sum;
};
#endif // ifdef USE_ATTACH_INTERRUPTS

//Do the actual blinking off and on
//This is not part of IRrecvBase because it may need to be inside an ISR
//and we cannot pass parameters to them.
void do_Blink(void);

/* This routine maps interrupt numbers used by attachInterrupt() into pin numbers.
 * NOTE: these interrupt numbers which are passed to “attachInterrupt()” are not 
 * necessarily identical to the interrupt numbers in the datasheet of the processor 
 * chip you are using. These interrupt numbers are a system unique to the 
 * “attachInterrupt()” Arduino function.  It is used by both IRrecvPCI and IRfrequency.
 */
unsigned char Pin_from_Intr(unsigned char inum);
// Some useful constants
// Decoded value for NEC when a repeat code is received
#define REPEAT 0xffffffff


#endif //IRLib_h
