/*
 * http://devel.0cpm.org/ -- Open source firmware for SIP phones.
 *
 * Timer drivers
 */


#ifndef HEADER_TIMER
#define HEADER_TIMER

#include <0cpm/irq.h>


/* The bottom defines a few useful things:
 *   typedef uint..._t timing_t;
 *   #define TIME_MSEC(x) ...
 *   #define TIME_SEC(x) ...
 *   #define TIME_BEOFRE(x,y) ...
 * The former can be used as a parameter without pointers;
 * the TIME_xxx define such periods in terms of that time.
 */


/* Timer drivers are split in a top half and a bottom half.  The top half is generic, the
 * bottom half is device-specific.  The bottom half routines start with bottom_irqtimer_
 * while the top half functions start with irqtimer_
 *
 * The bottom system is only aware of the time of the next hardware-raised interrupt.
 * The top system maintains a timeout-ordered queue of interrupts to raise.
 */

typedef struct irqtimer_type irqtimer_t;
struct irqtimer_type {
	irq_t tmr_irq;
	timing_t tmr_expiry;
};


/* Managing functions for kernel-defined timers */
void irqtimer_start   (irqtimer_t *tmr, timing_t delay, irq_handler_t hdl, priority_t prio);
void irqtimer_restart (irqtimer_t *tmr, timing_t intval);
void irqtimer_stop    (irqtimer_t *tmr);


/* Top-half operations to manipulate timers */
void top_timer_expiration (timing_t exptime);


/* Bottom-half operations to manipulate the next timer interrupt */
timing_t bottom_time (void);
timing_t bottom_timer_set (timing_t nextirq);


#endif
