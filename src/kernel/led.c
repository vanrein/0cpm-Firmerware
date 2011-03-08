/*
 * http://devel.0cpm.org/ -- Open source firmware for SIP phones.
 *
 * LED drivers
 */


#include <config.h>

#include <stdint.h>
#include <stdbool.h>

#include <0cpm/cpu.h>
#include <0cpm/irq.h>
#include <0cpm/timer.h>
#include <0cpm/led.h>


/* Refer to the top handler state storage structures for LED information.
 * The size of the array is LED_IDX_COUNT as derived above.
 */
struct led_state {
	irqtimer_t	led_timer;
        led_colour_t    led_colour;
        led_flashtime_t led_flashtime;
};

static struct led_state leds [LED_IDX_COUNT];


/* LED interrupts will change the state of a LED.
 */
static bool led_irq (irq_t *irq) {
	struct led_state *this = (struct led_state *) irq;
	led_idx_t ledidx = (this - leds) / sizeof (struct led_state);	// TODO: CALC IS GENERAL?
	this->led_colour = LED_FLASH_FLIP (this->led_colour);
	bottom_led_set (ledidx, LED_FLASHING_CURRENT (this->led_colour));
	irqtimer_restart (&this->led_timer, this->led_flashtime);
	return true;
}



/* Set the state of a LED.  If the current colour matches the initial colour of a
 * flashing LED, that a flash operation is immediately performed.
 */
void led_set (led_idx_t ledidx, led_colour_t col, led_flashtime_t ft) {
	if (leds [ledidx].led_flashtime != LED_FLASHTIME_NONE) {
		irqtimer_stop (&leds [ledidx].led_timer);
	}
	if (ft == LED_FLASHTIME_NONE) {
		col = LED_STABLE (col);
	} else if (LED_FLASHING_CURRENT (leds [ledidx].led_colour) == LED_FLASHING_CURRENT (col)) {
		col = LED_FLASH_FLIP (col);
	}
	leds [ledidx].led_colour = col;
	leds [ledidx].led_flashtime = ft;
	bottom_led_set (ledidx, col);
	irqtimer_start (&leds [ledidx].led_timer, ft, led_irq, CPU_PRIO_LOW);
}


/* Get the colour currently shown on the LED.
 */
led_colour_t led_getcolour (led_idx_t ledidx) {
	return LED_FLASHING_CURRENT (leds [ledidx].led_colour);
}

/* Get the flashing time for a given LED.
 */
led_flashtime_t led_getflashtime (led_idx_t ledidx) {
	return leds [ledidx].led_flashtime;
}

