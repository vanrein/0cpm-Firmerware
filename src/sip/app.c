/* sipapp.c -- SIP application handling.
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
#include <0cpm/led.h>	//TODO// TESTONLY
#include <0cpm/netinet.h>
#include <0cpm/netfun.h>
#include <0cpm/netdb.h>
#include <0cpm/text.h>
#include <0cpm/sip.h>
#include <0cpm/cons.h>	//TODO// TESTONLY


/*
 * This is the application layer of SIP.
 *
 * Its main function is to be aware of the
 * various methods and their meaning in terms
 * of what they change to call state.  This
 * knowledge is then communicated to the phone
 * level, where user-understood things happen,
 * such as ringing and exchanging sound.
 *
 * The main task of this layer could be said to
 * state the desired state of the phone layer,
 * triggered by the changes that are requested
 * by way of SIP methods.  The phone has an
 * idempotent API, meaning that it never adds
 * anything if the same request is repeated.
 * The reason for that is that the phone is
 * simply asked by the application layer to set 
 * its state so-and-so, instead of undergoing
 * a change.
 *
 * It could be said that a better designed SIP
 * protocol would have exchanged precisely such
 * desired-remote-state messages, instead of
 * changes to state.  Differential protocols
 * always lead to requirements of checking that
 * no changes have been lost, and none are
 * applied twice.  This would have saved a lot
 * of work in both transactional and application
 * layers.  Alas, that is not how influential
 * standards are made, not even in the IETF.
 */


/********** Global variables **********/


sipappmap_t sipapp_fnmap [] = {
	{ &invite_m, sipapp_invite },
	{ &cancel_m, sipapp_cancel },
	{ &bye_m, sipapp_bye },
	// { &hold_m, sipapp_hold },
	// { &resume_m, sipapp_resume },
	{ &refer_m, sipapp_refer },
	{ &register_m, sipapp_register },
	{ NULL, NULL }
};

static textptr_t const report_notallowed = { "405 Not Allowed", 15 };
static textptr_t const report_notimpl = { "501 Not Implemented", 19 };
static textptr_t const report_busyhere = { "486 Busy Here", 13 };
static textptr_t const report_trying = { "100 Trying", 10 };
static textptr_t const report_ringing = { "180 Ringing", 11 };
static textptr_t const report_notexist = { "481 Call/Transaction Does Not Exist", 35 };
static textptr_t const report_notfound = { "404 Not Found", 13 };
static textptr_t const report_notacceptable = { "488 Not Acceptable Here", 23 };
static textptr_t const report_urischeme = { "416 Unsupported URI Scheme", 26 };
static textptr_t const report_ok = { "200 OK", 6 };

static textptr_t const proto_sip = { "sip", 3 };


/*
 * The SIP application layer handles the various SIP methods
 * with knowledge about their individual meaning.  It runs on
 * top of the dialog and tract layers, and represents the UAS
 * interface to the phone.
 *
 * This is not the phone application; rather, it is the glue
 * logic between the generic message exchange facilities in
 * the siptr_XXX/sipdia_XXX modules and the user interface
 * aspects covered by the phoneXXX modules.
 *
 * What this layer does is receive requests over SIP, interact
 * with them as the phone's internals require.  For instance,
 * upon receiving an INVITE, it will validate the incoming
 * number and try to grab a free phone line, and fire off what
 * it takes to make it ring.
 *
 * The task of the sipapp_XXX routines is to handle upcalls in
 * response to SIP requests arriving over the network, in as
 * short a time as possible.
 */


/* An incoming INVITE is a request to setup a dialog with the
 * caller.  Before this may be done, the called number will be
 * verified against the number configured for each line.  If
 * the number exists, a line will be allocated and the phone
 * will be made to ring.
 */
void sipapp_invite (tract_t *tr, textptr_t const *requri, coreheaders_t *coreheaders, textptr_t const *sip, textptr_t const *sdp) {
	dialog_t *dia;
	linenr_t line;
	textptr_t proto, user, pass, dom;
	uint16_t port;
	bool stxok;
	bool newcall;
bottom_printf ("UAS received INVITE for %t\n", requri);
	//
	// If a transaction exists, then this is a resend
	// TODO: transactions should never exist when receiving a method!!!
	if (tr) {
		siptr_respond_again (tr, coreheaders, sip);
		return;
	}
	//
	// Send a provisional response to the sender
	//TODO:memhack-only-works-with-one-response-so-nothing-provisional// sipdia_respond (&report_trying, coreheaders, sip, NULL);
	//
	// Verify if the called party exists here
	stxok = sip_components_inuri (requri, &proto, &user, &pass, &dom, &port);
bottom_printf ("URI parser: retval=%d, proto=%t user=%t, pass=%t, dom=%t port=%d\n", (intptr_t) stxok, (intptr_t) &proto, (intptr_t) &user, (intptr_t) &pass, (intptr_t) &dom, (intptr_t) port);
	stxok = stxok && !textisnull (&user);
	stxok = stxok &&  textisnull (&pass);
	stxok = stxok && (port == 5060);
	if (!stxok) {
		sipdia_respond (&report_notacceptable, coreheaders, sip, NULL);
		return;
	}
	if (!texteq (&proto, &proto_sip)) {
		sipdia_respond (&report_urischeme, coreheaders, sip, NULL);
		return;
	}
{ textptr_t localuser = { "666*880", 7 };  // TODO:FIXED//
	if (!texteq (&user, &localuser)) {
		sipdia_respond (&report_notfound, coreheaders, sip, NULL);
		return;
	}
}
	//
	// Assign a line to this call
	newcall = (dia == NULL);
	if (newcall) {
		line = LINENR_NULL;
	} else {
		line = sipdia_getlinenr (dia);
	}
	if (line == LINENR_NULL) {
		if (true) {	//TODO// Allocate a line number
			line = 3;
		} else {
			sipdia_respond (&report_busyhere, coreheaders, sip, NULL);
			return;
		}
	}
	//
	// For new calls, create a new dialog
	// For all calls, create a new transaction
	if (newcall) {
		dia = sipdia_allocate (coreheaders, false);
		// dia==NULL result caught as tr==NULL below
	}
	tr = siptr_server (dia, &invite_m);
	if (!tr) {
		//TODO// Deallocate the line number
		sipdia_respond (&report_busyhere, coreheaders, sip, NULL);
		return;
	}
	//
	// Connect the phone line and dialog to each other
	sipdia_setlinenr (dia, line);
	// TODO: phoneline_setdialog (line, dia);
	//
	// Setup for ringing and fire up the ringing process
	// TODO: phoneline_setstate (line, PLS_RINGING);
	// TODO: phoneringer_update (line);
led_set (LED_IDX_MESSAGE, LED_FLASH (0,1), LED_FLASHTIME_FAST);
	//
	// Report "180 Ringing"
	sipdia_respond (&report_ringing, coreheaders, sip, NULL);
}

/* An incoming call is cancelled.  The phone should stop
 * ringing on any line that it might have made to ring.
 */
void sipapp_cancel (tract_t *tr, textptr_t const *requri, coreheaders_t *coreheaders, textptr_t const *sip, textptr_t const *sdp) {
	linenr_t line;
bottom_printf ("UAS received CANCEL for %t\n", requri);
	//
	// Ignore the request if this is not a dialog
	if (!tr->dialog) {
		sipdia_respond (&report_notexist, coreheaders, sip, NULL);
		return;
	}
	line = tr->dialog->linenr;
	//
	// Stop ringing the phone line for this dialog
	// TODO: phoneline_setstate (line, PLS_INACTIVE);
	// TODO: phoneringer_update (line);
bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_ON);
	//
	// Disconnect the phone line and dialog from each other
	//SAME_AS_BELOW// sipdia_setlinenr (tr->dialog, LINENR_NULL);
	sipdia_deallocate (tr->dialog);
	// TODO: phoneline_setdialog (line, NULL);
	sipdia_respond (&report_ok, coreheaders, sip, NULL);
}

/* An active dialog is terminated.
 */
void sipapp_bye (tract_t *tr, textptr_t const *requri, coreheaders_t *coreheaders, textptr_t const *sip, textptr_t const *sdp) {
	linenr_t line;
bottom_printf ("UAS received BYE for %t\n", requri);
	//
	// Ignore the request if this is not a dialog
	if (!tr->dialog) {
		sipdia_respond (&report_notexist, coreheaders, sip, NULL);
		return;
	}
	line = tr->dialog->linenr;
	//
	// Stop using the phone line for this dialog
	// TODO: phoneline_setstate (line, PLS_HANGUP);
	// TODO: phonetone_update (line);
bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_OFF);
	//
	// Disconnect the phone line and dialog from each other
	//SAME_AS_BELOW// sipdia_setlinenr (tr->dialog, LINENR_NULL);
	sipdia_deallocate (tr->dialog);
	// TODO: phoneline_setdialog (line, NULL);
	sipdia_respond (&report_ok, coreheaders, sip, NULL);
}

/* An active dialog is placed on hold.
 * TODO - Is "on hold" any different from a re-INVITE?
 */
void sipapp_hold (tract_t *tr, textptr_t const *requri, coreheaders_t *coreheaders, textptr_t const *sip, textptr_t const *sdp) {
	sipdia_respond (&report_notimpl, coreheaders, sip, NULL);
}

/* An active dialog that was placed on hold is being resumed.
 * TODO - Is "resume" any different from a re-INVITE?
 */
void sipapp_resume (tract_t *tr, textptr_t const *requri, coreheaders_t *coreheaders, textptr_t const *sip, textptr_t const *sdp) {
	sipdia_respond (&report_notimpl, coreheaders, sip, NULL);
}

/* A proposal to refer an active (possibly on-hold) dialog to another
 * location is being proposed.
 */
void sipapp_refer (tract_t *tr, textptr_t const *requri, coreheaders_t *coreheaders, textptr_t const *sip, textptr_t const *sdp) {
bottom_printf ("UAS received REFER for %t\n", requri);
	sipdia_respond (&report_notimpl, coreheaders, sip, NULL);
}

/* An external entity is trying to register with this phone.  This may
 * be a satellite that circles around a home base, possibly using a
 * redirection mechanism like mobile IPv6.
 */
void sipapp_register (tract_t *tr, textptr_t const *requri, coreheaders_t *coreheaders, textptr_t const *sip, textptr_t const *sdp) {
bottom_printf ("UAS received REGISTER for %t\n", requri);
	sipdia_respond (&report_notallowed, coreheaders, sip, NULL);
}

