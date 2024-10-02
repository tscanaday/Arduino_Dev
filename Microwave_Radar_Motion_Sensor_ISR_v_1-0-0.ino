/*
  Microwave_Radar_Motion_Sensor_ISR_v_1-0-0
  
  Version 1.0.0
  
  This is a simple program that reads the data line from a RCWL-0516 Microwave Radar
  Motion Sensor using an Interrupt Service Routine (ISR).  Motion that is detected
  will be displayed to the I2C LCD Display.

  Reference:  https://lastminuteengineers.com/rcwl0516-microwave-radar-motion-sensor-arduino-tutorial/

  Device:     Address:
  -------     --------
  I2C LCD     0x27

  2x16 LCD
  Panel
  Pin:      Connected To:

  1         GND
  2         5 VDC
  3         Arduino Pin A4 (SDA)
  4         Arduino Pin A5 (SCL)
  
  Created:   October 1, 2024
  Modified:  October 1, 2024
  by Todd S. Canaday
  tcanaday@memphis.edu

  Arduino Compiler Version:  arduino-1.8.13

  Revision Notes:
  
  10/01/2024  Initial release of source code.
*/

#include <avr/wdt.h>
#include <LiquidCrystal_I2C.h>

#define STATE_NONE 0x00             //  defined state 0x00...
#define STATE_ONE  0x01             //  defined state 0x01
#define BAUDRATE 9600               //  default baud rate for console...
#define DEBUG false                 //  set to true for console output, false for no console output...
#define Serial if(DEBUG)Serial

/********************/
/*  CONSTRUCTORS    */
/********************/
//  initialize I2C LCD display (address = 0x27)...
LiquidCrystal_I2C lcd(0x27,16,2);

/********************/
/*  VARIABLES       */
/********************/
const int ledPin = 13;              //  on board LED...
int blinkDelay = 100;               //  variable to delay between LED on / off state...
int delayTime = 2000;               //  variable to define the delay time for message to the LCD panel...
const int sensorPin = 2;            //  used for RCWL-0516 detector output...
int motionCounter = 0;              //  variable used to keep track of the number of movements detected...
int debounceTime = 200;             //  variable used to store value for debounce time...
//  specifically used for the ISR states to be able to detect change in state...
volatile unsigned long lastState = 0x00;
volatile byte stateStatus = STATE_NONE;

void setup()
{
  wdt_disable();                    //  disable the watchdog timer...

  //  set up interrupt function to fire on a RISING input signal...
  attachInterrupt(0, MotionDetectorISR, RISING);
  
  lcd.init();                       //  initialize the I2C LCD...
  lcd.noBacklight();                //  disable backlight for the I2C LCD...
  lcd.clear();                      //  clear the I2C LCD...
  lcd.backlight();                  //  enable backlight for the I2C LCD...
  
  Serial.begin(BAUDRATE);           //  start the serial port in case debug...
  
  pinMode(ledPin, OUTPUT);          //  Pin 13 has an LED connected...
  digitalWrite(ledPin, LOW);        //  turn the LED off...
  pinMode(sensorPin, INPUT);        //  set pin attached to RCWL-0516 chip...
  
  DisplayStartupMessageToLCD();     //  display a startup message to the I2C LCD display...
  delay(delayTime);                 //  delay for specified time...
  DisplayMessageToLCD("");          //  set LCD display to monitor state...
  wdt_enable(WDTO_2S);              //  enable the watchdog timer for 2 seconds...
}

void loop()
{
  wdt_reset();                      //  reset the watchdog timer...

  //  state has changed due to ISR...
  if(stateStatus)
  {
    motionCounter++;                              //  increment the counter...
    DisplayMessageToLCD("Motion detected");       //  display the message...
    delay(delayTime);                             //  delay to show message for a period of time...
    DisplayMessageToLCD("");                      //  set LCD display to monitor state...

    noInterrupts();                               //  clear all interrupts...
    stateStatus = STATE_NONE;                     //  reset the ISR state...
    interrupts();                                 //  enable all interrupts...
  }
}

/**
 * MotionDetectorISR()
 * Method used as an Interrupt Service Routine
 * to capture movement detection faster than it
 * would just checking logic levels.
 */
void MotionDetectorISR()
{
  //  get the current time from the processor (ticks)...
  unsigned long timeNow = millis();

  //  if interrupts come faster than debounceTime, assume it's a bounce and ignore...
  if(timeNow - lastState > debounceTime)
      stateStatus = STATE_ONE;
  //  capture the timestamp of last interrupt...
  lastState = timeNow;
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
  lcd.print("Motion Detector");
  //  set cursor to character 0 on line 1...
  lcd.setCursor(0,1);
  //  print message to the LCD screen...
  lcd.print("(ISR) v. 1.0.0");
}

/**
 * DisplayMessageToLCD
 * Method used to display a message
 * to the I2C LCD display.
 */
void DisplayMessageToLCD(String msg)
{
  //  clear the I2C LCD dsiplay...
  lcd.clear();
  //  set cursor to character 0 on line 0...
  lcd.setCursor(0,0);
  //  print message to the LCD screen...
  lcd.print("Count: ");
  lcd.print(motionCounter);
  //  set cursor to character 0 on line 1...
  lcd.setCursor(0,1);
  //  print message to the LCD screen...
  lcd.print(msg);
}
