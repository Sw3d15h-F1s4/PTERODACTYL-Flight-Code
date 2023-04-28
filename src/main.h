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
#include <Arduino.h>

#include <PT_LED.h>        // Custom Library to use LED's
#include <PT_Thermistor.h> // Custom Library to use Thermistors
#include <PT_OLED.h>       // Custom Library to use OLED
#include <PT_MS5611.h>     // Custom Library to use MS5611

#include <Adafruit_BNO055.h> // Adafruit Library for BNO055 orientation sensor
#include <TinyGPS++.h>       // Wrapper Library for FlightGPS Library <UbloxGPS.h>
#include <UbloxGPS.h>        // Library to communicate over NMEA with M9N
#include <XBee.h>            // Library for XBee Communication
#include <SD.h>              // Library for logging with the SD card on the Teensy

#include <SoftwareSerial.h> // GPS is wired to software serial pins
#include <OneWire.h>        // I2C for communicating with ___
#include <SPI.h>            // TODO: Find what uses the Serial Progamming Interface

// Settings

#define SERIAL_BAUD 115200         // Ensure serial monitor and this value are identical
#define CHIP_SELECT BUILTIN_SDCARD // Use the built-in SD card slot on the Teensy 4.1
#define SWITCH_PIN 30              // This is the pin the selection switch is wired to. Label on PCB
#define BUTTON_PIN 31              // This is the pin the button is wired to. Label on PCB

#define GPS_BAUD 39400 // Baud Rate for the GPS. This should stay at 39400 for the M9N

#define DELAY_DATA 999  // Delay between data
#define DELAY_SETUP 800 // Delay between each device's initialization
#define DELAY_XBEE 999

char dataLogName[] = "PTER00.csv";
char xBeeCommLog[] = "XBA00.csv";
char xBeeSendLog[] = "SENT00.csv";
char xBeeBinLog[] = "XBin.csv";
char xBeeFilenames[10][3];

#define DATA_FILE_N1 4
#define DATA_FILE_N2 5
#define XBEE_FILE_N1 3
#define XBEE_FILE_N2 4
#define XBEE_FILE_C1 2
#define XBEE_SENT_N1 4
#define XBEE_SENT_N2 5

// Hardware Initialization

SoftwareSerial gpsSS = SoftwareSerial(7, 8);
SoftwareSerial xBeeSS = SoftwareSerial(34, 35);

LED LED1(LED_BUILTIN);
LED LED2(2);
LED LED3(3);
LED LED4(4);
LED LED5(5);
Thermistor ThermistorInt(40);
Thermistor ThermistorExt(41);
MS5611 PressureSensor;
UbloxGPS GPS = UbloxGPS(&gpsSS);
XBee xBee(&xBeeSS);
Adafruit_BNO055 OrientationSensor = Adafruit_BNO055(55, 0x28, &Wire);
OLED Screen;

// Global Flags
bool extTherm = false;
bool intTherm = false;
bool msStatus = false;
bool bnoStatus = false;
bool sdStatus = false;
bool sdActive = false;
bool switchPosition = false;
bool gpsStatus = 0;

// Xbee Settings

bool isRelay = false;                // Whether the XBee is a relay (true) or a reciever (false)
String xbeeMessage;                  // Message recieved from XBee
byte xbeeRate = 1;                   // how many loops until it sends another data string
byte xbeeTimer = 1;                  // xbeeRate - xbeeTimer = what loop the first command is sent on
byte xbeeTimerStart = 0;             // what number the loop starts at: should be 0
byte xbeeSendValue = 1;              // can be a different value than xbeeRate, then the xbee will send on that value of xbeeTimer but only every xbeeRate
byte numberOfXbees = 1;              // Total number of xbees on network
String xbeeID = "X1";                // Choose an ID for your XBee - 2-4 character string, A-Z and 0-9 with 1 at the end
const byte indexOfIntID = 1;         // Index of the 1 in at the end of the ID
const char inboxTerminator = '&';    // Make sure that not too much is being taken from the XBees inbox
const String IDterminator = "?";     // Make sure the ID can be taken out of recieved String
const String stringTerminator = "!"; // Make sure the code knows when a full string has been built
String channelID = "10";             // XBee Channel
String networkID = "A100";           // AAAA, BBBB, CCCCC, etc. change if there are multiple networks of xbees on the balloon fliight
String scanChID = "CCCC";            // XBee Listen Channel
String xbeeSerialData;               // Incoming/Outgoing XBee Data
String xBeeSerialData;
int counterID = 0;
const int serialTimeout = 10;
unsigned long xbeeTimerSeconds;

// Global Data Storage Allocation

float magnetometer[3] = {0, 0, 0};
float accelerometer[3] = {0, 0, 0};
float gyroscope[3] = {0, 0, 0};
float orientation[3] = {0, 0, 0};

int gpsMonth = 0;
int gpsDay = 0;
int gpsYear = 0;
int gpsHour = 0;
int gpsMinute = 0;
int gpsSecond = 0;
double gpsLat = 0;
double gpsLon = 0;
double gpsAltM = 0;
double gpsAltFt = 0;
byte SIV = 0;

String _print;
File datalog;

unsigned long gpsTimer = 0;

// Data Logging
const String LOG_HEADER = "T(h:m:s),FT(h:m:s),T(Sec),UF(Hz),Sats,Date,Time,Lat,Lon,Alt(Ft),Alt(M),intT(C),intT(F),extT(C),extT(F),msTemp(C),msTemp(F),msPress(PSI),msPress(ATM),msAlt(ft),msAlt(M),VertVel(Ft/S),VertVel(M/S),mag x,mag y,mag z,accel x,accel y,accel z,gyro x,gyro y,gyro z,orien yaw,orien pitch,orien roll,xbee message";
String data;

String fullTimer = "XX:XX:XX";
unsigned long timer = 0;
float timerSeconds = 0;
unsigned int btnTimer = 0;

String flightTimerStr = "XX:XX:XX";
unsigned long flightStartTime = 0;
bool flightTimerStatus = false;
float altFtStart = 0;

unsigned long xBeeTimerSeconds;

unsigned long prevTime = 0;
float frequencyHz = 0;

// Function Declarations

/**
 * Waits a certain amount of time in milliseconds.
 * @param t time to wait in milliseconds
 *
 */
void wait(unsigned const int);

/**
 * Reads BUTTON_PIN to determine switch state.
 * Assumes the button is configured with a pullup resistor. This means that the voltage at the
 * Teensy pin is "pulled-up" to HIGH normally, and when the button makes contact it shorts this pin
 * to ground. Here's some EE210 for you: what's the voltage at a grounded pin? HIGH or LOW?
 * @returns true if button is pressed, false otherwise.
 */
bool updateButtonStatus();

/**
 * Reads SWITCH_PIN to determine the tactile switch state.
 * Works the same way as the button function, just reading a different pin. digitalRead reads a
 * voltage at a certain pin. System is at 5v. What would HIGH voltage be here?
 * What would LOW voltage be here?
 * @returns true if switch is on, fasle otherwise.
 */
bool updateSwitchStatus();

/**
 * Creates a file on the SD card with an indexed name. File names should be of the
 * format "name00", where N1 and N2 point to the first and second digit of the
 * number in the string. Each new file created will have an index 1 greater than
 * the last, for example the next file would be "name01" then "name02", etc.
 * @param dataFile Opens a file handle with name dataFile and modifies string to match current file
 * @param N1 Index of first digit
 * @param N2 Index of second digit
 * @returns Returns true if file was able to be created, false if sd card is "full" (name99 exists)
 */
bool sdSetup(char *dataFile, byte const &N1, byte const &N2);

/**
 * Prints data to a file on the SD card.
 * @param Data Bytes of line to append to file
 * @param dataFile  name of file on SD card
 */
void logData(String const &Data, char const *dataFile);

/**
 * Opens software serial communication with GPS and sends some calibration info.
 * Calibration info defined in UbloxGPS.cpp
 *
 * @note Unsure why specifically the GPS needs to be configured 50 times. Also why is this a
 * separate function when it is only used once?
 * @returns Returns true if gps is set to airborne false otherwise
 */
bool gpsSetup();

/**
 * Parses GPS values and stores them in global variables.
 */
void updateGPS();

/**
 * Reads the gyroscope, accelerometer, and magnetometer of the LSM9DS1.
 *
 * Only called when the LSM9DS1 is installed and selected at startup,
 * appears to be safe to remove otherwise.
 */
void updateIMU();

/**
 * Reads data from BNO055 Accelerometer, Magnetometer, Gyroscope, and orientation data.
 */
void updateBNO();

/**
 * Formats a time display.
 * @param t input time in seconds
 * @returns Returns a String formatted to HH:MM:SS
 */
String flightTimeStr(unsigned long t);

/**
 * Calculates the frequency of this function being called in a loop. Takes the current time as an
 * input, calculates the time step, then returns that as a frequency.
 * @param _time Input in milliseconds
 * @returns Returns the modified frequency in Hz
 */
float calculateFrequency(unsigned long _time);

/**
 * @brief Calls the other update functions, then formats their results for
 * display on the OLED screen.
 *
 * Pulses LED2 on then:
 *  - reads internal thermistor
 *  - reads external thermistor
 *  - reads the pressure sensor/altimeter
 *  - reads accelerometer/mangetometer/gyroscope data from either the LSM9DS1 or the BNO055
 *  - reads GPS data from M9N (not configured for high-precision)
 *  - checks current altitude, and if greater than initial begins flight timer
 *  - assembles this information into one large CSV string for logging on SD card
 *  - updates the OLED screen with select information
 *  - additionally sends data out on HW serial bus
 * Finally, turns LED2 off.
 *
 * Data is stored on the SD card and transmitted over serial in the following format: (no line breaks)
 * ~~~~
 * [Timer],[FlightTimer],[TimerSec],[frequencyHz],[GPS:SIV],[GPS:Month],[GPS:Day],[GPS:Year],[GPS:Hour],[GPS:Minute],[GPS:Second],[GPS:Lattitude],[GPS:Longitude],[GPS:Altimeter (ft)],[GPS:Altimeter (m)],[Internal Temp (C)],[Intenal Temp (F)],[External Temp (C)],[External Temp (F)],[PSENSE:Temp (C)],[PSENSE:Temp (F)],[PSENSE:Presusre (PSI)],[PSENSE:Pressure (atm)],[PSENSE:Altimeter (ft)],[PSENSE:Altimeter(m)],[PSENSE:Vertical Velocity (ft/s)],[PSENSE:Vertical Velocity (m/s)],[Magnetometer X],[Magnetometer Y],[Magnetometer Z],[Accelerometer X],[Accelerometer Y],[Accelerometer Z],[Gyroscope X],[Gyroscope Y],[Gyroscope Z],[Orientation X],[Orientation Y],[Orientation Z],[Message Recieved from XBee Radio]
 * ~~~~
 *
 * Data is output to the OLED Screen in the following format:
 * Lattitude
 * Longitude
 * Altitude (ft)
 * GPS:SIV
 * Internal Temp (F)   External Temp (F)
 * PSENSE: Pressure (PSI)
 */
void updateData();

/**
 * Initializes the xBee Radio and presents multiple options for the user.
 * Logs xBee communications in a separate file.
 * @returns Returns true if xBee Radio is configured as a relay.
 * @note Should probably hard-code radio channels and radio options before flight.
 */
bool xBeeSetup();

/**
 * Called when xBee is configured in recieve mode.
 * Pulses LED3 when recieving data.
 */
void listenXBee();

/**
 * Transmits data across xBee serial communications.
 * @param Data - Serialized bytes to send
 */
void updateXBee(String data);

/**
 * Reads an incoming string of data from the XBee Radio
 * First, it reads the incoming stream of data until it sees the inboxTerminator ("&")
 * Then, it checks if the string is empty or is "+++", in which case it just exits.
 * If no ID Terminator can be found, return the entire string which should be just a command.
 * If the last character of the string is "!" (stringTerminator), then just return the string
 * Otherwise, keep reading the serial until we read the string terminator followed by the inbox terminator
 *
 * @returns Returns the command that was read. Appears as if a properly formatted command will
 * end with a stringTerminator.
 */
String buildString();