/* switch2led.c -- simple test where hook contact drives message led
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

#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/timer.h>
#include <0cpm/led.h>
#include <0cpm/kbd.h>

#include <0cpm/app.h>		//TODO:EXTENSION-OF-MINIMAL-TEST//
#include <0cpm/show.h>		//TODO:EXTENSION-OF-MINIMAL-TEST//

#define complete_top_main top_main


void top_timer_expiration (timing_t timeout) {
	/* Keep the linker happy */ ;
}

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


void simpler_top_main (void) {
	while (true) {
		uint16_t ctr;
		ctr = 65000;
		while (ctr > 0) {
			ctr--;
		}
		bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_ON );	//TODO:EXTENSION-OF-MINIMAL-TEST//
		ctr = 65000;
		while (ctr > 0) {
			ctr--;
		}
		bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_OFF);	//TODO:EXTENSION-OF-MINIMAL-TEST//
	}
}


void complete_top_main (void) {
	while (true) {
		if (bottom_phone_is_offhook ()) {
			bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_ON );
			bottom_show_fixed_msg (APP_LEVEL_BACKGROUNDED, FIXMSG_RINGING);
		} else {
			bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_OFF);
			bottom_show_fixed_msg (APP_LEVEL_BACKGROUNDED, FIXMSG_READY);
		}
	}
}

