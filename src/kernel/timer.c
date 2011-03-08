/*
 * http://devel.0cpm.org/ -- Open source firmware for SIP phones.
 *
 * Timer drivers
 */


#include <config.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <0cpm/cpu.h>
#include <0cpm/irq.h>
#include <0cpm/timer.h>


/* The queue for waiting timer events.  There is no queue for ready
 * timer events; those are sent to general IRQ queues.
 */
static irqtimer_t *irqtimer_wait_queue = NULL;

#define tmr_next tmr_irq.irq_next


static bool irqtimer_interrupt_blocked = true;
static bool irqtimer_interrupt_occurred = false; // TODO: highest priority_t

static bool irqtimer_fire_blocked = false;


/* Process a timer interrupt.
 */
void top_timer_expiration (timing_t exptime) {
	if (irqtimer_interrupt_blocked) {
		printf ("Deferring timer interrupt\n");
		irqtimer_interrupt_occurred = true;
	} else {
		while (irqtimer_wait_queue && (irqtimer_wait_queue->tmr_expiry < exptime)) {
			irqtimer_t *here = irqtimer_wait_queue;
			irqtimer_wait_queue = (irqtimer_t *) here->tmr_next;
			printf ("Firing IRQ for timer interrupt\n");
			irq_fire (&here->tmr_irq);
		}
	}
}


/* Enable or disable a timer interrupt.
 */
void irqtimer_enable (void) {
#if 0
	timing_t now = bottom_time ();
	while (irqtimer_interrupt_occurred) {
		irqtimer_interrupt_occurred = false;
		top_timer_expiration (now);
		irqtimer_interrupt_blocked = false;
	}
#endif
	irqtimer_interrupt_blocked = false;
	if (irqtimer_interrupt_occurred) {
		printf ("Activating deferred timer interrupt\n");
		timing_t now = bottom_time ();
		top_timer_expiration (now);
	}
}
inline void irqtimer_disable (void) {
	irqtimer_interrupt_blocked = true;
}


/* Enqueue an initialised timer structure to the timer wait queue.
 */
static void irqtimer_enqueue (irqtimer_t *tmr) {
	irqtimer_t **here;
	irqtimer_disable ();
	here = &irqtimer_wait_queue;
	while ((*here) && ((*here)->tmr_expiry < tmr->tmr_expiry)) {
		here = (irqtimer_t **) & (*here)->tmr_next;
	}
	tmr->tmr_next = (void *) *here;
	(*here) = tmr;
	if (irqtimer_wait_queue == tmr) {
		bottom_timer_set ((*here)->tmr_expiry);
	}
	irqtimer_enable ();
}

/* Setup a new timer by enqueueing its structure in the timer list.
 */
void irqtimer_start (irqtimer_t *tmr, timing_t delay, irq_handler_t hdl, priority_t prio) {
	timing_t new_expiry = bottom_time () + delay;
	tmr->tmr_irq.irq_handler = hdl;
	tmr->tmr_irq.irq_priority = prio;
	tmr->tmr_expiry = new_expiry;
	irqtimer_enqueue (tmr);
}


/* Continue a repeating timer by adding the interval delay to the
 * last expiry time.  If more than the timer interval has passed
 * since the last timer expiration, the timer will fire immediately.
 */
void irqtimer_restart (irqtimer_t *tmr, timing_t intval) {
	tmr->tmr_expiry += intval;
	irqtimer_enqueue (tmr);
}


/* Prematurely remove a timer from the timer queue.  If it is not
 * there, ignore that fact silently.  Note that there may still
 * be interrupts that were caused by the timer that is now being
 * stopped.  That may actually revive an interval timer.  TODO...
 */
void irqtimer_stop (irqtimer_t *tmr) {
	irqtimer_t **here;
	irqtimer_disable ();
	here = &irqtimer_wait_queue;
	while ((*here)) {
		if (*here == tmr) {
			*here = (irqtimer_t *) tmr->tmr_next;
			break;
		}
		here = (irqtimer_t **) & (*here)->tmr_next;
	}
	irqtimer_enable ();
}


/* TODO: Initialise this module by registering the top-half timer interrupt handler.
 */
