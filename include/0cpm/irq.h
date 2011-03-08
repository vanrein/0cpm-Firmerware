/*
 * http://devel.0cpm.org/ -- Open source firmware for SIP phones.
 *
 * Interrupt drivers
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


/* Manage interrupts; fire one */
void irq_fire (irq_t *irq);

/* Top-half operations to manipulate interrupts */


/* Bottom-half operations to manipulate interrupts */
void bottom_irq_start (void);
void bottom_irq_stop (void);
void bottom_irq_enable  (irq_t *irq, irq_handler_t hdl);
void bottom_irq_disable (irq_t *irq);
void bottom_irq_wait (void);


#endif
