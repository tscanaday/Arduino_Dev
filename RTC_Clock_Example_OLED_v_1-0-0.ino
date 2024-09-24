/*
  RTC_Clock_Example_v_1-0-0
  
  Version 1.0.0
  
  This is a simple program that reads the time from a connected RTC
  module using the DS3231 IC and displays the information to
  an OLED display.

  HARDWARE NOTE:
  
  The DS3231 AT24C32 RTC Module purchased from Amazon
  is designed to use a rechargeable CR2032 battery to
  keep the RTC module date time memory intact when powered
  off.  You will need to remove the 200 Ohm resistor next
  to the SCL input pin on the board to disable this recharging
  circuit when using a non-rechargeable CR2032 battery.
  
  RTC
  Pin:      Connected To:

  1         SCL
  2         SDA
  3         5 VDC
  4         GND

  OLED Display
  Pin:      Connected To:

  1         GND
  2         5 VDC
  3         SCL
  4         SDA
  
  Created:   September 24, 2024
  Modified:  September 24, 2024
  by Todd S. Canaday
  tcanaday@memphis.edu

  Arduino Compiler Version:  arduino-1.8.13

  Revision Notes:
  
  09/24/2024  Initial release of source code.
*/

#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define DEBUG false                 //  set to true for console output, false for no console output...
#define Serial if(DEBUG)Serial
#define BAUDRATE 9600               //  default baud rate for console...

#define SCREEN_WIDTH 128            //  OLED display width, in pixels...
#define SCREEN_HEIGHT 64            //  OLED display height, in pixels...

#define DISPLAY_TEXT_SIZE 3         //  value for the display text size...
#define DISPLAY_TEXT_SIZE_SMALL 2   //  value for the display text size...
#define HEADER_TEXT_SIZE  1         //  value for the header text size in yellow area...

#define DATA_CURSOR_POINT 40        //  value for default cursor point on display...
#define SENSOR_LBL_CURSOR_POINT 20  //  value for moving sensor label above data...

/********************/
/*  CONSTRUCTORS    */
/********************/
//  create an object of the DS3231 RTC module using address 0x53...
RTC_DS3231 rtc;
//  initialize the OLED display (address = 0x3C)...
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/********************/
/*  VARIABLES       */
/********************/
const int ledPin = 13;              //  on board LED...
int delayTime = 1000;               //  variable to delay between clock readings...
int blinkDelay = 100;               //  variable to delay between LED on / off state...
long rtcReadTime = 0;               //  used to store display time for RTC clock...
//  array to define day names...
const char weekDays[7][10] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
//  array to define month names...
const char monthNames[12][10] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

void setup()
{
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
  
  Serial.begin(BAUDRATE);           //  start the serial port in case debug...
  
  pinMode(ledPin, OUTPUT);          //  Pin 13 has an LED connected...
  digitalWrite(ledPin, LOW);        //  turn the LED off...

  //  display a startup message to the OLED display...
  Startup_Message();
  //  delay for specified time...
  delay(2000);
  
  //  initialize and begin the RTC...
  if(!rtc.begin())
  {
    //  alert the user that the RTC module was not detected...
    Serial.println(F("Could not detect the RTC module!"));
    //  show error message on OLED display...
    DisplayErrorMessage();
    
    //  "blink" the onboard LED and loop forever until reset...
    while(true)
    {
      digitalWrite(ledPin, HIGH);   //  turn the LED on...
      delay(blinkDelay);            //  delay...
      digitalWrite(ledPin, LOW);    //  turn the LED off...
      delay(blinkDelay);            //  delay...
    }
  }

  //  dont need the 32K pin so disable it...
  rtc.disable32K();

  //  see if the RTC has lost its memory backup power
  //  (i.e. - battery is dying or not installed)...
  if(rtc.lostPower())
  {
    Serial.println(F("Looks like the RTC module has lost backup power!"));
    UpdateRTC();                      //  update the RTC with the current date and time...
  }
}

void loop()
{
  //  this allows the program to run within the loop on a "timed" basis
  //  so there is a delay of 'delayTime' which slows down the loop...
  if((millis() - rtcReadTime) > delayTime)
  {
    //  show the data on the OLED display...
    PrintDateTimeToOLEDDisplay();
    //  get and display the date and time from the RTC to the serial console...
    DisplayDateTimeToConsole();
    //  set current clock cycles for next pass...
    rtcReadTime = millis();
  }
}

/**
 * Method used to set the RTC clock.
 * NOTE:  This method should only be
 * called once.
 */
void UpdateRTC()
{
  // following line sets the RTC to the Date and Time this sketch was compiled...
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
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
  //  print startup to screen...
  display.setTextSize(HEADER_TEXT_SIZE);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, SENSOR_LBL_CURSOR_POINT);
  display.println("RTC Clock");
  display.setCursor(0, SENSOR_LBL_CURSOR_POINT+15);
  display.println("Todd S. Canaday");
  display.setCursor(0, SENSOR_LBL_CURSOR_POINT+30);
  display.println("September 24, 2024");
  //  send data to the OLED display...
  display.display();
}

/**
 * DisplayErrorMessage()
 * Method used to display an error
 * message to the OLED display.
 */
void DisplayErrorMessage()
{
  //  call method to display header data for OLED...
  OLED_Display_Header();
  //  print error message to screen...
  display.setTextSize(DISPLAY_TEXT_SIZE-2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, DATA_CURSOR_POINT);
  display.println("RTC Not Found!");
  //  send data to the OLED display...
  display.display();
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
  display.println("RTC Clock Example");
  //  send data to the OLED display...
  display.display();
}

/**
 * PrintDateTimeToDisplay
 * Method used to query the RTC module and
 * get the current Date / Time stamp then
 * print this to the OLED display using
 * the format "MM/DD/YYYY HH:MM:SS (AM/PM)".
 */
void PrintDateTimeToOLEDDisplay()
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
  int dow = rtcTime.dayOfTheWeek();

  //  call method to display header data for OLED...
  OLED_Display_Header();

  //  get ready to display the day of the week...
  display.setTextSize(DISPLAY_TEXT_SIZE-2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, DATA_CURSOR_POINT-20);

  //  first, print out the day of the week and the current date on the same line...
  display.print(weekDays[dow]);
  display.print(" ");

  //  print the month...
  if(MM < 10)
  {
    display.print("0");
    display.print(MM);
    display.print("/");
  }
  else
  {
    display.print(MM);
    display.print("/");
  }

  //  print the day...
  if(dd < 10)
  {
    display.print("0");
    display.print(dd);
    display.print("/");
  }
  else
  {
    display.print(dd);
    display.print("/");
  }

  //  print the year...
  display.print(yyyy);

  //  get ready to display the time...
  display.setTextSize(DISPLAY_TEXT_SIZE-2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, DATA_CURSOR_POINT);

  //  last, print out the current time from the RTC...

  //  print the hour...
  if(hh < 10)
  {
    display.print("0");
    display.print(hh);
    display.print(":");
  }
  else
  {
    display.print(hh);
    display.print(":");
  }

  //  print the minute...
  if(mm < 10)
  {
    display.print("0");
    display.print(mm);
    display.print(":");
  }
  else
  {
    display.print(mm);
    display.print(":");
  }

  //  print the second...
  if(ss < 10)
  {
    display.print("0");
    display.print(ss);
  }
  else
  {
    display.print(ss);
  }

  //  print AM / PM indication...
  if (rtcTime.isPM())
  {
    display.print(" PM");
  }
  else
  {
    display.print(" AM");
  }
  
  //  display the data...
  display.display();
}

/**
 * Method used to query the RTC module and
 * get the current Date / Time stamp then
 * print this to console using the format
 * "MM/DD/YYYY HH:MM:SS (AM/PM)".
 */
void DisplayDateTimeToConsole()
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

  //  first, print out the day of the week...
  Serial.print(weekDays[dow]);
  //  add a space...
  Serial.print(F(" "));
  
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

  //  add a space...
  Serial.print(F(" "));

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
    Serial.println(F("PM"));
  }
  else
  {
    Serial.println(F("AM"));
  }

  //  flush the serial port...
  Serial.flush();
}
