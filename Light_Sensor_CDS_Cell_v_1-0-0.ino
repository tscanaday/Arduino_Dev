/*
  Light_Sensor_CDS_Cell_v_1-0-0

  This program will read the input on Analog Input 5,
  perform the necessary conversions, and then 
  send the calculated results to the console.

  Reference Link For Connecting:

  https://spiceman.net/arduino-cds-program/
  
  Connection Data:
  
  cDs Photocells:
  
  Connect the CdS photocell to Vcc through a 10K pull-up
  resistor with the other lead to ground.  Connect A5 to
  the junction of the CdS photocell and the 10K resistor.

            CdS
            Cell        10K
           ____
  +5V ----|____|-------\/\/\/\-----|
                   |               |
                   |               |
                   |            -------  GND
                To A5             ---
                                   -

  Created:   June 18, 2024
  Modified:  June 18, 2024
  by Todd S. Canaday
  tcanaday@memphis.edu

  Arduino Compiler Version:  arduino-1.8.13

  Revision Notes:
  
  06-18-2024  Initial release of source code.
*/

const int ledPin = 13;                    //  on board LED...
const int analogInPin1 = A0;              //  used for pre-read...
const int lightSensorInput = A5;          //  used to monitor light through sensor...
const int ARRAY_SIZE = 10;                //  sets the size of the storage array...
int lightSensorArray[ARRAY_SIZE];         //  buffer to store light sensor readings...
int cnt_sensor_index = 0;                 //  used to track position on arrays for light sensor...
int sensorLightTotal = 0;                 //  used to store total of light sensor array...
int sensorLightAverage = 0;               //  used to store average of light sensor array...
double sensorLight = 0.0;                 //  used to store calculated light sensor value...
long lastDisplayTime = 0;                 //  used to keep track of last display of light sensor reading... 
const double boardVoltage = 5.0;          //  calibration value of DC voltage...
const long tenSeconds = 10000;            //  defined time span of 10 seconds (10,000 ms)...
const long fiveSeconds = 5000;            //  defined time span of 5 seconds (5,000 ms)...
const long postingInterval = tenSeconds;  //  sets the posting interval...

void setup()
{
  //  pin 13 has an LED connected...
  pinMode(ledPin, OUTPUT);
  //  turn the LED off
  digitalWrite(ledPin, LOW);
  
  Serial.begin(9600);
  Serial.println(F("Light_Sensor_CDS_Cell_v_1-0-0"));
  Serial.println(F("Todd S. Canaday"));
  Serial.println(F("tcanaday@memphis.edu"));
  Serial.println(F("June 18, 2024"));
  Serial.println();
}

void loop()
{
  //  constantly read the light sensor input...
  GetLightSensorReading();

  //  check to see if it is time to post data to the console...
  if((millis() - lastDisplayTime) > postingInterval)
  {
    //  display the light sensor voltage to the console...
    DisplayLightSensorReading(sensorLight);
    //  display the light sensor percentage to the console...
    DisplayLightSensorPercentage(sensorLight);
    
    //  capture the last posting time...
    lastDisplayTime = millis();
  }
}

/**
 * GetLightSensorReading()
 * Method used to read photocell value.  This method
 * employs an array to average the values read and
 * normalize the analog data received.
 */
void GetLightSensorReading()
{
  //  subtract the last reading...                                     
  sensorLightTotal = sensorLightTotal - lightSensorArray[cnt_sensor_index];

  //  read the input from analog 1 but
  //  do nothing with the value, then delay
  //  for 10 ms.  this will help in settling
  //  the A/D converter...
  analogRead(analogInPin1);
  //  delay for 10 ms...
  delay(10);
  //  read the analog input for the light sensor...
  lightSensorArray[cnt_sensor_index] = analogRead(lightSensorInput);
  //  delay for 20 ms...
  delay(20);

  //  add the reading to the total...
  sensorLightTotal = sensorLightTotal + lightSensorArray[cnt_sensor_index];

  //  advance to the next position in the array...  
  cnt_sensor_index++;

  //  if we're at the end of the array then
  //  wrap around to the beginning...
  if (cnt_sensor_index >= ARRAY_SIZE)
  {
    cnt_sensor_index = 0;
  }

  //  calculate the average...
  sensorLightAverage = sensorLightTotal / ARRAY_SIZE;

  //  convert read values to appropriate units...
  sensorLight = CalculateLightSensor(sensorLightAverage);
}

/**
 * double CalculateLightSensor(int v)
 * Method used to return the reading from the input pin
 * connected to the photocell device.  Returns the value
 * as a double.
 */
double CalculateLightSensor(int v)
{
  //  this returns as a voltage read from the A/D converter...
  return((v / 1024.0) * boardVoltage);
}

/**
 * double CalculatePercentageLight(double r)
 * Method used to calculate the percentage of light
 * detected.  Returns the percentage as a double.
 */
double CalculatePercentageLight(double r)
{
  double pct = 100.0-((r/boardVoltage)*100.0);
  return(pct);
}

/**
 * DisplayLightSensorReading(double value)
 * Displays the photocell sensor reading
 * to the console if enabled.
 */
void DisplayLightSensorReading(double value)
{
  Serial.print(F("Light Sensor V:  "));
  Serial.print(value);
  Serial.println(F(" VDC "));
}

/**
 * DisplayLightSensorPercentage(double value)
 * Displays the photocell sensor reading
 * percentage to the console if enabled.
 */
void DisplayLightSensorPercentage(double value)
{
  Serial.print(F("Darkness %:      "));
  Serial.print(CalculatePercentageLight(value));
  Serial.println(F("%"));
}
