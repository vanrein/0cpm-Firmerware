/* sipdialog.c -- Manage dialogs (call legs).
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
#include <stdarg.h>	//TODO// TESTONLY
#include <string.h>

#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/irq.h>
#include <0cpm/timer.h>
#include <0cpm/netinet.h>
#include <0cpm/netfun.h>
#include <0cpm/netdb.h>
#include <0cpm/text.h>
#include <0cpm/sip.h>
#include <0cpm/cons.h>	//TODO// TESTONLY



/********** Global variables **********/


#ifndef HAVE_DIALOGS
#  define HAVE_DIALOGS (HAVE_LINES*8)
#endif

dialog_t dialog [HAVE_DIALOGS];

extern packet_t wbuf, rbuf;
extern struct ip6binding ip6binding [IP6BINDING_COUNT];

textptr_t const coreheader_name [COREHDR_COUNT] = {
	{ "From", 4 },
	{ "To", 2 },
	{ "Call-id", 7 },
	{ "Contact", 7 },
	{ "Via", 3 },
	{ "Cseq", 4 },
	{ "Route", 5 }
};


/********** Fixed strings and other settings **********/


textptr_t const invite_m = { "INVITE", 6 };
textptr_t const cancel_m = { "CANCEL", 6 };
textptr_t const bye_m    = { "BYE", 3 };
textptr_t const refer_m  = { "REFER", 5 };
textptr_t const ack_m    = { "ACK", 3 };
extern textptr_t const register_m;

static struct { textptr_t const *name; sipdia_climth const constructor; } climth_mapping [] = {
	&invite_m,	sipdia_invite,
	&cancel_m,	sipdia_cancel,
	&bye_m,		sipdia_bye,
	&refer_m,	sipdia_refer,
	// &ack_m,	sipdia_ack,
	&register_m,	sipreg_start,
	NULL, NULL
};

static const textptr_t at = { "@", 1 };
static const textptr_t colon = { ":", 1 };


/********** GENERAL FUNCTIONS **********/


void sipdia_initialise (void) {	//TODO:DEPRECATE:but_leave:siptr_init//
	uint16_t dianr;
	for (dianr = 0; dianr < HAVE_DIALOGS; dianr++) {
		dialog [dianr].tract_usectr = 0;
		dialog [dianr].linenr = LINENR_NULL;
	}
	siptr_initialise ();
}


/* Allocate a free dialog structure.  The dialog is kept alive
 * until it has no active transactions running against it and
 * it does not have an assigned phoneline.
 * The coreheaders and myrequest values are used to initiate the
 * tag_local, tag_remot and tag_callid identifying values.
 * If the coreheaders are not provided, tag_local and tag_callid
 * will be generated at random. (TODO)
 */
dialog_t *sipdia_allocate (coreheaders_t const *coreheaders, bool myrequest) {
	uint16_t dianr = 0;
	dialog_t *dia;
	uint16_t strlen;
	textptr_t const *lsrc;
	textptr_t const *rsrc;
	while (dianr < HAVE_DIALOGS) {
		if ((dialog [dianr].tract_usectr == 0)
		&& (dialog [dianr].linenr == LINENR_NULL)) {
			dia = &dialog [dianr];
			break;
		}
		dianr++;
	}
	if (!dia) {
		return NULL;
	}
	//
	// Basic initialisation of the dialog
	bzero (dia, sizeof (dialog_t));
	dia->cseqnr = 101;	/* Some think this is fun */
	//
	// If no coreheaders are provided, fill in random values
	if (!coreheaders) {
		char *tagstr;
		uint8_t random [4];
		int ctr;
		tagstr = dia->txt_callid;
		dia->tag_callid.str = tagstr;
		dia->tag_callid.len = 48;
		bottom_rnd_pseudo (random, 4);
		for (ctr = 0; ctr < 4; ctr++) {
			tagstr = hexcat (tagstr, random [ctr], 2);
		}
		tagstr = txtcat (tagstr, &at);
		for (ctr = 0; ctr < 16; ctr += 2) {
			if (ctr > 0) {
				tagstr = txtcat (tagstr, &colon);
			}
			tagstr = hexcat (tagstr, ip6binding [0].ip6addr [ctr+0], 2);
			tagstr = hexcat (tagstr, ip6binding [0].ip6addr [ctr+1], 2);
		}
		tagstr = dia->txt_local;
		dia->tag_local.str = tagstr;
		dia->tag_local.len = 16;
		tagstr = hexcat (tagstr, (uint32_t) bottom_time (), 8);
		bottom_rnd_pseudo (random, 4);
		for (ctr = 0; ctr < 4; ctr++) {
			tagstr = hexcat (tagstr, random [ctr], 2);
		}
		return dia;
	}
	//
	// Clone local and remote values, and the callid
	if (myrequest) {
		lsrc = &coreheaders->from_tag;
		rsrc = &coreheaders->to_tag;
	} else {
		lsrc = &coreheaders->to_tag;
		rsrc = &coreheaders->from_tag;
	}
	if (!textisnull (lsrc)) {
		strlen = lsrc->len;
		if (strlen > sizeof (dia->txt_local)) {
			strlen = sizeof (dia->txt_local);
		}
		memcpy (dia->txt_local, lsrc->str, strlen);
		dia->tag_local.str = dia->txt_local;
		dia->tag_local.len = strlen;
	}
	if (!textisnull (rsrc)) {
		strlen = rsrc->len;
		if (strlen > sizeof (dia->txt_remot)) {
			strlen = sizeof (dia->txt_remot);
		}
		memcpy (dia->txt_remot, rsrc->str, strlen);
		dia->tag_remot.str = dia->txt_remot;
		dia->tag_remot.len = strlen;
	}
	if (!textisnull (&coreheaders->callid)) {
		strlen = coreheaders->callid.len;
		if (strlen > sizeof (dia->txt_callid)) {
			strlen = sizeof (dia->txt_callid);
		}
		memcpy (dia->txt_callid, coreheaders->callid.str, strlen);
		dia->tag_callid.str = dia->txt_callid;
		dia->tag_callid.len = strlen;
	}
	return dia;
}


/* Deallocate a dialog structure or, more accurately, prepare
 * it for disbanding as soon as all transactions have ended.
 * The core act here is to remove any setting of line number
 * in this dialog. 
 */
void sipdia_deallocate (dialog_t *dia) {
	dia->linenr = LINENR_NULL;
}


/* Move to the next Cseq: serial number in a dialog.
 */
void sipdia_next_cseq (dialog_t *dia) {
	dia->cseqnr++;
}

/* Assign a phone line to a given dialog.  This may also be used
 * to remove a phone line, by setting it to LINENR_NULL.  While
 * the phone line is set to a value unequal to LINENR_NULL, the
 * dialog will not be deallocated for reuse.
 */
void sipdia_setlinenr (dialog_t *dia, linenr_t ln) {
	dia->linenr = ln;
}

/* Retrieve the phone line assigned to a given dialog. */
linenr_t sipdia_getlinenr (dialog_t *dia) {
	return dia->linenr;
}


/* Map a methodname to a dialog creating function.
 */
sipdia_climth sipdia_client_method (textptr_t const *mth) {
	uint8_t mapidx = 0;
	while (climth_mapping [mapidx].name) {
		if (texteq (climth_mapping [mapidx].name, mth)) {
			return climth_mapping [mapidx].constructor;
		}
		mapidx++;
	}
	return NULL;
}


/********** DIALOG MANAGEMENT CLIENT FUNCTIONS **********/


/* Start an outgoing dialog on a given dialog */
bool sipdia_invite (dialog_t *dia, textptr_t *branch) {
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
	// StartLine: INVITE to_uri SIP/2.0
{textptr_t to_uri = { "sip:7425*880@welcome.0cpm.nl:5060", 33 };	//FIXED//
	msg = sipgen_request (msg, &invite_m, &to_uri, branch);
}
	//
	// Further headers
	msg = sipgen_from (msg, dia);
{textptr_t callee = { "7425*880", 8 };	//FIXED//
	msg = sipgen_to (msg, &callee, dia);
}
msg = sipgen_contact (msg, dia);	//TODO// Only needed for SIPproxy64?
	msg = sipgen_cseq (msg, &invite_m, dia->cseqnr);
	msg = sipgen_call_id (msg, dia);
	msg = sipgen_max_forwards (msg);
	msg = sipgen_user_agent (msg);
{textptr_t sdp = { "v=0\r\n"
"o=- 111416154 111416154 IN IP4 10.0.0.186\r\n"
"s=-\r\n"
"c=IN IP6 2001:610:66f:5060::1\r\n"
"t=0 0\r\n"
"m=audio 14000 RTP/AVP 0 8 101\r\n"
"a=rtpmap:0 PCMU/8000\r\n"
"a=rtpmap:8 PCMA/8000\r\n"
"a=rtpmap:101 telephone-event/8000\r\n"
"a=fmtp:101 0-15\r\n"
"a=ptime:20\r\n"
"a=sendrecv\r\n" , 242 };
	msg = sipgen_sdp_headers (msg, &sdp);
	msg = sipgen_attach_body (msg, &sdp);
}
	//
	// Send to upstream server for this dialog
	mem [MEM_ALL_DONE] = (intptr_t) msg;
	len = mem [MEM_ALL_DONE] - mem [MEM_ETHER_HEAD];
	if ((len > 0) && (len < 1500 + 18)) {
		netcore_send_buffer (mem, wbuf.data);
		return true;
	} else {
		return false;
	}
}

/* Stop an outgoing dialog because nobody seems to be responding */
bool sipdia_cancel (dialog_t *dia, textptr_t *branch) {
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
	// StartLine: CANCEL to_uri SIP/2.0
{textptr_t to_uri = { "sip:7425*880@welcome.0cpm.nl:5060", 33 };	//FIXED//
	msg = sipgen_request (msg, &cancel_m, &to_uri, branch);
}
	//
	// Further headers 
	msg = sipgen_from (msg, dia);
{textptr_t callee = { "7425*880", 8 };	//FIXED//
	msg = sipgen_to (msg, &callee, dia);
}
msg = sipgen_contact (msg, dia);	//TODO// Only needed for SIPproxy64?
	msg = sipgen_cseq (msg, &cancel_m, dia->cseqnr);
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
		return true;
	} else {
		return false;
	}
}

/* Accept an incoming dialog on a given dialog */
bool sipdia_accept (dialog_t *dia, textptr_t *branch) {
	// StartLine: SIP/2.0 200 OK
	// Headers: 
}

/* Reject an incoming dialog on a given dialog */
bool sipdia_reject (dialog_t *dia, textptr_t *branch) {
	// StartLine: SIP/2.0 486 Busy Here
	// Headers:
}

/* Enqueue an incoming dialog for later acceptance */
bool sipdia_enqueue (dialog_t *dia, textptr_t *branch) {
	// StartLine: SIP/2.0 182 Queued
	// Headers:
}

/* Terminate a dialog on a given dialog */
bool sipdia_bye (dialog_t *dia, textptr_t *branch) {
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
	// StartLine: BYE to_uri SIP/2.0
{textptr_t to_uri = { "sip:7425*880@welcome.0cpm.nl:5060", 33 };	//FIXED//
	msg = sipgen_request (msg, &bye_m, &to_uri, branch);
}
	//
	// Further headers 
	msg = sipgen_from (msg, dia);
{textptr_t callee = { "7425*880", 8 };	//FIXED//
	msg = sipgen_to (msg, &callee, dia);
}
msg = sipgen_contact (msg, dia);	//TODO// Only needed for SIPproxy64?
	msg = sipgen_cseq (msg, &bye_m, dia->cseqnr);
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
		return true;
	} else {
		return false;
	}
}

/* Place a dialog on hold */
bool sipdia_hold (dialog_t *dia, textptr_t *branch) {
}

/* Take a dialog off hold */
bool sipdia_resume (dialog_t *dia, textptr_t *branch) {
}

/* Redirect a call to another number */
bool sipdia_refer (dialog_t *dia, textptr_t *branch) {
	// StartLine: REFER to_uri SIP/2.0
	// Headers:
}


/* Send a SIP ACK message */
bool sip_ack (dialog_t *dia, textptr_t *branch) {
	intptr_t mem [MEM_NETVAR_COUNT];
	char *msg;
	uint16_t len;
bottom_printf ("Constructing ACK\n");
	// Headers: 
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
	// StartLine: ACK to_uri SIP/2.0
{textptr_t to_uri = { "sip:7425*880@welcome.0cpm.nl", 28 };	//FIXED//
	msg = sipgen_request (msg, &ack_m, &to_uri, branch);
}
	//
	// Further headers
	msg = sipgen_from (msg, dia);
{textptr_t callee = { "7425*880", 8 };	//FIXED//
	msg = sipgen_to (msg, &callee, dia);
}
	msg = sipgen_cseq (msg, &ack_m, dia->cseqnr);
	msg = sipgen_call_id (msg, dia);
	msg = sipgen_max_forwards (msg);
	msg = sipgen_user_agent (msg);
	msg = sipgen_sdp_headers (msg, NULL);
	msg = sipgen_attach_body (msg, NULL);
	//
	// Send to upstream server for this dialog
	mem [MEM_ALL_DONE] = (intptr_t) msg;
	len = mem [MEM_ALL_DONE] - mem [MEM_ETHER_HEAD];
bottom_printf ("Length of ACK is %d\n", (intptr_t) len);
	if ((len > 0) && (len < 1500 + 18)) {
bottom_printf ("Sending ACK\n");
		netcore_send_buffer (mem, wbuf.data);
		return true;
	} else {
		return false;
	}
}


/********** DIALOG MANAGEMENT SERVER FUNCTIONS **********/


static const textptr_t parname_tag = { "tag", 3 };
static const textptr_t parname_branch = { "branch", 6 };


static intptr_t *uasmem; //TODO:FINDBETTERWAY//
void sipdia_respond (textptr_t const *response, coreheaders_t const *coreheaders, textptr_t const *sip, textptr_t const *sdp_opt) {
	uint8_t *pout;
	//
	// Construct the response
	pout = wbuf.data;
	pout = netreply_udp6 (pout, uasmem);
	pout = (uint8_t *) sipgen_response ((char *) pout, response, coreheaders, sip, sdp_opt);
	//
	// Send the response
	uasmem [MEM_ALL_DONE] = (intptr_t) pout;
	netcore_send_buffer (uasmem, wbuf.data);
}


/* Extract the value of a named parameter from a SIP header */
void extract_named_parameter (textptr_t const *hdr, textptr_t const *parname, textptr_t *parvalue) {
	textptr_t p_nm, p_val;
	textnullify (parvalue);
	if ((!textisnull (hdr)) && sip_firstparm_inheader (hdr, &p_nm, &p_val)) {
		do {
bottom_printf ("Found parameter \"%t\" with value \"%t\"\n", (intptr_t) &p_nm, (intptr_t) &p_val);
			if (texteq (&p_nm, parname)) {
				memcpy (parvalue, &p_val, sizeof (textptr_t));
			}
		} while (sip_nextparm_inheader (hdr, &p_nm, &p_val));
	}
}


/* Parse the customary headers in a SIP message and return
 * the headers and parts of them in coreheaders.  If something
 * is not found or does not parse, it will be a null text.
 */
static void net_sip_h_parser (textptr_t const *sip, coreheaders_t *coreheaders) {
	textptr_t h_nm, h_val;
	//
	// Initialise the header strings
	bzero (coreheaders, sizeof (coreheaders_t));
	coreheaders->sip = sip->str;
	if (sip_firstheader (sip, &h_nm, &h_val)) {
		do {
			int idx;
			for (idx = 0; idx < COREHDR_COUNT; idx++) {
				if (textisnull (&coreheaders->headers [idx]) && texteq (&h_nm, &coreheader_name [idx])) {
					memcpy (&coreheaders->headers [idx], &h_val, sizeof (textptr_t));
				}
			}
		} while (sip_nextheader (sip, &h_nm, &h_val));
	}
	//
	// Extract tags and branch (or set them to NULL strings)
	extract_named_parameter (&coreheaders->headers [COREHDR_FROM], &parname_tag,    &coreheaders->from_tag  );
	extract_named_parameter (&coreheaders->headers [COREHDR_TO  ], &parname_tag,    &coreheaders->to_tag    );
	extract_named_parameter (&coreheaders->headers [COREHDR_VIA ], &parname_branch, &coreheaders->via_branch);
bottom_printf ("Received top From: header as %t\n", (intptr_t) &coreheaders->headers [COREHDR_FROM]);
bottom_printf ("Received top Via:  header as %t\n", (intptr_t) &coreheaders->headers [COREHDR_VIA]);
bottom_printf ("Received top Via: branch is %t\n", (intptr_t) &coreheaders->via_branch);
	//
	// Extract the callid value from its header (or set it to a NULL string)
	memcpy (&coreheaders->callid, &coreheaders->headers [COREHDR_CALL_ID], sizeof (coreheaders->callid));
	//
	// Extract the cseq method from its header (or set it to a NULL string)
	sip_split_cseq (&coreheaders->headers [COREHDR_CSEQ], &coreheaders->cseq_serialno, &coreheaders->cseq_method);
bottom_printf ("Received Cseq: method is %t\n", (intptr_t) &coreheaders->cseq_method);
}


/* Try to find a matching dialog for the core headers.
 * The values compared are the following:
 *  - Call-id: header value
 *  - From:tag
 * The parameters influence the operation as follows:
 *  - coreheaders provides the core headers and inferred info in the current request
 *  - myrequest   determines if From:to and To:tag are local or remote
 * This function returns NULL if no matching dialog could be found.
 */
dialog_t *sipdia_findmatch (coreheaders_t const *coreheaders, bool const myrequest) {
	uint16_t dianr;
	bool have_to_tag;
	have_to_tag = !textisnull (&coreheaders->to_tag);
	for (dianr = 0; dianr < HAVE_DIALOGS; dianr++) {
		//
		// Only work on allocated/used dialogs
		if ((dialog [dianr].tract_usectr == 0) && (dialog [dianr].linenr == LINENR_NULL)) {
			continue;
		}
		//
		// Ensure a matching Call-id:
		if (!texteq (&dialog [dianr].tag_callid,   &coreheaders->callid  )) {
bottom_printf ("Call-id: mismatch with \"%t\"\n", (intptr_t) &dialog [dianr].tag_callid);
			continue;
		}
		//
		// Ensure a matching From:tag (which is always present)
		if (!texteq (&coreheaders->from_tag, myrequest? &dialog [dianr].tag_local: &dialog [dianr].tag_remot)) {
bottom_printf ("From:tag mismatch with \"%t\"\n", (intptr_t) &dialog [dianr].tag_local);
			continue;
		}
		//
		// Ensure a matching To:tag if one is present
		if (have_to_tag) {
			textptr_t *internal_to = myrequest? &dialog [dianr].tag_remot: &dialog [dianr].tag_local;
			if (!textisnull (internal_to)) {
				if (!texteq (&coreheaders->to_tag, internal_to)) {
bottom_printf ("To:tag mismatch with \"%t\"\n", (intptr_t) internal_to);
					continue;
				}
			} else {
				//
				// Possibly learn the To:tag from an incoming response message
				if (myrequest && texteq (&coreheaders->cseq_method, &invite_m) && (memcmp (coreheaders->sip, "SIP/2.0 2", 9) == 0)) {
					uint16_t len = coreheaders->to_tag.len;
					if (len > 128) {
						len = 128;
					}
					memcpy (dialog [dianr].txt_remot, coreheaders->to_tag.str, len);
					dialog [dianr].tag_remot.str = dialog [dianr].txt_remot;
					dialog [dianr].tag_remot.len = len;
bottom_printf ("To:tag learnt as \"%t\"\n", (intptr_t) &dialog [dianr].tag_remot);
				}
			}
		}
#if TODO_OLD_TIMES_REVIVE
		if (!textisnull (&dialog [dianr].tag_remot)) {
			if (!texteq (&dialog [dianr].tag_remot, &coreheaders.to_tag  )) {
bottom_printf ("To:tag mismatch with \"%t\"\n", (intptr_t) &dialog [dianr].tag_remot);
				continue;
			}
		} else if (texteq (&coreheaders.cseq_method, &invite_m) && (coreheaders->sip [8] == '2')) {
			uint16_t len = coreheaders.to_tag.len;
			if (len > 128) {
				len = 128;
			}
			memcpy (dialog [dianr].txt_remot, coreheaders.to_tag.str, len);
			dialog [dianr].tag_remot.str = dialog [dianr].txt_remot;
			dialog [dianr].tag_remot.len = len;
bottom_printf ("To:tag learnt as \"%t\"\n", (intptr_t) &dialog [dianr].tag_remot);
		}
#endif
		//
		// Found a match!  Now proudly return it
bottom_printf ("sipdia_findmatch() Returning dialog found\n");
		return &dialog [dianr];
	}
bottom_printf ("sipdia_findmatch(): No matching dialog found\n");
	return NULL;
}


/* An incoming SIP request from the network.
 * Depending on the to Via:branch, it may not be necessary to find the dialog.
 * Instead, it may be possible to directly locate the transaction (which
 * references a dialog) if the Via:branch starts with "z9hG4bK".  If this is
 * the case, the dialog will be sent up to the transaction layer as NULL.
 * Another case in which the dialog is sent up as NULL is when there is no
 * dialog yet; a new one will be established only if the application layer
 * wants to.  The transaction layer can easily distinguish between the
 * two cases of a NULL dialog by looking at the Via:branch.
 */
static uint8_t *net_sip_request2uas (textptr_t *sip, textptr_t *attachment) {
	bool rfc3261branch;
	dialog_t *dia_opt = NULL;
	coreheaders_t coreheaders;
bottom_printf ("Called net_sip_request2uas()\n");
	//
	// Parse standard headers and extract tags: Call-id, From
	net_sip_h_parser (sip, &coreheaders);
	if (textisnull (&coreheaders.callid) || textisnull (&coreheaders.from_tag)) {
		return NULL;
	}
	//
	// Try to find the matching dialog, and trigger it;
	// but first use the Via: tag to locate a transaction
	rfc3261branch = (memcmp (coreheaders.via_branch.str, branchprefix_rfc3261.str, branchprefix_rfc3261.len) == 0);
	if (!rfc3261branch) {
		dia_opt = sipdia_findmatch (&coreheaders, false);
	}
	//
	// If no match is found, disapprove all but INVITE methods
	// TODO: This is an application layer check -- move it there
	if ((!rfc3261branch) && (!dia_opt) && (memcmp (sip->str, "INVITE ", 7) != 0)) {
bottom_printf ("Ignoring non-INVITE without pre-existing dialog\n");
		return NULL;	// Out-of-dialog non-INVITE	//TODO: Send "481 Call/Transaction Does Not Exist"
	}
	//
	// Approved, whether dialog is found or not
	siptr_request2uas (dia_opt, &coreheaders, sip, attachment);
	//
	// Do not return a packet to send
	return NULL;
}

/* An incoming SIP response from the network */
static uint8_t *net_sip_response2uac (textptr_t *sip, textptr_t *attachment) {
	dialog_t *dia;
	coreheaders_t coreheaders;
	//
	// Parse standard headers and extract tags: Call-id, From, To
	net_sip_h_parser (sip, &coreheaders);
	if (textisnull (&coreheaders.callid) || textisnull (&coreheaders.from_tag) || textisnull (&coreheaders.via_branch) || textisnull (&coreheaders.cseq_method)) {
		return NULL;
	}
	//
	// Try to find the matching dialog, and trigger it
	//TODO// First use the Via: tag to locate a transaction?
	dia = sipdia_findmatch (&coreheaders, true);
	if (dia) {
		siptr_response2uac (dia, sip, attachment, &coreheaders.via_branch, &coreheaders.cseq_method);
		return NULL;
	}
	//
	// No match found -- so this response is invalid and will be ignored
	return NULL;
}


/* An incoming SIP message from the network */
uint8_t *net_sip (uint8_t *msg, intptr_t *mem) {
	textptr_t sip, attachmt;
	//
	// Prepare SIP content for further parsing
	sip.str = (char *) mem [MEM_SIP_HEAD];
	sip.len = mem [MEM_ALL_DONE] - mem [MEM_SIP_HEAD];
bottom_printf ("sip.len prior to scrubbing: %d\n", (intptr_t) sip.len);
	sip_normalise (&sip, &attachmt);
bottom_printf ("sip.len after    scrubbing: %d\n", (intptr_t) sip.len);
bottom_printf ("SIP contents \"\"\"%t\"\"\"\n", &sip);
	//
	// Take notes of the most interesting headers
	//
	// Decide how to further handle the SIP content
	if (memcmp (sip.str, "SIP/2.0 ", 8) == 0) {
		//
		// Verify the response header, then process it
		if ((sip.str [ 8] < '0') || (sip.str [ 8] > '9')) {
			return NULL;
		}
		if ((sip.str [ 9] < '0') || (sip.str [ 9] > '9')) {
			return NULL;
		}
		if ((sip.str [10] < '0') || (sip.str [10] > '9')) {
			return NULL;
		}
		if (sip.str [11] != ' ') {
			return NULL;
		}
{int code = (sip.str [8] << 8) + (sip.str [9] << 4) + (sip.str [10]) - 0x3330;
bottom_printf ("Received response: %x\n", (intptr_t) code);
bottom_printf ("Response code in characters: %c%c%c\n", (intptr_t) sip.str [8] & 0xff, (intptr_t) sip.str [9] & 0xff, (intptr_t) sip.str [10] & 0xff);
}
		return net_sip_response2uac (&sip, &attachmt);
	} else {
		//
		// Verify the request header, then process it
		int sp = 0, idx = 0;
		while ((idx < sip.len) && (sip.str [idx] != '\r') && (sip.str [idx] != '\n')) {
			if (sip.str [idx++] == ' ') {
				sp++;
			}
		}
		if ((sp != 2) || (idx < 10) || (idx == sip.len)) {
			return NULL;
		}
		if (memcmp (sip.str + idx - 7, "SIP/2.0", 7) != 0) {
			return NULL;
		}
{textptr_t x; x.str = sip.str; x.len = idx; bottom_printf ("Received request: %t\n", &x); }
uasmem = mem; //TODO:FINDBETTERWAY//
		return net_sip_request2uas (&sip, &attachmt);
	}
}

