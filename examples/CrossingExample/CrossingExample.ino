// Crossing Signal Example
//
// Example using a pair of dual-lamp crossing flashers (e.g., one for each side of the tracks), having
// two lights each with common anode wiring on each mast.
//
// The program runs the flashers for 20 seconds, then goes dark for 10, then it repeats.
//
// Grade crossing lights lack the ramp-up/ramp-down behavior of a flashing lineside signal, and tend to
// flash more slowly (50 FPM is typical, but 40-60 are used in some places), so we alter the default rate here.
//
// This uses pins 2 and 3 for the commons for the two masts, and mast #1 (common 2),
// uses pins 3 & 4 for the lamps, while mast #2 (common 3), uses pins 2 & 4.
//
// This Arduino sketch (program) is released to the public domain.
//
// This sketch is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

#include <Arduino.h>

// include the library
#include "linesideSignal.h"

// create an instance for the signal library
linesideSignal signals;

// variables used in the loop (not used by the library)
long lastChange = 0; // time recorded for the prior change 
boolean goClear = true;  // we start with the signals running, so the next action is to clear them
long timeToWait = 20000; // start with them lit for 20 seconds

// perform initialization
void setup() {   
 
  signals.setupSignal();  // initialize the library
  
  // Define signals: mast, head, lamp, anode, cathode, color
  signals.addLamp(1, 1, 1, 2, 3, LSS_RED); // first mast, first head, lamp 1, common anode 2, cathode 3
  signals.addLamp(1, 1, 2, 2, 4, LSS_RED); // first mast, first head, lamp 2, common anode 2, cathode 4
  
  signals.addLamp(2, 1, 1, 3, 2, LSS_RED); // second mast, first head, lamp 1, common anode 3, cathode 2 (sharing pins w/ 1, 1)
  signals.addLamp(2, 1, 2, 3, 4, LSS_RED); // second mast, first head, lamp 2, common anode 3, cathode 4
  
  signals.setFlashRate(50); // Use a rate more typical of grade crossing flashers than the default 60  
  signals.setRamp(1, 1, 1, false); // these are crossings, so don't use the fade-in/fade-out
  signals.setRamp(1, 1, 2, false);
  signals.setRamp(2, 1, 1, false);
  signals.setRamp(2, 1, 2, false);
  
  signals.setAlternate(1, 1, 1, false); // start with the lights flashing
  signals.setAlternate(1, 1, 2, true);
  signals.setAlternate(2, 1, 1, false);
  signals.setAlternate(2, 1, 2, true);
  
  lastChange = long(millis()); // using signed longs for timer math
} // setup
	
void loop() {
    long thisLoop; // temporary time stamp for current time
		
  signals.updateSignals();  // update LED states if required
		
  thisLoop = long(millis());
  if ((thisLoop - lastChange) > timeToWait) { // every N seconds change from flashing to dark
    lastChange = thisLoop;
    if (goClear) {
      signals.setHeadColor(1, 1, LSS_DARK); // turn off (and clear flashing & alternate flags)
      signals.setHeadColor(2, 1, LSS_DARK); // turn off (and clear flashing & alternate flags)
      timeToWait = 10000; // stay dark 10 seconds
    } else {
      signals.setAlternate(1, 1, 1, false); // turn on, flashing
      signals.setAlternate(1, 1, 2, true);  // turn on, flashing on alternate cycle
      signals.setAlternate(2, 1, 1, false); // turn on, flashing
      signals.setAlternate(2, 1, 2, true);  // turn on, flashing on alternate cycle
      timeToWait = 20000; // stay line 20 seconds
    }
    
    goClear = !goClear; // toggle each time
  } // if time
} // loop

