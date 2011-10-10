/* llconly.c -- Network functions covering LLC only.
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


/* This mini network stack is designed for very simple targets,
 * such as network testing or a bootloader.  It only supports
 * IEEE 802.2, that is, LLC.  And it even does a fairly modest
 * job at that.
 *
 * This code is suitable for running a network console over LLC2
 * and for running TFTP over LLC1.  It was not designed with any
 * higher purpose in mind.  It is not designed to integrate with
 * a more complete network stack like the phone application's.
 * Hence the name llconly.c!
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#include <config.h>

#include <0cpm/cons.h>
#include <0cpm/netinet.h>
#include <0cpm/netfun.h>



/* Construct an LLC reply start; fill out ethernet addresses and SSAP/DSAP */
uint8_t *netreply_llc (uint8_t *pout, intptr_t *mem) {
        memcpy (pout +  0, ((uint8_t *) mem [MEM_ETHER_HEAD] + 6), 6);
	bottom_flash_get_mac (pout + 6);
        pout [14] = mem [MEM_LLC_SSAP];
        pout [15] = mem [MEM_LLC_DSAP];
}


static bool llc_connected = false;
static uint8_t peer_sap;
static uint8_t peer_mac [6];
static uint8_t llc_ua [6 + 6 + 2 + 3];
static uint8_t llc_rr [6 + 6 + 2 + 4];
static uint8_t llc_sent;
static uint8_t llc_received;
static uint8_t llc_input;
static intptr_t mem [MEM_NETVAR_COUNT];


struct llc2 {
	uint8_t dummy;
};
static struct llc2 llc2_dummy_handle;


/* Dummy LLC2 send routine, ignoring "cnx" as there is just one LLC2 connection.
 * Before sending, the routine will first establish whether the last send
 * was successful; if not, it will repeat that.  The return value is true if at
 * least the new send was done, relying on future calls to resend if need be.
 */
uint8_t llc_pkt [100];
uint16_t llc_pktlen;
bool netsend_llc2 (struct llc2 *cnx, uint8_t *data, uint16_t datalen) {
	bool newpkt;
	if (datalen > 80) {
		return false;
	}
	if (!llc_connected) {
		return false;
	}
	newpkt = (llc_sent == llc_received);
	if (newpkt) {
		// Sending is complete, construct new packet as requested
		memcpy (llc_pkt +  0, peer_mac, 6);
		memcpy (llc_pkt +  6, "\x00\x0b\x82\x19\xa0\xf4", 6);
		llc_pkt [12] = 0x00;
		llc_pkt [13] = datalen + 4;
		llc_pkt [14] = peer_sap;		// DSAP
		llc_pkt [15] = 20;			// SSAP
		llc_pkt [16] = llc_sent << 1;		// N(S) = 0x00, information frame
		llc_pkt [17] = llc_input << 1;		// N(R) = sent-up-to-here, low bit reset
		memcpy (llc_pkt + 18, data, datalen);
		llc_pktlen = 6 + 6 + 2 + 4 + datalen;
		llc_sent++;
		llc_sent &= 0x7f;
	}
	bottom_network_send (llc_pkt, llc_pktlen);
	return newpkt;
}

/* LLC-only network packet handling, specifically for:
 *  - LLC2 console at SAP 20
 *  - LLC1 firmware access through TFTP at SAP 68
 */
void nethandler_llconly (uint8_t *pkt, uint16_t pktlen) {
	uint16_t typelen;
	uint16_t cmd;
	bool ack = false;
	bool type1;
	typelen = (pkt [12] << 8) | pkt [13];
#if 0
	if ((pktlen < 14) || (typelen < 46)) {
#ifdef CONFIG_MAINFUNCTION_DEVEL_NETWORK
		bottom_printf ("Unpadded packet length %d received\n", (intptr_t) pktlen);
#endif
		return;
	}
#endif
	if (typelen > 1500) {
#ifdef CONFIG_MAINFUNCTION_DEVEL_NETWORK
		bottom_printf ("Traffic is not LLC but protocol 0x%4x\n", (intptr_t) typelen);
#endif
		return;
	}
	if ((typelen > 64) && (typelen != pktlen - 14)) {
#ifdef CONFIG_MAINFUNCTION_DEVEL_NETWORK
		bottom_printf ("Illegal length %d received (pktlen = %d)\n", (intptr_t) typelen, (intptr_t) pktlen);
#endif
		return;
	}
#if 0
	if (pkt [14] != 20) {
#ifdef CONFIG_MAINFUNCTION_DEVEL_NETWORK
		bottom_printf ("Received LLC traffic for SAP %d instead of 20\n", pkt [14]);
#endif
		return;
	}
#endif
	cmd = pkt [16];
	type1 = (cmd & 0x03) == 0x03;
	if (!type1) {
		cmd |= pkt [17] << 8;
	}
	if (llc_connected && !type1) {
		if (memcmp (pkt + 6, peer_mac, 6) != 0) {
			// Peer MAC should be the constant while connected
			return;
		}
		if ((pkt [15] & 0xfe) != peer_sap) {
			// Peer SAP should be constant while connected
			return;
		}
	}
	if (cmd == 0x03) {				// UI (llc.datagram)
#ifdef CONFIG_FUNCTION_FIRMWARE_UPGRADES
		if (pkt [14] == 68) {
			uint16_t pktlen;
			// Setup minimal mem[] array for TFTP over LLC1
			bzero (mem, sizeof (mem));
			mem [MEM_ETHER_HEAD] = (intptr_t) pkt;
			mem [MEM_ALL_DONE] = (intptr_t) &pkt [pktlen];
			mem [MEM_LLC_DSAP] = pkt [14];
			mem [MEM_LLC_SSAP] = pkt [15];
			pkt = netllc_tftp (pkt, mem);
			pktlen = mem [MEM_ALL_DONE] - (intptr_t) pkt;
			// Send and forget -- LLC1 is unconfirmed transmission
			bottom_network_send (pkt, pktlen);
		} else {
#ifdef CONFIG_MAINFUNCTION_DEVEL_NETWORK
			bottom_printf ("LLC1 UA is only used for TFTP, use SAP 68 and not %d\n", (intptr_t) pkt [14]);
#endif
		}
#else
#ifdef CONFIG_MAINFUNCTION_DEVEL_NETWORK
		bottom_printf ("No bootloader -- ignoring TFTP over LLC1\n");
#endif
	} else if (pkt [14] != 20) {
#ifdef CONFIG_MAINFUNCTION_DEVEL_NETWORK
		bottom_printf ("To access the network console, use SAP 20 and not %d\n", (intptr_t) pkt [14]);
#endif
#endif
	} else if (cmd == 0x7f) {			// SABME (llc.connect)
		memcpy (peer_mac, pkt + 6, 6);
		peer_sap = pkt [15] & 0xfe;
		llc_sent = llc_received = llc_input = 0x00;
		llc_connected = true;
		netcons_connect (&llc2_dummy_handle);
		ack = true;
	} else if (cmd == 0x53) {			// DISC  (llc.disconnect)
		llc_connected = false;
		netcons_close ();
		ack = true;
	} else if (cmd == 0x87) {			// FRMR (llc.framereject)
		llc_received = llc_sent;
	} else if ((cmd & 0x0001) == 0x0000) {		// Data sent back (will be ignored)
		memcpy (llc_rr +  0, peer_mac, 6);
		memcpy (llc_rr +  6, "\x00\x0b\x82\x19\xa0\xf4", 6);
                memcpy (llc_rr + 12, "\x00\x04\x00\x15", 4);
		llc_rr [14] = peer_sap;
		llc_rr [16] = 0x01;			// supervisory, RR
		llc_rr [17] = (cmd + 2) & 0xfe;		// outgoing N(R) = incoming N(S) + 1
		bottom_network_send (llc_rr, sizeof (llc_rr));
	} else if ((cmd & 0x0007) == 0x0001) {		// Receiver ready / Receiver Reject
		llc_received = (cmd >> 9);
	} else {
#ifdef CONFIG_MAINFUNCTION_DEVEL_NETWORK
		bottom_printf ("Selfishly ignoring LLC traffic with cmd bytes 0x%2x%2x\n",
					(intptr_t) pkt [16], (intptr_t) pkt [17]);
#endif
	}
	if (ack) {
		memcpy (llc_ua +  0, peer_mac, 6);
		memcpy (llc_ua +  6, "\x00\x0b\x82\x19\xa0\xf4", 6);
		memcpy (llc_ua + 12, "\x00\x03\x00\x15\x73", 5);
		llc_ua [14] = peer_sap;
		// Try sending; assume repeat will be caused by other/smarter side
		bottom_network_send (llc_ua, sizeof (llc_ua));
	}
}

