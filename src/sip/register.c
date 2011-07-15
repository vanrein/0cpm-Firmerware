/* sipregister.c -- Register phone lines with location services.
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
#include <stdarg.h>  //TODO// ONLY_FOR_TESTING

#include <config.h>

#include <0cpm/text.h>
#include <0cpm/cpu.h>
#include <0cpm/irq.h>
#include <0cpm/timer.h>
#include <0cpm/netinet.h>
#include <0cpm/netfun.h>
#include <0cpm/netdb.h>
#include <0cpm/sip.h>
#include <0cpm/cons.h>  //TODO// ONLY_FOR_TESTING


/********** Fixed strings and other settings **********/


textptr_t const register_m = { "REGISTER", 8 };


/********** Global variables **********/


extern struct ip6binding ip6binding [IP6BINDING_COUNT];
extern packet_t wbuf, rbuf;


/********** Registration support functions **********/


/* Parameterised registration of a phone line with an account */
static bool sipreg_generic (dialog_t *dia, uint32_t expires, textptr_t *branch) {
	intptr_t mem [MEM_NETVAR_COUNT];
	char *msg;
	uint16_t len;
	//
	// Create the UDP/IPv6 headers
	bzero (mem, sizeof (mem));
	mem [MEM_BINDING6] = (intptr_t) &ip6binding [0];
	mem [MEM_ETHER_DST] = /* TODO:perhaps_in:netsend_udp6 */
		(intptr_t) "\xbc\x05\x43\x70\x4f\xd3";  //FIXED: 10.0.0.1 Fritz!Box
	mem [MEM_IP6_DST] = /* TODO:proxy.0cpm.net */
		(intptr_t) "\x20\x01\x16\xf8\x00\x16\x00\x00\x00\x00\x05\x19\x10\x75\x33\x33";  //FIXED: welcome.0cpm.nl
	mem [MEM_UDP6_DST_PORT] = 5060;
	mem [MEM_UDP6_SRC_PORT] = 5060;
	msg = (char *) netsend_udp6 (wbuf.data, mem);
	//
	// StartLine: REGISTER sip:lineservice SIP/2.0
{textptr_t reghost_uri = { "sip:welcome.0cpm.nl", 19 };	//FIXED//
	msg = sipgen_request (msg, &register_m, &reghost_uri, branch);
}
	//
	// Further headers
	msg = sipgen_from (msg, dia);
	msg = sipgen_to_register (msg, dia);
	msg = sipgen_contact (msg, dia);
	msg = sipgen_expires (msg, expires);
	msg = sipgen_cseq (msg, &register_m, dia->cseqnr);
	msg = sipgen_call_id (msg, dia);
	msg = sipgen_max_forwards (msg);
	msg = sipgen_user_agent (msg);
	msg = sipgen_sdp_headers (msg, NULL);
	msg = sipgen_attach_body (msg, NULL);
	//
	// Send to upstream server for this dialog
	mem [MEM_ALL_DONE] = (intptr_t) msg;
	len = mem [MEM_ALL_DONE] - mem [MEM_ETHER_HEAD];
	if ((len > 0) && (len < 1500 + 18)) {
		netcore_send_buffer (mem, wbuf.data);
// wbuf.data [len] = 0;
// bottom_printf ("Sent SIP message: %t\n", wbuf);
bottom_printf ("Sent SIP REGISTER message\n");
		return true;
	} else {
		return false;
	}
}

/* Start registration of a dialog with an account */
bool sipreg_start (dialog_t *dia, textptr_t *branch) {
	return sipreg_generic (dia, 3600, branch);
}

/* Stop registering a dialog with an account */
bool sipreg_stop (dialog_t *dia, textptr_t *branch) {
	return sipreg_generic (dia, 0, branch);
}

