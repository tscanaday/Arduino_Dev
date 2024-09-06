/*
  Light_Sensor_CDS_Cell_v_1-0-1

  This program will read the input on Analog Input 1,
  perform the necessary conversions, and then 
  send the calculated results to the console and
  to an OLED display.

  Reference Link For Connecting:

  https://spiceman.net/arduino-cds-program/
  
  Connection Data:

  Device:     Address:
  -------     -----------
  OLED        0x3C
  
  OLED
  Display
  Pin:      Connected To:

  1         GND
  2         5 VDC
  3         SCL
  4         SDA
  
  cDs Photocells:
  
  Connect the CdS photocell to Vcc through a 10K pull-up
  resistor with the other lead to ground.  Connect A1 to
  the junction of the CdS photocell and the 10K resistor.

            CdS
            Cell        10K
           ____
  +5V ----|____|-------\/\/\/\-----|
                   |               |
                   |               |
                   |            -------  GND
                To A1             ---
                                   -

  Created:   June 18, 2024
  Modified:  September 6, 2024
  by Todd S. Canaday
  tcanaday@memphis.edu

  Arduino Compiler Version:  arduino-1.8.13

  Revision Notes:
  
  06/18/2024  Initial release of source code.
  09/06/2024  Added OLED display.
*/

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define BAUD_RATE 9600              //  default baud rate for the console...
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

/********************/
/*  VARIABLES       */
/********************/
const int ledPin = 13;                    //  on board LED...
const int analogInPin1 = A0;              //  used for pre-read...
const int lightSensorInput = A1;          //  used to monitor light through sensor...
const int ARRAY_SIZE = 10;                //  sets the size of the storage array...
int lightSensorArray[ARRAY_SIZE];         //  buffer to store light sensor readings...
int cnt_sensor_index = 0;                 //  used to track position on arrays for light sensor...
int sensorLightTotal = 0;                 //  used to store total of light sensor array...
int sensorLightAverage = 0;               //  used to store average of light sensor array...
double sensorLight = 0.0;                 //  used to store calculated light sensor value...
long lastDisplayTime = 0;                 //  used to keep track of last display of light sensor reading... 
const double boardVoltage = 5.0;          //  calibration value of DC voltage...
const long tenSeconds = 10000;            //  defined time span of 10 seconds (10,000 ms)...
const long fiveSeconds = 5000;            //  defined time span of 5 seconds (5,000 ms)...
const long postingInterval = tenSeconds;  //  sets the posting interval...
int blinkDelay = 100;                     //  variable to delay between LED on / off state...
int delay_time = 2000;                    //  variable to delay between readings...
int startUpCnt = 0;                       //  used to track countdown value... 
int countdown = 5;                        //  used to hold value for countdown to settle ADC circuit...

void setup()
{
  Serial.begin(BAUD_RATE);          //  start the serial port in case debug...
  
  pinMode(ledPin, OUTPUT);          //  Pin 13 has an LED connected...
  digitalWrite(ledPin, LOW);        //  turn the LED off...

  //  print the program information to the console in debug enabled...
  PrintProgramInfoToConsole();

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

  //  display startup message to the display...
  Startup_Message();

  //  delay for delay_time...
  delay(delay_time);
}

void loop()
{
  //  constantly read the light sensor input...
  GetLightSensorReading();

  //  check to see if it is time to post data to the console...
  if((millis() - lastDisplayTime) > postingInterval)
  {
    //  display the light sensor voltage to the console...
    DisplayLightSensorReading(sensorLight);
    //  display the light sensor percentage to the console...
    DisplayLightSensorPercentage(sensorLight);

    //  display the lumens percentage to the OLED display...
    DisplayLumensReadingToDisplay();

    //  capture the last posting time...
    lastDisplayTime = millis();
  }
  else
  {
    while(countdown >= 1)
    {
      //  do not display data until we have gathered enough
      //  readings to fill the arrays.  this is needed to 
      //  settle the ADC circuit...
      countdown = 10 - startUpCnt;
      Countdown_Message(countdown);
      startUpCnt++;
    }
  }
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
  display.println("Light Sensor");
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
  display.println("Light Sensor");
  display.setCursor(0, SENSOR_LBL_CURSOR_POINT+15);
  display.println("Todd S. Canaday");
  display.setCursor(0, SENSOR_LBL_CURSOR_POINT+30);
  display.println("September 6, 2024");
  //  send data to the OLED display...
  display.display();
}

/**
 * Countdown_Message()
 * Method used to display the startup
 * message to the OLED display.
 */
void Countdown_Message(int cntVal)
{
  //  call method to display header data for OLED...
  OLED_Display_Header();
  //  print humidity to screen...
  //display.setTextSize(HEADER_TEXT_SIZE);
  display.setTextSize(DISPLAY_TEXT_SIZE);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(50, SENSOR_LBL_CURSOR_POINT+10);
  display.println(cntVal);
  //  send data to the OLED display...
  display.display();
  //  estimated time to delay for countdown while ADC settles...
  delay(985);
}

/**
 * GetLightSensorReading()
 * Method used to read photocell value.  This method
 * employs an array to average the values read and
 * normalize the analog data received.
 */
void GetLightSensorReading()
{
  //  subtract the last reading...                                     
  sensorLightTotal = sensorLightTotal - lightSensorArray[cnt_sensor_index];

  //  read the input from analog 1 but
  //  do nothing with the value, then delay
  //  for 10 ms.  this will help in settling
  //  the A/D converter...
  analogRead(analogInPin1);
  //  delay for 10 ms...
  delay(10);
  //  read the analog input for the light sensor...
  lightSensorArray[cnt_sensor_index] = analogRead(lightSensorInput);
  //  delay for 20 ms...
  delay(20);

  //  add the reading to the total...
  sensorLightTotal = sensorLightTotal + lightSensorArray[cnt_sensor_index];

  //  advance to the next position in the array...  
  cnt_sensor_index++;

  //  if we're at the end of the array then
  //  wrap around to the beginning...
  if (cnt_sensor_index >= ARRAY_SIZE)
  {
    cnt_sensor_index = 0;
  }

  //  calculate the average...
  sensorLightAverage = sensorLightTotal / ARRAY_SIZE;

  //  convert read values to appropriate units...
  sensorLight = CalculateLightSensor(sensorLightAverage);
}

/**
 * double CalculateLightSensor(int v)
 * Method used to return the reading from the input pin
 * connected to the photocell device.  Returns the value
 * as a double.
 */
double CalculateLightSensor(int v)
{
  //  this returns as a voltage read from the A/D converter...
  return((v / 1024.0) * boardVoltage);
}

/**
 * double CalculatePercentageLight(double r)
 * Method used to calculate the percentage of light
 * detected.  Returns the percentage as a double.
 */
double CalculatePercentageLight(double r)
{
  double pct = 100.0-((r/boardVoltage)*100.0);
  return(pct);
}

/**
 * DisplayVoltageReadingToDisplay
 * Method to display the averave voltage
 * reading from the CdS photocell.
 */
void DisplayVoltageReadingToDisplay()
{
  //  call method to display header data for OLED...
  OLED_Display_Header();

  //  print the sensor type above the value...
  display.setTextSize(HEADER_TEXT_SIZE);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, DATA_CURSOR_POINT-SENSOR_LBL_CURSOR_POINT);
  display.print("Sensor Voltage");

  //  print temp to screen...
  display.setTextSize(DISPLAY_TEXT_SIZE_SMALL);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, DATA_CURSOR_POINT);
  display.print(sensorLight);
  display.println(" VDC");

  //  send data to the OLED display...
  display.display(); 
}

/**
 * DisplayVoltageReadingToDisplay
 * Method to display the lumens reading from
 * the CdS photocell as a percent.
 */
void DisplayLumensReadingToDisplay()
{
  double lumPct = CalculatePercentageLight(sensorLight);
  
  //  call method to display header data for OLED...
  OLED_Display_Header();

  //  print the sensor type above the value...
  display.setTextSize(HEADER_TEXT_SIZE);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, DATA_CURSOR_POINT-SENSOR_LBL_CURSOR_POINT);
  display.print("Lumens");

  //  print temp to screen...
  display.setTextSize(DISPLAY_TEXT_SIZE_SMALL);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, DATA_CURSOR_POINT);
  display.print(lumPct);
  display.println("%");

  //  send data to the OLED display...
  display.display(); 
}

/**
 * PrintProgramInfoToConsole()
 * Method used to display program info to the
 * serial console.
 */
void PrintProgramInfoToConsole()
{
  Serial.println(F("Light_Sensor_CDS_Cell_v_1-0-1"));
  Serial.println(F("Todd S. Canaday"));
  Serial.println(F("tcanaday@memphis.edu"));
  Serial.println(F("September 6, 2024"));
  Serial.println();
}

/**
 * DisplayLightSensorReading(double value)
 * Displays the photocell sensor reading
 * to the console if enabled.
 */
void DisplayLightSensorReading(double value)
{
  Serial.print(F("Light Sensor V:  "));
  Serial.print(value);
  Serial.println(F(" VDC "));
}

/**
 * DisplayLightSensorPercentage(double value)
 * Displays the photocell sensor reading
 * percentage to the console if enabled.
 */
void DisplayLightSensorPercentage(double value)
{
  Serial.print(F("Lumen Measured:    "));
  Serial.print(CalculatePercentageLight(value));
  Serial.println(F("%"));
}
