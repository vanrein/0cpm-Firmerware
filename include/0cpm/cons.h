/* Console logging facility
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


/* This implements logging over an LLC console, where supported.
 * Messages are written to the log when sent as bottom_printf (fmt, ...)
 * This is passed to the console routines through inline functions, as
 * not all embedded compilers support varargs in #defines.  The functions
 * should reduce to no code without CONFIG_FUNCTION_NETCONSOLE.
 *
 * The formats are very close to those of printf():
 *  - %s %c %d %x print string or (NULL), char, decimal and hexadecimal
 *  - %p prints a pointer in hexadecimal notation
 *  - %d and %x print unsigned int; %ld and %lx print unsigned long int
 *  - field lengths like %12s attach spaces; %5d and %4x prefix zeroes
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#ifndef HEADER_CONSOLE
#define HEADER_CONSOLE

#ifndef CONFIG_FUNCTION_NETCONSOLE
	inline void bottom_printf (char *fmt, ...) { ; }
#else
	void bottom_console_vprintf (char *fmt, va_list argh);
	void bottom_console_printf (char *fmt, ...);
	inline void bottom_printf (char *fmt, ...) {
		va_list argh;
		va_start (argh, fmt);
		// bottom_console_printf ("%s:%d ", __FILE__, __LINE__);
		bottom_console_vprintf (fmt, argh);
		va_end (argh);
	}
#endif

#endif /* CONFIG_FUNCTION_NETCONSOLE */
