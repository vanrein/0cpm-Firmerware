/*
 * http://devel.0cpm.org/ -- Open source firmware for SIP phones.
 *
 * Keyboard drivers
 */


#include <resource.h>
#include <kbd.h>


/* The keyboard can be used and reused in many different ways.  Which way applies when is
 * dependent on the state of the phone.  At each resource level, the application may
 * request the use of certain keys and other resources.  When not all resources are
 * available because higher resource levels overtake them, the application will be
 * suspended.  At a later moment, when the higher resource level application ends, the
 * application at the lower level may be resumed.
 */


resource_t resource_keyboard = {
	/* res_claims */	NULL,
	/* res_eventprio */	EVT_PRIO_USER,
};

