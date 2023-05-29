/*
  Green House system:

  Every 24 hours (following startup), water is discharged by each pump in turn  
  Between each pump the resevoir is filled until the float sensor is triggered

  Main loop:
    1) We record a pre-pump level recording
    2) Switch on the pump to fill the grow bed
    3) When FILL_VOLUME is reached according to the pressure level sensor
    4) We then wait until the next fill cycle and repeat

*/

#include        <Wire.h>
#include        "Adafruit_MPRLS.h"
#include        "Average.h"

#define         FLOAT_SENSOR_PIN              10
#define         RESEVOIR_PUMP_PIN             2
#define         PUMP_A_PIN                    3
#define         PUMP_B_PIN                    4
#define         PUMP_C_PIN                    7
#define         PUMP_D_PIN                    8
#define         PUMP_E_PIN                    9

#define         LED_PIN                       13
#define         MPRLS_SDA                     A4
#define         MPRLS_CLK                     A5
                                     
#define         PUMP_TIMEOUT_TIME             300000                                 // We never run the pump for longer than this despite sensor readings 
#define         RESEVOIR_FILL_TIMEOUT_TIME    60
#define         PUMPING_PERIOD                86400000                               // 24 HOURS

#define         PUMP_OFF                      0
#define         PUMP_RUNNING                  1
#define         DEACTIVATED                   1

#define         FLOAT_SENSOR                  digitalRead(FLOAT_SENSOR_PIN)

#define         TURN_RESEVOIR_PUMP_ON         digitalWrite(RESEVOIR_PUMP_PIN, HIGH)
#define         TURN_RESEVOIR_PUMP_OFF        digitalWrite(RESEVOIR_PUMP_PIN, LOW)
#define         TURN_PUMP_A_ON                digitalWrite(PUMP_A_PIN, HIGH)
#define         TURN_PUMP_A_OFF               digitalWrite(PUMP_A_PIN, LOW)
#define         TURN_PUMP_B_ON                digitalWrite(PUMP_B_PIN, HIGH)
#define         TURN_PUMP_B_OFF               digitalWrite(PUMP_B_PIN, LOW)
#define         TURN_PUMP_C_ON                digitalWrite(PUMP_C_PIN, HIGH)
#define         TURN_PUMP_C_OFF               digitalWrite(PUMP_C_PIN, LOW)
#define         TURN_PUMP_D_ON                digitalWrite(PUMP_D_PIN, HIGH)
#define         TURN_PUMP_D_OFF               digitalWrite(PUMP_D_PIN, LOW)
#define         TURN_PUMP_E_ON                digitalWrite(PUMP_E_PIN, HIGH)
#define         TURN_PUMP_E_OFF               digitalWrite(PUMP_E_PIN, LOW)

Adafruit_MPRLS  mpr = Adafruit_MPRLS(-1, -1);
Average         Pressure_Filter(20);

int Fill_Resevoir_Tank();
int Read_Water_Level();
void Turn_Pumps_Off();
unsigned long Calculate_Next_Fill_Time(float B_V);
void Turn_Pump_On(int Pump_Selection);

int TARGET_FILL_VOLUME[5] = {500, 500, 100, -100, -100};                // Negative numbers skip

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
  static unsigned long    Pump_Toggle_Time        = millis(); // + PUMPING_PERIOD; 
  static unsigned long    Display_Time            = 0;
  static int              Pre_Pump_Water_Volume   = 0;
  static byte             Pump_Status             = PUMP_OFF;

  static int              Pump_Selection          = 0;

  long Time_Remaining = Pump_Toggle_Time - millis();

  if(Time_Remaining <= 0)                                                                                 // If it's time to toggle the pump
  {   
    if(Pump_Status == PUMP_OFF)                                                                           // If pump is currently off, turn it on for PUMP_TIMEOUT_TIME milliseconds
    {            
      Pre_Pump_Water_Volume = Fill_Resevoir_Tank();
      Turn_Pump_On(Pump_Selection);
      Pump_Status = PUMP_RUNNING;

      Pump_Toggle_Time = millis() + PUMP_TIMEOUT_TIME;
      Serial.print("Pump "); Serial.print(Pump_Selection); Serial.print(" running. Pre-pump Pressure: "); Serial.println(Pre_Pump_Water_Volume);
    }     
    else                                                                                                  // Fault condition. We shouldn't have reached the PUMP_TIMEOUT_TIME
    {      
      Turn_Pumps_Off();
      Pump_Status = PUMP_OFF;
      
      //Fault(PUMPING_TIMEOUT_FAULT);
      Serial.println("FAULT! Pump timeout exceeded");
      Pump_Selection++;
    }
  }  

  int Resevoir_Water_Volume = Read_Water_Volume();
  int Pumped_Water_Volume = Pre_Pump_Water_Volume - Resevoir_Water_Volume;
  if(Pump_Status == PUMP_RUNNING and Pumped_Water_Volume > TARGET_FILL_VOLUME[Pump_Selection])            // We have pumped the target volume of water into the grow tank 
  {
    Turn_Pumps_Off();
    Pump_Status = PUMP_OFF;
      
    Serial.println("Finished. Turning pump off.");
    Pump_Toggle_Time = millis();
    Pump_Selection++; 
  }

  if(Pump_Selection > 5)
  {
    Pump_Selection = 0;
    Pump_Toggle_Time = millis() + PUMPING_PERIOD;
  }  


  // -------  
  long Next_Display_Time = Display_Time - millis();
  if(Next_Display_Time <= 0)
  {
    Display_Time = millis() + 1000;
        
    digitalWrite(LED_PIN, HIGH);
      
    Serial.print("Time: "); Serial.print(Time_Remaining / 1000); Serial.print(",");
    Serial.print(Resevoir_Water_Volume); Serial.print(","); Serial.println(Pumped_Water_Volume);

    digitalWrite(LED_PIN, LOW);
  }
}

//---------------------------------------------------------------
int Fill_Resevoir_Tank()
{
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
  delay(1000);                                                                  // Short delay to ensure no water flowing anymore
  
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

