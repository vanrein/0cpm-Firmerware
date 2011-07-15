/* CPU drivers
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


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/irq.h>
#include <0cpm/cons.h>


/* Round-robin queues with interrupts and closures, ordered by priority.
 * To be fair, the elements pointed at have already been completed
 * and are awaiting roll-over or removal.
 * TODO: Make the array contents volatile
 */
irq_t     *irqs     [CPU_PRIO_COUNT] = { NULL, NULL, NULL, NULL };
closure_t *closures [CPU_PRIO_COUNT] = { NULL, NULL, NULL, NULL };
#if CPU_PRIO_COUNT != 4
//TODO// #  error "Update the number of NULL value initialisations above"
#endif

/* The current priority is an optimisation; it tells what the highest
 * active priority is, so as to avoid too much searching.  The priority
 * starts at the low value CPU_PRIO_ZERO at which nothing runs; as soon
 * as anything is scheduled, the value is increased to signal work is to
 * be done.
 */
volatile priority_t cur_prio = CPU_PRIO_ZERO;


/* Add an IRQ to the round-robin task queue to be scheduled.
 * This routine must always be called with interrupts inactive;
 * either as part of an interrupt routine, or when called in the
 * course of the main program, from within a critical region.
 */
void irq_fire (irq_t *irq) {
	priority_t p = irq->irq_priority;
//TODO// if (irq->irq_next) return;
	if (irqs [p]) {
		irq->irq_next = irqs [p]->irq_next;
		irqs [p]->irq_next = irq;
	} else {
		irq->irq_next = irq;
	}
	irqs [p] = irq;	// Newly added becomes last in the queue
	if (p > cur_prio) {
		cur_prio = p;
	}
//TODO:TEST// ht162x_audiolevel (1 << 5);
}


/* Hop jobs until there is nothing left to do.  This is actually the main
 * procedure for the scheduler.  If it ends, then all the work has been
 * enqueued somewhere, and is awaiting a response.  In other words, it
 * is then time to sleep.  This assumes that no task or IRQ will ever
 * be set to CPU_PRIO_ZERO.
 *
 * This routine triggers sampling of random seed material after having
 * lowered the priority level of the CPU.  This ensures that it is
 * never done while handling anything of high priority.  It also helps
 * to do this after a series of jobs has been fulfilled, as its timing
 * is not likely to be very predictable at that point.
 */
void jobhopper (void) {
void ht162x_putchar (uint8_t idx, uint8_t ch, bool notify);
	//OVERZEALOUSLOGGER// bottom_printf ("Jobhopper starts.\n");
//TODO:TEST// ht162x_putchar (0, '0', true);
	while (cur_prio > CPU_PRIO_ZERO) {
		closure_t *here;
//TODO:TEST// ht162x_putchar (0, '1', true);
		while (irqs [cur_prio]) {
//TODO:TEST// ht162x_putchar (0, '2', true);
			//TODO:JUSTREMOVE// irqs [cur_prio] = irqs [cur_prio]->irq_next;
			while (true) {
				irq_t *prev;
				irq_t *todo;
				bottom_critical_region_begin ();
				prev = irqs [cur_prio];
				if (prev == NULL) {
					bottom_critical_region_end ();
					break;
				}
				todo = prev->irq_next;
				if (prev != todo) {
//TODO:TEST// ht162x_audiolevel (1 << 5);
//TODO:TEST// ht162x_putchar (0, '3', true);
					prev->irq_next = todo->irq_next;
				} else {
//TODO:TEST// ht162x_audiolevel (1 << 5);
//TODO:TEST// ht162x_putchar (0, '4', true);
					irqs [cur_prio] = NULL;
				}
				todo->irq_next = NULL;
				bottom_critical_region_end ();
				//OVERZEALOUSLOGGER// bottom_printf ("Jobhopper calls interrupt handler.\n");
//TODO:TEST// ht162x_audiolevel (2 << 5);
//TODO:NAAAHHH// if (this->irq_handler)
				(*todo->irq_handler) (todo);
//TODO:TEST// ht162x_putchar (0, '5', true);
			}
		}
		here = closures [cur_prio];
		if (here) {
			closures [cur_prio] = here->cls_next;
			here->cls_next = NULL;
			here = (*here->cls_handler) (here);
			if (here) {
				closure_t **last = &closures [cur_prio];
				while (*last != NULL) {
//TODO:TEST// ht162x_putchar (0, '6', true);
					last = &(*last)->cls_next;
				}
//TODO:TEST// ht162x_putchar (0, '7', true);
				bottom_printf ("Jobhopper found closure -- TODO: call it!\n");
				here->cls_next = NULL;
				*last = here;
			}
		} else {
//TODO:TEST// ht162x_putchar (0, '8', true);
			cur_prio--;
			bottom_rndseed ();
		}
	}
//TODO:TEST// ht162x_putchar (0, '9', true);
	//OVERZEALOUSLOGGER// bottom_printf ("Jobhopper ends.\n");
}

