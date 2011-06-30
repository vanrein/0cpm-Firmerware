/* netllc.c -- Optional LLC handling that plugs into the network stack.
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
#include <stdarg.h>

// #include <netinet/ip.h>
// #include <netinet/udp.h>
// #include <netinet/ip6.h>
// #include <netinet/ip_icmp.h>
// #include <netinet/icmp6.h>
// #include <netinet/if_ether.h>

#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/irq.h>
#include <0cpm/timer.h>
#include <0cpm/netinet.h>
#include <0cpm/netfun.h>
#include <0cpm/netdb.h>
#include <0cpm/cons.h>


/********** OPTIONAL FIRMWARE ACCESS FUNCTIONS **********/

/*
 * This functionality is located in src/function/bootloader.c
 */



/********** OPTIONAL CONSOLE ACCESS FUNCTIONS **********/
#ifdef CONFIG_FUNCTION_NETCONSOLE

extern uint8_t ether_mine [6];

static bool console_is_connected = false;
static uint8_t console_remote_mac [6];
static uint8_t console_remote_sap;

struct llc2 {
	uint8_t dummy;
};
static struct llc2 llc2_dummy_handle;

uint8_t llc_received = 0;
uint8_t llc_sent = 0;
uint8_t llc_input = 0;


static bool mismatch_console_connection (intptr_t *mem) {
	if ((mem [MEM_LLC_SSAP] & 0xfe) != console_remote_sap) {
		return true;
	}
	if (memcmp (((uint8_t *) mem [MEM_ETHER_HEAD]) + 6, console_remote_mac, 6)) {
		return true;
	}
	return false;
}


/* Construct an LLC reply start; fill out ethernet addresses and SSAP/DSAP */
uint8_t *netllc_reply (uint8_t *pout, intptr_t *mem) {
	memcpy (pout +  0, ((uint8_t *) mem [MEM_ETHER_HEAD] + 6), 6);
	memcpy (pout +  6, ether_mine, 6);
	pout [14] = mem [MEM_LLC_SSAP] | 0x00;	/* Individual */
	pout [15] = mem [MEM_LLC_DSAP] | 0x01;	/* Response */
}

/* Construct an LLC1 acknowledgement in reply to an incoming packet */
uint8_t *netllc_llc1_ack (uint8_t *pout, intptr_t *mem) {
	netllc_reply (pout, mem);
	netset16 (*(nint16_t *) (pout + 12), 3);
	netset8  (*(nint8_t  *) (pout + 16), 0x73);
	return pout + 17;
}

/* Dummy LLC2 send routine, ignoring "cnx" as there is just one LLC2 connection.
 * Before sending, the routine will first establish whether the last send
 * was successful; if not, it will repeat that.  The return value is true if at
 * least the new send was done, relying on future calls to resend if need be.
 * TODO: Possibly improve on this routine.
 */
uint8_t llc_pkt [100];
uint16_t llc_pktlen;
bool netsend_llc2 (struct llc2 *cnx, uint8_t *data, uint16_t datalen) {
        bool newpkt;
        if (datalen > 80) {
                return false;
        }
        if (!console_is_connected) {
                return false;
        }
        newpkt = (llc_sent == llc_received);
        if (newpkt) {
                // Sending is complete, construct new packet as requested
                memcpy (llc_pkt +  0, console_remote_mac, 6);
                memcpy (llc_pkt +  6, "\x00\x0b\x82\x19\xa0\xf4", 6);
                llc_pkt [12] = 0x00;
                llc_pkt [13] = datalen + 4;
                llc_pkt [14] = console_remote_sap;      // DSAP
                llc_pkt [15] = 20;                      // SSAP
                llc_pkt [16] = llc_sent << 1;           // N(S) = 0x00, information frame
                llc_pkt [17] = llc_input << 1;          // N(R) = sent-up-to-here, low bit reset
                memcpy (llc_pkt + 18, data, datalen);
                llc_pktlen = 6 + 6 + 2 + 4 + datalen;
                llc_sent++;
                llc_sent &= 0x7f;
        }
        bottom_network_send (llc_pkt, llc_pktlen);
        return newpkt;
}


/* Respond to an LLC SABMA console connection request */
uint8_t *netllc_console_sabme (uint8_t *pout, intptr_t *mem) {
#if 0
	if (console_is_connected) {
		return NULL;
	}
#endif
	memcpy (console_remote_mac, ((uint8_t *) mem [MEM_ETHER_HEAD]) + 6, 6);
	console_remote_sap = mem [MEM_LLC_SSAP] & 0xfe;
	llc_sent = llc_received = llc_input = 0x00;
//TODO: netcons_connect() overwrites memory buffer with 1st llc cnx pkt
netllc_llc1_ack (pout, mem);
bottom_network_send (pout, 6+6+2+3);
	if (!console_is_connected) {
		netcons_connect (&llc2_dummy_handle);
		console_is_connected = true;
	}
return NULL;
	return netllc_llc1_ack (pout, mem);
}

/* Respond to an LLC DISC console disconnect request */
uint8_t *netllc_console_disc (uint8_t *pout, intptr_t *mem) {
	if (mismatch_console_connection (mem)) {
		return NULL;
	}
	console_is_connected = false;
	netcons_close ();
	return netllc_llc1_ack (pout, mem);
}

/* Respond to an LLC FRMR frame rejection */
uint8_t *netllc_console_frmr (uint8_t *pout, intptr_t *mem) {
	if (mismatch_console_connection (mem)) {
		return NULL;
	}
	llc_received = llc_sent;
	return NULL;
}

/* Respond to an LLC data frame sent to the console.
 * As the console is output-only, the contents are dropped.
 */
uint8_t *netllc_console_datasentback (uint8_t *pout, intptr_t *mem) {
	if (mismatch_console_connection (mem)) {
		return NULL;
	}
	netllc_reply (pout, mem);
	netset16 (*(nint16_t *) (pout + 12), 4);
	netset8  (*(nint8_t  *) (pout + 16), 0x01);
	netset8  (*(nint8_t  *) (pout + 17), (mem [MEM_LLC_CMD] + 2) & 0xfe);
	return pout + 18;
}

/* Respond to an LLC receiver-ready (or receiver-reject) indication for the console */
uint8_t *netllc_console_receiverfeedback (uint8_t *pout, intptr_t *mem) {
	if (mismatch_console_connection (mem)) {
		return NULL;
	}
	llc_received = mem [MEM_LLC_CMD] >> 9;
	return NULL;
}


#endif


