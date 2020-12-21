// Arduino Plant Disease Ecology Weather Station with Sleep Mode

#include <SD.h>
#include <Wire.h>
#include <dht.h>
#include "RTClib.h"

// how many milliseconds between grabbing data and logging it. 
#define LOG_INTERVAL  300000 // mills between entries. 

// how many milliseconds before writing the logged data permanently to disk
// set it to the LOG_INTERVAL to write each time (safest)
// set it to 10*LOG_INTERVAL to write all data every 10 datareads, you could lose up to 
// the last 10 reads if power is lost but it uses less power and is much faster!
#define SYNC_INTERVAL 1000 // mills between calls to flush() - to write data to the card
uint32_t syncTime = 0; // time of last sync()

#define ECHO_TO_SERIAL   1 // echo data to serial port
#define WAIT_TO_START    0 // Wait for serial input in setup()
 
// Define temperature & relative humidity sensor DHT_11
dht DHT; 
#define dhtPin 2       // Data Digital Pin 2 
#define dhtPower 3     // Data Digital Pin 3

// Define wetness sensor FC-37 with potentiometer
#define leafPin 8    // Data Digital Pin 8
#define leafPower 9  // Power Digital Pin 9

RTC_PCF8523 RTC; // define the Real Time Clock object

const int chipSelect = 10; // Set to pin 10 for Arduino R3 Uno

// the logging file
File logfile;

void error(char *str)
{
  Serial.print("error: ");
  Serial.println(str);
  
  // red LED indicates error
  // digitalWrite(redLEDpin, HIGH);

  while(1);
}

// Intial settings; 
void setup(void) 
{
    Serial.begin(9600);
    Serial.println();
    
  pinMode(leafPower,OUTPUT);
  pinMode(dhtPower, OUTPUT);
  
  pinMode(LED_BUILTIN,OUTPUT);
  digitalWrite(LED_BUILTIN,LOW);
  
  digitalWrite(leafPower, LOW); 
  digitalWrite(dhtPower, LOW); 

  #if WAIT_TO_START
    Serial.println("Type any character to start");
    while(!Serial.available());
  #endif //WAIT_TO_START

  // initialize the SD card
  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);

// see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    error("Card failed, or not present");
  }
  Serial.println("card initialized.");
  
  // create a new file
  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE); 
      break;  // leave the loop!
    }
  }
  
  if (! logfile) {
    error("couldnt create file");
  }
  
  Serial.print("Logging to: ");
  Serial.println(filename);

  // connect to RTC
  Wire.begin();  
  if (!RTC.begin()) {
    logfile.println("RTC failed");
#if ECHO_TO_SERIAL
    Serial.println("RTC failed");
#endif  //ECHO_TO_SERIAL
  }
  
  logfile.println("datetime,lw,temp,rh");    
#if ECHO_TO_SERIAL
  Serial.println("datetime,lw,temp,rh");
#endif //ECHO_TO_SERIAL
 
  // If I want to set the aref to something other than 5v
  analogReference(EXTERNAL);
}

void loop(void)
{
  DateTime now;

  // delay for the amount of time we want between readings
  delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL));
  
  // digitalWrite(greenLEDpin, HIGH);
  
  // log milliseconds since starting
  // uint32_t m = millis();
  // logfile.print(m);         // milliseconds since start
  // logfile.print(", ");    
#if ECHO_TO_SERIAL
  // Serial.print(m);         // milliseconds since start
  // Serial.print(", ");  
#endif

  // fetch the time
  now = RTC.now();
  // log time
  // logfile.print(now.unixtime()); // seconds since 1/1/1970
//  logfile.print(", ");
//  logfile.print('"');
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
//  logfile.print('"');
  logfile.print(", ");
#if ECHO_TO_SERIAL
  //Serial.print(now.unixtime()); // seconds since 1/1/1970
  //Serial.print(", ");
  //Serial.print('"');
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
  //Serial.print('"');
  // Serial.print(", ");
#endif //ECHO_TO_SERIAL

// Get data from DHT-11 
  //  digitalRead(dhtPin);
  //  delay(2000); 
  // int dhtReading = DHT.read11(dhtPin); // DHT11

  // float t = DHT.temperature; 
  // float h = DHT.humidity; 

digitalWrite(LED_BUILTIN,HIGH); // Flash the light! 
int dhtData = dhtRead();    // Read DHT sensor
  
// Take leaf wetness reading 
//  delay(2000); 
//  int val = digitalRead(leafPin); 

// Take leaf wetness reading 
  int val = readSensor();
 
// Determine status of leaf wetness sensor
  if (val) {
    Serial.print(", ");
    Serial.print("Dry");
   
    logfile.print("Dry");
    logfile.print(", "); 
    
  } else {
    Serial.print(", ");
    Serial.print("Wet");
    
    logfile.print("Wet");
    logfile.print(", ");
  }
   
//  logfile.print(dhtReading);  
  logfile.print(DHT.temperature);
  logfile.print(", "); 
  logfile.print(DHT.humidity);
  logfile.println();
  
#if ECHO_TO_SERIAL

//  Serial.print(dhtReading);
  Serial.print(", ");
  Serial.print(DHT.temperature);
  Serial.print(", ");
  Serial.print(DHT.humidity);
  Serial.println();
  
#endif //ECHO_TO_SERIAL

  // digitalWrite(greenLEDpin, LOW);

  // Now we write data to disk! Don't sync too often - requires 2048 bytes of I/O to SD card
  // which uses a bunch of power and takes time
  if ((millis() - syncTime) < SYNC_INTERVAL) return;
  syncTime = millis();
  
//  // blink LED to show we are syncing data to the card & updating FAT!
//  digitalWrite(redLEDpin, HIGH);
logfile.flush();
//  digitalWrite(redLEDpin, LOW);
  
}

//  Function to wake-up leaf wetness sensor and return digital value (0 = wet, 1 = dry). 
int readSensor() {
  digitalWrite(leafPower, HIGH);  // Turn the sensor ON
  delay(10);              // Allow power to settle
  int val = digitalRead(leafPin); // Read the sensor output
  digitalWrite(leafPower, LOW);   // Turn the sensor OFF
  return val;             // Return the value
}

//  Function to return DHT-11 Temperature & RH digital values
int dhtRead() {
  digitalWrite(dhtPower, HIGH);      // Turn the sensor ON
  delay(2000);                       // Allow time for sensor to collect. 
  int dhtData = DHT.read11(dhtPin); // Read the sensor output
  digitalWrite(dhtPower, LOW);   // Turn the sensor OFF
  return dhtData;             // Return the values
  return DHT.temperature; 
  return DHT.humidity; 
}

  
