/* wrap.c -- API wrappers for the selected codecs.
 *
 * "Een beetje van jezelf, een beetje is magisch."
 * (Maar wij kunnen het zonder GSM of MSG.)
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


#include <stdint.h>
#include <stdbool.h>

#include <0cpm/codec.h>


/** \defgroup codec
 * Codecs pack and unpack media streams for delivery over RTP.
 * The abilities of codecs are negotiated using SDP-descriptions,
 * commonly sent in the body of a SIP or SAP message.
 */

/** \ingroup codec
 * The codec-wrapper functionality aligns the internal API used
 * by the 0cpm Firmerware with codec software used from other
 * open source projects.
 */



/********** PAYLOAD TYPE ALLOCATIONS **********/

// Pay	Mime-type		Rate	Registry
//
// 0	audio/PCMU		8000	IANA
// 8	audio/PCMA		8000	IANA
// 99	audio/L16		var.	LOCAL (arbitrary sample rate)
// 98	text/t140		1000	LOCAL
// 100  text/red		1000	LOCAL
// 101	audio/telephone-event	8000	LOCAL (but commonly used)
// 110	audio/G726-32		8000	LOCAL
// 111	audio/AAL2-G726-32	8000	LOCAL
// 115	audio/L8		var.	LOCAL
// 116  audio/L16		var.	LOCAL
// 125  audio/x-codec2		2550	LOCAL
// 124  audio/x-codec2		2400	LOCAL
// 123  audio/x-codec2		2000	LOCAL
// 122  audio/x-codec2		1500	LOCAL
// 121  audio/x-codec2		1000?	RESERVED
// 126	audio/speex		var.	LOCAL
// 127	audio/vorbis		var.	LOCAL



static void null_init (struct codec *hdl, uint32_t samplerate) {
	;
}

static void null_finish (struct codec *hdl) {
	;
}


/********** L8 AND L16 DEFINITIONS **********/


#ifdef CONFIG_CODEC_L8_L16

struct codec_fun decoder_l8 = {
	"audio", "L8", NULL,
	"L8",
	null_init, null_finish, l8_decode,
	8000, 115
};

struct codec_fun encoder_l8 = {
	"audio", "L8", NULL,
	"L8",
	null_init, null_finish, l8_encode,
	8000, 115
};

struct codec_fun decoder_l16 = {
	"audio", "L16", NULL,
	"L16",
	null_init, null_finish, l16_decode,
	8000, 116
};

struct codec_fun encoder_l16 = {
	"audio", "L16", NULL,
	"L16",
	null_init, null_finish, l16_encode,
	8000, 116
};

#endif


/********** G.711 DEFINITIONS **********/

#ifdef CONFIG_CODEC_G711


static void g711_init_ulaw (struct codec *hdl, uint32_t samplerate) {
	g711_init (&hdl->state.state_g711, G711_ULAW);
}


static void g711_init_alaw (struct codec *hdl, uint32_t samplerate) {
	g711_init (&hdl->state.state_g711, G711_ALAW);
}

static void g711_finish (struct codec *hdl) {
	g711_release (&hdl->state.state_g711);
}

static void g711_transform_encode (struct codec *hdl,
			int16_t *pcm, uint16_t *pcmlen,
			uint8_t *pkt, uint16_t *pktlen) {
	uint16_t len = *pcmlen;
	if (*pktlen < *pcmlen) {
		len = *pktlen;
	}
	len = g711_encode (&hdl->state.state_g711, pkt, pcm, len);
	*pcmlen = *pktlen = len;
}

static void g711_transform_decode (struct codec *hdl,
			int16_t *pcm, uint16_t *pcmlen,
			uint8_t *pkt, uint16_t *pktlen) {
	uint16_t len = *pcmlen;
	if (*pktlen < *pcmlen) {
		len = *pktlen;
	}
	len = g711_decode (&hdl->state.state_g711, pcm, pkt, len);
	*pcmlen = *pktlen = len;
}

struct codec_fun encoder_g711_ulaw = {
	"audio", "PCMU", NULL,
	"PCMU",
	g711_init_ulaw, g711_finish, g711_transform_encode,
	8000, 0
};

struct codec_fun decoder_g711_ulaw = {
	"audio", "PCMU", NULL,
	"PCMU",
	g711_init_ulaw, g711_finish, g711_transform_decode,
	8000, 0
};

struct codec_fun encoder_g711_alaw = {
	"audio", "PCMA", NULL,
	"PCMA",
	g711_init_alaw, g711_finish, g711_transform_encode,
	8000, 8
};

struct codec_fun decoder_g711_alaw = {
	"audio", "PCMA", NULL,
	"PCMA",
	g711_init_alaw, g711_finish, g711_transform_decode,
	8000, 8
};

#endif



/********** G.722 DEFINITIONS **********/

#ifdef CONFIG_CODEC_G722

static void g722_init_encode (struct codec *hdl, uint32_t samplerate) {
	g722_encode_init (&hdl->state.state_g722_encode, 64000, 0);
}

static void g722_init_decode (struct codec *hdl, uint32_t samplerate) {
	g722_decode_init (&hdl->state.state_g722_decode, 64000, 0);
}

static void g722_finish_encode (struct codec *hdl) {
	g722_encode_release (&hdl->state.state_g722_encode);
}

static void g722_finish_decode (struct codec *hdl) {
	g722_decode_release (&hdl->state.state_g722_decode);
}

static void g722_transform_encode (struct codec *hdl,
			int16_t *pcm, uint16_t *pcmlen,
			uint8_t *pkt, uint16_t *pktlen) {
	uint16_t len = *pcmlen;
	if (*pktlen < *pcmlen) {
		len = *pktlen;
	}
	len = g722_encode (&hdl->state.state_g722_encode, pkt, pcm, len);
	*pcmlen = *pktlen = len;
}

static void g722_transform_decode (struct codec *hdl,
			int16_t *pcm, uint16_t *pcmlen,
			uint8_t *pkt, uint16_t *pktlen) {
	uint16_t len = *pcmlen;
	if (*pktlen < *pcmlen) {
		len = *pktlen;
	}
	len = g722_decode (&hdl->state.state_g722_decode, pcm, pkt, len);
	*pcmlen = *pktlen = len;
}

struct codec_fun encoder_g722_historically_incorrect = {
	"audio", "G722", NULL,
	"HD-G722",
	g722_init_encode, g722_finish_encode, g722_transform_encode,
	8000 /*history error*/, 9
};
struct codec_fun decoder_g722_historically_incorrect = {
	"audio", "G722", NULL,
	"HD-G722",
	g722_init_decode, g722_finish_decode, g722_transform_decode,
	8000 /*history error*/, 9
};
struct codec_fun encoder_g722 = {
	"audio", "G722", NULL,
	"HD-G722",
	g722_init_encode, g722_finish_encode, g722_transform_encode,
	16000, 9
};
struct codec_fun decoder_g722 = {
	"audio", "G722", NULL,
	"HD-G722",
	g722_init_decode, g722_finish_decode, g722_transform_decode,
	16000, 9
};
#endif



/********** G.726 DEFINITIONS **********/


#ifdef CONFIG_CODEC_G726


static void g726_init_encode (struct codec *hdl, uint32_t samplerate) {
	/* TODO: G726_ENCODING_ULAW... and/or _ALAW? RTP profile? */
	g726_init (&hdl->state.state_g726_encode, 32000, G726_ENCODING_ULAW, G726_PACKING_LEFT);
}

static void g726_init_decode (struct codec *hdl, uint32_t samplerate) {
	/* TODO: G726_ENCODING_ULAW... and/or _ALAW? RTP profile? */
	g726_init (&hdl->state.state_g726_decode, 32000, G726_ENCODING_ULAW, G726_PACKING_LEFT);
}

static void g726aal2_init_encode (struct codec *hdl, uint32_t samplerate) {
	/* TODO: G726_ENCODING_ULAW... and/or _ALAW? RTP profile? */
	g726_init (&hdl->state.state_g726_encode, 32000, G726_ENCODING_ULAW, G726_PACKING_RIGHT);
}

static void g726aal2_init_decode (struct codec *hdl, uint32_t samplerate) {
	/* TODO: G726_ENCODING_ULAW... and/or _ALAW? RTP profile? */
	g726_init (&hdl->state.state_g726_decode, 32000, G726_ENCODING_ULAW, G726_PACKING_RIGHT);
}


static void g726_finish_encode (struct codec *hdl) {
	g726_release (&hdl->state.state_g726_encode);
}

static void g726_finish_decode (struct codec *hdl) {
	g726_release (&hdl->state.state_g726_decode);
}

static void g726_transform_encode (struct codec *hdl,
			int16_t *pcm, uint16_t *pcmlen,
			uint8_t *pkt, uint16_t *pktlen) {
	uint16_t len = *pcmlen;
	if (*pktlen < *pcmlen) {
		len = *pktlen;
	}
	len = g726_encode (&hdl->state.state_g726_encode, pkt, pcm, len);
	*pcmlen = *pktlen = len;
}

static void g726_transform_decode (struct codec *hdl,
			int16_t *pcm, uint16_t *pcmlen,
			uint8_t *pkt, uint16_t *pktlen) {
	uint16_t len = *pcmlen;
	if (*pktlen < *pcmlen) {
		len = *pktlen;
	}
	len = g726_decode (&hdl->state.state_g726_decode, pcm, pkt, len);
	*pcmlen = *pktlen = len;
}



struct codec_fun encoder_g726 = {
	"audio", "G726-32", NULL,
	"DECT",
	g726_init_encode, g726_finish_encode, g726_transform_encode,
	8000, 110
};
struct codec_fun decoder_g726 = {
	"audio", "G726-32", NULL,
	"DECT",
	g726_init_decode, g726_finish_decode, g726_transform_decode,
	8000, 110
};

struct codec_fun encoder_g726aal2 = {
	"audio", "AAL2-G726-32", NULL,
	"DECT",
	g726aal2_init_encode, g726_finish_encode, g726_transform_encode,
	8000, 111
};
struct codec_fun decoder_g726aal2 = {
	"audio", "AAL2-G726-32", NULL,
	"DECT",
	g726aal2_init_decode, g726_finish_decode, g726_transform_decode,
	8000, 111
};
#endif



/********** SPEEX DEFINITIONS **********/

#if defined (CONFIG_CODEC_SPEEX_NARROWBAND) || defined (CONFIG_CODEC_SPEEX_WIDEBAND) || defined (CONFIG_CODEC_SPEEX_ULTRAWIDEBAND)

void speex_init_encode (struct codec *hdl, uint32_t samplerate) {
	speex_encoder_init (&hdl->state.state_speex_encode);
}
void speex_finish_encode (struct codec *hdl) {
	speex_encoder_destroy (&hdl->state.state_speex_encode);
}
void speex_transform_encode (struct codec *hdl,
			uint16_t *pcm, uint16_t *pcmlen,
			uint8_t  *pkt, uint16_t *pktlen) {
	speex_encode (&hdl->state.state_speex_encode, NULL /*TODO*/);
}
void speex_init_decode (struct codec *hdl, uint32_t samplerate) {
	speex_decoder_init (&hdl->state.state_speex_decode);
}
void speex_finish_decode (struct codec *hdl) {
	speex_decoder_destroy (&hdl->state.state_speex_decode);
}
void speex_transform_decode (struct codec *hdl, uint16_t *pktlen) {
	speex_decode (&hdl->state.state_speex_decode, NULL /*TODO*/);
}

#endif

#ifdef CONFIG_CODEC_SPEEX_NARROWBAND
struct codec_fun encoder_speex_narrow = {
	"audio", "speex", "mode=any;vbr=on;cng=on",
	"SPEEX",
	speex_init_encode, speex_finish_encode, speex_transform_encode,
	8000, 126
};
struct codec_fun decoder_speex_narrow = {
	"audio", "speex", "mode=any;vbr=on;cng=on",
	"SPEEX",
	speex_init_decode, speex_finish_decode, speex_transform_decode,
	8000, 126
};
#endif

#ifdef CONFIG_CODEC_SPEEX_WIDEBAND
struct codec_fun encoder_speex_wide = {
	"audio", "speex", "mode=any;vbr=on;cng=on",
	"SPEEX",
	speex_init_encode, speex_finish_encode, speex_transform_encode,
	16000, 126
};
struct codec_fun decoder_speex_wide = {
	"audio", "speex", "mode=any;vbr=on;cng=on",
	"SPEEX",
	speex_init_decode, speex_finish_decode, speex_transform_decode,
	16000, 126
};
#endif

#ifdef CONFIG_CODEC_SPEEX_ULTRAWIDEBAND
struct codec_fun encoder_speex_ultra = {
	"audio", "speex", "mode=any;vbr=on;cng=on",
	"SPEEX",
	speex_init_encode, speex_finish_encode, speex_transform_encode,
	32000, 126
};
struct codec_fun decoder_speex_ultra = {
	"audio", "speex", "mode=any;vbr=on;cng=on",
	"SPEEX",
	speex_init_decode, speex_finish_decode, speex_transform_decode,
	32000, 126
};
#endif


/********** CODEC2 DEFINITIONS **********/

static void codec2_init (struct codec *hdl, uint32_t samplerate) {
	//TODO// Static allocation
	codec2_create (&hdl->state.codec2_state);
}

static void codec2_finish (struct codec *hdl, uint32_t samplerate) {
	codec2_destroy (&hdl->state.codec2_state);
}

static void codec2_transform_encode (struct codec *hdl,
			int16_t *pcm, uint16_t *pcmlen,
			uint8_t *pkt, uint16_t *pktlen) {
	codec2_encode (hdl, pkt, pcm);
	*pcmlen = CODEC2_SAMPLES_PER_FRAME;
	*pktlen = (CODEC2_BITS_PER_FRAME + 7) >> 3;
}

static void codec2_transform_decode (struct codec *hdl,
			int16_t *pcm, uint16_t *pcmlen,
			uint8_t *pkt, uint16_t *pktlen) {
	codec2_decode (hdl, pcm, pkt);
	*pcmlen = CODEC2_SAMPLES_PER_FRAME;
	*pktlen = (CODEC2_BITS_PER_FRAME + 7) >> 3;
}


#ifdef CONFIG_CODEC_CODEC2
struct codec_fun encoder_codec2_1500 = {
	"audio", "x-codec2", NULL,
	"CODEC2",
	codec2_init, codec2_finish, codec2_transform_encode,
	1500, 122
};
struct codec_fun decoder_codec2_1500 = {
	"audio", "x-codec2", NULL,
	"CODEC2",
	codec2_init, codec2_finish, codec2_transform_decode,
	1500, 122
};
struct codec_fun encoder_codec2_2000 = {
	"audio", "x-codec2", NULL,
	"CODEC2",
	codec2_init, codec2_finish, codec2_transform_encode,
	2000, 123
};
struct codec_fun decoder_codec2_2000 = {
	"audio", "x-codec2", NULL,
	"CODEC2",
	codec2_init, codec2_finish, codec2_transform_decode,
	2000, 123
};
struct codec_fun encoder_codec2_2400 = {
	"audio", "x-codec2", NULL,
	"CODEC2",
	codec2_init, codec2_finish, codec2_transform_encode,
	2400, 124
};
struct codec_fun decoder_codec2_2400 = {
	"audio", "x-codec2", NULL,
	"CODEC2",
	codec2_init, codec2_finish, codec2_transform_decode,
	2400, 124
};
struct codec_fun encoder_codec2_2550 = {
	"audio", "x-codec2", NULL,
	"CODEC2",
	codec2_init, codec2_finish, codec2_transform_encode,
	2550, 125
};
struct codec_fun decoder_codec2_2550 = {
	"audio", "x-codec2", NULL,
	"CODEC2",
	codec2_init, codec2_finish, codec2_transform_decode,
	2550, 125
};
#endif


/********** TELEPHONE EVENT DEFINITIONS **********/

#ifdef CONFIG_SIP_PHONE
struct codec_fun encodor_televt = {
	"audio", "telephone-event", "0-16",
	"DTMF",
	NULL, NULL, NULL, /*TODO*/
	8000, 101
};
struct codec_fun decoder_televt = {
	"audio", "telephone-event", "0-16",
	"DTMF",
	NULL, NULL, NULL, /*TODO*/
	8000, 101
};
#endif

/********** REALTIME TEXT DEFINITIONS **********/

#ifdef CONFIG_CODEC_RTT
struct codec_fun encoder_rtt = {
	"text", "t140", "cps=1000",
	"TEXT",
	NULL, NULL, NULL, /*TODO*/
	1000, 98
};
struct codec_fun decoder_rtt = {
	"text", "t140", "cps=1000",
	"TEXT",
	NULL, NULL, NULL, /*TODO*/
	1000, 98
};
struct codec_fun encoder_rtt_red = {
	"text", "red", "98/98/98",
	"TEXT",
	NULL, NULL, NULL, /*TODO*/
	1000, 100
};
struct codec_fun decoder_rtt_red = {
	"text", "red", "98/98/98",
	"TEXT",
	NULL, NULL, NULL, /*TODO*/
	1000, 100
};
#endif

#ifdef CONFIG_MULTICAST_CODEC_RTT
struct codec_fun decoder_mcast_rtt = {
	"text", "t140", "cps=1000",
	"TEXT",
	NULL, NULL, NULL, /*TODO*/
	1000, 98
};
struct codec_fun decoder_mcast_rtt_red = {
	"text", "red", "98/98/98",
	"TEXT",
	NULL, NULL, NULL, /*TODO*/
	1000, 100
};
#endif

#ifdef CONFIG_MULTICAST_CODEC_L16
struct codec_fun decoder_mcast_l16 = {
	"audio", "L16", NULL,
	"L16",
	NULL, NULL, NULL, /*TODO*/
	0, 99
};
#endif

#ifdef CONFIG_MULTICAST_CODEC_VORBIS
struct codec_fun decoder_mcast_vorbis = {
	"audio", "vorbis", "configuration=TODO:RFC5215",
	"VORBIS",
	NULL, NULL, NULL, /*TODO*/
	0, 127
};
#endif



/********** SDP SUPPORT THROUGH CODEC LIST STRUCTURES **********/

struct codec_fun *codec_phone_audio_encoders [] = {
#ifdef CONFIG_CODEC_SPEEX_ULTRAWIDEBAND
	&encoder_speex_ultra,
#endif
#ifdef CONFIG_CODEC_SPEEX_WIDEBAND
	&encoder_speex_wide,
#endif
#ifdef CONFIG_CODEC_G722
	&encoder_g722,
	&encoder_g722_historically_incorrect,
#endif
#ifdef CONFIG_CODEC_SPEEX_NARROWBAND
	&encoder_speex_narrow,
#endif
#ifdef CONFIG_CODEC_G726
	&encoder_g726,
#endif
#ifdef CONFIG_CODEC_G711
	&encoder_g711_ulaw,	/* 14 bits */
	&encoder_g711_alaw,	/* 13 bits */
#endif
#ifdef CONFIG_CODEC_CODEC2
	&encoder_codec2,
#endif
#ifdef CONFIG_SIP_PHONE
	&encoder_televt,
#endif
	NULL
};

struct codec_fun *codec_phone_audio_decoders [] = {
#ifdef CONFIG_CODEC_SPEEX_ULTRAWIDEBAND
	&decoder_speex_ultra,
#endif
#ifdef CONFIG_CODEC_SPEEX_WIDEBAND
	&decoder_speex_wide,
#endif
#ifdef CONFIG_CODEC_G722
	&decoder_g722,
	&decoder_g722_historically_incorrect,
#endif
#ifdef CONFIG_CODEC_SPEEX_NARROWBAND
	&decoder_speex_narrow,
#endif
#ifdef CONFIG_CODEC_G726
	&decoder_g726,
#endif
#ifdef CONFIG_CODEC_G711
	&decoder_g711_ulaw,	/* 14 bits */
	&decoder_g711_alaw,	/* 13 bits */
#endif
#ifdef CONFIG_CODEC_CODEC2
	&decoder_codec2,
#endif
#ifdef CONFIG_SIP_PHONE
	&decoder_televt,
#endif
	NULL
};

struct codec_fun *codec_phone_video_encoders [] = {
	NULL
};

struct codec_fun *codec_phone_video_decoders [] = {
	NULL
};

struct codec_fun *codec_phone_text_encoders [] = {
#ifdef CONFIG_CODEC_RTT
	&encoder_rtt_red,
	&encoder_rtt,
#endif
	NULL
};

struct codec_fun *codec_phone_text_decoders [] = {
#ifdef CONFIG_CODEC_RTT
	&decoder_rtt_red,
	&decoder_rtt,
#endif
	NULL
};

struct codec_fun *codec_mcast_audio_decoders [] = {
#ifdef CONFIG_MULTICAST_CODEC_L16
	&decoder_mcast_l16,
#endif
#ifdef CONFIG_MULTICAST_CODEC_VORBIS
	&decoder_mcast_vorbis,
#endif
#ifdef CONFIG_MULTICAST_CODEC_SPEEX
	&decoder_mcast_speex,
#endif
	NULL
};

struct codec_fun *codec_mcast_video_decoders [] = {
	NULL
};

struct codec_fun *codec_mcast_text_decoders [] = {
#ifdef CONFIG_CODEC_RTT
	&encoder_rtt_red,
	&encoder_rtt,
#endif
	NULL
};

