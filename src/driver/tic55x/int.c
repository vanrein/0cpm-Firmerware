/* tic55x interrupt handling */


#include <stdint.h>
#include <stdbool.h>

#define BOTTOM
#include <config.h>

#include <0cpm/cpu.h>


/* This flag is set by every interrupt that calls a top_XXX function.
 * It indicates that the top *may* have new work to do.  This is used
 * to establish whether the bottom can go to sleep, as part of a
 * two-phase sleep protocol: bottom_sleep_prepare() clears this flag,
 * then the top checks if any work is pending at that time, and then
 * bottom_sleep_commit() is called.  The latter will fall through
 * immediately if the flag has been set meanwhile, as the top may
 * have missed incoming work.
 */
volatile bool tic55x_top_has_been_interrupted;


void bottom_sleep_prepare (void) {
	tic55x_top_has_been_interrupted = false;
}

void bottom_sleep_commit (sleep_depth_t depth) {
	bottom_critical_region_begin ();
	if (!tic55x_top_has_been_interrupted) {
		if (depth == SLEEP_HIBERNATE) {
			// Hibernate: Idle CPU and most I/O
			ICR = (1 << REGBIT_ICR_CPUI) | (1 << REGBIT_ICR_ICACHEI) | (1 << REGBIT_ICR_MPI) | (1 << REGBIT_ICR_PERI);
			PICR = (1 << REGBIT_PICR_MISC) | (1 << REGBIT_PICR_BIOST) | (1 << REGBIT_PICR_WDT) | (1 << REGBIT_PICR_URT) | (1 << REGBIT_PICR_I2C) | (1 << REGBIT_PICR_SP1) | (1 << REGBIT_PICR_SP0);
		} else {
			// Snooze: Idle CPU but nothing more
			ICR = 1 << REGBIT_ICR_CPUI;
		}
		asm ("	IDLE");
	}
	bottom_critical_region_end ();
}

interrupt void tic55x_no_isr (void) {
	/* No action */ ;
}


// Setup interrupts -- probably move this to the trampoline code?
void tic55x_setup_interrupts (void) {
	// extern uint32_t interrupt_table;
	// IVPD = (uint16_t) (((uint32_t) interrupt_table) >> 8);
	asm (
"	.ref	isrmap0, isrmap1\n"
"	mov	#(isrmap0 >> 8), mmap(_IVPD)\n"
"	mov	#(isrmap1 >> 8), mmap(_IVPH)\n"
);
}

