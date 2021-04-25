// Test Routine for temperature and leaf wetness sensors
// Sketch does not use RTC / SD datalogger shield. 
// Used to field test and calibrate sensors. 

// Libraries 
#include <dht.h>       // DHT-22 Temperature and Relative Humidity sensor libraries 

// Define temperature & relative humidity sensor DHT-22
dht DHT; 
#define dhtPin 4       // Data Digital Pin 4 
#define dhtPower 5     // Data Digital Pin 5

// Define wetness sensor FC-37 with potentiometer
#define leafPin  A0    // Data Analog Pin A0; Data Digital Pin 8
#define leafPower 9    // Power Digital Pin 9

void setup()

{

  // put your setup code here, to run once:

  Serial.begin (9600);
  
  pinMode(leafPower,OUTPUT);    // FC-37  power
  pinMode(dhtPower, OUTPUT);    // DHT-11 power 
    
  digitalWrite(leafPower, HIGH);
  digitalWrite(dhtPower, HIGH);
  pinMode (leafPin, INPUT);
  pinMode (dhtPin, INPUT);
  
}

void loop()

{

  // put your main code here, to run repeatedly:

int wetness = analogRead(leafPin);
  Serial.print("Wetness: ");
  Serial.print(wetness);

int dhtData = DHT.read22(dhtPin); // Read the sensor output
  Serial.print(", Air Temp: ");
  Serial.print(DHT.temperature);
  Serial.print(", Rel. Humid: ");
  Serial.println(DHT.humidity);
 

  delay(2000);

}
