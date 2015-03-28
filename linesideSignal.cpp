/*  linesideSignal.cpp
    Version a1 - March 2015 - alpha release
	This library provides support for driving LED-based model railroad signals using charlieplexing.
    
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
    
    Released into the public domain.
    
    Note: this library contains timing that presumes an Arduino operating at 16 MHz. It may 
    work with faster ones, but using it with slower ones would require some adjustment and
    would be very limited in the number of lit signal lamps supported.
    
    This library depends on accurate results from the millis() and microseconds() functions.
    Sketches must not modify the timer behavior affecting these.
    
    *****
    Signals as defined in this library
    
    Signals are defined in terms of masts, heads and lamps, with arbitrary ordinals (0 - 255).
    The lamp ordinal is relative to the head (i.e., each head can have lamp #0) and the
    head ordinal is relative to the mast. Ordinals do not need to be contiguous or sequential.
    You can number things 1, 2 and 3, or 0, 1, 2, or even 3, 7 and 11. Whatever you want.
    
    Each lamp has permanent attributes:
    	color
    	anode and cathode pins powering it
    	
    And also transient state information:
    	is currently lit (user set)
    	is flashing (only applies if lit) (user set)
    	is flashing on an alternate cycle (user set)
    	is just starting
    	is just turning off
    	changes on/off without a ramp time (user set)
    	is in a "hold" state pending an udpate
    	
    The "user set" attributes apply only when lit, and are cleared when a lamp is turned off
    except for the "changes without a ramp time" attribute, which remains set until cleared.
    Note that "lit" means the lamp should be lit, and remains true even when the lamp is 
    temporarily off due to flashing or ramping behavior.
    
    In particular, it does not matter to the library if a mast or head is wired common-anode 
    or common-cathode (it matters to the wiring, not the program). You simply define the 
    anode (positive) and cathode (ground or negative) pins for each LED and then either 
    turn on individual LEDs or set a color on a head. The library will handle the details 
    of deciding what pins to turn on when and with what polarities.
    
    Color may be a singular color (like red) or it can describe a bi-color or multi-color 
    LED where the color displayed depends on the polarity. Again, the library will work 
    out how to manage the pins, and the sketch calling the library just needs to specify 
    which pins reflect a given color, and which color to display.

	See class signalLamp for the complete specification and the README for examples.
	
	At present only pins 0-12 and A0-A7 (if they exist) are usable as anode or cathode. Note
	that 0 and 1 are normally used for serial I/O via USB and should be avoided. Pin 13 is
	not used to avoid interactions with the LED normally on that pin. Extending this library 
	to support the added pins for an Arduino Mega would require some additions to the pin
	manipulation code.
	
	*****
	Flashing Lamps
	
	In real signals a flashing lamp tends to pulse in intensity rather than simply turning 
	on and off. 
	
	Observing modern and slightly older flashing and changing signal lamps via YouTube 
	videos showed that flashing lights use a slow change of intensity over an interval
	rather than simply turning on and off, and that depending on the signal these would 
	sometimes go completely dark, but others merely became dim without going completely 
	dark.  The library attempts to mimic this behavior when changing the lit/not-lit state 
	of a signal lamp by means of a an intensity ramp process based on dividing the flash
	cycle into ten phases.
	
	The ramp interval is three of these phases, so for 60 FPM (one flash per second), the
	ramp interval is 0.3 seconds. During this interval intensity is varied by skipping some
	cycles when the LED should be lit. This may make flashing LEDs seem to pulse oddly with 
	faster video cameras. Once lit, the lamps remains fully lit for 3 phases, and there is 
	a single "dark" phase when the lamp is fully out.
        
    *****
    Cycle Times and Flicker
    
	It is important that each LED is lit for as long as possible (to maximize intensity despite
	the LED being dark much of the time) during each cycle, and yet the cycle must be as short
	as possible to ensure that LEDs are each lit often enough to remain below the threshold 
	to perceive flicker. The cycle interval defaults to a reasonable length but can be set 
	by a running program, and may automatically be increased from the set value if you are driving a 
	large number of lit LEDs (dark LEDs do not matter) as there is a minimum time per LED to 
	allow for switching overhead and a reasonable intensity. The cycle can be shortened if you
	are driving fewer, but with some limitations. As released, the brightness should be 
	good for 9 - 12 LEDs, and better for 6 - 9. Even 24 wouldn't look bad though, as long
	as appropriate-size resistors are used (too-large resistors will also dim a LED).
	
	LEDs switch insanely fast. The Arduino pins switch even faster; for typical LEDs they 
	go from LOW to HIGH (or vice versa) in a quarter of a microsecond or less. Send the 
	command to put the pin HIGH, and a LED is lit somewhere between 0.1 microsecond and 10 
	or 20 microseconds later. Turn off the power, and it goes dark only slightly less
	fast. 
	
	It's not quite that simple as it does take time to reach full intensity and drop off 
	to zero. It's still really fast, and when multiplexing the LEDs will be dark for the 
	majority of the cycle. There is, however, a fair amount of software overhead in each 
	LED pulse, so keeping the LED lit longer each cycle is desirable to maximize the percentage
	of time LEDs are lit.
	
	With a cycle time of 2,500 microseconds, 12 LEDs would be lit for something like 168
	microseconds each after subtracting worst-case overhead without any minimum set, or a 
	duty cycle (percentage of on time to total cycle) of 6.7%, which doesn't sound like 
	much but is enough for them to be quite bright, Six LEDs, however, would be lit for 377 
	microseconds each, or a 15% duty cycle, and would be significantly brighter.
	
	With fewer lit LEDs, you can shorten the cycle, within limits, but should probably allow 
	at least 150 microseconds total per lit LED, and twice that will be better.  This can be
	adjusted via the LSS_LED_MIN constant, which is the minimum number of milliseconds to 
	leave a LED lit each pulse.
		
	However, the speed of the program matters.  It is difficult to get around loop() in less 
	than a few hundred microseconds if you are doing anything significant outside of the 
	library calls.  And the time a LED remains lit needs to be longer than one loop time 
	(and preferably longer than 2x that).  So while you can changed LSS_LED_MIN, it is not 
	a good idea to make it shorter than the default unless you know your loop is very fast 
	(i.e., is under 100 microseconds).  
	
	Longer loops can also be an issue. The library will adjust automatically to fit the 
	number of lit signal lamps, but if you have a very long loop(), you should increase
	the cycle time (by calling setCycleTime()). If you are only lighting a few and have 
	fast loop time, you can decrease the cycle time (the library will overrule you if you 
	set it too low, but will try to remain close to your preferred time
	
	For the human eye, a flickering point source (like a LED) can be perceived if the time
	between pulses of light is around 20 msec or more (it varies by person, and oddly 
	peripheral vision detects this more easily than looking directly at the LED). Faster 
	than that, and "persistence of vision" makes your brain think it was lit continuously. 
	That means that even though the LEDs are really lit less than 10% of the time, as long 
	as cycle times are short enough, you see the mostly-dark LEDs as being always lit.
	
	Even within this, longer cycles can lead to problems. While the human eye won't notice a solidly
	lit LED flickering for very long times, cameras are another matter.  Also, a flashing 
	LED is more likely to be seen to flicker as it will skip some cycles to reduce intensity 
	during the ramp-up and ramp-down phases.
	
	For a video camera the ability to notice flicker depends on the relationship of the 
	shutter speed to the interval between successive pulses (the cycle length).
	
	A sufficiently fast camera will see any multiplexed LED flickering since sometimes it is 
	lit when the shutter is open, and sometimes not. The default cycle time is short enough
	that this usually should not happen. Most consumer cameras moderate-speed shutters that
	will stay open longer than the cycle time.
	
	Note that what matters is shutter speed, not frame rate. At 30 fps successive frames are
	taken every 33 milliseconds, but the shutter may only be open for a few milliseconds during
	that time.  Some specs I've found suggest that for typical consumer cameras (which have
	somewhat fixed shutter speeds), the threshold is around 8 msec (for 60 fps video) or 16 msec 
	(for 30 fps video). Still cameras, or high-end video with settable shutter speeds, may 
	require cycles shorter than 2.5 msec (to cope with a 1/500th-second shutter speed).

    *****
	Using the fast write library (optional):

	As of Arduino 1.0.6 this may no longer be necessary, as timings show that worst-case
	times for mode and write commands only change from 24 usec to 16 usec when the library
	is included. Since the average "LED on" interval is around 275 usec, this is only a 3%
	improvement (and most of the time it's less that that). However, I've left the 
	instructions below in case it is still desired.
	
	Note that use of this library restricts which pins can be used to the basic digital and
	analog pins of an Uno or similar (i.e., not the added pins on a Mega).

	As provided, this does not use the digitalWriteFast library to speed up pin processing 
	for the LEDs.  If you want to use this, you can uncomment the include and the define 
	for LSS_USE_FAST_WRITE, otherwise the library will use normal pin routines.

	To use the digitalWriteFast library, it needs to be downloaded from Google Code:
	
		http://code.google.com/p/digitalwritefast/downloads/detail?name=digitalWriteFastinterruptSafe.zip
		
	Note: google code is likely to shut down eventually, so this library may move to github
	or some other repository at some time in the future.
		
	After downloading, the files from the zip must be placed in your library (put the 
	digitalWriteFast folder in there, not the zip). If you are using Arduino 1.00 or later, 
	you'll need to change the first two lines of the digitalWriteFast.h file as described here:
	
		http://code.google.com/p/digitalwritefast/issues/detail?id=4
		
		Which says to use these lines instead:
		
		#if ARDUINO >= 100
		#include <Arduino.h>
		#else
		#include "WProgram.h" 
		#include <wiring.h>
		#endif
		
	Note that there are two libraries that can be used.  One is the main Arduino library where
	libraries such as EEPROM are pre-installed.  This gets replaced when you upgrade 
	Arduino (which might be an advantage if they ever standardize these routines) and is
	the library recommended by the install instructions.
	
	Alternatively you can place digitalWriteFast in the user library, which is where
	linesideSignals (and most downloaded libraries) are supposed to be installed.
	
	Depending on which library you use, uncomment a different include further down in
	this file.
*/

#include "Arduino.h"

// if we are using this library with digitalWriteFast, uncomment the following define
// and one of the two includes, depending on where you installed digitalWriteFast.
// 
//#define LSS_USE_FAST_WRITE
// uncomment if digitalWriteFast is installed in the Arduino release library (as per its instructions)
//#include "digitalWriteFast.h"
// uncomment if instead it is installed in the user library along with linesideSignal
//#include "../digitalWriteFast/digitalWriteFast.h"


#include "linesideSignal.h"


/************************ signals class routines ******************************/

// routines for the signals class
// These set and return booleans packed into a single byte in order to be as frugal as
// we can in terms of SRAM use.

// set the bit (0-15) identified by flag
void signalLamp::setBitFlag(int flag, boolean flagVal)
{
	int tempFlags;
	
#if defined(LSS_DEBUG_REPORTING)
	int oldFlags;
	
	oldFlags = _lampFlags;
#endif
	
	if (flag > 15) return; // ignore invalid bits
	
	tempFlags = _lampFlags & ~(1 << flag); // bit cleared
	
	_lampFlags = tempFlags | ((flagVal & 1) << flag); // set bit 
	
} // setBitFlag

boolean signalLamp::getBitFlag(int flag)
{
	boolean tmpVal;
	
	if (flag > 15) return(false); // ignore invalid bits

	tmpVal = ((_lampFlags & (1 << flag)) >> flag); // return bit, "true" if bit is set
	
	return(tmpVal);

} // getBitFlag

// initialize storage to zero
void signalLamp::clearBitFlags()
{
	_lampFlags = 0;
} // clearBitFlags

boolean signalLamp::isOn()
{
	return(getBitFlag(LSS_SL_ISLIT));
} // isOn


boolean signalLamp::isFlash()
{
	return(getBitFlag(LSS_SL_ISFLASH));
} // isFlash

boolean signalLamp::isReversed()
{
	return(getBitFlag(LSS_SL_ISALTERNATE));
} // isReversed

boolean signalLamp::isStart()
{	
	return(getBitFlag(LSS_SL_START));
} // isStart

boolean signalLamp::isStop()
{	
	return(getBitFlag(LSS_SL_STOP));
} // isStop

boolean signalLamp::usesRamp()
{	
	return(getBitFlag(LSS_SL_RAMP));
} // usesRamp

boolean signalLamp::onHold()
{	
	return(getBitFlag(LSS_SL_DELAY));
} // onHold

/************************ linesideSignal class routines ******************************/


// class constructor - runs before the sketch setup to initialize an instance of the class
// Hardware and global data structures may not be initialized when this is run, put 
// anything like a write in the setup routine instead of here.
linesideSignal::linesideSignal()
{
  // do nothing here beyond initializing simple variables
  
	_setupIsDone = false;
	
	_lastLampCount = 0;		// no lamps lit

	_cycleTime = LSS_CYCLE_TIME; 	// initial cycle time (required for setFlashRate)
	_targetCycleTime = LSS_CYCLE_TIME;	// no user changes to apply

	_setFlashRate(LSS_FLASH_FPM);	// initialize the rate and associated things

	_anodeOn = false;
	_cathodeOn = false;
	
	_killSwitch = false;	// we don't need to turn anything off
	_killAnode = false;
	
	_anodeCount = 0; // safety net - count active pins
	_cathodeCount = 0;
	
	_pulseTimePerLED = 0;
	
	_lastLoopStamp = 0;
	
#if defined(LSS_DEBUG_REPORTING)
    _lastBankTime = 0;
    _maxBankTime = 0;
    _minBankTime = LSS_CYCLE_TIME;
    _minCycleTime = LSS_CYCLE_TIME;
    _maxCycleTime = 0;
#endif

#if defined(LSS_DEBUG_NOLEDS)
	_suppressLEDs = true;
#else
	_suppressLEDs = false;
#endif
	
	_lightTimerStart(1L, 0); // start with timers expired

	_lampList = NULL;
	_currentLED = NULL;
	
	_cycleCount = 0;
	
	_averageOverhead(100); 

	_pulseTimePerLED = LSS_LED_MIN; // start off at the minimum, adjust later to optimize as we learn how long things actually take
	
} // linesideSignal constructor

// setupSignal
//
// We use this to handle things that may not be ready when the constructor is run.
//
// Initialize the general structures used for signals
void linesideSignal::setupSignal()
{

	if (_setupIsDone) return; // only do this once
	
	_setupIsDone = true;

	// initialize the list of lamps with a permanently dark lamp
	// Note that this will always be the *LAST* lamp on the list, since new ones are pushed at front.
	_addLamp(LSS_NULL_SIG, LSS_NULL_SIG, LSS_NULL_SIG, LSS_NOT_PIN, LSS_NOT_PIN, LSS_DARK);
	_currentLED = _lampList; // start with the null lamp active
	_currentLED->setBitFlag(LSS_SL_RAMP, false); // it will never need a ramp
	
	_adjCycleTime(); // make sure the cycle and pulse rate are correct (also calls setFlashRate)
		
} // setupSignal

/************************ basic private utility functions ******************************/

// polled timer routines - simple timers based on comparing times, not exact since they aren't checked
// until the loop() gets around to calling them.
//
// Note: these use signed long to allow rollover to be handled correctly - DO NOT CHANGE

// start the microsecond timer used to track LED lighting
void linesideSignal::_lightTimerStart(long usec, long startTime)
{
  if (startTime == 0) {
    _lightExpirationTime = long(micros()) + usec; // if no start specified, start now
  } else {
    _lightExpirationTime = startTime + usec;
  } 
} // lightTimerStart

// return true if the microsecond light timer has expired (note it remains expired until started again)
boolean linesideSignal::_lightTimerExpired()
{
  if ( (long)((long)micros()  - _lightExpirationTime) >= 0) {
    return ( true );
  } else {
    return( false );     
  }
} // lightTimerExpired

// Averaging routines: these compute an exponential running average, which is used as a
// means of downplaying the impact of short-lived changes.  Algorithms are based on an 
// "alpha" that is the reciprocal of a power of two (e.g., a = 1/4) to make these
// computationally fast (i.e., using shifts instead of multiplication and division).
//
// These are based on an implementation of the algorithm described at:
// http://bleaklow.com/2012/06/20/sensor_smoothing_and_optimised_maths_on_the_arduino.html

// averageOverhead
//
// Add a new data point to an exponential rolling average of values. The internal value is 
// stored as a fixed-point number, so we use a function to covert it to an int.
//
// This is a slow-adapting method, which will reflect long-term changes but damp out
// differences that occur to due transient events.
//
// It computes an exponentially weighted moving average, with alpha = 1/16.  This takes
// a very long time to fully converge to a new setting (over 100 cycles).
//
// mult by 32 div by 16 is mult by 2 (<< 1), x 15/16 is * 15, right shift 4
// max val is N x 32 x 15 (where N is the largest value), so interim must be long
// but that will handle values up to ~21 bits (~2M).
// 
void linesideSignal::_averageOverhead(int newVal) 
{
	_interimOverhead = (long(newVal) << 1) + ((_interimOverhead * 15L) >> 4);
} // smoothOffset

// getOverhead
//
// return the overhead quantity (convert from fixed point to int)
int linesideSignal::_getOverhead() 
{
	int avgVal;
	
	avgVal = int((_interimOverhead + 16L) >> 5); // convert from fixed pt
	return(avgVal);
} // getOverhead

// averageLoop
//
// Add a new datapoint to an exponential rolling average of values. The internal value is 
// stored as a fixed-point number, so we use a function to covert it to an int.
//
// The specific algorithm used, alpha = 1/4, will move to the mid-point of a change in 
// values in 3 cycles (each cycle is about one second as used here). The effect is to 
// adapt this average reasonably swiftly, but the calling program will have to override 
// this value where rapid changes could affect program behavior.
//
// alpha = 4 (1/4), fixed-point scaled by 2**5 (32), fast adjust (modified)
// mult by 32 div by 4 is mult by 8 (<< 3), x 3/4 is * 3, right shift 2
// max val is 1024 x 32 x 3 = 98,304 so interim must be long
//
// See comments above averageOverhead for additional algorithm details.
void linesideSignal::_averageLoop(int newVal) 
{
	_interimLoop = (long(newVal) << 3) + ((_interimLoop * 3L) >> 2);
} // averageLoop

// getAverageLoop
//
// return the average (convert from fixed point to int)
int linesideSignal::_getAverageLoop() 
{
	int avgVal;
	
	avgVal = int((_interimLoop + 16L) >> 5); // convert from fixed pt
	return(avgVal);
} // getAverageLoop

// dropDead
//
// Halt in an infinite loop
void linesideSignal::_dropDead()
{
	for (int i = 0; i < 1;) {} // loop forever
} // dropDead

/************************ start public functions here ******************************/

// setCycleTime
//
// Change the interval in microseconds during which we cycle through the LEDs. This routine 
// is for the user, and should not be used internally as longer cycle times than an int 
// may be used in debugging.
// 
void linesideSignal::setCycleTime(int cycle) 
{
	//int rate;
	
	if (!_setupIsDone) return; // safety net - do nothing without setup

	if ((cycle < (2*LSS_LED_MIN)) || (cycle > 20000)) return; // ignore obviously wrong numbers
	
	_targetCycleTime = cycle; // save the new cycle for reference going forward
	
	if (_targetCycleTime < _cycleTime) { // if reduced, then force adjustment to avoid ramp overrunning the flash interval
		_resetCycleTime();
	}
	
} // setCycleTime

// setFlashRate
//
// This is the externalized version of the routine to change flash rates.
void linesideSignal::setFlashRate(int rate)
{

	if (!_setupIsDone) return; // safety net - do nothing without setup
	
	if ((rate < 1) || (rate > LSS_MAX_FLASH_RATE)) return; // Limit the user to numbers that are reasonable
	
	_setFlashRate(rate);

} // setFlashRate - external

// setRamp
//
// set the value of the isRamping flag to control if the lamp uses a slow start/stop or is abrupt.
// if false, the LED will light (or go dark) immediately
void linesideSignal::setRamp(byte mastOrd, byte headOrd, byte lampOrd, boolean ramp)
{
	signalLamp *lamp;
	
	if (!_setupIsDone) return; // safety net - do nothing without setup

	lamp = _lampList;
	while (lamp != NULL) {
		if ((lamp->mastNum == mastOrd) && (lamp->headNum == headOrd) && (lamp->lampNum == lampOrd)) 
		{
			lamp->setBitFlag(LSS_SL_RAMP, ramp);
		}
		lamp = lamp->nextLamp;  // advance
	} // while
} // setRamp

// addLamp
//
// create a new LED (i.e., an anode/cathode pair). This is the main (internal) code; what is externalized
// as addLamp has additional checks on the input we bypass when using this for initial list setup.
//
// Note that color here is the permanent attribute of the lamp, not a presently-lit color
void linesideSignal::_addLamp(byte mastOrd, byte headOrd, byte lampOrd, byte anode, byte cathode, byte colorVal)
{
	// allocate a structure for the LED and initialize it
		
	signalLamp *lamp = new signalLamp; // allocate a new lamp instance
	
	lamp->mastNum = mastOrd;
	lamp->headNum = headOrd;
	lamp->lampNum = lampOrd;
	lamp->color = colorVal;		
	lamp->anode = anode; 
	lamp->cathode = cathode;
	
	// set the flags for the lamp to default values
	lamp->clearBitFlags();  
	lamp->setBitFlag(LSS_SL_ISLIT, false);
	lamp->setBitFlag(LSS_SL_ISFLASH, false);
	lamp->setBitFlag(LSS_SL_ISALTERNATE, false);
	lamp->setBitFlag(LSS_SL_START, false);
	lamp->setBitFlag(LSS_SL_STOP, false);
	lamp->setBitFlag(LSS_SL_RAMP, true);		// normal lights ramp up and down in intensity
	lamp->setBitFlag(LSS_SL_DELAY, false);
	
#if defined(LSS_DEBUG_REPORTING)
	Serial.print(F("addLamp: _lampFlags="));Serial.print(lamp->_lampFlags, BIN);Serial.println(F("."));
#endif

	// put the lamp on the linked list (for now we insert at the front)
	lamp->nextLamp = _lampList;
	_lampList = lamp; 
	
	if ((anode == LSS_NOT_PIN) || (cathode == LSS_NOT_PIN)) { // if either is invalid, disable the lamp entry (this is used for the end-of-list entry)
		lamp->anode = LSS_NOT_PIN;
		lamp->cathode = LSS_NOT_PIN;
		lamp->color = LSS_DARK;
	} else {
		pinMode(cathode, INPUT);	// make sure its off
		pinMode( anode, OUTPUT);	// ground the pin to dissipate any existing charge
		digitalWrite( anode, LOW );
		delay(1);					// give it some time to drain any built-up charge in the circuit so we start fresh
		pinMode( anode, INPUT);		// ensure pins are in high-resistance state to start 
		pinMode( cathode, OUTPUT);	// ground the pin to dissipate any existing charge
		digitalWrite( cathode, LOW );
		delay(1);					// give it some time to drain any built-up charge in the circuit so we start fresh
		pinMode( cathode, INPUT);	// ensure pins are in high-resistance state to start 
	} // valid pins

} // addLamp

// addLamp - this is the external version
//
void linesideSignal::addLamp(byte mastOrd, byte headOrd, byte lampOrd, byte anode, byte cathode, byte colorVal)
{

	if (!_setupIsDone) return; // safety net - do nothing without setup

	// sanity-check inputs - do nothing if bad values provided
	if (anode == cathode) return;
	if (!_goodPin(anode)) return;
	if (!_goodPin(cathode)) return;
	if (colorVal == LSS_DARK) return;	// we don't need to track a permanently dark lamp

	_addLamp(mastOrd, headOrd, lampOrd, anode, cathode, colorVal);

} // addLamp - external

// clearHead
//
// Turn off all LEDS on a head (and clear flashing attribute)
void linesideSignal::clearHead(byte mastOrd, byte headOrd)
{
	signalLamp *lamp;
	
	if (!_setupIsDone) return; // safety net - do nothing without setup
	
	lamp = _lampList;
	while (lamp != NULL) {
		if ((lamp->mastNum == mastOrd) && (lamp->headNum == headOrd)) {
			if (lamp->isOn() && lamp->isStart() && lamp->onHold()) { // it never actually started, so just clear it
				_goDark(lamp);
			} else if (lamp->isOn()) {
				if (!lamp->isStop()) { // ignore a second attempt to set stop
					lamp->setBitFlag(LSS_SL_STOP, true); // begin shutdown
					lamp->setBitFlag(LSS_SL_START, false);
					lamp->setBitFlag(LSS_SL_DELAY, true); // force a delay until the next cycle
					// note that this will implicitly clear other attributes when the delay is processed
				} // isStop
			}  // isOn	
		} // if mast and head match
	
		lamp = lamp->nextLamp;  // advance
	} // while
} // clearHead

// setLamp 
//
// Turn a LED on or off and optionally set or clear the "flashing" attribute.
//
// 
void linesideSignal::setLamp(byte mastOrd, byte headOrd, byte lampOrd, boolean lit, boolean flashing)
{
	signalLamp *lamp;
	
	if (!_setupIsDone) return; // safety net - do nothing without setup

	lamp = _lampList;
	while (lamp != NULL) {
		if ((lamp->mastNum == mastOrd) && (lamp->headNum == headOrd) && (lamp->lampNum == lampOrd)) {
			lamp->setBitFlag(LSS_SL_ISFLASH, flashing); // put this here so it will affect lamps already on
			if (!lamp->isOn()) {
				lamp->setBitFlag(LSS_SL_STOP, false); // begin lite-up if it wasn't already lit
				lamp->setBitFlag(LSS_SL_START, true);
				lamp->setBitFlag(LSS_SL_ISLIT, true);
				lamp->setBitFlag(LSS_SL_DELAY, true); // force a delay until the next cycle
			} // isOn
		} // match
		
		lamp = lamp->nextLamp;  // advance
	} // while
} // setLamp (full definition, five parameters)

// overload definition to allow omission of flashing parameter
void linesideSignal::setLamp(byte mastOrd, byte headOrd, byte lampOrd, boolean lit)
{
	setLamp(mastOrd, headOrd, lampOrd, lit, false);
} // setLamp (four parameters)

// setHeadColor
//
// Clear all LEDs on a head and then turn on the first of a given color
// if called with color LSS_DARK it does the first part, then fails to find any lamp to 
// activate, so it remains dark (this could be optimized with a test, but why bother?).
void linesideSignal::setHeadColor(byte mastOrd, byte headOrd, byte color, boolean flashing)
{
	signalLamp *lamp;
	
	if (!_setupIsDone) return; // safety net - do nothing without setup
	
	clearHead(mastOrd, headOrd); // turn off all the LEDs on this head
	
	if (color > LSS_LAST_FOR_SETCOLOR) return; // bad color value, ignore it

	lamp = _lampList;
	while (lamp != NULL) {
		if ((lamp->mastNum == mastOrd) && (lamp->headNum == headOrd)) {
			if ((lamp->color == color) ||
				((color == LSS_YELLOW) && ((lamp->color == LSS_GREENYELLOW) || (lamp->color == LSS_REDYELLOW)) ) ||
				((color == LSS_GREEN)  && (lamp->color == LSS_GREENYELLOW) ) ||
				((color == LSS_RED)    && (lamp->color == LSS_REDYELLOW) ) ||
				((color == LSS_YELLOW) && ((lamp->color == LSS_GREENREDYELLOW) || (lamp->color == LSS_REDGREENYELLOW)) ) ||
				((color == LSS_GREEN)  && (lamp->color == LSS_GREENREDYELLOW) ) ||
				((color == LSS_RED)    && (lamp->color == LSS_REDGREENYELLOW) ) )
				// do not need to list other colors - covered by first line of if
			{
				lamp->setBitFlag(LSS_SL_ISFLASH, flashing); // change the flashing attribute no matter what
			
				// check for a color change on a lit multi-color lamp
				if (lamp->isStop() && ((lamp->color == LSS_GREENYELLOW) || (lamp->color == LSS_REDYELLOW) || 
					(lamp->color == LSS_GREENREDYELLOW) || (lamp->color == LSS_REDGREENYELLOW) )) {
					
					lamp->setBitFlag(LSS_SL_START, true);	// ensure it restarts
				} else if (!lamp->isOn()) { // start it up if it is not already on
					lamp->setBitFlag(LSS_SL_STOP, false); // begin lite-up
					lamp->setBitFlag(LSS_SL_ISLIT, true);
					lamp->setBitFlag(LSS_SL_START, true);
					lamp->setBitFlag(LSS_SL_DELAY, true); // force a delay until the next cycle
				} else if ((lamp->isOn()) && (color == lamp->color)) { // change to same color gets down/up sequence
					lamp->setBitFlag(LSS_SL_START, true);
				}
				
			} // same color
			else if (lamp->isOn()) // same lamp ordinal but not the right color, so turn if off unless it's already stopping  (this also handles use of LSS_DARK to turn off the lamp)
			{
				if (lamp->isStart() || !lamp->usesRamp()) { // if it's starting up or we dont need to ramp - kill it
					_goDark(lamp);
				} else if (!lamp->isStop()) { // set delayed off unless its already shutting down
					lamp->setBitFlag(LSS_SL_STOP, true);
					lamp->setBitFlag(LSS_SL_START, false);
					lamp->setBitFlag(LSS_SL_DELAY, true); 		
				} // if not stopping
			} // if on and not color match
		} // if same mast and head
		lamp = lamp->nextLamp;  // advance
	} // while
} // setHeadColor (4 params)

// overload function for setHeadColor with one less parameter
void linesideSignal::setHeadColor(byte mastOrd, byte headOrd, byte color)
{
	setHeadColor(mastOrd, headOrd, color, false);
} // setHeadColor (3 params)

// setLampColor
//
// Set a lamp to one color (if it supports more than one). Can also be used to turn on a
// single-color lamp if the right color is specified.
void linesideSignal::setLampColor(byte mastOrd, byte headOrd, byte lampOrd, byte color, boolean flashing)
{
	signalLamp *lamp;
	
	if (!_setupIsDone) return; // safety net - do nothing without setup
	
	if (color > LSS_LAST_FOR_SETCOLOR) return; // bad color value, ignore it

	lamp = _lampList;
	while (lamp != NULL) {
		if ((lamp->mastNum == mastOrd) && (lamp->headNum == headOrd) && (lamp->lampNum == lampOrd)) {
			if ((lamp->color == color) ||
				((color == LSS_YELLOW) && ((lamp->color == LSS_GREENYELLOW) || (lamp->color == LSS_REDYELLOW))) ||
				((color == LSS_GREEN)  && (lamp->color == LSS_GREENYELLOW) ) ||
				((color == LSS_RED)    && (lamp->color == LSS_REDYELLOW) ) ||
				((color == LSS_YELLOW) && ((lamp->color == LSS_GREENREDYELLOW) || (lamp->color == LSS_REDGREENYELLOW))) ||
				((color == LSS_GREEN)  && (lamp->color == LSS_GREENREDYELLOW) ) ||
				((color == LSS_RED)    && (lamp->color == LSS_REDGREENYELLOW)) )
			{
				lamp->setBitFlag(LSS_SL_ISFLASH, flashing);
				
				// check for a color change on a lit multi-color lamp
				if (lamp->isStop() && ( (lamp->color == LSS_GREENYELLOW) || (lamp->color == LSS_REDYELLOW) || 
					(lamp->color == LSS_GREENREDYELLOW) || (lamp->color == LSS_REDGREENYELLOW) )) {
					
					lamp->setBitFlag(LSS_SL_START, true);	// ensure it restarts
				} else if (!lamp->isOn()) {
					lamp->setBitFlag(LSS_SL_STOP, false); // begin lite-up sequence
					lamp->setBitFlag(LSS_SL_START, true);
					lamp->setBitFlag(LSS_SL_ISLIT, true);
					lamp->setBitFlag(LSS_SL_DELAY, true); // force a delay until the next cycle
				} else if ((lamp->isOn()) && (color == lamp->color)) { // change to same color gets down/up sequence
					lamp->setBitFlag(LSS_SL_STOP, true);
					lamp->setBitFlag(LSS_SL_START, true);
				}

			} // same color
			else if (lamp->isOn()) // same lamp ordinal, so turn if off unless it's already stopping (this also handles use of LSS_DARK to turn off all lamps on the head)
			{
				if (lamp->isStart() || !lamp->usesRamp()) { // if it's starting up or we dont need to ramp - kill it
					_goDark(lamp);
				} else if (!lamp->isStop()) { // set delayed off unless its already shutting down
					lamp->setBitFlag(LSS_SL_STOP, true);
					lamp->setBitFlag(LSS_SL_START, false);
					lamp->setBitFlag(LSS_SL_DELAY, true); 		
				} // if not stopping
			} // if on and not color match
		} // same mast, head and lamp
		lamp = lamp->nextLamp;  // advance
	} // while
} // setLampColor (5 params)

// overload function for setLampColor with one less parameter
void linesideSignal::setLampColor(byte mastOrd, byte headOrd, byte lampOrd, byte color)
{
	setLampColor(mastOrd, headOrd, lampOrd, color, false);
} // setLampColor (4 params)

// setAlternate
//
// Designate this lamp as one that flashes on the other cycle from normal.  Used mainly
// for setting up warning lights such as crossing signals.
// For activating a two-light alternating flasher, call setAlternate for each light, specifying 
// alternate as true or false to create pairs of alternating lights. To turn the lights off,
// call clearHead (assuming all lights share a head).
void linesideSignal::setAlternate(byte mastOrd, byte headOrd, byte lampOrd, boolean alternate)
{
	signalLamp *lamp;
	
	if (!_setupIsDone) return; // safety net - do nothing without setup

	lamp = _lampList;
	while (lamp != NULL) {
		if ((lamp->mastNum == mastOrd) && (lamp->headNum == headOrd) && (lamp->lampNum == lampOrd)) 
		{
			lamp->setBitFlag(LSS_SL_ISLIT, true);
			lamp->setBitFlag(LSS_SL_ISFLASH, true); 
			lamp->setBitFlag(LSS_SL_ISALTERNATE, alternate);
			lamp->setBitFlag(LSS_SL_STOP, false); // 
			lamp->setBitFlag(LSS_SL_START, true);
			lamp->setBitFlag(LSS_SL_DELAY, true); // force a delay until the next cycle
		}
		lamp = lamp->nextLamp;  // advance
	} // while
} // setAlternate


/************************ start internal routines here ******************************/

// goDark
//
// clear all of the flags that need clearing when a lamp is turned off.
void linesideSignal::_goDark(signalLamp *lamp)
{
	
	lamp->setBitFlag(LSS_SL_STOP, false);	// clear the stopping flag
	lamp->setBitFlag(LSS_SL_START, false);
	lamp->setBitFlag(LSS_SL_ISLIT, false);	// and now it is really off
	lamp->setBitFlag(LSS_SL_ISFLASH, false);	// flash is always cleared for dark lamps
	lamp->setBitFlag(LSS_SL_ISALTERNATE, false);// alternating is always cleared for dark lamps
	lamp->setBitFlag(LSS_SL_DELAY, false);	// and we dont need any delay

	if ((lamp->anode == _currentLED->anode) && (lamp->cathode == _currentLED->cathode)) {						
		_killSwitch = true; // deactivate if this is the current LED
	} // current lamp
} // goDark

// getFlashRate
//
// Derive the flash rate from the half interval.
int linesideSignal::_getFlashRate()
{
	int rate;
	
	rate = int(60000000L / (2 * _flashHalfInterval)); // _flashHalfInterval is independent of _cycleTime
	
	return(rate);
} // getFlashRate

// setFlashRate
//
// Depends on the cycle time and must be adjusted any time that changes.
//
// Change the rate (in flashes per minute) at which flashing signals will cycle.  This is the internal
// version, which doesn't validate input (used in setup).
void linesideSignal::_setFlashRate(int rate)
{
	int cyclesPerFlash;
	long low, high;

	if ((rate < 1) || (rate > 6000)) return; // ignore obviously wrong numbers
	
	_flashHalfInterval = 1000L * ((60000L / rate) / 2L); // convert rate in FPM to half-cycle in usec

	cyclesPerFlash = int(float(60000.0 / rate) / float(_cycleTime / 1000.0)); // should be 400 for 60 FPM and 2500 usec cycle
	_cyclesPerDiv = cyclesPerFlash / LSS_NUM_DIV; // 40 for 60 FPM @ 2500 usec cycle 
	
	// choose the closest multiple of LSS_RAMP_CYCLES_STEPS to cyclesPerDiv as the actual cyclesPerDiv
	low = ( _cyclesPerDiv - (_cyclesPerDiv % LSS_RAMP_CYCLES_STEP) );
	high = ( (_cyclesPerDiv+LSS_RAMP_CYCLES_STEP) - (_cyclesPerDiv % LSS_RAMP_CYCLES_STEP) );
	if ((_cyclesPerDiv - low) > (high - _cyclesPerDiv) ) {
		_cyclesPerDiv = high;
	} else {
		_cyclesPerDiv = low;
	}
} // setFlashRate

// resetCycleTime
//
// Change the division and count back to the start of a ramp cycle, and update all of
// the associated timers.  This needs to be done when something changes the assumptions
// behing the current timers, or when the cycle gets to the end and needs to start over.
void linesideSignal::_resetCycleTime()
{
	_rampDiv = 0;
	_cycleCount = 0;
	_adjCycleTime();
} // resetCycleTime

// adjCycleTime
//
// This routine should only be called at the start of a new cycle (division 0) so that
// it does not affect in-process flash ramping.
//
// Update the cycle time to reflect the time it actually takes to pulse the LEDs, where "all"
// is defined as the max of either the current cycle or a running average. Whenever possible,
// use the time specified by the user (or the original default if none specified).
// 
// note: when the number of lit lamps increases suddenly (as in approach lighting), it takes
// one cycle (about a second) to adapt to the change.
void linesideSignal::_adjCycleTime()
{
	long maxCycle;
	long pulseTimeMax;
	int numLamps;
	int rate;
	
	rate = _getFlashRate(); // save the rate for later
	
	_lastLampCount = _litLampCount();
	
	numLamps = _lastLampCount; // this cycles number of lit lamps (our minimum setting)
	if (numLamps == 0) numLamps = 1;
		
	// determine the values based on the past cycle
	pulseTimeMax = long(_targetCycleTime / numLamps) - long(_getOverhead());
	if (pulseTimeMax < LSS_LED_MIN) pulseTimeMax = LSS_LED_MIN;
	maxCycle = ((pulseTimeMax + long(_getOverhead())) * long(numLamps));
		
	// attempt to use the preferred cycle time, but extend it based on the running average
	// of lit lamps over time if necessary. And in any case always allow time for the 
	// most-recently lit quantity of lamps (override the average to deal with rapid changes).

	_cycleTime = maxCycle;
	_pulseTimePerLED = pulseTimeMax;
	
	_setFlashRate(rate); // recompute the rate values to reflect the new cycle time
	
} // adjCycleTime

// litLampCount
//
// Returns the number of lamps in On state.
int linesideSignal::_litLampCount()
{
	signalLamp *lamp;
	int litCount = 0;

	lamp = _lampList;
	while (lamp != NULL) {
		if (lamp->isOn()) litCount++;
		
		lamp = lamp->nextLamp;  // advance
	} // while
	
	return(litCount);
} // litLampCount

// anyLampsAre
// test the list of lamps to see if any have a certain flag set (mainly needed for start/stop).
// Note that we find starting/stopping lamps with the hold flag set, which has to be ignored elsewhere.
boolean linesideSignal::_anyLampsAre(int bitVec, int vecTwo, boolean useReverse, boolean reversed)
{
	signalLamp *lamp;
	boolean foundOne = false;
	boolean checkStart, checkIgnore;

	if ((bitVec < 0) || (bitVec > LSS_SL_MAX)) return(false);
	
	checkStart = (vecTwo == LSS_SL_START);
	checkIgnore = (vecTwo == LSS_SL_IGNORE);
	
	lamp = _lampList;
	while (lamp != NULL) {
		
		switch (bitVec) {
		
			case LSS_SL_START:
				if (useReverse)
					foundOne = foundOne || (lamp->isStart() && (lamp->isReversed() == reversed));
				else
					foundOne = foundOne || lamp->isStart();
			break;
		
			case LSS_SL_STOP:
				if (useReverse)
					foundOne = foundOne || (lamp->isStop() && (lamp->isReversed() == reversed));
				else
					foundOne = foundOne || lamp->isStop();
			break;
			
			case LSS_SL_DELAY:
				if ((useReverse && !reversed) || !useReverse) { // check only non-reversed
					if (checkIgnore) {
						foundOne = foundOne || (lamp->onHold() && !lamp->isReversed());
					} else if (checkStart) {
						foundOne = foundOne || (lamp->onHold() && lamp->isStart() && !lamp->isReversed());
					} else { // assume stop
						foundOne = foundOne || (lamp->onHold() && lamp->isStop() && !lamp->isReversed());
					}
				} else if (useReverse && reversed) {
					if (checkIgnore) {
						foundOne = foundOne || (lamp->onHold() && lamp->isReversed());
					} else if (checkStart) {
						foundOne = foundOne || (lamp->onHold() && lamp->isStart() && lamp->isReversed());
					} else { // assume stop
						foundOne = foundOne || (lamp->onHold() && lamp->isStop() && lamp->isReversed());
					}
				} // check only reversed
			break;
		
			default: // does nothing, here to avoid an out-of-bounds crash if called with an invalid bitVec ID
			break;
	
		} // switch
	
		lamp = lamp->nextLamp;  // advance
	} // while
	
	return(foundOne);
} // anyLampsAre


// getNextLamp
//
// return true if we found a new lit lamp, false if we cycled the whole list and found nothing.
// In the event we find nothing, we remain on the current lamp.
//
// This routine will return lamps with hold set, as those may still require processing.
//
// returns LSS_NOT_PIN,LSS_NOT_PIN if nothing found, may return the same 
// as _currentAnode, _currentCathode if only one LED lit
//void linesideSignal::_getLEDPins(int &newAnode, int &newCathode, boolean &isFlashing, 
//	boolean &isAlternate, boolean &newCycle, boolean &isStart, boolean &isStop, 
//	boolean &isRamp, boolean &onHold)
boolean linesideSignal::_getNextLamp(boolean &newCycle)
{
	signalLamp *lamp;
	boolean scanning;
	
	newCycle = false;
	
	if ((_lampList == NULL) || (_currentLED == NULL)) {  // this should never happen, but just in case bail out
		return(false);
	}
	
	scanning = true;
	lamp = _currentLED->nextLamp; // start with the lamp after this one
	do {
		if (lamp == NULL) {
			lamp = _lampList;	// loop back to start
			newCycle = true;	// and record that one cycle through the list has completed
		}
		
		if (lamp->isOn()) { // found one, which may be the one we started from if its the only one
			scanning = false;
			_currentLED = lamp; // advance to the new lamp
			break;
		} // found		
		
		if (lamp == _currentLED) break; // exit if we go full circle and even the LED we started from was dark		
		
		lamp = lamp->nextLamp; // advance
	} while (scanning);
	
	return(!scanning);
} // getNextLamp

// goodPin
//
// Returns true if the pin number is valid on this Arduino for wiring signals. The current code is rather 
// simplistic and allows configuration of pins that cant actually be used if we have enabled
// the digitalWriteFast library (it also allows references to pins only used on the Mega without
// checking to see if this is a mega.
boolean linesideSignal::_goodPin(int pinNum)
{
	boolean isGood;
	
	isGood = ((pinNum >= 0) && (pinNum < 70)); // need to change this to model-dependent logic
	
	return(isGood);
} // goodPin

// advanceLamps
//
// Check each lamp and advance those matching the criteria to the next stage.  This is used to
// take previously-held lamps out of the state that they were being held for, once that 
// processing is complete (i.e., at the end of a ramp interval).
//
// Note that while there is logic elsewhere to keep this routine from being called unnecessarily,
// it is still possible that no matching lamps may be found, and the routine will simply
// return without doing anything.
//
// Side effect: if the current lamp is turned off, _killSwitch is set to ensure the pins
// get updated to match (this isn't required for activation, since that gets caught on the
// cycle through updateSignals). 
//
// Note that there will be lamps both starting and stopping, and the stopping one will
// be processed on one cycle, while the starting one won't be processed until the next.
// but that logic is handled elsewhere.
//
// A lamp may be both stopping and starting if it is a multi-color LED (either kind)
// changing color.  In which case the stop is processed first, then the start.
//
// Note that a lamp on hold can not be advanced.
void linesideSignal::_advanceLamps(int toClear, boolean doAlt)
{
	signalLamp *lamp;
	
	lamp = _lampList;
	while (lamp != NULL) {
	
		if ((toClear == LSS_SL_START) && (lamp->isStart())) {
			if (!lamp->onHold()) {
				if ((!doAlt && !lamp->isReversed()) || (doAlt && lamp->isReversed())) {
					lamp->setBitFlag(LSS_SL_START, false); // clear the starting flag (ISLIT was already true)
				} // if need to clear
			} // not on hold
		} // toClear == START
		
		if ((toClear == LSS_SL_STOP) && (lamp->isStop())) {
			if (!lamp->onHold()) {
				if ((!doAlt && !lamp->isReversed()) || (doAlt && lamp->isReversed())) {
					if (lamp->isStart()) { // if restarting, then only clear the stop flag
						lamp->setBitFlag(LSS_SL_STOP, false);	// clear the stopping flag
					} else {
						_goDark(lamp);
					} // normal lamp shutdown
				} // if need to clear
			} // if not on hold
		}  // toClear == STOP
		lamp = lamp->nextLamp;  // advance
	} // while
} // advanceLamps

// releaseHold
//
// clear the hold flag for all lamps matching criteria.
void linesideSignal::_releaseHold(int toClear, boolean doAlt)
{
	signalLamp *lamp;

	lamp = _lampList;
	while (lamp != NULL) {
		if (lamp->onHold() && (toClear == LSS_SL_START) && (lamp->isStart())) {
			if ((doAlt && lamp->isReversed()) || (!doAlt && !lamp->isReversed())) {
				lamp->setBitFlag(LSS_SL_DELAY, false);
				
			}
		} // starting lamps
		
		if (lamp->onHold() && (toClear == LSS_SL_STOP) && (lamp->isStop())) {
			if ((doAlt && lamp->isReversed()) || (!doAlt && !lamp->isReversed())) {
				lamp->setBitFlag(LSS_SL_DELAY, false);
			}		
		} // stopping lamps
	
		lamp = lamp->nextLamp;  // advance
	} // while
} // releaseHold

// advanceDivision
//
// Check to see if we need to change any holds and make related updates.  This includes
// releasing holds on lamps that are waiting for a ramp (starting or stopping) as well
// as clearing the ramp flags (advancing the lamps) when they reach the end of a ramp.
//
// The choice of divisions is based on the ramp structure.  See the documentation at _enabledLED.
void linesideSignal::_advanceDivision()
{
	boolean lampsToStop, lampsToStart, lampsToRelease, lampsToReleaseAlt;
	boolean checkReversed;
		
	// test the div here, to allow the compiler to avoid calling the list-scan function if not needed
	checkReversed = (_rampDiv == LSS_DIV_RSTART) || (_rampDiv == LSS_DIV_RSTOP);
	
	lampsToStop = _anyLampsAre(LSS_SL_STOP, LSS_SL_IGNORE, false, false); // look for any lamp thats stopping
	lampsToStart = (!lampsToStop && _anyLampsAre(LSS_SL_START, LSS_SL_IGNORE, true, checkReversed)); // ignore starting if any stopping
		
	lampsToRelease = (((_rampDiv == LSS_DIV_STOP)  || (_rampDiv == LSS_DIV_RSTOP)  ||
					 (_rampDiv == LSS_DIV_HSTOP)  || (_rampDiv == LSS_DIV_RHSTOP)) && 
		_anyLampsAre(LSS_SL_DELAY, LSS_SL_IGNORE, false, false)); // look for any held lamps
		
	lampsToReleaseAlt = (((_rampDiv == LSS_DIV_RSTOP) || (_rampDiv == LSS_DIV_RHSTOP)) && 
		_anyLampsAre(LSS_SL_DELAY, LSS_SL_IGNORE, true, true)); // look for any held reversed lamps
		
	switch (_rampDiv) {
			
		case LSS_DIV_START:
			if (lampsToStart) {
				_advanceLamps(LSS_SL_START, false); // advance normal starting lamps
			}
		break;
			
		case LSS_DIV_STOP: // once we clear stops, we can start the next lamp
			if (lampsToStop) {
				_advanceLamps(LSS_SL_STOP, false); // advance normal stopping lamp
			}
			if (lampsToRelease) { // release startup hold only if we don't have any non-reversed lamps held for stopping
				if (!_anyLampsAre(LSS_SL_DELAY, LSS_SL_STOP, true, false)) {
					_releaseHold(LSS_SL_START, false); // release normal start hold
				}
			} 
		break;
			
		case LSS_DIV_RSTART:
			if (lampsToStart) {
				_advanceLamps(LSS_SL_START, true);
			}
		break;
			
		case LSS_DIV_RSTOP:
			if (lampsToStop) {
				_advanceLamps(LSS_SL_STOP, true); // advance reverse stopping lamp
			}
			if (lampsToReleaseAlt) { // release startup hold only if we don't have any reversed lamps held for stopping
				if (!_anyLampsAre(LSS_SL_DELAY, LSS_SL_STOP, true, true)) {
					_releaseHold(LSS_SL_START, true); // release reverse start hold
				}
			}
		break;
			
		case LSS_DIV_HSTOP:
			if (lampsToRelease) {
				_releaseHold(LSS_SL_STOP, false); // release normal stop hold
			}
		break;
			
		case LSS_DIV_RHSTOP:
			//Serial.println(" RH - RHSTOP (0).");
			if (lampsToReleaseAlt) {
				_releaseHold(LSS_SL_STOP, true); // release reverse stop hold
			}
		break;
			
		default:
			// do nothing on other divisions
		break;
	} // switch
} // advanceDivision

// newRampState
//
// determines which part of the ramp cycle we're in & returns true if it has changed.
boolean linesideSignal::_newRampState()
{
	byte rampDiv;	// temp for new state
	boolean newDiv;	// flag that we changed states
	
	rampDiv = byte(_cycleCount / _cyclesPerDiv); // compute the new division number
	
	if (rampDiv >= LSS_NUM_DIV) { // reset to div 0 and do associated cleanup	
		_resetCycleTime(); // also resets the ramp to division 0		
	} // div 0
	
	newDiv = (rampDiv != _rampDiv);	// if new != current then we changed

	_rampDiv = rampDiv;
	return(newDiv);
} // setRampState


/************************ fastWrite library wrapper functions ******************************/

#if defined(LSS_USE_FAST_WRITE)
// define routines used when we have the fast form of digitalWrite available

// writeLEDPin - use the fast form of digitalWrite
void linesideSignal::_writeLEDPin(int pin, int state)
{
  
  switch (pin) {
    case 0:
      if (state == HIGH)
        digitalWriteFast( 0, HIGH);
      else
        digitalWriteFast( 0, LOW);
    break;
  
    case 1:
      if (state == HIGH)
        digitalWriteFast( 1, HIGH);
      else
        digitalWriteFast( 1, LOW);
    break;
    
    case 2:
      if (state == HIGH)
        digitalWriteFast( 2, HIGH);
      else
        digitalWriteFast( 2, LOW);
    break;
    
    case 3:
      if (state == HIGH)
        digitalWriteFast( 3, HIGH);
      else
        digitalWriteFast( 3, LOW);
    break;
    
    case 4:
      if (state == HIGH)
        digitalWriteFast( 4, HIGH);
      else
        digitalWriteFast( 4, LOW);
    break;
    
    case 5:
      if (state == HIGH)
        digitalWriteFast( 5, HIGH);
      else
        digitalWriteFast( 5, LOW);
    break;

	case 6:
      if (state == HIGH)
        digitalWriteFast( 6, HIGH);
      else
        digitalWriteFast( 6, LOW);
    break;
    
    case 7:
      if (state == HIGH)
        digitalWriteFast( 7, HIGH);
      else
        digitalWriteFast( 7, LOW);
    break;
    
    case 8:
      if (state == HIGH)
        digitalWriteFast( 8, HIGH);
      else
        digitalWriteFast( 8, LOW);
    break;
    
    case 9:
      if (state == HIGH)
        digitalWriteFast( 9, HIGH);
      else
        digitalWriteFast( 9, LOW);
    break;
    
    case 10:
      if (state == HIGH)
        digitalWriteFast( 10, HIGH);
      else
        digitalWriteFast( 10, LOW);
    break;
    
    case 11:
      if (state == HIGH)
        digitalWriteFast( 11, HIGH);
      else
        digitalWriteFast( 11, LOW);
    break;
    
    case 12:
      if (state == HIGH)
        digitalWriteFast( 12, HIGH);
      else
        digitalWriteFast( 12, LOW);
    break;
    
    case A0:
      if (state == HIGH)
        digitalWriteFast( A0, HIGH);
      else
        digitalWriteFast( A0, LOW);
    break;

    case A1:
      if (state == HIGH)
        digitalWriteFast( A1, HIGH);
      else
        digitalWriteFast( A1, LOW);
    break;

    case A2:
      if (state == HIGH)
        digitalWriteFast( A2, HIGH);
      else
        digitalWriteFast( A2, LOW);
    break;

    case A3:
      if (state == HIGH)
        digitalWriteFast( A3, HIGH);
      else
        digitalWriteFast( A3, LOW);
    break;

    case A4:
      if (state == HIGH)
        digitalWriteFast( A4, HIGH);
      else
        digitalWriteFast( A4, LOW);
    break;

    case A5:
      if (state == HIGH)
        digitalWriteFast( A5, HIGH);
      else
        digitalWriteFast( A5, LOW);
    break;

    case A6:
      if (state == HIGH)
        digitalWriteFast( A6, HIGH);
      else
        digitalWriteFast( A6, LOW);
    break;

    case A7:
      if (state == HIGH)
        digitalWriteFast( A7, HIGH);
      else
        digitalWriteFast( A7, LOW);
    break;

    default:
    break;
  } // case
  
} // writeLEDPin

// setLEDMode - use the fast form of pinMode
void linesideSignal::_setLEDMode(int pin, int mode)
{
  switch (pin) {
     case 0:
      if (mode == INPUT)
        pinModeFast( 0, INPUT);
      else
        pinModeFast( 0, OUTPUT);
    break;
 
    case 1:
      if (mode == INPUT)
        pinModeFast( 1, INPUT);
      else
        pinModeFast( 1, OUTPUT);
    break;
    
    case 2:
      if (mode == INPUT)
        pinModeFast( 2, INPUT);
      else
        pinModeFast( 2, OUTPUT);
    break;
    
    case 3:
    if (mode == INPUT)
        pinModeFast( 3, INPUT);
      else
        pinModeFast( 3, OUTPUT);
    break;
    
    case 4:
    if (mode == INPUT)
        pinModeFast( 4, INPUT);
      else
        pinModeFast( 4, OUTPUT);
    break;
    
    case 5:
    if (mode == INPUT)
        pinModeFast( 5, INPUT);
      else
        pinModeFast( 5, OUTPUT);
    break;
    
    case 6:
    if (mode == INPUT)
        pinModeFast( 6, INPUT);
      else
        pinModeFast( 6, OUTPUT);
    break;
    
    case 7:
    if (mode == INPUT)
        pinModeFast( 7, INPUT);
      else
        pinModeFast( 7, OUTPUT);
    break;
    
    case 8:
    if (mode == INPUT)
        pinModeFast( 8, INPUT);
      else
        pinModeFast( 8, OUTPUT);
    break;
    
    case 9:
    if (mode == INPUT)
        pinModeFast( 9, INPUT);
      else
        pinModeFast( 9, OUTPUT);
    break;
    
    case 10:
    if (mode == INPUT)
        pinModeFast( 10, INPUT);
      else
        pinModeFast( 10, OUTPUT);
    break;
    
    case 11:
    if (mode == INPUT)
        pinModeFast( 11, INPUT);
      else
        pinModeFast( 11, OUTPUT);
    break;
    
    case 12:
    if (mode == INPUT)
        pinModeFast( 12, INPUT);
      else
        pinModeFast( 12, OUTPUT);
    break;
    
    case A0:
    if (mode == INPUT)
        pinModeFast( A0, INPUT);
      else
        pinModeFast( A0, OUTPUT);
    break;
    
    case A1:
    if (mode == INPUT)
        pinModeFast( A1, INPUT);
      else
        pinModeFast( A1, OUTPUT);
    break;
    
    case A2:
    if (mode == INPUT)
        pinModeFast( A2, INPUT);
      else
        pinModeFast( A2, OUTPUT);
    break;
    
    case A3:
    if (mode == INPUT)
        pinModeFast( A3, INPUT);
      else
        pinModeFast( A3, OUTPUT);
    break;
    
    case A4:
    if (mode == INPUT)
        pinModeFast( A4, INPUT);
      else
        pinModeFast( A4, OUTPUT);
    break;
    
    case A5:
    if (mode == INPUT)
        pinModeFast( A5, INPUT);
      else
        pinModeFast( A5, OUTPUT);
    break;
    
    case A6:
    if (mode == INPUT)
        pinModeFast( A6, INPUT);
      else
        pinModeFast( A6, OUTPUT);
    break;
    
    case A7:
    if (mode == INPUT)
        pinModeFast( A7, INPUT);
      else
        pinModeFast( A7, OUTPUT);
    break;
    
    default:
    break;
  } // case
  
} // setLEDMode
#endif

/************************ end fastWrite wrapper functions ******************************/

/************************ lamp control functions ******************************/

// anodeDisable
//
// Set the anode off
void linesideSignal::_anodeDisable(int anode)
{

#if defined(LSS_DEBUG_VERBOSE)
	Serial.print(millis());
	Serial.print(F(": AD"));Serial.println(anode);Serial.print(F(", "));
#endif

	if (!_goodPin(anode)) return;
	
	// safety net - ensure any code problems affecting active pins cant do harm
	_anodeCount = _anodeCount - 1;
	if (_anodeCount < 0) {
#if defined(LSS_DEBUG_REPORTING)
	Serial.print(F("AD: HALT "));Serial.println(anode);
	_dropDead();
#endif
	} // safety net

	if (_suppressLEDs) return; // debug code - LEDs cant be on, so we dont need to turn them off
	
#if defined(LSS_DEBUG_REPORTING)
  long now = long(micros());
#endif
	
#if defined(LSS_USE_FAST_WRITE)
	if (LSS_DRAIN_ON) {
	_writeLEDPin( anode, LOW);	// ground it
	delayMicroseconds(LSS_DRAIN_TIME);
	} 
    _setLEDMode( anode, INPUT);	// turn off previously lit LED by setting pin to high impedance
#else
	if (LSS_DRAIN_ON) {
		digitalWrite( anode, LOW);	//  ground it
		delayMicroseconds(LSS_DRAIN_TIME); 
	}
    pinMode( anode, INPUT);		// turn off previously lit LED by setting pin to high impedance
#endif

#if defined(LSS_DEBUG_REPORTING)
  _modeTime = long(micros()) - now;
#endif

} // anodeDisable

// cathodeDisable
//
// Set the cathode off
void linesideSignal::_cathodeDisable(int cathode)
{

#if defined(LSS_DEBUG_VERBOSE)
	Serial.print(millis());
	Serial.print(F(": CD"));Serial.print(cathode);Serial.print(F(", "));
#endif

	if (!_goodPin(cathode)) return;
	
	// safety net - ensure any code problems affecting active pins cant do harm
	_cathodeCount = _cathodeCount - 1;
	if (_cathodeCount < 0) {
#if defined(LSS_DEBUG_REPORTING)
	Serial.print("CD: HALT ");Serial.println(cathode);
#endif

	_dropDead();
	} // safety net

	if (_suppressLEDs) return; // debug code - LEDs cant be on, so we dont need to turn them off

#if defined(LSS_DEBUG_REPORTING)
	long now = long(micros());
#endif

#if defined(LSS_USE_FAST_WRITE)
	if (LSS_DRAIN_ON) {
		_writeLEDPin( cathode, LOW);	// ground it
		delayMicroseconds(LSS_DRAIN_TIME); 
	}
	_setLEDMode( cathode, INPUT);	// and set high impedance here too
#else
	if (LSS_DRAIN_ON) {
		digitalWrite( cathode, LOW);	//  ground it
		delayMicroseconds(LSS_DRAIN_TIME); 
	}
	pinMode( cathode, INPUT);       // and set high impedance here too
#endif

#if defined(LSS_DEBUG_REPORTING)
  _modeTime = long(micros()) - now;
#endif

} // cathodeDisable

// anodeEnable
//
// Set the anode on
void linesideSignal::_anodeEnable(int anode)
{

#if defined(LSS_DEBUG_VERBOSE)
	Serial.print(millis());
	Serial.print(F(": AE"));Serial.print(anode);Serial.print(F(", "));
#endif

	if (!_goodPin(anode)) return;
	
	// safety net - ensure any code problems affecting active pins cant do harm
	_anodeCount = _anodeCount + 1;
	if (_anodeCount > 1) {
#if defined(LSS_DEBUG_REPORTING)
	Serial.print(F("AE: HALT "));Serial.println(anode);
#endif

	_dropDead();
	} // safety net

	if (_suppressLEDs) return; // debug code - LEDs cant be on

#if defined(LSS_DEBUG_REPORTING)
	long now = long(micros());
#endif

	// need a test here to see if it is already high (keep pin state somewhere)
	// this may be redundant based on how we're called
	
#if defined(LSS_USE_FAST_WRITE)
        _setLEDMode( anode, OUTPUT); // light the current LED
        _writeLEDPin( anode, HIGH); // +5V
#else
        pinMode( anode, OUTPUT); // light the current LED
        digitalWrite( anode, HIGH); // +5V
#endif

#if defined(LSS_DEBUG_REPORTING)
  _writeTime = long(micros()) - now;
#endif

} // anodeEnable

// cathodeEnable
//
// Set the cathode on
void linesideSignal::_cathodeEnable(int cathode)
{


#if defined(LSS_DEBUG_VERBOSE)
	Serial.print(millis());
	Serial.print(F(": CE"));Serial.print(cathode);Serial.print(F(", "));
#endif

	if (!_goodPin(cathode)) return;

	// safety net - ensure any code problems affecting active pins cant do harm
	_cathodeCount = _cathodeCount + 1;
	if (_cathodeCount > 1) {
#if defined(LSS_DEBUG_REPORTING)
	Serial.print(F("CE: HALT "));Serial.println(cathode);
#endif

	_dropDead();
	} // safety net
	

	if (_suppressLEDs) return; // debug code - LEDs cant be on

#if defined(LSS_DEBUG_REPORTING)
	long now = long(micros());
#endif

#if defined(LSS_USE_FAST_WRITE)
            _setLEDMode( cathode, OUTPUT);
            _writeLEDPin( cathode, LOW); // ground
#else
            pinMode( cathode, OUTPUT);
            digitalWrite( cathode, LOW); // ground
#endif

#if defined(LSS_DEBUG_REPORTING)
  _writeTime = long(micros()) - now;
#endif

} // cathodeEnable

// enabledLED
//
// This is the core logic controlling the illumination of the current LED.  It handles
// ramping when a LED turns on or off, or is flashing. It does not affect when a LED 
// that has been held is released to beging starting or stopping; that is handled by
// advanceDivision. While the two are interdependent in terms of which times (divisions)
// they use, they do not otherwise depend on each other.
//
// The Ramp:
// A LED that is flashing or turning on or off can be "ramping" (have the attribute
// usesRamp() set) or not. When it turns on and off and to what extent will depend on
// that.  The "reversed" (isReversed()) and "flashing" (isFlash()) attributes also affect 
// it. A flashing or ramping LED always has the "lit" attribute (isOn()) set, even when it
// is briefly off to reduce it's intensity or during the dark portion of its cycle.
//
// The same flash interval (defaulting to one second, but set from the flash rate) is used 
// for all flashing and ramping LEDs. This is divided into ten divisions, and each division 
// contains an integral number of the fundamental cycles (_cycleTime) during which each LED 
// may be illuminated once. The actual length of a flash interval is thus a multiple of 
// 10 x N x _cycleTime, and does not exactly match the interval calculated from the flash rate.
//
// During the ramps, there are three intensity levels below fully lit, achieved by skipping
// some of the cycles when the LED would normally be illuminated.
//
// Divisions are numbered from 0 - 9, and are used to produce four illumination curves:
//
// Flashing or "ramping" LED turning on or off, normal:
//
//          X  X  X
//       X  X  X  X  X  
//    X  X  X  X  X  X  X
// X  X  X  X  X  X  X  X  X  _
// 0  1  2  3  4  5  6  7  8  9
// Up: 0-2, fully lit: 3 - 5, down: 6 - 8, off: 9
//
// Non-ramping flashing or turning on/off, normal:
//
//    X  X  X  X  X 
//    X  X  X  X  X 
//    X  X  X  X  X 
// _  X  X  X  X  X  _  _  _  _
// 0  1  2  3  4  5  6  7  8  9
// fully lit: 1 - 5, off: 6 - 9, 0
//
// Flashing or "ramping" LED turning on or off, reversed:
//
// X                       X  X
// X  X                 X  X  X
// X  X  X           X  X  X  X
// X  X  X  X  _  X  X  X  X  X
// 0  1  2  3  4  5  6  7  8  9
// Up: 5 - 7, fully lit: 8 - 9, 0, down: 1 - 3, off: 4
//
//
// Non-ramping flashing or turning on/off, reversed:
//
// X                 X  X  X  X
// X                 X  X  X  X
// X                 X  X  X  X
// X  _  _  _  _  _  X  X  X  X
// 0  1  2  3  4  5  6  7  8  9
// fully lit: 1 - 5, off: 6 - 9, 0
// 
boolean linesideSignal::_enabledLED()
{
	boolean eLED;
	//boolean dbg = false;
	
	if (!_currentLED->isOn()) { // if it isn't on at all, skip the rest
		_killSwitch = true;
		_killAnode = true;
		return(false); 
	} 
		
	eLED = false; // default;
		
	switch (_rampDiv) {
		case 0:
			if (_currentLED->onHold() && _currentLED->isStop())
				eLED = true;
			else if (_currentLED->onHold() ||
					(!_currentLED->isReversed() && !_currentLED->usesRamp() && (_currentLED->isFlash() || _currentLED->isStart())))
				eLED = false;
			else if (!_currentLED->isReversed() && _currentLED->usesRamp() && 
						(_currentLED->isStart() || _currentLED->isFlash()))
				eLED = ((_cycleCount % 6) == 0); // ramp up, low intensity (level 1)
			else
				eLED = true;
		break; 
     
		case 1:
			if (!_currentLED->isReversed() && _currentLED->onHold() && _currentLED->isStop())
				eLED = true;
			else if (_currentLED->onHold() || (_currentLED->isReversed() && !_currentLED->usesRamp() && _currentLED->isFlash()))
				eLED = false;
			else if (!_currentLED->isReversed() && _currentLED->usesRamp() && (_currentLED->isFlash() || _currentLED->isStart()))
				eLED = ((_cycleCount % 4) == 0); // ramp up, quarter intensity (level 2)
			else if (_currentLED->isReversed() && _currentLED->usesRamp() && (_currentLED->isFlash() || _currentLED->isStop()))
				eLED = ((_cycleCount % 2) == 1); // ramp down, half intensity (level 3)
			else
				eLED = true; // lit
		break;
     
		case 2:
			if (!_currentLED->isReversed() && _currentLED->onHold() && _currentLED->isStop())
				eLED = true;
			else if (_currentLED->onHold() || (_currentLED->isReversed() && !_currentLED->usesRamp() && _currentLED->isFlash()))
				eLED = false;
			else if (!_currentLED->isReversed() && _currentLED->usesRamp() && (_currentLED->isFlash() || _currentLED->isStart()))
				eLED = ((_cycleCount % 2) == 0); // ramp up, half intensity (level 3)
			else if (_currentLED->isReversed() && _currentLED->usesRamp() && (_currentLED->isFlash() || _currentLED->isStop()))
				eLED = ((_cycleCount % 4) == 3); // ramp down, quarter intensity (level 2)
			else
				eLED = true; // lit or normal non-ramp flashing
		break;
		
		case 3:
			if (!_currentLED->isReversed() && _currentLED->onHold() && _currentLED->isStop())
				eLED = true;
			else if (_currentLED->onHold() || (_currentLED->isReversed() && !_currentLED->usesRamp() && _currentLED->isFlash()))
				eLED = false;
			else if (_currentLED->isReversed() && _currentLED->usesRamp() && (_currentLED->isFlash() || _currentLED->isStop()))
				eLED = ((_cycleCount % 6) == 5); // ramp down, low intensity
			else 
				eLED = true;
		break;
		
		case 4:
			if (_currentLED->onHold() && _currentLED->isStop())
				eLED = true;
			else if (_currentLED->onHold() || (_currentLED->isReversed() && (_currentLED->isFlash() || 
						(_currentLED->usesRamp() && (_currentLED->isStart() || _currentLED->isStop())))))
				eLED = false;
			else
				eLED = true;
		break;
		
		case 5:
			if (_currentLED->onHold() && _currentLED->isStop())
				eLED = true;
			else if (_currentLED->onHold() || (_currentLED->isReversed() && !_currentLED->usesRamp() && _currentLED->isFlash()))
				eLED = false;
			else if (_currentLED->isReversed() && _currentLED->usesRamp() && (_currentLED->isStart() || _currentLED->isFlash()))
				eLED = ((_cycleCount % 6) == 0); // ramp up, low intensity (level 1)
			else
				eLED = true;
		break;
     
		case (LSS_NUM_DIV - 4): // 6
			if (_currentLED->onHold() && _currentLED->isStop())
				eLED = true;
			else if (_currentLED->onHold() || 
						(!_currentLED->isReversed() && !_currentLED->usesRamp() && (_currentLED->isFlash() || _currentLED->isStart() || _currentLED->isStop())))
				eLED = false;
			else if (_currentLED->isReversed() && _currentLED->usesRamp() && (_currentLED->isFlash() || _currentLED->isStart()))
				eLED = ((_cycleCount % 4) == 0); // ramp up, quarter intensity (level 2)
			else if (!_currentLED->isReversed() && _currentLED->usesRamp() && (_currentLED->isFlash() || _currentLED->isStop()))
				eLED = ((_cycleCount % 2) == 1); // ramp down, half intensity (level 3)
			else
				eLED = true; // lit or reverse no-ramp flash			
		break;
     
		case (LSS_NUM_DIV - 3): // 7
			if (_currentLED->onHold() && _currentLED->isStop())
				eLED = true;
			else if (_currentLED->onHold() || 
						(!_currentLED->isReversed() && !_currentLED->usesRamp() && (_currentLED->isFlash() || _currentLED->isStart() || _currentLED->isStop())))
				eLED = false;
			else if (_currentLED->isReversed() && _currentLED->usesRamp() && (_currentLED->isFlash() || _currentLED->isStart()))
				eLED = ((_cycleCount % 2) == 0); // ramp up, half intensity (level 3)
			else if (!_currentLED->isReversed() && _currentLED->usesRamp() && (_currentLED->isFlash() || _currentLED->isStop()))
				eLED = ((_cycleCount % 4) == 3); // ramp down, quarter intensity (level 2)
			else 
				eLED = true;
		break;
     
		case (LSS_NUM_DIV - 2): // 8
			if (_currentLED->onHold() && _currentLED->isStop())
				eLED = true;
			else if (_currentLED->onHold() || 
						(!_currentLED->isReversed() && !_currentLED->usesRamp() && (_currentLED->isFlash() || _currentLED->isStart() || _currentLED->isStop())))
				eLED = false;
			else if (!_currentLED->isReversed() && _currentLED->usesRamp() && (_currentLED->isFlash() || _currentLED->isStop()))
				eLED = ((_cycleCount % 6) == 5); // ramp down, low intensity (level 1)
			else
				eLED = true;
		break;
   
		case (LSS_NUM_DIV - 1): // 9, this is the special dark cycle for flashing LEDs
			if (_currentLED->onHold() && _currentLED->isStop())
				eLED = true;
			else if (_currentLED->onHold() || 
					(!_currentLED->isReversed() && !_currentLED->usesRamp() && (_currentLED->isFlash() || _currentLED->isStart()|| _currentLED->isStop())) ||
					(!_currentLED->isReversed() && (_currentLED->isFlash() || 
						(_currentLED->usesRamp() && (_currentLED->isStart() || _currentLED->isStop())))))
				eLED = false;
			else
				eLED = true;
		break;
     
		default: // should not be hit, but if so handle as a cycle where a lit or flashing LED is always on
			eLED = true;  
		break;
	} // switch
	
	if (_currentLED->onHold() && !_currentLED->isStop()) { // held LED with stop set remains lit
		eLED = false;
	} // hold

	return(eLED);
} // enabledLED


/************************ main logic updateSignals function ****************************/

// updateSignals
//
// Handle cycling through the set of signal LEDs and keeping each lit one on for its slice 
// of the total time available. The updateSignals function should be called once each time
// around loop(), and loop times should be kept to a fraction of the set "cycle time" during
// which each LED may be illuminated once.
//
// This routine is the heart of the signal control logic.  Each time it is called, it checks
// to see if it is time to switch to a new LED.  It also checks to see if this cycle requires
// any actions to be taken on the current (or newly selected) LED, including turning off the
// prior one and turning on the new one. Finally, it manages the state of the LEDs as they
// progress from off to on and on to off over multiple cycles.
//
void linesideSignal::updateSignals() 
{
	long now;
	long startTime;
	long startBank;
	long newOverhead;
	long errorTime;
	long beforeTime;
	byte lastAnode, lastCathode;
	boolean switchedBank = false;
	boolean newCycle = false;
	boolean LEDEnabled;
	boolean timerExp = false;
		
	startTime = long(micros());
	
	if (_litLampCount() > (_lastLampCount + 1)) { // if more than one new light turned on the timing will be wrong
		_resetCycleTime();
	}
		
	newCycle = false;
	LEDEnabled = false; // default to off unless something turns it on
	lastAnode = _currentLED->anode;
	lastCathode = _currentLED->cathode;

	// if it's time, advance to a new LED.

	if (_lightTimerExpired()) { // move to the next LED
		startBank = long(micros());
		timerExp = true;
		
  		if (_getNextLamp(newCycle))
  			_killSwitch = false; // reset this if we find a valid LED
  		  		
   		// start the timer for the newly-lit LEDs
		beforeTime = long(micros()); // set time here so we don't count the time spent changing pins

		errorTime = (_pulseTimePerLED %  _getAverageLoop()); // extra amount we waited after timer expired midway though last cycle
		_lightTimerStart( (_pulseTimePerLED - errorTime + long(_getOverhead()) ), beforeTime );  
		  		
	} // if LED usec timer expired
		
	// Check to see if there are lamps in hold status that need to be advanced because of 
	// the current division.
	
	if (_newRampState())	// advance the ramp state if needed
		_advanceDivision();	// and if we did, see if that causes any changes in lamp status
	
	if (!_killSwitch)
		LEDEnabled = _enabledLED(); // check to see if the LED should be on or off for ramping (do after possibly advancing to new lamp)
		
	// now we actually change the lit LEDs (this also updates the pins on a new lamp even 
	// if we aren't lighting the LED this cycle, so they'll be ready for later)
	// during the ramp portion of the lit phase, flashing LEDs may be on or off per the ramp progression
	// non-flashing LEDs will always be on
	// It is possible that no lamps are lit (dark signals) and we will just loop without doing
	// anything until that changes.
	
	startBank = long(micros());
	
	// if we are changing cathodes or starting a new cycle, turn the old one off first thing
  	if ((_currentLED->cathode != lastCathode) || (newCycle) || _killSwitch)
  	{ // turn off cathode first
		if (_cathodeOn) {
			_cathodeDisable(lastCathode);
  			_cathodeOn = false;
  			lastCathode = _currentLED->cathode;
  		}
  	} // cat off

	// if we are changing anodes, turn the old one off and the new one on (an anode is always on)
  	if ((_currentLED->anode != lastAnode) || _killSwitch) { // turn off the anode if were moving to a new one
  		if (_anodeOn) {
  			_anodeDisable(lastAnode);
  			_anodeOn = false;
  			if (!_killSwitch) {
  				_anodeEnable(_currentLED->anode);
  				_anodeOn = true;
  				lastAnode = _currentLED->anode;
  			}
  			switchedBank = true;
  		}
  	} // anode off
	
	if (_killSwitch) {
		_killAnode = true;		// ensure the anode is off too
		_killSwitch = false;	// all done
	}
	
	// if LED is lit but for some reason we've turned the anode off, turn it on
	if (LEDEnabled && !_anodeOn) {
		_anodeEnable(_currentLED->anode);
  		_anodeOn = true;
  		lastAnode = _currentLED->anode;
	} // turn on Anode
	
	// if the LED is lit, make sure it is on
	if (LEDEnabled)
	{
  		if (!_cathodeOn) {
  			_cathodeEnable(_currentLED->cathode); // turn it on if it wasn't already
  			_cathodeOn = true;
  		}
	} // lit 
	
	// if the LED isn't lit, turn the cathode off if it was on from the previous lamp
	if (!LEDEnabled && _cathodeOn) 	
	{
		_cathodeDisable(_currentLED->cathode);
  		_cathodeOn = false;		
	} // flashing and it was on but should now be off
	
	if (!_cathodeOn && _killAnode) { // turn off the anode if the current light is dark
  		if (_anodeOn) {
  			_anodeDisable(lastAnode);
  			_anodeOn = false;	
  			lastAnode = _currentLED->anode;	
  		}
	}
	_killAnode = false; // ensure this is cleared for the next cycle
	
	// keep a running average of how long we spend switching the pins
	now = long(micros());
	
	if (timerExp) { // new lamp, update times
		//errorTime = (_pulseTimePerLED %  _getAverageLoop()); // extra amount we waited after timer expired midway though last cycle
		
		newOverhead = (now - beforeTime); // time spent processing LEDs this cycle
		
		_averageOverhead( newOverhead ); // keep a running average of time spent per led in addition to timer value
		
	} // new Lamp due to timer expiration - update times

#if defined(LSS_DEBUG_REPORTING)
		//now = long(micros());
		if (switchedBank) _lastBankTime = now - startBank;
		if ((now - startTime) > _maxBankTime) _maxBankTime = now - startBank;
		if ((now - startTime) < _minBankTime) _minBankTime = now - startBank;
#endif

	if (newCycle) {
		_cycleCount++; // count each time we work through the list of lamps
	}
	
	_lastLoopTime = startTime - _lastLoopStamp;
	_lastLoopStamp = long(micros()); // remember the last time we were here
	
	if (_lastLoopTime > 1000) {
		_averageLoop(1000); // cap it for fixed point limits (and in any case, loops > 1000 won't provide a huge error variation
	} else {
		_averageLoop(_lastLoopTime);
	}
		
	#if defined(LSS_DEBUG_REPORTING)
		if (_lastLoopTime < _minCycleTime) _minCycleTime = _lastLoopTime;
		if (_lastLoopTime > _maxCycleTime) _maxCycleTime = _lastLoopTime;
	#endif

} // updateSignals

/************************ debugging utility functions ****************************/

// printSignals 
//
// Print the list of signals and their status
void linesideSignal::printSignals()
{
#if defined(LSS_DEBUG_REPORTING)
	signalLamp *lamp;
	lamp = _lampList;
	int i = 0;
	
	Serial.println(F("Signals: "));
	while (lamp != NULL) {
		Serial.print(F("M")); Serial.print(lamp->mastNum);
		Serial.print(F(", H")); Serial.print(lamp->headNum);
		Serial.print(F(", L")); Serial.print(lamp->lampNum);
		switch (lamp->color) {
		
			case LSS_DARK:
				Serial.print(F(" (d) "));
			break;
			
			//case LSS_LUNAR:
			case LSS_WHITE: // same ordinal as lunar, so this is redundant
				Serial.print(F(" (W) "));
			break;
			
			case LSS_RED:
				Serial.print(F(" (R) "));
			break;
			
			case LSS_YELLOW:
				Serial.print(F(" (Y) "));
			break;
			
			case LSS_GREEN:
				Serial.print(F(" (G) "));
			break;
			
			case LSS_BLUE:
				Serial.print(F(" (B) "));
			break;
			
			//case LSS_PURPLE:
			case LSS_VIOLET: // same ordinal as purple, so this is redundant
				Serial.print(F(" (V) "));
			break;
			
			case LSS_ORANGE:
				Serial.print(F(" (O) "));
			break;
			
			case LSS_GREENYELLOW:
				Serial.print(F(" (M) "));
			break;

			case LSS_REDYELLOW:
				Serial.print(F(" (m) "));
			break;

			case LSS_GREENREDYELLOW:
				Serial.print(F(" (C) "));
			break;
			
			case LSS_REDGREENYELLOW:
				Serial.print(F(" (c) "));
			break;
			
			
			default:
				Serial.print(F(" (X) "));
			break;
		} // switch
		
		Serial.print(F("[")); Serial.print(lamp->anode);Serial.print(F(", "));
		Serial.print(lamp->cathode); Serial.print(F("] "));
		if (lamp->isOn()) Serial.print(F("L"));
		if (lamp->isFlash()) Serial.print(F("F"));
		if (lamp->isReversed()) Serial.print(F("A"));
		if (lamp->isStart()) Serial.print(F("^"));
		if (lamp->isStop()) Serial.print(F("v"));
		if (lamp->usesRamp()) Serial.print(F("R"));
		if (lamp->onHold()) Serial.print(F("H"));
		
		if (i >= 6) {
			Serial.println(F(" /"));
			i = 0;
		} else Serial.print(F(" / "));
		
		lamp = lamp->nextLamp;  // advance
		i++;
	} // while
	Serial.println(".");
#endif
} // printSignals

// printInternal
//
// Print the value of internal variables of interest
void linesideSignal::printInternal()
{
#if defined(LSS_DEBUG_REPORTING)
	Serial.print(millis());
	Serial.println(F(": Internal variables:"));
	Serial.print(F("_flashHalfInterval=")); Serial.println(_flashHalfInterval);
	Serial.print(F("_cycleTime="));Serial.println(_cycleTime);
	Serial.print(F("_pulseTimePerLED="));Serial.println(_pulseTimePerLED);
	//Serial.print(F("_overheadPerLED="));Serial.println(_overheadPerLED);
	Serial.print(F("_cyclesPerDiv="));Serial.println(_cyclesPerDiv);
	Serial.print(F("overhead="));Serial.println(_getOverhead());	
#endif
} // printInternal

// printTimes
//
// Print the value of internal variables of interest
void linesideSignal::printTimes()
{
#if defined(LSS_DEBUG_REPORTING)

	Serial.print(millis());
	Serial.println(F(": Times:"));
	Serial.print(F(" _modeTime=")); Serial.print(_modeTime);
	Serial.print(F(", _writeTime="));Serial.print(_writeTime);
	Serial.print(F(", _lastBankTime="));Serial.print(_lastBankTime);	
	Serial.print(F(", _minBankTime="));Serial.print(_minBankTime);	
	Serial.print(F(", _maxBankTime="));Serial.print(_maxBankTime);	
	Serial.print(F(", _lastLoopTime="));Serial.print(_lastLoopTime);	
	Serial.print(F(", _minCycleTime="));Serial.print(_minCycleTime);	
	Serial.print(F(", _maxCycleTime="));Serial.print(_maxCycleTime);	
	Serial.print(F(", overhead="));Serial.print(_getOverhead());	
	Serial.print(F(", avgloop="));Serial.println(_getAverageLoop());	
	
	_lastBankTime = 0;
    _maxBankTime = 0;
    _minBankTime = _cycleTime;
    _maxCycleTime = 0;
    _minCycleTime = _cycleTime;


#endif
} // printTimes

// end of linesideSignal library