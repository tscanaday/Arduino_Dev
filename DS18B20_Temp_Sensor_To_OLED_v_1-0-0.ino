/*
  DS18B20_Temp_Sensor_To_OLED_v_1-0-0
  
  Version 1.0.0
  
  This is a simple program that reads from a DS18B20
  temperature sensor and displays the values read to
  an OLED display.
  
  Reference Link For Connecting DS18B20:
  
  https://lastminuteengineers.com/ds18b20-arduino-tutorial/

  Device:     Address:
  -------     -----------
  OLED        0x3C
  
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
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define BAUD_RATE 9600              //  default baud rate for the console...
#define DS18B20_SENSOR_PIN 9        //  value for pin connected to DS18B20 sensor...
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
//  DS18B20 connected to pin DS18B20_SENSOR_PIN (requires 4.7K resistor from DATA to VCC)...
OneWire ds(DS18B20_SENSOR_PIN);
//  initialize OLED display...
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

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
  Serial.begin(BAUD_RATE);          //  start the serial port in case debug...
  
  pinMode(ledPin, OUTPUT);          //  Pin 13 has an LED connected...
  digitalWrite(ledPin, LOW);        //  turn the LED off...

  //  startup OLED Display...
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    //  if display fails to start, loop forever
    //  and indicate failure by blinking onboard
    //  LED...
    while(true)
    {
      digitalWrite(ledPin, HIGH);   //  turn the LED on...
      delay(100);                   //  delay...
      digitalWrite(ledPin, LOW);    //  turn the LED off...
      delay(100);                   //  delay...
    }
  }
  
  //  print the program information to the console in debug enabled...
  PrintProgramInfoToConsole();
  
  //  begin the startup message on the OLED display...
  Startup_Message();
  //  delay for specified time...
  delay(2000);
}

void loop()
{
  //  show the temperature data on the OLED display...
  DisplayTemperatureData();
  //  delay between readings from the sensor...
  delay(delay_time);
  //  show the DS18B20 device address to the OLED display...
  PrintDeviceAddress(addr);
  //  delay between readings from the sensor...
  delay(delay_time);
  //  print the data to the console if enabled...
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
  display.println("DS18B20 Sensor");
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
  display.println("DS18B20 Sensor");
  display.setCursor(0, SENSOR_LBL_CURSOR_POINT+15);
  display.println("Todd S. Canaday");
  display.setCursor(0, SENSOR_LBL_CURSOR_POINT+30);
  display.println("September 4, 2024");
  //  send data to the OLED display...
  display.display();
}

/**
 * DisplayTemperatureData()
 * Method used to display the DS18B11 Temperature
 * sensor data to the OLED display.
 */
void DisplayTemperatureData()
{
  //  get the temperature from the DS18B20 sensor...
  FetchDs18B20TempData();

  //  call method to display header data for OLED...
  OLED_Display_Header();

  //  print the sensor type above the value...
  display.setTextSize(HEADER_TEXT_SIZE);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, DATA_CURSOR_POINT-SENSOR_LBL_CURSOR_POINT);
  display.print("Temperature");

  //  if the temperature received is going to be 3 digits plus two decimal
  //  digits, then reduce the size of the font to avoid characters spilling
  //  over and disappearing...
  if(temperature > 99)
  {
    //  print temp to screen...
    display.setTextSize(DISPLAY_TEXT_SIZE_SMALL);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, DATA_CURSOR_POINT);
    display.print(temperature);
    display.println(" F");
  }
  else
  {
    //  print temp to screen...
    display.setTextSize(DISPLAY_TEXT_SIZE);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, DATA_CURSOR_POINT);
    display.print(temperature);
    display.println(" F");
  }

  //  send data to the OLED display...
  display.display();
}

/**
 * PrintDeviceAddress
 * Method used to print the DS18B20 device
 * address to the OLED display.
 */
void PrintDeviceAddress(byte b[])
{
  //  call method to display header data for OLED...
  OLED_Display_Header();

  //  print the sensor type above the value...
  display.setTextSize(HEADER_TEXT_SIZE);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, DATA_CURSOR_POINT-SENSOR_LBL_CURSOR_POINT);
  display.print("Address");

  //  print address to screen...
  display.setTextSize(DISPLAY_TEXT_SIZE_SMALL-1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, DATA_CURSOR_POINT);
  display.print(b[0],HEX);
  display.print("-");
  display.print(b[6],HEX);
  display.print(b[5],HEX);
  display.print(b[4],HEX);
  display.print(b[3],HEX);
  display.print(b[2],HEX);
  display.println(b[1],HEX);

  //  send data to the OLED display...
  display.display();
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
 * PrintProgramInfoToConsole
 * Method used to print out program
 * information to the serial port if
 * in DEBUG mode.
 */
void PrintProgramInfoToConsole()
{
  //  print program info to console...
  Serial.println(F("DS18B20 Temperature Sensor"));
  Serial.println();
}

/**
 * PrintDeviceAddressToConsole()
 * Method used to dislpay the address of the
 * DS18B20 device to the console.
 */
void PrintDeviceAddressToConsole()
{
  Serial.print(addr[0],HEX);
  Serial.print("-");
  Serial.print(addr[6],HEX);
  Serial.print(addr[5],HEX);
  Serial.print(addr[4],HEX);
  Serial.print(addr[3],HEX);
  Serial.print(addr[2],HEX);
  Serial.println(addr[1],HEX);
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
  Serial.print(F("Temperature:  "));
  Serial.print(temperature);
  Serial.println(F("°F"));
  Serial.print(F("Device Address:  "));
  PrintDeviceAddressToConsole();
}
