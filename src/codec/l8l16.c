/* l8l16.c -- Codec support for L8 and L16 -- plain samples.
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


/** \ingroup codec
 * The L8 and L16 codecs are part of the audio/video profile for RTP,
 * and simply describe samples that are encoded directly, without any
 * further compression.  L8 is an 8-bit codec of unsigned values after
 * adding a 0x80 offset; L16 describes signed 16-bit integers.
 */


void l8_decode (struct codec *hdl, int16_t *pcm, uint16_t *pcmlen, uint8_t *pkt, uint16_t *pktlen) {
	while ((*pktlen >= 1) && (*pcmlen >= 1)) {
		// Note: The 0x00ff masks away extra bits from 16-bit DSPs like tic55x
		*pcm++ = ((((uint16_t) *pkt++) & 0x00ff) ^ 0x0080) << 8;
		(*pcmlen) --;
		(*pktlen) --;
	}
}

void l8_encode (struct codec *hdl, int16_t *pcm, uint16_t *pcmlen, uint8_t *pkt, uint16_t *pktlen) {
	while ((*pktlen >= 1) && (*pcmlen >= 1)) {
		// Note: The 0xff masks away extra bits from 16-bit DSPs like tic55x
		*pkt++ = (((*pcm++) >> 8) ^ 0x80) & 0xff;
		(*pcmlen) --;
		(*pktlen) --;
	}
}

void l16_decode (struct codec *hdl, int16_t *pcm, uint16_t *pcmlen, uint8_t *pkt, uint16_t *pktlen) {
	while ((*pktlen >= 2) && (*pcmlen >= 1)) {
		// Note: The 0xff masks away extra bits from 16-bit DSPs like tic55x
		*pcm++ = (int16_t) ((((uint16_t) *pkt++) << 8) | ((*pkt++) & 0xff));
		(*pcmlen) --;
		(*pktlen) -= 2;
	}
}

void l16_encode (struct codec *hdl, int16_t *pcm, uint16_t *pcmlen, uint8_t *pkt, uint16_t *pktlen) {
	while ((*pktlen >= 2) && (*pcmlen >= 1)) {
		// Note: The 0xff masks away extra bits from 16-bit DSPs like tic55x
		*pkt++ = ((*pcm) >> 8) & 0xff;
		*pkt++ = (*pcm++) & 0xff;
		(*pcmlen) --;
		(*pktlen) -= 2;
	}
}

