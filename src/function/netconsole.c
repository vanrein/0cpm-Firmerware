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

static uint16_t rpos = 0, wpos = 0;


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

void bottom_console_vprintf (char *fmt, va_list argh) {
	char *fp = fmt;
	char *str;
	char ch;
	unsigned long int intval;
	while (*fp) {
		uint8_t minpos = 0;
		bool longval = false;
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
				longval = true;
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
				ch = va_arg (argh, char);
				cons_putchar (ch);
				break;
			case 's':
				str = va_arg (argh, char *);
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
			case 'd':
				if (longval) {
					intval =                     va_arg (argh, unsigned long int);
				} else {
					intval = (unsigned long int) va_arg (argh, unsigned int);
				}
				cons_putint (intval, 10, minpos);
				break;
			case 'p':
				intval = (unsigned long int) va_arg (argh, void *);
				cons_putint (intval, 16, (minpos > 8)? minpos: 8);
				break;
			case 'x':
				if (longval) {
					intval =                     va_arg (argh, unsigned long int);
				} else {
					intval = (unsigned long int) va_arg (argh, unsigned int);
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
	trysend ();
}

void bottom_console_printf (char *fmt, ...) {
	va_list argh;
	va_start (argh, fmt);
	bottom_console_vprintf (fmt, argh);
	va_end (argh);
}


#endif   /* CONFIG_FUNCTION_DEVEL_NETCONSOLE */
