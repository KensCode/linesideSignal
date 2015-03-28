// Signal Example
//
// Example using a set of three, three-head, three color-per-head common anode wired signals.
//
// This Arduino sketch (program) is released to the public domain.
//
// This sketch is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

#include <Arduino.h>

// include the library
#include "linesideSignal.h"

// create an instance of the signal
linesideSignal signals;

long lastChange = 0; // time recorded for the prior change 
long timeToWait = 10000L; // start with them lit for 10 seconds
int nextColor = 1;

// perform initialization
void setup() {   
 
  signals.setupSignal();  // initialize the library
     
  // Define signals: mast, head, lamp, anode, cathode, color
  signals.addLamp(1, 1, 1, 2, 3, LSS_GREEN); // first mast, first head
  signals.addLamp(1, 1, 2, 2, 4, LSS_YELLOW);
  signals.addLamp(1, 1, 3, 2, 5, LSS_RED);
  signals.addLamp(1, 2, 1, 2, 6, LSS_GREEN); // first mast, second head
  signals.addLamp(1, 2, 2, 2, 7, LSS_YELLOW);
  signals.addLamp(1, 2, 3, 2, 8, LSS_RED);
  signals.addLamp(1, 3, 1, 2, 9, LSS_GREEN); // first mast, third head
  signals.addLamp(1, 3, 2, 2, 10, LSS_YELLOW);
  signals.addLamp(1, 3, 3, 2, 11, LSS_RED);
  
  signals.addLamp(2, 1, 1, 3, 2, LSS_GREEN); // second mast, first head
  signals.addLamp(2, 1, 2, 3, 4, LSS_YELLOW);
  signals.addLamp(2, 1, 3, 3, 5, LSS_RED);
  signals.addLamp(2, 2, 1, 3, 6, LSS_GREEN); // second mast, second head
  signals.addLamp(2, 2, 2, 3, 7, LSS_YELLOW);
  signals.addLamp(2, 2, 3, 3, 8, LSS_RED);
  signals.addLamp(2, 3, 1, 3, 9, LSS_GREEN); // second mast, third head
  signals.addLamp(2, 3, 2, 3, 10, LSS_YELLOW);
  signals.addLamp(2, 3, 3, 3, 11, LSS_RED);
  
  signals.addLamp(3, 1, 1, 4, 2, LSS_GREEN); // third mast, first head
  signals.addLamp(3, 1, 2, 4, 3, LSS_YELLOW);
  signals.addLamp(3, 1, 3, 4, 5, LSS_RED);
  signals.addLamp(3, 2, 1, 4, 6, LSS_GREEN); // third mast, second head
  signals.addLamp(3, 2, 2, 4, 7, LSS_YELLOW);
  signals.addLamp(3, 2, 3, 4, 8, LSS_RED);
  signals.addLamp(3, 3, 1, 4, 9, LSS_GREEN); // third mast, third head
  signals.addLamp(3, 3, 2, 4, 10, LSS_YELLOW);
  signals.addLamp(3, 3, 3, 4, 11, LSS_RED);
  
  // Set initial color: mast, head, color
  signals.setHeadColor(1, 1, LSS_GREEN);
  signals.setHeadColor(1, 2, LSS_YELLOW);
  signals.setHeadColor(1, 3, LSS_RED);
  
  signals.setHeadColor(2, 1, LSS_GREEN);
  signals.setHeadColor(2, 2, LSS_YELLOW);
  signals.setHeadColor(2, 3, LSS_RED);
  
  signals.setHeadColor(3, 1, LSS_GREEN);
  signals.setHeadColor(3, 2, LSS_YELLOW);
  signals.setHeadColor(3, 3, LSS_RED);
} // setup
	
// loop continues cycling, although most times around nothing happens except the call to updateSignals, which quickly returns
// periodically, updateSignals will do a little extra work to change to the next LED in its list of lit ones.  With nine LEDs
// lit, that's roughly every quarter millisecond.
//
// At very long interfaces (tens of thousands of cycles) the switch will be executed to change which LEDs should be lit.
void loop() {
  long thisLoop; // temporary time stamp for current time
		
  signals.updateSignals();  // update LED states if required
		
  thisLoop = long(millis());
  if ((thisLoop - lastChange) > timeToWait) { // every N seconds (roughly) change top head color on firs two masts masts
    lastChange = thisLoop;
  
    switch (nextColor) {
      case 0:
        signals.setHeadColor(1, 1, LSS_GREEN);
        signals.setHeadColor(2, 1, LSS_RED);
        nextColor = 1;
      break;

      case 1:
        signals.setHeadColor(1, 1, LSS_RED);
        signals.setHeadColor(2, 1, LSS_GREEN);
        nextColor = 2;
      break;
      
      case 2:
        signals.setHeadColor(1, 1, LSS_YELLOW);
        signals.setHeadColor(2, 1, LSS_YELLOW);
        nextColor = 0;
      break;
      
      default:
        nextColor = 0;
      break;
    } // switch
  } // if time to do something
} // loop

