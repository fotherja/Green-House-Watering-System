# Green-House-Watering-System

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


   
