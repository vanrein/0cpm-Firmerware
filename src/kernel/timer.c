/* Timer drivers
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


#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/irq.h>
#include <0cpm/led.h> //TODO// Just for testing
#include <0cpm/timer.h>
#include <0cpm/cons.h>



/* Timers are deferred actions.  Some of those actions may be of a
 * repetitive nature (at fixed intervals) and others may be oneshots.
 *
 * The actions triggered after the specified time has passed are
 * invoked as an interrupt handler.  If continued actions are needed,
 * including repeated activation, the interrupt handler should set it
 * up before it exits.
 *
 * The bottom half only needs to support a single timer.  This top
 * module administers a queue of timers, and will wait for the
 * passing of the shortest time in the queue.  The queue therefore
 * is ordered from the first timer to expire to the last.
 *
 * The bottom invokes top_timer_expiration() with the timing_t that
 * triggered the timer expiration.  The function should return the
 * new setting for the bottom timer, if any, or otherwise simply
 * the same value as was handed to it upon invocation.  The bottom
 * will then setup the hardware timer accordingly.  It is the
 * bottom's responsibility to avoid setting a timing_t value that
 * has actually expired; if that was allowed to happen, the whole
 * system might get stuck.  The top ensures that invocations of
 * top_timer_expiration() fire an interrupt for all expired timers,
 * but do nothing if no timers had expired.  Furthermore, there
 * is a facility to temporarily disable timer expirations to lead
 * to interrupts being triggered.
 *
 * The fine line between past and future (aka the present) is good
 * to define clearly.  The value passed to bottom_timer_set() is
 * the minimum value of bottom_time() at which the timer could
 * expire.  So if you bottom_timer_set(123), then you could see
 * top_timer_expiration (123) or top_timer_expiration (125) but
 * not top_timer_expiration (122) being called.  It is the task of
 * top_timer_expiration (exptime) to fire interrupts for any timer
 * whose expiration timer is set to a value <= exptime.  This is
 * because the value of the head's expiration timer is sent down
 * through bottom_timer_set().
 */


/* The queue for waiting timer events.  There is no queue for ready
 * timer events; those are sent to general IRQ queues.
 */
static irqtimer_t *irqtimer_wait_queue = NULL;

static bool irqtimer_interrupt_blocked = true;
static bool irqtimer_interrupt_occurred = false;


/* Process a timer interrupt.
 */
timing_t top_timer_expiration (timing_t exptime) {
bottom_led_set (LED_IDX_BACKLIGHT, 0);
	if (irqtimer_interrupt_blocked) {
		irqtimer_interrupt_occurred = true;
	} else {
		while (irqtimer_wait_queue && TIME_BEFOREQ (irqtimer_wait_queue->tmr_expiry, exptime)) {
			irqtimer_t *here = irqtimer_wait_queue;
			irqtimer_wait_queue = here->tmr_next;
			here->tmr_next = NULL;
			irq_fire (&here->tmr_irq);
		}
		if (irqtimer_wait_queue) {
			exptime = irqtimer_wait_queue->tmr_expiry;
		}
	}
bottom_led_set (LED_IDX_BACKLIGHT, 1);
	return exptime;
}


/* Enable or disable a timer interrupt.  This does not actually
 * stop the hardware interrupt from occurring, but it does stop
 * the queue from being observed or modified in the interrupt
 * handler routine.  This facility is useful to allow modifying
 * the queue without having to take down interrupt handling for
 * all the other hardware-related processes.  As an optimisation,
 * the flag irqtimer_interrupt_occurred will record that the
 * top_timer_expiration() handler was invoked, so that the queue
 * needs to be revisited.
 */
static inline void irqtimer_disable (void) {
	irqtimer_interrupt_blocked = true;
}
static void irqtimer_enable (void) {
	irqtimer_interrupt_blocked = false;
	if (irqtimer_interrupt_occurred) {
		timing_t now, next;
		bottom_printf ("Activating deferred timer interrupt\n");
		now = bottom_time ();
		next = top_timer_expiration (now);
		if (next != now) {
			bottom_timer_set (next);
		}
	}
}


/* Enqueue an initialised timer structure to the timer wait queue.
 */
static void irqtimer_enqueue (irqtimer_t *tmr) {
	irqtimer_t **here;
	irqtimer_disable ();
	here = &irqtimer_wait_queue;
	while ((*here) && TIME_BEFOREQ ((*here)->tmr_expiry, tmr->tmr_expiry)) {
		here = (irqtimer_t **) & (*here)->tmr_next;
	}
	tmr->tmr_next = *here;
	(*here) = tmr;
	if (irqtimer_wait_queue == tmr) {
		bottom_timer_set ((*here)->tmr_expiry);
	}
	irqtimer_enable ();
}

/* Setup a new timer by enqueueing its structure in the timer list.
 */
void irqtimer_start (irqtimer_t *tmr, timing_t delay, irq_handler_t hdl, priority_t prio) {
	tmr->tmr_irq.irq_next = NULL;
	tmr->tmr_irq.irq_handler = hdl;
	tmr->tmr_irq.irq_priority = prio;
	tmr->tmr_expiry = bottom_time () + delay;	// Always support: irqtimer_restart()
	if (delay == 0) {
		tmr->tmr_next = NULL;
		irq_fire (&tmr->tmr_irq);
	} else {
		irqtimer_enqueue (tmr);
	}
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

