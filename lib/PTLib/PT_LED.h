/************************************************
      MnSGC Ballooning PTERODACTYL Sketch
      Created by: Ashton Posey
      Date: 7/29/22
************************************************/
// Purpose: This sketch is a LED Class that will allow any number of LEDs
//          to be controlled
//  Mostly taken from: https://youtu.be/S_uaROFnWSg

#ifndef PT_LED_H
#define PT_LED_H
#include <Arduino.h>

/************************************************************************************************************************************************
LED Class Definition
************************************************************************************************************************************************/
class LED
{
public:
  /**
   * @brief Construct a new LED::LED object
   *
   * @param pin
   */
  LED(int pin);

  /**
   * @brief Sets the pinMode of the LED to OUTPUT
   *
   */
  void begin();

  /**
   * @brief Turns on LED
   *
   */
  void on();

  /**
   * @brief Turns off LED
   *
   */
  void off();
  
  /**
   * @brief Returns whether the LED is on
   *
   * @return true if the LED is on
   * @return false if the LED is off
   */
  bool status();

private:
  int _pin;
  bool _status;
};

#endif