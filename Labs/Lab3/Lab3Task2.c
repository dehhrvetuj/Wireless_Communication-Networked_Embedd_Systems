#include "contiki.h"
#include "sys/node-id.h"
#include "sys/log.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-sr.h"
#include "net/mac/tsch/tsch.h"
#include "net/routing/routing.h"
#include "dev/leds.h"
#define DEBUG DEBUG_PRINT
#include "net/ipv6/uip-debug.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_DBG

/*---------------------------------------------------------------------------*/
PROCESS(node_process, "RPL Node");
AUTOSTART_PROCESSES(&node_process);

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(node_process, ev, data)
{
  int is_coordinator;

  PROCESS_BEGIN();

  is_coordinator = 0;

  is_coordinator = (node_id == 1);

  if(is_coordinator) 
  {
    NETSTACK_ROUTING.root_start();
  }
  NETSTACK_MAC.on();

  {
    static struct etimer et;
    /* Print out routing tables every minute */
    etimer_set(&et, CLOCK_SECOND * 10);
    
	while(1) 
	{
        int num_routes = uip_ds6_route_num_routes();
		LOG_INFO("Routing entries: %u\n", num_routes);
        uip_ds6_route_t *route = uip_ds6_route_head();
      	
		if (!route)
		{
			if (NETSTACK_ROUTING.node_is_reachable())
			{
				/* Endpoint Node */
				leds_off(LEDS_ALL);
				leds_on(LEDS_RED);		
			}
			else
			{
				leds_off(LEDS_ALL);
			}
			
		}
		else
		{
			if (node_id == 1)
			{			
				/* Root Node */
				leds_off(LEDS_ALL);
				leds_on(LEDS_GREEN);
			}
			else
			{
				/* Intermediate Node */
				leds_off(LEDS_ALL);
				leds_on(LEDS_YELLOW); 
			}
			
			while(route) 
			{
				LOG_INFO("Route ");
				LOG_INFO_6ADDR(&route->ipaddr);
				LOG_INFO_(" via ");
				LOG_INFO_6ADDR(uip_ds6_route_nexthop(route));
				LOG_INFO_("\n");
				route = uip_ds6_route_next(route);
			}
			
		}
		
		
		// leds_off(LEDS_ALL);
		
		// if(!NETSTACK_ROUTING.node_is_reachable()) 
		// {
            // leds_off(LEDS_ALL);
        // } 
        // else if(is_coordinator) 
		// {
			// /* Root Node */
            // leds_on(LEDS_GREEN);
        // } 
        // else if(num_routes > 0) 
		// {
			// /* Intermediate Node */
            // leds_on(LEDS_YELLOW); 
        // } 
        // else if (num_routes == 0)
		// {
			// /* Endpoint Node */
            // leds_on(LEDS_RED);
        // }
		
		PROCESS_YIELD_UNTIL(etimer_expired(&et));
		etimer_reset(&et);
	}
	

  }

  PROCESS_END();
}
