/* A Simple Accelerometer Example
 *
 * Values only in the x-axis are detected in the following example.
 * For your Lab1, use extend the code for Y and Z axes.
 * Finally, interface them with a button so that the sensing starts onlt after the press of a button.
 *
 */
 
//#include "UserButton.h"
#include <Timer.h>
#include <stdio.h>
#include <string.h>
#include "printf.h"


module T1C @safe()
{
  	uses interface Leds;
  	uses interface Boot;

  	/* We use millisecond timer to check the shaking of client.*/
	uses interface Timer<TMilli> as TimerTemp;

  	/*Temperature Interface*/
	uses interface Read<uint16_t> as Temperature;
}


implementation
{
   int16_t lastTemp = -3000;
   int16_t currentTemp = -3000;
   int16_t ALARM_LEVEL = 2200;
   
    event void Boot.booted() 
    {
   		call TimerTemp.startPeriodic(100); //Starts timer

    }

	event void TimerTemp.fired()
	{
		if(call Temperature.read() == SUCCESS) //If temperature read was successful
		{
			/*call Leds.led2Toggle();*/
		}
		else
		{
			call Leds.led0Toggle();
		}
	}

    event void Temperature.readDone(error_t result, uint16_t data)
	{
		if(result == SUCCESS)
		{
         currentTemp = -3960 + ((int16_t) data);
         
         if (lastTemp < -2800)
         {
            lastTemp = currentTemp;
         }
         else
         {
            if(currentTemp >= ALARM_LEVEL)
            {
               call Leds.led2On();
            }
            else
            {
               call Leds.led2Off();
            }
            
            if (currentTemp > lastTemp)
            {
               call Leds.led0On();
               call Leds.led1Off();
            }
            else if (currentTemp < lastTemp)
            {
               call Leds.led1On();
               call Leds.led0Off();
            }
            else
            {
               call Leds.led0Off();
               call Leds.led1Off(); 
            }
            
            lastTemp = currentTemp;
         }
			printf("Current temp is: %d \r\n", currentTemp);
         printf("Last temp is: %d \r\n", lastTemp);
		}
		else
		{
			printf("Error reading from sensor! \r\n");
		}
		
		printfflush();
	}
}

