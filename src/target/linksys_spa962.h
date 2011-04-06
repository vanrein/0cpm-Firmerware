/*
 * http://devel.0cpm.org/ -- Open source SIP telephony firmware.
 *
 * Device-specific includes for Linksys SPA962
 */


/* The maximum number of supported handset */
#define HAVE_HANDSETS 1

/* Keyboard settings */
#undef DETECTS_DTMF
#define HAVE_KBD_STARHASH	1
#define HAVE_KBD_ABCD		0
#define HAVE_KBD_FLASH		0
#define HAVE_KBD_LINES		6
#define HAVE_KBD_SOFT_FUNCTION	4
#ifndef HAVE_KBD_SIDECARS
#   define HAVE_KBD_SIDECARS	0
#endif
#define HAVE_KBD_GENERIC		((HAVE_KBD_SIDECARS)*32)
#define HAVE_KBD_VOICEMAIL	1
#define HAVE_KBD_MENU		1
#define HAVE_KBD_OK		0
#define HAVE_KBD_ENTER		0
#define HAVE_KBD_UPDOWN		1
#define HAVE_KBD_LEFTRIGHT	1
#define HAVE_KBD_MUTE		1
#define HAVE_KBD_CONFERENCE	0
#define HAVE_KBD_HOLD		1
#define HAVE_KBD_TRANSFER	0
#define HAVE_KBD_PHONEBOOK	0
#define HAVE_KBD_VOLUPDOWN	1
#define HAVE_KBD_HEADSET		1
#define HAVE_KBD_SPEAKER		1
#define HAVE_KBD_HANDSET		1
#define HAVE_KBD_MISSEDCALLS	0
#define HAVE_KBD_REDIAL		0

/* Light settings */
#define HAVE_LED_MESSAGE		1
#define HAVE_LED_LINEKEYS	1
#define HAVE_LED_GENERIC		1
#define HAVE_LED_MUTE		1
#define HAVE_LED_HANDSET		1
#define HAVE_LED_HEADSET		1
#define HAVE_LED_SPEAKERPHONE	1
#define HAVE_LED_BACKLIGHT	1

/* Display settings */

/* Sound settings */
#define HAVE_HANDSET		1
#define HAVE_HEADSET		1
#define HAVE_SPEAKER_MIKE	1

/* Line settings */

/* Account settings */

/* Media settings */
#define HAVE_MEDIA_VOICE		1
#define HAVE_MEDIA_VIDEO		0
#define HAVE_MEDIA_URI		0
#define HAVE_MEDIA_IMAGE		1

