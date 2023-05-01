/************************************************
      MnSGC Ballooning PTERODACTYL Sketch
      Created by: Ashton Posey
      Date: 7/29/22
************************************************/
// Purpose: Class to control an OLED display

#include "PT_OLED.h"
#include <Wire.h>
#include <OneWire.h>
#include <SFE_MicroOLED.h> // https://github.com/sparkfun/SparkFun_Micro_OLED_Arduino_Library

#define PIN_RESET 9
#define DC_JUMPER 1

MicroOLED oled(PIN_RESET, DC_JUMPER); // I2C declaration

/************************************************************************************************************************************************
OLED Member Defenition
************************************************************************************************************************************************/

void OLED::begin()
{ // this initializes the OLED
  Wire.begin();
  oled.begin();     // Initialize the OLED
  oled.clear(ALL);  // Clear the display's internal memory
  oled.display();   // Display what's in the buffer (splashscreen)
  oled.clear(PAGE); // Clear the buffer.

  randomSeed(analogRead(A0) + analogRead(A1)); // why do we need to init random???
}

void OLED::update(String disp)
{ // displays a string
  oled.clear(PAGE);
  oled.setFontType(0);
  oled.setCursor(0, 0);
  oled.println(disp);
  oled.display();
}
