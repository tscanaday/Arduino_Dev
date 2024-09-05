/*
  DHT11_Humidity_Sensor_To_LCD_v_1-0-0
  
  Version 1.0.0
  
  This is a simple program that reads from a DHT11 temperature / humidity
  sensor and displays the values read to an I2C LCD display.
  
  Reference Link For Connecting DHT11:
  
  https://lastminuteengineers.com/dht11-module-arduino-tutorial/

  Device:     Address:
  -------     -----------
  I2C LCD     0x27
  
  DHT11
  Pin:      Connected To:

  1         5 VDC
  2         Pin 7
  3         GND

  2x16 LCD
  Panel
  Pin:      Connected To:

  1         GND
  2         5 VDC
  3         SDA
  4         SCL
  
  Created:   September 4, 2024
  Modified:  September 4, 2024
  by Todd S. Canaday
  tcanaday@memphis.edu

  Arduino Compiler Version:  arduino-1.8.13

  Revision Notes:
  
  09/04/2024  Initial release of source code.
*/

#include <DHT.h>
#include <LiquidCrystal_I2C.h>

#define DHT_SENSOR_PIN 7            //  value for pin connected to DHT11 sensor...
#define DHT_SENSOR_TYPE DHT11       //  value for sensor type...
#define DEBUG true                  //  set to true for console output, false for no console output...
#define Serial if(DEBUG)Serial

/********************/
/*  CONSTRUCTORS    */
/********************/
//  initialize DHT11 sensor...
DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);
//  initialize I2C LCD display (address = 0x27)...
LiquidCrystal_I2C lcd(0x27,16,2);

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
  lcd.init();                       //  initialize the I2C LCD...
  lcd.noBacklight();                //  disable backlight for the I2C LCD...
  lcd.clear();                      //  clear the I2C LCD...
  lcd.backlight();                  //  enable backlight for the I2C LCD...
  
  Serial.begin(9600);               //  start the serial port in case debug...
  
  pinMode(ledPin, OUTPUT);          //  Pin 13 has an LED connected...
  digitalWrite(ledPin, LOW);        //  turn the LED off...

  //  display a startup message to the I2C LCD display...
  DisplayStartupMessageToLCD();
  //  delay for specified time...
  delay(delay_time);
  
  //  initialize the DHT11 sensor...
  dht_sensor.begin();

  //  print program info to console if enabled...
  PrintProgramInfoToConsole();
}

void loop()
{
  //  get the humidity from the DHT11 sensor...
  humidity = dht_sensor.readHumidity();
  //  get the temperature from the DHT11 sensor...
  temperature = dht_sensor.readTemperature(true);

  //  make sure that we actually got a valid reading 
  //  from the DHT11 sensor...
  if(isnan(temperature) || isnan(humidity))
  {
    //  ooops, error reading from the sensor...
    DisplayErrorToLCD();
    Serial.println(F("Error reading from DHT11!"));
  }
  else
  {
    //  show the data on the I2C display...
    PrintDataToDisplay("Humidity",humidity,"%",false);
    //  delay between readings from the sensor...
    delay(delay_time);
    //  show the data on the I2C display...
    PrintDataToDisplay("Temperature",temperature,"F",true);
    //  delay between readings from the sensor...
    delay(delay_time);
  }
  
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
  lcd.print("DHT11 Humidity");
  //  set cursor to character 0 on line 1...
  lcd.setCursor(0,1);
  //  print message to the LCD screen...
  lcd.print("Sensor v. 1.0.1");
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
 * DisplayErrorToLCD
 * Method used to indicate an error
 * occurred.
 */
void DisplayErrorToLCD()
{
  //  clear the I2C LCD dsiplay...
  lcd.clear();
  //  set cursor to character 0 on line 0...
  lcd.setCursor(0,0);
  //  print the device type to the LCD screen...
  lcd.print("Error reading");
  //  set cursor to character 0 on line 1...
  lcd.setCursor(0,1);
  //  print the measured reading to the LCD screen...
  lcd.print("from DHT11!");
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
