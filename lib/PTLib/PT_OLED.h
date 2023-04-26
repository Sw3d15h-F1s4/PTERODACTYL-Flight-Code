/************************************************
      MnSGC Ballooning PTERODACTYL Sketch
      Created by: Ashton Posey
      Date: 7/29/22
************************************************/
// Purpose: Class to control an OLED display

#ifndef OLED_H
#define OLED_H
#include <Arduino.h>

/***********************************************************************************************************************************************
LED Class Definition
************************************************************************************************************************************************/
class OLED
{
public:
  /**
   * @brief Initializes the OLED screen.
   *
   * Clears the screen, initializes I2C, and enables output.
   */
  void begin();

  /**
   * @brief Displays a string to the screen
   *
   * @param disp String to display. Use "\\n" to indicate a newline. (only one backtick)
   */
  void update(String disp);

private:
  int _pin1;
  int _pin2;
};

#endif