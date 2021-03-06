/* General timer management.
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


/* The tic55x has multiple 64-bit timers, we'll use timer0 and timer1.
 *
 * Since a timer can either be set to fire or continue to count,
 * but not both, we'll need two timers:
 *
 *  - timer0 counts until the next time-based interrupt
 *  - timer1 counts the time since the system started
 *
 * The reason for assigning the timers like this?  timer0 has a higher
 * interrupt level than, say, uart.  That helps to get more accuracy
 * from a realtime system.
 *
 * The tic55x timers are 64-bit, and can be split in two halves.
 *
 * The first half (CNT4:CNT3 and PRD4:PRD3) is used as a prescale-counter,
 * to reduce the crystal clock frequency to a 1 ms pace.  This is chained
 * to the second half (CNT2:CNT1), which then forms the actual timer used
 * by the 0cpm firmerware.
 *
 * The actual 2:1 timer1 is a 32-bit counter that counts round and round
 * in circles, without ever causing an interrupt.  The actual 2:1 timer0
 * is set to the next interrupt time, and causes an interrupt when it is
 * reached.  This setting is based on the requested interrupt time and
 * the value in timer1 at the time of the request.
 *
 * The top half takes care of setting up timer queues to implement a more
 * general timing discipline, processing interrupts at such timeouts.
 *
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdint.h>
#include <stdbool.h>

#define BOTTOM
#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/irq.h>
#include <0cpm/timer.h>


/* Global variables */

static timing_t current_timer = 0;

#define ENTROPY_BUFLEN 17
static uint8_t entropy_pseudo [ENTROPY_BUFLEN];
static uint8_t entropy_wpos = 0;
static uint8_t entropy_rpos = 0;


/* Read the current time from timer1's top part.  Be careful about
 * timer increments between the two word reads.
 *
 * The tic55x copies timer registers to shadow registers to ensure
 * that all values are in sync; the code need not verify that.
 */
timing_t bottom_time (void) {
	uint16_t cnt1 = GPTCNT1_1;
	uint16_t cnt2 = GPTCNT2_1;
	return (((timing_t) cnt2) << 16) | ((timing_t) cnt1);
}


/* Setup a new timing for the following timer interrupt.  If one is
 * currently underway, disable it so it will not spark new interrupts
 * after this function returns.
 */
timing_t bottom_timer_set (timing_t tim) {
	timing_t intval, previous;
	uint16_t cnt1, cnt2;
	/* First of all, reset timer0 so it can be configured */
	GPTGCTL1_0 &= ~ ( REGVAL_GCTL_TIM12RS | REGVAL_GCTL_TIM34RS);
	IFR0 = (1 << REGBIT_IER0_TINT0);
	/* Juggle timing values to return the current setting */
	previous = current_timer;
	current_timer = tim;
again:
	/* Setup counter and period registers for timer0 */
	GPTCNT3_0 = GPTCNT3_1; // Read CNT3, copy CNT4 to shadow
	GPTCNT4_0 = GPTCNT4_1; // Load CNT4 from shadow copy
	GPTCNT1_0 = GPTCNT1_1; // Read CNT1, copy CNT2 to shadow
	GPTCNT2_0 = GPTCNT2_1; // Load CNT2 from shadow copy
	GPTPRD2_0 = (uint16_t) (tim >> 16   );
	GPTPRD1_0 = (uint16_t)  tim          ;
	/* Poll the running timer1 */
	cnt1 = GPTCNT1_0;
	cnt2 = GPTCNT2_0;
	intval = tim - ((GPTCNT2_0 << 16) | GPTCNT1_0);
	if ((intval == 0) || (intval >> 31)) {
		timing_t oldtimer = tim;
		// Invoke handler right now, do not run timer0
		tim = top_timer_expiration (oldtimer);
		if (tim != oldtimer) {
			goto again; //TODO:TEST//
		}
		//TODO:OLD// current_timer = 0;
		//TODO:OLD// return previous;
	}
	/* Finally, enable timer1 so it starts counting time */
	GPTGCTL1_0 |= REGVAL_GCTL_TIM12RS | REGVAL_GCTL_TIM34RS;
	return previous;
}


/* Timer interrupt handler for timer0 expiration */
interrupt void tic55x_tint0_isr (void) {
	extern volatile bool tic55x_top_has_been_interrupted;
	timing_t now = bottom_time ();
	timing_t newsetting;
	current_timer = 0;
	newsetting = top_timer_expiration (now);
	if (now != newsetting) {
		bottom_timer_set (now);
	}
	tic55x_top_has_been_interrupted = true;
}


/* Read the lower-half timer for a bit of entropy, at a time
 * deemed appropriate by the top half.
 */
void bottom_rndseed (void) {
	entropy_pseudo [entropy_wpos++] ^= GPTCNT3_1 & 0xff;
	if (entropy_wpos >= ENTROPY_BUFLEN) {
		entropy_wpos = 0;
	}
}

/* Retrieve a given number of random bytes from the entropy
 * buffer.
 */
void bottom_rnd_pseudo (uint8_t *rnd, uint8_t len) {
	while (len-- > 0) {
		// Sigh: 16-bit DSP... so uint8_t is 16-bit
		*rnd++ = entropy_pseudo [entropy_rpos += 4] & 0xff;
		if (entropy_rpos >= ENTROPY_BUFLEN) {
			entropy_rpos -= ENTROPY_BUFLEN;
		}
	}
}


/* Setup timers and the corresponding interrupts for timer0/timer1
 */
void tic55x_setup_timers (void) {
	/* Be sure that the timers are reset, so they can be configured */
	GPTGCTL1_0 =
	GPTGCTL1_1 = 0;
	/* Let the lower half of timer0/timer1 count once per ms */
	GPTPRD4_0 =
	GPTPRD4_1 = (uint16_t) (((uint32_t) SYSCLK1_TO_MS_DIVIDER-1) >> 16);
	GPTPRD3_0 =
	GPTPRD3_1 = (uint16_t) (((uint32_t) SYSCLK1_TO_MS_DIVIDER-1) & 0xffff);
	/* Let the upper half of timer1 count with a 2^32 period */
	GPTPRD2_1 =
	GPTPRD1_1 = 0xffff;
	/* Let timer1 run entirely free; let timer0 lower half run free */
	GPTCTL1_0 = REGVAL_CTL12_ENAMODE_ONETIME;
	GPTCTL1_1 = REGVAL_CTL12_ENAMODE_CONTINUOUS;
	GPTGCTL1_0 = REGVAL_GCTL_TIMMODE_DUAL32CHAINED;
	GPTGCTL1_1 = REGVAL_GCTL_TIMMODE_DUAL32CHAINED | REGVAL_GCTL_TIM34RS | REGVAL_GCTL_TIM12RS;
	/* Clear, then permit interrupts from timer0 */
	IFR0  = (1 << REGBIT_IER0_TINT0);
	IER0 |= (1 << REGBIT_IER0_TINT0);
}

