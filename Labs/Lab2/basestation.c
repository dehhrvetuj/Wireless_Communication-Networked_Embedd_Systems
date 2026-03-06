#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "contiki.h"
#include "dev/leds.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"

/* Declare all processes involved in the system */
PROCESS(basestation_process, "Clicker basestation");
PROCESS(led_process, "LED Control process");
PROCESS(temp_process, "Temp timer process");
PROCESS(button_process, "Button timer process");

/* Start all processes automatically on bootup */
AUTOSTART_PROCESSES(&basestation_process, &led_process, &temp_process, &button_process);

/* Global state variables shared across processes */
static int count = 0;
static bool temp_triggered = false;
static bool button_triggered = false;

/*---------------------------------------------------------------------------*/
/* Callback function for received packets */
static void recv(const void *data, uint16_t len,
                 const linkaddr_t *src, const linkaddr_t *dest) {
  count++;

  if (len > 0) {
    char msg = *((char *) data);

    /* Update global flags and poll respective timer processes based on msg type */
    switch (msg) {
      case 'A':
        temp_triggered = true;
        process_poll(&temp_process);   // Refresh/Start temperature timer
        break;
      case 'B':
        button_triggered = true;
        process_poll(&button_process); // Refresh/Start button timer
        break;
    }
  }
  
   /* Trigger the LED process to update the physical display immediately */
    process_poll(&led_process);
}

/*---------------------------------------------------------------------------*/
/* Main process: Initializes networking and callback registration */
PROCESS_THREAD(basestation_process, ev, data) {
  PROCESS_BEGIN();

  /* Register the NullNet input callback */
  nullnet_set_input_callback(recv);

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
/* Temperature Timeout Process */
PROCESS_THREAD(temp_process, ev, data) {
  static struct etimer timer;
  PROCESS_BEGIN();
  
  while(1) {
    /* Wait for the first signal to start timing */
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
    
    /* Keep checking/refreshing as long as it's triggered */
    while(temp_triggered) {
      etimer_set(&timer, CLOCK_SECOND * 10);
      
      /* Crucial: Wait for timer OR new packet poll */
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer) || ev == PROCESS_EVENT_POLL);
      
      if(etimer_expired(&timer)) {
        temp_triggered = false;
        process_poll(&led_process); // Notify LED process to turn off
        printf("Temperature Alarm Timeout - Client possibly offline\n");
      }
      /* If ev == PROCESS_EVENT_POLL, the while loop will re-run etimer_set */
    }
  }
  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
/* Button Timeout Process */
PROCESS_THREAD(button_process, ev, data) {
  static struct etimer timer;
  PROCESS_BEGIN();
  
  while(1) {
    /* Wait for the first button press */
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
    
    while(button_triggered) {
      etimer_set(&timer, CLOCK_SECOND * 10);
      
      /* Wait for 10s or another button press */
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer) || ev == PROCESS_EVENT_POLL);
      
      if(etimer_expired(&timer)) {
        button_triggered = false;
        process_poll(&led_process);
        printf("Button Timeout\n");
      }
    }
  }
  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
/* LED control process: Synchronizes physical LEDs with current logic states */
PROCESS_THREAD(led_process, ev, data) {
  PROCESS_BEGIN();

  while(1) {
    /* Wait here until any status changes (new packet or timeout) */
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

    printf("LED process awakened by poll!\n");
    
    /* Update Individual LEDs */
    if (temp_triggered) leds_on(0b0001);
    else leds_off(0b0001);
    
    if (button_triggered) leds_on(0b0010);
    else leds_off(0b0010);
    
    /* Handle Dual-Trigger Alarm (Task 2) */
    if (temp_triggered && button_triggered) leds_on(0b0100);
    else leds_off(0b0100);
  }

  PROCESS_END();
}