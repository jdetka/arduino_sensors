/* 
 * Power saving code implemented for , Plant Disease Ecology weather datalogger wakes on RTC-based interrupt and logs 
 * leaf wetness, air temperature, and relative humidity to SD card. 

 * Code based on tutorials at: 
 * https://thekurks.net/blog/2018/1/24/guide-to-arduino-sleep-mode 
 * https://learn.adafruit.com/adafruit-data-logger-shield
 * https://www.arduino.cc/en/Tutorial/LibraryExamples/Datalogger
 * Chris Wallis - Power saving and interrupt implementation. 
 * Jon Detka - sensor read functions and mashing.
 *
 * RTC setup based on example from Adafruit RTClib: pcf8523Countdown and Arduino Uno R3. 
 * RTC is provided by Adafruit datalogger shield with PCF8523
 * Important Note: INT/SQW pin from RTC shield must be connected to arduino pin 2 using jumper wire to trigger the interrupt. 
 */

//Libraries
#include <avr/sleep.h> // methods that controls the sleep modes
#include <SD.h>        // methods for accessing SD card on shield. 
#include <dht.h>       // DHT-11 Temperature and Relative Humidity sensor libraries 
#include <RTClib.h>    // When this is not included I get error: 'RTC_PCF8523' does not name a type

RTC_PCF8523 rtc;           //make the RTC object
const int chipSelect = 10; // Set to pin 10 for Arduino R3 Uno to connect with Adafruit shield. 

// Pin Definitions 
#define rtcInterruptPin 2   //Pin we are going to use to wake up the Arduino from the RTC
// Aruino Uno, pins 2 and 3 are HW interrupt capable and can be connected to the Adafruit SD datalogger shield with RTC SQ pin. 

// Define temperature & relative humidity sensor DHT-22
dht DHT; 
#define dhtPin 4       // Data Digital Pin 4 
#define dhtPower 5     // Data Digital Pin 5

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
  Serial.begin(9600); //Start Serial Comunication

// Connect to RTC
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }
  // Timer configuration is not cleared on an RTC reset due to battery backup!
  rtc.deconfigureAllTimers();

  // Uncomment an example below to configure RTC interrupt:
  // rtc.enableCountdownTimer(PCF8523_FrequencyHour, 24);     // 1 day
     rtc.enableCountdownTimer(PCF8523_FrequencyMinute, 10);   // 10 minutes
  // rtc.enableCountdownTimer(PCF8523_FrequencySecond, 5);    // 10 seconds
  // rtc.enableCountdownTimer(PCF8523_Frequency64Hz, 32);     // 1/2 second
  // rtc.enableCountdownTimer(PCF8523_Frequency64Hz, 16);     // 1/4 second
  // rtc.enableCountdownTimer(PCF8523_Frequency4kHz, 205);    // 50 milliseconds

  pinMode(LED_BUILTIN,OUTPUT);                 // We use the led on pin 13 to indicate when Arduino is asleep
  pinMode(rtcInterruptPin,INPUT_PULLUP);      // Set pin d2 to input using the built in pullup resistor
  digitalWrite(LED_BUILTIN,HIGH);             // Turn LED on 

// Added these for sensors and SD card (SD card on pin 10 of Arduino Uno R3). 
  pinMode(2, INPUT);
  digitalWrite(2,HIGH);
  
  pinMode(leafPower,OUTPUT);    // FC-37  power
  pinMode(dhtPower, OUTPUT);    // DHT-11 power
  
//digitalWrite(leafPower, LOW); 
  analogWrite(leafPower, LOW);
  digitalWrite(dhtPower, LOW); 

// inserted SD card write here 
  pinMode(10, OUTPUT);  // default chip select pin is set to output, even if you don't use it: requires defining const int chipSelect = 10;

 while (!Serial);

  Serial.print("Initializing SD card...");

  if (!SD.begin(chipSelect)) {

    Serial.println("Initialization failed. Here's some things to check:");
    Serial.println("1. Is an SD card inserted with the file 'datalog.txt'?");
    Serial.println("2. Is your wiring correct? (especially the interrupt");
    Serial.println("3. Is the chipSelect pin matching your shield or module?");
    Serial.println("Press reset or reopen this serial monitor after fixing the issue!");

    while (true); 
  }

  Serial.println("initialization done.");

}
  
void loop() {
 // if the loop is running, it's because we're awake.  This is because an interrupt fired
 // if timer has gone off, tally data and log to SD card
 // if counter interrupt has fired, increment counter tally and go back to sleep 
 if (g_timerFlag) { //if timer flag has fired, do all the sensor reads and SD card log

logfile = SD.open("datalog.txt", FILE_WRITE); // Open the file datalog.txt on the SD card. Requires manually uploading for now. 

// if the file is available, write to it:
if (logfile) {

// Get the RTC time from PCF8523 and log it to SD logfile.print
DateTime now;
 
now = rtc.now(); 
// logfile.print(now.unixtime()); // Unixtime seconds since 1/1/1970. 
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
  
// Get data from DHT-22 using dhtRead()function. See lines at end for dhtRead() function. 
 int dhtData = dhtRead();    // Read DHT-22 sensor for temperature and relative humidity
 int val = readSensor(); // Take leaf wetness reading from the FC-37
 
// Determine categorical status of digital logging leaf wetness sensor FC-37 
// val > 975 represents voltage reading threshold at nearly dry sensor. smallest possible drop bridging sensor circuit paths. 
// Logs val and categorical wet / dry reading. 

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
    
  logfile.print(DHT.temperature);
  logfile.print(","); 
  logfile.println(DHT.humidity);

  Serial.print(",");
  Serial.print(DHT.temperature);
  Serial.print(", ");
  Serial.println(DHT.humidity);
 
logfile.close(); // close the file when done logging data. 

 }

// End of logging time and sensor data. 
  
  g_timerFlag = false; //reset timer flag

  }

// if the file isn't open, print:
else {
    Serial.println("Opening the datalog file");

  }

 delay(2000);      //wait a couple of second before going to sleep. Required I think because DHT-11 is sloooow. 
 Going_To_Sleep(); //go back to sleep and wait for next event to be triggered by an interrupt regardless of what woke up processor.  
}

void Going_To_Sleep(){
    sleep_enable();//Enabling sleep mode
   
    attachInterrupt(digitalPinToInterrupt(rtcInterruptPin), wakeUp, FALLING); //attaching a interrupt to pin d2. pre-defined function wakeUp instead of defining our own ISR.
    Serial.println("Sleeping");
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);//Setting the sleep mode, in our case full sleep
    digitalWrite(LED_BUILTIN,LOW);//turning LED off
    delay(100); //wait a bit to allow the led to be turned off before going to sleep
    sleep_cpu();//activating sleep mode. stays here until sleep_disable() is executed in wakeup
    Serial.println("Logging");//next line of code executed after the interrupt 
    digitalWrite(LED_BUILTIN,HIGH);//turning LED on
  }

// Interrupt service request for when wakeup timer fires
void wakeUp(){         
  // Serial.println("Interrupt Fired");   // Print message to serial monitor for diagnostic purposes only. 
  sleep_disable();                        // Disable sleep mode
  g_timerFlag = true;                     // set timer flag to true to indicate timer has expired
  detachInterrupt(0);                     // Removes the interrupt from pin 2;
}

//  Function to wake-up leaf wetness sensor and return digital value (0 = wet, 1 = dry). Reduces corrosion build-up
int readSensor() { 
  digitalWrite(leafPower, HIGH);  // Turn the sensor ON
  delay(10);                      // Allow power to settle
  int val = analogRead(leafPin); // Read the sensor output
  digitalWrite(leafPower, LOW);   // Turn the sensor OFF
  return val;                     // Return the value
}

int dhtRead() { //  Function to return DHT-22 Temperature & RH digital values. 
  digitalWrite(dhtPower, HIGH);     // Turn the sensor ON
  delay(2000);                      // Allow time for sensor to collect. 
  int dhtData = DHT.read22(dhtPin); // Read the sensor output
  digitalWrite(dhtPower, LOW);      // Turn the sensor OFF
  return dhtData;                   // Return the values
  return DHT.temperature; 
  return DHT.humidity; 
}
