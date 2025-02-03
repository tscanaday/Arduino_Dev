/*
  Arduino MKR 1000 Example

  Version 3 - WiFi Reconnect Test Code for MKR1000
  
  Provides a working example for attempting to 
  reconnect the MKR 1000 when wireless router 
  becomes unavailable.  The program will display
  the WiFi RSSI value to indicate that the program
  is running.

  Created:    April 15, 2021
  Modified:   February 3, 2025
  by Todd S. Canaday, PMP
  toddcanaday@comcast.net
 
  Revision Notes:
  
  04/15/2021  Initial release of source based on
              Arduino MKR 1000 board.
  01/31/2025  Tested on home network - works now.
  02/03/2025  Release 1.0.1 code cleanup.
*/

#include <WiFi101.h>                      //  used for WiFi connectivity...

/******************************************/
/*  PLACE YOUR NETWORK INFORMATION HERE   */
/******************************************/
char ssid[] = "YOUR_SSID";                //  network SSID...
char pass[] = "YOUR_PASSWORD";            //  network password...

int status = WL_IDLE_STATUS;              //  the WiFi radio status...
WiFiClient client;                        //  WiFi client instance...
long rssi = 0;                            //  used to store WiFi RSSI reading value...

const long fiveSeconds = 5000;            //  defined time span of 5 seconds (5,000 ms)...
const long tenSeconds = 10000;            //  defined time span of 10 seconds (10,000 ms)...

long lastConnectionTime = 0;              //  last connection time...
const long postingInterval = fiveSeconds; //  interval for loop control...
int retries = 0;                          //  keep track of WiFi reconnect attempts...
int maxRetries = 3;                       //  define the number of retry attempts...

int led_pin = 6;                          //  on board LED specific to the MKR 1000...

int wifiConnected = 7;                    //  GREEN LED to indicate WiFi is connected...
int wifiDisconnected = 8;                 //  RED LED to indicate WiFi is disconnected...
int wifiRetry = 9;                        //  YELLOW LED to indicate WiFi is in retry attempt state...

void setup()
{
  pinMode(led_pin, OUTPUT);               //  pin 6 has an LED connected...
  digitalWrite(led_pin, LOW);             //  turn the LED off...

  pinMode(wifiConnected, OUTPUT);         //  green LED indicating WiFi connected...
  digitalWrite(wifiConnected, LOW);       //  turn the LED off...
  pinMode(wifiDisconnected, OUTPUT);      //  red LED indicating WiFi disconnected...
  digitalWrite(wifiDisconnected, LOW);    //  turn the LED off...
  pinMode(wifiRetry, OUTPUT);             //  yellow LED indicating WiFi retry connect...
  digitalWrite(wifiRetry, LOW);           //  turn the LED off...
  
  //  start the serial port...
  Serial.begin(9600);

  /****************************************************************/
  /*  NOTE:  you must comment out the code below if you want this */
  /*         program to run stand-alone (meaning not connected to */
  /*         a computer for serial port monitoring).  if you want */
  /*         to monitor the serial port output, then leave this   */
  /*         code uncommented so data will be sent to the serial  */
  /*         port monitor.                                        */
  /****************************************************************/
  //  wait for the serial port to connect...
  
  while(!Serial)
  {
    ;
  }
  
  Serial.println(F("Serial port active..."));
  
  /****************************************************************/
  /*  end serial port connect routine...                          */
  /****************************************************************/
  
  //  ensure WiFi is available...
  if(WiFi.status() == WL_NO_SHIELD)
  {
    Serial.println(F("WiFi shield not present..."));
    //  if WiFi shield not present, flash LEDs...
    FlashLEDs();
    //  remain in infinite loop here...
    while(true);
  }
  
  //  attempt to connect to WiFi network...
  while(status != WL_CONNECTED)
  {
    Serial.print(F("Attempting to connect to SSID: "));
    Serial.println(ssid);
    
    //  connect to the wireless network...
    status = WiFi.begin(ssid, pass);

    //  set status LED's - technically trying to connect...
    SetStatusLEDs(false, false, true);
    
    //  wait 10 seconds for WiFi connection...
    delay(tenSeconds);
  }

  //  show you are successfully connected to the network...
  Serial.print(F("*** Connected to "));
  Serial.print(ssid);
  Serial.println(F(" ***"));

  //  set status LED's showing we are connected...
  SetStatusLEDs(true, false, false);
}

void loop()
{
  //  check the interval posting time.  if longer than set posting interval time,
  //  we can enter this block of code and execute the commands...
  if(!client.connected() && ((millis() - lastConnectionTime) > postingInterval))
  {
    //  check the WiFi status on entry...
    status = WiFi.status();
  
    /********************************************/
    /*  get the WiFi signal strength (RSSI)...  */
    /********************************************/
    rssi = WiFi.RSSI();

    if(status == WL_CONNECTED) 
    {
      //  set status LED's showing we are connected...
      SetStatusLEDs(true, false, false);

      Serial.println(F("Successfully connected to the wireless network..."));
      //  write the RSSI value to the console using Serial...
      Serial.print(F("WiFi RSSI Value = "));
      Serial.println(rssi);
    }
    else if(status != WL_CONNECTED)
    {
      //  WiFi lost connection (status = 6) retry...
      if(status != WL_CONNECTED)
      {
        while((status != WL_CONNECTED) && (retries < maxRetries))
        {
          //  set status LED's showing we are trying to connect...
          SetStatusLEDs(false, false, true);
  
          //  connect to the wireless network...
          status = WiFi.begin(ssid, pass);
          //  increment number of retries...
          retries++;

          Serial.print(F("Lost connection to the network - Reconnect attempt "));
          Serial.print(retries);
          Serial.println(F("..."));
          
          //  wait 5 seconds for WiFi connection...
          delay(fiveSeconds);
        }

        //  if we were able to reconnect, display the RSSI...
        if(status == WL_CONNECTED) 
        {
          //  set status LED's showing we are connected...
          SetStatusLEDs(true, false, false);
      
          Serial.println(F("Successful reconnect to the network..."));
          //  write the RSSI value to the console using Serial...
          Serial.print(F("WiFi RSSI Value = "));
          Serial.println(rssi);
        }
        else
        {
          //  we have lost the WiFi connection and gone past our retry attempts,
          //  so show disconnected.  maybe we'll get it on the next loop...
          SetStatusLEDs(false, true, false);
          Serial.println(F("Lost connection to the network - retry attempts exceeded..."));
        }
      }
      else
      {
        //  nothing to do here...
      }

      //  reset the retries counter...
      retries = 0;
    }
    
    //  capture the last posting time...
    lastConnectionTime = millis();
  }
}

/**
 * TurnAllStatusLightsOn
 * Turns on all LED's 
 */
void TurnAllStatusLightsOn()
{
  digitalWrite(wifiConnected, HIGH);
  digitalWrite(wifiDisconnected, HIGH);
  digitalWrite(wifiRetry, HIGH);
}

/**
 * TurnAllStatusLightsOff
 * Turns off all LED's 
 */
void TurnAllStatusLightsOff()
{
  digitalWrite(wifiConnected, LOW);
  digitalWrite(wifiDisconnected, LOW);
  digitalWrite(wifiRetry, LOW);
}

/**
 * FlashLEDs
 * Turn on and off all indicator
 * LED's with a 100 ms delay inbetween
 */
void FlashLEDs()
{
  TurnAllStatusLightsOff();
  delay(100);
  TurnAllStatusLightsOn();
  delay(100);
}

/**
 * SetStatusLEDs
 * Sets the individual LED's based on the WiFi
 * state while in execution.
 */
void SetStatusLEDs(bool c, bool d, bool r)
{
  if(c == true)
  {
    //  turn on WiFi connected LED...
    digitalWrite(wifiConnected, HIGH);
  }
  else if(c == false)
  {
    //  turn off WiFi connected LED...
    digitalWrite(wifiConnected, LOW);
  }
  else
  {
  }

  if(d == true)
  {
    //  turn on WiFi disconnected LED...
    digitalWrite(wifiDisconnected, HIGH);
  }
  else if(d == false)
  {
    //  turn off WiFi disconnected LED...
    digitalWrite(wifiDisconnected, LOW);
  }
  else
  {
  }

  if(r == true)
  {
    //  turn on WiFi retry LED...
    digitalWrite(wifiRetry, HIGH);
  }
  else if(r == false)
  {
    //  turn off WiFi retry LED...
    digitalWrite(wifiRetry, LOW);
  }
  else
  {
  }
}
