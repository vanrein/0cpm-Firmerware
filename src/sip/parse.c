/* sipparse.c -- Normalisation and parsing of SIP messages.
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

#include <config.h>

#include <0cpm/text.h>


/*
 * These are parser routines that process incoming SIP messages.
 *
 * The first thing to do with a new SIP message is to run the
 * sip_normalise() routine on it.  This may make the message
 * smaller, by wiping away extravagant spaces and so on.  The
 * resulting message may have very long lines, as broken lines
 * are also turned into single long lines.
 *
 * Note on UTF8:
 *
 * Note that UTF8 is not treated special in any way; the new
 * characters introduced are all >= 0x80 and will not match the
 * checks for specific ASCII characters below.  Also note that
 * the SIP standard does not require knowledge of UTF8 in the
 * message handling part of a phone.
 */



/* A mapping from short header names to long ones.  This is used
 * to avoid ever showing short names to parser users, thus
 * simplifying the application.
 *
 * Source: http://www.iana.org/assignments/sip-parameters
 * (The mapping below incorporates RFCs until RFC 6228.)
 */
static textptr_t shortheadermap [26] = {
	/* a */		{ "Accept-contact", 14 },
	/* b */		{ "Referred-by", 11 },
	/* c */		{ "Content-type", 12 },
	/* d */		{ "Request-disposition", 19 },
	/* e */		{ "Content-encoding", 16 },
	/* f */		{ "From", 4 },
	/* g */		{ NULL, 0 },
	/* h */		{ NULL, 0 },
	/* i */		{ "Call-id", 7 },
	/* j */		{ "Reject-contact", 14 },
	/* k */		{ "Supported", 9 },
	/* l */		{ "Content-length", 14 },
	/* m */		{ "Contact", 7 },
	/* n */		{ "Identity-info", 13 },
	/* o */		{ "Event", 5 },
	/* p */		{ NULL, 0 },
	/* q */		{ NULL, 0 },
	/* r */		{ "Refer-to", 8 },
	/* s */		{ "Subject", 7 },
	/* t */		{ "To", 2 },
	/* u */		{ "Allow-events", 12 },
	/* v */		{ "Via", 3 },
	/* w */		{ NULL, 0 },
	/* x */		{ "Session-expires", 15 },
	/* y */		{ "Identity", 8 },
	/* z */		{ NULL, 0 }
};


/* Normalise a SIP message, as a prerequisite to further parsing:
 *  - Replace combined whitespace by its canonical or simplest form
 *  - Reunite split header lines
 *  - Standardise header capitalisation
 * An explicit pointer to the attachment will follow, if any.
 */
void sip_normalise (textptr_t *sipmsg, textptr_t *attachment) {
	register uint16_t togo = sipmsg->len;
	register uint16_t skip = 0;
	register char *here = sipmsg->str;
	register char *there = here;
	//
	// Initialise for empty attachment
	textnullify (attachment);
	//
	// First, skip any leading CRLF content
	while ((togo > 0) && ((*here == '\r') || (*here == '\n'))) {
		here++;
		togo--;
	}
	//
	// Copy the start-line
	while ((togo > 0) && (*here != '\r')) {
		*there++ = *here++;
		togo--;
	}
	//
	// Copy the CRLF terminating the start-line
	while ((togo > 0) && ((*here == '\r') || (*here == '\n'))) {
		*there++ = *here++;
		togo--;
	}
	//
	// Then, process any additional content
	// Do this by cloning from *here to *there, and
	// at any step decreasing togo.  When skipping
	// characters, only lower togo and increase skip
	// instead of cloning.
	while (togo > 0) {
		//
		// Recognise the end of headers, start of body
		if ((*here == '\r') || (*here == '\n')) {
			sipmsg->len -= skip + togo;
sipmsg->len += 2; //TODO:WHAT_IS_MISSING?!?//
			do {
				here++;
				togo--;
			} while ((togo > 0) && (*here == '\r') || (*here == '\n'));
			attachment->str = here;
			attachment->len = togo;
			break;
		}
		//
		// Skip a headername, changing to "Its-name" or "i" capitalisation.
		while ((togo > 0) && ((*here == ' ') || (*here == '\t'))) {
			here++;
			togo--;
			skip++;
		}
		//
		// Clone the first of at least two letters in uppercase form
		if ((togo > 1) && ((*here & 0xdf) >= 'A') && (((*here & 0xdf) <= 'Z'))) {
			if (((here [1] & 0xdf) >= 'A') && ((here [1] & 0xdf) <= 'Z')) {
				*there++ = *here++ & 0xdf;
				togo--;
			}
		}
		//
		// Clone remaining letters and other non-whitespace in lowercase form
		while ((togo > 0) && (*here != ' ') && (*here != '\t') && (*here != ':')) {
			if ((*here >= 'A') && ((*here <= 'Z'))) {
				*there++ = *here++ | 0x20;
			} else {
				*there++ = *here++;
			}
			togo--;
		}
		//
		// After whitespace starts, chase for the colon 
		while ((togo-- > 0) && (*here++ != ':')) {
			skip++;
		}
		*there++ = ':';
		//
		// Reduce whitespace after the colon, if any to one space
		if ((togo > 0) && ((*here == ' ') || (*here == '\t'))) {
			*there++ = ' ';
			here++;
			togo--;
			while ((togo > 0) && ((*here == ' ') || (*here == '\t'))) {
				here++;
				skip++;
				togo--;
			}
		}
		//
		// Clone header contents -- possibly spanning multiple lines
		while (togo > 0) {
			//
			// Clone the remainder of the current line
			while ((togo > 0) && (*here != '\r') && (*here != '\n')) {
				*there++ = *here++;
				togo--;
			}
			//
			// See if the current line is continued on the next
			if ((togo >= 3) && (here [0] == '\r') && (here [1] == '\n')
					&& ((here [2] == ' ') || (here [2] == '\t'))) {
				//
				// Replace CRLF + whitespace by a single space
				*there++ = ' ';
				skip += 2;
				togo -= 3;
				while ((togo > 0) && (*here == ' ') || (*here == '\t')) {
					here++;
					togo--;
					skip++;
				}
			} else {
				//
				// Clone CRLF and after that end cloning this header
				if ((togo > 0) && ((*here == '\r') || (*here == '\n'))) {
					*there++ = *here++;
					togo--;
					if ((togo > 0) && (*here != here [-1]) && ((*here == '\r') || (*here == '\n'))) {
						*there++ = *here++;
						togo--;
					}
				}
				break;	/* Done with this header */
			}
		}
	}
	//
	// Remove skipped chars from the SIP message
	sipmsg->len -= skip;
}

/* Extract the components from a request's start-line.
 */
bool sip_splitline_request (textptr_t const *sipmsg, textptr_t *method, textptr_t *requri) {
	uint16_t idx = 0;
	// Note: Parsing upon reception ensured that the folowing will work
	method->str = sipmsg->str;
	while (method->str [idx] != ' ') {
		idx++;
	}
	method->len = idx;
	requri->str = sipmsg->str + idx + 1;
	idx = 0;
	while (requri->str [idx] != ' ') {
		idx++;
	}
	requri->len = idx;
	return (method->len > 0) && (requri->len > 0);
}

/* Extract the components from a response's start-line.
 */
bool sip_splitline_response (textptr_t const *sipmsg, textptr_t *code, textptr_t *descr) {
	// Note: Parsing upon reception ensured that the following will work
	uint16_t idx;
	code->str = sipmsg->str + 8;
	code->len = 3;
	descr->str = sipmsg->str + 12;
	idx = 12;
	while ((idx < sipmsg->len) && (sipmsg->str [idx] != '\r') && (sipmsg->str [idx] != '\n')) {
		idx++;
	}
	descr->len = idx - 12;
	return true;
}

/* Find the first header in a given SIP message.  The result consists
 * of the full name in "Standard-capitalisation" stored in headername,
 * and the contents as a single line without termination stored in
 * headerval.  The application should pickup headers in their order
 * of appearance, and process them accordingly.
 * Return true if the header was found.
 */
bool sip_firstheader (textptr_t const *sipmsg, textptr_t *headername, textptr_t *headerval) {
	char *here   = sipmsg->str;
	uint16_t len = sipmsg->len;
	//
	// First find the end of the start-line
	while ((len > 0) && (*here != '\r') && (*here != '\n')) {
		here++;
		len--;
	}
	//
	// Then share the sip_nextheader() algorithm
	headerval->str = sipmsg->str;
	headerval->len = sipmsg->len - len;
	return sip_nextheader (sipmsg, headername, headerval);
}

/* Find the next header in a given SIP message.  The result consists
 * of the full name in "Standard-capitalisation" stored in headername,
 * and the contents as a single line without termination stored in
 * headerval.  When called, these values contain the previous header,
 * the one to find the following for.  The application should pickup
 * headers in their order of appearance, and process them accordingly.
 * Return true if the header was found.
 */
bool sip_nextheader (textptr_t const *sipmsg, textptr_t *headername, textptr_t *headerval) {
	uint16_t togo = sipmsg->len - (((intptr_t) headerval->str) - ((intptr_t) sipmsg->str));
	char *here = headerval->str + headerval->len;
	//
	// Skip the CRLF after the current headerval
	while ((togo > 0) && ((*here == '\r') || (*here == '\n'))) {
		here++;
		togo--;
	}
	//
	// Take note of the next header name
	headername->str = here;
	headername->len = togo;
	while ((togo > 0) && (*here != ':')) {
		here++;
		togo--;
	}
	headername->len -= togo;
	//
	// Expand single-letter header names to their full form
	if (headername->len == 1) {
		char hnam = headername->str [0];
		if ((hnam >= 'a') && (hnam <= 'z') && shortheadermap [hnam - 'a'].len) {
			headername->str = shortheadermap [hnam].str;
			headername->len = shortheadermap [hnam].len;
		}
	}
	//
	// Skip past ": " or else ":"
	if (togo > 0) {
		here++;
		togo--;
	}
	if ((togo > 0) && (*here == ' ')) {
		here++;
		togo--;
	}
	//
	// Take note of the next header value
	headerval->str = here;
	headerval->len = togo;
	while ((togo > 0) && (*here != '\r') && (*here != '\n')) {
		here++;
		togo--;
	}
	headerval->len -= togo;
	//
	// There should always be a CRLF left to skip; if not, fail
	return (togo > 0);
}


/* Find the first SIP URI in a header.  The result is stored in uri.
 * The URI is assumed to be enclosed in angular brackets of, if that
 * is not the case, it is assumed to be a list separated by commas,
 * or semicolons.
 * Return true if the URI was found.
 */
bool sip_firsturi_inheader (textptr_t const *siphdr, textptr_t *uri) {
	register char *hdr    = siphdr->str;
	register uint16_t max = siphdr->len;
	register uint16_t len = 0;
	//
	// Find if angular brackets occur in the header
	while (len < max) {
		if (hdr [len++] == '<') {
			//
			// Mark the area between < and >
			uri->str = hdr + len;
			uri->len = ~len; // Remember to add 1 later
			//
			// Count the length between < and >
			while (len < max) {
				if (hdr [len++] == '>') {
					uri->len += len; // 1 too high
					return true;
				}
			}
			//
			// Did not find closing angular bracket
			return false;
		}
	}
	//
	// Since angular brackets do not occur, find the first URI
	len = 0;
	if (hdr [len] == ' ') {
		len++;
		uri->str = hdr + 1;
		uri->len = ~0;	// offset -1
	} else {
		uri->str = hdr;
		uri->len = 0;
	}
	while ((len < max) &&
			(hdr [len] != ' ' ) && (hdr [len] != '\t') &&
			(hdr [len] != '\r') && (hdr [len] != '\n') &&
			(hdr [len] != ';' ) && (hdr [len] != ',' )) {
		len++;
	}
	uri->len += len;
	//
	// Return success if a URI was found
	return (uri->len != 0);
}

/* Find the next SIP URI in a header.  The result is stored in uri.
 * The URI is assumed to be enclosed in angular brackets of, if that
 * is not the case, it is assumed to be a list separated by commas,
 * or semicolons.
 * Return true if the URI was found.
 */
bool sip_nexturi_inheader (textptr_t const *siphdr, textptr_t *uri) {
	//
	// Construct a fake SIP header to cover the remainder
	textptr_t remainder;
	remainder.str = uri->str + uri->len;
	remainder.len = (uint16_t) ((intptr_t) siphdr->str - (intptr_t) siphdr->str);
	//
	// Find the next URI as it were the first URI in the remainder
	return sip_firsturi_inheader (&remainder, uri);
}

/* Find the first header parameter.  The name of the parameter is stored
 * in parnm, the value in parval.  Return true if the parameter was found.
 */
bool sip_firstparm_inheader (textptr_t const *siphdr, textptr_t *parnm, textptr_t *parval) {
	bool sip_firstparm_inuri (textptr_t const *uri, textptr_t *parnm, textptr_t *parval); //TODO//
	//TODO// Should first skip any <enclosed_URIs> in the header
	return sip_firstparm_inuri (siphdr, parnm, parval);
}

/* Find the next header parameter.  The name of the parameter is stored
 * in parnm, the value in parval.  Return true if the parameter was found.
 */
bool sip_nextparm_inheader (textptr_t const *siphdr, textptr_t *parnm, textptr_t *parval) {
	bool sip_nextparm_inuri (textptr_t const *uri, textptr_t *parnm, textptr_t *parval); //TODO//
	return sip_nextparm_inuri (siphdr, parnm, parval);
}


/* Common code for finding a parameter in a URI, used by the functions
 * sip_firstparm_inuri() and sip_nextparm_inuri ().  These routines
 * invoke this final part after having located the next parameter.
 */
static bool sip_foundparm_inuri (char *here, uint16_t togo, textptr_t *parnm, textptr_t *parval) {
	//
	// First, store the found location for a parameter name
	parnm->str = here;
	parnm->len = togo;
	//
	// Second, chase for an equals sign or semicolon
	while ((togo > 0) && (*here != ';') && (*here != '=')) {
		here++;
		togo--;
	}
	parnm->len -= togo;
	if (parnm->len == 0) {
		textnullify (parnm);
		return false;
	}
	//
	// End now if there is no equals sign
	if ((togo == 0) || (*here != '=')) {
		parval->str = here;
		parval->len = 0;
		return (parnm->len != 0);
	}
	//
	// Skip the equals sign and find the value
	here++;
	togo--;
	parval->str = here;
	parval->len = togo;
	// 
	// Chase the value of this parameter
	while ((togo > 0) && (*here != ';')) {
		here++;
		togo--;
	}
	parval->len -= togo;
	if (parval->len == 0) {
		textnullify (parval);
	}
	//
	// End in success, but with varying parval contents
	return true;
}

/* Find the first URI parameter.  The name of the parameter is stored
 * in parnm, the value in parval.  Return true if the parameter was found.
 */
bool sip_firstparm_inuri (textptr_t const *uri, textptr_t *parnm, textptr_t *parval) {
	char *here    = uri->str;
	uint16_t togo = uri->len;
	//
	// First chase for a semicolon
	while ((togo > 0) && (*here != ';')) {
		here++;
		togo--;
	}
	//
	// See if the end has been reached
	if (togo == 0) {
		return false;
	}
	//
	// Skip past the semicolon
	here++;
	togo--;
	//
	// Found the parameter name, process in general routine
	return sip_foundparm_inuri (here, togo, parnm, parval);
}

/* Find the next URI parameter.  The name of the parameter is stored
 * in parnm, the value in parval.  Return true if the parameter was found.
 */
bool sip_nextparm_inuri (textptr_t const *uri, textptr_t *parnm, textptr_t *parval) {
	char *here = parval->str + parval->len;
	int16_t togo = uri->len - ((intptr_t) here - (intptr_t) uri->str);
	//
	// Skip the current semicolon if there is one (so, before the end)
	if ((togo == 0) || (*here != ';')) {
		return false;
	}
	togo--;
	here++;
	//
	// Found the parameter name, process in general routine
	return sip_foundparm_inuri (here, togo, parnm, parval);
}

/* Take a SIP URI apart into its constituent components.
 * Parameters are stripped off, but their analysis should
 * be done with the sip_first/nextparm_inuri() functions.
 * Return false if the URI has a bad format.  Note that a
 * SIP URI need not have a user, so check it after this
 * call if you need the username part.
 */
bool sip_components_inuri (textptr_t const *uri,
				textptr_t *proto,
				textptr_t *user,
				textptr_t *pass,
				textptr_t *dom,
				uint16_t *port) {
	char *here = uri->str;
	uint16_t togo = uri->len;
	//
	// Parse out the protocol
	proto->str = here;
	proto->len = togo;
	while ((togo > 0) && (*here != ':')) {
		togo--;
		here++;
	}
	if ((proto->len -= togo) == 0) {
		// Fatal if no proto present
		// textnullify (proto);
		return false;
	}
	//
	// Skip the ':' after the protocol
	if (togo > 0) {
		togo--;
		here++;
	}
	//
	// Parse out the uesrname
	user->str = here;
	user->len = togo;
	while ((togo > 0) && (*here != '@') && (*here != ':')) {
		togo--;
		here++;
	}
	if ((user->len -= togo) == 0) {
		// Non-fatal if no user present -- so check!
		textnullify (user);
	}
	if ((togo > 0) && (*here == ':')) {
		here++;
		togo--;
		//
		// Parse out the password
		pass->str = here;
		pass->len = togo;
		while ((togo > 0) && (*here != '@')) {
			here++;
			togo--;
		}
		// Zero length can be meaningful, so allow it
		pass->len -= togo;
	} else {
		//
		// Set a null password -- which is non-fatal
		textnullify (pass);
	}
	//
	// Skip the '@' between userpart and domainname
	if ((togo > 0) && (*here == '@')) {
		here++;
		togo--;
	}
	//
	// Parse out the domain name (treat '?' as an end also)
	dom->str = here;
	dom->len = togo;
	if ((togo > 0) && (*here != '[')) {
		while ((togo > 0) && (*here != ':') && (*here != ';') && (*here != '?')) {
			here++;
			togo--;
		}
	} else {
		while ((togo > 0) && (*here != ']')) {
			here++;
			togo--;
		}
		if (togo > 0) {
			here++;
			togo--;
		}
	}
	if ((dom->len -= togo) == 0) {
		// An empty domain is fatal
		return false;
	}
	//
	// Parse out an optional port number
	if ((togo > 0) && (*here == ':')) {
		here++;
		togo--;
		*port = 0;
		while ((togo > 0) && (*here >= '0') && (*here <= '9')) {
			*port *= 10;
			*port += *here - '0';
			here++;
			togo--;
		}
		if (*port == 0) {
			// UDP does not allow port 0 to occur
			return false;
		}
		if ((togo > 0) && (*here != ';') && (*here != '?')) {
			// Any remaining text must hold options
			return false;
		}
	} else {
		// Fall back to default port 5060 for SIP
		*port = 5060;
	}
	//
	// No fatal component failures occurred, so report success
	return true;
}

/* Split a Cseq: header into its two parts: a serial number and a method name.
 * Return false (and set NULL string and 0 value) on error.
 */
bool sip_split_cseq (textptr_t const *cseq, uint32_t *serial, textptr_t *mth) {
	char *here = cseq->str;
	uint16_t togo = cseq->len;
	textnullify (mth);
	//
	// First parse the numeric part
	*serial = 0;
	if (!here) {
		return false;
	}
	while ((togo > 0) && (*here >= '0') && (*here <= '9')) {
		*serial *= 10;
		*serial += *here++ - '0';
		togo--;
	}
	if ((togo == 0) || (*here != ' ')) {
		*serial = 0;
		return false;
	}
	//
	// Then skip the space
	here++;
	togo--;
	//
	// Take in the method name (which is assumed to cover the rest of the line)
	mth->str = here;
	mth->len = togo;
	//
	// Succeeded -- return the two components and signal success
	return true;
}

