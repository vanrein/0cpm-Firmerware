/* ht162x.c -- driver for Holtek 162x chips
 *
 * This is a general driver for Holtek 162x LCD driver chips.
 * It is designed to output bits serially, using a few low-level
 * routines elsewhere that depend on how the hardware is wired.
 * 
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>


#define BOTTOM
#include <config.h>

#include <bottom/ht162x.h>


/* The display data is stored as a series of bytes.  This is
 * not as general as the actual displays, which work per nibble.
 * Since it is more common for computers to work with bytes, and
 * since this is certainly useful for several applications (think
 * of 7-segment displays, or a row of bits in a character) the
 * grainsize of a byte seems easier to do.
 */
uint8_t ht162x_dispdata [HAVE_HT162x_DISPBYTES];

/* The _minchg and _maxchg addresses mark an area that has
 * changed, and should be updated on the next opportunity.
 */
uint8_t ht162x_dispdata_minchg = HAVE_HT162x_DISPBYTES;
uint8_t ht162x_dispdata_maxchg = 0;

/* The _sendstate details the progress of clocking out data */
enum ht162x_sendstate {
	SS_DISABLED,	// No sending in progress
	SS_CMDBITS,	// Shifting the bits of a command
	SS_WRPREFIX,	// Shifting write prefix bits
	SS_ADDRESS,	// Shifting address bits
	SS_DATA,	// Shifting the data word
} sendstate;


static uint16_t wr_data = 0x8000;
static uint8_t wr_addr_cur = 1, wr_addr_max = 0;
static bool wr_state_high = true;
static bool wr_active = false;
static uint16_t *wr_cmdptr;


/* Start the process of shifting bits to the HT162x */
static void start_shifting (void) {
	void ht162x_bitshifter (void);
	// TODO: Start up the process asynchronously!
	wr_active = true;
	sendstate = SS_DISABLED;
	do {
		uint16_t ctr = 250;
		ht162x_bitshifter ();
		while (ctr > 0) { ctr--; }
	} while (wr_active);
}


/* Stop the process of shifting bits to the HT162x */
static void stop_shifting (void) {
	// TODO: Stop the asynchronous process
	wr_active = false;
}

/* Take another step in the process of shifting data to the ht162x */
void ht162x_bitshifter (void) {
	bool done = false;
	// Is the second half of a bit clock-in?
	if (!wr_state_high) {
		ht162x_databit_commit ();
		wr_state_high = true;
		return;
	}
	// Is this the end of a portion to be sent?
	if (wr_data == 0x8000) {
		switch (sendstate) {
		case SS_DISABLED:
			// This is the start of a new block of LCD writes
			if (wr_cmdptr) {
				sendstate = SS_CMDBITS;
			} else if (ht162x_dispdata_minchg <= ht162x_dispdata_maxchg) {
				wr_addr_cur = ht162x_dispdata_minchg;
				wr_addr_max = ht162x_dispdata_maxchg;
				ht162x_dispdata_minchg = HAVE_HT162x_DISPBYTES;
				ht162x_dispdata_maxchg = 0;
				wr_data = HT162x_LCDPREFIX_WRITE;
				sendstate = SS_WRPREFIX;
			} else {
				stop_shifting ();
				return;
			}
			wr_state_high = true;
			ht162x_databit_prepare (0); // Ensure high WR
			ht162x_chipselect (true);
			return;
		case SS_WRPREFIX:
			// Done sending the write prefix -> send address
			wr_data = HT162x_LCD_ADDRESS6 (wr_addr_cur << 1);
			sendstate = SS_ADDRESS;
			break;
		case SS_ADDRESS:
			// Done sending the address -> send data bytes
			sendstate = SS_DATA;
			// Continue into SS_DATA...
		case SS_DATA:
			if (wr_addr_cur > wr_addr_max) {
				done = true;
				break;
			}
			wr_data = HT162x_LCD_2NIBBLES (ht162x_dispdata [wr_addr_cur++]);
			break;
		case SS_CMDBITS:
			// Done sending CMDBITS -> increment cmdptr
			if (*wr_cmdptr == HT162x_LCDCMD_DONE) {
				wr_cmdptr = NULL;
				done = true;
				break;
			}
			wr_data = *wr_cmdptr++;
			break;
		}
	}
	// Is there nothing more to send in the CS period?
	if (done) {
		sendstate = SS_DISABLED;
		ht162x_chipselect (false);
		return;	// Next invocation may terminate sending
	}
	// Send out the first phase of the next bit
	ht162x_databit_prepare (wr_data >> 15);
	wr_data <<= 1;
	wr_state_high = false;
}


/* Notify the HT162x driver of an update to a ht162x_dispdata range */
void ht162x_dispdata_notify (uint8_t minaddr, uint8_t maxaddr) {
	if (minaddr < ht162x_dispdata_minchg) {
		ht162x_dispdata_minchg = minaddr;
	}
	if (maxaddr > ht162x_dispdata_maxchg) {
		ht162x_dispdata_maxchg = maxaddr;
	}
	if (!wr_active) {
		start_shifting ();
	}
}


/* Setup the HT162x by sending it the configuration commands */
void ht162x_setup_lcd (void) {
	ht162x_chipselect (false);  // Reset any current access
	wr_cmdptr = ht162x_setup_cmdseq;
	memset (ht162x_dispdata, 0x00, sizeof (ht162x_dispdata));
	ht162x_dispdata_notify (0, HAVE_HT162x_DISPBYTES-1);
	start_shifting ();
}

