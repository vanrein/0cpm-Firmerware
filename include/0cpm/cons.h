/* Console logging facility
 *
 * This implements logging over an LLC console, where supported.
 * Messages are written to the log when sent as bottom_printf (fmt, ...)
 * and will always be prefixed with __FILE__ and __LINE__ information.
 * The implementation is through a bottom_console_printf() call.
 * If the console is not compiled in, there will be no code generated
 * for such calls.
 *
 * For portability to all platforms, there must always be at least one
 * argument, even if it is not used.  There are ways to work around this
 * constraint, as described on the link below, but relying on it would
 * be asking for trouble when targetting all kinds of embedded compilers.
 * http://blog.mellenthin.de/archives/2010/10/26/portable-__va_args__-macros-for-linux-hp-ux-solaris-and-aix/
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#ifndef HEADER_CONSOLE
#define HEADER_CONSOLE

#ifndef CONFIG_FUNCTION_DEVEL_NETCONSOLE
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

#endif
