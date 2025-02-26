/*
  ThingSpeak_MKR-1000_Outdoor_Sensors_v_1-0-1
  
  Version 1.0.1
  
  Program used to connect to ThingSpeak and provide outdoor
  temperature, humidity, pressure and daylight readings.

  SPECIAL NOTE:
  -------------
  You MUST use the Adafruit BMP085 library in order for this
  program to work with the MKR1000 device.
  URL:  https://github.com/adafruit/Adafruit-BMP085-Library
  Download the ZIP file, extract the folder and place that in the 
  C:\arduino-1.8.19\libraries folder.
  
  Device:     Address:
  -------     -----------
  BMP180      0x77
  AHT10       0x38 
  
  BMP180
  Pin:      Connected To:

  1         SDA (MKR1000 Pin 11)
  2         SCL (MKR1000 Pin 12)
  3         GND
  4         3.3 VDC

  AHT10
  Pin:      Connected To:

  1         SDA (MKR1000 Pin 11)
  2         SCL (MKR1000 Pin 12)
  3         GND
  4         3.3 VDC

  Light Sensor Circuit:                 VCC
                       __________        |
          100K         | Light  |        |
   ------/\/\/\---0----| Sensor |--------|
   |              |    |        |
   |              |    ----------
  GND             A1

  Assignments:

  BMP180:       Reads Temperature (BMP180_Temperature)
                Reads Pressure (BMP180_Pressure)
  AHT10:        Reads Humidity (humidity.relative_humidity)
  Light Sensor: Reads from pin A1 (analogA1)
  RSSI:         Reads RSSI from MKR-1000 board

  Created:   February 24, 2025
  Modified:  February 26, 2025
  by Todd S. Canaday
  tcanaday@memphis.edu

  Arduino Compiler Version:  arduino-1.8.13

  Revision Notes:
  
  02/24/2025  Initial release of source code.
  02/26/2025  Added Adafruit library to interface with
              the BMP180 so that the sensor would work
              with the MKR1000 board.
*/
#include <Adafruit_BMP085.h>        //  use the Adafruit BMP085 library for the BMP180...
#include <Adafruit_AHTX0.h>         //  use the Adafruit AHTX0 library for the AHT10...
#include <WiFi101.h>                //  used for WiFi connectivity...
#include "ThingSpeak.h"             //  used for ThingSpeak connection...

#define BAUDRATE 9600               //  default baud rate for console...
#define ALTITUDE 69.0               //  altitude for Germantown TN (in meters)...

/********************/
/*  CONSTRUCTORS    */
/********************/
//  initialize the I2C BMP180 pressure sensor board (address = 0x77)...
Adafruit_BMP085 bmp;
//  initialize the I2C AHT10 humidity sensor board (address = ox38)...
Adafruit_AHTX0 aht;

/******************************************/
/*  PLACE YOUR NETWORK INFORMATION HERE   */
/******************************************/
char ssid[] = "YOUR_SSID_HERE";       //  network SSID...
char pass[] = "YOUR_PASSWORD_HERE";   //  network password...

/******************************************************/
/*  PLACE YOUR THINGSPEAK KEY AND CHANNEL NUMBER HERE */
/******************************************************/
//  ThingSpeak WRITE key (master)...
char thingSpeakKey[] = "YOUR_THINGSPEAK_KEY";
//  ThingSpeak Channel number...
unsigned long myChannelNumber = 0;

/********************/
/*  VARIABLES       */
/********************/
const int ledPin = 6;               //  on board LED...
sensors_event_t humidity, temp;     //  define variables of the AHT10 sensor...
double BMP180_Temperature;          //  variable to store the temperature reading from the BMP180 sensor...
double BMP180_Pressure;             //  variable to store the pressure reading from the BMP180 sensor...

int retries = 0;                    //  keep track of WiFi reconnect attempts...
int maxRetries = 3;                 //  define the number of retry attempts...
int totalRetries = 0;               //  store the total number of attempts to reconnect to WiFi...
String boardStatus = "";            //  string to hold reconnect attempts...

int tsResponse = 0;                 //  stores the response received from ThingSpeak server...

int wifiConnected = 7;              //  GREEN LED to indicate WiFi is connected...
int wifiDisconnected = 8;           //  RED LED to indicate WiFi is disconnected...
int wifiRetry = 9;                  //  YELLOW LED to indicate WiFi is in retry attempt state...

long rssi = 0;                      //  used to store WiFi RSSI reading value...

int oneSecondDelay = 1000;          //  variable to delay for 1 second (1000 ms)...
int startUpLoopCount = 3;           //  variable used to hold the start up loop count...

const long fiveMinutes = 300000;    //  delay time between posting to ThingSpeak...
const long tenSeconds = 10000;      //  delay time of 10 seconds (WiFi reconnect time)...
const long fiveSeconds = 5000;      //  delay time of 5 seconds (WiFi pause)...
const long twoSeconds = 5000;       //  delay time of 2 seconds (LCD Display pause)...

int status = WL_IDLE_STATUS;        //  the WiFi radio status...
WiFiClient client;                  //  WiFi client instance...
byte macAddr[6];                    //  variable to store MAC address of MKR1000 device...

bool debugMode = false;             //  variable to use for debug testing...
const double boardVoltage = 3.2;    //  calibration value of DC voltage...
const int ARRAY_SIZE = 10;          //  sets the size of each array...
const int analogA0 = A0;            //  used for pre-read...
const int analogA1 = A1;            //  used to monitor outdoor ambient light...
double outdoorLight = 0.0;          //  used to store calculated outdoor light value...
int outdoorLightArray[ARRAY_SIZE];  //  buffer to store outdoor light readings...
int outdoorLightAverage = 0;        //  used to store average of outdoor light array...
int cnt_outdoor_index = 0;          //  used to track position on arrays for outdoor light...
int outdoorLightTotal = 0;          //  used to store total of outdoor light array...

int delay_time = 2000;              //  variable to delay between readings...
float inHg_constant = 0.0295333727; //  convert pressure read as 1 Pa to inHg...
int blinkDelay = 100;               //  variable to delay between LED on / off state...

/************************************************************************/
/*  Posting Interval to ThingSpeak:                                     */
/*  It is recommended to post your data to ThingSpeak every 5 minutes   */
/*  as this will keep the channel well within the bounds of the free    */
/*  user license.  This will allow you to render 288 data points per    */
/*  24 hour period without the concern of exceeding your data limits    */
/*  under the free license.  12 data points/hr x 24 hours = 288 total   */
/************************************************************************/
/********************************************************************/
/*  In order to prevent the millis() rollover (see the following    */
/*  link https://www.gammon.com.au/millis).  Using unsigned long    */
/*  will help in properly handling the millis() or micros()         */
/*  overflow.                                                       */
/********************************************************************/
//  last connection time...
unsigned long lastConnectionTime = 0;
//  set the posting interval desired here...
unsigned long postingInterval = fiveMinutes;

void setup()
{
  Serial.begin(BAUDRATE);           		    //  start the serial port in case debug...

  pinMode(ledPin, OUTPUT);          		    //  Pin 6 has an LED connected...
  digitalWrite(ledPin, LOW);        		    //  turn the LED off...

  pinMode(analogA1, INPUT);         		    //  ensure analog input A1 is set as an input...

  pinMode(wifiConnected, OUTPUT);         	//  green LED indicating WiFi connected...
  digitalWrite(wifiConnected, LOW);       	//  turn the LED off...
  pinMode(wifiDisconnected, OUTPUT);      	//  red LED indicating WiFi disconnected...
  digitalWrite(wifiDisconnected, LOW);    	//  turn the LED off...
  pinMode(wifiRetry, OUTPUT);             	//  yellow LED indicating WiFi retry connect...
  digitalWrite(wifiRetry, LOW);           	//  turn the LED off...
  
  //  display startup message to the console...
  DisplayStartupMessageToConsole();
  //  delay for specified time...
  delay(delay_time);

  //  display startup visual using status LED's
  //  indicating that device is ready to begin
  //  startup process...
  for(int s=0; s<startUpLoopCount; s++)
  {
    //  set all status LED's on then off...
    BlinkStatusLEDs();
  }

  //  delay program...
  delay(oneSecondDelay);
  
  //  initialize the BMP180 sensor...
  if(bmp.begin())
  {
    Serial.println(F("BMP180 initialized."));
  }
  else
  {
    Serial.println(F("BMP180 failed to initialize."));
    while(true)
    {
      digitalWrite(ledPin, HIGH);   //  turn the LED on...
      delay(blinkDelay);            //  delay...
      digitalWrite(ledPin, LOW);    //  turn the LED off...
      delay(blinkDelay);            //  delay...
    }
  }

	//  initialize the AHT10 sensor...
  if(aht.begin())
  {
    Serial.println("AHT10 initialized.");
    Serial.println();
  }
  else
  {
    Serial.println("AHT10 failed to initialize.");
    while(true)
    {
      digitalWrite(ledPin, HIGH);   //  turn the LED on...
      delay(blinkDelay);            //  delay...
      digitalWrite(ledPin, LOW);    //  turn the LED off...
      delay(blinkDelay);            //  delay...
    }
  }

  /****************************************************************/
  /*  NOTE:  you must comment out the code below if you want this */
  /*         program to run stand-alone (meaning not connected to */
  /*         a computer for serial port monitoring).  if you want */
  /*         to monitor the serial port output, then leave this   */
  /*         code uncommented so data will be sent to the serial  */
  /*         port monitor.                                        */
  /****************************************************************/
  //  wait for the serial port to connect if using PC to monitor COM port...
  /*
  while(!Serial)
  {
    //  if we are not getting a response from the COM port monitor,
    //  then flash the YELLOW LED...
    //BlinkStatusLEDsWaitingOnSerial();
  }
  
  Serial.println(F("Serial port active..."));
  */
  /****************************************************************/
  /*  end serial port connect routine...                          */
  /****************************************************************/
  if(WiFi.status() == WL_NO_SHIELD)
  {
    Serial.println(F("WiFi shield not present..."));

    while(true)
    {
      //  blink the RED status LED indicating an error...
      BlinkStatusLEDsWiFiShieldNotPresent();
    }
  }

  //  attempt to connect to WiFi network...
  while(status != WL_CONNECTED)
  {
    Serial.print(F("Attempting to connect to SSID: "));
    Serial.println(ssid);
    
    //  connect to the wireless router...
    status = WiFi.begin(ssid, pass);

    //  set status LED's - technically trying to connect...
    SetStatusLEDs(false, false, true);
    
    //  wait 10 seconds for WiFi connection...
    delay(tenSeconds);
  }

  //  verify that we are connected to the wireless router,
  //  and if so turn on the GREEN LED..
  if(status == WL_CONNECTED)
  {
    //  show you are successfully connected to the wireless router...
    Serial.print(F("Connected to "));
    Serial.println(ssid);
  
    //  set status LED showing we are connected...
    SetStatusLEDs(true, false, false);

    //  get IP address the MKR1000 is associated with...
    IPAddress ip = WiFi.localIP();
    Serial.print(F("IP Address: "));
    Serial.println(ip);

    //  delay for a time period...
    delay(twoSeconds);
    
    //  get the MAC address of the MKR1000 device...
    WiFi.macAddress(macAddr);
    Serial.print(F("MAC Address: "));
    PrintMacAddress(macAddr);

    //  print out the encryption type used by the wireless router...
    Serial.print(F("Encryption type: "));
    PrintEncryptionType(WiFi.encryptionType());
    
    /**********************************************/
    /*  ThingSpeak client start...                */
    /**********************************************/
    ThingSpeak.begin(client);
  }
  else
  {
    //  show we are not connected to the wireless router...
    Serial.println(F("Not connected to the wireless router..."));

    //  delay for a time period...
    delay(twoSeconds);
  
    //  flash the RED and YELLOW LED's simultaneously
    //  showing we are not connected to the wireless
    //  router...
    while(true)
    {
      //  blink the RED and YELLOW LED's forever...
      BlinkStatusLEDsStartupNotConnected();
    }
  }
}

void loop()
{
  //  get the temperature from the BMP180 sensor...
  BMP180_Temperature = ReadTemperatureFromBMP180Sensor();
  //  get the pressure from the BMP180 sensor...
  BMP180_Pressure = ReadPressureFromBMP180Sensor();
  //  get the humidity from the AHT10 sensor...
  ReadFromAHT10Sensor();
  //  get light sensor reading...
  GetOutdoorLightSensorReading();

  //  check the interval posting time.  if longer than set posting interval time,
  //  we can send the data collected to ThingSpeak...
  if(!client.connected() && ((millis() - lastConnectionTime) > postingInterval))
  {
    //  check the WiFi status on entry...
    status = WiFi.status();

    //  lets make sure that we actually got a valid reading from the sensors...
    if(isnan(humidity.relative_humidity) || isnan(BMP180_Temperature) || isnan(BMP180_Pressure))
    {
      //  if any one sensor reading failed, we will not update ThingSpeak and
      //  we will update the console as to which device failed...
      if(isnan(BMP180_Temperature))
      {
        //  print out error message that temperature reading failed...
        PrintSensorReadingErrorMessage(1);
      }
      else if(isnan(humidity.relative_humidity))
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

      if(!debugMode)
      {
        //  we have valid data so prepare to send the data to ThingSpeak...
        //  take all of our readings from the sensors and pack them for transmission...
        ThingSpeak.setField(1,(float)BMP180_Pressure);                //  BMP180 temperature reading (already converted to proper value)...
        ThingSpeak.setField(2,(float)humidity.relative_humidity);     //  AHT10 humidity reading...
        ThingSpeak.setField(3,(float)BMP180_Pressure);                //  BMP180 pressure reading (already converted to proper value)...
        ThingSpeak.setField(4,(float)rssi);                           //  MKR1000 RSSI reading...
        ThingSpeak.setField(5,(float)outdoorLight);                   //  CdS Photosensor reading...

        //  write the status update to ThingSpeak...
        boardStatus = "Reconnect Attempts:  " + String(totalRetries);
        ThingSpeak.setStatus(boardStatus);
      }

      if(status == WL_CONNECTED) 
      {
        //  set status LED's showing we are connected...
        SetStatusLEDs(true, false, false);

        if(!debugMode)
        {
          //  write the fields that have been set to ThingSpeak...
          tsResponse = ThingSpeak.writeFields(myChannelNumber, thingSpeakKey);

          //  delay for a time period...
          delay(twoSeconds);
        }

        //  look for a 200 response back from the server...
        if(tsResponse == TS_OK_SUCCESS)
        {
          Serial.println(F("(1) Data sent to ThingSpeak..."));
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
        
            Serial.println(F("Successful reconnect to the network..."));

            if(!debugMode)
            {
              //  write the fields that have been set to ThingSpeak...
              tsResponse = ThingSpeak.writeFields(myChannelNumber, thingSpeakKey);

              //  delay for a time period...
              delay(twoSeconds);
            }
              
            //  look for a 200 response back from the server...
            if(tsResponse == TS_OK_SUCCESS)
            {
              Serial.println(F("(2) Data sent to ThingSpeak..."));
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
    }
    
    //  if serial output debug enabled, print the temperature,
    //  pressure, and humidity readings from the sensors to
    //  the console...
    PrintSensorReadingsToConsole();
    //  if serial output debug enabled, print the RSSI value
    //  from the MKR1000 device to the console...
    PrintRSSIReadingToConsole(rssi);
    //  if serial output debug enabled, print the light sensor
    //  reading to the console...
    PrintLightReadingToConsole();
  
    //  set the last ms connection time for the next posting...
    lastConnectionTime = millis();
  }
}

/**
 * BlinkStatusLEDs
 * Method to blink the status LED's
 * in case of start up error detected.
 */
void BlinkStatusLEDs()
{
  TurnAllStatusLightsOn();
  delay(blinkDelay);
  TurnAllStatusLightsOff();
  delay(blinkDelay);
}

/**
 * BlinkStatusLEDsWiFiShieldNotPresent
 * Method to blink the RED status
 * LED in case of WiFi Shield not 
 * present.
 */
void BlinkStatusLEDsWiFiShieldNotPresent()
{
  SetStatusLEDs(false, true, false);
  delay(blinkDelay);
  SetStatusLEDs(false, true, false);
  delay(blinkDelay);
}

/**
 * BlinkStatusLEDsStartupNotConnected
 * Method to blink the RED and YELLOW
 * status LED's in case of no connection
 * to the WiFi router during startup.
 */
void BlinkStatusLEDsStartupNotConnected()
{
  SetStatusLEDs(false, true, true);
  delay(blinkDelay);
  SetStatusLEDs(false, true, true);
  delay(blinkDelay);
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

/**
 * ReadTemperatureFromBMP180Sensor
 * Reads the temperature using the Adafruit library
 * and converts from Deg. C to Deg. F.
 */
double ReadTemperatureFromBMP180Sensor()
{
  //  read the raw degrees C value and convert to degrees F...
  return(ConvertFromCelsuisToFahrenheit(bmp.readTemperature()));
}

/**
 * ReadPressureFromBMP180Sensor
 * Reads the pressure using the Adafruit library
 * and converts from pascal to inHg.
 */
float ReadPressureFromBMP180Sensor()
{
  //  first, take the raw reading from the BMP180 sensor and
  //  divide by 100.0 to convert to millibars...
  double mbValue = (bmp.readPressure()/100.0);
  //  next, take the millibar value and convert to inHg by
  //  multiplying by inHg_constant (0.0295333727)...
  float inHgValue = mbValue * inHg_constant;
  return(inHgValue);
}

/**
 * ReadFromAHT10Sensor
 * Reads the humidity and temperature from the AHT10
 * sensor.
 */
void ReadFromAHT10Sensor()
{
  //  get the humidity and temperature from the AHT10 sensor...
  aht.getEvent(&humidity, &temp);
}

/**
 * GetOutdoorLightSensorReading
 * 
 * Method to read analog value from CdS
 * Photocell
 */
void GetOutdoorLightSensorReading()
{
  //  subtract the last reading...                                     
  outdoorLightTotal = outdoorLightTotal - outdoorLightArray[cnt_outdoor_index];

  //  read the input from analog 1 but
  //  do nothing with the value, then delay
  //  for 10 ms.  this will help in settling
  //  the A/D converter...
  analogRead(analogA1);
  delay(10);
  //  read the analog input for the light sensor...
  outdoorLightArray[cnt_outdoor_index] = analogRead(analogA1);
  delay(20);

  //  add the reading to the total...
  outdoorLightTotal = outdoorLightTotal + outdoorLightArray[cnt_outdoor_index];

  //  advance to the next position in the array...  
  cnt_outdoor_index++;

  //  if we're at the end of the array then
  //  wrap around to the beginning...
  if (cnt_outdoor_index >= ARRAY_SIZE)
  {
    cnt_outdoor_index = 0;
  }

  //  calculate the average...
  outdoorLightAverage = outdoorLightTotal / ARRAY_SIZE;

  //  convert read values to appropriate units...
  outdoorLight = CalculateLightSensor(outdoorLightAverage);
}

/**
 * PrintSensorReadingsToConsole
 * Method used to print out readings from
 * the sensors (BMP180 and AHT10) to the
 * serial port if in DEBUG mode.
 */
void PrintSensorReadingsToConsole()
{
  //  print out BMP180 temperature...
  Serial.print(F("BMP180 Temperature:  "));
  Serial.print(BMP180_Temperature);
  Serial.println(F("°F")); 

  //  print out BMP180 pressure...
  Serial.print(F("BMP180 Pressure:     "));
  Serial.print(BMP180_Pressure);
  Serial.println(F(" inhg"));

  //  print out AHT10 humidity...
  Serial.print(F("AHT10 Humidity:      "));
  Serial.print(humidity.relative_humidity);
  Serial.println(F("%"));
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
 * PrintLightReadingToConsole
 * Method used to print out light sensor reading
 * from the CdS photocell to the serial port if
 * in DEBUG mode.
 */
void PrintLightReadingToConsole()
{
  //  print readings to the console if enabled...
  Serial.print(F("Light Sensor:  "));
  Serial.println(outdoorLight);
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
      Serial.print(F("BMP180 Temperature reading failed..."));
      break;
    case 2:
      Serial.print(F("AHT10 Humidity reading failed..."));
      break;
    case 3:
      Serial.print(F("BMP180 Pressure reading failed..."));
      break;
    default:
      break;
  }
}

/**
 * DisplayStartupMessageToConsole
 * Method used to display the startup
 * message to the console.
 */
void DisplayStartupMessageToConsole()
{
  Serial.println(F("ThingSpeak_MKR-1000_Outdoor_Sensors"));
  Serial.println(F("v. 1.0.1"));
  Serial.println(F("Todd S. Canaday"));
  Serial.println(F("February 26, 2025"));
  Serial.println();
}

/**
 * PrintMacAddress
 * Method used to print the MAC
 * address of the MKR1000 device.
 */
void PrintMacAddress(byte mac[])
{
  for (int i = 5; i >= 0; i--)
  {
    if(mac[i] < 16)
    {
      Serial.print(F("0"));
    }
    
    Serial.print(mac[i], HEX);
    
    if (i > 0)
    {
      Serial.print(F(":"));
    }
  }
  Serial.println();
}

/**
 * PrintEncryptionType
 * Method used to translate the encryption
 * type number retruned from the WiFi Router
 * query.
 */
void PrintEncryptionType(int et)
{
  // read the encryption type and print out the title:
  switch(et)
  {
    case ENC_TYPE_WEP:
      Serial.println(F("WEP"));
      break;
    case ENC_TYPE_TKIP:
      Serial.println(F("WPA"));
      break;
    case ENC_TYPE_CCMP:
      Serial.println(F("WPA2"));
      break;
    case ENC_TYPE_AUTO:
      Serial.println(F("Auto"));
      break;
    case ENC_TYPE_NONE:
      Serial.println(F("None"));
      break;
    default:
      Serial.println(F("Unknown"));
      break;
  }
}

/**
 * CalculateLightSensor
 * 
 * Method used to calculate light sensor
 * value.
 */
double CalculateLightSensor(int v)
{
  //  invert the reading (because the sensor was
  //  wired on the sensor board wrong)...
  return(boardVoltage - ((v / 1024.0) * boardVoltage));
}

/**
 * ConvertFromCelsuisToFahrenheit
 * Method used to convert from degrees C
 * to degrees F.
 */
double ConvertFromCelsuisToFahrenheit(double degC)
{
  //  convert to degrees F...
  return((degC*(9.0/5.0))+32.0);
}
