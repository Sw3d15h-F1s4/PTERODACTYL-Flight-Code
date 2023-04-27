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

void setup() {
  // put your setup code here, to run once:
  Serial.begin(SERIAL_BAUD);                                                                // begin serial communication
  Serial.println("Serial Online.");

  LED1.begin();
  LED2.begin();
  LED3.begin();
  LED4.begin();
  LED5.begin();

  LED1.on();                                                                                // indicate loading

  pinMode(BUTTON_PIN, INPUT_PULLUP);                                                        // set BUTTON_PIN for digitalRead, pullup means true when button not pressed
  pinMode(SWITCH_PIN, INPUT_PULLUP);                                                        // set SWITCH_PIN to the same

  Screen.begin();                                                                           // initialize oled screen
  Screen.update("Init.");

  wait(DELAY_SETUP);                                                                        // give peripherals a chance to turn on

  sdSetup(dataLogName, DATA_FILE_N1, DATA_FILE_N2);                                         // check SD Card

  if (sdStatus && sdActive){                                                                // if the SD card is present and a file could be created
    Serial.println("SD Card Online. Creating Log File: " + String(dataLogName));            // notify logging success
    Screen.update("Logging:\n\n" + String(dataLogName));
  } else {                                                                                  // otherwise
    Serial.println("SD Setup Failed. Either clear SD card, or ensure insertion.");          // notify logging fail
    Screen.update("SD\nFailed\n\n\n\nSkip:B");
  }

  if (!sdStatus || !sdActive) LED3.on();                                                    // require button to confirm no sd card
  if (!sdStatus || !sdActive) while(!updateButtonStatus()) delay(20);

  wait(DELAY_SETUP);

  while (ThermistorInt.getStatus() == 0 || ThermistorExt.getStatus() == 0){
    if(millis()-timer > 250){                                                               // Check thermistors every 250 milliseconds after the button is pressed
      _print = (ThermistorInt.begin(12) && ThermistorExt.begin(12)) ? "Therm Int Online!\nTherm Ext Online!" : ThermistorInt.getStatus() ? "Therm Int Online!\nTherm Ext Offline...Skip: B\nRetry: S" : ThermistorExt.getStatus() ? "Therm Int Offline...\nTherm Ext Online!\nSkip: B\nRetry: S" : "Therm Int Offline...Therm Ext Offline...Skip: B\nRetry: S";
      Serial.println(_print);                                                               // Since this is a Teensy we are using, the read bit resolution can be at a max of 13 (12 for Teensy 4.1).
      Screen.update(_print);                                                                // The maximum adc value given to the thermistor, should be 8191 (2^13) for a Teensy 3.6, 
      timer = millis();                                                                     // 4095 (2^12) for a Teensy 4.1, and 1023 (2^10) for an Arduino
    }
    if(ThermistorInt.getStatus() == 0 || ThermistorExt.getStatus() == 0) while(updateButtonStatus() == 0 && updateSwitchStatus() == 0) delay(20);
    if (updateButtonStatus()) break; 
  }

  wait(DELAY_SETUP);

  Screen.update("Initializing\nMS5611");                                                    // We are using the MS5611
  Serial.println("Initializing MS5611 Pressure Sensor");

  wait(DELAY_SETUP);

  while (!msStatus) {                                                                       // Try to initialize the pressure sensor
    msStatus = PressureSensor.begin();
    _print = (msStatus) ? "MS5611    Online." : "MS5611    Offline.\nSkip: B\nRetry: S";    // If fails, prompt user to retry or skip.
    Serial.println(_print);
    Screen.update(_print);
    if (msStatus) break;                                                                    // if the pressure sensor worked then break out of the loop
    while(updateButtonStatus() == 0 && updateSwitchStatus() == 0) delay(20);                // Wait for switch or button to be updated
    if (updateButtonStatus()) break;                                                        // if the button was updated, then break
    wait(100);
  }

  PressureSensor.update();
  altFtStart = PressureSensor._altitudeFt;

  wait(DELAY_SETUP);

  while(!bnoStatus){                                                                        // We are using the BNO055
    bnoStatus = OrientationSensor.begin();                                                  // check if the sensor is turned on 
    _print = (bnoStatus) ? "BNO055    Online!" : "BNO055    Offline.\nSkip: B\nRetry: S";   // if it fails prompt to retry or skip
    Serial.println(_print);
    Screen.update(_print);
    if (bnoStatus) break;                                                                   // if it worked, no need to retry
    while(updateButtonStatus() == 0 && updateSwitchStatus() == 0) delay(20);                // otherwise check for switch to be moved or button to be pressed
    if (updateButtonStatus()) break;                                                        // if it was the button skip
    wait(100);
  }

  wait(DELAY_SETUP);

  while(!gpsStatus){                                                                       // We are using M9N GPS NOT in high-precision mode
    gpsStatus = gpsSetup();                                                                // try to initialize GPS
    _print = gpsStatus ? "M9N GPS    Online!" : "M9N GPS    Offline...\nSkip: B\nRetry: S";// if it fails prompt user to retry or skip
    Serial.println(_print);
    Screen.update(_print);
    if(gpsStatus) while(updateButtonStatus() == 0 && updateSwitchStatus() == 0) delay(20); // wait for button or switch to be updated
    if (updateButtonStatus()) break;                                                       // if it was the button then skip 
    wait(20);
  }

  wait(DELAY_SETUP);

  Screen.update("\n\n   XBee\n   Setup");                                                  // Begin XBee Setup
  _print = (xBeeSetup()) ? "XBee      Relay..." : "XBee      Reciever..";
  Serial.println(_print);
  Screen.update(_print);

  wait(DELAY_SETUP);

  logData(LOG_HEADER, dataLogName);

  Serial.println(LOG_HEADER);
  LED1.off();  
}

void loop() {
    //if(millis()-gpsTimer > 20 && !hpGPS){
      GPS.update(); // The structure of this loop is to ensure that the button, switch, GPS, and XBee all work properly
      //while (ss.available()) {
        //ss.parser.encode(ss.read());
      //}
      //gpsTimer = millis();
  //}

  if (updateButtonStatus()) LED5.on();
  else LED5.off();

  if (updateSwitchStatus()) { 
      LED3.on();
      // xbeeTimer = 0;
      // xbeeRate = 1;         // How many loops until the XBee sends another data string
      // xbeeTimerStart = 0;   // What number the loop starts at: should be 0
      // xbeeSendValue = 1;
  }
  else LED3.off();

  listenXBee();

  if (updateButtonStatus() && millis() - btnTimer > 250) { // Check if the button is pressed to send command.
      updateXBee("FLIP");                                     // Only soft pause that can use buttonTimer in loop
      btnTimer = millis();
  }

  if(millis() - timer > DELAY_DATA) { // Only soft pause that can use timer in loop
      timer = millis();
      updateData();
  }

  // if(millis() - oneHzTimer > oneHzDelay) { // Only soft pause that can use timer in loop
  //     oneHzTimer = millis();
  //     logData(data, oneHzFilename);
  // }

  if(millis() - xbeeTimerSeconds > DELAY_XBEE && isRelay) { // Only soft pause that can use timer in loop
      if (xbeeRate != 0) updateXBee(xbeeID.substring(0, indexOfIntID) + String(xbeeTimer) + IDterminator + "DATA" + stringTerminator + String(inboxTerminator));       // Send XBee data every XBee send value times, allows for multiple devices to send on different values of xbeeTimer
      if (xbeeTimer >= xbeeRate) xbeeTimer = xbeeTimerStart;  // Reset  xbeeTimer when xbeeRate is reached
      if (xbeeRate != 0) xbeeTimer++;
      xbeeTimerSeconds = millis();
      //while(XBeeSerial.available() == 0 && millis() - xbeeTimerSec < (xbeeDelay/2)) listenXBee();
  }


  
}




// Function Definitions

void wait(unsigned const int t){
  while(millis() - timer <= t) {}
  timer = millis();
}

bool updateButtonStatus(){ 
  bool buttonStatus = false;
  buttonStatus = (digitalRead(BUTTON_PIN) == 0) ? true : false;
  return buttonStatus;
}

bool updateSwitchStatus(){
  bool switchStatus = false;
  switchStatus = (digitalRead(SWITCH_PIN) == 0) ? true : false;
  return switchStatus;
}

bool sdSetup(char *dataFile, byte const &N1, byte const &N2){
  pinMode(CHIP_SELECT,OUTPUT);
  if(!SD.begin(CHIP_SELECT)) sdStatus = false;
  else {
      sdStatus = true;
      for (byte i = 0; i < 100; i++) {
          dataFile[N1] = '0' + i/10; 
          dataFile[N2] = '0' + i%10;
          if (!SD.exists(dataFile)) {
             datalog = SD.open(dataFile, FILE_WRITE);
              sdActive = true;
              break;
          }
          else sdActive = false;
      }
  } 
  return sdStatus;
}

void logData(String const &Data, char const *dataFile){
  datalog = SD.open(dataFile, FILE_WRITE);
  datalog.println(Data);
  datalog.close();
}

bool gpsSetup(){
  LED2.on();
  Screen.update("\nGPS\n\nSetup");
  gpsSS.begin(GPS_BAUD);
  //gps.init();
  
  for (byte i=0; i<=50; i++){ // Attempt to set GPS to Airborn mode 50 times
      if (GPS.setAirborne()) {
          gpsStatus = true;
          break;
      }
      if (i==50) gpsStatus = false;
  }
  LED2.off();
  return gpsStatus;
}

void updateGPS(){
  gpsMonth  = GPS.getMonth();
  gpsDay    = GPS.getDay();
  gpsYear   = GPS.getYear();
  gpsHour   = GPS.getHour() - 5;  // Their time zone is 5h ahead TODO: aren't we GMT-4?
  gpsMinute = GPS.getMinute();
  gpsSecond = GPS.getSecond();
  gpsLat    = GPS.getLat();
  gpsLon    = GPS.getLon();
  gpsAltM   = GPS.getAlt_meters();
  gpsAltFt  = GPS.getAlt_feet();
  SIV       = GPS.getSats();
}

void updateBNO(){
  //could add VECTOR_ACCELEROMETER, VECTOR_MAGNETOMETER, VECTOR_GRAVITY...
  sensors_event_t orientationData, angVelocityData, magnetometerData, accelerometerData;
  OrientationSensor.getEvent(&orientationData, Adafruit_BNO055::VECTOR_EULER);
  OrientationSensor.getEvent(&angVelocityData, Adafruit_BNO055::VECTOR_GYROSCOPE);
  OrientationSensor.getEvent(&magnetometerData, Adafruit_BNO055::VECTOR_MAGNETOMETER);
  OrientationSensor.getEvent(&accelerometerData, Adafruit_BNO055::VECTOR_ACCELEROMETER);

  accelerometer[0] = (float)accelerometerData.acceleration.x;
  accelerometer[1] = (float)accelerometerData.acceleration.y;
  accelerometer[2] = (float)accelerometerData.acceleration.z;
  orientation[0] = (float)orientationData.orientation.x;
  orientation[1] = (float)orientationData.orientation.y;
  orientation[2] = (float)orientationData.orientation.z;
  magnetometer[0] = (float)magnetometerData.magnetic.x;
  magnetometer[1] = (float)magnetometerData.magnetic.y;
  magnetometer[2] = (float)magnetometerData.magnetic.z;
  gyroscope[0] = (float)angVelocityData.gyro.x;
  gyroscope[1] = (float)angVelocityData.gyro.y;
  gyroscope[2] = (float)angVelocityData.gyro.z;

  uint8_t system, gyro, accel, mag = 0;
  OrientationSensor.getCalibration(&system, &gyro, &accel, &mag);
}

String flightTimeStr(unsigned long t) {
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

float updateFrequency(unsigned long _time){
  float frequency = 1.0 / ((_time - pastTime)/1000.0);
  pastTime = _time;
  return frequency;
}

void updateData(){
  LED2.on();
  timerSeconds = (timer * 1.0) / 1000.0;
  fullTimer = flightTimeStr(timerSeconds);
  frequencyHz = updateFrequency(timer);

  if (ThermistorInt.getStatus()) ThermistorInt.update();
  if (ThermistorExt.getStatus()) ThermistorExt.update();
  if (bnoStatus) updateBNO();
  if (gpsStatus == 1) updateGPS();

  if (PressureSensor.status()){
    PressureSensor.update();
      if (PressureSensor._altitudeFt - altFtStart > 100 && !flightTimerStatus) {
          flightStartTime = timerSeconds;
          flightTimerStatus = true;
      }
      if (flightTimerStatus) flightTimerStr = flightTimeStr(int(timerSeconds) - flightStartTime);
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

  data = data  + "," +  String(xbeeMessage); 
  xbeeMessage = "";                             // Data String Assembly end

  Screen.update(String(gpsLat,4) + "\n" + String(gpsLon,4) + "\n" + String(gpsAltFt, 2) + "ft\n" + String(SIV) + " Sats\nI:" + String(int(ThermistorInt.getTempF())) + " E:" + String(int(ThermistorExt.getTempF())) + "\n" + String(PressureSensor.msPressurePSI, 2) + " PSI");
  logData(data, dataLogName);  // Log data to SD
  Serial.println(data); // Serial Print data
  LED2.off(); 
}

bool xBeeSetup() {
  xBeeSS.begin(XBEE_BAUD);
  
  delay(300);
  counterID = 10;
  switchPosition = updateSwitchStatus();           // If switch is moved add one to the XBee Channel. 10-19
  while (updateButtonStatus() == 0){                
      if (updateSwitchStatus() != switchPosition) {
        counterID++;
        if (counterID == 20) counterID = 10;
        channelID = String(counterID);
      }
      Screen.update("XBee\nChannel:\n" + String(channelID) + "\nMove S\nto add 1\nPress: B");
      Serial.println("XBee Channel: " + String(channelID) + ". Move Switch to add 1. Press Button to continue.");
      switchPosition = updateSwitchStatus();
      while (switchPosition == updateSwitchStatus() && updateButtonStatus() == 0) delay(20); // wait for a change in switch status or a button press
  }
  counterID = 0;
  delay(300);

  counterID = 100;
  switchPosition = updateSwitchStatus();                     // If switch is moved add one to the XBee Network. A100-A200
  while (updateButtonStatus() == 0){ 
      if (updateSwitchStatus() != switchPosition) {
        counterID++;
        if (counterID == 200) counterID = 100;
        networkID = "A" + String(counterID);
      }
      Screen.update("XBee\nNetwork:\n" + String(networkID) + "\nMove S\nto add 1\nPress: B");
      Serial.println("XBee Network: " + String(networkID) + ". Move Switch to add 1. Press Button to continue.");
      switchPosition = updateSwitchStatus();
      while (switchPosition == updateSwitchStatus() && updateButtonStatus() == 0) delay(20); // wait for a change in switch status or a button press
  }
  counterID = 0;
  delay(300);

  xbeeTimerSeconds = millis();
  Screen.update("XBee Mode\nRly: S On\nRec:S Off\n\n\nPress B");
  Serial.println("XBee Mode\nRelay: Switch On\nReciever: Switch off\nPress Button to Continue");
  while(!updateButtonStatus()) {
      if(updateSwitchStatus()){
          LED3.on();
          LED5.off();
          isRelay = true;
          Screen.update("XBee Mode\nRly: S On+Rec:S Off\n\n\nPress B");
      }
      if(!updateSwitchStatus()){
          LED5.on();
          LED3.off();
          isRelay = false;
          Screen.update("XBee Mode\nRly: S On\nRec:S Off+\n\nPress B");
      }
      delay(20);
  }
  LED3.off();
  LED5.off();

  if (isRelay) {     //turn on the switch on startup to put the Arduino in Relay Mode
    LED3.on();
    isRelay = true;
    xBee.enterATmode();
    xBee.atCommand("ATRE");
    xBee.atCommand("ATCH" + channelID);
    xBee.atCommand("ATID" + networkID);
    xBee.atCommand("ATSC" + scanChID);
    xBee.atCommand("ATDL1");             //Configure XBee as a relay
    xBee.atCommand("ATMY0");
    //xBee.atCommand("ATD61");
    xBee.exitATmode();
    switchPosition = updateSwitchStatus(); 
    while (updateButtonStatus() == 0){ 
        if (updateSwitchStatus() != switchPosition) counterID++; // If switch is moved add one to the amount of xbees in the network
        Screen.update("# XBee " + String(counterID) + "\nMove S\nto add 1\n\n\nPress: B");
        Serial.println("# of XBees " + String(counterID) + ". Move S to add 1. Press B to continue.");
        switchPosition = updateSwitchStatus();
        while (switchPosition == updateSwitchStatus() && updateButtonStatus() == 0) delay(20); // wait for a change in switch status or a button press
    }
    //numberOfXbees = counterID;
    xbeeRate = counterID;
    for (int i=0; i<counterID; i++){  // create a filename for all xbee recievers that will be in the network
        xBeeCommLog[XBEE_FILE_C1] = char(i+65); // change the filename spot to corresponding char for each xbee, A is xbee 1, B is xbee 2, etc.
        _print = (sdSetup(xBeeCommLog, XBEE_FILE_N1, XBEE_FILE_N2) && sdActive) ? "SD Card   Online... Creating File... Logging to: " + String(xBeeCommLog) : (sdActive) ? "No available file names; clear SD card to enable logging" : "Card failed, or not present";  
        Serial.println(_print);
        //Serial.println(xBeeCommLog);     
        _print = (sdStatus && sdActive) ? "Logging:\n\n" + String(xBeeCommLog) + "\n\n" + String(xBeeCommLog[XBEE_FILE_C1]) : (sdActive) ? "NAFM\nClear SD" : "SD\nFailed";
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
  else {
    LED5.on();
    xBee.enterATmode();      //Configure XBee as a reciever
    xBee.atCommand("ATRE");  //Reset to Default Parameters
    xBee.atCommand("ATCH" + channelID);
    xBee.atCommand("ATID" + networkID);
    xBee.atCommand("ATSC" + scanChID);
    xBee.atCommand("ATDL0"); //The DL and MY values of XBees that talk to each other should be inverted
    xBee.atCommand("ATMY1"); //Note how these are the reverse of the above settings for the relay unit
    //xBee.atCommand("ATD61");
    xBee.exitATmode();
    counterID = 1;
    switchPosition = updateSwitchStatus();
    while (updateButtonStatus() == 0){
        if (updateSwitchStatus() != switchPosition){ // If switch is moved, add one to xbee ID
          counterID++;
          if (counterID < 11) xbeeID = xbeeID.substring(0, xbeeID.length()-1) + String(counterID);
          else xbeeID = xbeeID.substring(0, xbeeID.length()-2) + String(counterID);
          xBeeCommLog[XBEE_FILE_C1] = char(counterID+64); // change the filename spot to corresponding char for each xbee, A is xbee 1, B is xbee 2, etc.
        }
        Screen.update("XBee ID:\n" + xbeeID + "\n" + String(xBeeCommLog) + "\nMove S\nto add 1\n\nPress: B");
        Serial.println("XBee ID:" + xbeeID + "\nMove Swtich to add 1, Press Button to Skip");
        switchPosition = updateSwitchStatus();
        xBeeFilenames[counterID-1][0] = xBeeCommLog[XBEE_FILE_C1]; 
        xBeeFilenames[counterID-1][1] = xBeeCommLog[XBEE_FILE_N1]; 
        xBeeFilenames[counterID-1][2] = xBeeCommLog[XBEE_FILE_N2]; 
        while (switchPosition == updateSwitchStatus() && updateButtonStatus() == 0) delay(20); // wait for a change in switch status or a button press
    }
    _print = (sdSetup(xBeeCommLog, XBEE_FILE_N1, XBEE_FILE_N2) && sdActive) ? "SD Card   Online... Creating File... Logging to: " + String(xBeeCommLog) : (sdActive) ? "No available file names; clear SD card to enable logging" : "Card failed, or not present";  
    Serial.println(_print);
    _print = (sdStatus && sdActive) ? "Logging:\n\n" + String(xBeeCommLog) : (sdActive) ? "NAFM\nClear SD\nSkip: B" : "SD\nFailed\nSkip: B";
    Screen.update(_print);
    xBeeFilenames[counterID-1][1] = xBeeCommLog[XBEE_FILE_N1]; 
    xBeeFilenames[counterID-1][2] = xBeeCommLog[XBEE_FILE_N2]; 
    timer = millis();
    wait(DELAY_SETUP);
    delay(100);

    _print = "Data String Recieved, Other";
    logData(_print, xBeeCommLog);
    xbeeID = "X" + String(counterID);
    //updateXBee(LOG_HEADER); 
  }

  logData("Start of Flight Bin,2,3,4,5,6,7,8", xBeeBinLog);
  
  _print = (sdSetup(xBeeSendLog, XBEE_SENT_N1, XBEE_SENT_N2) && sdActive) ? "SD Card   Online... Creating File... Logging to: " + String(xBeeSendLog) : (sdActive) ? "No available file names; clear SD card to enable logging" : "Card failed, or not present";  
  Serial.println(_print);
  _print = (sdStatus && sdActive) ? "Logging:\n\n" + String(xBeeSendLog) : (sdActive) ? "NAFM\nClear SD" : "SD\nFailed";
  Screen.update(_print);
  timer = millis();
  wait(DELAY_SETUP);
  logData("XBee Sent,2,3,4,5,6,7,8", xBeeSendLog);
  
  xBeeSS.setTimeout(serialTimeout);
  xBee.setTimeout(serialTimeout);
  return isRelay; 
}

void listenXBee(){
  String recievedID = "";
  byte split;  //question mark separates id from command

  //digitalWrite(27, LOW);  // turn the RST on LOW for to allow data to be sent to the Teensy
  //Just pipe data from computer to XBee
  while (Serial.available() > 0) {
      String serial = Serial.readStringUntil('\n');
      Serial.print(serial);
      xBeeSS.print(xbeeID + IDterminator + serial + stringTerminator + String(inboxTerminator));
  }

  while (xBeeSS.available() > 0) {
    LED3.on();
    //Serial.println(xBeeSS.available());
    xbeeSerialData = buildString();
    //digitalWrite(27, HIGH);  // turn the RST on HIGH for flow control

    if (xbeeSerialData == "") return;        //No data was received 
    Serial.println(xbeeSerialData);  // for troubleshooting xbee messages

    if (xbeeSerialData.indexOf(IDterminator) != -1){ // If no IDterminator is found indexOf will return -1, so if it doesn't equal -1 continue
        split = xbeeSerialData.indexOf(IDterminator);  //question mark separates id from command
        recievedID = (xbeeSerialData.substring(0, split));
        //Serial.println(recievingID); // troubleshooting
        if (recievedID != xbeeID && !isRelay) break;
        xbeeSerialData = (xbeeSerialData.substring(split + 1, xbeeSerialData.length()-1));
        //Serial.println("Command ->" + xbeeSerialData); // troubleshooting
    }

    //LED3.off();

    if ((xbeeSerialData.substring(0,4)).equals("DATA")) { //Get full data string
        //xbeeSerialData = xbeeSerialData.substring(4);
        LED5.on();
        //xBeeSS.print(xbeeID + IDterminator + data + stringTerminator + String(inboxTerminator));
        updateXBee(xbeeID + IDterminator + data + stringTerminator + String(inboxTerminator));
        xbeeMessage = "DATA STRING TRANSMITTED";
        LED5.off();
    }
    
    else if ((xbeeSerialData.substring(0,4)).equals("FLIP")) { //SYNC Command
        //numberOfXbees = (xbeeSerialData.substring(4, 5)).toInt();  //check to see how many xbees are linked
        //Serial.println(numberOfXbees);
        if (!LED4.status()){
            LED4.on();
        }
        else LED4.off();

        // xbeeSendValue = counterID;  // Code for Sync Command
        // xbeeRate = numberOfXbees;
        // xbeeTimer = 0;
        // if (!isRelay) delay(1500);
    }
      
    else if ((xbeeSerialData.substring(0,2)).equals("BL")) { //blink command
        byte times = (xbeeSerialData.substring(2, xbeeSerialData.length())).toInt();  //check to see how many times to blink
        for (; times > 0; times--) {
            LED4.on();
            delay(250);
            LED4.off();
            delay(250);
        }
    }
    else if (xbeeSerialData.equals("TIME"))  //report total time on command
        xBeeSS.print(xbeeID + IDterminator + "Time on (ms): " + String(timer) + stringTerminator + String(inboxTerminator));
  
      //Serial.println(int(recievingID[4])-48-1); // Troubleshooting
    if (recievedID != "" && (recievedID.substring(0, indexOfIntID)).equals(xbeeID.substring(0, xbeeID.length()-1))){
        byte var = int(recievedID[indexOfIntID])-48-1;
        //if (isRelay){
            xBeeCommLog[XBEE_FILE_C1] = xBeeFilenames[var][0];
            xBeeCommLog[XBEE_FILE_N1] = xBeeFilenames[var][1];
            xBeeCommLog[XBEE_FILE_N2] = xBeeFilenames[var][2];
        //}
        if(xbeeSerialData.length()<30){
            xbeeSerialData += "," + String(gpsMonth) + "/" + String(gpsDay) + "/" + String(gpsYear) + "," +
                              String(gpsHour) + ":" + String(gpsMinute) + ":" + String(gpsSecond);
            logData(recievedID + "," + xbeeSerialData, xBeeBinLog);
        }
        else  logData(xbeeSerialData, xBeeCommLog);
    }
    else  {
        xbeeSerialData += "," + String(gpsMonth) + "/" + String(gpsDay) + "/" + String(gpsYear) + "," +
                          String(gpsHour) + ":" + String(gpsMinute) + ":" + String(gpsSecond);
        logData(xbeeSerialData, xBeeBinLog);
    }

    //Serial.println(xbeeSerialData);
    xbeeSerialData = ""; // delete the data that was recieved
  }
  //digitalWrite(27, HIGH);  // turn the RST on HIGH for flow control
  
  LED3.off();
}


void updateXBee(String Data){
  //String newData = Data;
  LED5.on();
  // if(Data.length() > stringLen){
  //     xBeeSS.print(xbeeID + IDterminator + "DATA" + Data.substring(0,stringLen));
  //     Data = Data.substring(stringLen);
  //     delay(10);
  //     while(Data.length() > stringLen){
  //         xBeeSS.print(Data.substring(0,stringLen));
  //         Data = Data.substring(stringLen);
  //         delay(10);
  //     }
  //     xBeeSS.print(Data.substring(0,stringLen) + stringTerminator + String(inboxTerminator));
  //     delay(10);
  // }
  // else xBeeSS.print(xbeeID + IDterminator + Data + "\n" + String(inboxTerminator));
  //xBeeSS.print(xbeeID + IDterminator + Data + stringTerminator + String(inboxTerminator));
  Serial.println(Data);
  xBeeSS.print(Data);
  if(Data.length()<30){
      Data += "," + String(gpsMonth) + "/" + String(gpsDay) + "/" + String(gpsYear) + "," +
                    String(gpsHour) + ":" + String(gpsMinute) + ":" + String(gpsSecond);
      logData(Data, xBeeSendLog);
  }
  //xbeeMessage = "DATA REQUEST SENT";
  //delay(20);
  LED5.off();
}


String buildString(){
  unsigned long xbeeDataTimer = millis(); //soft pause for thisfunction
  String command = xBeeSS.readStringUntil(inboxTerminator);
  if (command.equals("")) return "";
  if (command.equals("+++")) return "+++";
  if (command.indexOf(IDterminator) == -1) return command; // If no IDterminator is found indexOf will return -1 and the function will exit with the command
  // while ((command.substring(0,6)).indexOf(IDterminator) != -1 && command.indexOf(IDterminator) == command.lastIndexOf(IDterminator) && millis() - xbeeDataTimer < 800){
  //     command += xBeeSS.readStringUntil(inboxTerminator);
  // }
  if (command.substring(command.length()-1) != stringTerminator){
    while(command.substring(command.length()-1) != stringTerminator && millis() - xbeeDataTimer < 800){
        //Serial.println(command.substring(command.length()-1));
        //Serial.println(command);
        command += xBeeSS.readStringUntil(inboxTerminator);
    }
  }
  //Serial.println("Command -> " + command); // Print command for testing purposes
  return command;
}