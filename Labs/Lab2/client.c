#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "dev/sensor/sht11/sht11-sensor.h"

/* Declare our "main" process, the client process*/
PROCESS(client_process, "Clicker client");
/* The client process should be started automatically when
 * the node has booted. */
AUTOSTART_PROCESSES(&client_process);

/* Callback function for received packets.
 *
 * Whenever this node receives a packet for its broadcast handle,
 * this function will be called.
 *
 * As the client does not need to receive, the function does not do anything
 */
static void recv(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest) {
}

/*---------------------------------------------------------------------------*/
static struct etimer timer;
/*---------------------------------------------------------------------------*/

/* Our main process. */
PROCESS_THREAD(client_process, ev, data) {
	
   static char payload = '0';
   static int16_t ALARM_LEVEL = 3000;
	static int16_t OFFSET = 3960;
	int16_t temperature;

	PROCESS_BEGIN();
   
   /* Activate the temperature sensor. */
   SENSORS_ACTIVATE(sht11_sensor);

	/* Activate the button sensor. */
	SENSORS_ACTIVATE(button_sensor);

	/* Initialize NullNet */
	nullnet_buf = (uint8_t *)&payload;
	nullnet_len = sizeof(payload);
	nullnet_set_input_callback(recv);

	/* Loop forever. */
	while (1) {
		/* Wait until an event occurs. If the event has
		 * occured, ev will hold the type of event, and
		 * data will have additional information for the
		 * event. In the case of a sensors_event, data will
		 * point to the sensor that caused the event.
		 * Here we wait until the button was pressed. */
      
      etimer_set(&timer, CLOCK_SECOND/100);
      

      PROCESS_WAIT_EVENT();
      
      if (ev == sensors_event &&
			data == &button_sensor) {
         
         payload = 'B';
         
         /* Copy the string "hej" into the packet buffer. */
         memcpy(nullnet_buf, &payload, sizeof(payload));
         nullnet_len = sizeof(payload);

         /* Send the content of the packet buffer using the
          * broadcast handle. */
         NETSTACK_NETWORK.output(NULL);
      }
      
      if (ev == PROCESS_EVENT_TIMER && data == &timer) {
         
         leds_toggle(LEDS_RED);
      
         temperature = sht11_sensor.value(SHT11_SENSOR_TEMP) - OFFSET;
         
         printf("Temperature %02d.%02d ºC\n ", temperature / 100, temperature % 100);
         
         if (temperature >= ALARM_LEVEL) {
            
            printf("Temperature ALARM\n");
            
            payload = 'A';
            
            /* Copy the string "hej" into the packet buffer. */
            memcpy(nullnet_buf, &payload, sizeof(payload));
            nullnet_len = sizeof(payload);

            /* Send the content of the packet buffer using the
             * broadcast handle. */
            NETSTACK_NETWORK.output(NULL);
         }
         
      }
      
	}

	PROCESS_END();
}
