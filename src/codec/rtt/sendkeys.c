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


#ifndef CONFIG_CODEC_RTT
#warning "Overriding deselection of Realtime Text codec -- see help in configuration menus"
#endif



/* The number of historic versions kept for resending */
#ifndef RTT_GENERATIONS
#   define RTT_GENERATIONS 3
#endif

/* The maximum number of key bytes to buffer */
#ifndef RTT_HISTBUFSZ
#   define RTT_HISTBUFSZ 128
#endif

#if RTT_HISTBUFSZ > 1023 * RTT_GENERATIONS
#   error "Realtime Text cannot handle textblock generations of 1024 or more bytes"
#endif

/* The current historic generations of history being written */
static uint8_t rtt_generation = 1;

/* The historic buffer itself */
static uint8_t rtt_buffer [RTT_HISTBUFSZ];

/* The number of bytes per history version */
static uint16_t rtt_offsets [RTT_GENERATIONS + 1];

/* The timing of the individual submissions */
static timing_t rtt_timings [RTT_GENERATIONS + 1];

/* After radio silence, including at the start, send right away */
static bool rtt_silence = true;

/* Redundancy flag and payload types for redundant and t.140 text */
bool rtt_redundancy = true;
uint8_t rtt_paytp_red = 100;
uint8_t rtt_paytp_t140 = 98;

/* Sequence number for RTP packet counting */
static uint16_t rtt_seqnr = 0x9a81;	//TODO// Random starting point

/* Timer interrupt for the regular sending interval */
static irqtimer_t rtt_timer;


/* The strut 0xfeff in UTF-8.  ITU calls this "zero width no-break space" */
uint8_t rtt_strut [] = { 0xef, 0xbb, 0xbf };


/* TODO: rtt_init() should fill an rttctx_t for this "codec" */


/* Copy from a cyclical buffer to a linear one */
//TODO// This is generic functionality, move it to a central place
void memcpy_cyc2lin (uint8_t *dst, uint8_t *src, uint16_t src_ofs, uint16_t src_sz, uint16_t cplen) {
	uint16_t pass1maxlen = src_sz - src_ofs;
	if (cplen > pass1maxlen) {
		memcpy (dst, src + src_ofs, pass1maxlen);
		dst   += pass1maxlen;
		cplen -= pass1maxlen;
		src_ofs = 0;
	}
	memcpy (dst, src + src_ofs, cplen);
}


/* Copy from linear buffer to a cyclical one */
//TODO// This is generic functionality, move it to a central place
void memcpy_lin2cyc (uint8_t *dst, uint16_t dst_ofs, uint16_t dst_sz, uint8_t *src, uint16_t cplen) {
	uint16_t pass1maxlen = dst_sz - dst_ofs;
	if (cplen > pass1maxlen) {
		memcpy (dst + dst_ofs, src, pass1maxlen);
		src   += pass1maxlen;
		cplen -= pass1maxlen;
		dst_ofs = 0;
	}
	memcpy (dst + dst_ofs, src, cplen);
}



/* Send the current buffer contents immediately.  This involves
 * stopping any timer that may have been running.  Unless there
 * have been three resends with the exact same information, the
 * timer is restarted to expire in 300 ms from now.
 *
 * TODO: Setup cps*0.3 as the size limit for chars to add.
 */
void rtt_send_buffer (irq_t *irq) {
	extern packet_t wbuf;
	uint8_t *pout = wbuf.data;
	irqtimer_t *tmr = (irqtimer_t *) irq;
	intptr_t mem [MEM_NETVAR_COUNT];	//TODO// Consider global or per-connection static mem[]
	uint8_t idx, idx2;
	uint16_t cplen;
	//
	// If this is the 3rd time of sending nothing, stop the interval timer
	if ((rtt_generation == RTT_GENERATIONS) && (rtt_offsets [0] == rtt_offsets [RTT_GENERATIONS])) {
		irqtimer_stop (tmr);
		rtt_silence = true;
		return;
	}
	//
	// Construct a buffer with the RTP packet
	rtt_timings [rtt_generation] = bottom_time () * 1000 / TIME_MSEC (1000);
	bzero (mem, sizeof (mem));
	//TODO// Setup mem[] with actual output pointers
	pout = netsend_udp6 (pout, mem);
	//TODO// RTP-pktgen is generic functionality, move it to a more central place
	pout [0] = 0x80;		// V=2, P=0, X=0, CC=0
	pout [1] = (rtt_redundancy? rtt_paytp_red: rtt_paytp_t140) | (rtt_silence? 0x80: 0x00);
	netset16 (*(nint16_t *) &pout [2], rtt_seqnr);
	netset32 (*(nint32_t *) &pout [4], rtt_timings [rtt_generation]);
	netset32 (*(nint32_t *) &pout [8], 0);		// SSRC -- TODO, 0 OK?
	rtt_seqnr++;
	pout += 12;
	if (rtt_redundancy) {
		idx2 = 1;
		idx = idx2;
		while (idx2 != rtt_generation) {
			uint32_t tdiff;
			tdiff = rtt_timings [rtt_generation] - rtt_timings [idx2];
			if (tdiff <= 16383) {
				uint16_t bytelen = (RTT_HISTBUFSZ + rtt_offsets [idx2] - rtt_offsets [idx2-1]) % RTT_HISTBUFSZ;
				*pout++ = 0x80 | rtt_paytp_t140;	/* Flag: more to come */
				netset16 (*(nint16_t *) pout, (tdiff << 2) | (bytelen >> 8));
				pout += 2;
				*pout++ = bytelen & 0xff;
			} else {
				// too old => skip here => also when sending text
				idx = idx2 + 1;
			}
			idx2++;
		}
		*pout++ = 0x00 | rtt_paytp_t140;			/* Flag: last */
	} else {
		idx = rtt_generation;
	}
	//
	// Copy bytes from cyclic buffer at rtt_offsets [idx-1]..rtt_offsets [rtt_generation]
	cplen = (RTT_HISTBUFSZ + rtt_offsets [rtt_generation] - rtt_offsets [idx-1]) % RTT_HISTBUFSZ;
	memcpy_cyc2lin (pout, rtt_buffer, rtt_offsets [idx-1], RTT_HISTBUFSZ, cplen);
	pout += cplen;
	//
	// Actually send the buffer
	mem [MEM_ALL_DONE] = (intptr_t) pout;
	netcore_send_buffer (mem, wbuf.data);
	//
	// Move on to the next historic version (optional with t.140)
	if (rtt_generation < RTT_GENERATIONS) {
		rtt_generation++;
		rtt_offsets [rtt_generation] = rtt_offsets [rtt_generation - 1];
	} else {
		uint8_t i = 0;
		while (i++ < RTT_GENERATIONS) {
			rtt_offsets [i - 1] = rtt_offsets [i];
			rtt_timings [i - 1] = rtt_timings [i];
		}
	}
	if (rtt_silence) {
		irqtimer_start (tmr, TIME_MSEC(300), rtt_send_buffer, CPU_PRIO_MEDIUM);
		rtt_silence = false;
	} else {
		irqtimer_restart (tmr, TIME_MSEC(300));
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
	uint16_t genofs;
	uint16_t genlen;
	//
	// Determine the number of bytes to send in this block
	// TODO: Maximum can be determined per session, and inlcude cps*0.3
	genofs = rtt_offsets [rtt_generation];
	plen = ((RTT_HISTBUFSZ + rtt_offsets [0] - 1) - genofs) % RTT_HISTBUFSZ;
	if (len < plen) {
		plen = len;
	}
	if (plen >= RTT_HISTBUFSZ / RTT_GENERATIONS) {
		plen = RTT_HISTBUFSZ / RTT_GENERATIONS;
	}
	//TODO// Reduce plen to cover a fixed number of chars
	//
	// Store values in the history
	memcpy_lin2cyc (rtt_buffer, genofs, RTT_HISTBUFSZ, keys, plen);
	genofs += plen;
	genofs %= RTT_HISTBUFSZ;
	rtt_offsets [rtt_generation] = genofs;
	//
	// Consider sending the current buffer when currently silent
	if (rtt_silence) {
		rtt_send_buffer (&rtt_timer.tmr_irq);	/* Send now, and repeat from now on */
	}
	//
	// Return the number of copied key bytes
	return plen;
}

