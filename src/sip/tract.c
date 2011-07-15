/* siptract.c -- Transaction layer for SIP.
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
#include <stdarg.h>	//TODO// TESTING_ONLY

#include <config.h>

#include <0cpm/text.h>
#include <0cpm/cpu.h>
#include <0cpm/irq.h>
#include <0cpm/timer.h>
#include <0cpm/sip.h>
#include <0cpm/cons.h>	//TODO// TESTING_ONLY


/* Timer values, defaults applied according to page 265 of RFC 3261 */

#define T1 TIME_MSEC(500)
#define T2 TIME_MSEC(4000)
#define T4 TIME_MSEC(5000)

#define TA_init T1
#define TB (64*T1)
#define TC TIME_SEC(200)
#define TD TIME_SEC (45)
#define TE_init T1
#define TF (64*T1)
#define TG_init T1
#define TH (64*T1)
#define TI T4
#define TJ (64*T1)
#define TK T4


/*
 * SIP transactions are defined in RFC 3261.  There are four variants,
 * due to two variation factors: UAC or UAS role, and INIVITE or other.
 *
 * The procedures below initiate the desired flavour of SIP transaction
 * based on the state diagram.  They initiate a chain of interrupts,
 * several of which may be timer interrupts, and aim to eventually
 * report back to the upper layer that wants to assume transactional
 * semantics for the delivery of a message.
 *
 * The actual message to send is generated as part of the transaction,
 * so all the information required to construct it must have been
 * setup for the line addressed.  As a result, this module will only
 * need the line number to be able to act.
 *
 * There should not be multiple processes running at the same time
 * on the same dialog/line.  Also, before initiating a dialog for
 * submission of an INVITE in the UAC role, the dialog must have been
 * allocated.  Upon reception of an INVITE in the UAS role, the dialog
 * will be allocated on the fly.
 */


extern dialog_t dialog [HAVE_LINES];


#ifndef HAVE_TRACTS
#  define HAVE_TRACTS (HAVE_LINES*16)
#endif

static tract_t trs [HAVE_TRACTS];
static uint32_t trnr_next = 0;

/* A few simple reports to relay back to the transaction user */
//TODO// Change form by removing "SIP/2.0 " prefix
static textptr_t const report_timeout = { "SIP/2.0 408 Request Timeout\r\n\r\n", 31 };
static textptr_t const report_badreq  = { "SIP/2.0 400 Bad Request\r\n\r\n", 27 };
static textptr_t const report_tmpunavail = { "SIP/2.0 480 Temporarily Unavailable\r\n\r\n", 39 };
static textptr_t const report_mthnotallow = { "SIP/2.0 405 Method Not Allowed\r\n\r\n", 34 };
static textptr_t const report_calltrnotexist = { "SIP/2.0 481 Call/Transaction Does Not Exist", 43 };

/* A definition of a textptr_t constant with the branch prefix from RFC 3261 */
const textptr_t branchprefix_rfc3261 = { BRANCHPREFIX_RFC3261, BRANCHPREFIXLEN_RFC3261 };



/********** GENERAL FUNCTIONS **********/


/* Setup the transaction layer */
void siptr_initialise (void) {
	uint16_t i;
	for (i=0; i<HAVE_TRACTS; i++) {
		trs [i].state = TRS_TERMINATED;
	}
}

/* Find a free transaction (looking for state == TRS_TERMINATED) */
static tract_t *tract_find_free (void) {
	uint16_t trnr = trnr_next;
	uint16_t ctr;
	for (ctr = HAVE_TRACTS; ctr > 0; ctr--) {
		if (trs [trnr].state == TRS_TERMINATED) {
			trnr_next = trnr + 1;
			if (trnr_next >= HAVE_TRACTS) {
				trnr_next = 0;
			}
			return &trs [trnr];
		} else {
			trnr++;
			if (trnr >= HAVE_TRACTS) {
				trnr = 0;
			}
		}
	}
	return NULL;
}

/* Allocate a new transaction  */
static tract_t *tract_allocate (dialog_t *dia) {
	tract_t *retval;
	if (!dia) {
		return NULL;
	}
	retval = tract_find_free ();
	if (!retval) {
		return NULL;
	}
	bzero (retval, sizeof (tract_t));
	retval->state = TRS_ALLOCATED;
	retval->dialog = dia;
	dia->tract_usectr++;
	retval->branch.str = retval->txt_branch;
	retval->branch.len = 0;
	sipgen_create_branch (&retval->branch);
	return retval;
}

/* Clone a transaction as a new one with shared identities.
 * This is used for CANCEL and, at least in theory, for ACK.
 */
static tract_t *tract_clone (tract_t *tr) {
	tract_t *retval = tract_find_free ();
	if (!retval) {
		return NULL;
	}
	memcpy (retval, tr, sizeof (tract_t));
	retval->state = TRS_ALLOCATED;
	tr->dialog->tract_usectr++;
	retval->branch.str = retval->txt_branch;
	return retval;
}

/* Deallocate a transaction */
static inline void tract_deallocate (tract_t *tr) {
	tr->dialog->tract_usectr--;
	tr->dialog = NULL;
	tr->state = TRS_TERMINATED;
}

/* Find a transaction that matches the core headers.
 * This implements the RFC 3261 form of transaction
 * identification, based on a special Via:branch form.
 * This is usually requested by the UAS functions, so
 * in response to an incoming request.
 */
tract_t *siptr_findmatch (coreheaders_t const *coreheaders) {
	uint16_t trnr;
	//
	// No match if the Via:branch is not in RFC3261 format
	if (textisnull (&coreheaders->via_branch) || (memcmp (&coreheaders->via_branch, branchprefix_rfc3261.str, branchprefix_rfc3261.len) != 0)) {
		return NULL;
	}
	//
	// Iterate over transactions to find a match
	for (trnr = 0; trnr < HAVE_TRACTS; trnr++) {
		//
		// Skip if transaction is not allocated/used
		if (trs [trnr].state == TRS_TERMINATED) {
			continue;
		}
		//
		// Return transaction if its branch equals Via:branch
		if (texteq (&trs [trnr].branch, &coreheaders->via_branch)) {
			return &trs [trnr];
		}
	}
	//
	// No transaction found (yet) -- return NULL
	return NULL;
}

/* Report back to the user, when transactions need it */
static inline void siptr_report (tract_t *st, const textptr_t *msg) {
	memcpy (&st->incoming, msg, sizeof (textptr_t));
	(*st->userfn) (&st->incoming, &st->attachmt);
}


/********** TRANSACTION HANDLING FUNCTIONS **********/


/*

 * SIP INVITE in a UAC role, figure 5 on page 128 of RFC 3261:

                               |INVITE from TU
             Timer A fires     |INVITE sent
             Reset A,          V                      Timer B fires
             INVITE sent +-----------+                or Transport Err.
               +---------|           |---------------+inform TU
               |         |  Calling  |               |
               +-------->|           |-------------->|
                         +-----------+ 2xx           |
                            |  |       2xx to TU     |
                            |  |1xx                  |
    300-699 +---------------+  |1xx to TU            |
   ACK sent |                  |                     |
resp. to TU |  1xx             V                     |
            |  1xx to TU  -----------+               |
            |  +---------|           |               |
            |  |         |Proceeding |-------------->|
            |  +-------->|           | 2xx           |
            |            +-----------+ 2xx to TU     |
            |       300-699    |                     |
            |       ACK sent,  |                     |
            |       resp. to TU|                     |
            |                  |                     |      NOTE:
            |  300-699         V                     |
            |  ACK sent  +-----------+Transport Err. |  transitions
            |  +---------|           |Inform TU      |  labeled with
            |  |         | Completed |-------------->|  the event
            |  +-------->|           |               |  over the action
            |            +-----------+               |  to take
            |              ^   |                     |
            |              |   | Timer D fires       |
            +--------------+   | -                   |
                               |                     |
                               V                     |
                         +-----------+               |
                         |           |               |
                         | Terminated|<--------------+
                         |           |
                         +-----------+

                 Figure 5: INVITE client transaction

 */

static void isr_client_invite (irq_t *irq) {
	tract_t *st = (tract_t *) irq;
	textptr_t retval, descr;
	//
	// See if a response came in
	if (!textisnull (&st->incoming)) {
		sip_splitline_request (&st->incoming, &retval, &descr);
	} else {
		retval.str = NULL;
	}
	//
	// Decide on the next state
	switch (st->state) {
	case TRS_CALLING:
		if (retval.str) {
			(*st->userfn) (&st->incoming, &st->attachmt);	/* Pass back retval */
			if (*retval.str == '1') {
				st->state = TRS_PROCEEDING;
				irqtimer_stop (&st->tmr);
			} else if (*retval.str == '2') {
				// st->state = TRS_TERMINATED;
				irqtimer_stop (&st->tmr);
				tract_deallocate ((tract_t *) irq);
			} else {
				sip_ack (st->dialog, &st->branch);
				st->state = TRS_COMPLETED;
				irqtimer_stop (&st->tmr);
				irqtimer_start (&st->tmr, TD, isr_client_invite, CPU_PRIO_LOW);
			}
		} else {
			st->growtime <<= 1;
			if (TIME_BEFORE (T4, st->growtime)) {
				siptr_report (st, &report_timeout);
			} else if (!sipdia_invite (st->dialog, &st->branch)) {
				siptr_report (st, &report_badreq);
			} else {
				irqtimer_start (&st->tmr, st->growtime, isr_client_invite, CPU_PRIO_LOW);
			}
		}
		break;
	case TRS_PROCEEDING:
		if (retval.str) {
			(*st->userfn) (&st->incoming, &st->attachmt);	/* Pass back retval */
			if (*retval.str == '1') {
				;
			} else if (*retval.str == '2') {
				// st->state = TRS_TERMINATED;
				tract_deallocate ((tract_t *) irq);
			} else {
				st->state = TRS_COMPLETED;
				sip_ack (st->dialog, &st->branch);
			}
		}
		break;
	case TRS_COMPLETED:
		if (retval.str) {
			sip_ack (st->dialog, &st->branch);
		} else {
			// st->state = TRS_TERMINATED;
			tract_deallocate ((tract_t *) irq);
		}
		break;
	case TRS_TERMINATED:
	default:
		break;
	}
	textnullify (&st->incoming);
}


/*

 * SIP non-INVITE in a UAC role, figure 6 on page 133 of RFC 3261:

                                   |Request from TU
                                   |send request
               Timer E             V
               send request  +-----------+
                   +---------|           |-------------------+
                   |         |  Trying   |  Timer F          |
                   +-------->|           |  or Transport Err.|
                             +-----------+  inform TU        |
                200-699         |  |                         |
                resp. to TU     |  |1xx                      |
                +---------------+  |resp. to TU              |
                |                  |                         |
                |   Timer E        V       Timer F           |
                |   send req +-----------+ or Transport Err. |
                |  +---------|           | inform TU         |
                |  |         |Proceeding |------------------>|
                |  +-------->|           |-----+             |
                |            +-----------+     |1xx          |
                |              |      ^        |resp to TU   |
                | 200-699      |      +--------+             |
                | resp. to TU  |                             |
                |              |                             |
                |              V                             |
                |            +-----------+                   |
                |            |           |                   |
                |            | Completed |                   |
                |            |           |                   |
                |            +-----------+                   |
                |              ^   |                         |
                |              |   | Timer K                 |
                +--------------+   | -                       |
                                   |                         |
                                   V                         |
             NOTE:           +-----------+                   |
                             |           |                   |
         transitions         | Terminated|<------------------+
         labeled with        |           |
         the event           +-----------+
         over the action
         to take
 
                 Figure 6: non-INVITE client transaction

 */

static void isr_client_other (irq_t *irq) {
	tract_t *st = (tract_t *) irq;
	textptr_t retval, descr;
	//
	// See if a response came in
	if (!textisnull (&st->incoming)) {
		sip_splitline_request (&st->incoming, &retval, &descr);
	} else {
		textnullify (&retval);
		textnullify (&descr);
	}
	//
	// Decide on the next state
bottom_printf ("isr_client_other @ %c\n", (intptr_t) st->state);
	switch (st->state) {
	case TRS_TRYING:
		if (retval.str) {
			(*st->userfn) (&st->incoming, &st->attachmt);	/* Pass back retval */
			if (*retval.str == '1') {
				st->state = TRS_PROCEEDING;
				// Continue with current timer setup
			} else {
				st->state = TRS_COMPLETED;
				irqtimer_stop (&st->tmr);
				irqtimer_start (&st->tmr, TK, isr_client_other, CPU_PRIO_LOW);
			}
		} else {
			st->growtime <<= 1;
			if (TIME_BEFORE (TF, st->growtime)) {
				siptr_report (st, &report_timeout);
			} else if (!(*st->dialogfn) (st->dialog, &st->branch)) {
				siptr_report (st, &report_badreq);
			} else {
				irqtimer_start (&st->tmr, st->growtime, isr_client_other, CPU_PRIO_LOW);
			}
		}
		break;
	case TRS_PROCEEDING:
		if (retval.str) {
			(*st->userfn) (&st->incoming, &st->attachmt);	/* Pass back retval */
			if (*retval.str == '1') {
			} else {
				st->state = TRS_COMPLETED;
				irqtimer_stop (&st->tmr);
				irqtimer_start (&st->tmr, TK, isr_client_other, CPU_PRIO_LOW);
			}
		} else {
			st->growtime <<= 1;
			if (TIME_BEFORE (TF, st->growtime)) {
				siptr_report (st, &report_timeout);
			} else if (!(*st->dialogfn) (st->dialog, &st->branch)) {
				siptr_report (st, &report_badreq);
			} else {
				irqtimer_start (&st->tmr, st->growtime, isr_client_other, CPU_PRIO_LOW);
			}
		}
		break;
	case TRS_COMPLETED:
		if (retval.str) {
			;
		} else {
			// st->state = TRS_TERMINATED;
			tract_deallocate ((tract_t *) irq);
		}
	case TRS_TERMINATED:
	default:
		break;
	}
	textnullify (&st->incoming);
}

void siptr_client (dialog_t *dia, textptr_t const *mth, void (*truser) (textptr_t const *sip, textptr_t *sdp)) {
	bool (*dialogfunction) (void);
	tract_t *tr = NULL;
	dialogfunction = sipdia_client_method (mth);
	if (!dialogfunction) {
		truser (&report_mthnotallow, NULL);
		return;
	}
	//
	// The behaviour for CANCEL is exceptional:
	// Send it in a transaction cloned from INVITE
	if (dialogfunction == sipdia_cancel) {
		uint16_t trnr;
		for (trnr = 0; trnr < HAVE_TRACTS; trnr++) {
			if (trs [trnr].dialog != dia) {
				continue;
			}
			if (texteq (trs [trnr].method, &invite_m)) {
				tr = tract_clone (&trs [trnr]);
				break;
			}
		}
	//
	// For normal methods, create a new transaction:
	} else {
		tr = tract_allocate (dia);
	}
	if (tr == NULL) {
		truser (&report_tmpunavail, NULL);
		return;
	}
	tr->userfn = truser;
	tr->dialogfn = dialogfunction;
	tr->method = mth;
	textnullify (&tr->incoming);
	textnullify (&tr->attachmt);
	if (dialogfunction == sipdia_invite) {
		tr->state = TRS_CALLING;
		tr->growtime = TA_init;
		tr->tmr.tmr_irq.irq_handler = isr_client_invite;
	} else {
		tr->state = TRS_TRYING;
		tr->growtime = TE_init;
		tr->tmr.tmr_irq.irq_handler = isr_client_other;
	}
	(*tr->tmr.tmr_irq.irq_handler) (&tr->tmr.tmr_irq);
}


/* Network-entry of a response ends up at a dialog for further processing */

void siptr_response2uac (dialog_t *dia, textptr_t *sip, textptr_t *attachment, textptr_t *branch, textptr_t *cseqmth) {
	uint16_t trnr;
	for (trnr = 0; trnr < HAVE_TRACTS; trnr++) {
		//
		// Find the transaction that matches the incoming response
		if (trs [trnr].dialog != dia) {
			continue;
		}
		if (trs [trnr].dialog->tract_usectr == 0) {
			continue;
		}
		if (!texteq (&trs [trnr].branch, branch)) {
bottom_printf ("Branch mismatch: %t != %t\n", &trs [trnr].branch, branch);
			continue;
		}
		if (!texteq (trs [trnr].method, cseqmth)) {
bottom_printf ("Method mismatch: %t != %t\n", trs [trnr].method, cseqmth);
			continue;
		}
		//
		// Instead of transaction timeout, call the handler
bottom_printf ("Processing upcall for response\n");
		irqtimer_stop (&trs [trnr].tmr);
		memcpy (&trs [trnr].incoming, sip,        sizeof (textptr_t));
		memcpy (&trs [trnr].attachmt, attachment, sizeof (textptr_t));
#if 0
		if (memcmp (sip, "INVITE ", 7)) {
			isr_client_other  (&trs [trnr].tmr.tmr_irq);
		} else {
			isr_client_invite (&trs [trnr].tmr.tmr_irq);
		}
#endif
		(*trs [trnr].tmr.tmr_irq.irq_handler) (&trs [trnr].tmr.tmr_irq);
		return;
	}
}


/*

 * SIP INVITE in a UAC role, figure 7 on page 136 of RFC 3261:

                               |INVITE
                               |pass INV to TU
            INVITE             V send 100 if TU won't in 200ms
            send response+-----------+
                +--------|           |--------+101-199 from TU
                |        | Proceeding|        |send response
                +------->|           |<-------+
                         |           |          Transport Err.
                         |           |          Inform TU
                         |           |--------------->+
                         +-----------+                |
            300-699 from TU |     |2xx from TU        |
            send response   |     |send response      |
                            |     +------------------>+
                            |                         |
            INVITE          V          Timer G fires  |
            send response+-----------+ send response  |
                +--------|           |--------+       |
                |        | Completed |        |       |
                +------->|           |<-------+       |
                         +-----------+                |
                            |     |                   |
                        ACK |     |                   |
                        -   |     +------------------>+
                            |        Timer H fires    |
                            V        or Transport Err.|
                         +-----------+  Inform TU     |
                         |           |                |
                         | Confirmed |                |
                         |           |                |
                         +-----------+                |
                               |                      |
                               |Timer I fires         |
                               |-                     |
                               |                      |
                               V                      |
                         +-----------+                |
                         |           |                |
                         | Terminated|<---------------+
                         |           |
                         +-----------+

              Figure 7: INVITE server transaction

 */
static void isr_server_invite (irq_t *irq) {
	//TODO// CREATE THIS ROUTINE
}


/*

 * SIP non-INVITE in a UAC role, figure 8 on page 140 of RFC 3261:

                                  |Request received
                                  |pass to TU
                                  V
                            +-----------+
                            |           |
                            | Trying    |-------------+
                            |           |             |
                            +-----------+             |200-699 from TU
                                  |                   |send response
                                  |1xx from TU        |
                                  |send response      |
                                  |                   |
               Request            V      1xx from TU  |
               send response+-----------+send response|
                   +--------|           |--------+    |
                   |        | Proceeding|        |    |
                   +------->|           |<-------+    |
            +<--------------|           |             |
            |Trnsprt Err    +-----------+             |
            |Inform TU            |                   |
            |                     |                   |
            |                     |200-699 from TU    |
            |                     |send response      |
            |  Request            V                   |
            |  send response+-----------+             |
            |      +--------|           |             |
            |      |        | Completed |<------------+
            |      +------->|           |
            +<--------------|           |
            |Trnsprt Err    +-----------+
            |Inform TU            |
            |                     |Timer J fires
            |                     |-
            |                     |
            |                     V
            |               +-----------+
            |               |           |
            +-------------->| Terminated|
                            |           |
                            +-----------+

                Figure 8: non-INVITE server transaction

 */
static void isr_server_other (irq_t *irq) {
	//TODO// CREATE THIS ROUTINE
}

/* A downcall from the SIP application code, to create
 * a server transaction -- which is of course a reaction
 * to an incoming SIP message.
 */
tract_t *siptr_server (dialog_t *dia, textptr_t const *mth) {
	tract_t *tr = tract_allocate (dia);
	if (tr) {
		//TODO// CREATE THIS ROUTINE
	}
	return tr;
}

/* A downcall from the SIP application code, to send
 * a SIP response to a current SIP request.  Note that
 * the SDP body, if any, is assumed to be buffered by
 * the application layer.
 */
void siptr_respond (tract_t *tr, textptr_t const *response, coreheaders_t const *coreheaders, textptr_t const *sip, textptr_t const *sdp_buffered) {
	//TODO// PROCESS RESPONSE IN TRANSACTION STATE
	memcpy (&tr->lastresponse, response, sizeof (textptr_t));
	if (sdp_buffered) {
		memcpy (&tr->lastsdp, sdp_buffered, sizeof (textptr_t));
	} else {
		bzero  (&tr->lastsdp,               sizeof (textptr_t));
	}
	sipdia_respond (response, coreheaders, sip, sdp_buffered);
}

/* A downcall from the SIP application code, to resend
 * the last SIP response.
 */
void siptr_respond_again (tract_t *tr, coreheaders_t *coreheaders, textptr_t const *sip) {
	if (!textisnull (&tr->lastresponse)) {
		sipdia_respond (&tr->lastresponse, coreheaders, sip, &tr->lastsdp);
	}
}

/* An upcall from the network stack, to handle an
 * incoming SIP request.  This is basically passed
 * up to SIP application code, depending on the
 * method.
 * The dialog in dia_opt is optional, and if it is
 * not supplied then either a transaction may be
 * found directly through the Via:branch, or else
 * there is no current dialog.
 */
void siptr_request2uas (dialog_t *dia_opt, coreheaders_t *coreheaders, textptr_t const *sip, textptr_t const *attachment) {
	tract_t *tr = NULL;
	textptr_t method;
	textptr_t requri;
	sipappmap_t *fnmap = sipapp_fnmap;
	textptr_t const *retval = NULL;
bottom_printf ("Invoked siptr_request2uas() -- TODO: Integrate with state diagrams\n");
	//
	// Split start-line into method and request-uri
	if (!sip_splitline_request (sip, &method, &requri)) {
		// Send "400 Bad Request"
		sipdia_respond (&report_badreq, coreheaders, sip, NULL);
		return;
	}
bottom_printf ("Method is %t, URI is %t\n", &method, &requri);
	//
	// Iterate over available method names to find an upcall
	while (fnmap->method) {
		if (texteq (fnmap->method, &method)) {
			break;
		}
		fnmap++;
	}
	if (!fnmap->method) {
		// Send "405 Method Not Allowed"
		// TODO: ACK should be handled -- in the app or here?
		sipdia_respond (&report_mthnotallow, coreheaders, sip, NULL);
		return;
	}
	//
	// Find the transaction, if it already exists.
	// DO NOT be so kind to always create a transaction.
	// This choice for overhead is made by the application,
	// if it cannot avoid doing so.
	if (!dia_opt) {
		tr = siptr_findmatch (coreheaders);
		if (tr) {
			dia_opt = tr->dialog;
		}
	}
	//
	// TODO -- integrate transaction state diagram here
	//
	// Make an upcall to the corresponding sipapp_XXX handler
	(*fnmap->sipappfn) (tr, &requri, coreheaders, sip, attachment);
}

