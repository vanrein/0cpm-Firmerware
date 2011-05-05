/* netconsole.c -- run a console over LLC over ethernet
 *
 * This test program includes a very small (and very selfish)
 * network stack: It only does LLC2, and welcomes one LAN peer
 * at a time to connect to SAP 20 where a console is running.
 *
 * Console log events are saved up in a buffer, and reported
 * as soon as the client connects.  From then on, new entries
 * sent to the console are also sent immediately.  An entry is
 * made for incoming broadcast messages.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/timer.h>
#include <0cpm/led.h>
#include <0cpm/kbd.h>
#include <0cpm/app.h>
#include <0cpm/show.h>
#include <0cpm/cons.h>


#define onlinetest_top_main top_main


bool online = false;


void top_timer_expiration (timing_t timeout) {
	/* Keep the linker happy */ ;
}

void top_hook_update (bool offhook) {
	/* Keep the linker happy */ ;
}

void top_button_press (buttonclass_t bcl, buttoncode_t cde) {
	/* Keep the linker happy */ ;
}

void top_button_release (void) {
	/* Keep the linker happy */ ;
}

void top_network_online (void) {
	online = true;
}

void top_network_offline (void) {
	online = false;
}

void top_network_can_send (void) {
	/* Keep the linker happy */ ;
}

void top_network_can_recv (void) {
	/* Keep the linker happy */ ;
}


static bool llc_connected = false;
static uint8_t peer_sap;
static uint8_t peer_mac [6];
static uint8_t llc_ua [6 + 6 + 2 + 3];
static uint8_t llc_rr [6 + 6 + 2 + 4];
static uint8_t llc_sent;
static uint8_t llc_received;


/* Dummy LLC2 send routine, ignoring "cnx" as there is just one LLC2 connection.
 * This is out-only, so N(R) always sends as 0x00 and N(S) increments.
 * Before sending, the routine will first establish whether the last send
 * was successful; if not, it will repeat that.  The return value is true if
 * at least the send was done, relying on future calls to resend if need be.
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
		llc_pkt [17] = 0x00;			// N(R) = sent-up-to-here, low bit reset
		memcpy (llc_pkt + 18, data, datalen);
		llc_pktlen = 6 + 6 + 2 + 4 + datalen;
		llc_sent++;
		llc_sent &= 0x7f;
	}
	bottom_network_send (llc_pkt, llc_pktlen);
	return newpkt;
}

struct llc2 {
	uint8_t dummy;
};
static struct llc2 llc2_dummy_handle;

/* LLC2-only network packet handling */
void selfish_llc2_handler (uint8_t *pkt, uint16_t pktlen) {
	uint16_t typelen;
	uint16_t cmd;
	bool ack = false;
	typelen = (pkt [12] << 8) | pkt [13];
#if 0
	if ((pktlen < 14) || (typelen < 46)) {
		bottom_printf ("Unpadded packet length %d received\n", (unsigned int) pktlen);
		return;
	}
#endif
	if (typelen > 1500) {
		// bottom_printf ("Traffic is not LLC but protocol 0x%x\n", (unsigned int) typelen);
		return;
	}
	if ((typelen > 64) && (typelen != pktlen)) {
		bottom_printf ("Illegal length %d received (pktlen = %d)\n", (unsigned int) typelen, (unsigned int) pktlen);
		return;
	}
	if (pkt [14] != 20) {
		bottom_printf ("Received LLC traffic for SAP %d instead of 20\n", (unsigned int) pkt [14]);
		return;
	}
	if (llc_connected) {
		if (memcmp (pkt + 6, peer_mac, 6) != 0) {
			// Peer MAC should be the constant while connected
			return;
		}
		if ((pkt [15] & 0xfe) != peer_sap) {
			// Peer SAP should be constant while connected
			return;
		}
	}
	cmd = pkt [16];
	if (typelen > 3) {
		cmd |= pkt [17] << 8;
	}
	if (cmd == 0x007f) {				// SABME (llc.connect)
		memcpy (peer_mac, pkt + 6, 6);
		peer_sap = pkt [15] & 0xfe;
		llc_sent = llc_received = 0x00;
		llc_connected = true;
		netcons_connect (&llc2_dummy_handle);
		ack = true;
	} else if (cmd == 0x0053) {			// DISC  (llc.disconnect)
		llc_connected = false;
		netcons_close ();
		ack = true;
	} else if ((cmd & 0x0001) == 0x0000) {		// Data sent back (will be ignored)
		memcpy (llc_rr +  0, peer_mac, 6);
		memcpy (llc_rr +  6, "\x00\x0b\x82\x19\xa0\xf4", 6);
                memcpy (llc_rr + 12, "\x00\x04\x00\x15", 4);
		llc_rr [14] = peer_sap;
                // memcpy (llc_rr + 12, "\x00\x04\x14\x00", 4);
		// llc_rr [15] = peer_sap | 0x01;
		llc_rr [16] = 0x01;			// supervisory, RR
		llc_rr [17] = (cmd + 2) & 0xfe;		// outgoing N(R) = incoming N(S) + 1
		bottom_network_send (llc_rr, sizeof (llc_rr));
	} else if ((cmd & 0x0007) == 0x0001) {		// Receiver ready / Receiver Reject
		llc_received = (cmd >> 9);
	} else {
		bottom_printf ("Selfishly ignoring LLC traffic with cmd bytes 0x%x 0x%x\n", (uint32_t) pkt [16], (uint32_t) pkt [17]);
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


uint8_t netinput [1000];
uint16_t netinputlen;

void onlinetest_top_main (void) {
	bottom_critical_region_end ();
	bottom_led_set (LED_IDX_BACKLIGHT, LED_STABLE_ON);
	while (true) {
		if (online) {
			bottom_show_fixed_msg (APP_LEVEL_BACKGROUNDED, FIXMSG_READY);
			netinputlen = sizeof (netinput);
			if (bottom_network_recv (netinput, &netinputlen)) {
				if (memcmp (netinput, "\xff\xff\xff\xff\xff\xff", 6) == 0) {
					bottom_printf ("Broadcast!\n");
#if 0
					bottom_printf ("Broadcast from %x:%x:%x:%x:%x:%x\n",
							(unsigned int) netinput [ 6],
							(unsigned int) netinput [ 7],
							(unsigned int) netinput [ 8],
							(unsigned int) netinput [ 9],
							(unsigned int) netinput [10],
							(unsigned int) netinput [11]);
#endif
				} else {
					bottom_led_set (LED_IDX_BACKLIGHT, LED_STABLE_OFF);
					selfish_llc2_handler (netinput, netinputlen);
				}
			}
		} else {
			bottom_show_fixed_msg (APP_LEVEL_BACKGROUNDED, FIXMSG_OFFLINE);
			bottom_led_set (LED_IDX_BACKLIGHT, LED_STABLE_ON);
		}
	}
}

