/* Realtime Text sending, according to RFC 4103 (plus its 2 errata).
 *
 * RTT or Realtime Text was designed to support people with speech or
 * hearing impairments to communicate interactively with text.  The
 * realtime delivery of keystrokes is also useful to plain users,
 * for instance in interactive menu's served by an automated attendent.
 * For this reason, it is built into the 0cpm Firmerware by default.
 * An extra reason is that impaired people can now reach more other
 * people over plain telephony.
 *
 * To the end-user, RTT will merely show as an application named
 * "text" that happens to be interactive.  It can also be encrypted
 * through ZRTP, another advantage for this style of messaging.
 *
 * RTT is announced with MIME-types text/t140 and text/red in an SDP
 * profile.  If it is offered, the 0cpm Firmerware will enable an
 * application for local text entry, as well as display of text.  The
 * remote side is offered text/t140 and text/red in all SDP offers,
 * and any subsequent sends to this service will cause text to popup.
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


#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/irq.h>
#include <0cpm/timer.h>

#include <0cpm/netdb.h>
#include <0cpm/netinet.h>
#include <0cpm/netfun.h>



/* The number of historic versions kept for resending */
#ifndef RTT_HISTLEN
#   define RTT_HISTLEN 3
#endif

/* The maximum number of key bytes permitted in each block */
#ifndef RTT_MAXBYTES
#   define RTT_MAXBYTES 16
#endif

#if RTT_MAXBYTES >= 1024
#   error "Realtime Text cannot handle blocks of 1024 or more bytes"
#endif

/* The number of historic versions available */
static uint8_t rtt_histlen = 0;

/* The current history entry being write */
static uint8_t rtt_histcurr = 0;

/* The number of bytes per history version */
static uint16_t rtt_histbytes [RTT_HISTLEN];

/* The timing of the individual submissions */
static timing_t rtt_timing [RTT_HISTLEN];

/* The history itself */
static uint8_t rtt_history [RTT_HISTLEN] [RTT_MAXBYTES];

/* After radio silence, including at the start, send right away */
static bool rtt_silence = true;

/* Count the number of repeated sends of the exact same information */
static uint8_t rtt_repeats = 0;
static uint8_t rtt_repeated = 0xff;

/* Redundancy flag and payload types for redundant and t.140 text */
bool rtt_redundancy = true;
uint8_t rtt_paytp_red = 100;
uint8_t rtt_paytp_t140 = 98;

/* Sequence number for RTP packet counting */
static uint16_t rtt_seqnr = 0x9a81;	//TODO// Random starting point

/* Timer interrupt for the regular sending interval */
static irqtimer_t rtt_timer;


/* TODO: rtt_init() should fill an rttctx_t for this "codec" */


/* Send the current buffer contents immediately.  This involves
 * stopping any timer that may have been running.  Unless there
 * have been three resends with the exact same information, the
 * timer is restarted to expire in 300 ms from now.
 */
void rtt_send_buffer (irq_t *irq) {
	extern packet_t wbuf;
	uint8_t *pout = wbuf.data;
	irqtimer_t *tmr = (irqtimer_t *) irq;
	intptr_t mem [MEM_NETVAR_COUNT];	//TODO// Consider purpose-specific global mem[]
	uint8_t idx, idx2;
	//
	// Stop any current timer activity
	//TODO// Better to use restart further down, and always fire this after timer exp
	if (! rtt_silence) {
		irqtimer_stop (tmr);
	}
	//
	// Construct a buffer with the RTP packet
	rtt_timing [rtt_histcurr] = bottom_time () * 1000 / TIME_MSEC (1000);
	bzero (mem, sizeof (mem));
	//TODO// Setup mem[] with actual output pointers
	pout = netsend_udp6 (pout, mem);
	pout [0] = 0x80;		// V=2, P=0, X=0, CC=0
	pout [1] = (rtt_redundancy? rtt_paytp_red: rtt_paytp_t140) | (rtt_silence? 0x80: 0x00);
	netset16 (*(nint16_t *) &pout [2], rtt_seqnr++);
	netset32 (*(nint32_t *) &pout [4], rtt_timing [rtt_histcurr]);
	netset32 (*(nint32_t *) &pout [8], 0);		// SSRC -- TODO, 0 OK?
	pout += 12;
	if (rtt_redundancy) {
		idx2 = RTT_HISTLEN + rtt_histcurr - (rtt_histlen - 1);
		if (idx2 >= RTT_HISTLEN) {
			idx2 -= RTT_HISTLEN;
		}
		idx = idx2;
		while (idx2 != rtt_histcurr) {
			uint32_t tdiff;
			tdiff = rtt_timing [rtt_histcurr] - rtt_timing [idx2];
			if (tdiff <= 16383) {
				uint16_t bytelen = rtt_histbytes [idx2];
				*pout++ = 0x80 | rtt_paytp_t140;	/* Flag: more to come */
				netset16 (*(nint16_t *) pout, (tdiff << 2) | (bytelen >> 8));
				pout += 2;
				*pout++ = bytelen & 0xff;
			} else {
				// too old => skip here => also below
				idx = idx2 + 1;
				if (idx >= RTT_HISTLEN) {
					idx = 0;
				}
			}
			idx2++;
			if (idx2 >= RTT_HISTLEN) {
				idx2 = 0;
			}
		}
		*pout++ = 0x00 | rtt_paytp_t140;			/* Flag: last */
	} else {
		idx = rtt_histcurr;
	}
	do {
		memcpy (pout, rtt_history [idx], rtt_histbytes [idx]);
		pout += rtt_histbytes [idx];
		idx2 = idx;
		idx++;
		if (idx >= RTT_HISTLEN) {
			idx = 0;
		}
	} while (idx2 != rtt_histcurr);
	//
	// Actually send the buffer
	mem [MEM_ALL_DONE] = (intptr_t) pout;
	netcore_send_buffer (mem, wbuf.data);
	//
	// Move on to the next historic version
	rtt_histcurr++;
	if (rtt_histcurr >= RTT_HISTLEN) {
		rtt_histcurr = 0;
	}
	rtt_histbytes [rtt_histcurr] = 0;
	if (rtt_histlen < RTT_HISTLEN) {
		rtt_histlen++;
	}
	//
	// If not repeating endlessly, trigger this action again in 300 ms
	if (rtt_repeated != rtt_histcurr) {
		rtt_repeated = rtt_histcurr;
		rtt_repeats = 0;
	}
	//TODO// Better to wait for last timer expiry, but not send then
	rtt_silence = (rtt_repeats++ >= RTT_HISTLEN-1);
	if (! rtt_silence) {
		irqtimer_start (tmr, TIME_MSEC(300), rtt_send_buffer, CPU_PRIO_MEDIUM);
	}
}


/* Send a number of bytes, to indicate one or more keys being pressed.
 * The return value indicates how much of the offered characters were sent.
 * If not everything could be sent, it is recommended to wait before
 * trying to send the rest.  Buffers and sending limits may have gotten
 * in the way of a long text submission.
 */
uint16_t rtt_send_keys (uint8_t *keys, uint16_t len) {
	uint16_t plen;
	//
	// Determine the number of bytes to send in this block
	plen = RTT_MAXBYTES - rtt_histbytes [rtt_histcurr];
	if (len < plen) {
		plen = len;
	}
	//TODO// Reduce plen to cover a fixed number of chars
	//
	// Store values in the history
	memcpy (rtt_history [rtt_histcurr] + rtt_histbytes [rtt_histcurr], keys, plen);
	rtt_histbytes [rtt_histcurr] += plen;
	//
	// Consider sending the current buffer:
	//   1. When out of buffer space
	//   2. Otherwise, when currently silent
	//   3. Otherwise, after a regular interval delay.
	if ((len > plen) || (rtt_histbytes [rtt_histcurr] == RTT_MAXBYTES)) {
		rtt_send_buffer (&rtt_timer.tmr_irq);	/* Send now, next block may follow soon */
	} else if (rtt_silence) {
		rtt_send_buffer (&rtt_timer.tmr_irq);	/* Send now, and repeat from now on */
	} else {
		;	/* Await timer expiration */
	}
	return plen;
}

