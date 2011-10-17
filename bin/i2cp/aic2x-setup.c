/* aic20x-config.c -- setup individual TLV320AIC2x registers over I2C
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


/*
 * Note: If this program works (which it does), it proves that the
 * subregisters may be addressed directly by way of the distinguishing
 * top bits.  This is not documented in the datasheet, but that is
 * indeed not the best bit of technical work that TI has accomplished.
 */


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <fcntl.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <linux/i2c-dev.h>


#define INTERPOLL_WAIT_MS 5


const char *  regname [14] = { "1", "2", "3A", "3B", "3C", "3D", "4A", "4B", "5A", "5B", "5C", "5D", "6A", "6B" };
const uint8_t regmask [14] = { 0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0x80, 0x80, 0xc0, 0xc0, 0xc0, 0xc0, 0x80, 0x80 };
const uint8_t regseln [14] = { 0x00, 0x00, 0x00, 0x40, 0x80, 0xc0, 0x00, 0x80, 0x00, 0x40, 0x80, 0xc0, 0x00, 0x80 };

uint8_t argval (char *strval) {
	long intval;
	int base = 10;
	char *postfix;
	if (strncasecmp (strval, "0x", 2) == 0) {
		base = 16;
		strval += 2;
	} else if (*strval == '0') {
		base = 8;
	} else if (! *strval) {
		strval = "(empty)";	// Cause error later
	}
	intval = strtol (strval, &postfix, base);
	if ((*postfix) || (intval < 0) || (intval > 255)) {
		fprintf (stderr, "Value error: Value %s should be an integer in the range 0x00 to 0xff\n", strval);
		exit (1);
	}
	return (uint8_t) intval;
}

int main (int argc, char *argv []) {
	int i;
	uint8_t regs [13];
	uint8_t slave;

	if (argc < 4) {
		fprintf (stderr, "Usage: %s /dev/i2c-N SLAVEADDR REGISTER=VALUE...\n"
			"\tThe slave address is usually 0x40, 0x41, ... 0x4f\n"
			"\tThe register may be a subregister, the value is a number\n"
			"\tNumbers may be prefixed with 0 for octal, or 0x for hex\n"
			"\tNote: All subregister indexes are assumed to be zero\n",
			argv [0]);
		exit (1);
	}

	slave = argval (argv [2]);
	printf ("Slave address 0x%02x, channel %d\n", slave, slave & 0x0f);
	if ((slave < 0x40) || (slave > 0x4f)) {
		fprintf (stderr, "Warning: The slave address usually falls in the range 0x40 to 0x4f\n");
		sleep (5);
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

	for (i=0; i < argc-3; i++) {
		char *strval = argv [3+i];
		int reglen = 0;
		if ((!*strval) || (!strval [1]) || (!strval [2])) {
			fprintf (stderr, "Argument too short, specify REGISTER=VALUE for %s\n", strval);
			exit (1);
		}
		if (strval [1] == '=') {
			reglen = 1;
		} else if (strval [2] == '=') {
			reglen = 2;
		} else {
			fprintf (stderr, "Specify REGISTER=VALUE with REGISTER 1 or 2 characters long, not \n", strval);
			exit (1);
		}
		uint8_t intval = argval (strval + reglen + 1);
		int j;
		bool found = false;
		for (j=0; j < 13; j++) {
			if (reglen != strlen (regname [j])) {
				continue;
			}
			if (strncasecmp (regname [j], strval, reglen) != 0) {
				continue;
			}
			if ((intval & regmask [j]) != regseln [j]) {
				fprintf (stderr, "Error: Register %s value %s should fall between 0x%02x and 0x%02x\n", regname [j], strval + reglen + 1, regseln [j], regseln [j] + 255 - regmask [j]);
				exit (1);
			}
			uint8_t buf [2];
			buf [0] = *regname [j] - '0';
			buf [1] = intval;
			printf ("%-2s := 0x%02x\n", regname [j], intval);
			usleep (INTERPOLL_WAIT_MS * 1000);
			if (write (bus, buf, sizeof (buf)) != sizeof (buf)) {
				perror ("Failed to write to registers");
			}
			found = true;
		}
		if (!found) {
			fprintf (stderr, "Register for %s not found\n", strval);
			exit (1);
		}
	}

	close (bus);
	return 0;
}
