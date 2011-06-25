/* Application zero -- the one that makes the phone doze off to sleep.
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


#include <resource.h>
#include <timer.h>
#include <irq.h>
#include <led.h>


/* The timer that awaits the end of the phone's activity display.
 * When this timer fires, everything will be brought back to normal
 * and further power-saving facilities may be effected, such as
 * switching off backlights and other LEDs.
 */
static timer_t zero_sleeptimer;


/* Process a timeout of the zero_sleeptimer.  When this happens, the phone
 * can go into a deep power saving mode.  Given, that is, that it can
 * wakeup as soon as some explicit action is taken by the user or over the
 * network.
 */
static bool zero_timeout (irq_t irq) {
#if HAS_LED_BACKLIGHT
	led_set_off (LED_IDX_BACKLIGHT);
#endif
	// TODO: Fall back to a trivial do-nothing mode
}


/* Start and initialise the zero application.  This will make the phone wait for some
 * time and then fall asleep.
 */
void zero_start (void) {
	timer_start (&zero_sleeptimer, TIME_MIN(1), zero_timeout);
}


/* Stop any attempts to make the phone fall asleep.
 */
void zero_stop (void) {
	timer_stop (&zero_sleeptimer);
#if HAS_LED_BACKLIGHT
	led_set (LED_IDX_BACKLIGHT, HAS_LED_BACKLIGHT);
#endif
}


/* Describe the application interace.  The zero application has no use for suspend/resume,
 * because it is stateless.  As soon as another application takes over, the zero app
 * forgets any state it may have, and will make a fresh start when resumed next time.
 * TODO: Claim the display; only when nobody is interested can it switch off.
 */
static application_t app_zero = {

	/* Operations */
	.app_start = zero_start,
	.app_stop = zero_stop,
	.app_suspend = NULL,
	.app_resume = NULL,
	.app_event = NULL,

	/* Resource specifications */
	.app_res_wish = NULL,
	.app_res_demand = NULL,

};


// TODO: init this module by registering at RES_LEVEL_ZERO the application structure for app_zero
