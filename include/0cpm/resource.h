/* Resource framework
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


#ifndef HEADER_RESOURCE
#define HEADER_RESOURCE

/*
 * Phone applications may run in some situations, and not in others.
 * This results in dynamicity of interactions, especially towards humans.
 * The resource framework is setup to support resource claims that depend
 * on the priority of an application.  Applications at a higher priority
 * will be offered a first chance to process events.
 * 
 * A resource forms a queue of resource claims, and each of these claims
 * ties to an application.  The first active application is in charge
 * of a resource.  Only if the first chooses to share the events with
 * lower applications will an event be made visible downstream.  This
 * can be used, for instance, to have a high-priority application shutdown
 * if certain buttons are pressed, the hook is placed back, and so on.
 */


#include <0cpm/app.h>


/* Event priorities define levels of importance for events.  They are
 * defined in resource_t structures, and applied to all events that
 * emenate from that resource.  The lower the class, the later it will
 * be handled.
 */

typedef enum eventpriority {
	EVT_BACKGROUND,
	EVT_USER,
	EVT_NEGOTIATION,
	EVT_REALTIME,
	EVT_IMMEDIATE,
} evtprio_t;


/* Resource claims will be ordered by application priority, and only
 * actually be used if an application is set to running.
 * Input resources send events to clm_eventhandler() and output
 * resources may invoke clm_outputrefresh() when assigned to another
 * application.
 */
typedef struct claim claim_t;
struct claim {
	claim_t *clm_next;
	struct application *clm_app;
	//TODO:event// bool (*clm_eventhandler)  (claim_t *clm, struct event *evt);
	bool (*clm_eventhandler)  (claim_t *clm, void *evt);
	void (*clm_outputrefresh) (claim_t *clm);
};


/* Resources are global structures that can be claimed.  They are known
 * by their name, or their position in an array, and so on.  In general,
 * anything the resource offering party and resource claiming party can
 * agree on will work.
 */
typedef struct resource resource_t;
struct resource {
	struct claim *res_claims;
	evtprio_t res_eventprio;
};


/* Events report the occurrence of resource-related situations to an
 * application.  Liberally included are timer events, which are not
 * quite the same.
 *
 * The description of events is split into event class, instance and
 * value.  For further information an event trigger function is always
 * called with the matching resource claim structure, which may be
 * embedded in a larger structure holding addtional state of use to
 * the event processing function.
 *
 * Event handlers are defined as part of a resource claim, where they
 * are known as clm_eventhandler functions.  Multiple events may
 * occur in relation to a single resource claim.  Events are queued by
 * the scheduler, ordered by the event priority defined in the original
 * resource that triggered it.
 */

typedef struct event {
	struct claim *evt_claim;
	uint8_t evt_class; //TODO// eventclass_t evt_class;
	uint8_t evt_instance;
	uint16_t evt_value;
} event_t;


#endif
