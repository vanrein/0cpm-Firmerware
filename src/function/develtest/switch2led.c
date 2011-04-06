/* switch2led.c -- simple test where hook contact drives message led */

#include <stdint.h>
#include <stdbool.h>

#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/timer.h>
#include <0cpm/led.h>


#define complete_top_main top_main


void top_timer_expiration (timing_t when) { ; }


void simpler_top_main (void) {
	while (true) {
		uint16_t ctr;
		ctr = 65000;
		while (ctr > 0) {
			ctr--;
		}
		bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_ON );
		ctr = 65000;
		while (ctr > 0) {
			ctr--;
		}
		bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_OFF);
	}
}


void complete_top_main (void) {
	while (true) {
		if (bottom_phone_is_offhook ()) {
			bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_ON );
		} else {
			bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_OFF);
		}
	}
}

