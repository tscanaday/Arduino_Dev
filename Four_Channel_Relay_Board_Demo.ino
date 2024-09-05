/*
  Four Channel Electromechanical Relay Board
  
  Version 1
  
  This is a simple state machine that controls four relays
  on the SeeedStudio Relay Board.
    
  Created:   April 17, 2024
  Modified:  April 17, 2024
  by Todd S. Canaday
  tcanaday@memphis.edu
  
  Reference Link For Understanding Relays:
  
  https://docs.arduino.cc/tutorials/4-relays-shield/4-relay-shield-basics/
   
  Revision Notes:
  
  04-17-2024: Initial release of source code.

  Hardware Used:

  SeeedStudio Four Channel Relay Board
  SKU: 103030003
  Link:   https://www.seeedstudio.com/Relay-shield-V2-0-p-1376.html
*/

#define DEBUG true                       //  set to true for debug output, false for no debug output...
#define Serial if(DEBUG)Serial

const int ledPin = 13;                    //  on board LED...
const int ch1 = 7;                        //  relay #1...
const int ch2 = 6;                        //  relay #2...
const int ch3 = 5;                        //  relay #3...
const int ch4 = 4;                        //  relay #4...

const int fiftyms = 50;                   //  constant value for fifty milliseconds...
const int oneHunderedms = 100;            //  constant value for one hundered milliseconds...
const int oneSecond = 1000;               //  constant value for one second...
const long twoSeconds = 2000;             //  constant value for two seconds...
const long fiveSeconds = 5000;            //  constant value for five seconds...
const int startupDelay = fiftyms;         //  value used in between relay on / off in startup routine...
const int routineDelay = fiveSeconds;     //  the delay in ms between each loop during the program...

void setup()
{
  pinMode(ledPin, OUTPUT);      //  pin 13 has an LED connected...
  digitalWrite(ledPin, LOW);    //  turn the LED off...
  
  pinMode(ch1,OUTPUT);          //  relay #1...
  digitalWrite(ch1, LOW);       //  turn the relay off...
  pinMode(ch2,OUTPUT);          //  relay #2...
  digitalWrite(ch2, LOW);       //  turn the relay off...
  pinMode(ch3,OUTPUT);          //  relay #3...
  digitalWrite(ch3, LOW);       //  turn the relay off...
  pinMode(ch4,OUTPUT);          //  relay #4...
  digitalWrite(ch4, LOW);       //  turn the relay off...

  Serial.begin(9600);           //  start the serial port...
  
  //  used for displaying information to the console, if connected...
  Serial.println(F("Four Channel Relay Demo V1"));
  Serial.println(F("Todd S. Canaday"));
  Serial.println(F("tcanaday@memphis.edu"));
  Serial.println(F("April 17, 2024"));
  Serial.println();

  //  call startup method to indicate system
  //  startup was successful...
  WaterfallStartup();
}

/**
 * loop()
 * 
 * This is the main method for
 * program execution.
 */
void loop()
{
  //  state machine loop to execute the
  //  program...
  CycleRelaysByChannel();
}

/**
 * BlinkStartup()
 * 
 * Routine used to show visual confirmation
 * that the state machine program is running.
 */
void BlinkStartup()
{
  //  cause the relays to blink rapidly
  //  indicating the system is running...
  for(int a=0; a<3; a++)
  {
    //  turn on all relays...
    TurnAllRelaysOn;
    //  delay a period of time...
    delay(startupDelay);
    //  turn off all relays...
    TurnAllRelaysOff();
    //  delay a period of time...
    delay(startupDelay);
  }
  
  //  wait for a period of time before returning...
  delay(oneSecond);
}

/**
 * WaterfallStartup()
 * 
 * Routine used to show visual confirmation
 * that the state machine program is running.
 */
void WaterfallStartup()
{
  //  cause the relays to blink rapidly
  //  indicating the system is running...
  for(int a=0; a<3; a++)
  {
    //  turn on relay 4...
    TurnSingleRelayOn(ch4);
    //  delay a period of time...
    delay(startupDelay);
    //  turn off relay 4...
    TurnSingleRelayOff(ch4);
    //  delay a period of time...
    delay(startupDelay);

    //  turn on relay 2...
    TurnSingleRelayOn(ch2);
    //  delay a period of time...
    delay(startupDelay);
    //  turn off relay 2...
    TurnSingleRelayOff(ch2);
    //  delay a period of time...
    delay(startupDelay);

    //  turn on relay 3...
    TurnSingleRelayOn(ch3);
    //  delay a period of time...
    delay(startupDelay);
    //  turn off relay 3...
    TurnSingleRelayOff(ch3);
    //  delay a period of time...
    delay(startupDelay);

    //  turn on relay 1...
    TurnSingleRelayOn(ch1);
    //  delay a period of time...
    delay(startupDelay);
    //  turn off relay 1...
    TurnSingleRelayOff(ch1);
    //  delay a period of time...
    delay(startupDelay);
  }
  
  //  wait for a period of time before returning...
  delay(oneSecond);
}

/**
 * CycleRelaysByChannel()
 * 
 * Method used to cycle through each relay
 * one by one with a delay.
 */
void CycleRelaysByChannel()
{
  digitalWrite(ch1, HIGH);
  delay(routineDelay);
  digitalWrite(ch1, LOW);
  delay(routineDelay);

  digitalWrite(ch2, HIGH);
  delay(routineDelay);
  digitalWrite(ch2, LOW);
  delay(routineDelay);

  digitalWrite(ch3, HIGH);
  delay(routineDelay);
  digitalWrite(ch3, LOW);
  delay(routineDelay);

  digitalWrite(ch4, HIGH);
  delay(routineDelay);
  digitalWrite(ch4, LOW);
  delay(routineDelay);
}

/**
 * TurnAllRelaysOn
 * 
 * Method that turns all relays on.
 */
void TurnAllRelaysOn()
{
  digitalWrite(ch1, HIGH);
  digitalWrite(ch2, HIGH);
  digitalWrite(ch3, HIGH);
  digitalWrite(ch4, HIGH);
}

/**
 * TurnAllRelaysOff()
 * 
 * Method that turns all relays off.
 */
void TurnAllRelaysOff()
{
  digitalWrite(ch1, LOW);
  digitalWrite(ch2, LOW);
  digitalWrite(ch3, LOW);
  digitalWrite(ch4, LOW);
}

/**
 * TurnSingleRelayOn(int ch)
 * 
 * Method used to enable a single
 * relay.
 */
void TurnSingleRelayOn(int ch)
{
  digitalWrite(ch, HIGH);
}

/**
 * TurnSingleRelayOff(int ch)
 * 
 * Method used to disable a single
 * relay.
 */
void TurnSingleRelayOff(int ch)
{
  digitalWrite(ch, LOW);
}
