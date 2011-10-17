/* aic20x-dump.c -- dump registers from a TLV320AIC2x codec accessible over I2C
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


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <linux/i2c-dev.h>


#define INTERPOLL_WAIT_MS 5


uint8_t regshift [7] = { 0,    8,    8,    6,    7,    6,    7    };
uint8_t regsubs  [7] = { 0,    1,    1,    4,    2,    4,    2    };


int main (int argc, char *argv []) {

	int minslave = 0x40;
	int maxslave = 0x4f;

	if ((argc < 2) || (argc > 3)) {
		fprintf (stderr, "Usage: %s /dev/i2c-N [slave-address]\n", argv [0]);
		exit (1);
	}

	if (argc >= 3) {
		int base = 10;
		char *intstr = argv [2];
		long intval = strtol (intstr, &intstr, 0);
		if ((*intstr) || (intval < 0x00) || (intval > 0x7f)) {
			fprintf (stderr, "Slave address %s is out of range\n", argv [2]);
			exit (1);
		}
		minslave = maxslave = intval;
	}

	int bus = open (argv [1], O_RDWR);
	if (bus < 0) {
		perror ("Failed to open I2C bus");
		exit (1);
	}
	ioctl (bus, I2C_TIMEOUT, 1);
	ioctl (bus, I2C_RETRIES, 2);

	int slave;
	for (slave = minslave; slave <= maxslave; slave++) {
		if (ioctl (bus, I2C_SLAVE, slave) == -1) {
			perror ("Failed to set slave address");
			exit (1);
		}
		printf ("Channel %d registers (slave 0x%02x):", slave & 0x0f, slave);
		int reg;
		for (reg = 1; reg <= 6; reg++) {
			uint8_t buf [6];
			int subreg;
			for (subreg = 0; subreg < regsubs [reg]; subreg++) {
				usleep (INTERPOLL_WAIT_MS * 1000);
buf [0] =  0x01;
				if (write (bus, buf, 1) != 1) {		// Set register number to 1
					perror ("Failed to setup register address");
					goto nextslave; // exit (1);
				}
				usleep (INTERPOLL_WAIT_MS * 1000);
				if (read (bus, buf + 1, 6) != 6) {		// Read current register value
					perror ("Failed to read register data");
					goto nextslave; exit (1);
				}
				printf (" %d%c=%02x", reg, "ABCD" [(buf [reg] >> regshift [reg])], buf [reg]);
			}
		}
nextslave:
		printf ("\n");
	}

	close (bus);
	return 0;
}
