/*
  DFPlayerMini Example
  Version 1
  
  Interfaces ATMega-328 with DFPlayer module
  to play MP3 fileswhen button pressed.  

  *** Utilizes the DFPlayerMini_Fast library:
  *** URL:
  *** https://github.com/PowerBroker2/DFPlayerMini_Fast

  This version will play a single file or a 
  series of files based on the state of a 
  particular pin.
    
  Created:   December 1, 2024
  Modified:  December 3, 2024
  by Todd S. Canaday
  tcanaday@memphis.edu
     
  Revision Notes:
  
  12-01-2024:  Initial release of source code.
  12-01-2024:  Modified code to disable interrupts while 
               busy pin is low.
  12-02-2024:  Added logic to enable interrupts only once
               in the loop when player is complete.
  12-03-2024:  Added option to play series of files based on 
               input pin state.
               
  DFPlayer Connections:
  
  Pin:          Connects To:
  ----          ------------
   1            VCC
   2            To Arduino Pin 11 via 1K Ohm
   3            To Arduino Pin 10 via 1K Ohm
   6            SPK (-)
   7            GND
   8            SPK (+)
   16           To Arduino Pin 12
   
  Additional Information:
  
  Requires the inclusion of the DFPlayerMini_Fast library:
  (https://github.com/PowerBroker2/DFPlayerMini_Fast)

  Example Wiring Diagram:
  DFPlayerMini_Fast:
  https://user-images.githubusercontent.com/20977405/54732436-2623ae80-4b6a-11e9-91a7-fe4cce416eaa.png
*/

#include "Arduino.h"
#include <SoftwareSerial.h>
#include <DFPlayerMini_Fast.h>

int rx_pin = 10;                //  DFPlayer RX to Arduino pin 10...
int tx_pin = 11;                //  DFPlayer TX toArduinopin 11...
int busy_pin = 12;              //  DFPlayer BUSY connected to pin 12...
const int led_pin = 13;         //  this is the on board LED...
const int error_pin = 9;        //  visual error indicator...
volatile const int btnISR = 2;  //  pin 2 (interrupt pin) used for button connection...
int allow_isr_enable = false;   //  flag to allow interrupt enable just once in the loop...

int sound_file = 1;             //  used to store the sound file number...
int MAX_FILES = 0;              //  used to store the mp3 file count...
const int state_pin = 8;        //  used to set state of system from pin 8...
int current_state = 0;          //  stores the value read from the state_pin input...

SoftwareSerial mySoftwareSerial(rx_pin, tx_pin);
DFPlayerMini_Fast myDFPlayer;

void setup()
{
  pinMode(led_pin, OUTPUT);           //  pin 13 is the on board LED...
  digitalWrite(led_pin, LOW);         //  turn the on board LED off...
  pinMode(busy_pin, INPUT);           //  set busy_pin to input...
  pinMode(error_pin, OUTPUT);         //  error_pin gives visual indication for DFPlayer error...
  digitalWrite(error_pin, LOW);       //  turn error_pin LED off...
  pinMode(btnISR, INPUT);             //  set button as interrupt pin...
  pinMode(state_pin, INPUT);          //  pin 8 used to detect desired state...

  Serial.begin(9600);
  mySoftwareSerial.begin(9600);

  //  adding true,false eliminates the pops at startup...
  if(!myDFPlayer.begin(mySoftwareSerial))
  {
    //  visually show error LED indicating problem with DFPlayer module...
    digitalWrite(error_pin, HIGH);
    //  stay here forever until ATMega-328 is reset...
    while(true);
  }
  else
  {
    //  turn off error LED...
    digitalWrite(error_pin, LOW); 
  }

  myDFPlayer.EQSelect(EQ_BASE);                 //  set EQ to BASS (normal/pop/rock/jazz/classic/bass)...
  myDFPlayer.volume(13);                        //  set volume value (0~30)...

  //  read the number of files on the SD card...
  MAX_FILES = myDFPlayer.numSdTracks();

  //  reading the state pin within the setup() function will keep the
  //  selected program running even if the state of the input changes.
  //  state of '0' (LOW) - play single file only
  //  state of '1' (HIGH) - play ordered file from collection on SD card
  //  REQUIRES HARDWARE RESET TO DETECT CHANGE!
  current_state = digitalRead(state_pin);

  //  set up interrupt function to be active low...
  attachInterrupt(0, ButtonPressISR, LOW);
}

void loop()
{
  while((digitalRead(busy_pin) == LOW) && (allow_isr_enable == false))
  {
    //  disable interrupts while mp3 file is playing...
    noInterrupts();
    //  set flag to allow interrupts to be enabled after play is done...
    allow_isr_enable = true;
  }
  
  while((digitalRead(busy_pin) == HIGH) && (allow_isr_enable == true))
  {
    //  enable interrupts after the mp3 file is complete...
    interrupts();
    //  clear the flag so we're not constantly enabling interrupts...
    allow_isr_enable = false;
  }
}

void ButtonPressISR()
{
  static unsigned long last_interrupt_time = 0;   //  variable to store last milliseconds read value...
  unsigned long interrupt_time = millis();        //  get the current milliseconds value...
  
  //  if interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 200)
  {
    if(current_state == 0)
    {
      myDFPlayer.playFromMP3Folder(1);            //  play the mp3 file...
    }
    else
    {
      myDFPlayer.playFromMP3Folder(sound_file);   //  play the selected mp3 file...

      //  check to see if we can increment to file number...
      if(sound_file < MAX_FILES)
      {
        //  increment to the next file...
        sound_file++;
      }
      else
      {
        //  reset back to the first file...
        sound_file = 1;
      }
    }
  }
  
  last_interrupt_time = interrupt_time;           //  capture and store the last interrupt milliseconds...
}
