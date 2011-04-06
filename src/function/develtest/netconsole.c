/* netconsole.c -- run a console over LLC over ethernet */

#include <stdint.h>
#include <stdbool.h>

#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/timer.h>
#include <0cpm/led.h>

void top_main (void) {
	bottom_critical_region_end ();
	while (true) {
		TODO: CONSOLE DRIVER OVER LLC
	}
}

