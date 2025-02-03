/*
  ThingSpeak Example Using Ethernet Shield

  Version 1.0.1
  
  Environment monitoring utilizing the BMP180 Pressure Sensor
  and the DHT11 Humidity Sensor that will report data every
  5 minutes to ThingSpeak.

  Device:     I2C Address:
  -------     -----------
  BMP180      0x77
  
  Connection Data:
  
  DHT11     Ethernet Shield
  Pin:      Connected To:

  1         5 VDC
  2         Pin 7
  3         GND

  BMP180    Ethernet Shield
  Pin:      Connected To:

  1         SDA (A4)
  2         SCL (A5)
  3         GND
  4         5 VDC
  
  Created:   January 30, 2025
  Modified:  February 3, 2025
  by Todd S. Canaday
  toddcanaday@comcast.net

  Arduino Compiler Version:  arduino-1.8.19

  Revision Notes:
  
  01/30/2025  Initial release of source code.
  01/31/2025  Tested on home network - works now.
  02/03/2025  Release 1.0.1 code cleanup.
*/
#include <SPI.h>
#include <Ethernet.h>
#include <DHT.h>
#include <SFE_BMP180.h>
#include "ThingSpeak.h"

#define DHT_SENSOR_PIN 7            //  value for pin connected to DHT11 sensor...
#define DHT_SENSOR_TYPE DHT11       //  value for sensor type...
#define ALTITUDE 69.0               //  altitude for Germantown TN (in meters)...
#define DEBUG true                  //  set to true for console output, false for no console output...
#define Serial if(DEBUG)Serial

/********************/
/*  CONSTRUCTORS    */
/********************/
//  initialize DHT11 sensor...
DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);
//  initialize the I2C BMP180 pressure sensor board (address = 0x77)...
SFE_BMP180 prs;
//  initialize the Ethernet client...
EthernetClient client;

/********************/
/*  VARIABLES       */
/********************/
const int ledPin = 13;              //  on board LED...
int delay_time = 2000;              //  variable to delay between readings...
float inHg_constant = 0.0295333727; //  convert pressure read as 1 Pa to inHg...
char stat;                          //  variable for SparkFun BMP180 library for status...
double T,P,p0,a;                    //  variables for SparkFun BMP180 library for temp and pressure...
float temperature = 0.0;            //  variable to store reading from DHT11 sensor...
float humidity = 0.0;               //  variable to store humidity from DHT11 sensor...
int blinkDelay = 100;               //  variable to delay between LED on / off state...
const long fiveMinutes = 300000;    //  delay time between posting to ThingSpeak...
const long oneSecond = 1000;        //  delay time of 1 second...
long lastConnectionTime = 0;        //  last connection time to the server...

/************************************************************************/
/*  Posting Interval to ThingSpeak:                                     */
/*  It is recommended to post your data to ThingSpeak every 5 minutes   */
/*  as this will keep the channel well within the bounds of the free    */
/*  user license.  This will allow you to render 288 data points per    */
/*  24 hour period without the concern of exceeding your data limits    */
/*  under the free license.  12 data points/hr x 24 hours = 288 total   */
/************************************************************************/

//  set the posting interval desired here...
const long postingInterval = fiveMinutes;

//  Ethernet shield MAC address (can be any 6 byte hexadecimal number)...
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 0, 185);     //  define an unused IP address on network...

/******************************************************/
/*  PLACE YOUR THINGSPEAK KEY AND CHANNEL NUMBER HERE */
/******************************************************/
//  ThingSpeak WRITE key (master)...
char thingSpeakKey[] = "YOUR_API_WRITE_KEY";
//  ThingSpeak Channel number (6 digits)...
unsigned long myChannelNumber = 0;

void setup()
{
  Serial.begin(9600);               //  start the serial port in case debug...

  pinMode(ledPin, OUTPUT);          //  Pin 13 has an LED connected...
  digitalWrite(ledPin, LOW);        //  turn the LED off...

  //  initialize the DHT11 sensor...
  dht_sensor.begin();

  /************************************************************/
  /*  NOTE:  For the BMP180 sensor, you must have the proper  */
  /*         pin connections to the I2C bus.  Pin 1 of the    */
  /*         BMP180 connects to SDA (Pin A4 on Arduino Uno)   */
  /*         and pin 2 of the BMP180 connects to SCL (Pin A5  */
  /*         on Arduino Uno)                                  */
  /************************************************************/
  //  initialize the BMP180 sensor...
  if(prs.begin())
  {
    Serial.println(F("BMP180 initialized"));
  }
  else
  {
    //  we could not find the BMP180, so loop here
    //  forever (the program will no longer run while
    //  stuck in this loop)...
    Serial.println(F("BMP180 failed to initialize"));
    //  blink the on board LED forever...
    while(true)
    {
      digitalWrite(ledPin, HIGH);   //  turn the LED on...
      delay(blinkDelay);            //  delay...
      digitalWrite(ledPin, LOW);    //  turn the LED off...
      delay(blinkDelay);            //  delay...
    }
  }
  
  //  start the Ethernet shield...
  Serial.println(F("Starting ethernet shield..."));
  if(Ethernet.begin(mac) == 0)
  {
    Serial.println(F("Failed to configure Ethernet using DHCP..."));
    // try to congifure using IP address instead of DHCP...
    Ethernet.begin(mac, ip);
  }

  Serial.println(F("Ethernet shield started..."));
  //  give the ethernet module time to boot up...
  delay(oneSecond);

  //  let's print out the IP address, just for fun...
  Serial.print(F("IP Address:  "));
  Serial.println(Ethernet.localIP());
  
  //  initialize the ThingSpeak client...
  ThingSpeak.begin(client);
}

void loop()
{
  //  read the BMP180 sensor...
  ReadFromBMP180Sensor();
  //  read the DHT11 sensor...
  ReadFromDHT11Sensor();
  
  //  check the interval posting time.  if longer than set posting interval time,
  //  we can send the data collected to ThingSpeak...
  if(!client.connected() && (millis() - lastConnectionTime > postingInterval))
  {
    //  lets make sure that we actually got a valid reading from the sensors...
    if(isnan(temperature) || isnan(humidity) || isnan(p0))
    {
      //  if any one sensor reading failed, we will not update ThingSpeak and
      //  we will update the console as to which device failed...
      if(isnan(temperature))
      {
        //  print out error message that temperature reading failed...
        PrintSensorReadingErrorMessage(1);
      }
      else if(isnan(humidity))
      {
        //  print out error message that humidity reading failed...
        PrintSensorReadingErrorMessage(2);
      }
      else
      {
        //  print out error message that pressure reading failed...
        PrintSensorReadingErrorMessage(3);
      }
    }
    else
    {
      //  we have valid data so prepare to send the data to ThingSpeak...
      //  take all of our readings from the sensors and pack them for transmission...
      ThingSpeak.setField(1,(float)temperature);        //  DHT11 temperature reading...
      ThingSpeak.setField(2,(float)humidity);           //  DHT11 humidity reading...
      ThingSpeak.setField(3,(float)p0);                 //  BMP180 pressure reading...
  
      //  write the fields to ThingSpeak...
      ThingSpeak.writeFields(myChannelNumber, thingSpeakKey);
  
      //  print out readings from the sensors to the serial port if
      //  in DEBUG mode.  these will be the values sent to ThingSpeak...
      PrintBMP180ReadingToConsole();
      PrintDHT11ReadingToConsole();
    }

    //  set the last ms connection time for the next posting...
    lastConnectionTime = millis();
  }
}

/**
 * ReadFromBMP180Sensor
 * Method used to read the pressure
 * from the BMP180 sensor.
 */
void ReadFromBMP180Sensor()
{
  //  first, per the Sparkfun Library, we must get the temperature reading
  //  before getting the pressure reading...
  stat = prs.startTemperature();

  if(stat != 0)
  {
    //  wait for the measurement to complete...
    delay(stat);

    //  read the temperature from the device...
    stat = prs.getTemperature(T);

    if(stat != 0)
    {
      //  begin reading of pressure data...
      stat = prs.startPressure(3);
  
      if(stat != 0)
      {
        //  wait for the measurement to complete...
        delay(stat);

        //  get the absolute pressure reading...
        stat = prs.getPressure(P,T);
  
        if(stat != 0)
        {
          //  get the pressure reading compensating for sea-level (altitude)...
          p0 = prs.sealevel(P,ALTITUDE)*inHg_constant;
        }
      }
    }
  }
}

/**
 * ReadFromDHT11Sensor
 * Method used to read the humidity and
 * temperature from the DHT11 sensor.
 */
void ReadFromDHT11Sensor()
{
  //  get the humidity from the DHT11 sensor...
  humidity = dht_sensor.readHumidity();
  //  get the temperature from the DHT11 sensor...
  temperature = dht_sensor.readTemperature(true);
}

/**
 * PrintProgramInfoToConsole
 * Method used to print out program
 * information to the serial port if
 * in DEBUG mode.
 */
void PrintProgramInfoToConsole()
{
  //  print program info to console...
  Serial.println(F("ThingSpeak Client Example with Ethernet Shield"));
  Serial.println(F("February 3, 2025"));
  Serial.println();
}

/**
 * PrintBMP180ReadingToConsole
 * Method used to print out readings from
 * the sensors to the serial port if
 * in DEBUG mode.
 */
void PrintBMP180ReadingToConsole()
{
  //  print readings to the console if enabled...
  Serial.print(F("BMP180 Pressure:  "));
  Serial.print(p0);
  Serial.println(F(" inHg"));
  Serial.print(F("BMP180 Temperature:  "));
  Serial.print(((T*9/5)+32));
  Serial.println(F("°F"));
}

/**
 * PrintDHT11ReadingToConsole
 * Method used to print out readings from
 * the sensors to the serial port if
 * in DEBUG mode.
 */
void PrintDHT11ReadingToConsole()
{
  //  print readings to the console if enabled...
  Serial.print(F("DHT11 Humidity:  "));
  Serial.print(humidity);
  Serial.println(F("%"));
  Serial.print(F("DHT11 Temperature:  "));
  Serial.print(temperature);
  Serial.println(F("°F"));
}

/**
 * PrintSensorReadingErrorMessage
 * Method to print error message based on failure of
 * reading and measurement number.
 */
void PrintSensorReadingErrorMessage(int sensorNum)
{
  switch(sensorNum)
  {
    case 1:
      Serial.print(F("DHT11 Temperature reading failed..."));
      break;
    case 2:
      Serial.print(F("DHT11 Humidity reading failed..."));
      break;
    case 3:
      Serial.print(F("BMP180 Pressure reading failed..."));
      break;
    default:
      break;
  }
}
