/*
  ThingSpeak Example Using Arduino MKR1000

  Version 1.0.1
  
  Environment monitoring utilizing the BMP180 Pressure Sensor
  and the DHT11 Humidity Sensor that will report data every
  5 minutes to ThingSpeak through the wireless Arduino MKR1000.

  Device:     I2C Address:
  -------     -----------
  BMP180      0x77
  
  Connection / Wiring Information:
  
  DHT11     MKR1000
  Pin:      Connected To:

  1         5 VDC
  2         Pin 5
  3         GND

  BMP180    MKR1000
  Pin:      Connected To:

  1         SDA (Pin 11)
  2         SCL (Pin 12)
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
#include <DHT.h>
#include <SFE_BMP180.h>
#include <WiFi101.h>                //  used for WiFi connectivity...
#include "ThingSpeak.h"             //  used for ThingSpeak connection...

#define DHT_SENSOR_PIN 5            //  value for pin connected to DHT11 sensor...
#define DHT_SENSOR_TYPE DHT11       //  value for sensor type...
#define ALTITUDE 69.0               //  altitude for Germantown TN (in meters)...

/********************/
/*  CONSTRUCTORS    */
/********************/
//  initialize DHT11 sensor...
DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);
//  initialize the I2C BMP180 pressure sensor board (address = 0x77)...
SFE_BMP180 prs;

/******************************************/
/*  PLACE YOUR NETWORK INFORMATION HERE   */
/******************************************/
char ssid[] = "YOUR_SSID";          //  network SSID...
char pass[] = "YOUR_PASSWORD";      //  network password...

/******************************************************/
/*  PLACE YOUR THINGSPEAK KEY AND CHANNEL NUMBER HERE */
/******************************************************/
//  ThingSpeak WRITE key (master)...
char thingSpeakKey[] = "YOUR_API_WRITE_KEY";
//  ThingSpeak Channel number (6 digits)...
unsigned long myChannelNumber = 0;

/********************/
/*  VARIABLES       */
/********************/
const int ledPin = 6;               //  on board LED...
float inHg_constant = 0.0295333727; //  convert pressure read as 1 Pa to inHg...
char stat;                          //  variable for SparkFun BMP180 library for status...
double T,P,p0,a;                    //  variables for SparkFun BMP180 library for temp and pressure...
float temperature = 0.0;            //  variable to store reading from DHT11 sensor...
float humidity = 0.0;               //  variable to store humidity from DHT11 sensor...
long rssi = 0;                      //  used to store WiFi RSSI reading value...
int blinkDelay = 100;               //  variable to delay between LED on / off state...

int retries = 0;                    //  keep track of WiFi reconnect attempts...
int maxRetries = 3;                 //  define the number of retry attempts...
int totalRetries = 0;               //  store the total number of attempts to reconnect to WiFi...
String boardStatus = "";            //  string to hold reconnect attempts...

int tsResponse = 0;                 //  stores the response received from ThingSpeak server...
int led_pin = 6;                    //  on board LED specific to the MKR1000 is pin 6...

int wifiConnected = 7;              //  GREEN LED to indicate WiFi is connected...
int wifiDisconnected = 8;           //  RED LED to indicate WiFi is disconnected...
int wifiRetry = 9;                  //  YELLOW LED to indicate WiFi is in retry attempt state...

const long fiveMinutes = 300000;    //  delay time between posting to ThingSpeak...
const long tenSeconds = 10000;      //  delay time of 10 seconds (WiFi reconnect time)...
const long fiveSeconds = 5000;      //  delay time of 5 seconds (WiFi pause)...
long lastConnectionTime = 0;        //  last connection time to the server...

int status = WL_IDLE_STATUS;        //  the WiFi radio status...
WiFiClient client;                  //  WiFi client instance...

/************************************************************************/
/*  Posting Interval to ThingSpeak:                                     */
/*  It is recommended to post your data to ThingSpeak every 5 minutes   */
/*  as this will keep the channel well within the bounds of the free    */
/*  user license.  This will allow you to render 288 data points per    */
/*  24 hour period without the concern of exceeding your data limits    */
/*  under the free license.  12 data points/hr x 24 hours = 288 total 	*/
/************************************************************************/
//  set the posting interval desired here...
const long postingInterval = fiveMinutes;

void setup()
{
  Serial.begin(9600);               //  start the serial port...

  pinMode(ledPin, OUTPUT);          //  Pin 6 has an LED connected...
  digitalWrite(ledPin, LOW);        //  turn the LED off...

  //  initialize the DHT11 sensor...
  dht_sensor.begin();

  /************************************************************/
  /*  NOTE:  For the BMP180 sensor, you must have the proper  */
  /*         pin connections to the I2C bus.  Pin 1 of the    */
  /*         BMP180 connects to SDA (Pin 11 on MKR1000) and   */
  /*         pin 2 of the BMP180 connects to SCL (Pin 12 on   */
  /*         MKR1000)										  */
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

  pinMode(wifiConnected, OUTPUT);         //  green LED indicating WiFi connected...
  digitalWrite(wifiConnected, LOW);       //  turn the LED off...
  pinMode(wifiDisconnected, OUTPUT);      //  red LED indicating WiFi disconnected...
  digitalWrite(wifiDisconnected, LOW);    //  turn the LED off...
  pinMode(wifiRetry, OUTPUT);             //  yellow LED indicating WiFi retry connect...
  digitalWrite(wifiRetry, LOW);           //  turn the LED off...

  /****************************************************************/
  /*  NOTE:  you must comment out the code below if you want this */
  /*         program to run stand-alone (meaning not connected to */
  /*         a computer for serial port monitoring).  if you want */
  /*         to monitor the serial port output, then leave this   */
  /*         code uncommented so data will be sent to the serial  */
  /*         port monitor.                                        */
  /****************************************************************/
  //  wait for the serial port to connect if using PC to monitor COM port...
  
  while(!Serial)
  {
    ;
  }
  
  Serial.println(F("Serial port active..."));
  
  //  ensure WiFi is available...
  if(WiFi.status() == WL_NO_SHIELD)
  {
    Serial.println(F("WiFi shield not present..."));
    //  set status LED's...
    SetStatusLEDs(false, true, false);
    //  remain in infinite loop here...
    while(true);
  }
  
  //  attempt to connect to WiFi network...
  while(status != WL_CONNECTED)
  {
    Serial.print(F("Attempting to connect to WPA SSID: "));
    Serial.println(ssid);
    
    //  connect to WPA/WPA2 network...
    status = WiFi.begin(ssid, pass);

    //  set status LED's - technically trying to connect...
    SetStatusLEDs(false, false, true);
    
    //  wait 10 seconds for WiFi connection...
    delay(tenSeconds);
  }

  //  show you are successfully connected to the network...
  Serial.println(F("Successful connection to the network..."));

  //  set status LED's showing we are connected...
  SetStatusLEDs(true, false, false);
    
  /**********************************************/
  /*  ThingSpeak client start...                */
  /**********************************************/
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
  if(!client.connected() && ((millis() - lastConnectionTime) > postingInterval))
  {
    //  check the WiFi status on entry...
    status = WiFi.status();

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
      //  pull the rssi value from the MKR1000 board...
      rssi = WiFi.RSSI();
      
      //  we have valid data so prepare to send the data to ThingSpeak...
      //  take all of our readings from the sensors and pack them for transmission...
      ThingSpeak.setField(1,(float)temperature);        //  DHT11 temperature reading...
      ThingSpeak.setField(2,(float)humidity);           //  DHT11 humidity reading...
      ThingSpeak.setField(3,(float)p0);                 //  BMP180 pressure reading...
      ThingSpeak.setField(4,(float)rssi);               //  MKR1000 RSSI reading...

      if(status == WL_CONNECTED) 
      {
        //  set status LED's showing we are connected...
        SetStatusLEDs(true, false, false);
            
        //  write the fields that have been set to ThingSpeak...
        tsResponse = ThingSpeak.writeFields(myChannelNumber, thingSpeakKey);
  
        if(tsResponse == 200)
        {
          Serial.println("(1) Data sent to ThingSpeak...");
        }
        else
        {
          Serial.println("Problem updating channel. HTTP error code " + String(tsResponse));
        }
      }
      else if(status != WL_CONNECTED)
      {
        //  WiFi lost connection, retry...
        if(status != WL_CONNECTED)
        {
          while((status != WL_CONNECTED) && (retries <= maxRetries))
          {
            //  set status LED's showing we are trying to connect...
            SetStatusLEDs(false, false, true);
    
            //  connect to WPA/WPA2 network...
            status = WiFi.begin(ssid, pass);
            //  increment number of retries...
            retries++;
            //  keep track of uptime total reconnect attempts...
            totalRetries++;
  
            Serial.print(F("Reconnect attempt "));
            Serial.print(retries);
            Serial.println(F("..."));
            
            //  wait 5 seconds for WiFi connection...
            delay(fiveSeconds);
  
            //  if we are now connected, set retries to high number
            //  and leave the while loop...
            //if(status == WL_CONNECTED)
            //  retries = 9;
          }
  
          //  if we were able to reconnect, send the data...
          if(status == WL_CONNECTED) 
          {
            //  set status LED's showing we are connected...
            SetStatusLEDs(true, false, false);
        
            Serial.println("Successful reconnect to the network...");
            //  write the fields that have been set to ThingSpeak...
            //ThingSpeak.writeFields(myChannelNumber, thingSpeakKey);
            tsResponse = ThingSpeak.writeFields(myChannelNumber, thingSpeakKey);
  
            if(tsResponse == 200)
            {
              Serial.println("(2) Data sent to ThingSpeak...");
            }
            else
            {
              Serial.println("Problem updating channel. HTTP error code " + String(tsResponse));
            }
          }
          else
          {
            //  we have lost the WiFi connection and gone past our retry attempts,
            //  so show disconnected.  maybe we'll get it on the next loop...
            SetStatusLEDs(false, true, false);
          }
        }
        else
        {
          
        }
  
        //  reset the retries counter...
        retries = 0;
      }
      
      //  print out readings from the sensors to the serial port if
      //  in DEBUG mode.  these will be the values sent to ThingSpeak...
      PrintBMP180ReadingToConsole();
      PrintDHT11ReadingToConsole();
      PrintRSSIReadingToConsole(rssi);
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
  Serial.println(F("ThingSpeak Client Example with MKR1000"));
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
 * PrintRSSIReadingToConsole
 * Method used to print out RSSI reading from
 * the RSSI to the serial port if in DEBUG mode.
 */
void PrintRSSIReadingToConsole(long theRssi)
{
  //  print readings to the console if enabled...
  Serial.print(F("RSSI From MKR1000 Board:  "));
  Serial.println(theRssi);
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

/**
 * TurnAllStatusLightsOn
 * Turns on all LED's 
 */
void TurnAllStatusLightsOn()
{
  digitalWrite(wifiConnected, HIGH);
  digitalWrite(wifiDisconnected, HIGH);
  digitalWrite(wifiRetry, HIGH);
}

/**
 * TurnAllStatusLightsOff
 * Turns off all LED's 
 */
void TurnAllStatusLightsOff()
{
  digitalWrite(wifiConnected, LOW);
  digitalWrite(wifiDisconnected, LOW);
  digitalWrite(wifiRetry, LOW);
}

/**
 * SetStatusLEDs
 * Sets the individual LED's based on the WiFi
 * state while in execution.
 */
void SetStatusLEDs(bool c, bool d, bool r)
{
  if(c == true)
  {
    //  turn on WiFi connected LED...
    digitalWrite(wifiConnected, HIGH);
  }
  else if(c == false)
  {
    //  turn off WiFi connected LED...
    digitalWrite(wifiConnected, LOW);
  }
  else
  {
  }

  if(d == true)
  {
    //  turn on WiFi disconnected LED...
    digitalWrite(wifiDisconnected, HIGH);
  }
  else if(d == false)
  {
    //  turn off WiFi disconnected LED...
    digitalWrite(wifiDisconnected, LOW);
  }
  else
  {
  }

  if(r == true)
  {
    //  turn on WiFi retry LED...
    digitalWrite(wifiRetry, HIGH);
  }
  else if(r == false)
  {
    //  turn off WiFi retry LED...
    digitalWrite(wifiRetry, LOW);
  }
  else
  {
  }
}
