/* timer2led.c -- simple test where a timer toggles a LED (T=1s, D=50%) */

#include <stdint.h>
#include <stdbool.h>

#define BOTTOM
#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/timer.h>
#include <0cpm/led.h>


#define complete_top_main top_main


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
			// nextirq = bottom_time () + TIME_MSEC(500);
			nextirq += TIME_MSEC(500);
		}
	}
}


void irqpolling_top_main (void) {
	timing_t nextirq;
redo:
	nextirq = bottom_time () + TIME_MSEC(1500);
bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_ON);
{ uint16_t ctr = 65000; while (ctr > 0) ctr--; }
bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_OFF);
{ uint16_t ctr = 65000; while (ctr > 0) ctr--; }
	bottom_timer_set (nextirq);
bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_ON);
{ uint16_t ctr = 65000; while (ctr > 0) ctr--; }
bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_OFF);
{ uint16_t ctr = 65000; while (ctr > 0) ctr--; }
	while (true) {
#ifdef INCLUDE_BOTTOM_IRQ_ACCESS_HALFWAY_DEVELOPMENT_HACK
		if ("irq_flag_is_set") {
			"clear_irq_flag";
			bottom_timer_expiration (bottom_time () + TIME_MSEC(1500));
		}
#else
		if ((GPTCNT1_0==0) && (GPTCNT2_0==0)) {
			IFR0 = (1 << REGBIT_IER0_TINT0);
			// bottom_timer_expiration (bottom_time () + TIME_MSEC(1500));
			bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_OFF);
			goto done;
		} else {
			uint16_t ctr;
			uint16_t ctr1a, ctr1b;
			ctr1a = GPTCNT1_0;
			ctr1b = GPTCNT2_0;
			bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_OFF);
			ctr = 65000;
			while (ctr > 0) {
				ctr--;
			}
			if ((ctr1a != GPTCNT1_0) || (ctr1b != GPTCNT2_0)) {
				bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_ON);
				ctr = 65000;
				while (ctr > 0) {
					ctr--;
				}
			}
		}
#endif
	}
done:
while (!bottom_phone_is_offhook ()) {
	;
}
while (bottom_phone_is_offhook ()) {
	;
}
goto redo;
}


volatile bool busy = false;

void top_timer_expiration (timing_t exptime) {
	uint16_t ctr;
	// static led_colour_t nextstate = LED_STABLE_OFF;
	// nextstate ^= LED_STABLE_ON ^ LED_STABLE_OFF;
	// bottom_led_set (LED_IDX_MESSAGE, nextstate);
	bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_OFF);
	// ctr = 65000;
	// while (ctr > 0) {
	// 	ctr--;
	// }
	//ULTIMATELY// nextstate ^= LED_STABLE_ON ^ LED_STABLE_OFF;
	//ULTIMATELY// bottom_led_set (LED_IDX_MESSAGE, nextstate);
	// bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_ON);
	// bottom_timer_set (exptime + TIME_MSEC(1500));
}

void complete_top_main (void) {
	timing_t nextirq;
	bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_ON);
	nextirq = bottom_time () + TIME_MSEC(1500);
	busy = true;
	// bottom_timer_set (nextirq);
	// bottom_critical_region_end ();
	while (true) {
		if (busy) {
			bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_ON );
		} else {
			bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_OFF);
		}
	}
}

