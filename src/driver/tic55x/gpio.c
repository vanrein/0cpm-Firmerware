/* GPIO driver for tic55x
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

#include <stdint.h>
#include <stdbool.h>

#define BOTTOM
#include <config.h>

void tic55x_output_set (uint16_t regaddr, uint16_t regval) {
	* ( (ioport uint16_t *) regaddr) = regval;
}

uint16_t tic55x_input_get (uint16_t regaddr) {
	return * ( (uint16_t *) regaddr);
}

void tic55x_output_set_bit (uint16_t regaddr, uint8_t regbit, bool value) {
	if (value) {
		* ( (ioport uint16_t *) regaddr) |=    1 << regbit ;
	} else {
		* ( (ioport uint16_t *) regaddr) &= ~( 1 << regbit);
	}
}

bool tic55x_input_get_bit (uint16_t regaddr, uint8_t regbit) {
	uint16_t value = * (ioport uint16_t *) regaddr;
	return (value & (1 << regbit)) ? true: false;
}
