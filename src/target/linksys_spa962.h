/*
 * http://devel.0cpm.org/ -- Open source SIP telephony firmware.
 *
 * Device-specific includes for Linksys SPA962
 */


/* The maximum number of supported handset */
#define HAS_HANDSETS 1

/* Keyboard settings */
#undef DETECTS_DTMF
#define HAS_KBD_STARHASH	1
#define HAS_KBD_ABCD		0
#define HAS_KBD_FLASH		0
#define HAS_KBD_LINES		6
#define HAS_KBD_SOFT_FUNCTION	4
#ifndef HAS_KBD_SIDECARS
#   define HAS_KBD_SIDECARS	0
#endif
#define HAS_KBD_GENERIC		((HAS_KBD_SIDECARS)*32)
#define HAS_KBD_VOICEMAIL	1
#define HAS_KBD_MENU		1
#define HAS_KBD_UPDOWN		1
#define HAS_KBD_LEFTRIGHT	1
#define HAS_KBD_MUTE		1
#define HAS_KBD_CONFERENCE	0
#define HAS_KBD_HOLD		1
#define HAS_KBD_TRANSFER	0
#define HAS_KBD_PHONEBOOK	0
#define HAS_KBD_VOLUPDOWN	1
#define HAS_KBD_HEADSET		1
#define HAS_KBD_SPEAKER		1
#define HAS_KBD_HEADSET		1
#define HAS_KBD_MISSEDCALLS	0
#define HAS_KBD_REDIAL		0

/* Light settings */
#define HAS_LED_MESSAGE		1
#define HAS_LED_LINEKEYS	1
#define HAS_LED_GENERIC		1
#define HAS_LED_MUTE		1
#define HAS_LED_HANDSET		1
#define HAS_LED_HEADSET		1
#define HAS_LED_SPEAKERPHONE	1
#define HAS_LED_BACKLIGHT	1

/* Display settings */

/* Sound settings */
#define HAS_HANDSET		1
#define HAS_HEADSET		1
#define HAS_SPEAKER_MIKE	1

/* Line settings */

/* Account settings */

/* Media settings */
#define HAS_MEDIA_VOICE		1
#define HAS_MEDIA_VIDEO		0
#define HAS_MEDIA_URI		0
#define HAS_MEDIA_IMAGE		1

