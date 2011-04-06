/* Grandstream BT20x driver as an extension to the tic55x driver */


#include <stdbool.h>
#include <stdint.h>

#define BOTTOM
#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/timer.h>
#include <0cpm/led.h>


/* Bottom-half operations to manipulate LED states */
void bottom_led_set (led_idx_t ledidx, led_colour_t col) {
	switch (ledidx) {
	case LED_IDX_MESSAGE:
		//TODO: Figure out which pin -- incorrectly change backlight
	case LED_IDX_BACKLIGHT:
		// Set bit DXSTAT=5 in PCR=0x2812 to 1/0
		if (col != 0) {
			PCR0 |=     1 << REGBIT_PCR_DXSTAT  ;
		} else {
			PCR0 &= ~ ( 1 << REGBIT_PCR_DXSTAT );
		}
		break;
	default:
		break;
	}
}


/* See if the phone (actually, the horn) is offhook */
bool bottom_phone_is_offhook (void) {
	// The hook switch is attached to GPIO pin 5
	// return ! tic55x_input_get_bit (REGADDR_IODATA, 5);
	return (IODATA & (1 << 5)) != 0;
}


/* Setup the connectivity of the TIC55x as used on Grandstream BT20x */
void main (void) {
	led_colour_t led = LED_STABLE_ON;
	bottom_critical_region_begin (); // _disable_interrupts ();
	IER0 = IER1 = 0x0000;
	tic55x_setup_timers ();
	tic55x_setup_interrupts ();
	//TODO// IER0 = 0xdefc;
	//TODO// IER1 = 0x00ff;
	PCR0 = (1 << REGBIT_PCR_XIOEN) | (1 << REGBIT_PCR_RIOEN);
#if 0
	while (true) {
		uint32_t ctr;
		bottom_led_set (LED_IDX_MESSAGE, led);
		for (ctr=6500000; ctr>0; ctr--) { ; }
		led ^= LED_STABLE_ON ^ LED_STABLE_OFF;
	}
#endif
	top_main ();
}

