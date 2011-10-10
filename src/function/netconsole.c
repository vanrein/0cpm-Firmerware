/* Network console interface.
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


/* This interface makes it possible to connect to the phone's console
 * over LLC.  The advantage of LLC is that it works on a LAN only,
 * and does not require any support of protocols higher than Ethernet.
 * This makes it suitable to even debug things like IP leases.
 *
 * The common interface to these routines can be found in <0cpm/cons.h>
 * and defines a user-level routine bottom_printf (fmt, ...) which is
 * a restricted printf() function.  It is mapped to the implementation
 * of bottom_console_printf() below.
 *
 * The network side of this facility uses netcons_connect() and
 * netcons_close() to notify the console of a LAN host opening and
 * closing the connection.  The console will send data out using the
 * commonly available network sending routines.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

#include <config.h>
#include <0cpm/cons.h>
#include <0cpm/cpu.h>
#include <0cpm/irq.h>
#include <0cpm/timer.h>
#include <0cpm/text.h>


/*
 * The entire data and code portion depends on the function NETCONSOLE
 * being configured through "make menuconfig".
 */
#ifdef CONFIG_FUNCTION_NETCONSOLE


/*
 * Avoid including parts from the kernel while testing.
 */
#define FILECONFIG_IRQTIMER

#ifdef CONFIG_MAINFUNCTION_DEVEL_NETWORK
#undef FILECONFIG_IRQTIMER
#endif

#ifdef CONFIG_MAINFUNCTION_DEVEL_SOUND
#undef FILECONFIG_IRQTIMER
#endif

/******** BUFFER STATIC VARIABLES ********/

// TODO: Configuration option would be nice for CONSBUFLEN (max 64k - 1)
#define CONSBUFLEN 4096

static char consbuf [CONSBUFLEN];

static uint16_t rpos = 0, wpos = 0;

static bool console_timer_is_running = false;


/******** NETWORK INTERFACE ROUTINES ********/

static struct llc2 *netcons_connection = NULL;

void trysend (void) {
	if (netcons_connection) {
		if (rpos > wpos) {
			while (rpos < CONSBUFLEN) {
				uint16_t blksz = CONSBUFLEN - rpos;
				if (blksz > 80) {
					blksz = 80;
				}
				if (netsend_llc2 (netcons_connection, consbuf + rpos, blksz)) {
					rpos += blksz;
				} else {
					return;
				}
			}
			rpos = 0;
		}
		while (rpos < wpos) {
			uint16_t blksz = wpos - rpos;
			if (blksz > 80) {
				blksz = 80;
			}
			if (netsend_llc2 (netcons_connection, consbuf + rpos, blksz)) {
				rpos += blksz;
			} else {
				return;
			}
		}
	}
}

#ifdef FILECONFIG_IRQTIMER
static irqtimer_t console_timer;
static void netcons_interval (irq_t *tmr) {
	trysend ();
	if (rpos == wpos) {
		console_timer_is_running = false;
	}
	if (console_timer_is_running) {
		irqtimer_restart ((irqtimer_t *) tmr, TIME_MSEC(20));
	}
}
static void ensure_console_timer_runs (void) {
	if (netcons_connection && !console_timer_is_running) {
		console_timer_is_running = true;
		irqtimer_start (&console_timer, 0, netcons_interval, CPU_PRIO_LOW);
	}
}
#endif

void netcons_connect (struct llc2 *cnx) {
	netcons_connection = cnx;
#ifndef FILECONFIG_IRQTIMER
	trysend ();
#else
	ensure_console_timer_runs ();
#endif
}

void netcons_close (void) {
#ifdef FILECONFIG_IRQTIMER
	irqtimer_stop (&console_timer);
#endif
	netcons_connection = NULL;
}


/******** PRINTING ROUTINES ********/

static const char digits [] = "0123456789abcdef";

static void cons_putchar (char c) {
	if (wpos >= CONSBUFLEN) {
		wpos = 0;
	}
	if (wpos + 1 == rpos) {
		rpos++;
		if (rpos >= CONSBUFLEN) {
			rpos -= CONSBUFLEN;
		}
	}
	consbuf [wpos++] = c;
#ifdef FILECONFIG_IRQTIMER
	ensure_console_timer_runs ();
#endif
}

static void cons_putint (unsigned long int val, uint8_t base, uint8_t minpos) {
	unsigned long int divisor = 1;
	while (minpos-- > 1) {
		divisor *= base;
	}
	while (val / base > divisor) {
		divisor *= base;
	}
	do {
		cons_putchar (digits [(val / divisor)]);
		val %= divisor;
		divisor /= base;
	} while (divisor > 0);
}

static textptr_t const nulltext = { "(NULL)", 6 };

void bottom_console_vprintf (char *fmt, va_list argh) {
	char *fp = fmt;
	char *str;
	char ch;
	uint32_t intval;
	textptr_t const *txt;
	uint16_t idx;
	while (*fp) {
		uint8_t minpos = 0;
		bool val32bit = false;
		if (*fp != '%') {
			cons_putchar (*fp++);
		} else {
			fp++;
		moremeuk:
			switch (*fp++) {
			case '\0':
				fp--;
				break;
			case 'l':
				val32bit = true;
				goto moremeuk;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				minpos *= 10;
				minpos += fp [-1] - '0';
				goto moremeuk;
			case 'c':
				ch = (char) va_arg (argh, intptr_t);
				cons_putchar (ch);
				break;
			case 's':
				str = (char *) va_arg (argh, intptr_t);
				if (str == NULL) {
					str = "(NULL)";
				}
				while (*str) {
					cons_putchar (*str++);
					if (minpos > 0) {
						minpos--;
					}
				}
				while (minpos-- > 0) {
					cons_putchar (' ');
				}
				break;
			case 't':
				txt = (textptr_t *) va_arg (argh, intptr_t);
				idx = 0;
				if (textisnull (txt)) {
					txt = &nulltext;
				}
				while (idx < txt->len) {
					cons_putchar (txt->str [idx++]);
				}
				break;
			case 'd':
				if (val32bit) {
					intval = (uint32_t) va_arg (argh, intptr_t);
				} else {
					intval = (uint32_t) va_arg (argh, intptr_t);
				}
				cons_putint (intval, 10, minpos);
				break;
			case 'p':
				intval = (uint32_t) va_arg (argh, intptr_t);
				cons_putint (intval, 16, (minpos > 8)? minpos: 8);
				break;
			case 'x':
				if (val32bit) {
					intval = (uint32_t) va_arg (argh, intptr_t);
				} else {
					intval = (uint32_t) va_arg (argh, intptr_t);
				}
				cons_putint (intval, 16, minpos);
				break;
			case '%':
			default:
				cons_putchar (fp [-1]);
				break;
			}
		}
	}
#ifndef FILECONFIG_IRQTIMER
	trysend ();
#endif
}

void bottom_console_printf (char *fmt, ...) {
	va_list argh;
	va_start (argh, fmt);
	bottom_console_vprintf (fmt, argh);
	va_end (argh);
}


#endif   /* CONFIG_FUNCTION_NETCONSOLE */
