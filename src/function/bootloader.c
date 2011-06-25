/* bootloader.c -- Access to flash partitions
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


/* This module can be linked in with various targets, to give
 * them access to the flash memory, and support updates.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

#include <config.h>

#include <0cpm/cons.h>
#include <0cpm/flash.h>


/* Sanity check */

#ifndef HAVE_FLASH_PARTITIONS
#error "This target does not define a flash partition table yet"
#endif


/* Global data */

static struct flashpart *current = NULL;
static uint16_t blocknum;
static bool sending = false, receiving = false;


/* Handle an incoming TFTP request (over LLC)
 *
 * This code is rather brutal -- it only does binary transfers,
 * and it supports no options at all.
 */

void bootloader_datagram (uint8_t *pkt, uint16_t pktlen) {
	void netreply_llc1 (uint8_t *pkt, uint16_t pktlen);
	uint16_t cmd;
	uint16_t pktlen2;
	if (pktlen < 12 + 2 + 3 + 4) {
		return;
	}
	pktlen2 = (pkt [12] << 8) | pkt [13];
	if (pktlen < pktlen2 + 12 + 2) {
		return;
	}
	cmd = (pkt [12 + 2 + 3 + 0] << 8) | pkt [12 + 2 + 3 + 1];
	if ((cmd == 1) || (cmd == 2)) {		/* 1==RRQ, 2=WRQ, new setup */
		pkt [pktlen - 1] = 0;
		bottom_printf ("TFTP %s for %s\n",
			(cmd == 1)? "RRQ": "WRQ",
			pkt + 12 + 2 + 3 + 2);
		current = (struct flashpart *) "TODO"; //TODO// current=flash_find_file(...), return if not found
		sending   = (current != NULL) && (cmd == 1);
		receiving = (current != NULL) && (cmd == 2);
		pktlen = 12 + 2 + 3;
		blocknum = 0;
		if (sending) {
			goto sendblock;
		}
		if (receiving) {
			pkt [pktlen++] = 0x00;	// send ACK
			pkt [pktlen++] = 0x04;
			pkt [pktlen++] = 0x00;
			pkt [pktlen++] = 0x00;
		}
		netreply_llc1 (pkt, pktlen);
	} else if (cmd == 3) {			/* 3==DATA, store or burn */
		if (!current) {
			bottom_printf ("TFTP DATA received without connection\n");
		} else if (!receiving) {
			bottom_printf ("TFTP DATA received while not receiving\n");
		} else if ((pktlen2 != 3 + 4 + 512) && (pktlen2 != 3 + 4 + 0)) {
			bottom_printf ("TFTP DATA is not a 512-byte block\n");
		} else if (((pkt [12 + 2 + 3 + 2] << 8) | pkt [12 + 2 + 3 + 3]) != (blocknum + 1)) {
			bottom_printf ("TFTP DATA numbering out of sequence\n");
		} else {
			blocknum++;
			/* Ignore, do not actually store the block anywhere */
			pkt [12 + 2 + 3 + 0] = 0x00;	// send ACK
			pkt [12 + 2 + 3 + 1] = 0x04;
			pkt [12 + 2 + 3 + 2] = blocknum >> 8;
			pkt [12 + 2 + 3 + 3] = blocknum & 0xff;
			netreply_llc1 (pkt, 12 + 2 + 3 + 4);
		}
	} else if (cmd == 4) {			/* 4==ACK, send next if sending  */
		if (!current) {
			bottom_printf ("TFTP ACK received without connection\n");
		} else if (!sending) {
			bottom_printf ("TFTP ACK received while not sending\n");
#if 0
		} else if (pktlen != 12 + 2 + 3 + 4) {
			bottom_printf ("TFTP ACK is not of the proper legnth\n");
#endif
		} else if (((pkt [12 + 2 + 3 + 2] << 8) | pkt [12 + 2 + 3 + 3]) != blocknum) {
			bottom_printf ("TFTP ACK numbering out of sequence\n");
		} else {
sendblock:
			/* If more data is available, send it */
			blocknum++;
			pkt [12 + 2 + 3 + 0] = 0x00;	// send DATA
			pkt [12 + 2 + 3 + 1] = 0x03;
			pkt [12 + 2 + 3 + 2] = blocknum >> 8;
			pkt [12 + 2 + 3 + 3] = blocknum & 0xff;
			if (bottom_flash_read (blocknum - 1, pkt + 12 + 2 + 3 + 4)) {
				netreply_llc1 (pkt, 12 + 2 + 3 + 4 + 512);
			} else {
				netreply_llc1 (pkt, 12 + 2 + 3 + 4 + 0  );
			}
		}
	} else if (cmd == 5) {			/* 5==ERR */
		pkt [pktlen-1] = 0;
		bottom_printf ("TFTP error %d received: %s\n", 
			(pkt [12 + 2 + 3 + 2] << 8) | pkt [12 + 2 + 3 + 3],
			pkt + 12 + 2 + 3 + 4);
		/* no further error handling */
	}
	
}



/********** END OF LIBRARY FUNCTIONS ***** START OF MAIN PROGRAM **********/



#ifdef CONFIG_MAINFUNCTION_BOOTLOADER

/* Bootloader main program.
 *
 * Only included if the bootloader is compiled as the main target.
 * If not, the bootloader is an optional add-on for the target.
 */

uint8_t pkt [700];
void top_main (void) {
	void nethandler_llconly (uint8_t *pkt, uint16_t pktlen);
	bool active = false;
	uint16_t pktlen;
	bottom_critical_region_end ();
	while (bottom_phone_is_offhook ()) {
		if (!active) {
			bottom_printf ("TFTP bootloader starts on SAP 68\n");
			active = true;
		}
		pktlen = sizeof (pkt);
		if (bottom_network_recv (pkt, &pktlen)) {
			nethandler_llconly (pkt, pktlen);
		}
	}
	if (active) {
		bottom_printf ("TFTP bootloader ends\n");
		//TODO// Flush and close logging?
	} else {
		bottom_printf ("TFTP bootloader skipped -- reboot offhook to activate\n");
	}
}

/* If this is the main program, various other functions are
 * needed to keep the linker happy.
 */
void top_button_press (void) { ; }
void top_button_release (void) { ; }
void top_hook_update (void) { ; }
void top_network_can_recv (void) { ; }
void top_network_can_send (void) { ; }
void top_network_offline (void) { ; }
void top_network_online (void) { ; }
void top_timer_expiration (void) { ; }
void top_can_play (uint16_t samples) { ; }
void top_can_record (uint16_t samples) { ; }

#endif /* CONFIG_MAINFUNCTION_BOOTLOADER */
