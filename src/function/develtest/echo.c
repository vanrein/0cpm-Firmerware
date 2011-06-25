/* echo.c -- Sound I/O test program
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


/* This program will retrieve sound from the codec (sound chip), hold it for a bit,
 * and send it back out.  When the phone is on-hook, sound communicates over the
 * speakerphone function (if present).  When the phone is off-hook, sound
 * communicates over the handset.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

#include <config.h>

#include <0cpm/kbd.h>
#include <0cpm/app.h>
// #include <0cpm/cons.h>
#include <0cpm/show.h>
#include <0cpm/snd.h>
#include <0cpm/led.h>


volatile uint16_t tobeplayed   = 0;
volatile uint16_t toberecorded = 0;


void top_timer_expiration (timing_t timeout) {
        /* Keep the linker happy */ ;
}

void top_hook_update (bool offhook) {
        if (offhook) {
		bottom_soundchannel_device (0, SOUNDDEV_SPEAKER);
	} else {
		bottom_soundchannel_device (0, SOUNDDEV_HANDSET);
	}
}

void top_network_online (void) {
        /* Keep the linker happy */ ;
}

void top_network_offline (void) {
        /* Keep the linker happy */ ;
}

void top_network_can_send (void) {
        /* Keep the linker happy */ ;
}

void top_network_can_recv (void) {
        /* Keep the linker happy */ ;
}

void top_button_press (buttonclass_t bcl, buttoncode_t cde) {
	/* Keep the linker happy */
}

void top_button_release (void) {
	/* Keep the linker happy */
}

void top_can_play (uint16_t samples) {
	tobeplayed = samples;
}

void top_can_record (uint16_t samples) {
	toberecorded = samples;
}


uint8_t samples [10000];

uint16_t playpos = 0;
uint16_t recpos = 0;
uint16_t sampled = 0;

#define abs(x) (((x)>0)?(x):(-(x)))

void top_main (void) {
	bottom_critical_region_end ();
	top_hook_update (bottom_phone_is_offhook ());
	bottom_codec_play_samplerate (0, 8000);
	bottom_codec_record_samplerate (0, 8000);
	bottom_show_fixed_msg (APP_LEVEL_ZERO, FIXMSG_READY); // TODO: Not really necessary
	while (true) {
		if (toberecorded > 0) {
			int16_t rec = toberecorded;
bottom_led_set (LED_IDX_HANDSET, 1);
			if (rec > 10000 - sampled) {
				rec = 10000 - sampled;
			}
			if (rec > 0) {
				// Codec implies that #samples and #bytes are the same
				rec -= abs (bottom_codec_record (0, CODEC_G711A, samples + recpos, rec, rec));
				recpos += rec;
				sampled += rec;
				bottom_critical_region_begin ();
				toberecorded -= rec;
				bottom_critical_region_end ();
			}
		}
		if (tobeplayed > 0) {
			int16_t ply = tobeplayed;
bottom_led_set (LED_IDX_SPEAKERPHONE, 1);
			if (ply > sampled - 8000) {
				ply = sampled - 8000;
			}
			if (ply > 0) {
				ply -= abs (bottom_codec_play (0, CODEC_G711A, samples + playpos, ply, ply));
				playpos += ply;
				sampled -= ply;
				bottom_critical_region_begin ();
				tobeplayed -= ply;
				bottom_critical_region_end ();
			}
		}
	}
}

