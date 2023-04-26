/************************************************
      MnSGC Ballooning PTERODACTYL Sketch
      Created by: Ashton Posey
      Date: 7/29/22
************************************************/
//Purpose: This sketch is a LED Class that will allow any number of LEDs
//         to be controlled

#include "PT_LED.h"

/************************************************************************************************************************************************
LED Member Defenition
************************************************************************************************************************************************/


LED::LED(int pin){ 
  _pin = pin;
}


void LED::begin(){ 
  pinMode(_pin, OUTPUT);
}


void LED::on(){
  digitalWrite(_pin, HIGH);
  _status = 1;
}


void LED::off(){
  digitalWrite(_pin, LOW);
  _status = 0;
}


bool LED::status(){
  return _status;
}
