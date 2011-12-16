/* keys2display.c -- capture keys and show them on the display
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
#include <0cpm/app.h>
#include <0cpm/show.h>


timing_t top_timer_expiration (timing_t timeout) {
	/* Keep the linker happy */ ;
	return timeout;
}

void top_hook_update (bool offhook) {
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

void top_codec_can_play (uint8_t chan) {
	/* Keep the linker happy */ ;
}

void top_codec_can_record (uint8_t chan) {
	/* Keep the linker happy */ ;
}


#define complete_top_main top_main


buttonclass_t buttonclass = BUTCLS_NONE;
buttoncode_t  buttoncode  = BUTTON_NULL;

void top_button_press (buttonclass_t bcl, buttoncode_t cde) {
	bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_ON );
	buttonclass = bcl;
	buttoncode  = cde;
}

void top_button_release (void) {
	bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_OFF);
	buttonclass = BUTCLS_NONE;
	buttoncode  = BUTTON_NULL;
}



/* A simple preliminary test that switches on a LED for as long
 * as any key is pressed.  Note the brightness of the LED; if it
 * glows more dimly for the last row than the first, then the
 * LED is constantly being switched on and off, so there is a
 * continuous sequence of top_button_press() / top_button_release()
 * which may be the result of reading the matrix output too fast
 * after having selecting the matrix input.
 */
void keytoled_top_main (void) {
	bottom_critical_region_end ();
	while (true) {
#if defined NEED_KBD_SCANNER_BETWEEN_KEYS || defined NEED_KBD_SCANNER_DURING_KEYPRESS
		bottom_keyboard_scan ();
#endif
#if defined NEED_HOOK_SCANNER_WHEN_ONHOOK || defined NEED_HOOK_SCANNER_WHEN_OFFHOOK
		// bottom_hook_scan ();
#endif
	}
}


/* An experiment to show text describing the button being pressed
 * for as long as it is being pressed.  On any other times, it will
 * show how long the phone has been running.
 */
void keytodisplay_top_main (void) {
	bottom_critical_region_end ();
	bottom_show_fixed_msg (APP_LEVEL_ZERO, FIXMSG_OFFLINE);
	while (true) {
#if defined NEED_KBD_SCANNER_BETWEEN_KEYS || defined NEED_KBD_SCANNER_DURING_KEYPRESS
		bottom_keyboard_scan ();
#endif
		if (buttonclass == BUTCLS_NONE) {
			bottom_show_close_level (APP_LEVEL_BACKGROUNDED);
		} else {
			timing_t now = bottom_time () / TIME_SEC (1);
			uint8_t h, m, s;
			s = now % 60;
			now = now / 60;
			m = now % 60;
			h = now / 60;
			bottom_show_period (APP_LEVEL_BACKGROUNDED, h, m, s);
		}
	}
}


/* The complete application waits for digit keys being entered, and
 * enters them in an IPv4 number.
 * and, if need be, scrolls away older content.
 */
void complete_top_main (void) {
	int which = 0;
	uint8_t ip4 [4];
	int value = 0;
	bool processed = false;
	bottom_critical_region_end ();
	bottom_show_fixed_msg (APP_LEVEL_BACKGROUNDED, FIXMSG_READY);
	ip4 [0] = ip4 [1] = ip4 [2] = ip4 [3] = 0x00;
	while (true) {
#if defined NEED_KBD_SCANNER_BETWEEN_KEYS || defined NEED_KBD_SCANNER_DURING_KEYPRESS
		bottom_keyboard_scan ();
#endif
		if (buttonclass == BUTCLS_NONE) {
			processed = false;
		} else if (!processed) {
			if (buttonclass == BUTCLS_DTMF) {
				if ((buttoncode >= '0') && (buttoncode <= '9')) {
					value %= 100;
					value *= 10;
					value += buttoncode - '0';
					if (value > 255) {
						value %= 100;
					}
					ip4 [which] = value;
					bottom_show_ip4 (APP_LEVEL_DIALING, ip4);
				} else {
					which++;
					if (which == 4) {
						which = 0;
						bottom_show_close_level (APP_LEVEL_DIALING);
					}
					value = ip4 [which];
				}
			}
			processed = true;
		}
	}
}
