/************************************************
      MnSGC Ballooning PTERODACTYL Sketch
      Created by: Ashton Posey
      Date: 7/29/22
************************************************/
// Purpose: This Sketch will allow any number of Thermistors
//          to be controlled frictionlessly

#include "PT_Thermistor.h"

/************************************************************************************************************************************************
Member Defenition
************************************************************************************************************************************************/

Thermistor::Thermistor(int pin)
{
  _pin = pin;
}

bool Thermistor::begin(int analogResolutionBits)
{                                             // this initializes the pin
  analogReadResolution(analogResolutionBits); // this value should be
  _analogResolutionBits = analogResolutionBits;
  update();
  return updateStatus();
}

bool Thermistor::updateStatus()
{
  _status = (currentTempF > -80 && currentTempF < 160) ? true : false;
  return _status;
}

bool Thermistor::getStatus()
{
  return _status;
}

float Thermistor::getTempC()
{ // reads the temperature but the output value is in Celsius
  return currentTempC;
}

float Thermistor::getTempF()
{ // reads the temperature but the output value is in Farenheit
  return currentTempF;
}

void Thermistor::update()
{
  analogReadResolution(_analogResolutionBits);
  adcMax = pow(2, _analogResolutionBits) - 1.0;
  adcVal = analogRead(_pin);
  logR = log(((adcMax / adcVal) - 1) * R1);
  Tinv = A + B * logR + C * logR * logR * logR;
  T = 1 / Tinv;
  currentTempC = T - 273.15; // converting to celcius
  currentTempF = currentTempC * 9 / 5 + 32;
}
