/* SIP over IPv6 firmware phone
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


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/kbd.h>
#include <0cpm/led.h>
#include <0cpm/irq.h>	//TODO:ONLY-FOR-PATCHWORK-BELOW
#include <0cpm/timer.h>
#include <0cpm/cons.h>
#include <0cpm/text.h>
#include <0cpm/sip.h>


/* TODO: shared variables for debugging's sake */
enum netcfgstate {
	NCS_OFFLINE,	// Await top_network_online()
	NCS_AUTOCONF,	// A few LAN autoconfiguration attempts
	NCS_DHCPV6,	// A few LAN DHCPv6 attempts
	NCS_DHCPV4,	// A few LAN DHCPv4 attempts
	NCS_ARP,	// A few router-MAC address fishing attempts
	NCS_6BED4,	// A few 6BED4 attempts over IPv4
	NCS_ONLINE,	// Stable online performance
	NCS_RENEW,	// Stable online, but renewing DHCPv6/v4
	NCS_PANIC,	// Not able to bootstrap anything
	NCS_HAVE_ALT = NCS_6BED4,
};
extern enum netcfgstate boot_state;
static uint8_t nextround = 0x00;	// Local, only for test purposes


void top_codec_can_play (uint8_t chan) {
	/* Keep the linker happy */ ;
}

void top_codec_can_record (uint8_t chan) {
	/* Keep the linker happy */ ;
}

void top_button_press (buttonclass_t bcl, buttoncode_t cde) {
	if (bcl != BUTCLS_DTMF) {
		return;
	}
	switch (cde) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
		nextround = cde - '0';
		bottom_critical_region_begin ();
		top_network_offline ();
		bottom_critical_region_end ();
		break;
	default:
		break;
	}
}

void top_button_release (void) {
	if (nextround != 0x00) {
		bottom_critical_region_begin ();
		top_network_online ();
		boot_state = nextround;	// Haven't switched to handler yet!
		bottom_critical_region_end ();
	}
	nextround = 0x00;
}

/* TODO: shared variables for debugging's sake */
extern uint8_t          boot_retries;
#include <0cpm/cpu.h>
#include <0cpm/irq.h>
#include <0cpm/timer.h>
#include <0cpm/netdb.h>
extern struct ip6binding ip6binding [IP6BINDING_COUNT];
uint8_t bad;

irqtimer_t disptimer;
int nibblepos = 0;

bool registered = false;
bool am_offhook = false;
bool connected = false;
bool talking = false;
irqtimer_t regtimer;
dialog_t *regdia = NULL;
dialog_t *calldia = NULL;

void invite_response (textptr_t *sip, textptr_t *sdp) {
	textptr_t code, desc;
	sip_splitline_response (sip, &code, &desc);
	bottom_printf ("SIP response to INVITE received: %t %t\n", &code, &desc);
	if (memcmp (code.str, "180", 3)) {
		ht162x_led_set (11, 1, true); // ringing
		ht162x_led_set (13, 0, true); // horn.off
		ht162x_led_set (15, 0, true); // error.off
	} else if (memcmp (code.str, "200", 3)) {
		ht162x_led_set (11, 0, true); // ringing.off
		ht162x_led_set (13, 1, true); // horn
		ht162x_led_set (15, 0, true); // error.off
	} else if (*code.str >= '4') {
		ht162x_led_set (11, 0, true); // ringing.off
		ht162x_led_set (13, 0, true); // horn.off
		ht162x_led_set (15, 1, true); // error.on
	} else {
		ht162x_led_set (11, 0, true); // ringing.off
		ht162x_led_set (13, 0, true); // horn.off
		ht162x_led_set (15, 0, true); // error.off
	}
}

void byeresponse (textptr_t *sip, textptr_t *sdp) {
	textptr_t code, desc;
	sip_splitline_response (sip, &code, &desc);
	bottom_printf ("SIP response (TODO:process) to BYE received: %t %t\n", &code, &desc);
}

void cancelresponse (textptr_t *sip, textptr_t *sdp) {
	textptr_t code, desc;
	sip_splitline_response (sip, &code, &desc);
	bottom_printf ("SIP response (TODO:process) to CANCEL received: %t %t\n", &code, &desc);
}

void register_response (textptr_t *sip, textptr_t *sdp) {
	textptr_t code, desc;
	sip_splitline_response (sip, &code, &desc);
	bottom_printf ("SIP response to REGISTER: %t %t\n", &code, &desc);
	if (regdia) {
		sipdia_deallocate (regdia);
		regdia = NULL;
	}
}

void top_hook_update (bool offhook) {
	bottom_led_set (LED_IDX_BACKLIGHT, offhook? LED_STABLE_ON: LED_STABLE_OFF);
	if (am_offhook != offhook) {
		am_offhook = offhook;
		if (registered && offhook) {
			if (calldia) {
				sipdia_deallocate (calldia);
				calldia = NULL;
			}
			calldia = sipdia_allocate (NULL, true);
			if (calldia) {
				sipdia_setlinenr (calldia, 2);
				siptr_client (calldia, &invite_m, invite_response);
			}
		}
		if (registered && !offhook) {
			siptr_client (calldia,
					talking? &bye_m: &cancel_m,
					talking? byeresponse: cancelresponse);
		}
	}
}


extern priority_t cur_prio;

/* TODO: bottom definitions shouldn't be copied in the top */
extern volatile uint16_t IER0, IER1;
asm ("_IER0 .set 0x0000");
asm ("_IER1 .set 0x0045");
#define REGBIT_IER0_TINT0       4
#define REGBIT_IER0_DMAC1       9
#define REGBIT_IER0_INT0	2
#define REGBIT_IER1_TINT1       6
#define REGBIT_IER1_DMAC0       2

void register_phone (irq_t *tmr) {
	if (regdia) {
		bottom_printf ("Previous registration still active -- repeating the attempt\n");
	}
	if (!regdia) {
		regdia = sipdia_allocate (NULL, true);
	}
	if (regdia) {
		sipdia_setlinenr (regdia, 1);
		bottom_printf ("Registering phone\n");
		siptr_client (regdia, &register_m, register_response);
	} else {
		bottom_printf ("No free phone line -- not registering\n");
	}
	irqtimer_restart ((irqtimer_t *) tmr, TIME_SEC (300));
}

void show_info (irq_t *tmr) {
	int relpos;
#if defined NEED_KBD_SCANNER_BETWEEN_KEYS || defined NEED_KBD_SCANNER_DURING_KEYPRESS
	bottom_keyboard_scan ();
#endif
#if defined NEED_HOOK_SCANNER_WHEN_ONHOOK || defined NEED_HOOK_SCANNER_WHEN_OFFHOOK
	bottom_hook_scan ();
#endif
	if ((ip6binding [0].flags & (I6B_DEFEND_ME | I6B_EXPIRED)) == I6B_DEFEND_ME) {
		if (!registered) {
			registered = true;
			irqtimer_start (&regtimer, 0, register_phone, CPU_PRIO_LOW);
		}
		for (relpos = 0; relpos < 12; relpos++) {
			int actpos = relpos + nibblepos;
			uint8_t ip6digit = 0x00;
			if ((actpos >= 12) && (actpos < 44)) {
				ip6digit = ip6binding [0].ip6addr [(actpos - 12) >> 1];
				if ((actpos & 0x01) == 0x00) {
					ip6digit >>= 4;
				}
				ip6digit &= 0x0f;
				ip6digit += (ip6digit < 10)? '0': ('A'-10);
			}
			ht162x_putchar (14 - relpos, ip6digit, false);
		}
		nibblepos = (nibblepos + 1) % 44;
	} else {
		for (relpos=0; relpos<12; relpos++) {
			ht162x_putchar (14 - relpos, '-', false);
		}
		nibblepos = 0;
	}
	ht162x_dispdata_notify (3, 14);
	irqtimer_restart ((irqtimer_t *) tmr, TIME_MSEC (1000));
}

/* The main operational loop for the CPU.  Try to run code, and when
 * nothing remains to be done, stop to wait for interrupts.
 */
void top_main (void) {
	uint8_t ipish [4] = { 0, 0, 0, 0 };
	bottom_led_set (LED_IDX_BACKLIGHT, LED_STABLE_OFF);
	netdb_initialise ();
	sipdia_initialise ();
	bottom_critical_region_end ();
	irqtimer_start (&disptimer, TIME_MSEC (1000), show_info, CPU_PRIO_LOW);
	while (true) {
		jobhopper ();
		//TODO// bottom_sleep_prepare ();
		//TODO// if (cur_prio == CPU_PRIO_ZERO) {
			//TODO// bottom_sleep_commit (SLEEP_SNOOZE);
		//TODO// }
	}
}

