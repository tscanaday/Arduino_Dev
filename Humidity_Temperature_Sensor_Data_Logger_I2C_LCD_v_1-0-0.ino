/*
  Humidity_Temperature_Sensor_Data_Logger_I2C_LCD_v_1-0-0
  
  Version 1.0.0
  
  This is a simple program that reads data from the DHT11 humidity / temperature sensor,
  displays the values read to an I2C LCD display, then writes the data to the SD card
  with a date / time stamp for each reading.  The output data to the SD card will look
  like this:  

    Format:   DATE,TIME,TEMP,HUMIDITY
    Example:  09/30/2024,09:39:41 AM,68.36,55.00

  Reference:  https://lastminuteengineers.com/arduino-micro-sd-card-module-tutorial/

  Device:     Address:
  -------     --------
  I2C LCD     0x27
  RTC Clock   0x68 
  
  NOTE:       there are two addresses on the RTC Clock:
              0x57 = on board EEPROM (not used in this instance)
              0x68 = RTC Clock

  HARDWARE NOTE:
  
  The DS3231 AT24C32 RTC Module purchased from Amazon
  is designed to use a rechargeable CR2032 battery to
  keep the RTC module date time memory intact when powered
  off.  You will need to remove the 200 Ohm resistor next
  to the SCL input pin on the board to disable this recharging
  circuit when using a non-rechargeable CR2032 battery.

  RTC
  Pin:      Connected To:

  1         Arduino Pin A5 (SCL)
  2         Arduino Pin A4 (SDA)
  3         5 VDC
  4         GND

  2x16 LCD
  Panel
  Pin:      Connected To:

  1         GND
  2         5 VDC
  3         Arduino Pin A4 (SDA)
  4         Arduino Pin A5 (SCL)
  
  DHT11
  Pin:      Connected To:

  1         5 VDC
  2         Arduino Pin 7
  3         GND

  SD Card
  Pin:      Connected To:

  CS        Arduino Pin 10
  SCK       Arduino Pin 13
  MOSI      Arduino Pin 11
  MISO      Arduino Pin 12
  VCC       5 VDC
  GND       GND 
  
  Created:   September 30, 2024
  Modified:  September 30, 2024
  by Todd S. Canaday
  tcanaday@memphis.edu

  Arduino Compiler Version:  arduino-1.8.13

  NOTE:   Using SparkFun Source Library
          C:\Users\tcanaday\OneDrive - TK Elevator\Documents\Arduino\libraries\dht11esp8266
          C:\Users\tcanaday\OneDrive - TK Elevator\Documents\Arduino\libraries\SD
   
  Revision Notes:
  
  09/30/2024  Initial release of source code.
*/

#include <OneWire.h>
#include <DHT.h>
#include <SD.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>

#define BAUDRATE 9600               //  default baud rate for console...
#define DHT_SENSOR_PIN 7            //  value for pin connected to DHT11 sensor...
#define DHT_SENSOR_TYPE DHT11       //  value for sensor type...
#define DEBUG false                 //  set to true for console output, false for no console output...
#define Serial if(DEBUG)Serial

/********************/
/*  CONSTRUCTORS    */
/********************/
//  initialize DHT11 sensor...
DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);
//  initialize I2C LCD display (address = 0x27)...
LiquidCrystal_I2C lcd(0x27,16,2);
//  create an object of the DS3231 RTC module using address 0x53...
RTC_DS3231 rtc;

/********************/
/*  VARIABLES       */
/********************/
const int ledPin = 13;              //  on board LED...
float temperature = 0.0;            //  variable to store reading from DHT11 sensor...
float humidity = 0.0;               //  variable to store DHT11 humidity reading...
int delayTime = 2000;              //  variable to delay between readings...
const int chipSelect = 10;          //  chip select for micro SD card module...
Sd2Card card;                       //  create instance for SD Card...
SdVolume volume;                    //  create file volume of SD Card...
SdFile root;                        //  create file root directory of SD Card...
int blinkDelay = 100;               //  variable to delay between LED on / off state...

void setup()
{
  lcd.init();                       //  initialize the I2C LCD...
  lcd.noBacklight();                //  disable backlight for the I2C LCD...
  lcd.clear();                      //  clear the I2C LCD...
  lcd.backlight();                  //  enable backlight for the I2C LCD...
  
  Serial.begin(BAUDRATE);           //  start the serial port in case debug...
  
  pinMode(ledPin, OUTPUT);          //  Pin 13 has an LED connected...
  digitalWrite(ledPin, LOW);        //  turn the LED off...

  pinMode(chipSelect, OUTPUT);      //  chip select on SD Card shield...

  //  display a startup message to the I2C LCD display...
  DisplayStartupMessageToLCD();
  //  delay for specified time...
  delay(delayTime);
  
  //  see if the card is present and can be initialized...
  if(!SD.begin(chipSelect))
  {
    //  if debug, send message to serial port...
    Serial.println(F("SD Card Failed to start."));
    //  display message to I2C LCD display...
    DisplaySDCardErrorMessage();
    
    //  if SD Card fails to start, loop forever
    //  and indicate failure by blinking LED...
    while(true)
    {
      digitalWrite(ledPin, HIGH);   //  turn the LED on...
      delay(blinkDelay);            //  delay...
      digitalWrite(ledPin, LOW);    //  turn the LED off...
      delay(blinkDelay);            //  delay...
    }
  }
  else
  {
    //  if debug, send message to serial port...
    Serial.println(F("SD Card started."));
    //  display message to I2C LCD display...
    DisplaySDCardSuccessfulStart();
    //  delay for specified time...
    delay(delayTime);
  }

  //  initialize the DHT11 sensor...
  dht_sensor.begin();
  
  //  initialize and begin the RTC...
  if(!rtc.begin())
  {
    //  alert the user that the RTC module was not detected...
    Serial.println(F("Could not detect the RTC module!"));
    //  "blink" the onboard LED and loop forever until reset...
    while(true)
    {
      digitalWrite(ledPin, HIGH);   //  turn the LED on...
      delay(blinkDelay);            //  delay...
      digitalWrite(ledPin, LOW);    //  turn the LED off...
      delay(blinkDelay);            //  delay...
    }
  }
  else
  {
    Serial.println(F("RTC has been initialized."));
  }

  //  dont need the 32K pin so disable it...
  rtc.disable32K();

  //  see if the RTC has lost its memory backup power
  //  (i.e. - battery is dying or not installed)...
  if(rtc.lostPower())
  {
    Serial.println(F("Looks like the RTC module has lost backup power!"));
    //  we need the RTC to be set, so if power was lost then set time...
    UpdateRTC();
  }

  //  display the date / time to the LCD display...
  PrintDateTimeToDisplay();
  //  delay for specified time...
  delay(delayTime);
}

void loop()
{
  //  get the humidity from the DHT11 sensor...
  humidity = dht_sensor.readHumidity();
  //  show the data on the I2C display...
  PrintDataToDisplay("Humidity",humidity,"%",false);
  //  delay between readings from the sensor...
  delay(delayTime);

  //  get the temperature from the DHT11 sensor...
  temperature = dht_sensor.readTemperature(true);
  //  show the data on the I2C display...
  PrintDataToDisplay("Temperature",temperature,"F",true);
  //  delay between readings from the sensor...
  delay(delayTime);
  //  show the date time on the I2C display...
  PrintDateTimeToDisplay();
  //  delay between readings from the sensor...
  delay(delayTime);
  
  //  write all data collected in the loop iteration to the SD card...
  WriteDataToSDCard(temperature,humidity);
  //  if serial output debug enabled, print values to console...
  PrintReadingsToConsole();
}

/**
 * WriteDataToSDCard
 * Method used to write sensor data to the
 * SD card module.
 */
void WriteDataToSDCard(float h, float t)
{
  //  open the file on the SD card...
  File dataFile = SD.open("datalog.txt", FILE_WRITE);

  //  if file successfully opened, then begin writing data...
  if(dataFile)
  {
    // get time and date from RTC and save in variables
    DateTime rtcTime = rtc.now();
  
    int ss = rtcTime.second();
    int mm = rtcTime.minute();
    int hh = rtcTime.twelveHour();
    int DD = rtcTime.dayOfTheWeek();
    int dd = rtcTime.day();
    int MM = rtcTime.month();
    int yyyy = rtcTime.year();
    
    //  if month is single digit, add leading zero...
    if(MM < 10)
    {
      dataFile.print(0);
      dataFile.print(MM);
      dataFile.print("/");
    }
    else
    {
      dataFile.print(MM);
      dataFile.print("/");
    }
  
    //  if day is single digit, add leading zero...
    if (dd < 10)
    {
      dataFile.print(0);
      dataFile.print(dd);
      dataFile.print("/");
    }
    else
    {
      dataFile.print(dd);
      dataFile.print("/");
    }
  
    //  print the year...
    dataFile.print(yyyy);
    //  add the comma to the string...
    dataFile.print(",");
    
    //  if month is single digit, add leading zero...
    if (hh < 10)
    {
      dataFile.print(0);
      dataFile.print(hh);
      dataFile.print(":");
    }
    else
    {
      dataFile.print(hh);
      dataFile.print(":");
    }
  
    //  if day is single digit, add leading zero...
    if (mm < 10)
    {
      dataFile.print(0);
      dataFile.print(mm);
      dataFile.print(":");
    }
    else
    {
      dataFile.print(mm);
      dataFile.print(":");
    }
  
    //  if second is single digit, add leading zero...
    if (ss < 10)
    {
      dataFile.print(0);
      dataFile.print(ss);
      dataFile.print(":");
    }
    else
    {
      dataFile.print(ss);
    }
  
    //  write AM / PM indication...
    if (rtcTime.isPM())
    {
      dataFile.print(" PM");
    }
    else
    {
      dataFile.print(" AM");
    }

    //  add the comma to the string...
    dataFile.print(",");
    
    //  print the DHT11 humidity, followed by a comma...
    dataFile.print(h);
    dataFile.print(",");
    //  print the DHT11 temperature but do not add an ending comma,
    //  instead add a CR-LF at the end...
    dataFile.print(t);
    dataFile.println();
    //  close the file...      
    dataFile.close();
  }  
  else
  {
    //  display error message to console if debug...
    Serial.println(F("Failed to write to SD card."));
    //  display error message to I2C LCD...
    DisplaySDCardWriteDataErrorMessage();
  }
}

/**
 * DisplayStartupMessageToLCD
 * Method used to display the startup
 * message to the I2C LCD display.
 */
void DisplayStartupMessageToLCD()
{
  //  clear the I2C LCD dsiplay...
  lcd.clear();
  //  set cursor to character 0 on line 0...
  lcd.setCursor(0,0);
  //  print message to the LCD screen...
  lcd.print("DHT11 Logger");
  //  set cursor to character 0 on line 1...
  lcd.setCursor(0,1);
  //  print message to the LCD screen...
  lcd.print("v. 1.0.0");
}

/**
 * DisplaySDCardErrorMessage
 * Method used to display message
 * to the I2C LCD display if SD Card
 * module fails.
 */
void DisplaySDCardErrorMessage()
{
  //  clear the I2C LCD dsiplay...
  lcd.clear();
  //  set cursor to character 1 on line 0...
  lcd.setCursor(1,0);
  //  print message to the LCD screen...
  lcd.print("SD Card Failed");
  //  set cursor to character 4 on line 1...
  lcd.setCursor(4,1);
  //  print message to the LCD screen...
  lcd.print("to start");
}

/**
 * DisplaySDCardWriteDataErrorMessage
 * Method used to display error message
 * if there is an error writing to the
 * SD Card module.
 */
void DisplaySDCardWriteDataErrorMessage()
{
  //  clear the I2C LCD dsiplay...
  lcd.clear();
  //  set cursor to character 1 on line 0...
  lcd.setCursor(1,0);
  //  print message to the LCD screen...
  lcd.print("Error Writing");
  //  set cursor to character 4 on line 1...
  lcd.setCursor(3,1);
  //  print message to the LCD screen...
  lcd.print("to SD Card");
}

/**
 * DisplaySDCardSuccessfulStart
 * Method used to display successful SD
 * Card module startup.
 */
void DisplaySDCardSuccessfulStart()
{
  //  clear the I2C LCD dsiplay...
  lcd.clear();
  //  set cursor to character 0 on line 0...
  lcd.setCursor(4,0);
  //  print message to the LCD screen...
  lcd.print("SD Card");
  //  set cursor to character 0 on line 1...
  lcd.setCursor(4,1);
  //  print message to the LCD screen...
  lcd.print("Started");
}

/**
 * DisplayReprogramRTCMessage
 * Method used to tell the user to set the
 * RTC clock again.
 */
void DisplayReprogramRTCMessage()
{
  //  clear the I2C LCD dsiplay...
  lcd.clear();
  //  set cursor to character 0 on line 0...
  lcd.setCursor(0,0);
  //  print message to the LCD screen...
  lcd.print("RTC NEEDS TO BE");
  //  set cursor to character 0 on line 1...
  lcd.setCursor(0,1);
  //  print message to the LCD screen...
  lcd.print("REPROGRAMMED!");
}

/**
 * PrintDataToDisplay
 * Method used to print sensor value
 * to the I2C LCD display.
 */
void PrintDataToDisplay(String device, float value, String unitOfMeasure, bool isTemp)
{
  //  clear the I2C LCD dsiplay...
  lcd.clear();
  //  set cursor to character 0 on line 0...
  lcd.setCursor(0,0);
  //  print the device type to the LCD screen...
  lcd.print(device);
  //  set cursor to character 0 on line 1...
  lcd.setCursor(0,1);
  //  print the measured reading to the LCD screen...
  lcd.print(value);
  //  set cursor to character 6 on line 1...
  lcd.setCursor(6,1);
  
  //  if temperature, then add degree symbol...
  if(isTemp)
    lcd.print((char)223);
    
  //  print the measured reading to the LCD screen...
  lcd.print(unitOfMeasure);
}

/**
 * PrintReadingsToConsole
 * Method used to print out readings from
 * the sensors to the serial port if
 * in DEBUG mode.
 */
void PrintReadingsToConsole()
{
  //  get the date and time from the RTC...
  DateTime rtcTime = rtc.now();

  int ss = rtcTime.second();
  int mm = rtcTime.minute();
  int hh = rtcTime.twelveHour();
  int dd = rtcTime.day();
  int MM = rtcTime.month();
  int yyyy = rtcTime.year();
  int dow = rtcTime.dayOfTheWeek();

  //  if month is single digit, add leading zero...
  if (MM < 10)
  {
    Serial.print(F("0"));
    Serial.print(MM);
    Serial.print(F("/"));
  }
  else
  {
    Serial.print(MM);
    Serial.print(F("/"));
  }

  //  if day is single digit, add leading zero...
  if (dd < 10)
  {
    Serial.print(F("0"));
    Serial.print(dd);
    Serial.print(F("/"));
  }
  else
  {
    Serial.print(dd);
    Serial.print(F("/"));
  }

  //  print the year...
  Serial.print(yyyy);

  //  add a comma...
  Serial.print(F(","));

  //  if hour is single digit, add leading zero...
  if (hh < 10)
  {
    Serial.print(F("0"));
    Serial.print(hh);
    Serial.print(F(":"));
  }
  else
  {
    Serial.print(hh);
    Serial.print(F(":"));
  }

  //  if minute is single digit, add leading zero...
  if (mm < 10)
  {
    Serial.print(F("0"));
    Serial.print(mm);
    Serial.print(F(":"));
  }
  else
  {
    Serial.print(mm);
    Serial.print(F(":"));
  }

  //  if second is single digit, add leading zero...
  if (ss < 10)
  {
    Serial.print(F("0"));
    Serial.print(ss);
    Serial.print(F(" "));
  }
  else
  {
    Serial.print(ss);
    Serial.print(F(" "));
  }

  //  print AM / PM indication...
  if (rtcTime.isPM())
  {
    Serial.print(F("PM"));
  }
  else
  {
    Serial.print(F("AM"));
  }

  Serial.print(",");
  
  Serial.print(temperature);
  Serial.print(",");

  Serial.print(humidity);
  Serial.println();
}

/**
 * Method used to set the RTC clock.
 * NOTE:  This method should only be
 * called once.
 */
void UpdateRTC()
{
  //  the following line sets the RTC to the Date and Time this code was compiled...
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

/**
 * PrintDateTimeToDisplay
 * Method used to query the RTC module and
 * get the current Date / Time stamp then
 * print this to the I2C LCD display using
 * the format "MM/DD/YYYY HH:MM:SS (AM/PM)".
 */
void PrintDateTimeToDisplay()
{
  // get time and date from RTC and save in variables
  DateTime rtcTime = rtc.now();

  int ss = rtcTime.second();
  int mm = rtcTime.minute();
  int hh = rtcTime.twelveHour();
  int DD = rtcTime.dayOfTheWeek();
  int dd = rtcTime.day();
  int MM = rtcTime.month();
  int yyyy = rtcTime.year();
  
  //  clear the I2C LCD dsiplay...
  lcd.clear();
  //  set cursor to character 0 on line 0...
  lcd.setCursor(0,0);
  //  print the date to the LCD screen...
  //  if month is single digit, add leading zero...
  if (MM < 10)
  {
    lcd.print("0");
    lcd.print(MM);
    lcd.print("/");
  }
  else
  {
    lcd.print(MM);
    lcd.print("/");
  }

  //  if day is single digit, add leading zero...
  if (dd < 10)
  {
    lcd.print("0");
    lcd.print(dd);
    lcd.print("/");
  }
  else
  {
    lcd.print(dd);
    lcd.print("/");
  }

  //  print the year...
  lcd.print(yyyy);
  
  //  set cursor to character 0 on line 1...
  lcd.setCursor(0,1);
  //  print the time to the LCD screen...
  //  if month is single digit, add leading zero...
  if (hh < 10)
  {
    lcd.print("0");
    lcd.print(hh);
    lcd.print(":");
  }
  else
  {
    lcd.print(hh);
    lcd.print(":");
  }

  //  if day is single digit, add leading zero...
  if (mm < 10)
  {
    lcd.print("0");
    lcd.print(mm);
    lcd.print(":");
  }
  else
  {
    lcd.print(mm);
    lcd.print(":");
  }

  //  if second is single digit, add leading zero...
  if (ss < 10)
  {
    lcd.print("0");
    lcd.print(ss);
    lcd.print(":");
  }
  else
  {
    lcd.print(ss);
  }

  //  write AM / PM indication...
  if (rtcTime.isPM())
  {
    lcd.print(" PM");
  }
  else
  {
    lcd.print(" AM");
  }
}
