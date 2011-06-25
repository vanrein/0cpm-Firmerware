/* CPU scheduling
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


#ifndef HEADER_CPU
#define HEADER_CPU


/* Depth of sleep is used to make the hardware dive as deeply
 * as practical into a sleep state.  Hardware usually offers
 * a choice of such states, with varying wake-up times.
 * For a phone, two levels seem to matter: one for sleeping
 * lightly, and being able to process realtime traffic as part
 * of an active phone call (SLEEP_SNOOZE); the other for deep
 * sleep, spending as little energy as possible because nothing
 * appears to be going on and an interrupt would be a serious
 * change as it is a sudden call for action (SLEEP_HIBERNATE).
 */
typedef enum sleep_depth_type sleep_depth_t;
enum sleep_depth_type {
	SLEEP_SNOOZE = 0,
	SLEEP_HIBERNATE = 1,
};


/* The priority levels of work (to be) done by the processor
 * varies, and is steered by application levels, realtime
 * requirements and so on.  A number of levels is defined and
 * scheduling of tasks and interrupts is done with a preference
 * for higher levels.
 * TODO: Actually define something sensible.  event, irq levels?
 */
typedef enum priority_type priority_t;
enum priority_type {
	CPU_PRIO_ZERO,		// Never used for closures
	CPU_PRIO_LOW,		// Low    priority closure
	CPU_PRIO_MEDIUM,	// Medium priority closure
	CPU_PRIO_HIGH,		// High   priority closure
	CPU_PRIO_COUNT,		// Number of priority levels
	// Aliases:
	CPU_PRIO_NONE       = CPU_PRIO_ZERO,
	CPU_PRIO_BACKGROUND = CPU_PRIO_LOW,
	CPU_PRIO_UNKNOWN    = CPU_PRIO_MEDIUM,
	CPU_PRIO_NORMAL     = CPU_PRIO_MEDIUM,
	CPU_PRIO_REALTIME   = CPU_PRIO_HIGH,
};


/* Closures are self-contained units of work.  In light of the
 * stateless nature of the phone user experience, these closures
 * do not include a stack but only global state.  They will
 * normally be statically allocated, to further avoid dynamic
 * behaviour that would be too complicated for the user.  This
 * is a serious choice but it seems to make sense for a phone,
 * which is a sort of stimulus-response device.  If it was not
 * for the realtime character and the need to wait for remote
 * actors to respond, the whole phone would have been a single
 * thread without even the need of closures.
 *
 * A closure is usually contained in a local structure holding
 * variables (possibly pointers to shared data structures)
 * which can be accessed by the handling function after
 * invocation.  For the sake of convenience, some common
 * data forms is included with every closure.  Where these
 * suffice, they can save work.
 *
 * Closures contain facilities to queue them for orderly
 * handling; the _next field and _priority field are used for
 * control as a prioritised step in a larger whole of actions.
 *
 * Among others, closures are used as a deferred response to
 * interrupts and timeouts, but they can just as easily be
 * used to respond to certain kinds of incoming traffic on a
 * network port.
 *
 * When the handler function in the closure is called, it is
 * removed from the queue.  Upon return, a new closure may
 * be provided, which will then be enqueued.  This simplifies
 * many practical uses of closures; if forking is needed, a
 * closure may be explicitly registered.
 */
typedef struct closure_type closure_t;
typedef closure_t * (*closure_handler) (closure_t *);
struct closure_type {
	closure_t *cls_next;
	priority_t cls_prio;
	closure_handler cls_handler;
	uint32_t cls_int;	// Utility data
	void *cls_ptr;		// Utility data
};


/* Management functions for dealing with closures */

void cpu_add_closure (closure_t *cls);
void cpu_rm_closure  (closure_t *cls);


/* Top half functions to support driving the CPU */
void top_main (void);

/* Bottom half drivers for processor control */
void bottom_critical_begin (void);
void bottom_critical_end (void);
void bottom_sleep_prepare (void);
void bottom_sleep_commit (sleep_depth_t depth);

#endif
