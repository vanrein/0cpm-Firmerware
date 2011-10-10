/* Sound calls and data types.
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


#ifndef HEADER_SND
#define HEADER_SND


/* The channel devices that can carry sound */
typedef enum {
	SOUNDDEV_NONE = 0,
	SOUNDDEV_HANDSET,
	SOUNDDEV_HEADSET,
	SOUNDDEV_SPEAKER,
	SOUNDDEV_LINE
} sounddev_t;


/* The codec types that may be supported.
 * The selection is based on cost & patent freedeom, as specified on
 * http://en.wikipedia.org/wiki/Comparison_of_audio_codecs
 */
typedef enum {
	CODEC_L8,
	CODEC_L16,
	CODEC_G711A,
	CODEC_G711MU,
	CODEC_G722,	/* G722 is free, but .1 and .2 are non-free */
	CODEC_G726,
	CODEC_ILBC,
	CODEC_SPEEX,
#ifdef CONFIG_FUNCTION_RADIO_VORBIS
	CODEC_VORBIS,
#endif
	// End marker and the number of codecs:
	CODEC_COUNT
} codec_t;


/* Calls to change sound channels */

void bottom_soundchannel_device (uint8_t chan, sounddev_t dev);
void bottom_soundchannel_setvolume (uint8_t chan, uint8_t vol);
uint8_t bottom_soundchannel_getvolume (uint8_t chan);

/* Upcalls to indicate that sound can be inserted */

uint16_t top_codec_can_play   (uint8_t chan, uint16_t samples);
uint16_t top_codec_can_record (uint8_t chan, uint16_t samples);

/* Calls to play or record through a codec */

void bottom_codec_play_samplerate   (uint8_t chan, uint32_t samplerate);
void bottom_codec_record_samplerate (uint8_t chan, uint32_t samplerate);

int16_t bottom_codec_play   (uint8_t chan, codec_t codec, uint8_t *coded_samples, uint16_t coded_bytes, uint16_t samples);

int16_t bottom_codec_record (uint8_t chan, codec_t codec, uint8_t *coded_samples, uint16_t coded_bytes, uint16_t samples);

void bottom_codec_play_skip (codec_t codec, uint16_t samples);


#endif
