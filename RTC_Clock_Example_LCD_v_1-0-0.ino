/*
  RTC_Clock_Example_LCD_v_1-0-0
  
  Version 1.0.0
  
  This is a simple program that reads the time from a connected RTC
  module using the DS3231 IC and displays the information to
  a I2C 2x16 LCD display.

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

  I2C
  Display:  Connected To:

  GND       GND
  VCC       5 VDC
  SDA       SDA
  SCL       SCL
  
  Created:   August 29, 2024
  Modified:  August 29, 2024
  by Todd S. Canaday
  tcanaday@memphis.edu

  Arduino Compiler Version:  arduino-1.8.13

  Revision Notes:
  
  08/29/2024  Initial release of source code.
*/

#include <RTClib.h>
#include <LiquidCrystal_I2C.h>

#define DEBUG false                  //  set to true for console output, false for no console output...
#define Serial if(DEBUG)Serial
#define BAUDRATE 9600               //  default baud rate for console...

/********************/
/*  CONSTRUCTORS    */
/********************/
//  create an object of the DS3231 RTC module using address 0x53...
RTC_DS3231 rtc;
//  initialize I2C LCD display (address = 0x27)...
LiquidCrystal_I2C lcd(0x27,16,2);

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
  lcd.init();                       //  initialize the I2C LCD...
  lcd.noBacklight();                //  disable backlight for the I2C LCD...
  lcd.clear();                      //  clear the I2C LCD...
  lcd.backlight();                  //  enable backlight for the I2C LCD...
  
  Serial.begin(BAUDRATE);           //  start the serial port in case debug...
  
  pinMode(ledPin, OUTPUT);          //  Pin 13 has an LED connected...
  digitalWrite(ledPin, LOW);        //  turn the LED off...

  //  display a startup message to the I2C LCD display...
  DisplayStartupMessageToLCD();
  //  delay for specified time...
  delay(2000);
  
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
    //  show the data on the I2C display...
    PrintDateTimeToDisplay();
    //  get and display the date and time from the RTC to the serial console...
    DisplayDateTimeFromRTC();
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
 * Method used to query the RTC module and
 * get the current Date / Time stamp then
 * print this to console using the format
 * "MM/DD/YYYY HH:MM:SS (AM/PM)".
 */
void DisplayDateTimeFromRTC()
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
  lcd.print("RTC Example LCD");
  //  set cursor to character 0 on line 1...
  lcd.setCursor(0,1);
  //  print message to the LCD screen...
  lcd.print("v. 1.0.0");
}
