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

typedef unsigned char uint8_t;

#include <0cpm/netdb.h>
#include <0cpm/netinet.h>
#include <0cpm/netfun.h>


#ifndef CONFIG_CODEC_RTT
#warning "Overriding deselection of Realtime Text codec -- see help in configuration menus"
#endif


/* The missing character code 0xfffd, encoded in UTF-8 */
uint8_t rtt_missing_text [] = { 0xef, 0xbf, 0xbd };

/* The newline character 0x2028, encoded in UTF-8 */
uint8_t rtt_linesep [] = { 0xe2, 0x80, 0xa8 };


static uint16_t rtt_seqnr;


static void rtt_skipheader (uint8_t **msg, uint16_t *len, uint16_t *newseqptr, uint16_t *gencountptr) {
	uint8_t hdrlen = 12;
	uint8_t gencount;
	if ((*msg) [0] & 0x20) {	/* "P" for padding? */
		*len -= (*msg) [*len - 1];
	}
	hdrlen += ((*msg) [0] & 0x0f) << 2;
	if ((*msg) [0] & 0x10) {	/* "X" for extension header? */
		hdrlen += netget16 (* (nint16_t *) &(*msg) [hdrlen]);

	}
	*newseqptr = netget16 (* (nint16_t *) &(*msg) [2]);
	if ((*msg) [1] & 0x80) {	/* "M" marks (re)start of a sequence */
		rtt_seqnr = *newseqptr - 1;
	}
	*msg += hdrlen;
	*len -= hdrlen;
	if (gencountptr) {
		uint8_t *msgptr = *msg;
		gencount = 1;
		while (*msgptr & 0x80) {
			msgptr += 4;
			gencount++;
		}
		*gencountptr = gencount;
	}
}


void rtp_paytype_text_t140 (uint8_t *msg, uint16_t len) {
	uint16_t newseq;
#ifdef TODO_WAKEUP_TEXTSHOW_PROCESS
	void rtt_recv_keys (uint8_t *text, uint16_t len);
#endif
	rtt_skipheader (&msg, &len, &newseq, NULL);
	switch ((int16_t) (newseq - rtt_seqnr)) {
	case -2:
	case -1:
	case 0:
		/* Replicated packet, known seqnr */
		return;
	default:
		/* Packets out of sync, report missing text */
#ifdef TODO_WAKEUP_TEXTSHOW_PROCESS
		rtt_recv_keys (rtt_missing_text, sizeof (rtt_missing_text));
#endif
		// ...continue into handling the one extension...
	case 1:
		/* Packets properly ordered */
#ifdef TODO_WAKEUP_TEXTSHOW_PROCESS
		rtt_recv_keys (msg, len);
#endif
		rtt_seqnr = newseq;
	}
}


void rtp_paytype_text_red (uint8_t *msg, uint16_t len) {
	uint16_t newseq;
	uint16_t gencount, skipcount, skipbytes;
#ifdef TODO_WAKEUP_TEXTSHOW_PROCESS
	void rtt_recv_keys (uint8_t *text, uint16_t len);
#endif
	rtt_skipheader (&msg, &len, &newseq, &gencount);
	//
	// Skip the redundant parts that were processed before
	if (gencount < (newseq - rtt_seqnr)) {
		/* Packets are missing -- report and use what is supplied */
#ifdef TODO_WAKEUP_TEXTSHOW_PROCESS
		rtt_recv_keys (rtt_missing_text, sizeof (rtt_missing_text));
#endif
		skipcount = 0;
	} else {
		/* No missing packets -- skip 0 or more redundant generations */
		skipcount = gencount - (newseq - rtt_seqnr);
	}
	rtt_seqnr = newseq;
	skipbytes = 0;
	while (skipcount-- > 0) {
		uint16_t blklen = ((msg [2] & 0x03) << 8) | msg [3];
		skipbytes += blklen;
		len -= 4;
		if ((msg [0] & 0x80) == 0x00) {
			break;	// This must have been the last one
		}
		msg += 4;
	}
	while ((msg [0] & 0x80) == 0x80) {
		/* Continue to skip headers, but produce their bytes later on */
		len -= 4;
		msg += 4;
	}
	msg++;	/* Skip the primary element header */
	len--;
	msg += skipbytes;	/* Skip the duplicated bytes (from skipcount) */
	len -= skipbytes;
	//
	// Copy the redundant parts that are new
	if (((int16_t) len) > 0) {
#ifdef TODO_WAKEUP_TEXTSHOW_PROCESS
		rtt_recv_keys (msg, len);
#endif
	}
}


