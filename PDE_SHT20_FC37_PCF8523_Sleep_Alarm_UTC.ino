/* 
 * Power saving code implemented for , Plant Disease Ecology weather datalogger wakes on RTC-based interrupt and logs 
 * leaf wetness, air temperature, and relative humidity to SD card. 

 * Code based on tutorials at: 
 * https://thekurks.net/blog/2018/1/24/guide-to-arduino-sleep-mode 
 * https://learn.adafruit.com/adafruit-data-logger-shield
 * https://www.arduino.cc/en/Tutorial/LibraryExamples/Datalogger
 * Chris Wallis - Power saving and interrupt implementation for sleep mode on RTC clock. 
 * Jon Detka - sensor read functions, general mashing of sleep mode to sensor readings, trying hard not to break stuff, and then working harder to fix
 * broken stuff in the field.
 *
 * RTC setup based on example from Adafruit RTClib: pcf8523Countdown and Arduino Uno R3. 
 * RTC is provided by Adafruit datalogger shield with PCF8523
 * Important Note: INT/SQW pin from RTC shield must be connected to arduino pin 2 using jumper wire to trigger the interrupt. 
 */

// Load Libraries
#include <avr/sleep.h> // methods that controls the sleep modes
#include <SD.h>        // methods for accessing SD card on shield. 
#include <RTClib.h>    // Need this for the 'RTC_PCF8523' chip. 

#include <Wire.h>      // i2c library
#include <DFRobot_SHT20.h>  // DFRobot i2c library to read SHT-20 sensor


RTC_PCF8523 rtc;           //make the RTC object
const int chipSelect = 10; // Set to pin 10 for Arduino R3 Uno to connect with Adafruit datalogger SD card RTC shield. 

// Pin Definitions 
#define rtcInterruptPin 2   //Pin we are going to use to wake up the Arduino from the RTC
// Aruino Uno, pins 2 and 3 are HW interrupt capable and can be connected to the Adafruit SD datalogger shield with RTC SQ pin. 

// Define temperature & relative humidity sensor SHT-20
#define shtPower 5     // Data Digital Pin 5
DFRobot_SHT20    sht20;

// Define wetness sensor FC-37 with potentiometer
#define leafPin  A0    // Data Analog Pin A0; Data Digital Pin 8
#define leafPower 9    // Power Digital Pin 9

//initialize timer Flag to value of false.  Value of true means timer has dinged.
volatile bool g_timerFlag = false; 

// the logging file
File logfile;

void error(char *str)
{
  Serial.print("error: ");
  Serial.println(str);
  while(1);
}

// Initial Set-Up Settings
void setup() {
  Wire.begin();       //Start i2c Communication
  Serial.begin(9600); //Start Serial Comunication

    sht20.initSHT20();                                  // Init SHT20 Sensor
    delay(100);
    sht20.checkSHT20();                                 // Check SHT20 Sensor
  

// Connect to RTC
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Timer configuration is not cleared on an RTC reset due to battery backup!
  rtc.deconfigureAllTimers();
  int sampleInterval = 600; //seconds 
  DateTime now = rtc.now(); //current time
//  int now_unix = now.unixtime(); //elapsed seconds since 1/1/1970
  long lastSampleTime = now.unixtime()/sampleInterval; //calculate last 10 second clock interval
  Serial.print(lastSampleTime);

  while (now.unixtime()/sampleInterval == lastSampleTime) {
    Serial.println("waiting for start point"); 
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); //toggle the LED
    delay(1000);
    Serial.println(now.second());
    now = rtc.now();
  }
  Serial.println('end of wait');
  
  // Uncomment an example below to configure RTC interrupt:
  // rtc.enableCountdownTimer(PCF8523_FrequencyHour, 24);     // 1 day
     rtc.enableCountdownTimer(PCF8523_FrequencyMinute, 10);   // 10 minutes
  // rtc.enableCountdownTimer(PCF8523_FrequencySecond, 60);    // 10 seconds
  // rtc.enableCountdownTimer(PCF8523_Frequency64Hz, 32);     // 1/2 second
  // rtc.enableCountdownTimer(PCF8523_Frequency64Hz, 16);     // 1/4 second
  // rtc.enableCountdownTimer(PCF8523_Frequency4kHz, 205);    // 50 milliseconds

  digitalWrite(LED_BUILTIN, LOW);              // turn off the LED if it ended in the "ON" state to save power
 
  pinMode(LED_BUILTIN,OUTPUT);                 // We use the led on pin 13 to indicate when Arduino is asleep
  pinMode(rtcInterruptPin,INPUT_PULLUP);       // Set pin d2 to input using the built in pullup resistor. It gets pulled down by the clock. 
  digitalWrite(LED_BUILTIN,HIGH);              // Turn LED on.  

// Added these for sensors and SD card (SD card on pin 10 of Arduino Uno R3). 
  pinMode(2, INPUT);
  digitalWrite(2,HIGH);
  
  pinMode(leafPower,OUTPUT);      // FC-37  power
  pinMode(shtPower, OUTPUT);      // SHT-20 power
   
  analogWrite(leafPower, LOW);
  digitalWrite(shtPower, HIGH);  // Set to HIGH. 

// inserted SD card write  
  pinMode(10, OUTPUT);  // default chip select pin is set to output, even if you don't use it: requires defining const int chipSelect = 10;

// Load SD card
  while (!Serial);
  Serial.print("Initializing SD card...");

  if (!SD.begin(chipSelect)) {
    Serial.println("Initialization failed.");

    while (true); 
  }
  Serial.println("initialization done.");
}

void loop() {
 // if the loop is running, it's because we're awake.  This is because an interrupt fired. 
 // if timer has gone off, tally data and log to SD card
 // if counter interrupt has fired, increment counter tally and go back to sleep 
 // if timer flag has fired, do all the sensor reads and SD card log
 if (g_timerFlag) { 

logfile = SD.open("datalog.txt", FILE_WRITE); // Open the file datalog.txt on the SD card. Requires manually uploading for now. 

// if the file is available, write to it:
if (logfile) {

// Get the RTC time from PCF8523 and log it to SD logfile.print
DateTime now;
 
now = rtc.now(); 
  logfile.print(now.unixtime()); // Unixtime seconds since 1/1/1970. 
  logfile.print(", ");
  logfile.print(now.year(), DEC);
  logfile.print("/");
  logfile.print(now.month(), DEC);
  logfile.print("/");
  logfile.print(now.day(), DEC);
  logfile.print(" ");
  logfile.print(now.hour(), DEC);
  logfile.print(":");
  logfile.print(now.minute(), DEC);
  logfile.print(":");
  logfile.print(now.second(), DEC);
  logfile.print(", ");

  Serial.print(now.unixtime()); // Unixtime seconds since 1/1/1970. 
  Serial.print(", ");
  Serial.print(now.year(), DEC);
  Serial.print("/");
  Serial.print(now.month(), DEC);
  Serial.print("/");
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(":");
  Serial.print(now.minute(), DEC);
  Serial.print(":");
  Serial.print(now.second(), DEC);
  Serial.println();
  
// Get the data 

 int val = readLeaf();   // Take leaf wetness reading from the FC-37
// Determine categorical status of digital logging leaf wetness sensor FC-37 
// val > 975 represents voltage reading threshold at dry leaf sensor.  

  if (val >975) {
    Serial.print(val);
    Serial.print(","); 
    logfile.print(val);
    logfile.print(","); 
    
    Serial.print("Dry");
    logfile.print("Dry");
    logfile.print(", ");
  
  } else {
    Serial.print(val);
    Serial.print(","); 
    logfile.print(val);
    logfile.print(","); 
    
    Serial.print("Wet");
    logfile.print("Wet");
    logfile.print(",");
  }

// Read SHT20 temp rh sensor 

float temp = sht20.readTemperature();               // Read Temperature
float humd = sht20.readHumidity();                  // Read Humidity
    Serial.print(",");
    Serial.print(temp, 2);
    Serial.print(",");
    Serial.println(humd, 2);

    logfile.print(temp, 2);
    logfile.print(","); 
    logfile.println(humd, 2);
    delay(100);


logfile.close(); // close the file when done logging data. 

 }

// End of logging time and sensor data. 

  g_timerFlag = false; //reset timer flag
  }

// if the file isn't open, print:
else {
    Serial.println("Opening the datalog file");
  }

 delay(2000);      //wait a couple of second before going to sleep. Required I think because DHT-22 is sloooow. 
 Going_To_Sleep(); //go back to sleep and wait for next event to be triggered by an interrupt regardless of what woke up processor.  
}

void Going_To_Sleep(){
    sleep_enable();//Enabling sleep mode
   
    attachInterrupt(digitalPinToInterrupt(rtcInterruptPin), wakeUp, FALLING); //attaching a interrupt to pin d2. pre-defined function wakeUp instead of defining our own ISR.
    Serial.println("Sleeping");
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);           // Setting the sleep mode, in our case full sleep
    digitalWrite(LED_BUILTIN,LOW);                 // turning LED off
    delay(100);                                    // wait a bit to allow the led to be turned off before going to sleep
    sleep_cpu();                                   // activating sleep mode. stays here until sleep_disable() is executed in wakeup
    Serial.println("Logging");                     // next line of code executed after the interrupt 
    digitalWrite(LED_BUILTIN,HIGH);                // turning LED on

  }

// Interrupt service request for when wakeup timer fires
void wakeUp(){         
  Serial.println("Interrupt Fired");      // Print message to serial monitor for diagnostic purposes only. 
  sleep_disable();                        // Disable sleep mode
  g_timerFlag = true;                     // set timer flag to true to indicate timer has expired
  detachInterrupt(0);                     // Removes the interrupt from pin 2;

}

//  Function to wake-up leaf wetness sensor and return digital value (0 = wet, 1 = dry). Reduces corrosion build-up
int readLeaf() { 
  digitalWrite(leafPower, HIGH);   // Turn the sensor ON
  delay(100);                      // Allow power to settle
  int val = analogRead(leafPin);   // Read the sensor output
  digitalWrite(leafPower, LOW);    // Turn the sensor OFF
  return val;                      // Return the value
}
