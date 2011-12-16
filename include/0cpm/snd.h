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

bool bottom_soundchannel_acceptable_samplerate (uint8_t chan, uint32_t samplerate);
bool bottom_soundchannel_preferred_samplerate  (uint8_t chan, uint32_t samplerate);
void bottom_soundchannel_set_samplerate (uint8_t chan,
                uint32_t samplerate, uint8_t blocksize,
                uint8_t upsample_play, uint8_t downsample_record);

/* Upcalls to indicate that sound can be inserted */

void top_codec_can_play   (uint8_t chan);
void top_codec_can_record (uint8_t chan);

/* Calls to access the buffer space for playback and recording */

int16_t *bottom_play_claim (uint8_t chan);
int16_t *bottom_echo_claim (uint8_t chan);
int16_t *bottom_record_claim (uint8_t chan);
void bottom_play_release (uint8_t chan);
void bottom_record_release (uint8_t chan);


/* Definitions for sound channels and devices.
 * These can be overridden by setting them in device-dependent includes.
 */

#ifndef PHONE_CHANNEL_TELEPHONY
#define PHONE_CHANNEL_TELEPHONY 0
#endif

#ifndef PHONE_CHANNEL_SOUNDCARD
#define PHONE_CHANNEL_SOUNDCARD 0
#endif

#ifndef PHONE_SOUNDDEV_NONE
#define PHONE_SOUNDDEV_NONE SOUNDDEV_NONE
#endif

#ifndef PHONE_SOUNDDEV_HANDSET
#define PHONE_SOUNDDEV_HANDSET SOUNDDEV_HANDSET
#endif

#ifndef PHONE_SOUNDDEV_HEADSET
#define PHONE_SOUNDDEV_HEADSET SOUNDDEV_HEADSET
#endif

#ifndef PHONE_SOUNDDEV_SPEAKER
#define PHONE_SOUNDDEV_SPEAKER SOUNDDEV_SPEAKER
#endif

#ifndef PHONE_SOUNDDEV_LINE
#define PHONE_SOUNDDEV_LINE SOUNDDEV_LINE
#endif


#endif
