/* keys2display.c -- capture keys and show them on the display */

#include <stdint.h>
#include <stdbool.h>

#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/timer.h>
#include <0cpm/led.h>

void top_main (void) {
	bottom_critical_region_end ();
	while (true) {
		TODO: KEYBOARD AND DISPLAY DRIVING CODE
	}
}

