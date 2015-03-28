/*  linesideSignal.h
    Version a1 - March 2015 - alpha release
	Define routines driving model railroad signals using charlieplexing.
    
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
    
    Released into the public domain.
*/

#include "Arduino.h"

#ifndef	linesideSignal_h
#define linesideSignal_h

// uncomment for debugging
// LSS_DEBUG_REPORTING = turns on print statements for general debugging
// LSS_DEBUG_VERBOSE = turns on print statements for details (only useful if you slow the loop down to human speed
// LSS_DEBUG_NOLEDS = turns on all the pin-activation code so we can test stuff without risking the electronics
//#define LSS_DEBUG_REPORTING
//#define LSS_DEBUG_VERBOSE
//#define LSS_DEBUG_NOLEDS

// LSS_FLASH_FPM = rate of flashing signals in full cycles per minute (flashes per min)
// Note: Arduino clocks aren't exact, so "60 FPM" may end up slightly faster or slower, but 
// then so do real signals. For best results, all flashers at one grade crossing should 
// operate from a single Auduino to ensure synchronization. This isn't necessarily true of all
// signals at one location, as each prototype signal will have it's own relays, so flashes 
// are not likely to be synchronized anyway.
//
// U.S. prototype "typically" use 60 FPM (1 flash / 1.0 second), but this doesn't seem to be documented in any official standards.
// Japanese rail standards do not specify the rate.
// British rail standards document a 60 FPM rate. (1/1.0 sec) rate.
// Dutch rail (modern) apparently documents a 75 FPM (1/0.8 sec) rate (not confirmed).
// I have timed a number of these (mostly U.S.) from youtube videos, and they seem to be 60 FPM with 10% variation,
// so from ~54 FPM to ~66 FPM (even on different modern signals on the same railroad).
//
// U.S. highway rail crossing flashers (alternating) operate from 35 FPM to 65 FPM (per CA code), 
// and some commercial relays are rated 45 FPM.
// I've timed these at around 45 - 50 FPM for US signals, but haven't done this extensively 
// so I probably missed some slower ones. I did find an Australian crossing using 40 FPM.
#define LSS_FLASH_FPM 60

// LSS_MAX_FLASH_RATE =  Define the maximum allowed flash rate that can be set by the user.
// This has dependencies on the cycle time, number of lit lamps (which affects cycle time),
// and the spacing of flashes on the ramp for flashing lamps. Setting it to more than about
// 240 fpm is likely problematic with several signals.  It could be set higher if no signals
// flash (the maximum limit then is a rate the interval of which does not drop below 
//  LSS_NUM_DIV x LSS_CYCLE_TIME (or the user-set cycle time if it differs).
#define LSS_MAX_FLASH_RATE 200

// LSS_CYCLE_TIME = default cycle time
// Cycle times over 8000 may appear to flicker for some people, but 20000 is the upper limit
// Cycle times over 4000 will be problematic for still photography in bright light and for
// flashing lamps (which may appear to flicker for some people).
// Cycle times under 4000 may be too fast for 8 MHz Arduinos or with large numbers of lamps lit.
#define LSS_CYCLE_TIME 2500

// LSS_LED_MIN = minimum number of microseconds a LED will be lit each cycle
// This should be set longer than one trip around loop(), preferably at least 2x that, to
// ensure that times will be handled close to their actual time.  If not, the FPM rate of
// pulsing LEDs is likely to be wrong. It's a little undersized here, as a fast loop takes
// around 150 microseconds, so it should be set around 300, but that would limit the maximum
// number of lit LEDs rather severely without extending cycle time, so we'll cheat a little.
// This should be adjusted when running at 8 MHz.
#define LSS_LED_MIN 200

// LSS_DRAIN_TIME = microseconds to hold cathode at ground before turning off
// the drain time is only applied if LSS_DRAIN_ON is true
// This appears to be unnecessary and is presently disabled.  It may be removed in the future.
#define LSS_DRAIN_TIME 100
#define LSS_DRAIN_ON false

// pin state (LOW, HIGH, Z)
#define LSS_PIN_GROUND 0
#define LSS_PIN_HIGH 1
#define LSS_PIN_Z 2

// colors used for signal lamps
// note: this is lamp color (a fixed attribute), not the color lit on head, although the
// basic colors (all except the special colors) are also used by setLampColor and setHeadColor
// to select which lamp to light.
// Note: use the basic colors on setLampColor and setHeadColor even if special colors were used on addLamp.
#define LSS_DARK 0				// used only on setColor, effectively same as clearHead

// principal colors
#define LSS_RED 1				// red LED
#define LSS_YELLOW 2			// yellow LED
#define LSS_GREEN 3				// green LED (on real signals this is often blue-green, but called "green")
#define LSS_LUNAR 4				// lunar is a blue-white used in a variety of signals (apparently called "milk white" in Netherlands)
#define LSS_WHITE 4				// white can be another name for lunar, or a warmer white - I am not aware of any railroad using both on the same signal

// other colors
#define	LSS_BLUE 5				// blue LED, infrequently used in the U.S., more common elsewhere, often as an absolute stop 
#define LSS_PURPLE 6			// used on some European signals (and some old U.S. railroads apparently as a dwarf "stop" color)
#define LSS_VIOLET 6			// another name for Purple (somewhat preferred in Europe, apparently)
#define LSS_ORANGE 7			// This may simply be an alternate name for yellow on some railroads, but given a separate ID just in case
#define LSS_AMBER 8				// This may simply be an alternate name for yellow on some railroads, but it is also used for the fog-piercing colors used on position-light signals
#define LSS_PINK 9				// just to be complete, "Kerosene Pink" signal glass was manufactured in the U.S., but it is unclear if any railroad ever used it on a signal.

#define LSS_LAST_FOR_SETCOLOR 127	// This is the highest "color" that can be used as a basic color.

// special colors for multi-color LEDs (two LEDs, separate leads or three-lead package)
#define LSS_GREENYELLOW 188		// Green LED that when lit with paired LED gives yellow
#define LSS_REDYELLOW 189		// Red LED that when lit with paired LED gives yellow

// special colors for bi-color LEDs (alternating current)
#define LSS_GREENREDYELLOW 198	// bi-color LED that can be yellow with alternating voltage
#define LSS_REDGREENYELLOW 199	// bi-color LED that can be yellow with alternating voltage

#define LSS_NOT_PIN 255	// used to indicate that a pin is not valid
#define LSS_NULL_SIG 0	// the mast, head and lamp values for the null signal (code assumes 0)

// the following ramp-related defines can not be changed without modifying code

// number of cycles max between pulses during a ramp division sub-interval (used as a multiplier for division length)
#define LSS_RAMP_CYCLES_STEP 8

// number of divisions for ramping LED intensity (0 - (N-1))
#define LSS_NUM_DIV 10

// Identify the ramp divisions where things have to happen
// assumes LSS_NUM_DIV of 10, with a 3-up, 3-lit, 3-down, 1-dark 0-9 progression
// the "ALT" versions are for alternating flashers and are a half-cycle offset
// the "HOLD" versions are for when we clear the hold flag
#define LSS_DIV_START 3
#define LSS_DIV_RSTOP 4
#define LSS_DIV_RSTART 8
#define LSS_DIV_STOP 9
#define LSS_DIV_HSTOP 5
#define LSS_DIV_RHSTOP 0

// signallamp bit vector
// store booleans as packed bits in a byte to reduce memory per lamp
//
// Note: ISLIT is set for any illuminated lamp, including those starting and stopping.
// _lampFlags is a byte - extend if more bits needed

#define LSS_SL_ISLIT 0			// 0 = set (true) if lamp lit or in a ramping state (isOn) - lamps w/o this set are ignored
#define LSS_SL_ISFLASH 1		// 1 = set (true) if lamp flashing (isFlashing)
#define LSS_SL_ISALTERNATE 2	// 2 = set (true) if lamp flashes on the odd half of the cycle (used for crossings etc)
#define LSS_SL_START 3			// 3 = set (true) if lamp was just turned on and is lighting up (not used when flashing)
#define LSS_SL_STOP 4			// 4 = set (true) if lamp was just turned off and is darkening (not used when flashing)
#define LSS_SL_RAMP 5			// 5 = set (true) if lamp does not use the ramp-up/ramp-down and instead goes fully it/dark with no delay
#define LSS_SL_DELAY 6			// 6 = set (true) if we are holding a pending "turn on" or "turn off" action until the next cycle
#define LSS_SL_MAX 6			// highest valid value
#define LSS_SL_IGNORE (LSS_SL_MAX+1)	// special value to be ignored

// signalLamp
// Describes the characteristics of one LED.
//
// These are dynamically allocated for each LED and occupy 12 bytes each.
// A typical 3-head, 9-light mast thus requires 9x12=108 bytes of SRAM.
//
// The signalLamp class is used internal to linesideSignal, do not attempt to manipulate directly.
//
class signalLamp
{
	private:
	
	// Use a bitvector for booleans to save some bytes (a boolean uses 8 bits normally)
	// See definition of LSS_SL_ constants above for bit use.
	
	public:
	int _lampFlags; // make this an int to avoid memory overwrite problems when using byte
	byte mastNum;	// the ordinal of the signal 
	byte headNum;	// the head (collection of lamps) within the mast (number is relative to mast)
	byte lampNum;	// ordinal of this lamp on the head (number is relative to head)
	byte color;		// lamp color
	byte anode;		// voltage source pin wired to LED anode
	byte cathode;	// ground (enable) pin wired to LED cathode

    // functions to manipulate the bit vector
    void setBitFlag(int flag, boolean flagVal);
    void clearBitFlags();
    boolean getBitFlag(int flag);
	boolean isOn();
	boolean isFlash();
	boolean isReversed();
	boolean isStart();
	boolean isStop();
	boolean usesRamp();
	boolean onHold();
	
	signalLamp *nextLamp; // linked list pointer to next, or NULL
}; // signalLamp

class linesideSignal
{
  private:
    // internal variables
    
    long _lightExpirationTime;	// absolute microsecond timestamp for expiration of light activation timer

    signalLamp *_lampList;		// the list of LEDs
    
    long _cycleTime;			// microseconds to cycle through all lit LEDs (can we make this an int?)
    int _targetCycleTime;		// the user-set cycle time for deferred application, or 0 for none
    long _lastLoopStamp;		// absolute microsecond time of last cycle through updateSignals
    long _interimOverhead;		// rolling average of overhead for LED switching (must be a long)
    long _interimLoop;			// rolling average of usec of loop times for cycle time setting
    
    int _cycleCount;			// LED cycle number relative to start of flashing interval
    int _cyclesPerDiv;			// cycles per division of the flashing interval
    int _lastLampCount;			// how many lamps were lit last time we adjusted the cycle
    byte _rampDiv;				// current division of the ramp cycle for flashing lamps
    
    signalLamp *_currentLED;	// The LED being processed at this time
    
    boolean _setupIsDone;
    boolean _suppressLEDs;		// internal flag used for debugging
    
    boolean _killSwitch;		// flag that something has changed and we need to turn off a lit LED mid-cycle
    boolean _killAnode;			// ensure the anode if off if we are not using  it
    boolean _anodeOn;			// true if we have a powered Anode
    boolean _cathodeOn;			// true if we have a powered Cathode
    int _anodeCount;			// safety-net: count active anodes, must be 0 or 1
    int _cathodeCount;			// safety-net: count active cathodes, must be 0 or 1
        
    long _pulseTimePerLED;		// time to leave the LED lit (in usec)
	long _lastLoopTime;	// time between calls to updateSignals (including time spent in that function)
    
    // used in loop() processing of flashing lamps
    long _flashHalfInterval;	// microseconds during which a flashing lamp is lit (where we store the flash rate)
	    
#if defined(LSS_DEBUG_REPORTING)
    // used to record times for reporting
        
    int _modeTime;			// time it takes to change a pin mode (or equivalent)
    int _writeTime;			// time it takes to perform a digitalWrite (or equivalent)
    long _lastBankTime;		// time in updateSignals switching anodes and cathodes to the next signal common
    long _maxBankTime;		// minimum time spent in core portion of updateSignals (cleared when reported)
    long _minBankTime;		// maximum time spent in core portion of updateSignals (cleared when reported)
    long _maxCycleTime;
    long _minCycleTime;
#endif
    
    // internal functions
    void _lightTimerStart(long usec, long startTime);
    boolean _lightTimerExpired();
    void _dropDead();
    void _addLamp(byte mastOrd, byte headOrd, byte lampOrd, byte anode, byte cathode, byte colorVal);
    boolean _getNextLamp(boolean &newCycle);
    void _setFlashRate(int rate);
    int _getFlashRate();
    void _resetCycleTime();
    void _adjCycleTime();
    boolean _enabledLED();
    boolean _newRampState();
    void _advanceLamps(int toClear, boolean doAlt);
    boolean _anyLampsAre(int bitVec, int vecTwo, boolean useReverse, boolean reversed);
    int _litLampCount();
    void _averageOverhead(int newVal);
    int _getOverhead();
    void _averageLoop(int newVal);
    int _getAverageLoop();
    void _advanceDivision();
    void _releaseHold(int toClear, boolean doAlt);
    void _goDark(signalLamp *lamp);
    
    void _writeLEDPin(int pin, int state);
    void _setLEDMode(int pin, int mode);
    void _bankEnable(boolean turnOn, int bankNum);
	void _enableSignal(boolean turnOn, int bankNum);
	void _setLEDTime();
	
	boolean _goodPin(int pinNum);
	void _anodeDisable(int anode);
	void _cathodeDisable(int cathode);
	void _anodeEnable(int anode);
	void _cathodeEnable(int cathode);

  public:
    
    // external functions
	linesideSignal(); // constructor
	void setupSignal();
	void addLamp(byte mastOrd, byte headOrd, byte lampOrd, byte anode, byte cathode, byte colorVal);
	void updateSignals();
	void setLamp(byte mastOrd, byte headOrd, byte lampOrd, boolean lit, boolean flashing);
	void setLamp(byte mastOrd, byte headOrd, byte lampOrd, boolean lit);
	void clearHead(byte mastOrd, byte headOrd);
	void setCycleTime(int cycle);
	void setFlashRate(int rate);
	void setHeadColor(byte mastOrd, byte headOrd, byte color, boolean flashing);
	void setHeadColor(byte mastOrd, byte headOrd, byte color);
	void setLampColor(byte mastOrd, byte headOrd, byte lampOrd, byte color, boolean flashing);
	void setLampColor(byte mastOrd, byte headOrd, byte lampOrd, byte color);
	void setAlternate(byte mastOrd, byte headOrd, byte lampOrd, boolean alternate);
	void setRamp(byte mastOrd, byte headOrd, byte lampOrd, boolean ramp);
	
	// debugging routines called externally - code is empty unless LSS_DEBUG_REPORTING is defined
	// but calls are public so external code doesnt need to be modified when changing that flag in the library.
	void printSignals();
	void printInternal();
	void printTimes();
	
}; // linesideSignal

#endif