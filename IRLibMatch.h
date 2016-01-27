/* IRLibMatch.h from IRLib – an Arduino library for infrared encoding and decoding
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

#ifndef IRLibMatch_h
#define IRLibMatch_h

/*
 * Originally all timing comparisons for decoding were based on a percent of the
 * target value. However when target values are relatively large, the percent tolerance
 * is too much.  In some instances an absolute tolerance is needed. In order to maintain
 * backward compatibility, the default will be to continue to use percent. If you wish to default
 * to an absolute tolerance, you should comment out the line below.
 */
#define IRLIB_USE_PERCENT

/*
 * These are some miscellaneous definitions that are needed by the decoding routines. 
 * You need not include this file unless you are creating custom decode routines 
 * which will require these macros and definitions. Even if you include it, you probably
 * don't need to be intimately familiar with the internal details.
 */

#define USECPERTICK 50  // microseconds per clock interrupt tick
#define PERCENT_TOLERANCE 25  // percent tolerance in measurements
#define DEFAULT_ABS_TOLERANCE 75 //absolute tolerance in microseconds
#define MINIMUM_TIME_GAP_PERMITTED 100 //us; minimum Mark or Space period permitted; GS: for use in IRrecvPCI: if a Mark or Space is less than this value I will filter it out, as if it never occurred.

/* 
 * These revised MATCH routines allow you to use either percentage or absolute tolerances.
 * Use ABS_MATCH for absolute and PERC_MATCH for percentages. The original MATCH macro
 * is controlled by the IRLIB_USE_PERCENT definition a few lines above.
 */
 
#define PERCENT_LOW(us) (unsigned int) (((us)*(1.0 - PERCENT_TOLERANCE/100.)))
#define PERCENT_HIGH(us) (unsigned int) (((us)*(1.0 + PERCENT_TOLERANCE/100.) + 1))

#define ABS_MATCH(v,e,t) ((v) >= ((e)-(t)) && (v) <= ((e)+(t)))
#define PERC_MATCH(v,e) ((v) >= PERCENT_LOW(e) && (v) <= PERCENT_HIGH(e))

#ifdef IRLIB_USE_PERCENT
#define MATCH(v,e) PERC_MATCH(v,e)
#else
#define MATCH(v,e) ABS_MATCH(v,e,DEFAULT_ABS_TOLERANCE)
#endif

//The following two routines are no longer necessary because mark/space adjustments are done elsewhere
//These definitions maintain backward compatibility.
#define MATCH_MARK(t,u) MATCH(t,u)
#define MATCH_SPACE(t,u) MATCH(t,u)

#ifdef IRLIB_TRACE
void IRLIB_ATTEMPT_MESSAGE(const __FlashStringHelper * s);
void IRLIB_TRACE_MESSAGE(const __FlashStringHelper * s);
byte IRLIB_REJECTION_MESSAGE(const __FlashStringHelper * s);
byte IRLIB_DATA_ERROR_MESSAGE(const __FlashStringHelper * s, unsigned char index, unsigned int value, unsigned int expected);
#define RAW_COUNT_ERROR IRLIB_REJECTION_MESSAGE(F("number of raw samples"));
#define HEADER_MARK_ERROR(expected) IRLIB_DATA_ERROR_MESSAGE(F("header mark"),offset,rawbuf[offset],expected);
#define HEADER_SPACE_ERROR(expected) IRLIB_DATA_ERROR_MESSAGE(F("header space"),offset,rawbuf[offset],expected);
#define DATA_MARK_ERROR(expected) IRLIB_DATA_ERROR_MESSAGE(F("data mark"),offset,rawbuf[offset],expected);
#define DATA_SPACE_ERROR(expected) IRLIB_DATA_ERROR_MESSAGE(F("data space"),offset,rawbuf[offset],expected);
#define TRAILER_BIT_ERROR(expected) IRLIB_DATA_ERROR_MESSAGE(F("RC5/RC6 trailer bit length"),offset,rawbuf[offset],expected);
#else
#define IRLIB_ATTEMPT_MESSAGE(s)
#define IRLIB_TRACE_MESSAGE(s)
#define IRLIB_REJECTION_MESSAGE(s) false
#define IRLIB_DATA_ERROR_MESSAGE(s,i,v,e) false
#define RAW_COUNT_ERROR false
#define HEADER_MARK_ERROR(expected) false
#define HEADER_SPACE_ERROR(expected) false
#define DATA_MARK_ERROR(expected) false
#define DATA_SPACE_ERROR(expected) false
#define TRAILER_BIT_ERROR(expected) false
#endif

//For identifying Marks & Spaces in IR codes:
//-used by IRrecvPCI's ISR, for example 
//Note:  HIGH times are either: A) dead-time between codes, or B) code Spaces 
//       LOW times are code Marks
#define MARK_START (LOW) //this edge indicates the start of a mark, and the end of a space, in the IR code sequence
                         //we are LOW now, so we were HIGH before 
#define SPACE_START (HIGH) //this edge indicates the start of a space, and the end of a mark, in the IR code sequence
                           //we are HIGH now, so we were LOW before
                           
//FOR SPEED PROFILING WITH OSCILLOSCOPE
//-normally, keep this all commented out; uncomment when you need to use an oscilloscope to check the speed or timing of something
//-Don't forget that a direct-port-access write like this takes 2 clock cycles, or 0.125 us, so you have to subtract
//2 clock cycles (NOT 4) from whatever time period you profile with the oscilloscope.
#define PROFILE_PIN3_OUTPUT pinMode(3,OUTPUT)
#define PROFILE_PIN3_HIGH   PORTD |= _BV(3) //write Arduino pin D3 HIGH
#define PROFILE_PIN3_LOW    PORTD &= ~_BV(3) //write Arduino pin D3 LOW

#endif //IRLibMatch_h