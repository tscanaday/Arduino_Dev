/*
  BMP180_Pressure_Sensor_To_OLED_v_1-0-0
  
  Version 1.0.0
  
  This is a simple program that reads from a BMP180 pressure
  sensor and displays the values read to an OLED display.
  
  Reference Link For Connecting BMP180:
  
  https://lastminuteengineers.com/bmp180-arduino-tutorial/

  Device:     Address:
  -------     -----------
  BMP180      0x77
  OLED        0x3C
  
  OLED
  Display
  Pin:      Connected To:

  1         GND
  2         5 VDC
  3         SCL
  4         SDA

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
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define BAUD_RATE 9600              //  default baud rate for the console...
#define ALTITUDE 69.0               //  altitude for Germantown TN (in meters)...
#define SCREEN_WIDTH 128            //  OLED display width, in pixels...
#define SCREEN_HEIGHT 64            //  OLED display height, in pixels...
#define DISPLAY_TEXT_SIZE 3         //  value for the display text size...
#define DISPLAY_TEXT_SIZE_SMALL 2   //  value for the display text size...
#define HEADER_TEXT_SIZE  1         //  value for the header text size in yellow area...
#define DATA_CURSOR_POINT 40        //  value for default cursor point on display...
#define SENSOR_LBL_CURSOR_POINT 20  //  value for moving sensor label above data...
#define DEBUG true                  //  set to true for console output, false for no console output...
#define Serial if(DEBUG)Serial

/********************/
/*  CONSTRUCTORS    */
/********************/
//  initialize OLED display...
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
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
  Serial.begin(BAUD_RATE);          //  start the serial port in case debug...
  
  pinMode(ledPin, OUTPUT);          //  Pin 13 has an LED connected...
  digitalWrite(ledPin, LOW);        //  turn the LED off...

  //  print the program information to the console in debug enabled...
  PrintProgramInfoToConsole();
  
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

  //  startup OLED Display...
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    //  if display fails to start, loop forever
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
    Serial.println(F("OLED display started."));
  }

  //  begin the startup message on the OLED display...
  Startup_Message();
  //  delay for specified time...
  delay(2000);
}

void loop()
{
  //  show the pressure data on the OLED display...
  DisplayPressureData();
  //  delay between readings from the sensor...
  delay(delay_time);
  //  show the temperature data on the OLED display...
  DisplayTemperatureData();
  //  delay between readings from the sensor...
  delay(delay_time);

  //  if serial output debug enabled, print values to console...
  PrintReadingsToConsole();
}

/**
 * OLED_Display_Header()
 * Method used to display top line (yellow
 * area) message to the OLED display.
 */
void OLED_Display_Header()
{
  //  clear the OLED display...
  display.clearDisplay();
  //  set the text size and text color...
  display.setTextSize(HEADER_TEXT_SIZE);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);  //  yellow area only...
  // Display static text at the header (yellow area)...
  display.println("BMP180 Sensor");
  //  send data to the OLED display...
  display.display();
}

/**
 * Startup_Message()
 * Method used to display the startup
 * message to the OLED display.
 */
void Startup_Message()
{
  //  call method to display header data for OLED...
  OLED_Display_Header();
  //  print humidity to screen...
  display.setTextSize(HEADER_TEXT_SIZE);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, SENSOR_LBL_CURSOR_POINT);
  display.println("BMP180 Sensor");
  display.setCursor(0, SENSOR_LBL_CURSOR_POINT+15);
  display.println("Todd S. Canaday");
  display.setCursor(0, SENSOR_LBL_CURSOR_POINT+30);
  display.println("September 4, 2024");
  //  send data to the OLED display...
  display.display();
}

/**
 * DisplayPressureData()
 * Method used to display the BMP180 Pressure
 * sensor data to the OLED display.
 */
void DisplayPressureData()
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

  //  call method to display header data for OLED...
  OLED_Display_Header();

  //  print the sensor type above the value...
  display.setTextSize(HEADER_TEXT_SIZE);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, DATA_CURSOR_POINT-SENSOR_LBL_CURSOR_POINT);
  display.print("Pressure");

  //  print temp to screen...
  display.setTextSize(DISPLAY_TEXT_SIZE_SMALL);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, DATA_CURSOR_POINT);
  display.print(p0);
  display.println(" inHg");

  //  send data to the OLED display...
  display.display();  
}

/**
 * DisplayTemperatureData()
 * Method used to display the BMP180 Pressure
 * sensor temperature data to the OLED display.
 */
void DisplayTemperatureData()
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

  //  call method to display header data for OLED...
  OLED_Display_Header();

  //  print the sensor type above the value...
  display.setTextSize(HEADER_TEXT_SIZE);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, DATA_CURSOR_POINT-SENSOR_LBL_CURSOR_POINT);
  display.print("Temperature");

  //  print temp to screen...
  display.setTextSize(DISPLAY_TEXT_SIZE_SMALL);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, DATA_CURSOR_POINT);
  display.print(((T*9/5)+32));
  display.println(" F");

  //  send data to the OLED display...
  display.display();  
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
 * the sensor to the serial port if
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
