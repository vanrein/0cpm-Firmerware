/* Interrupt drivers
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


#ifndef HEADER_IRQ
#define HEADER_IRQ


/* Interrupt drivers are split in a top half and a bottom half.  The top half is generic,
 * the bottom half is device-specific.  The bottom half routines start with bottom_irq_
 * while the top half functions start with irq_
 *
 * The bottom system is only aware of the hardware interrupts, and whether they have
 * occurred.  The top half is responsible for accepting interrupts from the bottom
 * half, and using them to notify top handlers.
 */


/* A top-half interrupt handler is a plain C function that is called shortly after
 * a bottom-half interrupt has occurred.
 */

typedef struct irq_type irq_t;
typedef void (*irq_handler_t) (irq_t *);

/* An irq_t structure represents an interrupt and how it invokes top-half handlers.
 */
struct irq_type {
	irq_handler_t irq_handler;
	irq_t *irq_next;
	priority_t irq_priority;
};


/* Add an IRQ to the queue of interrupt tasks to be scheduled.
 * This routine must always be called with interrupts inactive;
 * either as part of an interrupt routine, or when called in the
 * course of the main program, from within a critical region.
 */
void irq_fire (irq_t *irq);

/* Top-half operations to manipulate interrupts */


/* Bottom-half operations to manipulate interrupts */
void bottom_irq_start (void);
void bottom_irq_stop (void);
void bottom_irq_enable  (irq_t *irq, irq_handler_t hdl);
void bottom_irq_disable (irq_t *irq);
void bottom_irq_wait (void);


/* Bottom-half operations to process pseudo-random information */
void bottom_rndseed (void);
void bottom_rnd_pseudo (uint8_t *rnd, uint8_t len);


#endif
