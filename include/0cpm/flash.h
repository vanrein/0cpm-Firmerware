/* flash.h -- Interface to flash memory.
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


/* This module enables reading and/or writing flash memory.
 * It also details where the flash partition table is.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#ifndef HEADER_FLASH
#define HEADER_FLASH

#ifdef HAVE_FLASH_PARTITIONS

/* The structure of a flash partition in the partition table.
 * Definitions for the flags field follow.
 *
 * Bootable partitions will usually be checksummed to avoid that
 * they take down the system; if a checksum fails, the booting
 * does not take place and the partition will be skipped.  This
 * provides a fallback mechanism for half-installed images.  If
 * checksums are applied, they are local to the bottom half.
 *
 * A partition can be write-protected in general, or it may only
 * be writeable if the entire image is first downloaded into RAM.
 * A good example of the latter is the very first piece of booting
 * codes; it is dangerous to replace, and this should only be done
 * atomically, that is, not dependent on anything but sustained
 * power supply; network protocols should definately not cause a
 * problem.  This is why the _WRITE_AFTER_LOAD2RAM exists.  This
 * particular condition will usually be met only in the dedicated
 * bootloader image; and it may not be physically possible for all
 * images but the smallest.  The bootloader is definately designed
 * to be one such small image, so it will always be setup safely.
 */
struct flashpart {
	uint16_t flags;
	uint8_t *filename;
	uint16_t firstblock;
	uint16_t numblocks;
};

#define FLASHPART_FLAG_BOOTABLE			0x0001
#define FLASHPART_FLAG_CHECKSUMMED		0x0002

#define FLASHPART_FLAG_WRITEPROTECT		0x0010
#define FLASHPART_FLAG_WRITE_AFTER_LOAD2RAM	0x0020

#define FLASHPART_FLAG_LAST			0x8000


/* An external definition (usually in phone-specific code)
 * contains an array of one or more entries of flashpart
 * structures.  Only the last will have the FLASHPART_FLAG_LAST
 * flag set.
 *
 * There will usually be one partition with name "ALLFLASH.BIN"
 * that covers the entire flash memory, including things that
 * may not actually be in any partition.  Usually, this is the
 * last entry in the flash partition table.
 */
extern struct flashpart bottom_flash_partition_table [];


/* Read a 512-byte block from flash.
 * The return value indicates success.
 */
bool bottom_flash_read (uint16_t blocknr, uint8_t data [512]);


/* Write a 512-byte block to flash.  It is assumed that this
 * is done sequentially; any special treatment for a header
 * page will be done by the bottom layer, not the top.
 * The return value indicates success.
 */
bool bottom_flash_write (uint16_t blocknr, uint8_t data [512]);


#endif	/* HAVE_FLASH_PARTITIONS */

#endif	/* HEADER_FLASH */
