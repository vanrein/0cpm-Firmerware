/* Realtime Text sending, according to RFC 4103 (plus its 2 errata).
 *
 * This file is part of 0cpm Firmerware, but will not be included
 * in the firmware itself.  It is here for testing on a desktop
 * system.  See the README in this directory for details.
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


#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/irq.h>
#include <0cpm/timer.h>

// #include <0cpm/netdb.h>
// #include <0cpm/netinet.h>
typedef uint8_t  nint8_t;
typedef uint16_t nint16_t;
typedef uint32_t nint32_t;
#include <0cpm/netfun.h>

#undef htons
#undef htonl
#undef ntohs
#undef ntohl

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/timeb.h>

#include <ncurses.h>


extern uint8_t rtt_paytp_red;
extern uint8_t rtt_paytp_t140;


packet_t wbuf;


bool have_timer = false;
struct timeval timer;
irqtimer_t *rtos_timer;
irq_handler_t timer_hdl;

int sox;


WINDOW *top, *bot;


struct termios stdin_oldstate;


/* Return the time in the timer base value */
timing_t bottom_time (void) {
	struct timeb tp;
	ftime (&tp);
	return ((timing_t) tp.time) * TIME_MSEC (1000) + tp.millitm * TIME_MSEC (1);
}


/* Prefix and fill UDP/IPv6 headers -- Linux socket does that for us */
uint8_t *netsend_udp6 (uint8_t *pout, intptr_t *mem) {
	mem [MEM_UDP6_HEAD] = (intptr_t) pout;
	return pout;
}

/* Send buffer after calculating checksums -- which Linux will handle for us */
void netcore_send_buffer (intptr_t *mem, uint8_t *wbuf) {
	size_t udp6len = mem [MEM_ALL_DONE] - mem [MEM_UDP6_HEAD];
	if (send (sox, wbuf, udp6len, 0) == -1) {
		perror ("Failed sending keystrokes");
	}
}


/* RTT received key presses -- process them by printing on the output tty */
void rtt_recv_keys (uint8_t *text, uint16_t len) {
	// ssize_t shown = write (1, text, len);
	ssize_t shown = len;
	while (len-- > 0) {
		if (*text == 127) {
			*text = 0x08;
		} else if (*text == '\r') {
			*text = '\n';
		}
		if (wprintw (top, "%c", *text++) != ERR) {
			shown--;
		}
	}
#if 0
	if (shown != (ssize_t) len) {
		wprintw (top, "\nWarning: Only %d out of %d characters shown\n", (int) shown, (int) len);
	}
#endif
	wrefresh (top);
}


void irqtimer_start (irqtimer_t *tmr, timing_t dt, irq_handler_t hdl, priority_t prio) {
	/*
	 * Explicitly use the fact that Linux reduces the timer value
	 * when it returns from select(), so another call would just
	 * continue with the timer, a somewhat inaccurate but
	 * test-worthy but realtime timer mechanism.
	 */
	rtos_timer = tmr;
	have_timer = true;
	timer_hdl = hdl;
	timer.tv_sec = 0;
	timer.tv_usec = dt * TIME_MSEC (1000);		/* 1000 is strange but effective */
}

void irqtimer_restart (irqtimer_t *tmr, timing_t dt) {
	/*
	 * Although it would be best to add this offset to the time of
	 * the previous timer expiry, simply restarting when done will be
	 * sufficiently accurate for testing purposes.
	 */
	irqtimer_start (tmr, dt, timer_hdl, 0);
}

void irqtimer_stop (irqtimer_t *tmr) {
	/*
	 * Not having a timer actually means that the timer is used at
	 * a 57-second interval, and an empty UDP message is sent out,
	 * under the assumption that this interval will suffice to keep
	 * local firewalls open for return traffic.  As a result, a
	 * session should not expire as long as both ends are running
	 * this program.
	 *
	 * Explicitly use the fact that Linux reduces the timer value
	 * when it returns from select(), so another call would just
	 * continue with the timer, a somewhat inaccurate but
	 * test-worthy but realtime timer mechanism.
	 */
	have_timer = false;
	timer.tv_sec = 57;
	timer.tv_usec = 0;
}


/* Open local firewalls for RTT, or keep them open */
void open_local_firewalls () {
	if (send (sox, "", 0, 0) == -1) {
		perror ("Failed opening firewall hole");
	}
	irqtimer_stop (NULL);
}


/* Return stdin tty to normal use; done at exit */
void stdin_reset(void) { /* set it to normal! */
	tcsetattr(0, TCSAFLUSH, &stdin_oldstate);
}

/* Return the screen to normal mode */
void screen_reset (void) {
	if (bot) delwin (bot);
	if (top) delwin (top);
	endwin ();
}

/* Code based on online demonstration code */
void stdin_raw(void) {       /* RAW! mode */
	struct termios  stdin_newstate;

	if (tcgetattr(0, &stdin_oldstate) < 0) { /* get the original state */
        	fprintf (stderr, "Failed to retrieve original terminal setup\n");
		exit (1);
	}
	atexit (stdin_reset);

	stdin_newstate = stdin_oldstate;

	stdin_newstate.c_lflag &= ~( ICANON | IEXTEN | ISIG);
                    /* canonical mode off, extended input
                       processing off, signal chars off;
		       do not suppress local echo */

	stdin_newstate.c_iflag &= ~(BRKINT | ISTRIP | IXON);
                    /* no SIGINT on BREAK, input parity
                       check off, don't strip the 8th bit on input,
                       ouput flow control off;
 		       retain CR-toNL off */

	stdin_newstate.c_iflag |= ICRNL;
		    /* Translate CR to newline on input */

	stdin_newstate.c_lflag |= ISIG;
		    /* recognise and process specials like ^C */

	stdin_newstate.c_cflag &= ~(CSIZE | PARENB);
                    /* clear size bits, parity checking off */

	stdin_newstate.c_cflag |= CS8;
                    /* set 8 bits/char */

	// stdin_newstate.c_oflag &= ~(OPOST);
                    /* output processing off */

	stdin_newstate.c_oflag |= ONLCR;
		    /* map newline to CR-LF on output  */

	stdin_newstate.c_cc[VMIN] = 1;  /* 1 byte at a time */
	stdin_newstate.c_cc[VTIME] = 0; /* no timer on input */
	stdin_newstate.c_cc[VERASE] = 127; /* use backspace to delete characters */

	if (tcsetattr(0, TCSAFLUSH, &stdin_newstate) < 0) {
		perror ("Failed to reconfigure input in raw mode\n");
		exit (1);
	}
}


int main (int argc, char *argv []) {
	int fdo;
	int port;
	uint8_t input [20];
	size_t inlen = 0;
	uint16_t inofs = 0;
	struct sockaddr_in6 local, remot;
	//
	// Test arguments
	if (argc != 5) {
		fprintf (stderr, "Usage: %s myAddr myPort remoteAddr remotePort\n"
				"   Where is an output terminal, and addresses are IPv6 (as RTP would deliver).\n"
				"   This is not a fancy interface, but rather demonstrates the protocol.\n",
			argv [0]);
		exit (1);
	}
	//
	// Test stdin to be interactive
	if (!isatty (0)) {
		fprintf (stderr, "Please run the command with a terminal for input\n");
		exit (1);
	}
	//
	// Create sockaddr_in6 for local/remot
	bzero (&local, sizeof (local));
	bzero (&remot, sizeof (remot));
	local.sin6_family = AF_INET6;
	remot.sin6_family = AF_INET6;
	if (inet_pton (AF_INET6, argv [1], &local.sin6_addr) != 1) {
		fprintf (stderr, "Failed to parse local IPv6 address\n");
		exit (1);
	}
	if (inet_pton (AF_INET6, argv [3], &remot.sin6_addr) != 1) {
		fprintf (stderr, "Failed to parse remote IPv6 address\n");
		exit (1);
	}
	port = atoi (argv [2]);
	if ((port <= 0) || (port > 65535)) {
		fprintf (stderr, "Invalid local port\n");
		exit (1);
	}
	local.sin6_port = htons (port);
	port = atoi (argv [4]);
	if ((port <= 0) || (port > 65535)) {
		fprintf (stderr, "Invalid remote port\n");
		exit (1);
	}
	remot.sin6_port = htons (port);
	//
	// Create a socket for UDP with the given endpoints
	sox = socket (AF_INET6, SOCK_DGRAM, 0);
	if (sox == -1) {
		perror ("Failed to create local UDP socket");
		exit (1);
	}
	if (bind (sox, (struct sockaddr *) &local, sizeof (local)) == -1) {
		perror ("Failed to bind to local address/port");
		exit (1);
	}
	if (connect (sox, (struct sockaddr *) &remot, sizeof (remot)) == -1) {
		perror ("Failed to setup remote address/port");
		exit (1);
	}
	//
	// An empty UDP message will open any local firewalls for return traffic
	open_local_firewalls ();
	//
	// Change input to raw mode (and have it returned to normal upon exit)
	stdin_raw ();
	//
	// Split the screen, using ncurses
	top = NULL;
	bot = NULL;
	initscr ();
	atexit (screen_reset);
	mvhline (LINES/2, 0, ACS_HLINE, COLS);
	refresh ();
	top = newwin (LINES/2, COLS, 0, 0);
	bot = newwin (LINES - LINES/2 - 1, COLS, LINES/2 + 1, 0);
	wprintw (top, "Remote:\n");
	wprintw (bot, "Your input:\n");
	wrefresh (top);
	wrefresh (bot);
	scrollok (bot, true);
	scrollok (top, true);
	//
	// Main loop -- wait for input from stdin or the UDP port, and relay it
	while (true) {
		fd_set ears;
		int seln;
		FD_ZERO (&ears);
		if (inlen == 0) {
			FD_SET (0, &ears);	// Only when nothing left to send
		}
		FD_SET (sox,   &ears);		// No limits on incoming cps
		seln = select (sox+1, &ears, NULL, NULL, &timer);
		if (seln == -1) {
			perror ("Failed to select input source");
			exit (1);
		}
		if (seln == 0) {
			if (have_timer) {
				(*timer_hdl) (&rtos_timer->tmr_irq);
				if (inlen > 0) {
					/* Try again on pending data */
					FD_SET (0, &ears);
				}
			} else {
				open_local_firewalls ();
			}
		}
		if (FD_ISSET (0, &ears)) {
			/* If no input awaits sending, pickup new data */
			if (inlen == 0) {
				uint16_t inecho = 0;
				inlen = read (0, input, sizeof (input));
				while (inecho < inlen) {
					if (input [inecho] == 127) {
						input [inecho] = 0x08;
					} else if (input [inecho] == '\r') {
						input [inecho] = '\n';
					}
					wprintw (bot, "%c", input [inecho++]);
				}
				wrefresh (bot);
				inofs = 0;
				if (inlen == -1) {
					perror ("Error reading from stdin");
					exit (1);
				}
			}
			/* If input is available, send as much of it as possible */
			if (inlen > 0) {
				uint16_t fwlen = rtt_send_keys (input + inofs, (uint16_t) inlen);
				inlen -= fwlen;
				inofs += fwlen;
			}
		}
		if (FD_ISSET (sox, &ears)) {
			uint8_t rtpbuf [2048];
			size_t rtplen = recv (sox, rtpbuf, sizeof (rtpbuf), 0);
			if (rtplen == -1) {
				if (errno != ECONNREFUSED) {
					perror ("Failed to receive realtime text");
					exit (1);
				} else {
					wprintw (top, "** NOT CONNECTED TO REMOTE PARTY **\n");
					wrefresh (top);
				}
			}
			if (rtplen < 12) {
				continue;
			}
			if ((rtpbuf [1] & 0x7f) == rtt_paytp_red)  {
				rtp_paytype_text_red (rtpbuf, rtplen);
			} else if ((rtpbuf [1] & 0x7f) == rtt_paytp_t140) {
				rtp_paytype_text_t140 (rtpbuf, rtplen);
			} else {
				//TODO:TOO_TOLERANT?// fprintf (stderr, "Received RTP with unknown payload type %d\n", rtpbuf [1] & 0x7f);
				//TODO:TOO_TOLERANT?// exit (1);
			}
		}
	}
}

