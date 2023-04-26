/************************************************
      MnSGC Ballooning PTERODACTYL Sketch
      Created by: Ashton Posey
      Date: 7/29/22
************************************************/
// Purpose: This Sketch will allow any number of Thermistors
//          to be controlled frictionlessly

#ifndef PT_THERMISTOR_H
#define PT_THERMISTOR_H
#include <Arduino.h>

/***********************************************************************************************************************************************
Class Definition
************************************************************************************************************************************************/
class Thermistor
{
public:
  /**
   * @brief Construct a new Thermistor:: Thermistor object
   *
   * @param pin Thermistor Pin
   */
  Thermistor(int pin);
  /**
   * @brief Initializes the thermistor
   *
   * @param analogResolutionBits Number of bits of resolution. Should be 12 on a Teensy 4.1
   * @return true if thermistor is set up properly
   * @return false otherwise
   */
  bool begin(int analogResolutionBits);
  /**
   * @brief Checks whether the thermistor is outputting a value within -80 to 160 F. Updates the thermistor's status. Use Thermistor::getStatus() if you do not want to update the thermistor's status.
   *
   * @return true if thermistor is within normal parameters
   * @return false otherwise, likely due to missing thermistor
   */
  bool updateStatus();
  /**
   * @brief Returns the status of the thermistor without updating it. Use Thermistor::updateStatus() to update the status of the thermistor.
   *
   * @return true if functioning properly
   * @return false if not functioning properly
   */
  bool getStatus();
  /**
   * @brief Reads temperature value
   *
   * @return float temperature in Celsius
   */
  float getTempC();
  /**
   * @brief Reads temperature value
   *
   * @return float temperature in Farenheit
   */
  float getTempF();
  /**
   * @brief Reads current temperature and stores locally. Retrieve this value with either Thermistor::getTempC() or Thermistor::getTempF()
   *
   */
  void update();

private:
  int _pin;
  int _analogResolutionBits;
  float adcMax;                // The maximum adc value given to the thermistor
  float A = 0.001125308852122; // A, B, and C are constants used for a 10k resistor and 10k thermistor for the steinhart-hart equation
  float B = 0.000234711863267;
  float C = 0.000000085663516;
  float R1 = 10000; // 10k Ω resistor
  float Tinv;
  float adcVal;
  float logR;
  float T;                  // these three variables are used for the calculation from adc value to temperature
  float currentTempC = 999; // The current temperature in Celcius
  float currentTempF = 999; // The current temperature in Fahrenheit
  bool _status = false;
};

#endif