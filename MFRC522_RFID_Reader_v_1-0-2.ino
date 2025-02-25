/*
  MFRC522_RFID_Reader_v_1-0-2

  Version 1.0.2
  
  Example code interfacing with the MFRC522 
  RFID reader.  This version allows for a device
  (LED or Relay) to be activated once a valid
  UID is decoded.

  Connection / Wiring Information:
  
  MFRC522     
  Pin:      Connected To:

  SDA       Arduino Uno Pin 10
  SCK       Arduino Uno Pin 13
  MOSI      Arduino Uno Pin 11
  MISO      Arduino Uno Pin 12
  IRQ       No Connection
  GND       Arduino Uno GND
  RST       Arduino Uno Pin 9
  3.3V      Arduino Uno 3.3V

  Arduino
  Uno:      Connected To:

  2         Green LED
  3         Red LED
  4         Yellow LED
  
  Created:   February 24, 2025
  Modified:  February 25, 2025
  by Todd S. Canaday
  toddcanaday@comcast.net

  Arduino Compiler Version:  arduino-1.8.19

  Reference:  https://lastminuteengineers.com/how-rfid-works-rc522-arduino-tutorial/

  Revision Notes:
  
  02/24/2025  Initial release of source code.
  02/25/2025  Added Green, Red, and Yellow LED's.
  02/25/2025  Added method to display UID read.
*/
#include <SPI.h>                    //  include SPI bus library...
#include <MFRC522.h>                //  include MFRC522 library...

#define RST_PIN 9                   //  define RESET pin as pin 9 (Arduino Uno Pin 9)...
#define SS_PIN 10                   //  define SS pin as SDA (Arduino Uno Pin 10)...
#define BAUDRATE 9600               //  default baud rate for console...
#define DEBUG true                  //  set to true for console output, false for no console output...
#define Serial if(DEBUG)Serial

String MasterTag1 = "DA38B73";      //  RFID tag shipped with card reader key FOB (UID: DA38B73)...
String MasterTag2 = "00000000";     //  RFID tag shipped with card reader card (UID: 438F2931)...
String tagID = "";                  //  variable used to store the RFID UID read from the reader...

const int ledPin = 13;              //  on board LED...
const int grantedPin = 2;           //  pin to Green LED indicating access granted on device swipe...
const int deniedPin = 3;            //  pin to Red LED indicating access denied on device swipe...
const int pausePin = 4;             //  pin to Yellow LED indicating when reset delay is complete...

const int ledDelayValue = 2000;     //  value to keep the LED on...
const int pauseTime = 1000;         //  value to pause between card readings...
const int blinkDelay = 100;         //  value to pause between LED blinking...

//  create an object of the MFRC522 class...
MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup() 
{
  Serial.begin(BAUDRATE);           //  set the baud rate for the serial port...

  pinMode(ledPin, OUTPUT);          //  Pin 13 has the onboard LED connected...
  digitalWrite(ledPin, LOW);        //  turn the LED off...

  pinMode(grantedPin, OUTPUT);      //  Pin 2 used for Green granted LED...
  digitalWrite(grantedPin, LOW);    //  turn the LED off...

  pinMode(deniedPin, OUTPUT);       //  Pin 3 used for Red denied LED...
  digitalWrite(deniedPin, LOW);     //  turn the LED off...

  pinMode(pausePin, OUTPUT);        //  Pin 4 used for Yellow pause LED...
  digitalWrite(pausePin, LOW);      //  turn the LED off...
  
  //  display the program information to the serial port...
  DisplayProgramInformation();
  
  //  start the SPI bus and the MFRC522 reader...
  SPI.begin();
  mfrc522.PCD_Init();

  //  call the startup routine...
  StartupRoutine();
  
  //  provide instructions to the user...
  ProvideInstructionsToUser();
}

void loop() 
{
  //  wait until a new tag is available...
  while(GetUID()) 
  {
    //  if this is the UID we want to see, grant access...
    if(tagID == MasterTag1) 
    {
      //  print message to the console...
      Serial.println(F("Access Granted!"));
      //  display the UID to the serial port monitor...
      DisplayUIDToConsole(tagID);
      //  enable the appropriate LED...
      EnableGrantedLed();
    }
    else if(tagID == MasterTag2) 
    {
      //  print message to the console...
      Serial.println(F("Access Granted!"));
      //  display the UID to the serial port monitor...
      DisplayUIDToConsole(tagID);
      //  enable the appropriate LED...
      EnableGrantedLed();
    }
    else
    {
      //  print message to the console...
      Serial.println(F("Access Denied!"));
      //  display the UID to the serial port monitor...
      DisplayUIDToConsole(tagID);
      //  enable the appropriate LED...
      EnableDeniedLed();
    }

    //  turn on the yellow LED...
    TurnPausePinOn();
    //  set a time delay for next reading...
    delay(pauseTime);
    //  turn off the yellow LED...
    TurnPausePinOff();
    
    //  provide instructions to the user...
    ProvideInstructionsToUser();
  }
}

/**
 * DisplayProgramInformation
 * Method used to display the program
 * information to the console.
 */
void DisplayProgramInformation()
{
  Serial.println(F("MFRC522 RFID Reader Example"));
  Serial.println(F("Todd S. Canaday"));
  Serial.println(F("toddcanaday@comcast.net"));
  Serial.println(F("February 25, 2025"));
  Serial.println();
}

/**
 * ProvideInstructionsToUser
 * Method used to give the user instructions
 * on what to do.
 */
void ProvideInstructionsToUser()
{
  Serial.println();
  Serial.println(F("Place card / tag near the MFRC522 reader..."));
  Serial.println();
}

/**
 * DisplayUIDToConsole
 * Displays the UID read to the console
 * interface.
 */
void DisplayUIDToConsole(String theUid)
{
  //  display the UID to the console...
  Serial.print(F("UID tag:   "));
  Serial.println(theUid);
}
/**
 * GetUID
 * Method to read the UID from
 * the card reader.
 */
boolean GetUID() 
{
  //  getting ready for reading the RFID tag / card...
  if(!mfrc522.PICC_IsNewCardPresent())
  {
    //  if a new RFID device is placed to the reader, then read...
    return false;
  }
  if(!mfrc522.PICC_ReadCardSerial())
  {
    //  new RFID device is placed to the reader, then read...
    return false;
  }

  //  clear the contents of the tagID variable...
  tagID = "";
  
  for(uint8_t i = 0; i < 4; i++)
  {
    //  the MIFARE RFID that is being used...
    tagID.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  //  convert the UID to upper case...
  tagID.toUpperCase();
  //  stop reading the RFID device...
  mfrc522.PICC_HaltA();
  //  return from the method...
  return true;
}

/**
 * TurnPausePinOn
 * Method to turn the yellow LED on.
 */
void TurnPausePinOn()
{
  digitalWrite(pausePin, HIGH);       //  turn the LED on...
}

/**
 * TurnPausePinOff
 * Method to turn the yellow LED off.
 */
void TurnPausePinOff()
{
  digitalWrite(pausePin, LOW);        //  turn the LED off...
}

/**
 * EnableGrantedLed
 * Method used to enable the green
 * LED so user has a visual indication
 * that device was accepted.
 */
void EnableGrantedLed()
{
  digitalWrite(grantedPin, HIGH);     //  turn the LED on...
  delay(ledDelayValue);               //  hold the LED on for a period of time...
  digitalWrite(grantedPin, LOW);      //  turn the LED off...
}

/**
 * EnableDeniedLed
 * Method used to enable the red
 * LED so user has a visual indication
 * that device was denied.
 */
void EnableDeniedLed()
{
  digitalWrite(deniedPin, HIGH);      //  turn the LED on...
  delay(ledDelayValue);               //  hold the LED on for a period of time...
  digitalWrite(deniedPin, LOW);       //  turn the LED off...
}

/**
 * StartupRoutine
 * Method called on program startup.
 */
void StartupRoutine()
{
  //  loop through the number of iterations to blink the LED's...
  for(int x=0; x<3; x++)
  {
    digitalWrite(grantedPin, HIGH);     //  turn the Green LED on...
    digitalWrite(deniedPin, HIGH);      //  turn the Red LED on...
    digitalWrite(pausePin, HIGH);       //  turn the Yellow LED on...
    delay(blinkDelay);                  //  pause between LED state...
    
    digitalWrite(grantedPin, LOW);      //  turn the Green LED off...
    digitalWrite(deniedPin, LOW);       //  turn the Red LED off...
    digitalWrite(pausePin, LOW);        //  turn the Yellow LED off...
    delay(blinkDelay);                  //  pause between LED state...
  }
}
