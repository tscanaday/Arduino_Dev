/*
  DS18B20_Temp_Sensor_To_LCD_v_1-0-0
  
  Version 1.0.0
  
  This is a simple program that reads from a DS18B20
  temperature sensor and displays the values read to
  an I2C LCD display.

  Device:     Address:
  -------     -----------
  I2C LCD     0x27
  
  2x16 LCD
  Panel
  Pin:      Connected To:

  1         GND
  2         5 VDC
  3         SDA
  4         SCL

  DS18B20
  Pin:      Connected To:

  1         GND
  2         Pin 9
  3         5 VDC (4.7K across pin 2 and 3)
  
  Created:   September 4, 2024
  Modified:  September 4, 2024
  by Todd S. Canaday
  tcanaday@memphis.edu

  Arduino Compiler Version:  arduino-1.8.13

  Revision Notes:
  
  09/04/2024  Initial release of source code.
*/

#include <OneWire.h>
#include <LiquidCrystal_I2C.h>

#define DS18B20_SENSOR_PIN 9        //  value for pin connected to DS18B20 sensor...
#define DEBUG true                  //  set to true for console output, false for no console output...
#define Serial if(DEBUG)Serial

/********************/
/*  CONSTRUCTORS    */
/********************/
//  DS18B20 connected to pin DS18B20_SENSOR_PIN (requires 4.7K resistor from DATA to VCC)...
OneWire ds(DS18B20_SENSOR_PIN);
//  initialize I2C LCD display (address = 0x27)...
LiquidCrystal_I2C lcd(0x27,16,2);

/********************/
/*  VARIABLES       */
/********************/
const int ledPin = 13;              //  on board LED...
float temperature = 0.0;            //  variable to store reading from DS18B20 sensor...
int delay_time = 2000;              //  variable to delay between readings...
byte addr[8];                       //  address storage for the DS18B20...
byte data[12];                      //  data storage for the DS18B20...
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
}

void loop()
{
  //  get the temperature from the DS18B20 sensor...
  FetchDs18B20TempData();
  //  show the data on the I2C display...
  PrintTempDataToDisplay("Temperature",temperature,"F",true);
  //  delay between readings from the sensor...
  delay(delay_time);
  //  show the DS18B20 device address to the LCD panel...
  PrintDeviceAddress(addr);
  //  delay between readings from the sensor...
  delay(delay_time);
}

/**
 * FetchDs18B20TempData
 * Method used to get data from DS18B20
 * temperature sensor
 */
void FetchDs18B20TempData()
{
  byte i;
  byte present = 0;
  byte type_s;
  //byte data[12];
  float celsius, fahrenheit;
  
  if(!ds.search(addr))
  {
    ds.reset_search();
    delay(250);
    return;
  }
  
  if(OneWire::crc8(addr,7) != addr[7])
  {
    //  had a CRC error...
    return;
  }
  
  switch(addr[0])
  {
    case 0x10:
      type_s = 1;
      break;
    case 0x28:
      type_s = 0;
      break;
    case 0x22:
      type_s = 0;
      break;
    default:
      return;    
  }
  
  ds.reset();
  ds.select(addr);
  ds.write(0x44);
  
  delay(1000);
  
  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE);

  for(i=0; i<9; i++)
  {
    data[i] = ds.read();
  }
  
  int16_t raw = (data[1] << 8) | data[0];
  if(type_s)
  {
    raw = raw << 3;
    
    if(data[7] == 0x10)
    {
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  }
  else
  {
    byte cfg = (data[4] & 0x60);
    
    if(cfg == 0x00)
    {
      raw = raw & ~7;
    }
    else if(cfg = 0x20)
    {
      raw = raw & ~3;
    }
    else if(cfg = 0x40)
    {
      raw = raw & ~1;
    }
  }
  
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.9 + 32.0;
  
  temperature = fahrenheit;
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
  lcd.print("DS18B20 Temp.");
  //  set cursor to character 0 on line 1...
  lcd.setCursor(0,1);
  //  print message to the LCD screen...
  lcd.print("Sensor v. 1.0.1");
}

/**
 * PrintTempDataToDisplay
 * Method used to print sensor value
 * to the I2C LCD display.
 */
void PrintTempDataToDisplay(String device, float value, String unitOfMeasure, bool isTemp)
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
 * PrintDeviceAddress
 * Method used to print the DS18B20 device
 * address to the LCD panel.
 */
void PrintDeviceAddress(byte b[])
{
  //  clear the I2C LCD dsiplay...
  lcd.clear();
  //  set cursor to character 0 on line 0...
  lcd.setCursor(0,0);
  //  print the device type to the LCD screen...
  lcd.print("DS18B20 Address");
  //  set cursor to character 0 on line 1...
  lcd.setCursor(0,1);
  //  print the device address to the LCD panel...
  LCDPrintHexPlain(b[0]);
  lcd.print("-");
  LCDPrintHexPlain(b[6]);
  LCDPrintHexPlain(b[5]);
  LCDPrintHexPlain(b[4]);
  LCDPrintHexPlain(b[3]);
  LCDPrintHexPlain(b[2]);
  LCDPrintHexPlain(b[1]);
}

/**
 * LCDPrintHexPlain
 * Helper method used to print byte values
 * to the LCD panel.
 */
void LCDPrintHexPlain(byte value)
{
  lcd.print(value >> 4 & 0x0f, HEX);
  lcd.print(value >> 0 & 0x0f, HEX);  
}

/**
 * PrintReadingsToConsole
 * Method used to print out readings from
 * the sensors to the serial port if
 * in DEBUG mode.
 */
void PrintReadingsToConsole()
{
  Serial.println(temperature);
}
