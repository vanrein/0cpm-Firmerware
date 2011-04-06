/* GPIO driver for tic55x */

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
