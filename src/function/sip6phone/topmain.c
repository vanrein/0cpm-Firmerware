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

#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/kbd.h>
#include <0cpm/led.h>
#include <0cpm/irq.h>	//TODO:ONLY-FOR-PATCHWORK-BELOW



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


void top_hook_update (bool offhook) {
	/* Keep the linker happy */ ;
}

void top_can_play (uint16_t samples) {
	/* Keep the linker happy */ ;
}

void top_can_record (uint16_t samples) {
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
timing_t nextrot = 0;
void show_info (irq_t *tmr) {
	int relpos;
	//TODO:MANUAL_TIMER_AS_REAL_ONE_FAILS://
#if 0
	timing_t now = bottom_time ();
	if (TIME_BEFORE (now, nextrot)) {
		return;
	}
	nextrot += TIME_MSEC (1000);
#endif
bottom_keyboard_scan ();
	if ((ip6binding [0].flags & (I6B_DEFEND_ME | I6B_EXPIRED)) == I6B_DEFEND_ME) {
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
#if 0
bool have_ipv4 (void);
bool have_ipv6 (void);
#endif
	uint8_t ipish [4] = { 0, 0, 0, 0 };
	// TODO: init ();
bottom_led_set (LED_IDX_SPEAKERPHONE, 1);
bottom_led_set (LED_IDX_BACKLIGHT, 1);
	// netdb_initialise ();
	// TODO: cpu_add_closure (X);
	// TODO: cpu_add_closure (Y);
	// TODO: cpu_add_closure (Z);
// IER0 &= ~(1 << REGBIT_IER0_INT0);
// IER1 &= 0xffff; // ~(1 << REGBIT_IER1_DMAC0);
	bottom_critical_region_end ();
	// netcore_bootstrap_initiate ();
bottom_led_set (LED_IDX_SPEAKERPHONE, 0);
	irqtimer_start (&disptimer, TIME_MSEC (1000), show_info, CPU_PRIO_LOW);
	while (true) {
#if 0
		if (have_ipv6 ()) {
			bottom_show_ip6 (1, ip6binding [0].ip6addr);
		} else {
			ipish [0] = (uint8_t) boot_state;
			ipish [1] = boot_retries;
			ipish [2] = bad;
			ipish [3] = have_ipv4 ()? 4: 0;
			bottom_show_ip4 (1, ipish);
		}
#else
		//TODO:TIMERED// show_info (&disptimer);
#endif
bottom_led_set (LED_IDX_BACKLIGHT, bottom_phone_is_offhook ()? 1: 0);

		jobhopper ();
bottom_led_set (LED_IDX_HANDSET, 1);
		//TODO// bottom_sleep_prepare ();
		//TODO// if (cur_prio == CPU_PRIO_ZERO) {
			//TODO// bottom_sleep_commit (SLEEP_SNOOZE);
		//TODO// }
#if defined NEED_KBD_SCANNER_BETWEEN_KEYS || defined NEED_KBD_SCANNER_DURING_KEYPRESS
		//TODO:TIMERED// bottom_keyboard_scan ();
#endif
	}
}

