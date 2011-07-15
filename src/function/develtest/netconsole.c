/* netconsole.c -- run a console over LLC over ethernet
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


/* This test program includes a very small (and very selfish)
 * network stack: It only does LLC2, and welcomes one LAN peer
 * at a time to connect to SAP 20 where a console is running.
 *
 * Console log events are saved up in a buffer, and reported
 * as soon as the client connects.  From then on, new entries
 * sent to the console are also sent immediately.  An entry is
 * made for incoming broadcast messages.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/timer.h>
#include <0cpm/led.h>
#include <0cpm/kbd.h>
#include <0cpm/app.h>
#include <0cpm/show.h>
#include <0cpm/cons.h>


#define onlinetest_top_main top_main


bool online = false;


timing_t top_timer_expiration (timing_t timeout) {
	/* Keep the linker happy */ ;
	return timeout;
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
	online = true;
}

void top_network_offline (void) {
	online = false;
}

void top_network_can_send (void) {
	/* Keep the linker happy */ ;
}

void top_network_can_recv (void) {
	/* Keep the linker happy */ ;
}

void top_can_play (void) {
	/* Keep the linker happy */ ;
}

void top_can_record (void) {
	/* Keep the linker happy */ ;
}



uint8_t netinput [1000];
uint16_t netinputlen;

void onlinetest_top_main (void) {
	void nethandler_llconly (uint8_t *pkt, uint16_t pktlen);
	bottom_critical_region_end ();
	while (true) {
		if (online) {
			bottom_show_fixed_msg (APP_LEVEL_BACKGROUNDED, FIXMSG_READY);
			netinputlen = sizeof (netinput);
			if (bottom_network_recv (netinput, &netinputlen)) {
				if (memcmp (netinput, "\xff\xff\xff\xff\xff\xff", 6) == 0) {
					bottom_printf ("Broadcast from %2x:%2x:%2x:%2x:%2x:%2x\n",
							(intptr_t) netinput [ 6],
							(intptr_t) netinput [ 7],
							(intptr_t) netinput [ 8],
							(intptr_t) netinput [ 9],
							(intptr_t) netinput [10],
							(intptr_t) netinput [11]);
				} else {
					nethandler_llconly (netinput, netinputlen);
				}
			}
		} else {
			bottom_show_fixed_msg (APP_LEVEL_BACKGROUNDED, FIXMSG_OFFLINE);
		}
	}
}

