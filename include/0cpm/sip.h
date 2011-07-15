/* SIP message handling.
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


#ifndef HEADER_SIP
#define HEADER_SIP


/*
 * DIALOG, TRANSACTION, LINE
 *
 * What is known to a phone user as a call leg, is called a Dialog in
 * SIP terminology.  To quote RFC 3261,
 *
 *    A dialog is a peer-to-peer SIP relationship between two UAs that
 *    persists for some time.  A dialog is established by SIP messages,
 *    such as a 2xx response to an INVITE request.  A dialog is
 *    identified by a call identifier, local tag, and a remote tag.
 *    A dialog was formerly known as a call leg in RFC 2543.
 * 
 * We use the term a bit broader than the RFC, by including early
 * dialogs.  That means that a dialog may exist before it has been
 * accepted by both sides.
 *
 * A transaction is a message exchange between two SIP speakers, and
 * and exists solely to describe the reliable exchange of a message.
 * At any time, a dialog consists of zero or more transactions that
 * may be in progress.  Zero transactions may apply to a call that
 * is in progress, and multiple transactions may apply when a few
 * actions occur in rapid sequence.
 *
 * Some dialogs make it all the way to become a call leg, and are
 * attached to a line.  A line is the user-level concept of a call
 * leg endpoint, it is the unit that may be answered, put on hold,
 * pulled into a conference call or kicked out of it, and so on.
 * This is the end goal of a dialog started with an INVITE.  A
 * dialog with a line attached is what is termed as a dialog in
 * RFC 3261.
 *
 * Dialogs are identified with a local tag and a call-id and, where
 * available, a remote tag.  Transactions are attached to a dialog
 * and are further distinguished as described in section 17.1.3.
 *
 * Note that a transaction can linger on for some time after it
 * appears to have ended; this may happen if repeated sends must
 * be fended in some way.  Any dialogs under which such transactions
 * fall will also survive, even after they have been detached from
 * a phone line.
 *
 * The general rule is that a dialog exists as long as it is either
 * attached to a phone line, or has transactions in progress.  The
 * creation of a new dialog is done in the layer that uses the
 * transactions and handles SIP messages; when requests enter the
 * phone, they are sent up as transactions, for which the SIP-aware
 * layer may then have to create a dialog.
 *
 * Many fields in a dialog occur on two sides.  Sometimes the
 * traffic's direction helps in determining where information
 * should go; in those cases, field names are X_send and X_recv.
 * In other cases, descriptions are tied to the user agent
 * itself; in those cases, field names are X_local and X_remote.
 */


/* A line is a phone concept.  As this is basically a dialog,
 * the only type required at the phone level is a line index.
 * This can be represented in a single byte.  The same index
 * number may be used in several places to index different
 * aspects of a line, for instance its LED signalling.  The
 * range is [0..HAVE_LINES> so the array size to create
 * in various locations would be ... [HAVE_LINES]
 */
typedef uint8_t linenr_t;
#define LINENR_NULL 0xff

typedef struct transaction tract_t;
typedef struct dialog dialog_t;


struct transaction {
	irqtimer_t tmr;
	dialog_t *dialog;
	void (*userfn) (textptr_t *sip, textptr_t *sdp);
	bool (*dialogfn) (dialog_t *dia, textptr_t *branch);
	textptr_t const *method;
	textptr_t incoming;
	textptr_t attachmt;
	timing_t growtime;
	textptr_t branch;	/* Characters are in txt_branch */
	textptr_t lastresponse;
	textptr_t lastsdp;
	char txt_branch [128];	/* BRANCHPREFIX_RFC3261, bottom_time, rnd32 */
	char state;		/* See TRS_xxx below */
};

#define TRS_ALLOCATED 'A'
#define TRS_CALLING 'C'
#define TRS_PROCEEDING 'P'
#define TRS_COMPLETED 'D'
#define TRS_TERMINATED 0x00
#define TRS_TRYING 'Y'
#define TRS_CONFIRMED 'M'

#define BRANCHPREFIX_RFC3261 "z9hG4bK"
#define BRANCHPREFIXLEN_RFC3261 7

extern const textptr_t branchprefix_rfc3261;


struct dialog {
	uint32_t connflags_send, connflags_recv;	/* See DCF_XXX below */
	uint32_t sideflags_local, sideflags_remot;	/* See DSF_XXX below */
	uint32_t cseqnr;				/* Incremented by one */
	uint16_t tract_usectr;				/* How many tract_t point here? */
	struct sdp *sdp_send, *sdp_recv;		/* TODO:Or_different... */
	textptr_t tag_local, tag_remot, tag_callid;	/* tag_XXX.str is txt_XXX or NULL */
	char txt_local [128], txt_remot [128], txt_callid [128];
	linenr_t linenr;				/* Line keeping this dialog open */
};

/* 1. State flags describing call progress */
#define DSF_CONNECTED		0x00000001	/* Connection wanted */
#define DSF_DISCONNECTED	0x00000002	/* Connection hung up */

/* 2. Sound choices describing what sounds are played */
#define DCF_SOUND_MASK		0x000000ff	/* Sound values */
#define DCF_SOUND_INACTIVE	0x00000000	/* Inactive, no sound */
#define DCF_SOUND_HOLD		0x00000001	/* Hold music, or silence */
#define DCF_SOUND_RINGING	0x00000001	/* Ringing sound */
#define DCF_SOUND_BUSY		0x00000002	/* Busy sound */
#define DCF_SOUND_RECORDED	0x00000004	/* Playing recorded sound */
#define DCF_SOUND_EARLYMEDIA	0x00000008	/* Early media is played */

/* 3. SIP behaviour modifications */
#define DF_100REL		0x00000100	/* RFC 3262 */
#define DF_199			0x00000200	/* RFC 6228 */

/* 4. Event packages that have been activated */
#define DCF_EVENT_MASK		0xffff0000	/* Event package flags */
#define DCF_EVENT_PRESENCE	0x00010000	/* RFC 3856 */
#define DCF_EVENT_REFER		0x00020000	/* RFC 3515 */
#define DCF_EVENT_DIALOG	0x00040000	/* RFC 4235 */
#define DCF_EVENT_CONFERENCE	0x00080000	/* RFC 4575 */
#define DCF_EVENT_PROFILE	0x00100000	/* RFC 6080 */


/* Method name string constants
 */
extern textptr_t const invite_m;
extern textptr_t const cancel_m;
extern textptr_t const bye_m;
extern textptr_t const refer_m;
extern textptr_t const ack_m;
extern textptr_t const register_m;


/* Core headers are passed around between functions.
 */

typedef enum coreheadernumber coreheadernum_t;
typedef struct coreheaders coreheaders_t;

enum coreheadernumber {
	COREHDR_FROM,
	COREHDR_TO,
	COREHDR_CALL_ID,
	COREHDR_CONTACT,
	COREHDR_VIA,
	COREHDR_CSEQ,
	COREHDR_ROUTE,
	// The number of core header numbers
	COREHDR_COUNT,
	// A tag for other headers
	COREHDR_OTHER = 31
};

extern textptr_t const coreheader_name [COREHDR_COUNT];

struct coreheaders {
	textptr_t headers [COREHDR_COUNT];
	char *sip;
	textptr_t from_tag;
	textptr_t to_tag;
	textptr_t callid;
	textptr_t via_branch;
	textptr_t cseq_method;
	uint32_t  cseq_serialno;
};


/* Operations to allocate a phone line for a dialog
 */
void sipdia_initialise (void);
dialog_t *sipdia_allocate (coreheaders_t const *coreheaders, bool myrequest);
void sipdia_deallocate (dialog_t *dia);
void sipdia_setlinenr (dialog_t *dia, linenr_t ln);
linenr_t sipdia_getlinenr (dialog_t *dia);
void sipdia_next_cseq (dialog_t *dia);
void sipdia_respond (textptr_t const *response, coreheaders_t const *coreheaders, textptr_t const *sip, textptr_t const *sdp_opt);
dialog_t *sipdia_findmatch (coreheaders_t const *coreheaders, bool const myrequest);

typedef void (*sipdia_climth) (dialog_t *dia);
sipdia_climth sipdia_client_method (textptr_t const *mth);


/* Transaction management dialogs */
void siptr_initialiase (void);
tract_t *siptr_findmatch (coreheaders_t const *coreheaders);
void siptr_client (dialog_t *dia, textptr_t const *mth, void (*truser) (textptr_t const *sip, textptr_t *sdp));
tract_t *siptr_server (dialog_t *dia, textptr_t const *mth);
void siptr_respond (tract_t *tr, textptr_t const *response, coreheaders_t const *coreheaders, textptr_t const *sip, textptr_t const *sdp_buffered);
void siptr_respond_again (tract_t *tr, coreheaders_t *coreheaders, textptr_t const *sip);
void siptr_response2uac (dialog_t *dia, textptr_t *sip, textptr_t *attachment, textptr_t *branch, textptr_t *cseqmth);
void siptr_request2uas (dialog_t *dia, coreheaders_t *coreheaders, textptr_t const *sip, textptr_t const *sdp);


/* Operations to manage dialogs
 */
bool sipdia_invite (dialog_t *dia, textptr_t *branch);
bool sipdia_cancel (dialog_t *dia, textptr_t *branch);
bool sipdia_accept (dialog_t *dia, textptr_t *branch);
bool sipdia_reject (dialog_t *dia, textptr_t *branch);
bool sipdia_bye (dialog_t *dia, textptr_t *branch);
bool sipdia_hold (dialog_t *dia, textptr_t *branch);
bool sipdia_resume (dialog_t *dia, textptr_t *branch);
bool sipdia_refer (dialog_t *dia, textptr_t *branch);
bool sip_ack (dialog_t *dia, textptr_t *branch);


/* Upcalls from the dialog/tract layer to the SIP application.
 * Each of the upcalls is invoked with the SIP message and body
 * (usually containing SDP) and the return value should be quick
 * and provide a 3-digit status code, a space and a description.
 * The return value may not contain line-end markers.
 * TODO: Additional parameters, like a Contact: header?
 */
void sipapp_invite (tract_t *tr, textptr_t const *requri, coreheaders_t *coreheaders, textptr_t const *sip, textptr_t const *sdp);
void sipapp_cancel (tract_t *tr, textptr_t const *requri, coreheaders_t *coreheaders, textptr_t const *sip, textptr_t const *sdp);
void sipapp_bye (tract_t *tr, textptr_t const *requri, coreheaders_t *coreheaders, textptr_t const *sip, textptr_t const *sdp);
void sipapp_hold (tract_t *tr, textptr_t const *requri, coreheaders_t *coreheaders, textptr_t const *sip, textptr_t const *sdp);
void sipapp_resume (tract_t *tr, textptr_t const *requri, coreheaders_t *coreheaders, textptr_t const *sip, textptr_t const *sdp);
void sipapp_refer (tract_t *tr, textptr_t const *requri, coreheaders_t *coreheaders, textptr_t const *sip, textptr_t const *sdp);
void sipapp_register (tract_t *tr, textptr_t const *requri, coreheaders_t *coreheaders, textptr_t const *sip, textptr_t const *sdp);

/* A {NULL,NULL}-terminated array holding the mapping from
 * methodname to sipapp_XXX upcall/handler function.
 */
typedef struct {
	textptr_t const *method;
	void (*sipappfn) (tract_t *tr, textptr_t const *requri, coreheaders_t *coreheaders, textptr_t const *sip, textptr_t const *sdp);
} sipappmap_t;
extern sipappmap_t sipapp_fnmap [];


/* Operations to manage registration
 */
bool sipreg_start (dialog_t *dia, textptr_t *branch);
bool sipreg_stop (dialog_t *dia, textptr_t *branch);


/* Message parser functions
 */
void sip_normalise (textptr_t *sipmsg, textptr_t *attachment);
bool sip_splitline_request (textptr_t const *sipmsg, textptr_t *method, textptr_t *requri);
bool sip_splitline_response (textptr_t const *sipmsg, textptr_t *code, textptr_t *descr);
bool sip_firstheader (textptr_t const *sipmsg, textptr_t *headername, textptr_t *headerval);
bool sip_nextheader  (textptr_t const *sipmsg, textptr_t *headername, textptr_t *headerval);
bool sip_firsturi_inheader (textptr_t const *siphdr, textptr_t *uri);
bool sip_nexturi_inheader  (textptr_t const *siphdr, textptr_t *uri);
bool sip_firstparm_inheader (textptr_t const *siphdr, textptr_t *parnm, textptr_t *parval);
bool sip_nextparm_inheader  (textptr_t const *siphdr, textptr_t *parnm, textptr_t *parval);
bool sip_firstparm_inuri (textptr_t const *uri, textptr_t *parnm, textptr_t *parval);
bool sip_nextparm_inuri  (textptr_t const *uri, textptr_t *parnm, textptr_t *parval);
bool sip_components_inuri (textptr_t const *uri, textptr_t *proto, textptr_t *user, textptr_t *pass, textptr_t *dom, uint16_t *port);
bool sip_split_cseq (textptr_t const *cseq, uint32_t *serial, textptr_t *mth);


/* Message generation functionss
 */
char *sipgen_request (char *pout, textptr_t const *method, textptr_t const *uri, textptr_t *branch);
char *sipgen_response (char *pout, textptr_t const *response, coreheaders_t const *coreheaders, textptr_t const *sip, textptr_t const *sdp_opt);
char *sipgen_max_forwards (char *pout);
char *sipgen_user_agent (char *pout);
char *sipgen_from (char *pout, dialog_t *dia);
char *sipgen_to (char *pout, textptr_t const *callee, dialog_t const *dia);
char *sipgen_to_register (char *pout, dialog_t const *dia);
char *sipgen_contact (char *pout, dialog_t const *dia);
char *sipgen_expires (char *pout, uint32_t const secs);
char *sipgen_cseq (char *pout, textptr_t const *method, uint32_t cseqnr);
char *sipgen_call_id (char *pout, dialog_t *dia);
char *sipgen_sdp_headers (char *pout, textptr_t const *sdp);
char *sipgen_attach_body (char *pout, textptr_t const *sdp);
void sipgen_create_branch (textptr_t *branch);



#endif
