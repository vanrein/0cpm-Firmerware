/* Micrel KSZ8842-16 drivers.  Probably suitable for -32 as well.
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


/* This driver uses a few low-level routines that should be part
 * of the architecture definition of a particular phone:
 *  - kszmap16() maps 16-bit values between CPU and KSZ endianity
 *  - kszmap32() maps 32-bit values between CPU and KSZ endianity
 *  - kszreg16(N) is an l-value for 16-bit register N
 * The registers of this chip are defined in terms of kszreg16()
 *
 * The kszmap functions are their own inverse functions.  They are
 * not applied automatically to the registers; in fact, it is more
 * efficient to apply them to what is applied to the registers, as
 * the mappings may be efficiently removed.  For instance, in
 *
 * if (KSZ8842_BANK18_ISR & kszmap16(REGVAL_KSZ8842_ISR_LCIS)) ...
 *
 * the kszmap16 would not cost anything, as it can be applied at
 * compile time to the REGVAL_KSZ8842_ISR_LCIS constant.  Variables
 * that contain values mapped like this would be of type kszint16_t
 * or kszint32_t; the same as uint16_t and uint32_t, respectively.
 *
 * Note that KSZ8842_BANK_SET (N) uses a "normal" uint16_t N.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdint.h>
#include <stdbool.h>

#define BOTTOM

#include <config.h>
#include <bottom/ksz8842.h>


/* What the software knows about the online state of the chip */
static bool online_status = false;


/* KSZ8842 interrupt handler, _but_ compiled as a plain function.
 * Retrieve the ISR register and handle each interrupt reason.
 */
void ksz8842_interrupt_handler (void) {
	kszint16_t curbank;
	kszint16_t ifr;

	//
	// Remember the bank that was active before the interrupt
	curbank = KSZ8842_BANKx_BSR;

	//
	// Get hold of the interrupt flags, and clear them
	KSZ8842_BANK_SET (18);
	ifr = KSZ8842_BANK18_ISR;
	KSZ8842_BANK18_ISR = ifr;

	//
	// See if the link status has changed
	if (ifr & kszmap16 (REGVAL_KSZ8842_ISR_LCIS)) {
		// Link change
		bool either;
		kszint16_t pxmbsr;
		KSZ8842_BANK_SET (45);
		pxmbsr = KSZ8842_BANK45_P1MBSR;
		KSZ8842_BANK_SET (46);
		pxmbsr |= KSZ8842_BANK46_P2MBSR;
		either = (pxmbsr & kszmap16 (REGVAL_KSZ8842_PxMBSR_ONLINE)) != 0;
		if (either != online_status) {
			online_status = either;
			if (online_status) {
				top_network_online ();
			} else {
				top_network_offline ();
			}
		}
	}

	//
	// See if more frames can be sent or received
	if (ifr & kszmap16 (REGVAL_KSZ8842_ISR_TXIS)) {
		//TODO// top_network_can_send ();
	}
	if (ifr & kszmap16 (REGVAL_KSZ8842_ISR_RXIS)) {
		top_network_can_recv ();
	}

	//
	// Ignore a few error conditions after resetting their interrupt flag
	// kszmap16 (REGVAL_KSZ8842_ISR_RXOIS): RX overrun status
	// kszmap16 (REGVAL_KSZ8842_ISR_TXPSIE): TX stopped
	// kszmap16 (REGVAL_KSZ8842_ISR_RXPSIE): RX stopped
	// kszmap16 (REGVAL_KSZ8842_ISR_RXEFIE): RX frame error

	//
	// Recover the bank that was active before the interrupt
	KSZ8842_BANKx_BSR = curbank;
}


/* Try to send a network packet */
bool bottom_network_send (uint8_t *pkt, uint16_t pktlen) {
	uint16_t txmem;
	uint16_t pktidx;
	KSZ8842_BANK_SET (16);
	txmem = kszmap16 (KSZ8842_BANK16_TXMIR);
	if (txmem < (pktlen + 4)) {
		return false;
	}
	KSZ8842_BANK_SET (17);
	//
	// Hold while previous send data is being queued
#ifdef 0
	while (KSZ8842_BANK17_TXQCR & kszmap16 (REGVAL_KSZ8842_TXQCR_TXETF)) {
		/* Yes, this is busy waiting.  It should not take long.
		 * Usually, all work will have been done when we get back here.
		 * If not, returning false could cause the network to block.
		 * So it is better to do busy waiting for a short while.
		 */
		;
	}
#else
	if (KSZ8842_BANK17_TXQCR & kszmap16 (REGVAL_KSZ8842_TXQCR_TXETF)) {
		return false;
	}
#endif
	//
	// First send packet metadata: control, bytecount
	// TODO: It is possible to set a control-ID in QDRL, see Table 4
	KSZ8842_BANK17_QDRL = kszmap16 (CTLVAL_KSZ8842_TXIC);
	KSZ8842_BANK17_QDRH = kszmap16 (pktlen);
	//
	// Now write out the data itself
	pktidx = 0;
	while (pktidx < pktlen) {
		KSZ8842_BANK17_QDRL = ksz_memget16 (pkt + pktidx + 0);
		KSZ8842_BANK17_QDRH = ksz_memget16 (pkt + pktidx + 2);
		pktidx += 4;
	}
	//
	// Indicate that the data has all been written
	KSZ8842_BANK17_TXQCR |= kszmap16 (REGVAL_KSZ8842_TXQCR_TXETF);
	return true;
}


/* Try to receive a network packet */
bool bottom_network_recv (uint8_t *pkt, uint16_t *pktlen) {
	bool ok = true;
	uint16_t rxlen;
	uint16_t rxdata;
	//
	// Hold until previous recv data has been skipped
	KSZ8842_BANK_SET (17);
	while (KSZ8842_BANK17_RXQCR & kszmap16 (REGVAL_KSZ8842_RXQCR_RXRRF)) {
		/* Yes, this is busy waiting.  It should not take long.
		 * Usually, all work will have been done when we get back here.
		 * If not, returning false could cause the network to block.
		 * So it is better to do busy waiting for a short while.
		 */
		;
	}
	//
	// Is there a frame ready to be picked up?
	// Note that only correct frames are welcomed.
	KSZ8842_BANK_SET (18);
	if ((KSZ8842_BANK18_RXSR & kszmap16 (REGVAL_KSZ8842_RXSR_RXFV)) == 0) {
		return false;
	}
	//
	// Check if the frame length does not exceed the available buffer
	rxlen = (kszmap16 (KSZ8842_BANK18_RXBC) & 0x07ff);
	ok = ok && (rxlen >= 6 + 6 + 2);
	ok = ok && (rxlen <= *pktlen);
	KSZ8842_BANK_SET (17);
	if (ok) {
		uint16_t pktidx = 0;
		*pktlen = rxlen;
		//
		// Receive the header info (just skip it)
		rxdata = KSZ8842_BANK17_QDRL;	// Drop; already got RXSR
		rxdata = KSZ8842_BANK17_QDRH;	// Drop; already got rxlen
		//
		// Receive regular data in 4-byte blocks (possible overshoot up to 3)
		while (pktidx < rxlen) {
			rxdata = KSZ8842_BANK17_QDRL;
			ksz_memset16 (rxdata, pkt + pktidx + 0);
			rxdata = KSZ8842_BANK17_QDRH;
			ksz_memset16 (rxdata, pkt + pktidx + 2);
			pktidx += 4;
		}
	}
	//
	// Indicate that the data has all been read
	KSZ8842_BANK17_RXQCR |= kszmap16 (REGVAL_KSZ8842_RXQCR_RXRRF);
	//
	// Return whether a good packet was actually received
	return ok;
}


/* Setup the KSZ8842 chip */
void ksz8842_setup_network (void) {
	KSZ8842_BANK_SET (2);
	// TODO: Where is the MAC address stored in Flash?
	KSZ8842_BANK2_MARH = kszmap16 (0x000b);		// 00:0b:__:__:__:__
	KSZ8842_BANK2_MARM = kszmap16 (0x8219);		// __:__:82:19:__:__
	KSZ8842_BANK2_MARL = kszmap16 (0xa0f4);		// __:__:__:__:a0:f4
	KSZ8842_BANK_SET (17);
	KSZ8842_BANK17_TXFDPR = kszmap16 (0x4000);	// auto-inc TXQ ptr at 0
	KSZ8842_BANK17_RXFDPR = kszmap16 (0x4000);	// auto-inc RXQ ptr at 0
	KSZ8842_BANK_SET (32);
	KSZ8842_BANK32_SIDER |= kszmap16 (0x0001);	// Switch enable
	KSZ8842_BANK_SET (16);
	KSZ8842_BANK16_TXCR =
		kszmap16 (REGVAL_KSZ8842_TXCR_TXPE) |
		kszmap16 (REGVAL_KSZ8842_TXCR_TXCE) |
		kszmap16 (REGVAL_KSZ8842_TXCR_TXE);
	KSZ8842_BANK16_RXCR =
		kszmap16 (REGVAL_KSZ8842_RXCR_RXFCE) |
		kszmap16 (REGVAL_KSZ8842_RXCR_RXBE) |
		kszmap16 (REGVAL_KSZ8842_RXCR_RXME) |
		kszmap16 (REGVAL_KSZ8842_RXCR_RXUE) |
		kszmap16 (REGVAL_KSZ8842_RXCR_RXSCE) |
		kszmap16 (REGVAL_KSZ8842_RXCR_RXE);
	KSZ8842_BANK_SET (18);
	KSZ8842_BANK18_IER |=
		kszmap16 (REGVAL_KSZ8842_ISR_LCIS) |
		kszmap16 (REGVAL_KSZ8842_ISR_TXIS) |
		kszmap16 (REGVAL_KSZ8842_ISR_RXIS);
	KSZ8842_BANK_SET (0);	// TODO: needless
}

