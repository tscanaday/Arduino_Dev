/*
  BMP180_Pressure_Sensor_To_LCD_v_1-0-0
  
  Version 1.0.0
  
  This is a simple program that reads from a BMP180 pressure
  sensor and displays the values read to an I2C LCD display.

  Device:     Address:
  -------     -----------
  BMP180      0x77
  I2C LCD     0x27
  
  2x16 LCD
  Panel
  Pin:      Connected To:

  1         GND
  2         5 VDC
  3         SDA
  4         SCL

  BMP180
  Pin:      Connected To:

  1         SDA
  2         SCL
  3         GND
  4         5 VDC
  
  Created:   September 4, 2024
  Modified:  September 4, 2024
  by Todd S. Canaday
  tcanaday@memphis.edu

  Arduino Compiler Version:  arduino-1.8.13

  Revision Notes:
  
  09/04/2024  Initial release of source code.
*/

#include <SFE_BMP180.h>
#include <LiquidCrystal_I2C.h>

#define ALTITUDE 69.0               //  altitude for Germantown TN (in meters)...
#define DEBUG true                  //  set to true for console output, false for no console output...
#define Serial if(DEBUG)Serial

/********************/
/*  CONSTRUCTORS    */
/********************/
//  initialize I2C LCD display (address = 0x27)...
LiquidCrystal_I2C lcd(0x27,16,2);
//  initialize the I2C BMP180 pressure sensor board (address = 0x77)...
SFE_BMP180 prs;

/********************/
/*  VARIABLES       */
/********************/
const int ledPin = 13;              //  on board LED...
float temperature = 0.0;            //  variable to store reading from DS18B20 sensor...
int delay_time = 2000;              //  variable to delay between readings...
float inHg_constant = 0.0295333727; //  convert pressure read as 1 Pa to inHg...
char stat;                          //  variable for SparkFun BMP180 library for status...
double T,P,p0,a;                    //  variables for SparkFun BMP180 library for temp and pressure...
int blinkDelay = 100;               //  variable to delay between LED on / off state...

void setup()
{
  lcd.init();                       //  initialize the I2C LCD...
  lcd.noBacklight();                //  disable backlight for the I2C LCD...
  lcd.clear();                      //  clear the I2C LCD...
  lcd.backlight();                  //  enable backlight for the I2C LCD...
  
  Serial.begin(9600);               //  start the serial port in case debug...
  
  pinMode(ledPin, OUTPUT);          //  Pin 13 has an LED connected...
  digitalWrite(ledPin, LOW);        //  turn the LED off...

  //pinMode(chipSelect, OUTPUT);      //  chip select on SD Card shield...

  //  display a startup message to the I2C LCD display...
  DisplayStartupMessageToLCD();
  //  delay for specified time...
  delay(delay_time);
  
  //  initialize the BMP180 sensor...
  if (prs.begin())
  {
    Serial.println(F("BMP180 initialized"));
  }
  else
  {
    Serial.println(F("BMP180 failed to initialize"));
    while(true)
    {
      digitalWrite(ledPin, HIGH);   //  turn the LED on...
      delay(blinkDelay);            //  delay...
      digitalWrite(ledPin, LOW);    //  turn the LED off...
      delay(blinkDelay);            //  delay...
    }
  }

  //  print program info to console if enabled...
  PrintProgramInfoToConsole();
}

void loop()
{
  //  get the pressure from the BMP180 sensor...
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

  //  show the data on the I2C display...
  PrintDataToDisplay("Pressure",p0,"inHg",false);
  //  delay between readings from the sensor...
  delay(delay_time);

  //  show the data on the I2C display...
  PrintDataToDisplay("Temperature",((T*9/5)+32),"F",true);
  //  delay between readings from the sensor...
  delay(delay_time);

  //  if serial output debug enabled, print values to console...
  PrintReadingsToConsole();
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
  lcd.print("BMP180 Pressure");
  //  set cursor to character 0 on line 1...
  lcd.setCursor(0,1);
  //  print message to the LCD screen...
  lcd.print("Sensor v. 1.0.0");
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
 * PrintProgramInfoToConsole
 * Method used to print out program
 * information to the serial port if
 * in DEBUG mode.
 */
void PrintProgramInfoToConsole()
{
  //  print program info to console...
  Serial.println(F("BMP180 Pressure Sensor"));
  Serial.println();
}

/**
 * PrintReadingsToConsole
 * Method used to print out readings from
 * the sensors to the serial port if
 * in DEBUG mode.
 */
void PrintReadingsToConsole()
{
  //  print readings to the console if enabled...
  Serial.print(F("Pressure:  "));
  Serial.print(p0);
  Serial.println(F(" inHg"));
  Serial.print(F("Temperature:  "));
  Serial.print(((T*9/5)+32));
  Serial.println(F("°F"));
}
