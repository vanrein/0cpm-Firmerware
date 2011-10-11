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

//TODO// test inclusion of bottom definitions
#define BOTTOM
#include <config.h>

#include <0cpm/kbd.h>
#include <0cpm/app.h>
#include <0cpm/cons.h>
#include <0cpm/show.h>
#include <0cpm/snd.h>
#include <0cpm/led.h>


volatile uint16_t tobeplayed   = 0;
volatile uint16_t toberecorded = 0;


timing_t top_timer_expiration (timing_t timeout) {
        /* Keep the linker happy */ ;
	return timeout;
}

void top_hook_update (bool offhook) {
        if (offhook) {
		bottom_soundchannel_device (PHONE_CHANNEL_TELEPHONY, PHONE_SOUNDDEV_HANDSET);
	} else {
		bottom_soundchannel_device (PHONE_CHANNEL_TELEPHONY, PHONE_SOUNDDEV_SPEAKER);
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
	if (bcl != BUTCLS_FIXED_FUNCTION) {
		return;
	}
	switch (cde) {
#ifdef HAVE_BUTTON_UP
	case HAVE_BUTTON_UP:
		bottom_soundchannel_setvolume (PHONE_CHANNEL_TELEPHONY,
			bottom_soundchannel_getvolume (PHONE_CHANNEL_TELEPHONY) + 1);
		break;
#endif
#ifdef HAVE_BUTTON_DOWN
	case HAVE_BUTTON_DOWN:
		bottom_soundchannel_setvolume (PHONE_CHANNEL_TELEPHONY,
			bottom_soundchannel_getvolume (PHONE_CHANNEL_TELEPHONY) - 1);
		break;
#endif
	default:
		break;
	}
}

void top_button_release (void) {
	/* Keep the linker happy */
}

uint16_t plyirqs = 0;
void top_can_play (uint16_t samples) {
	tobeplayed = samples;
	plyirqs++;
}

uint16_t recirqs = 0;
void top_can_record (uint16_t samples) {
	toberecorded = samples;
	recirqs++;
}


#define top_main_delay_1sec top_main
// #define top_main_sine_1khz  top_main


#ifdef CONFIG_FUNCTION_NETCONSOLE
uint8_t netinput [1000];
uint16_t netinputlen;
void nethandler_llconly (uint8_t *pkt, uint16_t pktlen);
#endif


/******** TOP_MAIN FOR A 1 KHZ SINE WAVE OUTPUT ********/

uint8_t sinewave [8] = {
	0x00, 0x5a, 0x7f, 0x5a, 0x00, 0xa5, 0x80, 0xa5
};

void top_main_sine_1khz (void) {
	uint16_t oldirqs = 0;
extern volatile uint16_t available_play;
	top_hook_update (bottom_phone_is_offhook ());
	bottom_codec_play_samplerate (0, 8000);
	bottom_codec_record_samplerate (0, 8000); // Both MUST be called for now
	bottom_soundchannel_setvolume (PHONE_CHANNEL_TELEPHONY, 127);
	bottom_show_fixed_msg (APP_LEVEL_ZERO, FIXMSG_RINGING); // TODO: Not really necessary
	bottom_printf ("Playing 1 kHz tone to speaker or handset\n");
	tobeplayed = 64;
	while (true) {
		uint16_t newplayed = tobeplayed;
		uint16_t oldplayed = newplayed;
		if (oldirqs != plyirqs) {
			bottom_printf ("New playing IRQs detected\n");
			oldirqs = plyirqs;
		}
		while (newplayed >= 8) {
			bottom_codec_play (0, CODEC_L8, sinewave, 8, 8);
			newplayed -= 8;
			bottom_critical_region_end ();
		}
#if 0
		if (oldplayed != newplayed) {
			bottom_printf ("available_play := %d\n", (intptr_t) available_play);
			bottom_printf ("Playbuffer reduced from %d to %d\n", (intptr_t) oldplayed, (intptr_t) newplayed);
		}
#endif

#ifdef CONFIG_FUNCTION_NETCONSOLE
		trysend ();
bottom_led_set (LED_IDX_BACKLIGHT, 1);
		netinputlen = sizeof (netinput);
		if (bottom_network_recv (netinput, &netinputlen)) {
			nethandler_llconly (netinput, netinputlen);
			{ uint32_t ctr = 10000; while (ctr--) ; }
bottom_led_set (LED_IDX_BACKLIGHT, 0);
		}
		trysend ();
#endif

#if defined NEED_KBD_SCANNER_BETWEEN_KEYS || defined NEED_KBD_SCANNER_DURING_KEYPRESS
		bottom_keyboard_scan ();
#endif

#if defined NEED_HOOK_SCANNER_WHEN_ONHOOK || defined NEED_HOOK_SCANNER_WHEN_OFFHOOK
		bottom_hook_scan ();
#endif

	}
}


/******** TOP_MAIN FOR AN ECHO WITH 1 SECOND DELAY ********/

uint16_t playpos = 0;
uint16_t recpos = 0;
uint16_t sampled = 0;

#define abs(x) (((x)>0)?(x):(-(x)))

/* 1 second at 8000 samples per second, plus 25% extra */
uint8_t samples [10000];

void top_main_delay_1sec (void) {
	uint16_t prevsampled = 0;
	uint16_t prevrecirqs = 0;
	uint16_t prevplyirqs = 0;
uint16_t oldspcr1 = 0xffff;
uint16_t loop = 0;
	bottom_critical_region_end ();
	top_hook_update (bottom_phone_is_offhook ());
	bottom_codec_play_samplerate (PHONE_CHANNEL_TELEPHONY, 8000);
	bottom_codec_record_samplerate (PHONE_CHANNEL_TELEPHONY, 8000);
	bottom_soundchannel_setvolume (PHONE_CHANNEL_TELEPHONY, 127);
	bottom_show_fixed_msg (APP_LEVEL_ZERO, FIXMSG_READY); // TODO: Not really necessary
	bottom_printf ("Running the development function \"echo\" (Test sound)\n");
	while (true) {
#if 0
if (loop++ == 0) {
uint8_t reg, subreg;
uint8_t chan = PHONE_CHANNEL_TELEPHONY;
for (reg = 1; reg <= 6; reg++) {
uint8_t val0, val1, val2, val3;
for (subreg = 0; subreg <=3 ; subreg++) {
val0 = tlv320aic2x_getreg (chan, reg);
val1 = tlv320aic2x_getreg (chan, reg);
val2 = tlv320aic2x_getreg (chan, reg);
val3 = tlv320aic2x_getreg (chan, reg);
}
bottom_printf ("TLV_%d = %02x, %02x, %02x, %02x\n", (intptr_t) reg, (intptr_t) val0, (intptr_t) val1, (intptr_t) val2, (intptr_t) val3);
}
}
#endif
#if 0
		if (recirqs != prevrecirqs) {
			bottom_printf ("Record IRQs #%d, ", (intptr_t) recirqs);
			bottom_printf ("available %d\n", (intptr_t) toberecorded);
			prevrecirqs = recirqs;
		}
		if (plyirqs != prevplyirqs) {
			bottom_printf ("Playbk IRQs #%d, ", (intptr_t) plyirqs);
			bottom_printf ("available %d\n", (intptr_t) tobeplayed);
			prevplyirqs = plyirqs;
		}
#endif
{uint16_t xor = oldspcr1 ^ SPCR1_1; if (xor) { oldspcr1 ^= xor; bottom_printf ("SPCR1_1 := 0x%04x\n", oldspcr1); } }
		if (sampled != prevsampled) {
			bottom_printf ("Buffered %d samples\n", (intptr_t) sampled);
			prevsampled = sampled;
		}
		if (toberecorded > 0) {
			int16_t rec = toberecorded;
bottom_led_set (LED_IDX_HANDSET, 1);
			if (sampled + rec > 10000) {
				rec = 10000 - sampled;
			}
			if (recpos + rec > 10000) {
				rec = 10000 - recpos;
			}
			if (rec > 0) {
				bottom_printf ("Recording %d extends buffer from %d to %d\n", (intptr_t) rec, (intptr_t) sampled, (intptr_t) (rec+sampled));
				// Codec implies that #samples and #bytes are the same
				rec -= abs (bottom_codec_record (0, CODEC_G711A, samples + recpos, rec, rec));
				sampled += rec;
				recpos += rec;
				if (recpos >= 10000) {
					recpos = 0;
				}
				bottom_critical_region_begin ();
				//TODO:SPYING-ON-NEXT-LINE// toberecorded -= rec;
				{ extern uint16_t available_record; toberecorded = available_record; }
				bottom_critical_region_end ();
			}
bottom_led_set (LED_IDX_SPEAKERPHONE, 0);
		}
		if (sampled > 8000) {
			uint16_t ply = sampled - 8000;
bottom_led_set (LED_IDX_SPEAKERPHONE, 1);
			if (playpos + ply > 10000) {
				ply = 10000 - playpos;
			}
			if (ply > 0) {
				bottom_printf ("Playback of %d samples reduces buffer from %d to %d\n", (intptr_t) ply, (intptr_t) sampled, (intptr_t) (sampled - ply));
				ply -= abs (bottom_codec_play (0, CODEC_G711A, samples + playpos, ply, ply));
				sampled -= ply;
				playpos += ply;
				if (playpos >= 10000) {
					playpos = 0;
				}
bottom_led_set (LED_IDX_HANDSET, 0);
			}
		}
#ifdef CONFIG_FUNCTION_NETCONSOLE
		// { uint32_t ctr = 10000; while (ctr--) ; }
		trysend ();
		// { uint32_t ctr = 10000; while (ctr--) ; }
bottom_led_set (LED_IDX_BACKLIGHT, 1);
		netinputlen = sizeof (netinput);
		if (bottom_network_recv (netinput, &netinputlen)) {
			nethandler_llconly (netinput, netinputlen);
			{ uint32_t ctr = 10000; while (ctr--) ; }
bottom_led_set (LED_IDX_BACKLIGHT, 0);
		}
		trysend ();
#endif
#if defined NEED_KBD_SCANNER_BETWEEN_KEYS || defined NEED_KBD_SCANNER_DURING_KEYPRESS
		bottom_keyboard_scan ();
#endif
#if defined NEED_HOOK_SCANNER_WHEN_ONHOOK || defined NEED_HOOK_SCANNER_WHEN_OFFHOOK
		bottom_hook_scan ();
#endif
	}
}

