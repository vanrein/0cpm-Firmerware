/* TLV320AIC20/21/24/25/20K/24K codec driver
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

/*
 * The name "codec" is generally used for sound I/O chips that take a
 * realtime stream of samples and map send them to speakers; and that
 * do the opposite for microphone input.
 *
 * This driver has a generic part, described here, and a lower part
 * that actually gets the generated byte patterns to and from the chip,
 * which fits better in the actual phone implementation and/or in the
 * DSP/CPU chip driver.
 *
 * The assumption made in this driver is that there is a single unit
 * of this codec, and that its outputs are used as prescribed in the
 * datasheet for headset in/out, handset in/out and handsfree in/out
 * inasfar as these facilities are configured on the target phone in
 * HAVE_xxx variables.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include <config.h>
#include <0cpm/snd.h>
#include <0cpm/cons.h>


/* The code below distinguishes the two directions of using the
 * TLV320AIC2x codec as play and record:
 *  - Play   translates from the digital domain to domain
 *  - Record translates from the sound domain to the digital
 *
 * A buffer exists for each direction, and the host facilities
 * (interrupts, DMA) are exploited to move data as efficiently
 * as possible.
 *
 * Communincation normally does not wrap around the boundaries
 * of the buffer.  That is, DMA will end a block at the end of
 * the buffer, just to start over right away.  Similarly, the
 * communication with the top is in terms of bytes left but
 * never more than to the end of the buffers.  The top has to
 * adapt to partial deliveries anyway, it is aware of that.
 */


/******** "Local" function definitions ********/


uint8_t tlv320aic2x_getreg (uint8_t channel, uint8_t reg);
void tlv320aic2x_setreg (uint8_t channel, uint8_t reg, uint8_t val);
void tlv320aic2x_set_samplerate (uint32_t samplerate);
void dmahint_play   (void);
void dmahint_record (void);


/******** Global variables ********/


/* A stored version of the current volume, defaulting halfway */
static uint8_t volume [2];


/* Read and write buffers, with counters of read-ahead */

#define BUFSZ (64*25)

volatile uint16_t samplebuf_play   [BUFSZ];
volatile uint16_t samplebuf_record [BUFSZ];

volatile uint16_t available_play   = 0;
volatile uint16_t available_record = 0;

volatile uint16_t threshold_play   = 64;
volatile uint16_t threshold_record = 64;

static uint16_t nextwrite_play   = 0;
static uint16_t nextwrite_record = 0;

static uint16_t nextread_play    = 0;
static uint16_t nextread_record  = 0;


/******** Calls to change sound channels ********/


/* Setup the sound channel to use: SOUNDDEV_NONE, _HANDSET, _HEADSET,
 * _SPEAKER and _LINE.  When set to SOUNDDEV_NONE, the chip is in setup
 * in configuration programming mode; in any other modes, it is setup
 * in continuous data transfer mode.
 */
void bottom_soundchannel_device (uint8_t chan, sounddev_t dev) {
	if (dev != SOUNDDEV_NONE) {
		tlv320aic2x_setreg (chan, 3, 0x00 | 0x01); /* power up */
	}
	switch (dev) {
	case SOUNDDEV_NONE:
		// Microphone input could act as a random seed
		// (such material would be hashed to aid privacy)
		// (but for now just powerdown the codec)
		tlv320aic2x_setreg (chan, 6, 0x00 | 0x04);
		tlv320aic2x_setreg (chan, 6, 0x80 | 0x00);
		//TODO:TMP_NO_POWERDOWN// tlv320aic2x_setreg (chan, 3, 0x00 | 0x31); /* power down */
		break;
	case SOUNDDEV_HANDSET:
		tlv320aic2x_setreg (chan, 6, 0x00 | 0x02);
		tlv320aic2x_setreg (chan, 6, 0x80 | 0x02);
		break;
	case SOUNDDEV_HEADSET:
		tlv320aic2x_setreg (chan, 6, 0x00 | 0x01);
		tlv320aic2x_setreg (chan, 6, 0x80 | 0x01);
		break;
	case SOUNDDEV_SPEAKER:
		tlv320aic2x_setreg (chan, 6, 0x00 | 0x04);
		tlv320aic2x_setreg (chan, 6, 0x80 | 0x08);
		break;
	case SOUNDDEV_LINE:
		tlv320aic2x_setreg (chan, 6, 0x00 | 0x08);
		tlv320aic2x_setreg (chan, 6, 0x80 | 0x04);
		break;
	default:
		break;
	}
}

/* Retrieve the current volume */
uint8_t bottom_soundchannel_getvolume (uint8_t chan) {
	return volume [chan];
}

/* Set the current volume, but stay aware of wrap-around and beyond-range
 * attempts.
 */
void bottom_soundchannel_setvolume (uint8_t chan, uint8_t vol) {
	if (vol >= 200) {
		/* wrap-around due to decrement */
		vol = 0;
	} else if (vol > 31) {
		/* out-of-range due to increment (top does not know range) */
		vol = 31;
	} else {
		/* regular value: literal copy or proper increment/decrement */
	}
	volume [chan] = vol;
	tlv320aic2x_setreg (chan, 5, (0x40 | 31) - vol);
}


/******** Interrupt and DMA handling to indicate that sound can be inserted ********/


//TODO:IRQHANDLER:CALLS// uint16_t top_codec_can_play   (uint8_t chan, uint16_t samples);
//TODO:IRQHANDLER:CALLS// uint16_t top_codec_can_record (uint8_t chan, uint16_t samples);


/******** Calls to play or record through a codec ********/


int16_t codec_decode (codec_t codec, uint8_t *in, uint16_t inlen, uint16_t *out, uint16_t outlen);
int16_t codec_encode (codec_t codec, uint16_t *in, uint16_t inlen, uint8_t *out, uint16_t outlen);
void tlv320aic2x_set_samplerate (uint32_t samplerate);


void bottom_codec_play_samplerate   (uint8_t chan, uint32_t samplerate) {
	/* TODO: Filters only work up to rate 26000 (bandwidth 11700);
	 * TODO: Consider 2x oversampling and no filter for higher rates?
	 * TODO: Stop oversampling for really high frequencies, over 52000
	 * TODO: Disable playback for out-of-range frequencies, over 104000
	 * TODO: Consider disabling over 52000 (which is already excessive)
	 * TODO: Consider downsampling of rediculously high rates
	 */
	tlv320aic2x_set_samplerate (samplerate);
}

void bottom_codec_record_samplerate (uint8_t chan, uint32_t samplerate) {
	// No setting to make: documentation says to call both with the same values
	// bottom_codec_play_samplerate (chan, samplerate);
	dmahint_record ();
}

int16_t bottom_codec_play   (uint8_t chan, codec_t codec, uint8_t *coded_samples, uint16_t coded_bytes, uint16_t samples) {
	int16_t retval;
	//TODO// Guard against buffer wraparound
	retval = codec_decode (codec,
				coded_samples, coded_bytes,
				(uint16_t *) (samplebuf_play + nextwrite_play), samples);
	nextwrite_play += samples;
	available_play += samples;
	if (retval < 0) {
		nextwrite_play += retval;
		available_play += retval;
	}
	if (nextwrite_play >= BUFSZ) {
		nextwrite_play -= BUFSZ;
	}
	dmahint_play ();
	return retval;
}

int16_t bottom_codec_record (uint8_t chan, codec_t codec, uint8_t *coded_samples, uint16_t coded_bytes, uint16_t samples) {
	int16_t retval;
	//TODO// Guard against buffer wraparound
	uint16_t ar = available_record;
	if (samples > ar) {
		samples = ar;
	}
	retval = codec_encode (codec,
				(uint16_t *) (samplebuf_play + nextread_play), samples,
				coded_samples, coded_bytes);
	nextread_record += samples;
	available_record -= samples;
	if (retval < 0) {
		nextread_record += retval;
		available_record -= retval;
	}
	dmahint_record ();
	return retval;
	//TODO:ALT-API-TEST// return available_record;
}

void bottom_codec_play_skip (codec_t codec, uint16_t samples) {
	uint16_t ctr;
	//TODO: Better fill up with previous sample
	if (available_play + samples > BUFSZ) {
		samples = BUFSZ - available_play;
	}
	ctr = samples;
	while (ctr-- > 0) {
		samplebuf_play [nextwrite_play++] = 0x0000;
		if (nextwrite_play >= BUFSZ) {
			nextwrite_play -= BUFSZ;
		}
	}
	available_play += samples;
	dmahint_play ();
}


/******** Initialisation of this module ********/


/* Setup the communication with the codec, initialise the chip
 */
void tlv320aic2x_setup_sound (void) {
	uint8_t chan;
	// Setup the various registers in the TLV320AIC2x
	for (chan = 0; chan < 2; chan++) {
		tlv320aic2x_setreg (chan, 1,        0x49);	/* Continuous 16-bit, 2.35V bias */
		tlv320aic2x_setreg (chan, 2,        0xa0);	/* Turbo mode */
		/* Setup to SOUNDDEV_NONE below will power down the device */
		tlv320aic2x_setreg (chan, 3, 0x40 | 0x20);	/* Arrange mute with volume 0 */
		/* Ignore register 3C */
		tlv320aic2x_setreg (chan, 3, 0xc0 | 0x00);	/* Switch off LCD DAC */
		/* Bypass MNP setup, it is phone-speficic */
		tlv320aic2x_setreg (chan, 5, 0x00 | 0x20);	/* ADC gain 27 dB -- ok? */
		bottom_soundchannel_setvolume (chan, 16);	/* DAC gain -24 dB initially */
		tlv320aic2x_setreg (chan, 5, 0x80 | 0x00);	/* No sidetones */
		tlv320aic2x_setreg (chan, 5, 0xc0 | 0x30);	/* SPKR gain +3 dB -- ok? */
		bottom_soundchannel_device (chan, SOUNDDEV_NONE);/* No input/output, powerdown */
	}
}
