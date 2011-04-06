/* i2cp.c -- EEPROM read/write over i2c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <linux/i2c-dev.h>


#define INTERPAGE_WAIT_MS 5
#define PAGESIZE_BYTES 128


int main (int argc, char *argv []) {

	uint32_t size = ~0L;
	uint32_t address = 0;

	if (argc != 3) {
		fprintf (stderr, "Usage: %s image.bin /dev/i2c-N\n   or: %s /dev/i2c-N image.bin\n", argv [0], argv [0]);
		exit (1);
	}
	
	struct stat stin;
	if (stat (argv [1], &stin) == -1) {
		perror ("Failed to stat input");
		exit (1);
	}
	if (stin.st_rdev == 0) {
		if (stin.st_size >= 8 * 65536) {
			fprintf (stderr, "Input image exceeds maximum possible\n");
			exit (1);
		} else {
			size = stin.st_size;
		}
	}

	struct stat stout;
	if (stat (argv [2], &stout) == -1) {
		perror ("Failed to stat output");
		exit (1);
	}
	if (stout.st_rdev == 0) {
		if (stout.st_size >= 8 * 65536) {
			fprintf (stderr, "Output image exceeds maximum possible\n");
			exit (1);
		} else if (size == (uint32_t) ~0L) {
			size = stout.st_size;
		}
	}


	int fin = open (argv [1], O_RDWR);
	if (fin < 0) {
		perror ("Failed to read input");
		exit (1);
	}
	int fout = open (argv [2], O_RDWR);
	if (fout < 0) {
		perror ("Failed to access output");
		exit (1);
	}

	if (stin.st_rdev) {
		ioctl (fin, I2C_TIMEOUT, 1);
		ioctl (fin, I2C_RETRIES, 2);
	}
	if (stout.st_rdev) {
		ioctl (fout, I2C_TIMEOUT, 1);
		ioctl (fout, I2C_RETRIES, 2);
	}

	while (address < size) {
		uint8_t buf [2 + PAGESIZE_BYTES];
		buf [0] = (address & 0x0000ff00) >> 8;
		buf [1] = (address & 0x000000ff);
		if ((address & 0x0000ffff) == 0x000000) {
			uint8_t slave = address >> 16;
			slave &= 0x07;
			slave += 0x50;
			if (stin.st_rdev) {
				if (ioctl (fin, I2C_SLAVE, slave) == -1) {
					perror ("Failed to set input slave address");
					exit (1);
				}
			}
			if (stout.st_rdev) {
				if (ioctl (fout, I2C_SLAVE, slave) == -1) {
					perror ("Failed to set output slave address");
					exit (1);
				}
			}
		}
		if (stin.st_rdev) {
			write (fin, buf, 2);
		}
		int rdlen = read (fin, buf+2, (size-address) >= PAGESIZE_BYTES? PAGESIZE_BYTES: size-address);
		if (rdlen >= 0) {
			if (rdlen == 0) {
				break;
			}
			if (stout.st_rdev) {
				if (write (fout, buf, 2 + rdlen) < 2 + rdlen) {
					perror ("Failed to write to i2c");
				}
			} else {
				if (write (fout, buf+2, rdlen) < rdlen) {
					perror ("Failed to write to file");
				}
			}
		} else {
			perror ("Failed to read");
			exit (1);
		}
		address += rdlen;
		usleep (INTERPAGE_WAIT_MS * 1000);
	}

	close (fin);
	close (fout);
	return 0;
}
