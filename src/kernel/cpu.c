/*
 * http://devel.0cpm.org/ -- Open source firmware for SIP phones.
 *
 * CPU drivers
 */


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <0cpm/cpu.h>
#include <0cpm/irq.h>


/* Round-robin queues with interrupts and closures, ordered by priority.
 * To be fair, the elements pointed at have already been completed
 * and are awaiting roll-over or removal.
 */
irq_t     *irqs     [CPU_PRIO_COUNT];
closure_t *closures [CPU_PRIO_COUNT];

/* The current priority is an optimisation; it tells what the highest
 * active priority is, so as to avoid too much searching.  The priority
 * starts at the low value CPU_PRIO_ZERO at which nothing runs; as soon
 * as anything is scheduled, the value is increased to signal work is to
 * be done.
 */
priority_t cur_prio = CPU_PRIO_ZERO;


/* Add an IRQ to the round-robin task queue to be scheduled */
void irq_fire (irq_t *irq) {
	priority_t p = irq->irq_priority;
	if (p > cur_prio) {
		cur_prio = p;
	}
	irq->irq_next = irqs [p]? irqs [p]: irq;
	irqs [p] = irq;
	irqs [p] = irqs [p]->irq_next;
}


/* Hop jobs until there is nothing left to do.  This is actually the main
 * procedure for the scheduler.  If it ends, then all the work has been
 * enqueued somewhere, and is awaiting a response.  In other words, it
 * is then time to sleep.  This assumes that no task or IRQ will ever
 * be set to CPU_PRIO_ZERO.
 */
void jobhopper (void) {
	printf ("Jobhopper starts.\n");
	while (cur_prio > CPU_PRIO_ZERO) {
		while (irqs [cur_prio]) {
			irq_t *this;
			irqs [cur_prio] = irqs [cur_prio]->irq_next;
			while (this = irqs [cur_prio], this != NULL) {
				if (this != this->irq_next) {
					irqs [cur_prio] = this->irq_next;
				} else {
					irqs [cur_prio] = NULL;
				}
				this->irq_next = NULL;
				printf ("Jobhopper calls interrupt handler.\n");
				(*this->irq_handler) (this);
			}
		}
		closure_t *here = closures [cur_prio];
		if (here) {
			closures [cur_prio] = here->cls_next;
			here->cls_next = NULL;
			here = (*here->cls_handler) (here);
			if (here) {
				closure_t **last = &closures [cur_prio];
				while (*last != NULL) {
					last = &(*last)->cls_next;
				}
				printf ("Jobhopper found closure -- TODO: call it!\n");
				here->cls_next = NULL;
				*last = here;
			}
		} else {
			cur_prio--;
		}
	}
	printf ("Jobhopper ends.\n");
}

