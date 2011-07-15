/* LED simulation support -- print them in tabbed columns
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



/* Descriptions below are all restricted to 7 chars or less, so they
 * can be printed with a '\t' in between.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdlib.h>
#include <stdarg.h>

#include <config.h>

#include <0cpm/led.h>
#include <0cpm/cons.h>


char *led_colours_2 [] = { "off", "green" };
char *led_colours_3 [] = { "off", "green", "red" };

struct led_descript {
	char *name;
	char **states;
	char *current;
};

/*
 * LED descriptive structure, following the structure of led.h
 */
struct led_descript num2descr [] = {
#if HAVE_LED_LINEKEYS
#  if NUM_LINEKEYS >= 1
	{ "line1", led_colours_3, NULL, },
#  endif
#  if NUM_LINEKEYS >= 2
	{ "line2", led_colours_3, NULL, },
#  endif
#  if NUM_LINEKEYS >= 3
	{ "line3", led_colours_3, NULL, },
#  endif
#  if NUM_LINEKEYS >= 4
	{ "line4", led_colours_3, NULL, },
#  endif
#  if NUM_LINEKEYS >= 5
	{ "line5", led_colours_3, NULL, },
#  endif
#  if NUM_LINEKEYS >= 6
	{ "line6", led_colours_3, NULL, },
#  endif
#  if NUM_LINEKEYS >= 7
#	error "Please define additional line LEDs in ledsimu.c"
#  endif
#endif
#if HAVE_LED_GENERIC
#	error "Please define generic LEDs in ledsimu.c"
#endif
#if HAVE_LED_MESSAGE
	{ "vmail", led_colours_2, NULL, },
#endif
#if HAVE_LED_MUTE
	{ "mute", led_colours_2, NULL, },
#endif
#if HAVE_LED_HANDSET
	{ "handset", led_colours_2, NULL, },
#endif
#if HAVE_LED_HEADSET
	{ "headset", led_colours_2, NULL, },
#endif
#if HAVE_LED_SPEAKERPHONE
	{ "speaker", led_colours_2, NULL, },
#endif
#if HAVE_LED_BACKLIGHT
	{ "display", led_colours_2, NULL, },
#endif
};


void bottom_led_set (led_idx_t lednum, led_colour_t col) {
	int i;
	struct timeval tv;
	gettimeofday (&tv, NULL);
	static headctr = 0;
	if (headctr++ % 20 == 0) {
		for (i = 0; i < LED_IDX_COUNT; i++) {
			bottom_printf ("\t%s", (intptr_t) num2descr [i].name);
		}
		bottom_printf ("\n");
	}
	bottom_printf ("%3d.%03d", (intptr_t) tv.tv_sec, (intptr_t) (tv.tv_usec / 1000));
	char capscol [8];
	char *colnm = num2descr [lednum].states [col];
	for (i = 0; i < 7; i++) {
		if (colnm [i] == 0) {
			break;
		}
		capscol [i] = toupper (colnm [i]);
	}
	capscol [i] = '\0';
	for (i = 0; i < LED_IDX_COUNT; i++) {
		if (i == lednum) {
			colnm = capscol;
		} else if (num2descr [lednum].current != NULL) {
			colnm = num2descr [lednum].current;
		} else {
			colnm = "UNSET";
		}
		bottom_printf ("\t%s", (intptr_t) colnm);
	}
	bottom_printf ("\n");
}
