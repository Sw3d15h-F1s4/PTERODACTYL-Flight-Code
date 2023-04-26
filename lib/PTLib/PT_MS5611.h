/************************************************
      MnSGC Ballooning PTERODACTYL Sketch
      Created by: Ashton Posey
      Date: 7/29/22
************************************************/
// Purpose: Skript to control a MS5611 Pressure sensor

#ifndef PT_MS5611_H
#define PT_MS5611_H
#include <Arduino.h>

#define MS5611_ADDRESS (0x77)

#define MS5611_CMD_ADC_READ (0x00)
#define MS5611_CMD_RESET (0x1E)
#define MS5611_CMD_CONV_D1 (0x40)
#define MS5611_CMD_CONV_D2 (0x50)
#define MS5611_CMD_READ_PROM (0xA2)

typedef enum
{
  MS5611_ULTRA_HIGH_RES = 0x08,
  MS5611_HIGH_RES = 0x06,
  MS5611_STANDARD = 0x04,
  MS5611_LOW_POWER = 0x02,
  MS5611_ULTRA_LOW_POWER = 0x00
} ms5611_osr_t;

/************************************************************************************************************************************************
Class Definition
************************************************************************************************************************************************/
class MS5611
{
public:
  /**
   * @brief Initializes a MS5611 Pressure Sensor reference
   *
   * @param osr enum ms5611_osr_t
   * @return true if initialization successful
   * @return false otherwise
   */
  bool begin(ms5611_osr_t osr = MS5611_HIGH_RES);
  /**
   * @brief Reads raw temperature value
   *
   * @return uint32_t
   */
  uint32_t readRawTemperature(void);
  /**
   * @brief Reads raw pressure value
   *
   * @return uint32_t
   */
  uint32_t readRawPressure(void);
  /**
   * @brief Reads calibrated temperature value
   *
   * @param compensation if true, compensate for different temperature ranges
   * @return double Temperature in Celsius
   */
  double readTemperature(bool compensation = false);
  /**
   * @brief Reads Pressure Value
   *
   * @param compensation if true, compensate for different temperature ranges
   * @return int32_t Pressure in mbar
   */
  int32_t readPressure(bool compensation = false);

  /**
   * @brief Sets the oversampling value
   *
   * @param osr enum ms5611_osr_t
   */
  void setOversampling(ms5611_osr_t osr);

  /**
   * @brief Returns oversampling rate
   *
   * @return enum ms5611_osr_t
   */
  ms5611_osr_t getOversampling(void);
  /**
   * @brief Read temperature in Celsius
   *
   * @return float
   */
  float tempC();
  /**
   * @brief Read temperature in Fahrenheit
   *
   * @return float
   */
  float tempF();
  /**
   * @brief Read atmospheric pressure in Pounds per Square Inch
   *
   * @return float
   */
  float pressurePSI();
  /**
   * @brief Read atmospheric pressure in Atmospheres
   *
   * @return float
   */
  float pressureATM();
  /**
   * @brief Calculates current altitude based on pressure readings
   *
   * @return float altitude in feet
   */
  float altitudeFt();
  /**
   * @brief Calculates altitude based on pressure and converts to meters
   *
   * @return float altitude in meters
   */
  float altitudeM();
  /**
   * @brief Returns whether MS5611 has been initalized successfully
   *
   * @return true if successful
   * @return false if not initialized
   */
  bool status();
  /**
   * @brief Reads all values and performs all calculations
   *
   */
  void update();

  float msTemperatureC;

  float msTemperatureF;

  float msPressurePSI;

  float msPressureATM;

  float _altitudeFt;

  float _altitudeM;

  float vertVelFt;

  float vertVelM;

private:
  uint16_t fc[6];

  uint8_t ct;

  uint8_t uosr;

  int32_t TEMP2;

  int64_t OFF2, SENS2;

  /**
   * @brief Sends reset command to MS5611
   *
   */
  void reset(void);
  /**
   * @brief Reads PROM from MS5611
   *
   */
  void readPROM(void);
  // Read 16 bit from register (oops MSB, LSB)
  uint16_t readRegister16(uint8_t reg);

  // Read 24-bit from register (oops XSB, MSB, LSB)
  uint32_t readRegister24(uint8_t reg);

  int _pin1;
  int _pin2;
  bool _status;

  double pressureBoundary1;
  double pressureBoundary2;
  double pressureBoundary3;
  float h1 = 36152.0;
  float h2 = 82345.0;
  float T1;
  float T2;
  float T3;

  float pressurePSF;

  double prevAltitudeFt;
  double prevTime;
};

#endif