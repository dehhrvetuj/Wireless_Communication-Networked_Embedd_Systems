#define NEW_PRINTF_SEMANTICS
#include "printf.h"


configuration T1AppC
{
}
implementation
{
  components MainC, T1C, LedsC;

  T1C -> MainC.Boot;

  T1C.Leds -> LedsC;
  
  components PrintfC;
  components SerialStartC;


  components new TimerMilliC() as TimerTemp;
	T1C.TimerTemp -> TimerTemp;

  components new SensirionSht11C() as TempSensor;
	T1C.Temperature -> TempSensor.Temperature; 

}

