/* IRLibRData.h from IRLib – an Arduino library for infrared encoding and decoding
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

#ifndef IRLibRData_h
#define IRLibRData_h

/*
 * The structure contains a variety of variables needed by the receiver routines.
 * Typically this data would be part of the IRrecv class however the interrupt service routine
 * must have access to it and you cannot pass a parameter to such a routine. The data must be global.
 * You need not include this file unless you are creating a custom receiver class or extending
 * the provided IRrecv class.
 */

// receiver states
enum rcvstate_t {STATE_UNKNOWN, STATE_IDLE, STATE_MARK, STATE_SPACE, STATE_STOP, STATE_RUNNING};

// information for the interrupt handlers (ISRs)
typedef struct {
  unsigned char recvpin;    // pin for IR data from detector
  rcvstate_t rcvstate;       // state machine
  unsigned long timer;     // state timer, counts 50uS ticks.(and other uses); for IRrecvPCI, this is the last time stamp (us) when a Mark or Space edge occurred 
  
  /*
  Buffers:
  By Gabriel Staples, 29 Jan. 2016
  
  Double-buffer definitions:
  1) Primary Buffer = rawbuf1 - this buffer is accessed directly by the decoder during decoding; ie: the data contained here is what is decoded, in order to obtain a numerical value from any given set of IR code Mark & Space pulses 
  2) Secondary Buffer = rawbuf2 - this buffer is accessed directly by the ISR, storing new data into it as data comes in 
  
  Double-buffer Notes:
  -The user enables double-buffered data by passing in an external buffer through the decoder's IRdecodeBase::useDoubleBuffer method 
  --When you call IRdecodeBase::useDoubleBuffer, rawbuf2 will be assigned to point to the external buffer passed in.
  -The ISR will always store data directly into rawbuf2 
  -When using only *one* buffer (double-buffer not enabled), rawbuf2 will point to rawbuf1, therefore, the ISR is actually storing data directly into rawbuf1 
  -the decoder (IRdecodeBase) contains a "rawbuf" pointer; this pointer will *always* point to rawbuf1 (the *internal* buffer), NOT rawbuf2 (the *external* buffer)
  --therefore, when IRdecodeBase::useDoubleBuffer has been called, its rawbuf points to rawbuf1, which is *different* from rawbuf2; when useDoubleBuffer has NOT been called, its rawbuf points to rawbuf1, which is the same as rawbuf2 since rawbuf2 will in this case point to rawbuf1 
  -when IRrecvBase::getResults manipulates the buffer, keep in mind that irparams.rawbuf1 is the *same buffer* as decoder->rawbuf, so these two ways to access the buffer are interchangeable 
  */
  
  unsigned int rawbuf1[RAWBUF]; //raw data (time periods) for Marks (IR receiver output LOW), and Spaces (IR receiver output HIGH)
  unsigned char rawlen1; //counter of entries in rawbuf1
  
  //extra variables used by IRrecvPCI & other ISR-based IRrecv'ers (for double-buffered data --> no missed incoming signals): 
  bool doubleBuffered; //true if an external buffer has been passed in to IRdecodeBase::useDoubleBuffer, false otherwise 
  bool pauseISR; //set to true to cause the ISR to *not* store new, incoming IR data, until the previous data is decoded; this is necessary only when running single-buffered (ie: when doubleBuffered==false);
  volatile unsigned int* volatile rawbuf2; //GS added; a volatile pointer to volatile data--an extra buffer; this will become the *secondary* buffer, written do by the IRrecvPCI ISR, for example, while rawbuf1 will remain the *primary* buffer, accessed directly during decoding. This pointer will point to an external buffer that the user must create in their main sketch for use with IRrecvPCI; the user will pass this buffer in via the IRdecodeBase::useDoubleBuffer method. 
  unsigned char rawlen2; //corresponds to the length of rawbuf2, above; used by IRrecvPCI when double-buffered 
  bool dataStateChangedToReady; //GS added; IR code buffer *change* state: true if dataStateIsReady (found inside checkForEndOfIRCode()) just made a transition from false to true; false otherwise. This may seem redundant, but it is not. dataStateIsReady indicates the present state, dataStateChangedToReady indicates state transitions. We only want My_Receiver.getResults to return true if the data state *transitioned* from false to true (ie: dataStateChangedToReady==true), so that we only decode a given set of data once. If getResults returned true just because dataStateIsready==true, then if you rapidly called getResults again and again it would keep wasting time decoding and returning the same set of data again and again, rather than decoding and returning each set of data only *once.* 
  
  //for LED blinking 
  uint8_t LEDpinNum;
  uint8_t LEDbitMask;
  volatile uint8_t* volatile LEDp_PORT_out; //volatile pointer to volatile data - http://www.barrgroup.com/Embedded-Systems/How-To/C-Volatile-Keyword
  bool LEDblinkActive; //set true to enable blinking of LED pinNum as IR data comes in
} 
irparams_t;
extern volatile irparams_t irparams;
#endif
