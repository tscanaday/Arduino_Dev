/*
  Web_Based_Two_Channel_Relay_Controller_v_1-0-1
  
  Version 1.0.1
  
  Application developed using the Arduino MKR1000 WiFi
  board to control a two channel relay board.  This can
  be used to remotely turn on / off each relay via a
  wireless connection through a web browser using the
  local network.
  
  Created:   February 12, 2025
  Modified:  February 14, 2025
  by Todd S. Canaday
  toddcanaday@comcast.net

  Arduino Compiler Version:  arduino-1.8.19

  Revision Notes:
  
  02/12/2025  Initial release of source code.
  02/14/2025  Clean up source, removing unused code,
              adding comments.
*/

#include <WiFi101.h>              //  used for WiFi connectivity...

#define BAUDRATE 9600             //  default baud rate for console...

IPAddress ip(192, 168, 0, 157);   //  the IP address to be used for this instance...

char ssid[] = "YOUR_SSID";        //  WiFi network SSID...
char pass[] = "YOUR_PASSWORD";    //  WiFi network password...
int status = WL_IDLE_STATUS;      //  the WiFi radio status...
WiFiServer server(80);            //  WiFi server instance (listening on port 80)...
long rssi = 0;                    //  used to store WiFi RSSI reading value...

const int ledPin = 6;             //  on board LED specific to the MKR 1000...

int wifiConnected = 7;            //  green LED to indicate WiFi is connected...
int wifiDisconnected = 8;         //  red LED to indicate WiFi is disconnected...
int wifiRetry = 9;                //  yellow LED to indicate WiFi is in retry attempt state...

int relayPin1 = 4;                //  relay 1 to pin 4...
int relayPin2 = 5;                //  relay 2 to pin 5...

String relayOneState = "Off";     //  string holding state of relay 1 on form...
String relayTwoState = "Off";     //  string holding state of relay 2 on form...

char linebuf[80];                 //  string for fetching data from address...
int charcount = 0;                //  character counter for request...
int blinkDelay = 100;             //  variable to delay between LED on / off state...
const long tenSeconds = 10000;    //  defined time span of 10 seconds (10,000 ms)...

bool debugMode = false;           //  variable used to enable console messages if serial enabled...

byte macAddr[6];                  //  variable to store MAC address of MKR1000 device...

//  set up the button names...
char *buttonName[] = {"Relay 1", "Relay 2"};

void setup()
{
  pinMode(ledPin, OUTPUT);              //  pin 6 has an LED connected...
  digitalWrite(ledPin, LOW);            //  turn the LED off...

  pinMode(relayPin1,OUTPUT);            //  relay 1 configuration as an output...
  digitalWrite(relayPin1, HIGH);        //  turn relay 1 off...

  pinMode(relayPin2,OUTPUT);            //  relay 2 configuration as an output...
  digitalWrite(relayPin2, HIGH);        //  turn relay 2 off...

  /****************************************************************/
  /*  NOTE:  you must comment out the code below if you want this */
  /*         program to run stand-alone (meaning not connected to */
  /*         a computer for serial port monitoring).  if you want */
  /*         to monitor the serial port output, then leave this   */
  /*         code uncommented so data will be sent to the serial  */
  /*         port monitor.                                        */
  /****************************************************************/
  //  wait for the serial port to connect...
  /*
  while(!Serial)
  {
    //  if we are not getting a response from the COM port monitor,
    //  then flash the YELLOW LED...
    BlinkStatusLEDsWaitingOnSerial();
  }
  
  Serial.println(F("Serial port active..."));
  */
  /****************************************************************/
  /*  end serial port connect routine...                          */
  /****************************************************************/
  
  //  display program information to the Serial monitor...
  Serial.begin(BAUDRATE);
  Serial.println("MKR1000 Relay Controller");
  Serial.println("Todd S. Canaday");
  Serial.println("toddcanaday@comcast.net");
  Serial.println("February 14, 2025");
  Serial.println();
  
  //  ensure WiFi is available...
  if(WiFi.status() == WL_NO_SHIELD)
  {
    Serial.println("WiFi shield not present...");
    //  remain in infinite loop here...
    while(true);
  }

  //  set the IP address for this device (configured for the WiFi network)...
  WiFi.config(ip);
  
  //  attempt to connect to WiFi network...
  while(status != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    
    //  connect to the wireless router...
    status = WiFi.begin(ssid, pass);

    //  set status LED's - technically trying to connect...
    SetStatusLEDs(false, false, true);
    
    //  wait 10 seconds for WiFi connection...
    delay(tenSeconds);
  }

  //  verify that we are connected to the wireless router,
  //  and if so turn on the GREEN LED..
  if(status == WL_CONNECTED)
  {
    //  show you are successfully connected to the wireless router...
    Serial.print(F("Connected to "));
    Serial.println(ssid);
  
    //  set status LED showing we are connected...
    SetStatusLEDs(true, false, false);

    //  get IP address the MKR1000 is associated with...
    IPAddress ip = WiFi.localIP();
    Serial.print(F("IP Address: "));
    Serial.println(ip);

    //  get the MAC address of the MKR1000 device...
    WiFi.macAddress(macAddr);
    Serial.print(F("MAC Address: "));
    PrintMacAddress(macAddr);
    
    //  print out the encryption type used by the wireless router...
    Serial.print(F("Encryption type: "));
    PrintEncryptionType(WiFi.encryptionType());

    //  start the MKR1000 server...
    server.begin();
  }
}

void loop()
{
  HttpRequest();    //  constantly listen for HTTP request on port...
}

/**
 * HttpRequest
 * Method used to check for any HTTP requests
 * incoming and parse through the received
 * request to begin rendering the page.
 */
void HttpRequest()
{
  //  get request...
  WiFiClient client = server.available();
  
  if(client)
  {
    memset(linebuf,0,sizeof(linebuf));
    charcount = 0;

    // an http request ends with a blank line...
    boolean currentLineIsBlank = true;

    //  as long as we are connected we can proceed...
    while(client.connected())
    {
      if(client.available())
      {
        char c = client.read();
        //  read char by char HTTP request...
        linebuf[charcount] = c;

        if(debugMode)
        {
          Serial.print(F("char c:  "));
          Serial.println(c);
        }

        if(charcount<sizeof(linebuf)-1)
        {
          charcount++;
        }

        //  if you've reached the end of the line (received a newline
        //  character) and the line is blank, the http request has ended,
        //  so you can now send a reply...
        if (c == '\n' && currentLineIsBlank)
        {
          if(debugMode)
          {
            Serial.println(F("In if statement..."));
  
            for(int p=0; p<sizeof(linebuf)-1; p++)
            {
              Serial.println(linebuf[p]);
            }
          }
          
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          client.println("<html>");
          client.println("<head>");
          client.println("<title>MKR1000 Relay Controller</title>");
          client.println("</head>");

          client.println("<body style=background-color:white>");
          client.println("<table border='0'>");
          client.println("<tr width='50%'>");
          client.println("<td style='font-family: times, courier;font-size: 24px'>");
          client.println("MKR1000 Relay Controller");
          
          client.println("</td>");
          client.println("</tr>");
          client.println("</table>");
          client.println("<br />");

          //  show the main HTML form to the user...
          DashboardView(client);

          //  handle end of HTML page...
          PrintBottomPartOfPage(client);

          break;
        }

        if (c == '\n')
        {
          /********************************/
          /*  RELAY 1                     */
          /********************************/
          if (strstr(linebuf,"GET /relay1off") > 0)
          {
            digitalWrite(relayPin1, HIGH);
            relayOneState = "Off";
          }
          else if (strstr(linebuf,"GET /relay1on") > 0)
          {
            digitalWrite(relayPin1, LOW);
            relayOneState = "On";
          }

          /********************************/
          /*  RELAY 2                     */
          /********************************/
          if (strstr(linebuf,"GET /relay2off") > 0)
          {
            digitalWrite(relayPin2, HIGH);
            relayTwoState = "Off";
          }
          else if (strstr(linebuf,"GET /relay2on") > 0)
          {
            digitalWrite(relayPin2, LOW);
            relayTwoState = "On";
          }
          
          // starting a new line...
          currentLineIsBlank = true;
          memset(linebuf,0,sizeof(linebuf));
          charcount=0;
        }
        else if (c != '\r')
        {
          //  you have a character on the current line...
          currentLineIsBlank = false;
        }
      }
    }

    delay(100);         //  give the web browser time to receive the data...
    client.stop();      //  close the connection...
  }
}

/**
 * DashboardView
 * Method used to display the control buttons on
 * the HTML page and the state of each button.
 */
void DashboardView(WiFiClient cl)
{
  cl.println("<table border='0'>");
 
  /********************************/
  /*  RELAY 1                     */
  /********************************/
  if(relayOneState == "Off")
  {
    // if the relay is off, set button to turn relay on...
    cl.println("<tr>");
    cl.println("<td align='left'>");
    cl.println("<font size='3' face='Trebuchet MS'>");
    cl.println(buttonName[0]);
    cl.println("</font>");
    cl.println("</td>");
    cl.println("<td>");
    cl.println("&nbsp;&nbsp;<a href=\"/relay1on\"><button>Turn ON</button></a>");
    cl.println("</td>");
    cl.println("</tr>");
  }
  else if(relayOneState == "On")
  {
    // if the relay is on, set button to turn relay off...
    cl.println("<tr>");
    cl.println("<td align='left'>");
    cl.println("<font size='3' face='Trebuchet MS'>");
    cl.println(buttonName[0]);
    cl.println("</font>");
    cl.println("</td>");
    cl.println("<td>");
    cl.println("&nbsp;&nbsp;<a href=\"/relay1off\"><button>Turn OFF</button></a>");
    cl.println("</td>");
    cl.println("</tr>");
  }

  /********************************/
  /*  RELAY 2                     */
  /********************************/
  if(relayTwoState == "Off")
  {
    // if the relay is off, set button to turn relay on...
    cl.println("<tr>");
    cl.println("<td align='left'>");
    cl.println("<font size='3' face='Trebuchet MS'>");
    cl.println(buttonName[1]);
    cl.println("</font>");
    cl.println("</td>");
    cl.println("<td>");
    cl.println("&nbsp;&nbsp;<a href=\"/relay2on\"><button>Turn ON</button></a>");
    cl.println("</td>");
    cl.println("</tr>");
  }
  else if(relayTwoState == "On")
  {
    // if the relay is on, set button to turn relay off...
    cl.println("<tr>");
    cl.println("<td align='left'>");
    cl.println("<font size='3' face='Trebuchet MS'>");
    cl.println(buttonName[1]);
    cl.println("</font>");
    cl.println("</td>");
    cl.println("<td>");
    cl.println("&nbsp;&nbsp;<a href=\"/relay2off\"><button>Turn OFF</button></a>");
    cl.println("</td>");
    cl.println("</tr>");
  }

  cl.println("</table>");
}

/**
 * PrintBottomPartOfPage
 * Prints out the bottom section of the HTML
 * page with RSSI info and the copyright
 * info.
 */
void PrintBottomPartOfPage(WiFiClient cl)
{
  //  get the WiFi RSSI from the MKR1000...
  rssi = WiFi.RSSI();

  //  string to hold the copyright information...
  String cr = "Copyright &#169; 2025";

  //  finish off the copyright information...
  cr = cr + "<br />Add Your Name Here<br />All rights reserved<br>";
    
  cl.println("<table border='0'>");
  cl.println("<tr>");
  cl.println("<td>&nbsp;</td>");
  cl.println("</tr>");
  cl.println("<tr>");
  cl.println("<td>&nbsp;</td>");
  cl.println("</tr>");
  cl.println("<tr>");
  cl.println("<td>&nbsp;</td>");
  cl.println("</tr>");
  cl.println("</table>");

  cl.println("<table border='0' align='left' width='50%'>");

  //  display the MKR1000 RSSI...
  cl.println("<tr>");
  cl.println("<td>");
  cl.println("<font size='2' face='Trebuchet MS'>");
  //  display RSSI in bold...
  cl.println("<b>MKR1000 RSSI:&nbsp;&nbsp;");
  cl.println(String(rssi));
  cl.println("</b></font>");
  cl.println("</td>");
  cl.println("</tr>");

  //  add space between RSSI and copyright info...
  cl.println("<tr>");
  cl.println("<td>&nbsp;</td>");
  cl.println("</tr>");

  //  display the copyright info next...
  cl.println("<tr>");
  cl.println("<td>");
  cl.println("<font size='2' face='Trebuchet MS'>");
  cl.println(cr);
  cl.println("</font>");
  cl.println("</td>");
  cl.println("</tr>");
  
  cl.println("</table>");

  cl.println("<br />");
  cl.println("</body>");
  cl.println("</html>");
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

/**
 * BlinkStatusLEDsWaitingOnSerial
 * Method to blink the YELLOW status
 * LED in case of waiting on serial port 
 */
void BlinkStatusLEDsWaitingOnSerial()
{
  SetStatusLEDs(false, false, true);
  delay(blinkDelay);
  SetStatusLEDs(false, false, true);
  delay(blinkDelay);
}

/**
 * PrintEncryptionType
 * Method used to translate the encryption
 * type number retruned from the WiFi Router
 * query.
 */
void PrintEncryptionType(int et)
{
  // read the encryption type and print out the title:
  switch(et)
  {
    case ENC_TYPE_WEP:
      Serial.println(F("WEP"));
      break;
    case ENC_TYPE_TKIP:
      Serial.println(F("WPA"));
      break;
    case ENC_TYPE_CCMP:
      Serial.println(F("WPA2"));
      break;
    case ENC_TYPE_AUTO:
      Serial.println(F("Auto"));
      break;
    case ENC_TYPE_NONE:
      Serial.println(F("None"));
      break;
    default:
      Serial.println(F("Unknown"));
      break;
  }
}

/**
 * PrintMacAddress
 * Method used to print the MAC
 * address of the MKR1000 device.
 */
void PrintMacAddress(byte mac[])
{
  for (int i = 5; i >= 0; i--)
  {
    if(mac[i] < 16)
    {
      Serial.print(F("0"));
    }
    
    Serial.print(mac[i], HEX);
    
    if (i > 0)
    {
      Serial.print(F(":"));
    }
  }
  Serial.println();
}
