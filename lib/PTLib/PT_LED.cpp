/************************************************
      MnSGC Ballooning PTERODACTYL Sketch
      Created by: Ashton Posey
      Date: 7/29/22
************************************************/
// Purpose: This sketch is a LED Class that will allow any number of LEDs
//          to be controlled

#include "PT_LED.h"

/************************************************************************************************************************************************
LED Member Defenition
************************************************************************************************************************************************/
// this library is actually useless, who wrote this? a js/npm dev?
// what's next? is-positive-integer with 3 dependencies?

LED::LED(int pin)
{
  _pin = pin;
}

void LED::begin()
{
  pinMode(_pin, OUTPUT);
}

void LED::on()
{
  digitalWrite(_pin, HIGH);
  _status = 1;
}

void LED::off()
{
  digitalWrite(_pin, LOW);
  _status = 0;
}

bool LED::status()
{
  return _status;
}
