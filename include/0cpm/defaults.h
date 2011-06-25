/* Device-generic default settings for undefined phone variables.
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


/* The maximum number of supported handset */
#ifndef HAS_HANDSETS
#  define HAS_HANDSETS 			1
#endif

/* Keyboard settings */
#ifdef DETECTS_DTMF
#   error "Unsure how to set the defaults for a DETECTS_DTMF phone, sorry..."
#endif
#ifndef HAS_KBD_STARHASH
#   define HAS_KBD_STARHASH		1
#endif
#ifndef HAS_KBD_ABCD
#   define HAS_KBD_ABCD			0
#endif
#ifndef HAS_KBD_FLASH
#   define HAS_KBD_FLASH		1
#endif
#ifndef HAS_KBD_LINES
#   define HAS_KBD_LINES		0
#endif
#ifndef HAS_KBD_SOFT_FUNCTION
#   define HAS_KBD_SOFT_FUNCTION	0
#endif
#ifndef HAS_KBD_GENERIC
#   define HAS_KBD_GENERIC		0
#endif
#ifndef HAS_KBD_VOICEMAIL
#   define HAS_KBD_VOICEMAIL		1
#endif
#ifndef HAS_KBD_MENU
#   define HAS_KBD_MENU			0
#endif
#ifndef HAS_KBD_UPDOWN
#   define HAS_KBD_UPDOWN		0
#endif
#ifndef HAS_KBD_LEFTRIGHT
#   define HAS_KBD_LEFTRIGHT		0
#endif
#ifndef HAS_KBD_MUTE
#   define HAS_KBD_MUTE			0
#endif
#ifndef HAS_KBD_CONFERENCE
#   define HAS_KBD_CONFERENCE		0
#endif
#ifndef HAS_KBD_HOLD
#   define HAS_KBD_HOLD			0
#endif
#ifndef HAS_KBD_TRANSFER
#   define HAS_KBD_TRANSFER		0
#endif
#ifndef HAS_KBD_PHONEBOOK
#   define HAS_KBD_PHONEBOOK		0
#endif
#ifndef HAS_KBD_VOLUPDOWN
#   define HAS_KBD_VOLUPDOWN		0
#endif
#ifndef HAS_KBD_HADSET
#   define HAS_KBD_HADSET		1
#endif
#ifndef HAS_KBD_SPEAKER
#   define HAS_KBD_SPEAKER		0
#endif
#ifndef HAS_KBD_HEADSET
#   define HAS_KBD_HEADSET		0
#endif
#ifndef HAS_KBD_MISSEDCALLS
#   define HAS_KBD_MISSEDCALLS		0
#endif
#ifndef HAS_KBD_REDIAL
#   define HAS_KBD_REDIAL		0
#endif

/* Light settings */
#ifndef HAS_LED_MESSAGE
#   define HAS_LED_MESSAGE		1
#endif
#ifndef HAS_LED_LINEKEYS
#   define HAS_LED_LINEKEYS		0
#endif
#ifndef HAS_LED_GENERIC
#   define HAS_LED_GENERIC		0
#endif
#ifndef HAS_LED_MUTE
#   define HAS_LED_MUTE			0
#endif
#ifndef HAS_LED_HANDSET
#   define HAS_LED_HANDSET		0
#endif
#ifndef HAS_LED_HEADSET
#   define HAS_LED_HEADSET		0
#endif
#ifndef HAS_LED_SPEAKERPHONE
#   define HAS_LED_SPEAKERPHONE		0
#endif
#ifndef HAS_LED_BACKLIGHT
#   define HAS_LED_BACKLIGHT		1
#endif

/* Display settings */

/* Sound settings */
#ifndef HAS_HANDSET
#   define HAS_HANDSET			1
#endif
#ifndef HAS_HEADSET
#   define HAS_HEADSET			0
#endif
#ifndef HAS_SPEAKER_MIKE
#   define HAS_SPEAKER_MIKE		0
#endif

/* Line settings */
#ifndef NUM_LINES
#   if HAS_KBD_FLASH
#      define NUM_LINES			2
#   else
#      define NUM_LINES			1
#   endif
#endif

/* Account settings */
#ifndef NUM_ACCOUNTS
#   define NUM_ACCOUNTS			NUM_LINES
#endif

/* Call settings */
#define NUM_CALLS			NUM_LINES

/* Media settings */
#ifndef HAS_MEDIA_VOICE
#   define HAS_MEDIA_VOICE		1
#endif
#ifndef HAS_MEDIA_VIDEO
#   define HAS_MEDIA_VIDEO		0
#endif
#ifndef HAS_MEDIA_URI
#   define HAS_MEDIA_URI		0
#endif
#ifndef HAS_MEDIA_IMAGE
#   define HAS_MEDIA_IMAGE		0
#endif

