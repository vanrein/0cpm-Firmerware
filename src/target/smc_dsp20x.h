/* Device-specific includes for Linksys SPA962
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
#define HAVE_HANDSETS 1

/* Keyboard settings */
#undef DETECTS_DTMF
#define HAVE_KBD_STARHASH	1
#define HAVE_KBD_ABCD		0
#define HAVE_KBD_FLASH		0
#define HAVE_KBD_LINES		0
#define HAVE_KBD_SOFT_FUNCTION	0
#define HAVE_KBD_GENERIC	0
#define HAVE_KBD_VOICEMAIL	1
#define HAVE_KBD_MENU		1
#define HAVE_KBD_OK		0
#define HAVE_KBD_ENTER		1
#define HAVE_KBD_UPDOWN		1
#define HAVE_KBD_LEFTRIGHT	1
#define HAVE_KBD_MUTE		0
#define HAVE_KBD_CONFERENCE	1
#define HAVE_KBD_HOLD		1
#define HAVE_KBD_TRANSFER	1
#define HAVE_KBD_PHONEBOOK	0
#define HAVE_KBD_VOLUPDOWN	1
#define HAVE_KBD_HEADSET		0
#define HAVE_KBD_SPEAKER		1
#define HAVE_KBD_HANDSET		0
#define HAVE_KBD_MISSEDCALLS	0
#define HAVE_KBD_REDIAL		1

/* Light settings */
#define HAVE_LED_MESSAGE		1
#define HAVE_LED_LINEKEYS	0
#define HAVE_LED_GENERIC		0
#define HAVE_LED_MUTE		0
#define HAVE_LED_HANDSET		0
#define HAVE_LED_HEADSET		0
#define HAVE_LED_SPEAKERPHONE	0
#define HAVE_LED_BACKLIGHT	1

/* Display settings */

/* Sound settings */
#define HAVE_HANDSET		1
#define HAVE_HEADSET		0
#define HAVE_SPEAKER_MIKE	1

/* Line settings */

/* Account settings */

/* Media settings */
#define HAVE_MEDIA_VOICE		1
#define HAVE_MEDIA_VIDEO		0
#define HAVE_MEDIA_URI		0
#define HAVE_MEDIA_IMAGE		0


