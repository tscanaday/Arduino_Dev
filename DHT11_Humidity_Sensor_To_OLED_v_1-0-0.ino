/*
  DHT11_Humidity_Sensor_To_OLED_v_1-0-0
  
  Version 1.0.0
  
  This is a simple program that reads from a DHT11 temperature / humidity
  sensor and displays the values read to an OLED display.
  
  Reference Link For Connecting DHT11:
  
  https://lastminuteengineers.com/dht11-module-arduino-tutorial/

  Device:     Address:
  -------     -----------
  I2C LCD     0x27
  OLED        0x3C
  
  DHT11
  Pin:      Connected To:

  1         5 VDC
  2         Pin 7
  3         GND

  Created:   September 4, 2024
  Modified:  September 4, 2024
  by Todd S. Canaday
  tcanaday@memphis.edu

  Arduino Compiler Version:  arduino-1.8.13

  Revision Notes:
  
  09/04/2024  Initial release of source code.
*/

#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define BAUD_RATE 9600              //  default baud rate for the console...
#define DHT_SENSOR_PIN 7            //  value for pin connected to DHT11 sensor...
#define DHT_SENSOR_TYPE DHT11       //  value for sensor type...
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
//  initialize DHT11 sensor...
DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);
//  initialize OLED display...
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/********************/
/*  VARIABLES       */
/********************/
const int ledPin = 13;              //  on board LED...
float temperature = 0.0;            //  variable to store reading from DS18B20 sensor...
float humidity = 0.0;               //  variable to store humidity reading...
int delay_time = 2000;              //  variable to delay between readings...
int blinkDelay = 100;               //  variable to delay between LED on / off state...

void setup()
{
  Serial.begin(BAUD_RATE);          //  start the serial port in case debug...
  
  pinMode(ledPin, OUTPUT);          //  Pin 13 has an LED connected...
  digitalWrite(ledPin, LOW);        //  turn the LED off...

  //  print the program information to the console in debug enabled...
  PrintProgramInfoToConsole();
  
  //  initialize the DHT11 sensor...
  dht_sensor.begin();

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
  //  get the humidity from the DHT11 sensor...
  DisplayHumidityData();
  //  delay between readings from the sensor...
  delay(delay_time);
  //  get the temperature from the DHT11 sensor...
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
  display.println("DHT11 Sensor");
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
  display.println("DHT11 Sensor");
  display.setCursor(0, SENSOR_LBL_CURSOR_POINT+15);
  display.println("Todd S. Canaday");
  display.setCursor(0, SENSOR_LBL_CURSOR_POINT+30);
  display.println("September 4, 2024");
  //  send data to the OLED display...
  display.display();
}

/**
 * DisplayErrorToOLED()
 * Method used to display a generic
 * error message to the OLED display.
 */
void DisplayErrorToOLED()
{
  //  call method to display header data for OLED...
  OLED_Display_Header();
  //  print humidity to screen...
  display.setTextSize(DISPLAY_TEXT_SIZE);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, DATA_CURSOR_POINT);
  display.println("ERROR");
  //  send data to the OLED display...
  display.display();
}

/**
 * DisplayHumidityData()
 * Method used to display the DHT11 Humidity
 * sensor data to the OLED display.
 */
void DisplayHumidityData()
{
  //  get the humidity from the DHT11 sensor...
  humidity = dht_sensor.readHumidity();

  //  make sure that we actually got a valid reading 
  //  from the DHT11 sensor...
  if(isnan(humidity))
  {
    //  ooops, error reading from the sensor...
    DisplayErrorToOLED();
    Serial.println(F("Error reading from DHT11!"));
  }
  else
  {
    //  call method to display header data for OLED...
    OLED_Display_Header();
  
    //  print the sensor type above the value...
    display.setTextSize(HEADER_TEXT_SIZE);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, DATA_CURSOR_POINT-SENSOR_LBL_CURSOR_POINT);
    display.print("Humidity");
  
    //  print temp to screen...
    display.setTextSize(DISPLAY_TEXT_SIZE_SMALL);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, DATA_CURSOR_POINT);
    display.print(humidity);
    display.println("%");
  
    //  send data to the OLED display...
    display.display(); 
  }
}

/**
 * DisplayTemperatureData()
 * Method used to display the DHT11 temperature
 * sensor data to the OLED display.
 */
void DisplayTemperatureData()
{
  //  get the temperature from the DHT11 sensor...
  temperature = dht_sensor.readTemperature(true);

  //  make sure that we actually got a valid reading 
  //  from the DHT11 sensor...
  if(isnan(temperature))
  {
    //  ooops, error reading from the sensor...
    DisplayErrorToOLED();
    Serial.println(F("Error reading from DHT11!"));
  }
  else
  {
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
    display.print(temperature);
    display.println(" F");
  
    //  send data to the OLED display...
    display.display(); 
  }
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
  Serial.println(F("DHT11 Humidity Sensor"));
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
  Serial.print(F("Humidity:  "));
  Serial.print(humidity);
  Serial.println(F("%"));
  Serial.print(F("Temperature:  "));
  Serial.print(temperature);
  Serial.println(F("°F"));
}
