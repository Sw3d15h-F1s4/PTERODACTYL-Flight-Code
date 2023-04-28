/************************************************
      MnSGC Ballooning PTERODACTYL Sketch
      Rewritten for PSU SSPL Labs
      Remade by:  Sam Thuransky
      Created by: Ashton Posey
                  Seyon Wallo
                  Paul Wehling
                  Andrew Van Gerpen
      Modification Date: 4/22/2023
      Remade using PlatformIO for VSCode
************************************************/

#include <main.h>

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(SERIAL_BAUD); // begin serial communication
  Serial.println("Serial Online.");

  LED1.begin();
  LED2.begin();
  LED3.begin();
  LED4.begin();
  LED5.begin();

  LED1.on(); // indicate loading

  pinMode(BUTTON_PIN, INPUT_PULLUP); // set BUTTON_PIN for digitalRead, pullup means true when button not pressed
  pinMode(SWITCH_PIN, INPUT_PULLUP); // set SWITCH_PIN to the same

  Screen.begin(); // initialize oled screen
  Screen.update("Init.");

  wait(DELAY_SETUP); // give peripherals a chance to turn on

  sdSetup(dataLogName, DATA_FILE_N1, DATA_FILE_N2); // check SD Card

  if (sdStatus && sdActive)
  {                                                                              // if the SD card is present and a file could be created
    Serial.println("SD Card Online. Creating Log File: " + String(dataLogName)); // notify logging success
    Screen.update("Logging:\n\n" + String(dataLogName));
  }
  else
  {                                                                                // otherwise
    Serial.println("SD Setup Failed. Either clear SD card, or ensure insertion."); // notify logging fail
    Screen.update("SD\nFailed\n\n\n\nSkip:B");
  }

  if (!sdStatus || !sdActive)
    LED3.on(); // require button to confirm no sd card
  if (!sdStatus || !sdActive)
    while (!updateButtonStatus())
      wait(20);

  wait(DELAY_SETUP);

  while (!intTherm || !extTherm)
  { // Loop setup until it succeeds or user skips
    intTherm = ThermistorInt.begin(12);
    extTherm = ThermistorExt.begin(12);

    // Check thermistors every 250 milliseconds after the button is pressed

    if (intTherm && extTherm)
    {                                                  // Check if intTherm and extTherm were successfully initialized.
      _print = "Therm Int Online.\nTherm Ext Online."; // i rewrote this one because frankly it was ridiculous
    }
    else if (intTherm && !extTherm)
    {
      _print = "Therm Int Online.\nTherm Ext Offline.  Skip: B\nRetry: S"; // Prompt user to either hit the button or the switch
    }
    else if (!intTherm && extTherm)
    {
      _print = "Therm Ext Online.\nTherm Int Offline.  Skip: B\nRetry: S";
    }
    else if (!intTherm && !extTherm)
    {
      _print = "Therm Ext Offline.\nTherm Int Offline.  Skip: B\nRetry: S";
    }
    Serial.println(_print); // Since this is a Teensy we are using, the read bit resolution can be at a max of 13 (12 for Teensy 4.1).
    Screen.update(_print);  // The maximum adc value given to the thermistor, should be 8191 (2^13) for a Teensy 3.6,
    if (intTherm && extTherm)
      break;
    while (!updateButtonStatus() && !updateSwitchStatus())
      wait(20); // wait for the user to make a choice
    if (updateButtonStatus())
      break; // button skips
    wait(100);
  }

  wait(DELAY_SETUP);

  Screen.update("Initializing\nMS5611"); // We are using the MS5611
  Serial.println("Initializing MS5611 Pressure Sensor");

  wait(DELAY_SETUP);

  while (!msStatus)
  { // Try to initialize the pressure sensor
    msStatus = PressureSensor.begin();
    _print = (msStatus) ? "MS5611    Online." : "MS5611    Offline.\nSkip: B\nRetry: S"; // If fails, prompt user to retry or skip.
    Serial.println(_print);
    Screen.update(_print);
    if (msStatus)
      break; // if the pressure sensor worked then break out of the loop
    while (!updateButtonStatus() && !updateSwitchStatus())
      wait(20); // Wait for switch or button to be updated
    if (updateButtonStatus())
      break; // if the button was updated, then break
    wait(100);
  }

  PressureSensor.update();
  altFtStart = PressureSensor._altitudeFt;

  wait(DELAY_SETUP);

  while (!bnoStatus)
  {                                                                                       // We are using the BNO055
    bnoStatus = OrientationSensor.begin();                                                // check if the sensor is turned on
    _print = (bnoStatus) ? "BNO055    Online!" : "BNO055    Offline.\nSkip: B\nRetry: S"; // if it fails prompt to retry or skip
    Serial.println(_print);
    Screen.update(_print);
    if (bnoStatus)
      break; // if it worked, no need to retry
    while (!updateButtonStatus() && !updateSwitchStatus())
      wait(20); // otherwise check for switch to be moved or button to be pressed
    if (updateButtonStatus())
      break; // if it was the button skip
    wait(100);
  }

  wait(DELAY_SETUP);

  while (!gpsStatus)
  {                                                                                         // We are using M9N GPS NOT in high-precision mode
    gpsStatus = gpsSetup();                                                                 // try to initialize GPS
    _print = gpsStatus ? "M9N GPS    Online!" : "M9N GPS    Offline...\nSkip: B\nRetry: S"; // if it fails prompt user to retry or skip
    Serial.println(_print);
    Screen.update(_print);
    if (gpsStatus)
      break;
    while (!updateButtonStatus() && !updateSwitchStatus())
      wait(20); // wait for button or switch to be updated
    if (updateButtonStatus())
      break; // if it was the button then skip
    wait(100);
  }

  wait(DELAY_SETUP);

  Screen.update("\n\n   XBee\n   Setup"); // Begin XBee Setup
  _print = (xBeeSetup()) ? "XBee      Relay..." : "XBee      Reciever..";
  Serial.println(_print);
  Screen.update(_print);

  wait(DELAY_SETUP);

  logData(LOG_HEADER, dataLogName);

  Serial.println(LOG_HEADER);
  LED1.off();
}

void loop()
{
  if (updateButtonStatus())
    LED5.on();
  else
    LED5.off();

  if (updateSwitchStatus())
  {
    LED3.on();
    // xbeeTimer = 0;
    // xbeeRate = 1;         // How many loops until the XBee sends another data string
    // xbeeTimerStart = 0;   // What number the loop starts at: should be 0
    // xbeeSendValue = 1;
  }
  else
    LED3.off();

  listenXBee();

  if (updateButtonStatus() && millis() - btnTimer > 250)
  {
    updateXBee("FLIP");  // Im not sure what they thought this does. Not what they think for sure.
    btnTimer = millis(); // this will trigger every time you wait 250 ms between button presses, did they think this was for holding down the button?
  }

  if (millis() - timer > DELAY_DATA)
  {
    timer = millis();
    updateData(); // Call the updateData() function every DELAY_DATA ms
  }

  if (millis() - xbeeTimerSeconds > DELAY_XBEE && isRelay)
  { // appears to send data over xbee
    if (xbeeRate != 0)
      updateXBee(xbeeID.substring(0, indexOfIntID) + String(xbeeTimer) + IDterminator + "DATA" + stringTerminator + String(inboxTerminator)); // Send XBee data every XBee send value times, allows for multiple devices to send on different values of xbeeTimer
    if (xbeeTimer >= xbeeRate)
      xbeeTimer = xbeeTimerStart; // Reset  xbeeTimer when xbeeRate is reached
    if (xbeeRate != 0)
      xbeeTimer++;
    xbeeTimerSeconds = millis();
    // while(XBeeSerial.available() == 0 && millis() - xbeeTimerSec < (xbeeDelay/2)) listenXBee();
  }
}

// Function Definitions

void wait(unsigned const int t)
{                   // millis() returns the number of milliseconds the processor has been powered on
  timer = millis(); // record the current time when this function is called
  while (millis() - timer <= t)
  {
  } // wait until the millis counter has increased to at leat t ms greater than timer
} // the way they had this before woudln't work. Can you figure out why?
  // Hint: the while loop originally came first, then the timer variable assignment.

bool updateButtonStatus()
{
  return digitalRead(BUTTON_PIN) == LOW; // The button is configured with a pull-up resistor. Pressing the button connects the pin on the Teensy to ground. What should the pin read when the button is pressed?
}

bool updateSwitchStatus()
{
  return digitalRead(SWITCH_PIN) == LOW; // The switch is configured the same as the button.
}

bool sdSetup(char *dataFile, byte const &N1, byte const &N2)
{
  pinMode(CHIP_SELECT, OUTPUT);
  sdStatus = SD.begin(CHIP_SELECT);
  if (!sdStatus)
    return false;
  for (byte i = 0; i < 100; i++)
  {
    dataFile[N1] = '0' + i / 10;
    dataFile[N2] = '0' + i % 10;
    if (!SD.exists(dataFile))
    {
      datalog = SD.open(dataFile, FILE_WRITE); // uhh datalog is opened but never closed and then used again later in logData
      sdActive = true;
      break;
    }
    else
      sdActive = false;
  }
  return sdStatus;
}

void logData(String const &Data, char const *dataFile)
{
  datalog = SD.open(dataFile, FILE_WRITE); // WARN: potentially opening the same file twice if called after sdSetup.
  datalog.println(Data);
  datalog.close();
}

bool gpsSetup()
{
  LED2.on();
  Screen.update("\nGPS\n\nSetup");
  gpsSS.begin(GPS_BAUD);

  gpsStatus = false;
  for (byte i = 0; i <= 50; i++)
  { // Attempt to set GPS to Airborn mode 50 times -sam WHY???
    if (GPS.setAirborne())
    {
      gpsStatus = true;
      break;
    }
  }
  LED2.off();
  return gpsStatus;
}

void updateGPS()
{
  gpsMonth = GPS.getMonth();
  gpsDay = GPS.getDay();
  gpsYear = GPS.getYear();
  gpsHour = GPS.getHour() - 5; // Their time zone is 5h ahead TODO: aren't we GMT-4? Check if this is accurate
  gpsMinute = GPS.getMinute();
  gpsSecond = GPS.getSecond();
  gpsLat = GPS.getLat();
  gpsLon = GPS.getLon();
  gpsAltM = GPS.getAlt_meters();
  gpsAltFt = GPS.getAlt_feet();
  SIV = GPS.getSats();
}

void updateBNO()
{
  // could add VECTOR_ACCELEROMETER, VECTOR_MAGNETOMETER, VECTOR_GRAVITY...
  sensors_event_t orientationData, angVelocityData, magnetometerData, accelerometerData;
  OrientationSensor.getEvent(&orientationData, Adafruit_BNO055::VECTOR_EULER);
  OrientationSensor.getEvent(&angVelocityData, Adafruit_BNO055::VECTOR_GYROSCOPE);
  OrientationSensor.getEvent(&magnetometerData, Adafruit_BNO055::VECTOR_MAGNETOMETER);
  OrientationSensor.getEvent(&accelerometerData, Adafruit_BNO055::VECTOR_ACCELEROMETER);

  // if this code doesn't work try (&accelerometerData)->acceleration.x
  accelerometer[0] = accelerometerData.acceleration.x;
  accelerometer[1] = accelerometerData.acceleration.y;
  accelerometer[2] = accelerometerData.acceleration.z;
  orientation[0] = orientationData.orientation.x;
  orientation[1] = orientationData.orientation.y;
  orientation[2] = orientationData.orientation.z;
  magnetometer[0] = magnetometerData.magnetic.x;
  magnetometer[1] = magnetometerData.magnetic.y;
  magnetometer[2] = magnetometerData.magnetic.z;
  gyroscope[0] = angVelocityData.gyro.x;
  gyroscope[1] = angVelocityData.gyro.y;
  gyroscope[2] = angVelocityData.gyro.z;
}

String flightTimeStr(unsigned long t)
{
  String fTime = "";
  fTime += (String(t / 3600) + ":");
  t %= 3600;
  fTime += String(t / 600);
  t %= 600;
  fTime += (String(t / 60) + ":");
  t %= 60;
  fTime += (String(t / 10) + String(t % 10));
  return fTime;
}

float calculateFrequency(unsigned long _time)
{
  float frequency = 1.0 / ((_time - prevTime) / 1000.0);
  prevTime = _time;
  return frequency;
}

void updateData()
{
  LED2.on();
  timerSeconds = (timer * 1.0) / 1000.0;
  fullTimer = flightTimeStr(timerSeconds);
  frequencyHz = calculateFrequency(timer);

  if (ThermistorInt.getStatus())
    ThermistorInt.update();
  if (ThermistorExt.getStatus())
    ThermistorExt.update();
  if (bnoStatus)
    updateBNO();
  if (gpsStatus == 1)
    updateGPS();

  if (PressureSensor.status())
  {
    PressureSensor.update();
    if (PressureSensor._altitudeFt - altFtStart > 100 && !flightTimerStatus)
    {
      flightStartTime = timerSeconds;
      flightTimerStatus = true;
    }
    if (flightTimerStatus)
      flightTimerStr = flightTimeStr(int(timerSeconds) - flightStartTime);
  }
  // Data String Assembly
  data = fullTimer + "," + flightTimerStr + "," + String(timerSeconds, 1) + "," + String(frequencyHz, 1) + "," +

         String(SIV) + "," + String(gpsMonth) + "/" + String(gpsDay) + "/" + String(gpsYear) + "," +
         String(gpsHour) + ":" + String(gpsMinute) + ":" + String(gpsSecond) + "," +
         String(gpsLat, 6) + "," + String(gpsLon, 6) + "," + String(gpsAltFt) + "," + String(gpsAltM) + "," +

         String(ThermistorInt.getTempC()) + "," + String(ThermistorInt.getTempF()) + "," + String(ThermistorExt.getTempC()) + "," + String(ThermistorExt.getTempF()) + "," +

         String(PressureSensor.msTemperatureC) + "," + String(PressureSensor.msTemperatureF) + "," + String(PressureSensor.msPressurePSI) + "," + String(PressureSensor.msPressureATM) + "," +
         String(PressureSensor._altitudeFt) + "," + String(PressureSensor._altitudeM) + "," + String(PressureSensor.vertVelFt) + "," + String(PressureSensor.vertVelM) + "," +

         String(magnetometer[0]) + "," + String(magnetometer[1]) + "," + String(magnetometer[2]) + "," +
         String(accelerometer[0]) + "," + String(accelerometer[1]) + "," + String(accelerometer[2]) + "," +
         String(gyroscope[0]) + "," + String(gyroscope[1]) + "," + String(gyroscope[2]) + "," +
         String(orientation[0]) + "," + String(orientation[1]) + "," + String(orientation[2]);

  data = data + "," + String(xbeeMessage);
  xbeeMessage = ""; // Data String Assembly end

  Screen.update(String(gpsLat, 4) + "\n" + String(gpsLon, 4) + "\n" + String(gpsAltFt, 2) + "ft\n" + String(SIV) + " Sats\nI:" + String(int(ThermistorInt.getTempF())) + " E:" + String(int(ThermistorExt.getTempF())) + "\n" + String(PressureSensor.msPressurePSI, 2) + " PSI");
  logData(data, dataLogName); // Log data to SD
  Serial.println(data);       // Serial Print data
  LED2.off();
}

bool xBeeSetup()
{ // talk about a real mess
  xBeeSS.begin(XBEE_BAUD);

  wait(300);
  counterID = 10;
  switchPosition = updateSwitchStatus(); // If switch is moved add one to the XBee Channel. 10-19
  while (updateButtonStatus() == 0)
  {
    if (updateSwitchStatus() != switchPosition)
    {
      counterID++;
      if (counterID == 20)
        counterID = 10;
      channelID = String(counterID);
    }
    Screen.update("XBee\nChannel:\n" + String(channelID) + "\nMove S\nto add 1\nPress: B");
    Serial.println("XBee Channel: " + String(channelID) + ". Move Switch to add 1. Press Button to continue.");
    switchPosition = updateSwitchStatus();
    while (switchPosition == updateSwitchStatus() && updateButtonStatus() == 0)
      wait(20); // wait for a change in switch status or a button press
  }
  counterID = 0;
  wait(300);

  counterID = 100;
  switchPosition = updateSwitchStatus(); // If switch is moved add one to the XBee Network. A100-A200
  while (updateButtonStatus() == 0)
  {
    if (updateSwitchStatus() != switchPosition)
    {
      counterID++;
      if (counterID == 200)
        counterID = 100;
      networkID = "A" + String(counterID);
    }
    Screen.update("XBee\nNetwork:\n" + String(networkID) + "\nMove S\nto add 1\nPress: B");
    Serial.println("XBee Network: " + String(networkID) + ". Move Switch to add 1. Press Button to continue.");
    switchPosition = updateSwitchStatus();
    while (switchPosition == updateSwitchStatus() && updateButtonStatus() == 0)
      wait(20); // wait for a change in switch status or a button press
  }
  counterID = 0;
  wait(300);

  xbeeTimerSeconds = millis();
  Screen.update("XBee Mode\nRly: S On\nRec:S Off\n\n\nPress B");
  Serial.println("XBee Mode\nRelay: Switch On\nReciever: Switch off\nPress Button to Continue");
  while (!updateButtonStatus())
  {
    if (updateSwitchStatus())
    {
      LED3.on();
      LED5.off();
      isRelay = true;
      Screen.update("XBee Mode\nRly: S On+Rec:S Off\n\n\nPress B");
    }
    if (!updateSwitchStatus())
    {
      LED5.on();
      LED3.off();
      isRelay = false;
      Screen.update("XBee Mode\nRly: S On\nRec:S Off+\n\nPress B");
    }
    wait(20);
  }
  LED3.off();
  LED5.off();

  if (isRelay)
  { // turn on the switch on startup to put the Arduino in Relay Mode
    LED3.on();
    isRelay = true;
    xBee.enterATmode();
    xBee.atCommand("ATRE");
    xBee.atCommand("ATCH" + channelID);
    xBee.atCommand("ATID" + networkID);
    xBee.atCommand("ATSC" + scanChID);
    xBee.atCommand("ATDL1"); // Configure XBee as a relay
    xBee.atCommand("ATMY0");
    // xBee.atCommand("ATD61");
    xBee.exitATmode();
    switchPosition = updateSwitchStatus();

    while (updateButtonStatus() == 0)
    {
      if (updateSwitchStatus() != switchPosition)
        counterID++; // If switch is moved add one to the amount of xbees in the network
      Screen.update("# XBee " + String(counterID) + "\nMove S\nto add 1\n\n\nPress: B");
      Serial.println("# of XBees " + String(counterID) + ". Move S to add 1. Press B to continue.");
      switchPosition = updateSwitchStatus();
      while (switchPosition == updateSwitchStatus() && updateButtonStatus() == 0)
        wait(20); // wait for a change in switch status or a button press
    }
    // numberOfXbees = counterID;
    xbeeRate = counterID; // for each XBee on the network, wait an additional cycle before sending data. Ehh, might work.
    for (int i = 0; i < counterID; i++)
    {                                           // create a filename for all xbee recievers that will be in the network
      xBeeCommLog[XBEE_FILE_C1] = char(i + 65); // change the filename spot to corresponding char for each xbee, A is xbee 1, B is xbee 2, etc.
      _print = (sdSetup(xBeeCommLog, XBEE_FILE_N1, XBEE_FILE_N2) && sdActive) ? "SD Card   Online... Creating File... Logging to: " + String(xBeeCommLog) : (sdActive) ? "No available file names; clear SD card to enable logging"
                                                                                                                                                                       : "Card failed, or not present";
      Serial.println(_print);
      _print = (sdStatus && sdActive) ? "Logging:\n\n" + String(xBeeCommLog) + "\n\n" + String(xBeeCommLog[XBEE_FILE_C1]) : (sdActive) ? "NAFM\nClear SD"
                                                                                                                                       : "SD\nFailed";
      Screen.update(_print);
      logData(LOG_HEADER, xBeeCommLog);
      xBeeFilenames[i][0] = xBeeCommLog[XBEE_FILE_C1];
      xBeeFilenames[i][1] = xBeeCommLog[XBEE_FILE_N1];
      xBeeFilenames[i][2] = xBeeCommLog[XBEE_FILE_N2];
      xBeeCommLog[XBEE_FILE_N1] = '0';
      xBeeCommLog[XBEE_FILE_N2] = '0';
      timer = millis();
      wait(DELAY_SETUP);
    }

    LED3.off();
  }
  else
  {
    LED5.on();
    xBee.enterATmode();     // Configure XBee as a reciever
    xBee.atCommand("ATRE"); // Reset to Default Parameters
    xBee.atCommand("ATCH" + channelID);
    xBee.atCommand("ATID" + networkID);
    xBee.atCommand("ATSC" + scanChID);
    xBee.atCommand("ATDL0"); // The DL and MY values of XBees that talk to each other should be inverted
    xBee.atCommand("ATMY1"); // Note how these are the reverse of the above settings for the relay unit
    // xBee.atCommand("ATD61");
    xBee.exitATmode();
    counterID = 1;
    switchPosition = updateSwitchStatus();
    while (updateButtonStatus() == 0)
    {
      if (updateSwitchStatus() != switchPosition)
      { // If switch is moved, add one to xbee ID
        counterID++;
        if (counterID < 11)
          xbeeID = xbeeID.substring(0, xbeeID.length() - 1) + String(counterID);
        else
          xbeeID = xbeeID.substring(0, xbeeID.length() - 2) + String(counterID);
        xBeeCommLog[XBEE_FILE_C1] = char(counterID + 64); // change the filename spot to corresponding char for each xbee, A is xbee 1, B is xbee 2, etc.
      }
      Screen.update("XBee ID:\n" + xbeeID + "\n" + String(xBeeCommLog) + "\nMove S\nto add 1\n\nPress: B");
      Serial.println("XBee ID:" + xbeeID + "\nMove Swtich to add 1, Press Button to Skip");
      switchPosition = updateSwitchStatus();
      xBeeFilenames[counterID - 1][0] = xBeeCommLog[XBEE_FILE_C1];
      xBeeFilenames[counterID - 1][1] = xBeeCommLog[XBEE_FILE_N1];
      xBeeFilenames[counterID - 1][2] = xBeeCommLog[XBEE_FILE_N2];
      while (switchPosition == updateSwitchStatus() && updateButtonStatus() == 0)
        wait(20); // wait for a change in switch status or a button press
    }
    _print = (sdSetup(xBeeCommLog, XBEE_FILE_N1, XBEE_FILE_N2) && sdActive) ? "SD Card   Online... Creating File... Logging to: " + String(xBeeCommLog) : (sdActive) ? "No available file names; clear SD card to enable logging"
                                                                                                                                                                     : "Card failed, or not present";
    Serial.println(_print);
    _print = (sdStatus && sdActive) ? "Logging:\n\n" + String(xBeeCommLog) : (sdActive) ? "NAFM\nClear SD\nSkip: B"
                                                                                        : "SD\nFailed\nSkip: B";
    Screen.update(_print);
    xBeeFilenames[counterID - 1][1] = xBeeCommLog[XBEE_FILE_N1];
    xBeeFilenames[counterID - 1][2] = xBeeCommLog[XBEE_FILE_N2];
    timer = millis();
    wait(DELAY_SETUP);
    wait(100);

    _print = "Data String Recieved, Other";
    logData(_print, xBeeCommLog);
    xbeeID = "X" + String(counterID);
    // updateXBee(LOG_HEADER);
  }

  logData("Start of Flight Bin,2,3,4,5,6,7,8", xBeeBinLog);

  _print = (sdSetup(xBeeSendLog, XBEE_SENT_N1, XBEE_SENT_N2) && sdActive) ? "SD Card   Online... Creating File... Logging to: " + String(xBeeSendLog) : (sdActive) ? "No available file names; clear SD card to enable logging"
                                                                                                                                                                   : "Card failed, or not present";
  Serial.println(_print);
  _print = (sdStatus && sdActive) ? "Logging:\n\n" + String(xBeeSendLog) : (sdActive) ? "NAFM\nClear SD"
                                                                                      : "SD\nFailed";
  Screen.update(_print);
  timer = millis();
  wait(DELAY_SETUP);
  logData("XBee Sent,2,3,4,5,6,7,8", xBeeSendLog);

  xBeeSS.setTimeout(serialTimeout);
  xBee.setTimeout(serialTimeout);
  return isRelay;
}

void listenXBee()
{
  String recievedID = "";
  byte split; // index of question mark separates id from command

  // digitalWrite(27, LOW);  // turn the RST on LOW for to allow data to be sent to the Teensy
  while (Serial.available() > 0)
  { // Send any Serial Input through to XBee.
    String serial = Serial.readStringUntil('\n');
    Serial.print("Sending: " + serial);
    xBeeSS.print(xbeeID + IDterminator + serial + stringTerminator + String(inboxTerminator));
  }

  while (xBeeSS.available() > 0)
  {
    LED3.on();
    xbeeSerialData = buildString();
    // digitalWrite(27, HIGH);  // turn the RST on HIGH for flow control

    if (xbeeSerialData == "")
      return;                       // No data was received
    Serial.println(xbeeSerialData); // Spit out recieved message over Serial

    if (xbeeSerialData.indexOf(IDterminator) != -1)
    {                                               // If no IDterminator is found indexOf will return -1, so if it doesn't equal -1 continue
      split = xbeeSerialData.indexOf(IDterminator); // IDTerminator = ? separates id from command
      recievedID = (xbeeSerialData.substring(0, split));
      if (recievedID != xbeeID && !isRelay)
        break;                                                                             // If we are recieving and the message is not for us break
      xbeeSerialData = (xbeeSerialData.substring(split + 1, xbeeSerialData.length() - 1)); // break up message, the remaining part is the serial data
    }

    // LED3.off();

    if ((xbeeSerialData.substring(0, 4)).equals("DATA"))
    { // Get full data string
      // xbeeSerialData = xbeeSerialData.substring(4);
      LED5.on();
      // xBeeSS.print(xbeeID + IDterminator + data + stringTerminator + String(inboxTerminator));
      updateXBee(xbeeID + IDterminator + data + stringTerminator + String(inboxTerminator));
      xbeeMessage = "DATA STRING TRANSMITTED";
      LED5.off();
    }

    else if ((xbeeSerialData.substring(0, 4)).equals("FLIP"))
    { // SYNC Command
      // numberOfXbees = (xbeeSerialData.substring(4, 5)).toInt();  //check to see how many xbees are linked
      // Serial.println(numberOfXbees);
      if (!LED4.status())
      {
        LED4.on();
      }
      else
        LED4.off();

      // xbeeSendValue = counterID;  // Code for Sync Command
      // xbeeRate = numberOfXbees;
      // xbeeTimer = 0;
      // if (!isRelay) wait(1500);
    }

    else if ((xbeeSerialData.substring(0, 2)).equals("BL"))
    {                                                                              // blink command
      byte times = (xbeeSerialData.substring(2, xbeeSerialData.length())).toInt(); // check to see how many times to blink
      for (; times > 0; times--)
      {
        LED4.on();
        wait(250);
        LED4.off();
        wait(250);
      }
    }
    else if (xbeeSerialData.equals("TIME")) // report total time on command
      xBeeSS.print(xbeeID + IDterminator + "Time on (ms): " + String(timer) + stringTerminator + String(inboxTerminator));

    // Serial.println(int(recievingID[4])-48-1); // Troubleshooting
    if (recievedID != "" && (recievedID.substring(0, indexOfIntID)).equals(xbeeID.substring(0, xbeeID.length() - 1)))
    {
      byte var = int(recievedID[indexOfIntID]) - 48 - 1;
      // if (isRelay){
      xBeeCommLog[XBEE_FILE_C1] = xBeeFilenames[var][0];
      xBeeCommLog[XBEE_FILE_N1] = xBeeFilenames[var][1];
      xBeeCommLog[XBEE_FILE_N2] = xBeeFilenames[var][2];
      //}
      if (xbeeSerialData.length() < 30)
      {
        xbeeSerialData += "," + String(gpsMonth) + "/" + String(gpsDay) + "/" + String(gpsYear) + "," +
                          String(gpsHour) + ":" + String(gpsMinute) + ":" + String(gpsSecond);
        logData(recievedID + "," + xbeeSerialData, xBeeBinLog);
      }
      else
        logData(xbeeSerialData, xBeeCommLog);
    }
    else
    {
      xbeeSerialData += "," + String(gpsMonth) + "/" + String(gpsDay) + "/" + String(gpsYear) + "," +
                        String(gpsHour) + ":" + String(gpsMinute) + ":" + String(gpsSecond);
      logData(xbeeSerialData, xBeeBinLog);
    }

    // Serial.println(xbeeSerialData);
    xbeeSerialData = ""; // delete the data that was recieved
  }
  // digitalWrite(27, HIGH);  // turn the RST on HIGH for flow control

  LED3.off();
}

void updateXBee(String Data)
{
  // String newData = Data;
  LED5.on();
  // if(Data.length() > stringLen){
  //     xBeeSS.print(xbeeID + IDterminator + "DATA" + Data.substring(0,stringLen));
  //     Data = Data.substring(stringLen);
  //     wait(10);
  //     while(Data.length() > stringLen){
  //         xBeeSS.print(Data.substring(0,stringLen));
  //         Data = Data.substring(stringLen);
  //         wait(10);
  //     }
  //     xBeeSS.print(Data.substring(0,stringLen) + stringTerminator + String(inboxTerminator));
  //     wait(10);
  // }
  // else xBeeSS.print(xbeeID + IDterminator + Data + "\n" + String(inboxTerminator));
  // xBeeSS.print(xbeeID + IDterminator + Data + stringTerminator + String(inboxTerminator));
  Serial.println(Data);
  xBeeSS.print(Data);
  if (Data.length() < 30)
  {
    Data += "," + String(gpsMonth) + "/" + String(gpsDay) + "/" + String(gpsYear) + "," +
            String(gpsHour) + ":" + String(gpsMinute) + ":" + String(gpsSecond);
    logData(Data, xBeeSendLog);
  }
  // xbeeMessage = "DATA REQUEST SENT";
  // wait(20);
  LED5.off();
}

String buildString()
{
  unsigned long xbeeDataTimer = millis(); // soft pause for thisfunction
  String command = xBeeSS.readStringUntil(inboxTerminator);
  if (command.equals(""))
    return "";
  if (command.equals("+++"))
    return "+++";
  if (command.indexOf(IDterminator) == -1)
    return command; // If no IDterminator is found indexOf will return -1 and the function will exit with the command
  // while ((command.substring(0,6)).indexOf(IDterminator) != -1 && command.indexOf(IDterminator) == command.lastIndexOf(IDterminator) && millis() - xbeeDataTimer < 800){
  //     command += xBeeSS.readStringUntil(inboxTerminator);
  // }
  if (command.substring(command.length() - 1) != stringTerminator)
  {
    while (command.substring(command.length() - 1) != stringTerminator && millis() - xbeeDataTimer < 800)
    {
      // Serial.println(command.substring(command.length()-1));
      // Serial.println(command);
      command += xBeeSS.readStringUntil(inboxTerminator);
    }
  }
  // Serial.println("Command -> " + command); // Print command for testing purposes
  return command;
}