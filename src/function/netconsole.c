/* Network console interface.
 *
 * This interface makes it possible to connect to the phone's console
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


/*
 * The entire data and code portion depends on the function NETCONSOLE
 * being configured through "make menuconfig".
 */
#ifdef CONFIG_FUNCTION_DEVEL_NETCONSOLE


/******** BUFFER STATIC VARIABLES ********/

// TODO: Configuration option would be nice for CONSBUFLEN (max 64k - 1)
#define CONSBUFLEN 1024

static char consbuf [CONSBUFLEN];

static uint16_t rpos = CONSBUFLEN, wpos = 0;


/******** NETWORK INTERFACE ROUTINES ********/

static struct llc2 *netcons_connection = NULL;

static void trysend (void) {
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

void netcons_connect (struct llc2 *cnx) {
	netcons_connection = cnx;
	trysend ();
}

void netcons_close (void) {
	netcons_connection = NULL;
}


/******** PRINTING ROUTINES ********/

static const char digits [] = "0123456789abcdef";

static void cons_putchar (char c) {
	// TODO: Handle buffer full conditions
	if (wpos >= CONSBUFLEN) {
		wpos = 0;
	}
	consbuf [wpos++] = c;
}

static void cons_putint (uint32_t val, uint8_t base, uint8_t minpos) {
	uint32_t divisor = 1;
	while (minpos-- > 0) {
		divisor *= base;
	}
	while (val / base > divisor) {
		divisor *= base;
	}
	while (divisor > 0) {
		cons_putchar (digits [(val / divisor)]);
		val %= divisor;
		divisor /= base;
	}
}

void bottom_console_vprintf (char *fmt, va_list argh) {
	char *fp = fmt;
	char *str;
	char ch;
	uint32_t intval;
	while (*fp) {
		if (*fp != '%') {
			cons_putchar (*fp++);
		} else {
			fp++;
			switch (*fp++) {
			case '\0':
				fp--;
				break;
			case 'c':
				ch = va_arg (argh, char);
				cons_putchar (ch);
				break;
			case 's':
				str = va_arg (argh, char *);
				while (*str) {
					cons_putchar (*str++);
				}
				break;
			case 'd':
				intval = (uint32_t) va_arg (argh, unsigned int);
				cons_putint (intval, 10, 0);
				break;
			case 'l':
				intval = va_arg (argh, uint32_t);
				cons_putint (intval, 10, 0);
				break;
			case 'p':
				intval = (uint32_t) va_arg (argh, void *);
				cons_putint (intval, 16, 8);
				break;
			case 'x':
				intval = (uint32_t) va_arg (argh, unsigned int);
				cons_putint (intval, 16, 8);
				break;
			case '%':
			default:
				cons_putchar (fp [-1]);
				break;
			}
		}
	}
	trysend ();
}

void bottom_console_printf (char *fmt, ...) {
	va_list argh;
	va_start (argh, fmt);
	bottom_console_vprintf (fmt, argh);
	va_end (argh);
}


#endif   /* CONFIG_FUNCTION_DEVEL_NETCONSOLE */
