#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"

/* --- Task 1 Configuration --- */
#define MAX_NUMBER_OF_EVENTS 3
#define ALARM_TIMEOUT (30 * CLOCK_SECOND)

struct event 
{
    clock_time_t time;
    linkaddr_t addr;
};

/* History of recent neighbor clicks */
static struct event event_history[MAX_NUMBER_OF_EVENTS];
static int history_count = 0;

/*---------------------------------------------------------------------------*/
PROCESS(clicker_ng_process, "Clicker NG Process");
AUTOSTART_PROCESSES(&clicker_ng_process);
/*---------------------------------------------------------------------------*/

/**
 * Core Logic: Handles both local and remote button click events.
 */
void handle_event(const linkaddr_t *src) 
{
    int i = 0, j = 1;
    int distinct_nodes = 0;
    int node_id[MAX_NUMBER_OF_EVENTS];
    bool already_in_history = false;
    clock_time_t now = clock_time();

    /* 1. Check if the node is already in our history to update its time */
    for(i = 0; i < history_count; i++) 
    {
        if(linkaddr_cmp(&(event_history[i].addr), src)) 
        {
            event_history[i].time = now;
            already_in_history = true;
            break;
        }
    }

    /* 2. If it's a new node, add it to the history */
    if(!already_in_history) 
    {
        if(history_count < MAX_NUMBER_OF_EVENTS) 
        {
            linkaddr_copy(&(event_history[history_count].addr), src);
            event_history[history_count].time = now;
            history_count++;
        } 
        else 
        {
            /* History full: Replace the oldest entry */
            int oldest = 0;
            for(j = 1; j < MAX_NUMBER_OF_EVENTS; j++) 
            {
                if(event_history[j].time < event_history[oldest].time) 
                {
                    oldest = j;
                }
            }
            linkaddr_copy(&(event_history[oldest].addr), src);
            event_history[oldest].time = now;
        }
    }

    /* 3. Count how many distinct nodes are active within the 30s window */
    for(i = 0; i < history_count; i++) 
    {
        if(now - event_history[i].time <= ALARM_TIMEOUT) 
        {
            distinct_nodes++;
            node_id[i] = event_history[i].addr.u8[0];
        }
    }

    /* 4. Trigger Alarm if condition met */
    if(distinct_nodes >= 3) 
    {
        printf("ALARM: 3 distinct nodes (%d, %d, %d) detected!\n", node_id[0], node_id[1], node_id[2]);
        
        leds_on(LEDS_YELLOW);
    }
}

/* Callback function for receiving NullNet packets */
static void recv(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest) 
{
    printf("Received: %s - from %d\n", (char*) data, src->u8[0]);
    leds_toggle(LEDS_GREEN);
    handle_event(src);
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(clicker_ng_process, ev, data)
{
    static char payload[] = "hej";
    static struct etimer periodic_timer; 
    int i = 0;

    PROCESS_BEGIN();

    /* Initialize NullNet */
    nullnet_buf = (uint8_t *)&payload;
    nullnet_len = sizeof(payload);
    nullnet_set_input_callback(recv);
    
    /* Activate the button sensor. */
    SENSORS_ACTIVATE(button_sensor);
    
    /* Set a periodic timer to check for alarm reset every second */
    etimer_set(&periodic_timer, CLOCK_SECOND);

    while(1) 
    {
        PROCESS_WAIT_EVENT();
        if (ev == sensors_event && data == &button_sensor)
        {      
			printf("Button on node %d is clicked\n", linkaddr_node_addr.u8[0]);
			
			leds_toggle(LEDS_RED);
			handle_event(&linkaddr_node_addr);
			
			memcpy(nullnet_buf, &payload, sizeof(payload));
			nullnet_len = sizeof(payload);

			/* Send the content of the packet buffer */
			NETSTACK_NETWORK.output(NULL);
        }
         
        /* Periodic check logic below... */
        if (etimer_expired(&periodic_timer)) 
        {
            clock_time_t now = clock_time();
            int active_recent = 0;
            
            for(i = 0; i < history_count; i++) 
            {
                if(now - event_history[i].time <= ALARM_TIMEOUT) 
                {
                    active_recent++;
                }
            }

            if(active_recent < 3) 
            {
                leds_off(LEDS_YELLOW);
            }
            etimer_reset(&periodic_timer);
        }
    }

    PROCESS_END();
}
