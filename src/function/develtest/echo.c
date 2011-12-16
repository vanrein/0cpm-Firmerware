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

//TODO// test inclusion of bottom and text definitions
#define BOTTOM
#include <config.h>
#include <0cpm/text.h>

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
void top_codec_can_play (uint8_t chan) {
	plyirqs++;
}

uint16_t recirqs = 0;
void top_codec_can_record (uint8_t chan) {
	recirqs++;
}


#define top_main_sine_1khz  top_main
// #define top_main_delay_1sec top_main


#ifdef CONFIG_FUNCTION_NETCONSOLE
uint8_t netinput [1000];
uint16_t netinputlen;
void nethandler_llconly (uint8_t *pkt, uint16_t pktlen);
#endif


/******** TOP_MAIN FOR A 1 KHZ SINE WAVE OUTPUT ********/

uint8_t sinewaveL8 [64] = {
	// 8-bit samples with 0x80 offset
	0x80, 0xda, 0xff, 0xda, 0x80, 0x25, 0x00, 0x25,
	0x80, 0xda, 0xff, 0xda, 0x80, 0x25, 0x00, 0x25,
	0x80, 0xda, 0xff, 0xda, 0x80, 0x25, 0x00, 0x25,
	0x80, 0xda, 0xff, 0xda, 0x80, 0x25, 0x00, 0x25,
	0x80, 0xda, 0xff, 0xda, 0x80, 0x25, 0x00, 0x25,
	0x80, 0xda, 0xff, 0xda, 0x80, 0x25, 0x00, 0x25,
	0x80, 0xda, 0xff, 0xda, 0x80, 0x25, 0x00, 0x25,
	0x80, 0xda, 0xff, 0xda, 0x80, 0x25, 0x00, 0x25
};

int8_t sinewaveL16 [128] = {
	// 16-bit signed values, encoded as byte pairs H,L
	0,0, 45,65, 63,255, 45,65, 0,0, 210,191, 192,1, 210,191,
	0,0, 45,65, 63,255, 45,65, 0,0, 210,191, 192,1, 210,191,
	0,0, 45,65, 63,255, 45,65, 0,0, 210,191, 192,1, 210,191,
	0,0, 45,65, 63,255, 45,65, 0,0, 210,191, 192,1, 210,191,
	0,0, 45,65, 63,255, 45,65, 0,0, 210,191, 192,1, 210,191,
	0,0, 45,65, 63,255, 45,65, 0,0, 210,191, 192,1, 210,191,
	0,0, 45,65, 63,255, 45,65, 0,0, 210,191, 192,1, 210,191,
	0,0, 45,65, 63,255, 45,65, 0,0, 210,191, 192,1, 210,191
};

void top_main_sine_1khz (void) {
	uint16_t oldirqs = 0;
extern volatile uint16_t available_play;
extern volatile uint16_t available_record;
uint8_t l16ctr = 1;
	bottom_critical_region_end ();
	if (!bottom_soundchannel_acceptable_samplerate (PHONE_CHANNEL_TELEPHONY, 8000)) {
		bottom_printf ("Failed to set sample rate");
		bottom_show_fixed_msg (APP_LEVEL_ZERO, FIXMSG_CALL_ENDED);
		exit (1);
	}
	bottom_soundchannel_set_samplerate (PHONE_CHANNEL_TELEPHONY, 8000, 64, 1, 1);
	bottom_soundchannel_setvolume (PHONE_CHANNEL_TELEPHONY, 127);
	bottom_show_fixed_msg (APP_LEVEL_ZERO, FIXMSG_RINGING);
	bottom_printf ("Playing 1 kHz tone to speaker or handset\n");
	top_hook_update (bottom_phone_is_offhook ());
	while (true) {
#if 0
if (SPCR2_1 & REGVAL_SPCR2_XRDY) { DXR1_1 = sinewaveL16 [l16ctr++]; if (l16ctr == 8) { l16ctr = 0; } }
{ uint16_t _ = DRR1_1; }
#else
		do {
			int16_t *outbuf = bottom_play_claim (PHONE_CHANNEL_TELEPHONY);
			uint16_t pcmlen, pktlen;
			if (!outbuf) {
				break;
			}
#if 1
			pcmlen = 64;
			pktlen = 64;
			// Note: No handle needed for stateless L8
			l8_decode (NULL, outbuf, &pcmlen, sinewaveL8, &pktlen);
#else
			pcmlen = 64;
			pktlen = 128;
			// Note: No handle needed for stateless L16
			l16_decode (NULL, outbuf, &pcmlen, sinewaveL16, &pktlen);
#endif
			bottom_play_release (PHONE_CHANNEL_TELEPHONY);
		} while (true);
#endif

#ifdef CONFIG_FUNCTION_NETCONSOLE
		// { uint32_t ctr = 10000; while (ctr--) ; }
		trysend ();
		// { uint32_t ctr = 10000; while (ctr--) ; }
bottom_led_set (LED_IDX_BACKLIGHT, 1);
		netinputlen = sizeof (netinput);
		if (bottom_network_recv (netinput, &netinputlen)) {
			nethandler_llconly (netinput, netinputlen);
bottom_led_set (LED_IDX_BACKLIGHT, 0);
			{ uint32_t ctr = 10000; while (ctr--) ; }
		}
bottom_led_set (LED_IDX_BACKLIGHT, 0);
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
	uint16_t prepblocks = 0;
	uint16_t playptr = 2000, recptr = 0;
memset (samples, 0x33, sizeof (samples));
	bottom_critical_region_end ();
	if (!bottom_soundchannel_acceptable_samplerate (PHONE_CHANNEL_TELEPHONY, 8000)) {
		bottom_printf ("Failed to set sample rate");
		bottom_show_fixed_msg (APP_LEVEL_ZERO, FIXMSG_CALL_ENDED);
		exit (1);
	}
	bottom_soundchannel_set_samplerate (PHONE_CHANNEL_TELEPHONY, 8000, 100, 1, 1);
	bottom_soundchannel_setvolume (PHONE_CHANNEL_TELEPHONY, 127);
	bottom_show_fixed_msg (APP_LEVEL_ZERO, FIXMSG_READY);
	bottom_printf ("Running the development function \"echo\" (Test sound)\n");
	top_hook_update (bottom_phone_is_offhook ());
	while (true) {
		int16_t *buf;
		uint16_t pcmlen, pktlen;

		do {
			buf = bottom_play_claim (PHONE_CHANNEL_TELEPHONY);
		} while (buf == NULL);
		pcmlen = 100;
		pktlen = 100;
		// Note: No handle needed for stateless L8
		l8_encode (NULL, buf, &pcmlen, samples + playptr, &pktlen);
		bottom_play_release (PHONE_CHANNEL_TELEPHONY);

		playptr += 100;
		if (playptr >= 10000) {
			playptr -= 10000;
		}

		do {
			buf = bottom_record_claim (PHONE_CHANNEL_TELEPHONY);
		} while (buf == NULL);
		pcmlen = 100;
		pktlen = 100;
		// Note: No handle needed for stateless L8
		l8_decode (NULL, buf, &pcmlen, samples + recptr, &pktlen);

		recptr += 100;
		if (recptr >= 10000) {
			recptr -= 10000;
		}

#ifdef CONFIG_FUNCTION_NETCONSOLE
		// { uint32_t ctr = 10000; while (ctr--) ; }
		trysend ();
		// { uint32_t ctr = 10000; while (ctr--) ; }
bottom_led_set (LED_IDX_BACKLIGHT, 1);
		netinputlen = sizeof (netinput);
		if (bottom_network_recv (netinput, &netinputlen)) {
			nethandler_llconly (netinput, netinputlen);
bottom_led_set (LED_IDX_BACKLIGHT, 0);
			{ uint32_t ctr = 10000; while (ctr--) ; }
		}
bottom_led_set (LED_IDX_BACKLIGHT, 0);
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

