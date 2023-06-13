/*
  James Fotherby June 2023
  MIT Liscence 

  Greenhouse watering system:
  This simple system waters up to 5 different plant pots with a precise volume of water.

  Hardware:
   - A 20L water tank with 5 small 12V submersible pumps in it each one with tubing to a separate plant pot
   - A pressure sensor (Adafruit's MPRLS Ported Pressure Sensor) with an open ended tube weighted to that it sits submerged near the bottom of the water tank
   - A float sensor near the top of the water tank
   - A 6th submerged 12V pump in a large water butt which fills the 20L water tank when switched on.

  Software:
   - Every 12 hours (following startup) a watering cycle commences:
   - -> The water butt pump is switched on until the 20L water tank is full according to the float switch
   - -> Once the water tank is full a pressure reading is taken (Pre_Pump_Water_Volume)
   - -> Pump 1 is switched on until the pressure falls by a specified amount (equating to a calibrated volume of water)
   - -> Once the correct volume of water has been squirted into the plant pot Pump 2, 3, 4, 5 carry out the same process.

   - There are a few failsafes to prevent pumps running dry endlessly:
     1) The water butt pump will only run for RESEVOIR_FILL_TIMEOUT_TIME whilst attempting to fill the 20L tank - this is normally plenty of time
     2) Each pump is only allowed PUMP_TIMEOUT_TIME to fill it's respective plant pot - again should be plenty of time
     3) The software completely relies on the pressure sensor so if that's not detected the software hangs at startup and rapidly blinks the LED

   - An LED is used to indicate when the next watering cycle is going to occur
     -> The length of the on vs off time relates to the time left until the next watering cycle. A short flash therefore means watering is soon to occur!
*/

#include <Wire.h>
#include "Adafruit_MPRLS.h"
#include "Average.h"

#define               FLOAT_SENSOR_PIN              10
#define               RESEVOIR_PUMP_PIN             2
#define               PUMP_A_PIN                    3
#define               PUMP_B_PIN                    4
#define               PUMP_C_PIN                    7
#define               PUMP_D_PIN                    8
#define               PUMP_E_PIN                    9

#define               LED_PIN                       13
#define               MPRLS_SDA                     A4
#define               MPRLS_CLK                     A5
                                    
#define               PUMP_OFF                      0
#define               PUMP_RUNNING                  1
#define               DEACTIVATED                   1

#define               FLOAT_SENSOR                  digitalRead(FLOAT_SENSOR_PIN)

#define               TURN_RESEVOIR_PUMP_ON         digitalWrite(RESEVOIR_PUMP_PIN, HIGH)
#define               TURN_RESEVOIR_PUMP_OFF        digitalWrite(RESEVOIR_PUMP_PIN, LOW)
#define               TURN_PUMP_A_ON                digitalWrite(PUMP_A_PIN, HIGH)
#define               TURN_PUMP_A_OFF               digitalWrite(PUMP_A_PIN, LOW)
#define               TURN_PUMP_B_ON                digitalWrite(PUMP_B_PIN, HIGH)
#define               TURN_PUMP_B_OFF               digitalWrite(PUMP_B_PIN, LOW)
#define               TURN_PUMP_C_ON                digitalWrite(PUMP_C_PIN, HIGH)
#define               TURN_PUMP_C_OFF               digitalWrite(PUMP_C_PIN, LOW)
#define               TURN_PUMP_D_ON                digitalWrite(PUMP_D_PIN, HIGH)
#define               TURN_PUMP_D_OFF               digitalWrite(PUMP_D_PIN, LOW)
#define               TURN_PUMP_E_ON                digitalWrite(PUMP_E_PIN, HIGH)
#define               TURN_PUMP_E_OFF               digitalWrite(PUMP_E_PIN, LOW)

const unsigned long   PUMP_TIMEOUT_TIME             = 300000;                                 // We never run the pump for longer than this despite sensor readings 
const unsigned long   RESEVOIR_FILL_TIMEOUT_TIME    = 180;
const unsigned long   PUMPING_PERIOD                = 43200000;                               // 12 HOURS watering cycles
const unsigned long   DISPLAY_PERIOD                = 1000;
const unsigned long   LED_PERIOD                    = 4000;

Adafruit_MPRLS  mpr = Adafruit_MPRLS(-1, -1);
Average         Pressure_Filter(20);

int Fill_Resevoir_Tank(int Pump_Selection);
int Read_Water_Level();
void Turn_Pumps_Off();
unsigned long Calculate_Next_Fill_Time(float B_V);
void Turn_Pump_On(int Pump_Selection);

int TARGET_FILL_VOLUME[5] = {150, 150, 150, 0, 0};                                            // 125 units corresponds to 1 litre
unsigned long TimeStamp_Start_of_Pumping_Session = 0;

//---------------------------------------------------------------
void setup() 
{                 
  Serial.begin(115200);
  delay(1000);
  
  pinMode(RESEVOIR_PUMP_PIN, OUTPUT); 
  digitalWrite(RESEVOIR_PUMP_PIN, LOW); 
  pinMode(PUMP_A_PIN, OUTPUT); 
  digitalWrite(PUMP_A_PIN, LOW);  
  pinMode(PUMP_B_PIN, OUTPUT); 
  digitalWrite(PUMP_B_PIN, LOW); 
  pinMode(PUMP_C_PIN, OUTPUT); 
  digitalWrite(PUMP_C_PIN, LOW); 
  pinMode(PUMP_D_PIN, OUTPUT); 
  digitalWrite(PUMP_D_PIN, LOW); 
  pinMode(PUMP_E_PIN, OUTPUT); 
  digitalWrite(PUMP_E_PIN, LOW);  
    
  pinMode(FLOAT_SENSOR_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  
  if (!mpr.begin()) {
    Serial.println("Failed to communicate with MPRLS sensor, check wiring?");
    while (1) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(100);
    }
  }
  Serial.println("Found MPRLS sensor");
}

//---------------------------------------------------------------
void loop() 
{
  static unsigned long    PreviousMillis_PTT      = 0; 
  static unsigned long    PreviousMillis_DT       = 0;
  static unsigned long    PreviousMillis_LEDTT    = 0;
  static unsigned long    Pumping_Interval        = PUMPING_PERIOD;
  static unsigned long    LED_Flash_Period        = LED_PERIOD;

  static int              Pump_Selection          = 0;
  static int              Pre_Pump_Water_Volume   = 0;
  static int              Pump_Status             = PUMP_OFF;

  unsigned long CurrentMillis_PTT = millis();
  if((unsigned long)(CurrentMillis_PTT - PreviousMillis_PTT) >= Pumping_Interval)                         // If it's time to toggle the pump                                                                                
  {   
    if(Pump_Status == PUMP_OFF)                                                                           // If pump is currently off, turn it on for PUMP_TIMEOUT_TIME milliseconds
    {            
      Pre_Pump_Water_Volume = Fill_Resevoir_Tank(Pump_Selection);
      Turn_Pump_On(Pump_Selection);
      Pump_Status = PUMP_RUNNING;

      CurrentMillis_PTT = millis();
      PreviousMillis_PTT = CurrentMillis_PTT;                                                                                                                 
      Pumping_Interval = PUMP_TIMEOUT_TIME;

      Serial.print("Pump "); Serial.print(Pump_Selection); Serial.print(" running. Pre-pump Pressure: "); Serial.println(Pre_Pump_Water_Volume);
    }     
    else                                                                                                  // Fault condition. We're here because of PUMP_TIMEOUT_TIME
    {      
      Turn_Pumps_Off();
      Pump_Status = PUMP_OFF;
      
      //Fault(PUMPING_TIMEOUT_FAULT);
      Serial.println("FAULT! Pump timeout exceeded");
      Pump_Selection++;                                                                                   // Move on to trying the next pump
    }
  }  

  int Resevoir_Water_Volume = Read_Water_Volume();
  int Pumped_Water_Volume = Pre_Pump_Water_Volume - Resevoir_Water_Volume;
  if(Pump_Status == PUMP_RUNNING and Pumped_Water_Volume > TARGET_FILL_VOLUME[Pump_Selection])            // We have pumped the target volume of water into the grow tank 
  {
    Turn_Pumps_Off();
    Pump_Status = PUMP_OFF;
      
    Serial.println("Finished. Turning pump off.");
    Pump_Selection++; 
    Pumping_Interval = 0;                                                                                 // Ends this pumping period and will start the next
  }

  while(Pump_Selection <= 5 && TARGET_FILL_VOLUME[Pump_Selection] <= 0)  {
    Pump_Selection++;
  }

  if(Pump_Selection > 5)
  {
    Pump_Selection = 0;
    Pumping_Interval = PUMPING_PERIOD; 
    PreviousMillis_PTT = TimeStamp_Start_of_Pumping_Session;                                              // Want the pumping to start at the same time each day
  }  

  // ------- Each second we Tx data over UART in case anyone is listening
  unsigned long CurrentMillis_DT = millis();
  if((unsigned long)(CurrentMillis_DT - PreviousMillis_DT) >= DISPLAY_PERIOD)
  {
    PreviousMillis_DT = CurrentMillis_DT;

    unsigned long Time_Remaining = Pumping_Interval - (unsigned long)(CurrentMillis_PTT - PreviousMillis_PTT);

    Serial.print(Time_Remaining / 1000); Serial.print("s, "); 
    Serial.print(Resevoir_Water_Volume);
    
    if(Pump_Status == PUMP_RUNNING)
    {
      Serial.print("hPa, "); Serial.print(Pumped_Water_Volume); 
      Serial.println("units"); 
    }
    else
    {
      Serial.println();
    }
  }

  // ------- This Routine flashes the onboard LED with a pulse width proportional to remaining time left until watering to give a visual indication (short blink = soon to water)
  unsigned long CurrentMillis_LEDTT = millis();
  if((unsigned long)(CurrentMillis_LEDTT - PreviousMillis_LEDTT) >= LED_Flash_Period)
  {
    PreviousMillis_LEDTT = CurrentMillis_LEDTT;

    unsigned long Time_Remaining = Pumping_Interval - (unsigned long)(CurrentMillis_PTT - PreviousMillis_PTT); 
    unsigned long Max_time = PUMPING_PERIOD / 1000;                                                       //Convert to seconds else overflows occur
    Time_Remaining /= 1000;
    
    if(digitalRead(LED_PIN))              
    {
      digitalWrite(LED_PIN, LOW);
      LED_Flash_Period = LED_PERIOD - map(Time_Remaining, 0, Max_time, 0, LED_PERIOD);
    }
    else
    {
      digitalWrite(LED_PIN, HIGH);
      LED_Flash_Period = map(Time_Remaining, 0, Max_time, 0, LED_PERIOD);
    }
  }
}

//---------------------------------------------------------------
int Fill_Resevoir_Tank(int Pump_Selection)
{
  if(Pump_Selection == 0)                                                                                 // Only fill the resevoir at the start (so liquid feed doesn't dilute)
  {
    TimeStamp_Start_of_Pumping_Session = millis();

    TURN_RESEVOIR_PUMP_ON;
    Serial.println("Filling resevoir tank... Timeout in: ");
    
    int Counter = RESEVOIR_FILL_TIMEOUT_TIME;
    while(FLOAT_SENSOR == DEACTIVATED)
    {
      delay(1000);
      
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      Serial.println(Counter);

      Counter--;
      if(Counter == 0)
      {
        //Fault(RESEVOIR_PUMP_TIMEOUT_FAULT);
        break;
      }
    }

    Turn_Pumps_Off();
    Serial.println("Resevoir should be full");
  }

  delay(10000);                                                                 // Delay to ensure no water flowing anymore
  
  int V;                                                                        // Read and take average of tank water volume
  for(int i = 0; i < 32; i++) {
    V = Read_Water_Volume();
  }

  return(V);
}

//---------------------------------------------------------------
int Read_Water_Volume()
{
  delay(50);
  float pressure_hPa = (mpr.readPressure() - 1000.0) * 100.0;
  int P = (int)pressure_hPa; 

  P = Pressure_Filter.Rolling_Average(P);

  return(P);
}

//---------------------------------------------------------------
void Turn_Pumps_Off()
{
  TURN_RESEVOIR_PUMP_OFF;
  TURN_PUMP_A_OFF;
  TURN_PUMP_B_OFF;
  TURN_PUMP_C_OFF;
  TURN_PUMP_D_OFF;
  TURN_PUMP_E_OFF;
}

//---------------------------------------------------------------
void Turn_Pump_On(int Pump_Selection)
{
  switch (Pump_Selection) {
    case 0: 
      TURN_PUMP_A_ON;
      break;
    case 1: 
      TURN_PUMP_B_ON;
      break;
    case 2:  
      TURN_PUMP_C_ON;
      break;
    case 3:  
      TURN_PUMP_D_ON;
      break;
    case 4:  
      TURN_PUMP_E_ON;
      break;
  } 
}

