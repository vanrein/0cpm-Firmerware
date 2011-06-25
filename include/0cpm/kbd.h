/* Keyboard drivers
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


#ifndef HEADER_KBD
#define HEADER_KBD


/* Button classes: DTMF, function-specific, lines, soft functions, userprog */
typedef enum {
	BUTCLS_NONE,
	BUTCLS_DTMF,
	BUTCLS_FIXED_FUNCTION,
	BTCLS_LINE,
	BUTCLS_SOFT_FUNCTION,
	BUTCLS_USER_PROGRAMMABLE
} buttonclass_t;


#define BUTTON_NULL	'\x00'


/* Button codes: An ASCII code or a special function */
typedef uint8_t buttoncode_t;


/* Functions that may be required if the keyboard and/or hook need to be
 * scanned actively, instead of initiated purely by level-change interrupts.
 */
#if defined NEED_KBD_SCANNER_BETWEEN_KEYS || defined NEED_KBD_SCANNER_DURING_KEYPRESS
void bottom_keyboard_scan (void);
#endif

#if defined NEED_HOOK_SCANNER_WHEN_ONHOOK || defined NEED_HOOK_SCANNER_WHEN_OFFHOOK
void bottom_hook_scan (void);
#endif


/* Interrupt routines invoked by the bottom, possibly during bottom_xxx_scan().
 * The application may assume that the phone is onhook, and that no key is
 * pressed when the phone comes online.  The top functions will be called if the
 * situation is found to be different.
 */
void top_button_press (buttonclass_t bcl, buttoncode_t cde);
void top_button_release (void);
void top_hook_update (bool offhook);


#endif	/* HEADER_KBD */
