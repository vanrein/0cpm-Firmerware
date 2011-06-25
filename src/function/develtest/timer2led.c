/* timer2led.c -- simple test where a timer toggles a LED (T=1s, D=50%)
 *
 * This file is part of 0cpm Firmerware.
 *
 * 0cpm Firmerware is Copyright (c)2011 Rick van Rein, OpenFortress.
 *
 * 0cpm Firmerware is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 3.
 *
 * 0cpm Firmerware is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0cpm Firmerware.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdint.h>
#include <stdbool.h>

#define BOTTOM
#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/timer.h>
#include <0cpm/led.h>
#include <0cpm/kbd.h>


volatile led_colour_t ledstate = LED_STABLE_ON;
volatile timing_t nextirq;


#define snoozing_top_main top_main


void top_hook_update (bool offhook) {
	/* Keep the linker happy */ ;
}

void top_button_press (buttonclass_t bcl, buttoncode_t cde) {
	/* Keep the linker happy */ ;
}

void top_button_release (void) {
	/* Keep the linker happy */ ;
}

void top_network_online (void) {
	/* Keep the linker happy */ ;
}

void top_network_offline (void) {
	/* Keep the linker happy */ ;
}

void top_network_can_send (void) {
	/* Keep the linker happy */ ;
}

void top_network_can_recv (void) {
	/* Keep the linker happy */ ;
}

void top_can_play (uint16_t samples) {
	/* Keep the linker happy */ ;
}

void top_can_record (uint16_t samples) {
	/* Keep the linker happy */ ;
}


void doesitcount_top_main (void) {
	uint16_t ctr;
	timing_t lastcaught = bottom_time ();
	led_colour_t nextstate = LED_STABLE_OFF;
	while (true) {
		if (lastcaught != bottom_time ()) {
			lastcaught = bottom_time ();
			nextstate ^= LED_STABLE_ON ^ LED_STABLE_OFF;
			bottom_led_set (LED_IDX_MESSAGE, nextstate);
			ctr = 65000;
			while (ctr > 0) {
				ctr--;
			}
		}
	}
}


void noirq_top_main (void) {
	led_colour_t nextstate = LED_STABLE_OFF;
	timing_t nextirq = bottom_time ();
	while (true) {
		if (TIME_BEFORE (nextirq, bottom_time ())) {
			nextstate ^= LED_STABLE_ON ^ LED_STABLE_OFF;
			bottom_led_set (LED_IDX_MESSAGE, nextstate);
			nextirq += TIME_MSEC(500);
		}
	}
}


void top_timer_expiration (timing_t exptime) {
	uint16_t ctr;
	ledstate ^= LED_STABLE_ON ^ LED_STABLE_OFF;
	nextirq += TIME_MSEC(500);
	bottom_timer_set (nextirq);
}


void irqpolling_top_main (void) {
	nextirq = bottom_time () + TIME_MSEC(500);
	bottom_timer_set (nextirq);
	while (true) {
		bottom_led_set (LED_IDX_MESSAGE, ledstate);
#ifdef INCLUDE_BOTTOM_IRQ_ACCESS_HALFWAY_DEVELOPMENT_HACK
		if ("irq_flag_is_set_AND_can_be_polled_with_certainty") {
			"clear_irq_flag";
			top_timer_expiration (nextirq);
		}
#else
		// your code here... if you think this is a useful step
#endif
	}
}

void interrupted_top_main (void) {
	nextirq = bottom_time () + TIME_MSEC(500);
	bottom_timer_set (nextirq);
	bottom_critical_region_end ();
	while (true) {
		bottom_led_set (LED_IDX_MESSAGE, ledstate);
	}
}

void snoozing_top_main (void) {
	led_colour_t curcol = LED_STABLE_OFF;
	nextirq = bottom_time () + TIME_MSEC(500);
	bottom_timer_set (nextirq);
	bottom_critical_region_end ();
	while (true) {
		bottom_sleep_prepare ();
		if (curcol == ledstate) {
			bottom_sleep_commit (SLEEP_SNOOZE);
		}
		curcol = ledstate;
		bottom_led_set (LED_IDX_MESSAGE, curcol);
	}
}

void complete_top_main (void) {
	led_colour_t curcol = LED_STABLE_OFF;
	nextirq = bottom_time () + TIME_MSEC(500);
	bottom_timer_set (nextirq);
	bottom_critical_region_end ();
	while (true) {
		bottom_sleep_prepare ();
		if (curcol == ledstate) {
			bottom_sleep_commit (SLEEP_HIBERNATE);
		}
		curcol = ledstate;
		bottom_led_set (LED_IDX_MESSAGE, curcol);
	}
}

