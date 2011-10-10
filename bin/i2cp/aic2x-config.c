/* aic20x-config.c -- configure registers of a TLV320AIC2x codec over I2C
 *
 * This file is provided as part of 0cpm Firmerware.
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
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <linux/i2c-dev.h>


#define INTERPOLL_WAIT_MS 5


const char *  regname [13] = { "1", "2", "3A", "3B", "3C", "3D", "4", "5A", "5B", "5C", "5D", "6A", "6B" };
const uint8_t regmask [13] = { 0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0x80, 0x80 };
const uint8_t regseln [13] = { 0x00, 0x00, 0x00, 0x40, 0x80, 0xc0, 0x00, 0x00, 0x40, 0x80, 0xc0, 0x00, 0x80 };

const uint8_t  regidx0 [8] = { 0, 1, 2, 6, 7,  11 };
const uint8_t  regidx1 [8] = { 0, 1, 3, 6, 8,  12 };
const uint8_t  regidx2 [8] = { 0, 1, 4, 6, 9,  11 };
const uint8_t  regidx3 [8] = { 0, 1, 5, 6, 10, 12 };

const uint8_t *regidxs [4] = { regidx0, regidx1, regidx2, regidx3 };

int main (int argc, char *argv []) {
	int i;
	uint8_t regs [13];
	uint8_t slave;

	if (argc != 16) {
		fprintf (stderr, "Usage: %s /dev/i2c-N SLAVEADDR 1 2 3A 3B 3C 3D 4 5A 5B 5C 5D 6A 6B\n"
			"\tThe slave address is usually 0x40, 0x41, ... 0x4f\n"
			"\tNumbers may be prefixed with 0 for octal, or 0x for hex\n",
			"\tNote: All subregister indexes are assumed to be zero\n",
			argv [0]);
		exit (1);
	}

	for (i=0; i < 14; i++) {
		char *strval = argv [2+i];
		long intval;
		int base = 10;
		if (strncasecmp (strval, "0x", 2) == 0) {
			base = 16;
			strval += 2;
		} else if (*strval == '0') {
			base = 8;
		} else if (! *strval) {
			strval = "xyz";	// Cause error later
		}
		intval = strtol (strval, &strval, base);
		if ((*strval) || (intval < 0) || (intval > 255)) {
			fprintf (stderr, "Value error: %s value %s should be an integer in the range 0x00 to 0xff\n", regname [i-1], argv [2+i]);
			exit (1);
		}
		if (i) {
			if ((intval & regmask [i-1]) != regseln [i-1]) {
				fprintf (stderr, "Error: Register %s value %s should fall between 0x%02x and 0x%02x\n", regname [i-1], argv [2+i], regseln [i-1], regseln [i-1] + regmask [i-1] - 1);
				exit (1);
			}
			regs [i-1] = intval;
			printf ("%-2s := 0x%02x\n", regname [i-1], intval);
		} else {
			if ((intval < 0x40) || (intval > 0x4f)) {
				fprintf (stderr, "Warning: The slave address usually falls in the range 0x40 to 0x4f\n");
				sleep (5);
			}
			slave = intval;
			printf ("Slave address 0x%02x, channel %d\n", intval, intval & 0x0f);
		}
	}

	int bus = open (argv [1], O_RDWR);
	if (bus < 0) {
		perror ("Failed to open I2C bus");
		exit (1);
	}
	ioctl (bus, I2C_TIMEOUT, 1);
	ioctl (bus, I2C_RETRIES, 2);

	if (ioctl (bus, I2C_SLAVE, slave) == -1) {
		perror ("Failed to set channel as address");
		exit (1);
	}
	uint8_t buf [9];
	buf [0] = 0x01;		// Sequences start at register 1
	int rpt, regnum;
	for (rpt = 0; rpt < 4; rpt++) {		// Countdown works too!
		for (regnum = 0; regnum < 8; regnum++) {
			buf [1+regnum] = regs [regidxs [rpt] [regnum]];
		}
		printf ("Register pass '%c'\n", 'A' + rpt);
		usleep (INTERPOLL_WAIT_MS * 1000);
		if (write (bus, buf, sizeof (buf)) != sizeof (buf)) {
			perror ("Failed to write to registers");
		}
	}

	close (bus);
	return 0;
}
