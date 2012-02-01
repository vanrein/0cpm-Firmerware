/* Grandstream BT20x driver as an extension to the tic55x driver
 *
 * Ideally, all this would be is wiring to the generic functions of
 * chips connected to the DSP.  And of course a lot of register setup
 * code.  In practice, it is not as sharply divided, sometimes for
 * reasons of efficiency, sometimes for other reasons.  The ideal is
 * the best judgement however, and any debate on where code should go
 * should be based on this ideal plus the realism that too much
 * indirection will slow down a program that is going to deal with
 * static hardware anyway.
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


#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#define BOTTOM
#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/timer.h>
#include <0cpm/led.h>
#include <0cpm/kbd.h>
#include <0cpm/app.h>
#include <0cpm/show.h>
#include <0cpm/flash.h>
#include <0cpm/snd.h>
#include <0cpm/cons.h>

#include <bottom/ht162x.h>
#include <bottom/ksz8842.h>



/******** EXTERNAL INTERRUPTS SERVICE ROUTINES ********/


interrupt void tic55x_int0_isr (void) {
	tic55x_top_has_been_interrupted = true;
	ksz8842_interrupt_handler ();
}

#if 0
interrupt void tic55x_int1 (void) {
	tic55x_top_has_been_interrupted = true;
	//TODO//
}

interrupt void tic55x_int2 (void) {
	tic55x_top_has_been_interrupted = true;
	//TODO//
}

interrupt void tic55x_int3 (void) {
	tic55x_top_has_been_interrupted = true;
	//TODO//
}
#endif



/******** FLASH PARTITION ACCESS ********/


/* An external definition (usually in phone-specific code)
 * contains an array of at least one entry of flashpart
 * structures.  Only the last will have the FLASHPART_FLAG_LAST
 * flag set.
 *
 * There will usually be one partition with name ALLFLASH.BIN
 * that covers the entire flash memory, including things that
 * may not actually be in any partition.  Usually, this is the
 * last entry in the flash partition table.
 */
struct flashpart bottom_flash_partition_table [] = {
	{ FLASHPART_FLAG_LAST, "ALLFLASH.BIN", 0, 4096 }
};


/* Read a 512-byte block from flash.
 * The return value indicates success.
 */
bool bottom_flash_read (uint16_t blocknr, uint8_t data [512]) {
	uint32_t flashidx;
	uint16_t ctr = 0;
	if (blocknr >= 4096) {
		return false;
	}
	flashidx = ((uint32_t) blocknr) * 256;
	while (ctr < 512) {
		uint16_t sample = flash_16 [flashidx];
		flashidx += 1;
		// data [ctr++] = (sample >> 24) & 0xff;
		// data [ctr++] = (sample >> 16) & 0xff;
		data [ctr++] = (sample >>  8) & 0xff;
		data [ctr++] =  sample	      & 0xff;
	}
	return true;
}


/* Write a 512-byte block to flash.  It is assumed that this
 * is done sequentially; any special treatment for a header
 * page will be done by the bottom layer, not the top.
 * The return value indicates success.
 */
bool boot_flash_write (uint16_t blocknr, uint8_t data [512]) {
	return false;
}


/* Retrieve the current phone's MAC address from Flash.
 */
void bottom_flash_get_mac (uint8_t mac [6]) {
	uint32_t flashidx = flash_offset_mymac;
	uint16_t ctr = 0;
	while (ctr < 6) {
		uint16_t sample = flash_16 [flashidx];
		flashidx++;
		mac [ctr++] = (sample >> 8) & 0xff;
		mac [ctr++] =  sample       & 0xff;
	}
}



/******** TLV320AIC20K PROGRAMMING ACCESS OVER I2C ********/


/* The codec can be programmed over I2C, so the low-level routines
 * for driving the codec end up passing bytes over I2C.  The
 * procedure of cycling through subregisters is not performed here.
 */

void tlv320aic2x_setreg (uint8_t channel, uint8_t reg, uint8_t val) {
	// Wait as long as the bus is busy
bottom_led_set (LED_IDX_MESSAGE, 1);
	while (I2CSTR & REGVAL_I2CSTR_BB) {
		;
	}
// bottom_printf ("I2C.pre  = 0x%04x,0x%04x\n", (intptr_t) I2CSTR, (intptr_t) I2CMDR);
	// Set transmission mode for 2 bytes to "channel"
	I2CSAR = 0x40 | channel;
	//TODO// I2CSAR = 0x00;	// Broadcast
	I2CCNT = 2;
	I2CDXR = reg;
	// Send the register index
	// Initiate the transfer by setting STT and STP flags
	I2CMDR = REGVAL_I2CMDR_TRX | REGVAL_I2CMDR_MST | REGVAL_I2CMDR_STT | REGVAL_I2CMDR_STP | REGVAL_I2CMDR_NORESET | REGVAL_I2CMDR_FREE | REGVAL_I2CMDR_BC_8;
// bottom_printf ("I2C.set  = 0x%04x,0x%04x\n", (intptr_t) I2CSTR, (intptr_t) I2CMDR);
	// Wait for the START condition to occur
// bottom_led_set (LED_IDX_HANDSET, 1);
	while (I2CMDR & REGVAL_I2CMDR_STT) {
		;
	}
// bottom_printf ("I2C.stt  = 0x%04x,0x%04x\n", (intptr_t) I2CSTR, (intptr_t) I2CMDR);
// bottom_led_set (LED_IDX_HANDSET, 0);
	// Wait until the I2C bus is ready, then send the value
// bottom_led_set (LED_IDX_SPEAKERPHONE, 1);
	while (!(I2CSTR & REGVAL_I2CSTR_XRDY)) {
		if (I2CSTR & REGVAL_I2CSTR_NACK) {
			// bottom_printf ("I2C received NACK\n");
			I2CSTR = REGVAL_I2CSTR_NACK;
			I2CMDR = REGVAL_I2CMDR_MST | REGVAL_I2CMDR_STP | REGVAL_I2CMDR_NORESET | REGVAL_I2CMDR_FREE | REGVAL_I2CMDR_BC_8;
			return;
		}
	}
// bottom_printf ("I2C.xrdy = 0x%04x,0x%04x\n", (intptr_t) I2CSTR, (intptr_t) I2CMDR);
// bottom_led_set (LED_IDX_SPEAKERPHONE, 0);
	I2CDXR = val;
	// Wait for the STOP condition to occur
	while (I2CMDR & REGVAL_I2CMDR_STP) {
		;
	}
// bottom_printf ("I2C.post = 0x%04x,0x%04x\n", (intptr_t) I2CSTR, (intptr_t) I2CMDR);
bottom_led_set (LED_IDX_MESSAGE, 0);
}

uint8_t tlv320aic2x_getreg (uint8_t channel, uint8_t reg) {
	uint8_t val;
uint32_t ctr;
bottom_led_set (LED_IDX_MESSAGE, 1);
// bottom_led_set (LED_IDX_HANDSET, 0);
	// Wait as long as the bus is busy
	while (I2CSTR & REGVAL_I2CSTR_BB) {
		;
	}
	// Set transmission mode for 1 byte to "channel"
	I2CSAR = 0x40 | channel;
	//TODO// I2CSAR = 0x00;	// Broadcast
	I2CCNT = 1;
	I2CDXR = reg;
	// Send the register index
	// Initiate the transfer by setting STT flag, but withhold STP
	I2CMDR = REGVAL_I2CMDR_TRX | REGVAL_I2CMDR_MST | REGVAL_I2CMDR_STT | /* REGVAL_I2CMDR_STP | */ REGVAL_I2CMDR_NORESET | REGVAL_I2CMDR_FREE | REGVAL_I2CMDR_BC_8;
	// Wait until the START condition has occurred
	while (I2CMDR & REGVAL_I2CMDR_STT) {
		;
	}
	// ...send address and write mode bit...
// bottom_led_set (LED_IDX_HANDSET, 1);
	// Wait until ready to setup for receiving
	// while (I2CMDR & REGVAL_I2CMDR_STP) {
	while (I2CSTR & (REGVAL_I2CSTR_XRDY | REGVAL_I2CSTR_XSMT) != (REGVAL_I2CSTR_XRDY | REGVAL_I2CSTR_XSMT)) {
		if (I2CSTR & (REGVAL_I2CSTR_NACK | REGVAL_I2CSTR_AL)) {
			I2CSTR = REGVAL_I2CSTR_NACK | REGVAL_I2CSTR_AL;
			I2CMDR = REGVAL_I2CMDR_MST | REGVAL_I2CMDR_STP | REGVAL_I2CMDR_NORESET | REGVAL_I2CMDR_FREE | REGVAL_I2CMDR_BC_8;
			return 0;
		}
	}
// bottom_led_set (LED_IDX_HANDSET, 0);
	I2CCNT = 1;
	// Restart with STT flag, also permit stop with STP; do not set TRX
	I2CMDR = REGVAL_I2CMDR_MST | /* REGVAL_I2CMDR_STT | */ REGVAL_I2CMDR_STP | REGVAL_I2CMDR_NORESET | REGVAL_I2CMDR_FREE | REGVAL_I2CMDR_BC_8;
	// ...recv val...
	while (!(I2CSTR & REGVAL_I2CSTR_RRDY)) {
#if 0
		if (I2CSTR & REGVAL_I2CSTR_NACK) {
			I2CSTR = REGVAL_I2CSTR_NACK;
			// I2CMDR = REGVAL_I2CMDR_MST | REGVAL_I2CMDR_STP | REGVAL_I2CMDR_NORESET | REGVAL_I2CMDR_FREE | REGVAL_I2CMDR_BC_8;
			// return 0;
		}
#endif
		;
	}
	val = I2CDRR;
bottom_led_set (LED_IDX_MESSAGE, 0);
	return val;
}


/******** TLV320AIC20K DATA ACCESS OVER MCBSP1 ********/


#define BUFSZ (80*24)

static volatile int16_t samplebuf_play   [BUFSZ];
static volatile int16_t samplebuf_record [BUFSZ];

static uint16_t samplebuf_blocksize = 80;
static uint16_t samplebuf_wrapindex = BUFSZ;

/* The buffer is a contiguous array of samples, made accessible
 * to the top layer per block.  Upon setting sample rate and
 * block size, all pointers start at the beginning of the buffer.
 * While processing, the pointers advance with one blocksize at
 * a time; they wrap around if there is not enough space left to
 * cover a complete block within the bounds of the sample buffer.
 *
 * The buffer has three types of areas, each with their own
 * consumer/producer relationships.  The areas are the same for
 * the playback and recording buffer.  They are:
 *  - playable: Can be filled be the codec for playback
 *  - dmaready: Can be played/recorded through lockstep DMA
 *  - recordable: Can be processed by the codec in recording
 * Each of these areas is marked with an index pointer; their
 * ends +1 are marked by another index pointer, as follows:
 *
 *	AREA		START	END+1
 *	playable	play0	rec0
 *	dmaready	dma0	play0
 *	recordable	rec0	dma0
 *
 * DMA is possible if the dmaready area is non-empty, so if
 * dma0 != play0.  After DMA has transferred a block,
 * the dma0 pointer is incremented, and this is checked.
 *
 * bottom_record_claim() succeeds if rec0 != dma0, and when
 * bottom_record_release() is called, rec0 increments.  Note
 * that bottom_echo_claim() applies the same condition, but
 * it returns the rec0 offset of the playbuf, instead of the
 * rec0 offset in the recbuf as is returned by record_claim.
 *
 * bottom_play_claim() succeeds if play0 != rec0 *or* if the
 * buffer is empty, which is noticed if DMA is not active. This
 * means that playback is the first thing to start when a new
 * set of index pointers is setup with zero values.  When the
 * bottom_play_release() is called, play0 increments, and the
 * DMA conditions are evaluated.
 */

volatile bool dma_active = false;
/*TODO:static*/ volatile uint16_t bufofs_play0 = 0;
/*TODO:static*/ volatile uint16_t bufofs_dma0  = 0;
/*TODO:static*/ volatile uint16_t bufofs_rec0  = 0;


inline void bottom_bufferdma_progress (uint8_t chan) {
	bool recordinghint = (bufofs_dma0 == bufofs_rec0);
	bufofs_dma0 = (bufofs_dma0 + samplebuf_blocksize) % samplebuf_wrapindex;
	if (bufofs_dma0 == bufofs_play0) {
		SPCR2_1 &= ~REGVAL_SPCR2_FRST_NOTRESET;		// Stop DMA
		dma_active = false;
	}
	if (recordinghint) {
		top_codec_can_record (chan);
	}
}

int16_t *bottom_play_claim (uint8_t chan) {
	if ((!dma_active) || (bufofs_play0 != bufofs_rec0)) {
		return (int16_t *) &samplebuf_play [bufofs_play0];
	} else {
		return NULL;
	}
}

void bottom_play_release (uint8_t chan) {
	bool dmahint = (bufofs_play0 == bufofs_dma0);
	bufofs_play0 = (bufofs_play0 + samplebuf_blocksize) % samplebuf_wrapindex;
	if (dmahint) {
		//TODO// Following 5 lines can be done much earlier
		(void) DRR1_1;	// Flag down RFULL
		(void) DRR1_1;
		DMACCR_0 |= REGVAL_DMACCR_EN;
		DXR1_1 = DXR1_1;	// Flag down XEMPTY
		DMACCR_1 |= REGVAL_DMACCR_EN;
		SPCR2_1 |= REGVAL_SPCR2_FRST_NOTRESET;		// Start DMA
		dma_active = true;
	}
}

int16_t *bottom_record_claim (uint8_t chan) {
	if (bufofs_rec0 != bufofs_dma0) {
		return (int16_t *) &samplebuf_record [bufofs_rec0];
	} else {
		return NULL;
	}
}

int16_t *bottom_echo_claim (uint8_t chan) {
	if (bufofs_rec0 != bufofs_dma0) {
		return (int16_t *) &samplebuf_play [bufofs_rec0];
	} else {
		return NULL;
	}
}

void bottom_record_release (uint8_t chan) {
	bool playbackhint = (bufofs_rec0 == bufofs_play0);
	bufofs_rec0 = (bufofs_rec0 + samplebuf_blocksize) % samplebuf_wrapindex;
	if (playbackhint) {
		top_codec_can_play (chan);
	}
}

static int TODO_setratectr = 0;

/* Set a frequency divisor for the intended sample rate */
//TODO// Not all this code is properly split between generic TLV and specific BT200
void tlv320aic2x_set_samplerate (uint8_t chan, uint32_t samplerate) {
	uint16_t m, n, p;
	SPCR2_1 |= REGVAL_SPCR2_GRST_NOTRESET | REGVAL_SPCR2_FRST_NOTRESET;
//TRY-WITHOUT// { uint32_t ctr = 100000; while (ctr-- > 0) ; }	// Helps to sync TLV --TODO-- wait4what?
	SPCR1_1 |= REGVAL_SPCR1_RRST_NOTRESET;
	SPCR2_1 |= REGVAL_SPCR2_XRST_NOTRESET;
//TRY-WITHOUT// { uint32_t ctr = 100000; while (ctr-- > 0) ; }	// Helps to sync TLV --TODO-- wait4what?
// Getting in tune with the TLV probably means waiting
// until it has picked up changes.  This means waiting
// for a FS to occur.  One way of doing that is to see
// when a server overrun (RFULL) occurs.
#if 1
(void) DRR1_1;		// Flag down RFULL
while (! (SPCR1_1 & REGVAL_SPCR1_RFULL) ) {
	;
}
#endif
	DXR1_1 = DXR1_1;	// Flag down XEMPTY
tlv320aic2x_setreg (chan, 3, 0x31);	// Channel offline
//TRY-WITHOUT// { uint32_t ctr = 10000; while (ctr-- > 0) ; }
	(void) DRR1_1;		// Flag down RFULL
	(void) DRR1_1;
	// Determine the dividors m, n and p
	n = 1;
	p = 2;
	m = ( 30720000 / 16 ) / ( n * p * samplerate );
	if ((m & 0x03) == 0x00) {
		// Save PLL energy without compromising accuracy
		p = 8;		// Factor 2 -> 8 so multiplied by 4
		m >>= 2;	// Divide by 4
	}
	while (m > 128) {
		m >>= 1;
		n <<= 1;
	}
{ uint8_t ip4 [4]; ip4 [0] = m; ip4 [1] = n; ip4 [2] = p; ip4 [3] = ++TODO_setratectr; bottom_show_ip4 (APP_LEVEL_CONNECTING, ip4); }
#ifdef CONFIG_FUNCTION_NETCONSOLE
bottom_printf ("TLV320AIC20K setting: M=%d, N=%d, P=%d\n", (intptr_t) m, (intptr_t) n, (intptr_t) p);
#endif
	m &= 0x7f;
	n &= 0x0f;	// Ignore range problems?
	p &= 0x07;
	// With the codec up and running, configure the sample rate
	tlv320aic2x_setreg (chan, 4, 0x00 | (n << 3) | p);
	tlv320aic2x_setreg (chan, 4, 0x80 | m);
{ uint32_t ctr = 1000; while (ctr-- > 0) ; }
// #ifndef TODO_FS_ONLY_DURING_SOUND_IO
	// SPCR2_1 |= REGVAL_SPCR2_GRST_NOTRESET | REGVAL_SPCR2_FRST_NOTRESET;
	// SPCR1_1 |= REGVAL_SPCR1_RRST_NOTRESET;
	// SPCR2_1 |= REGVAL_SPCR2_XRST_NOTRESET;
	SPCR1_1 &= ~REGVAL_SPCR1_RRST_NOTRESET;
	SPCR2_1 &= ~REGVAL_SPCR2_XRST_NOTRESET;
{ uint32_t ctr = 10000; while (ctr-- > 0) ; }
	SPCR2_1 &= ~ ( REGVAL_SPCR2_GRST_NOTRESET | REGVAL_SPCR2_FRST_NOTRESET );
{ uint32_t ctr = 100000; while (ctr-- > 0) ; }	// Helps to sync TLV --TODO-- wait4what?
	// SPCR2_1 &= ~ ( REGVAL_SPCR2_FRST_NOTRESET );
{ uint32_t ctr = 10000; while (ctr-- > 0) ; }
// #endif
	samplerate = 12288000 / samplerate;
	if (samplerate >= 4096) {
		samplerate = 4096;
	} else if (samplerate == 0) {
		samplerate = 1;
	}
	SRGR2_1 = REGVAL_SRGR2_CLKSM | REGVAL_SRGR2_FSGM | (samplerate - 1);
// #ifndef TODO_FS_ONLY_DURING_SOUND_IO
	// SPCR2_1 |= REGVAL_SPCR2_GRST_NOTRESET | REGVAL_SPCR2_FRST_NOTRESET;
	SPCR2_1 |= REGVAL_SPCR2_GRST_NOTRESET;
tlv320aic2x_setreg (chan, 3, 0x01);	// Channel online
{ uint32_t ctr = 1000; while (ctr-- > 0) ; }
	// tlv320aic2x_setreg (chan, 4, 0x00 | (n << 3) | p);
	// tlv320aic2x_setreg (chan, 4, 0x80 | m);
// { uint32_t ctr = 10000; while (ctr-- > 0) ; }
	SPCR1_1 |= REGVAL_SPCR1_RRST_NOTRESET;
	SPCR2_1 |= REGVAL_SPCR2_XRST_NOTRESET;
{ uint32_t ctr = 1000; while (ctr-- > 0) ; }
	//NOT_FOR_NEW_STYLE_BUFFERS// SPCR2_1 |= REGVAL_SPCR2_FRST_NOTRESET;
{ uint32_t ctr = 10000; while (ctr-- > 0) ; }
	DXR1_1 = DXR1_1;	// Flag down XEMPTY
	(void) DRR1_1;	// Flag down RFULL
	(void) DRR1_1;
{ uint32_t ctr = 10000; while (ctr-- > 0) ; }
// #endif
}

void bottom_soundchannel_set_samplerate (uint8_t chan, uint32_t samplerate,
			uint8_t blocksize, uint8_t upsample_play, uint8_t downsample_record) {
	//
	// Setup the buffersize of the sample buffers
	samplebuf_blocksize = blocksize;
	samplebuf_wrapindex = (BUFSZ / blocksize) * blocksize;
	//
	// Setup hardware with the requested sample rate (but no FRST generated)
	tlv320aic2x_set_samplerate (chan, samplerate);
	//
	// Setup buffer index pointers at the start; effectively clearing all
	dma_active = false;
	bufofs_play0 = 0;
	bufofs_dma0  = 0;
	bufofs_rec0  = 0;
}

/* A full frame of 64 samples has been recorded.  See if space exists for
 * another, otherwise disable DMA until a dmahint_play() restarts it.
 * TODO: No processing needed for RX_DMA_IRQ, as TX_DMA_IRQ comes later.
 */
interrupt void tic55x_dmac0_isr (void) {
#ifdef PREFER_OLD_STUFF
	uint16_t irq = DMACSR_0;	// Note causes and clear
	tic55x_top_has_been_interrupted = true;
	if ((available_record += 64) > (BUFSZ - 64)) {
		SPCR2_1 &= ~REGVAL_SPCR2_XRST_NOTRESET;
		SPCR2_1 |=  REGVAL_SPCR2_XRST_NOTRESET;
		DMACCR_0 &= ~REGVAL_DMACCR_EN;
// #ifdef TODO_FS_ONLY_DURING_SOUND_IO
		// SPCR1_1 &= ~REGVAL_SPCR1_RRST_NOTRESET;
		// if ((SPCR2_1 & REGVAL_SPCR2_XRST_NOTRESET) == 0) {
			// SPCR2_1 &= ~ (REGVAL_SPCR2_GRST_NOTRESET | REGVAL_SPCR2_FRST_NOTRESET);
		// }
// #endif
	}
	if (available_record >= threshold_record) {
		top_codec_can_record (available_record);
	}
#endif
}

/* A full frame of 64 samples has been played.  See if another is availabe,
 * otherwise disable DMA until a dmahint_record() restarts it.
 */
interrupt void tic55x_dmac1_isr (void) {
#ifdef PREFER_OLD_STUFF
	uint16_t irq = DMACSR_1;	// Note causes and clear
	uint16_t toplay;
	tic55x_top_has_been_interrupted = true;
	if ((available_play -= 64) < 64) {
// #ifdef TODO_FS_ONLY_DURING_SOUND_IO
		// SPCR2_1 &= ~REGVAL_SPCR2_XRST_NOTRESET;
		// if ((SPCR1_1 & REGVAL_SPCR1_RRST_NOTRESET) == 0) {
			// SPCR2_1 &= ~ (REGVAL_SPCR2_GRST_NOTRESET | REGVAL_SPCR2_FRST_NOTRESET);
		// }
// #endif
		DMACCR_1 &= ~REGVAL_DMACCR_EN;
	}
	toplay = BUFSZ - available_play;
	if (BUFSZ - available_play >= threshold_play) {
		top_codec_can_play (available_play);
	}
#endif
	bottom_bufferdma_progress (0);
}

#ifdef PREFER_OLD_STUFF
/* Data has been removed from what was recorded.  As a result,
 * it may be possible to restart DMA channel 1 if it was disabled.
 */
void dmahint_record (void) {
	if (! (DMACCR_0 & REGVAL_DMACCR_EN)) {
		if (available_record <= (BUFSZ - 64)) {
			if (!(DMACCR_0 & REGVAL_DMACCR_EN)) {
				(void) DRR1_1;	// Flag down RFULL
				(void) DRR1_1;
				DMACCR_0 |= REGVAL_DMACCR_EN;
// #ifdef TODO_FS_ONLY_DURING_SOUND_IO
				// SPCR1_1 |= REGVAL_SPCR1_RRST_NOTRESET;
				// SPCR2_1 |= REGVAL_SPCR2_GRST_NOTRESET | REGVAL_SPCR2_FRST_NOTRESET;
// #endif
			}
// bottom_printf ("dmahint_record() enabled DMA from %d bytes out of %d\n", (intptr_t) available_record, (intptr_t) BUFSZ);
		}
	}
}
#endif

#ifdef PREFER_OLD_STUFF
/* New data has been written for playback.  As a result, it may
 * be possible to restart DMA channel 0 if it was disabled.
 */
void dmahint_play (void) {
	if ((available_play >= 64) && ! (DMACCR_1 & REGVAL_DMACCR_EN)) {
		DXR1_1 = DXR1_1;	// Flag down XEMPTY
		DMACCR_1 |= REGVAL_DMACCR_EN;
#ifdef CONFIG_FUNCTION_NETCONSOLE
bottom_printf ("dmahint_play() started playing DMA\n");
#endif
// #ifdef TODO_FS_ONLY_DURING_SOUND_IO
		// SPCR2_1 |= REGVAL_SPCR2_XRST_NOTRESET | REGVAL_SPCR2_GRST_NOTRESET | REGVAL_SPCR2_FRST_NOTRESET;
// #endif
	}
//TODO:DEBUG// else bottom_printf ("dmahint_play() did not start playing -- available_play = %d\n", (intptr_t) available_play);
}
#endif



/******** HT162x LCD DRIVER LOW-LEVEL FUNCTIONS ********/


/* The program defines an array of LCDCMD_xxx to
 * initialise the HT162x chip.  The array should be
 * terminated with HT162x_LCDCMD_DONE.
 */
extern uint16_t ht162x_setup_cmdseq [] = {
	HT162x_LCDPREFIX_CMD,
	HT162x_LCDCMD_LCDOFF,
	HT162x_LCDCMD_NORMAL,
	HT162x_LCDCMD_RC256K,
	HT162x_LCDCMD_SYSEN,
	HT162x_LCDCMD_LCDON,
	HT162x_LCDCMD_TIMERDIS,
	HT162x_LCDCMD_WDTDIS,
	HT162x_LCDCMD_TONEOFF,
	HT162x_LCDCMD_BIAS3_4,
	HT162x_LCDCMD_TONE2K,
	// Terminate the sequence:
	HT162x_LCDCMD_DONE
};


/* Chip-enable or -disable the LCD driver */
void ht162x_chipselect (bool selected) {
	if (selected) {
		IODATA &= ~ (1 << 1);
	} else {
		IODATA |=   (1 << 1);
	}
}


/* Send out a databit for the LCD driver, but do not clock it in */
static uint16_t kbdisp_outbuf = 0x0000;
void ht162x_databit_prepare (bool databit) {
	if (databit) {
		kbdisp_outbuf &= ~0x4040;
		kbdisp_outbuf |=  0x8080;
	} else {
		kbdisp_outbuf &= ~0xc0c0;
	}
	kbdisp = kbdisp_outbuf;
}

/* Clock in the database that was prepared for the LCD driver */
void ht162x_databit_commit (void) {
	kbdisp_outbuf |= 0x4040;
	kbdisp = kbdisp_outbuf;
}


/******** LCD-SPECIFIC FORM PAINTING ROUTINES ********/


/* Digit notations on the LCD's 7 segment displays */
uint8_t lcd7seg_digits [10] = {
	0xaf, 0xa0, 0xcb, 0xe9, 0xe4,	// 01234
	0x6d, 0x6f, 0xa8, 0xef, 0xed 	// 56789
};

/* Alphabetic notations on the LCD's 7 segment displays */
uint8_t lcd7seg_alpha [26] = {
	0xee, 0x67, 0x0f, 0xe3, 0x4f,	// AbCdE
	0x4e, 0x2f, 0xe6, 0x06, 0xa1,	// FGHIJ
	0x6e, 0x07, 0x6a, 0x62, 0x63,	// kLMno
	0xce, 0xec, 0x42, 0x6d, 0x47,	// PQRST
	0xa7, 0x23, 0x2b,		// Uvw
	0x49, 0xe5, 0xc2
};

/* Put a character into display byte idx, with ASCII code ch */
void ht162x_putchar (uint8_t idx, uint8_t ch, bool notify) {
	uint8_t chup = ch & 0xdf;
	uint8_t lcd7seg;
	if ((ch >= '0') && (ch <= '9')) {
		lcd7seg = lcd7seg_digits [ch - '0'];
	} else if ((chup >= 'A') && (chup <= 'Z')) {
		lcd7seg = lcd7seg_alpha [chup - 'A'];
	} else if (chup == 0x00) {
		lcd7seg = 0x00;  // space for chup one of 0x00 and 0x20
	} else {
		lcd7seg = 0x40;  // dash
	}
	ht162x_dispdata [idx] = (ht162x_dispdata [idx] & 0x10) | lcd7seg;
	if (notify) {
		ht162x_dispdata_notify (idx, idx);
	}
}

uint8_t lcd_bars [8] = {
	0x00, 0x20, 0x60, 0xe0, 0xe8, 0xec, 0xee, 0xef
};

/* Level indication for volume, showing the upper 3 bits of "level" */
void ht162x_audiolevel (uint8_t level) {
	uint8_t bars = lcd_bars [level >> 5];
	ht162x_dispdata [15] = (ht162x_dispdata [15] & 0x10) | bars;
	ht162x_dispdata_notify (15, 15);
}

#define LCDSYM_NETWORK	14
#define LCDSYM_HANDSET	13
#define LCDSYM_SPEAKER	11
#define LCDSYM_FORWARD	10
#define LCDSYM_CLOCK	8
#define LCDSYM_LOCK	7
#define LCDSYM_BINARY	15
#define LCDSYM_CN1	4
#define LCDSYM_CN2	5
#define LCDSYM_HOUR10	0
#define LCDSYM_HMSEP	1
#define LCDSYM_AM	2
#define LCDSYM_PM	3
#define LCDSYM_DOT12	12
#define LCDSYM_DOT9	9
#define LCDSYM_DOT6	6

/* Special flag setting on the LCD, to flag conditions */
void ht162x_led_set (uint8_t lcdidx, led_colour_t col, bool notify) {
	if (col) {
		ht162x_dispdata [lcdidx] |=  0x10;
	} else {
		ht162x_dispdata [lcdidx] &= ~0x10;
	}
	if (notify) {
		ht162x_dispdata_notify (lcdidx, lcdidx);
	}
}


/******** BOTTOM FUNCTIONS FOR LEDS AND BUTTONS ********/


/* Bottom-half operations to manipulate LED states */
void bottom_led_set (led_idx_t ledidx, led_colour_t col) {
	switch (ledidx) {
	case LED_IDX_HANDSET:
		ht162x_led_set (13, col, true);
		break;
	case LED_IDX_SPEAKERPHONE:
		ht162x_led_set (11, col, true);
		break;
	case LED_IDX_MESSAGE:
		if (col != 0) {
			asm (" bset xf");
		} else {
			asm (" bclr xf");
		}
		break;
	case LED_IDX_BACKLIGHT:
		// Set bit DXSTAT=5 in PCR=0x2812 to 1/0
		if (col != 0) {
			PCR0 |=     1 << REGBIT_PCR_DXSTAT  ;
		} else {
			PCR0 &= ~ ( 1 << REGBIT_PCR_DXSTAT );
		}
		break;
	default:
		break;
	}
}


/* See if the phone (actually, the horn) is offhook */
bool bottom_phone_is_offhook (void) {
	// The hook switch is attached to GPIO pin 5
	return (IODATA & 0x20) != 0;
}


/* Scan to see if the top_hook_update() function must be called */
#if !defined NEED_HOOK_SCANNER_WHEN_ONHOOK || !defined NEED_HOOK_SCANNER_WHEN_OFFHOOK
#  error "The BT200 does not generate an interrupt on hook contact changes"
#endif
static bool bt200_offhook = false;
void bottom_hook_scan (void) {
	if (bt200_offhook != bottom_phone_is_offhook ()) {
		bt200_offhook = !bt200_offhook;
		top_hook_update (bt200_offhook);
	}
}

/* Scan to see if the top_button_press() or top_button_release()
 * functions must be called.
 *
 * The following bits are written to activate one or more keyboard rows:
 * D0..D4 are sent to the variable "kbdisp" which has the proper address.
 * The intermediate variable "kbdisp_outbuf" stores a copy of the bits,
 * and is shared with the LCD routines so unused bits must be retained.
 *
 * The following bits of McBSP0 are read from PCR0 as keyboard columns:
 * C0=CLKRP, C1=CLKXP, C2=FSRP, C3=FSXP, C4=DRSTAT
 *
 * The following routine is designed from the assumption that the keyboard
 * is operated a single key at a time; if not, then any response could be
 * valid.  Note that we do not suppress multiple keys by sending an error
 * code, or behaving like with key release.
 */
#if !defined NEED_KBD_SCANNER_BETWEEN_KEYS || !defined NEED_KBD_SCANNER_DURING_KEYPRESS
#  error "The BT200 does not generate an interrupt on key changes"
#endif
#define KBD_COLUMNS_MASK ( REGVAL_PCR_CLKRP | REGVAL_PCR_CLKXP | REGVAL_PCR_FSRP | REGVAL_PCR_FSXP | REGVAL_PCR_DRSTAT )
static bool bt200_kbd_pressed = false;
static const buttoncode_t keynum2code  [25] = {
	'1',		'2',		'3',		HAVE_BUTTON_MESSAGE,	HAVE_BUTTON_HOLD,
	'4',		'5',		'6',		HAVE_BUTTON_TRANSFER,	HAVE_BUTTON_CONFERENCE,
	'7',		'8',		'9',		HAVE_BUTTON_FLASH,	HAVE_BUTTON_MUTE,
	'*',		'0',		'#',		HAVE_BUTTON_SEND,	HAVE_BUTTON_SPEAKER,
	HAVE_BUTTON_DOWN, HAVE_BUTTON_UP, HAVE_BUTTON_CALLERS, HAVE_BUTTON_CALLED, HAVE_BUTTON_MENU
};
void bottom_keyboard_scan (void) {
	uint16_t scan;
	scan = PCR0 & KBD_COLUMNS_MASK;
	if (bt200_kbd_pressed) {
		// Respond if the key is released
		if (scan == KBD_COLUMNS_MASK) {
			bt200_kbd_pressed = false;
			kbdisp_outbuf &= 0xe0e0;		// Make D0..D4 low
			kbdisp = kbdisp_outbuf;
			top_button_release ();
		}
	} else {
		// Respond if a key is being pressed
		if (scan != KBD_COLUMNS_MASK) {
			uint16_t row;
			for (row = 0; row < 5; row++) {
				kbdisp_outbuf |=   0x1f1f;	// Make D0..D4 high
				kbdisp_outbuf &= ~(0x0101 << row); // Set D$row low
				kbdisp = kbdisp_outbuf;
				{ volatile int pause = 5; while (pause > 0) pause--; }
				scan = PCR0 & KBD_COLUMNS_MASK;
				if (scan != KBD_COLUMNS_MASK) {
					uint16_t col;
					for (col = 0; col < 5; col++) {
						if (! (scan & (0x01 << col))) {
							uint16_t keynum = row * 5 + col;
							buttonclass_t bcl = ((row <= 3) && (col <= 2))? BUTCLS_DTMF: BUTCLS_FIXED_FUNCTION;
							buttoncode_t  cde = keynum2code  [keynum];
							bt200_kbd_pressed = true;
							top_button_press (bcl, cde);
							return;
						}
					}
				}
			}
			// Failed, usually due to an I/O glitch
			kbdisp_outbuf &= 0xe0e0;		// Make D0..D4 low
			kbdisp = kbdisp_outbuf;
		}
	}
}


/******** BOTTOM FUNCTIONS FOR FORMATTING / PRINTING INFORMATION ********/


/* Send an elapsed period (not a wallclock time) to the display.
 * For BT200, this is shown in the top digits as MM:SS or as HH:MM.
 * Note that there are 2 positions, and the first digit can only show a 1.
 */
static app_level_t bt200_period_level = APP_LEVEL_ZERO;
void bottom_show_period (app_level_t level, uint8_t h, uint8_t m, uint8_t s) {
	if (bt200_period_level > level) {
		// Ignore those lower values, as this is highly time-dependent
		return;
	}
	bt200_period_level = level;
	if ((h == 0) && (m <= 19)) {
		h = m;
		m = s;
	}
	ht162x_led_set (2, false,   false); // "AM" off
	ht162x_led_set (3, false,   false); // "PM" off
	ht162x_led_set (1, true,    false); // ":" in "xx:xx"
	ht162x_led_set (0, h >= 10, false); // "1" in "1x:xx"
	ht162x_putchar (0, (h %  10) + '0', false);
	ht162x_putchar (1, (m /  10) + '0', false);
	ht162x_putchar (2, (m %  10) + '0', false);
	ht162x_dispdata_notify (0, 3);
}

/* The level of the main portion of the display, along with an array
 * with the contents at each level
 */
static bt200_displine_level = APP_LEVEL_ZERO;
static bool    bt200_level_active [APP_LEVEL_COUNT];
static uint8_t bt200_level_maintext [APP_LEVEL_COUNT] [12];
static uint8_t bt200_level_dotbits  [APP_LEVEL_COUNT];

/* Internal routine to move a display level to the display.
 * Note that no "led" style bits other than the dots in the main
 * line are influenced.
 */
static void bt200_displine_showlevel (app_level_t level) {
	uint8_t i;
	for (i = 0; i < 12; i++) {
		ht162x_putchar (14 - i, bt200_level_maintext [level] [i], false);
	}
	ht162x_led_set (12, (bt200_level_dotbits [level] & 0x01), false);
	ht162x_led_set ( 9, (bt200_level_dotbits [level] & 0x02), false);
	ht162x_led_set ( 6, (bt200_level_dotbits [level] & 0x04), false);
	ht162x_dispdata_notify (3, 14);
}

/* Internal routine to send a string to the display, either with or
 * without setting the various dots.  The dots are coded in bits
 * 0, 1 and 2 of the dotbits parameter, from left to right.
 */
static void bt200_display_showtxt (app_level_t level, char *txt, uint8_t dotbits) {
	// 1. Print text to the proper level
	int idx = 0;
	while ((idx < 12) && *txt) {
		bt200_level_maintext [level] [idx++] = *txt++;
	}
	while (idx < 12) {
		bt200_level_maintext [level] [idx++] = ' ';
	}
	bt200_level_dotbits [level] = dotbits;
	bt200_level_active [level] = true;
	// 2. If the level is not exceeded by the current, reveal it
	if (bt200_displine_level <= level) {
		bt200_displine_showlevel (level);
		bt200_displine_level = level;
	}
}


/* Stop displaying content at the specified level.  In some cases, older
 * content may now pop up, in others the display could get cleared.
 */
void bottom_show_close_level (app_level_t level) {
	if (bt200_period_level == level) {
		bt200_period_level = APP_LEVEL_ZERO;
		ht162x_led_set (0, false, false);
		ht162x_led_set (1, false, false);
		ht162x_dispdata [0] = 0x00;
		ht162x_dispdata [1] = 0x00;
		ht162x_dispdata [2] = 0x00;
		ht162x_dispdata_notify (0, 2);
	}
	if (bt200_displine_level == level) {
		bt200_level_active [level] = false;
		memset (bt200_level_maintext [level], ' ', 12);
		while ((bt200_displine_level > APP_LEVEL_ZERO)
				&& !bt200_level_active [bt200_displine_level]) {
			bt200_displine_level--;
		}
		bt200_displine_showlevel (bt200_displine_level);
	}
}

/* Print an IPv4 address on the display */
void bottom_show_ip4 (app_level_t level, uint8_t bytes [4]) {
	uint8_t idx;
	char ip [12];
	char *ipptr = ip;
	for (idx = 0; idx < 4; idx++) {
		*ipptr++ = '0' +  (bytes [idx] / 100);
		*ipptr++ = '0' + ((bytes [idx] % 100) / 10);
		*ipptr++ = '0' +  (bytes [idx] % 10);
	}
	bt200_display_showtxt (level, ip, 0x07);
}

/* Print an IPv6 address, as far as this is possible, on the display
 *  - Remove the prefix /64 as it is widely known
 *  -TODO- If middle word is 0xfffe, remove it, display the rest, set dot #2
 *  -TODO- If first word is 0x0000, remove it, display the rest, set dot #1
 *  -TODO- If last word is 0x0001, remove it, display the rest, set dot #3
 *  - If nothing else is possible, use 6 bits per digit, dots and last digit blanc
void bottom_show_ip6 (app_level_t level, uint16_t words [8]) {
	// TODO: Extra cases; for now, just dump 6 bits per 7-segment display
	uint8_t idxi;
	uint8_t idxo;
	uint8_t shfi;
	uint8_t shfo;
	ht162x_led_set (12, 0, false);
	ht162x_led_set ( 9, 0, false);
	ht162x_led_set ( 6, 0, false);
	for (idxo = 14; idxo >= 3; idxo--) {
		ht162x_dispdata [idxo] &= 0x10;
	}
	shfi = 6;
	shfo = 4;
	idxi = 4;
	idxo = 14;
	while (idxi < 8) {
		uint8_t twobits = (words [idxi] >> shfi) & 0x03;
		twobits << shfo;
		if (twobits & 0x10) {
			twobits += 0x80 - 0x10;	// bit 7 replaces bit 4
		}
		ht162x_dispdata [idxo] |= (twobits << shfo);
		if (shfi > 0) {
			shfi -= 2;
		} else {
			shfi = 6;
			idxi++;
		}
		if (shfo > 0) {
			shfo -= 2;
		} else {
			shfo = 5;
			idxo--;
		}
	}
	ht162x_dispdata_notify (14, 3);
}

/* A list of fixed messages, matching the fixed_msg_t values */

static char *bt200_fixed_messages [FIXMSG_COUNT] = {
	"offline",
	"registering",
	"ready",
	"bootload",
	"sending call",
	"ringing",
	"call ended",
};

/* Print a fixed message on the main line of the display */
void bottom_show_fixed_msg (app_level_t level, fixed_msg_t msg) {
	if (msg < FIXMSG_COUNT) {
		bt200_display_showtxt (level, bt200_fixed_messages [msg], 0x00);
	}
}

/* Print a notification of the number of new / old voicemails */
void bottom_show_voicemail (app_level_t level, uint16_t new, uint16_t old) {
	char msg [12];
	if (new > 999) {
		new = 999;
	}
	memcpy (msg, "xxx messages", 12);
	msg [0] =  new / 100;
	msg [1] = (new % 100) / 10;
	msg [2] =  new        % 10;
	bt200_display_showtxt (level, msg, 0x00);
}

/******** BOTTOM LEVEL MAIN PROGRAM ********/

/* Setup the connectivity of the TIC55x as used on Grandstream BT20x */
void main (void) {
	uint16_t idx;
	led_colour_t led = LED_STABLE_ON;
	//
	// PLL setup: Crystal is 16.384 MHz, increase that 15x
	// 1. Switch to bypass mode by setting the PLLEN bit to 0.
	PLLCSR &= ~REGVAL_PLLCSR_PLLEN;
	// 2. Set the PLL to its reset state by setting the PLLRST bit to 1.
	PLLCSR |= REGVAL_PLLCSR_PLLRST;
	// 3. Change the PLL setting through the PLLM and PLLDIV0 bits.
	PLLM = REGVAL_PLLM_TIMES_15;
	PLLDIV0 = REGVAL_PLLDIVx_DxEN | REGVAL_PLLDIVx_PLLDIVx_1;
	// 4. Wait for 1 µs.
	{ int ctr = 1000; while (ctr--) ; }
	// 5. Release the PLL from its reset state by setting PLLRST to 0.
	PLLCSR &= ~REGVAL_PLLCSR_PLLRST;
	// 6. Wait for the PLL to relock by polling the LOCK bit or by setting up a LOCK interrupt.
	while ((PLLCSR & REGVAL_PLLCSR_LOCK) == 0) {
		/* wait */ ;
	}
	// 7. Switch back to PLL mode by setting the PLLEN bit to 1.
	PLLCSR |= REGVAL_PLLCSR_PLLEN;
	PLLDIV1 = REGVAL_PLLDIVx_DxEN | REGVAL_PLLDIVx_PLLDIVx_2;
	PLLDIV2 = REGVAL_PLLDIVx_DxEN | REGVAL_PLLDIVx_PLLDIVx_4;
	PLLDIV3 = REGVAL_PLLDIVx_DxEN | REGVAL_PLLDIVx_PLLDIVx_4;
	//TODO// PLLDIV3 = REGVAL_PLLDIVx_DxEN | REGVAL_PLLDIVx_PLLDIVx_2;
	//TODO// PLLCSR |= REGVAL_PLLCSR_PLLEN;
	// Now we have:
	//	CPU clock is 245.76 MHz
	//	SYSCLK1   is 122.88 MHz
	//	SYSCLK2   is  61.44 MHz
	//
	// EMIF settings, see SPRU621F, section 2.12 on "EMIF Registers"
	//
	EGCR1 = 0xff7f;
	// EGCR2 = 0x0009; // ECLKOUT2-DIV-4
// EGCR2 = 0x0001;	//ECLKOUT2-DIV-1//
EGCR2 = 0x0005;	//ECLKOUT2-DIV-2//
	// EGCR1 = ...;   // (defaults)
	// EGCR2 = ...;   // (defaults)
	// CESCR1 = ...;   // (defaults)
	// CESCR2 = ...;   // (defaults)
	//
	// CE0 selects the network interface
	// CE0_1 = 0xff03;   // DEFAULT 8-bit async (and defaults)
	// Fail: CE0_1 = 0xc112;   // 16-bit async, rd setup 2, rd strobe 1, rd hold 2
	// Fail: CE0_2 = 0x20a2;	  // wr setup 2, wr strobe 2, wr hold 2, rd setup 2
	CE0_1 = 0x3f2f;
	CE0_2 = 0xffff;
	// CE0_2 = ...;   // (defaults)
	// CE0_SC1 = ...;   // (defaults)
	// CE0_SC2 = ...;   // (defaults)
	//
	// CE1 selects the flash chip
	//WORKED?// CE1_1 = 0xff13;   // 16-bit async (and defaults)
	CE1_1 = 0x0922;
	CE1_2 = 0x31f2;
	// CE1_2 = ...;   // (defaults)
	// CE1_SC1 = ...;   // (defaults)
	// CE1_SC2 = ...;   // (defaults)
	//
	// CE2 selects the SDRAM chips
	//WORKS?// CE2_1 = 0xff33;   // 32-bit SDRAM (and defaults)
	CE2_1 = 0xff37;
	//TEST// CE2_1 = 0xff13;   // 16-bit async (and defaults)
	// CE2_2 = ...;   // (defaults)
	// CE2_SC1 = ...;   // (defaults)
	// CE2_SC2 = ...;   // (defaults)
	// Possible: SDC1, SDC2, SDRC1, SDRC2, SDX1, SDX2
	SDC1 = 0x6ffe;
	SDC2 = 0x8712;
	SDRC1 = 0xf5dc;
	SDRC2 = 0xffff;
	SDX1 = 0xb809;
	CESCR1 = 0xfffc;
	//
	// CE3 selects the D-flipflops for keyboard and LCD
	CE3_1 = 0xff13;   // 16-bit async (and defaults)
	//BT200ORIG// CE3_1 = 0x0220;
	//BT200ORIG// CE3_2 = 0x0270;
	// CE3_2 = ...;   // (defaults)
	// CE3_SC1 = ...;   // (defaults)
	// CE3_SC2 = ...;   // (defaults)
	//
	// Setup output ports; reset TLV chip; reset message levels
{ uint32_t ctr = 10000; while (ctr-- > 0) ; }
	IODIR  |= (1 << 7) | (1 << 1);
	IODATA  =            (1 << 1);	// Updated below small delay
	{ uint32_t ctr; for (ctr=0; ctr < 7 * (600 / 12); ctr++) /* Wait 7x MCLK */ ; }
	IODATA |= (1 << 7) | (1 << 1);  // See above small delay
	{ uint32_t ctr; for (ctr=0; ctr < 132 * (600 / 12); ctr++) /* Wait at least 132 MCLK cycles */ ; }
	//
	// Setup McBSP1 for linking to TLV320AIC20K
	// Generate a CLKG at 12.288 MHz, and FS at 8 kHz
	// following the procedure of spru592e section 3.5
	//
	DXR1_1 = 0x0000;
	SPCR1_1 = 0x0000;	// Disable/reset receiver, required in spru592e
	SPCR2_1 = 0x0000;	// Disable/reset sample rate generator
	SRGR1_1 = REGVAL_SRGR1_FWID_1 | REGVAL_SRGR1_CLKGDIV_4;
	SRGR2_1 = REGVAL_SRGR2_CLKSM | REGVAL_SRGR2_FSGM | REGVAL_SRGR2_FPER_1535;
	//COPIED_BELOW// PCR1 = /*TODO: (1 << REGBIT_PCR_IDLEEN) | */ (1 << REGBIT_PCR_FSXM) /* | (1 << REGBIT_PCR_FSRM) */ | (1 << REGBIT_PCR_CLKXM) /* | (1 << REGBIT_PCR_CLKRM) */ | (1 << REGBIT_PCR_CLKXP) | (1 << REGBIT_PCR_CLKRP);
	PCR1 = (1 << REGBIT_PCR_FSXM) | (1 << REGBIT_PCR_CLKXM);
{ uint32_t ctr = 10000; while (ctr-- > 0) ; }
// SPCR2_1 |= REGVAL_SPCR2_GRST_NOTRESET | REGVAL_SPCR2_FRST_NOTRESET;
	RCR1_1 = (0 << 8) | (2 << 5);	// Read  1 frame of 16 bits per FS
	XCR1_1 = (0 << 8) | (2 << 5);	// Write 1 frame of 16 bits per FS
	RCR2_1 = 0x0001;		// Read  with 1 clockcycle delay
	XCR2_1 = 0x0001;		// Write with 1 clockcycle delay
	//TODO:NOT-SPI-BUT-CONTINUOUS-CLOCK// SPCR1_1 |= REGVAL_SPCR1_CLKSTP_NODELAY;
	//
	// Setup I2C for communication with the TLV320AIC20K codec
	// Prescale SYSCLK2 down from 61.44 MHz to 10.24 MHz so it falls
	// in the required 7 Mhz to 12 MHz range; support a 100 kHz I2C bus
	// by setting low/high period to 51 such periods.
	// Note: The only peripheral TLV320AIC20K could go up to 900 kHz
	I2CMDR = REGVAL_I2CMDR_MST | /* reset to set PSC */  REGVAL_I2CMDR_FREE | REGVAL_I2CMDR_BC_8;
	I2CPSC = 5;
	I2CCLKH = 51 - 5;	/* TODO: 900 kHz is possible on TLV320AIC20K */
	I2CCLKL = 51 - 5;
	I2COAR = REGVAL_I2COAR;
	I2CMDR = REGVAL_I2CMDR_MST | REGVAL_I2CMDR_NORESET | REGVAL_I2CMDR_FREE | REGVAL_I2CMDR_BC_8;
	//
	// Setup DMA channel 0 for playing from DSP to TLV320AIC2x
	// Setup DMA channel 1 for recording from TLV320AIC2x to DSP
	//
	// Both channels have a block (their entire RAM buffer) comprising
	// of 25 frames, which each are 64 samples of 16 bits in size.
	// At 8 kHz sample rate, frames cause 125 interrupts per second;
	// at 48 kHz this rises to 750 (per channel), still comfortable.
	// The total buffer is 1600 samples of 16 bits long.  Each time a
	// frame send finishes, an interrupt checks if there is another
	// frame of 64 samples ready to go; if not, it will disable the
	// DMA channel.  Hint routines serve to restart DMA after that.
	// Conversely, the DMA interrupt handlers can make top-calls to
	// indicate that data is ready for reading or that space is
	// available for writing.
	// The settings below prepare DMA for continuous playing and
	// recording, but with the setup disabled until hinted.
	//
	DMAGCR = REGVAL_DMAGCR_FREE;
	DMAGTCR = 0x00;		// No timeout support
	DMACCR_0 = REGVAL_DMACCR_SRCAMODE_CONST | REGVAL_DMACCR_DSTAMODE_POSTINC | REGVAL_DMACCR_PRIO | REGVAL_DMACCR_SYNC_MCBSP1_REV | REGVAL_DMACCR_REPEAT | REGVAL_DMACCR_AUTOINIT;
	DMACCR_1 = REGVAL_DMACCR_SRCAMODE_POSTINC | REGVAL_DMACCR_DSTAMODE_CONST | REGVAL_DMACCR_PRIO | REGVAL_DMACCR_SYNC_MCBSP1_TEV | REGVAL_DMACCR_REPEAT | REGVAL_DMACCR_AUTOINIT;
	DMACICR_0 = REGVAL_DMACICR_FRAMEIE;
	DMACICR_1 = REGVAL_DMACICR_FRAMEIE;
	//TODO// DMACSDP_0 = REGVAL_DMACSDP_SRC_PERIPH | REGVAL_DMACSDP_DST_DARAM1 | REGVAL_DMACSDP_DATATYPE_16BIT;
	//TODO// DMACSDP_1 = REGVAL_DMACSDP_SRC_DARAM0 | REGVAL_DMACSDP_DST_PERIPH | REGVAL_DMACSDP_DATATYPE_16BIT;
	DMACSDP_0 = REGVAL_DMACSDP_SRC_PERIPH | REGVAL_DMACSDP_DST_EMIF | REGVAL_DMACSDP_DATATYPE_16BIT;
	DMACSDP_1 = REGVAL_DMACSDP_SRC_EMIF | REGVAL_DMACSDP_DST_PERIPH | REGVAL_DMACSDP_DATATYPE_16BIT;
	DMACSSAL_0 = ((intptr_t) &DRR1_1) <<  1;
	DMACSSAU_0 = ((intptr_t) &DRR1_1) >> 15;
	DMACDSAL_0 = (uint16_t) (((intptr_t) samplebuf_record) <<  1);
	DMACDSAU_0 = (uint16_t) (((intptr_t) samplebuf_record) >> 15);
	DMACSSAL_1 = (uint16_t) (((intptr_t) samplebuf_play)   <<  1);
	DMACSSAU_1 = (uint16_t) (((intptr_t) samplebuf_play)   >> 15);
	DMACDSAL_1 = ((intptr_t) &DXR1_1) <<  1;
	DMACDSAU_1 = ((intptr_t) &DXR1_1) >> 15;
#if TRY_SOMETHING_ELSE_TO_GET_INTERRUPTS
	DMACEN_0 = 64;           /* 64 elements (samples) per frame (continue-checks) */
	DMACEN_1 = 64;
	DMACFN_0 = (BUFSZ / 64); /* 25 frames (continue-checks) per block (buffer) */
	DMACFN_1 = (BUFSZ / 64);
#else
	DMACEN_0 = 64;
	DMACEN_1 = 64;
	DMACFN_0 = 1;
	DMACFN_1 = 1;
#endif
	/* TODO? */
	//
	// Further initiation follows
	//
	asm (" bclr xf");  // Switch off MESSAGE LED
{ uint16_t ctr = 250; while (ctr > 0) { ctr--; } }	
	bottom_critical_region_begin (); // _disable_interrupts ();
	IER0 = IER1 = 0x0000;
	tic55x_setup_timers ();
	tic55x_setup_interrupts ();
	ht162x_setup_lcd ();
	tlv320aic2x_set_samplerate (0, 8000);
	tlv320aic2x_setup_sound ();
	ksz8842_setup_network ();
	// Enable INT0..INT3
	//TODO:TEST// IER0 |= 0x0a0c;
	//TODO:TEST// IER1 |= 0x0005;
	IER0 |= (1 << REGBIT_IER0_DMAC1) | (1 << REGBIT_IER0_INT0) | (1 << REGBIT_IER0_TINT0); // 0x0214;
	IER1 |= (1 << REGBIT_IER1_DMAC0); // 0x0004;
	PCR0 = (1 << REGBIT_PCR_XIOEN) | (1 << REGBIT_PCR_RIOEN);

	for (idx = 0; idx < APP_LEVEL_COUNT; idx++) {
		bt200_level_active [idx] = false;
	}
#if 0
{uint8_t idx, dig, bar;
idx=0; dig=0; bar=0;
memset (ht162x_dispdata, 0x00, sizeof (ht162x_dispdata));
while (true) {
uint32_t ctr;
ht162x_putchar (14 - idx, (dig < 10)? (dig + '0'): (dig + 'A' - 11));
ht162x_audiolevel ((bar < 8)? (bar << 5): ((14-bar) << 5));
// bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_OFF);
ht162x_dispdata_notify (14 - idx, 14 - idx);
ht162x_dispdata_notify (15, 15);
// bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_ON );
idx = (idx + 1) % 12;
dig = (dig + 1) % 38;
bar = (bar + 1) % 14;
ctr = 650000; while (ctr>0) ctr--;
}
}
#endif

	//TODO// IER0 = 0xdefc;
	//TODO// IER1 = 0x00ff;
/* TODO: Iterate over flash partitions that can boot, running each in turn: */
	top_main ();
}

