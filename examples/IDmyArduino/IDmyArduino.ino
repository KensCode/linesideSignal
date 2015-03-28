// IDmyArduino
// Version a1 - March 2015, by Ken Shores
//
// Compile and run this program with a Serial Monitor window open and it
// will wait five seconds then report useful details about your Arduino 
// and development environment.
// 
// This required a current (v1.00 or later) Arduino environment to work.
//
// Development environemnt: version number of the IDE.
//
// Clock: clock speed of the processor in Hz.
//
// SRAM = available SRAM after loading this program, in bytes
// (on a 2K model there will be about 1800 free)
//
// Flash = total Flash memory in bytes.
//
// Some code used comes from Arduino Playground ShowInfo sketch.
//
// This Arduino sketch (program) is released to the public domain.
//
// This sketch is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
//

#include <Arduino.h>

// From Arduino Playground ShowInfo sketch:
// Helper function for free ram.
//   With use of http://playground.arduino.cc/Code/AvailableMemory
//
int freeRAM(void)
{
  extern unsigned int __heap_start;
  extern void *__brkval;

  int free_memory;
  int stack_here;

  if (__brkval == 0)
    free_memory = (int) &stack_here - (int) &__heap_start;
  else
    free_memory = (int) &stack_here - (int) __brkval;

  return (free_memory);
}

// perform initialization - and report on this arduino
void setup() {   
  
  delay(5000);          // give the user time to open the serial window
  Serial.begin(9600); 

#if defined (__AVR_ATmega32U4__)
  while(!Serial);        // For Leonardo, wait for serial port
#endif
 
  // report available memory
  Serial.println(F("IDmyArduino: "));
  Serial.print(F("Arduino development environment (IDE) version "));Serial.print((float) ARDUINO / 100.0, 2);Serial.println(".");
  Serial.print(F("Clock  : "));Serial.print(F_CPU);Serial.println(". (Hz)");
  Serial.print(F("SRAM   : ")); Serial.print(freeRAM()); Serial.println(F(". (Available SRAM)"));
  Serial.print(F("Flash  : "));Serial.print(FLASHEND);Serial.println(F(". (Total Flash)"));  
 
} // setup

void loop() {
  // do nothing, forever
} // loop


