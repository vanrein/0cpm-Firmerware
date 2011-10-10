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


uint8_t regshift [7] = { 0,    8,    8,    6,    8,    6,    7    };
uint8_t regsubs  [7] = { 0,    1,    1,    4,    1,    4,    2    };


int main (int argc, char *argv []) {

	if (argc != 2) {
		fprintf (stderr, "Usage: %s /dev/i2c-N\n", argv [0]);
		exit (1);
	}

	int bus = open (argv [1], O_RDWR);
	if (bus < 0) {
		perror ("Failed to open I2C bus");
		exit (1);
	}
	ioctl (bus, I2C_TIMEOUT, 1);
	ioctl (bus, I2C_RETRIES, 2);

	int chan;
	for (chan = 0; chan <= 15; chan++) {
		if (ioctl (bus, I2C_SLAVE, 0x40 | chan) == -1) {
			perror ("Failed to set channel as address");
			exit (1);
		}
		printf ("Channel %d registers:", chan);
		int reg;
		for (reg = 1; reg <= 6; reg++) {
			uint8_t buf [6];
			int subreg;
			for (subreg = 0; subreg < regsubs [reg]; subreg++) {
				usleep (INTERPOLL_WAIT_MS * 1000);
buf [0] =  0x01;
				if (write (bus, buf, 1) != 1) {		// Set register number to 1
					perror ("Failed to setup register address");
					goto nextchan; // exit (1);
				}
				usleep (INTERPOLL_WAIT_MS * 1000);
				if (read (bus, buf + 1, 6) != 6) {		// Read current register value
					perror ("Failed to read register data");
					goto nextchan; exit (1);
				}
				printf (" %d%c=%02x", reg, "ABCD" [(buf [reg] >> regshift [reg])], buf [reg]);
			}
		}
nextchan:
		printf ("\n");
	}

	close (bus);
	return 0;
}
