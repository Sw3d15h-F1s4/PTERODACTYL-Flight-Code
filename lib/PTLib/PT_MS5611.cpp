/************************************************
      MnSGC Ballooning PTERODACTYL Sketch
      Created by: Ashton Posey
      Date: 7/29/22
************************************************/
// Purpose: Skript to control a MS5611 Pressure sensor

#include <PT_MS5611.h>
#include "Arduino.h"
#include <Wire.h>
#include <math.h>

/************************************************************************************************************************************************
Member Defenition
************************************************************************************************************************************************/


bool MS5611::begin(ms5611_osr_t osr)
{
  Wire.begin();

  reset();

  setOversampling(osr);

  delay(100);

  readPROM();

  // setting up the pressure to altitude conversions
  T1 = 59 - .00356 * h1;
  T2 = -70;
  T3 = -205.05 + .00164 * h2;

  pressureBoundary1 = (2116 * pow(((T1 + 459.7) / 518.6), 5.256));
  pressureBoundary2 = (473.1 * exp(1.73 - .000048 * h2)); // does exp function work??
  pressureBoundary3 = (51.97 * pow(((T3 + 459.7) / 389.98), -11.388));

  _status = true;

  return _status;
}


void MS5611::setOversampling(ms5611_osr_t osr)
{
  switch (osr)
  {
  case MS5611_ULTRA_LOW_POWER:
    ct = 1;
    break;
  case MS5611_LOW_POWER:
    ct = 2;
    break;
  case MS5611_STANDARD:
    ct = 3;
    break;
  case MS5611_HIGH_RES:
    ct = 5;
    break;
  case MS5611_ULTRA_HIGH_RES:
    ct = 10;
    break;
  }

  uosr = osr;
}


ms5611_osr_t MS5611::getOversampling(void)
{
  return (ms5611_osr_t)uosr;
}


void MS5611::reset(void)
{
  Wire.beginTransmission(MS5611_ADDRESS);

  Wire.write(MS5611_CMD_RESET);

  Wire.endTransmission();
}


void MS5611::readPROM(void)
{
  for (uint8_t offset = 0; offset < 6; offset++)
  {
    fc[offset] = readRegister16(MS5611_CMD_READ_PROM + (offset * 2));
  }
}


uint32_t MS5611::readRawTemperature(void)
{
  Wire.beginTransmission(MS5611_ADDRESS);

  Wire.write(MS5611_CMD_CONV_D2 + uosr);

  Wire.endTransmission();

  delay(ct);

  return readRegister24(MS5611_CMD_ADC_READ);
}


uint32_t MS5611::readRawPressure(void)
{
  Wire.beginTransmission(MS5611_ADDRESS);

  Wire.write(MS5611_CMD_CONV_D1 + uosr);

  Wire.endTransmission();

  delay(ct);

  return readRegister24(MS5611_CMD_ADC_READ);
}

int32_t MS5611::readPressure(bool compensation)
{
  uint32_t D1 = readRawPressure();

  uint32_t D2 = readRawTemperature();
  int32_t dT = D2 - (uint32_t)fc[4] * 256;

  int64_t OFF = (int64_t)fc[1] * 65536 + (int64_t)fc[3] * dT / 128;
  int64_t SENS = (int64_t)fc[0] * 32768 + (int64_t)fc[2] * dT / 256;

  if (compensation)
  {
    int32_t TEMP = 2000 + ((int64_t)dT * fc[5]) / 8388608;

    OFF2 = 0;
    SENS2 = 0;

    if (TEMP < 2000)
    {
      OFF2 = 5 * ((TEMP - 2000) * (TEMP - 2000)) / 2;
      SENS2 = 5 * ((TEMP - 2000) * (TEMP - 2000)) / 4;
    }

    if (TEMP < -1500)
    {
      OFF2 = OFF2 + 7 * ((TEMP + 1500) * (TEMP + 1500));
      SENS2 = SENS2 + 11 * ((TEMP + 1500) * (TEMP + 1500)) / 2;
    }

    OFF = OFF - OFF2;
    SENS = SENS - SENS2;
  }

  uint32_t P = (D1 * SENS / 2097152 - OFF) / 32768;

  return P;
}


double MS5611::readTemperature(bool compensation)
{
  uint32_t D2 = readRawTemperature();
  int32_t dT = D2 - (uint32_t)fc[4] * 256;

  int32_t TEMP = 2000 + ((int64_t)dT * fc[5]) / 8388608;

  TEMP2 = 0;

  if (compensation)
  {
    if (TEMP < 2000)
    {
      TEMP2 = (dT * dT) / (2 << 30);
    }
  }

  TEMP = TEMP - TEMP2;

  return ((double)TEMP / 100);
}


uint16_t MS5611::readRegister16(uint8_t reg)
{
  uint16_t value;
  Wire.beginTransmission(MS5611_ADDRESS);

  Wire.write(reg);

  Wire.endTransmission();

  Wire.beginTransmission(MS5611_ADDRESS);
  Wire.requestFrom(MS5611_ADDRESS, 2);

  byte i = 0;
  while (i < 50)
  {
    i++;
    if (Wire.available())
    {
      break;
    }
  }
  uint8_t vha = Wire.read();
  uint8_t vla = Wire.read();
  Wire.endTransmission();

  value = vha << 8 | vla;

  return value;
}

uint32_t MS5611::readRegister24(uint8_t reg)
{
  uint32_t value;
  Wire.beginTransmission(MS5611_ADDRESS);

  Wire.write(reg);

  Wire.endTransmission();

  Wire.beginTransmission(MS5611_ADDRESS);
  Wire.requestFrom(MS5611_ADDRESS, 3);
  while (!Wire.available())
  {
  };
  uint8_t vxa = Wire.read();
  uint8_t vha = Wire.read();
  uint8_t vla = Wire.read();
  Wire.endTransmission();

  value = ((int32_t)vxa << 16) | ((int32_t)vha << 8) | vla;

  return value;
}


float MS5611::tempC()
{
  msTemperatureC = readTemperature();

  return msTemperatureC;
}


float MS5611::tempF()
{
  msTemperatureC = readTemperature();
  msTemperatureF = msTemperatureC * (9.0 / 5.0) + 32.0; // Celcius to Fahrenheit

  return msTemperatureF;
}


float MS5611::pressurePSI()
{
  msPressurePSI = readPressure();
  msPressurePSI = msPressurePSI * 0.000145038; // mbar to PSI (i think)

  return msPressurePSI;
}


float MS5611::pressureATM()
{
  msPressurePSI = readPressure();
  msPressurePSI = msPressurePSI * 0.000145038; // mbar to PSI (i think)
  msPressureATM = msPressurePSI * 0.068046;    // PSI to ATM

  return msPressureATM;
}


float MS5611::altitudeFt()
{
  msPressurePSI = readPressure();
  msPressurePSI = msPressurePSI * 0.000145038; // mbar to PSI (i think)

  pressurePSF = (msPressurePSI * 144);

  _altitudeFt = -999.9;
  if (pressurePSF > pressureBoundary1) // altitude is less than 36,152 ft ASL
  {
    _altitudeFt = (459.7 + 59 - 518.6 * pow((pressurePSF / 2116), (1 / 5.256))) / .00356;
  }
  else if (pressurePSF <= pressureBoundary1 && pressurePSF > pressureBoundary2) // altitude is between 36,152 and 82,345 ft ASL
  {
    _altitudeFt = (1.73 - log(pressurePSF / 473.1)) / .000048;
  }
  else if (pressurePSF <= pressureBoundary2) // altitude is greater than 82,345 ft ASL
  {
    _altitudeFt = (459.7 - 205.5 - 389.98 * pow((pressurePSF / 51.97), (1 / -11.388))) / -.00164;
  }

  return _altitudeFt;
}


float MS5611::altitudeM()
{
  _altitudeFt = altitudeFt();

  _altitudeM = _altitudeFt * 0.3048;

  return _altitudeM;
}


bool MS5611::status()
{ // this returns the
  return _status;
}


void MS5611::update()
{
  msTemperatureC = tempC();
  msTemperatureF = tempF();
  msPressurePSI = pressurePSI();
  msPressureATM = pressureATM();
  _altitudeFt = altitudeFt();
  _altitudeM = _altitudeFt * 0.3048;
  vertVelFt = (_altitudeFt - prevAltitudeFt) / ((millis() / 1000.0) - prevTime);
  vertVelM = vertVelFt * 0.3048;
  prevAltitudeFt = _altitudeFt;
  prevTime = (millis() / 1000.0);
}
