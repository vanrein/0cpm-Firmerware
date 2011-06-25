/* Resource contention handlers.
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


#ifndef HEADER_APP
#define HEADER_APP


/* Hardware phones vary wildy in their configurations of buttons, screens and LEDs.
 * To handle all phones in the same way would imply that the simplest phone dictated
 * the facilities on all others.  Instead, it is more desirable to have the best
 * possible phone dictate what facilities are possible.  This is indeed possible by
 * exploitation of context sensitivity, generic buttons that can be dynamically
 * mapped to a function and of course the menu that is present on virtually all
 * phones.
 *
 * In cases where resources are really scarce, such as on an analog phone attached
 * to an ATA or a phone with a one-line display of 7-segment digits, another
 * tactic is applied.  Resources then line up according to their priority, which
 * is pretty much the same as their level of urgency.  An incoming call counts as
 * more urgent than a call being setup, and that in turn outranks a clock.  If
 * resources are tight, the resource access priority determines what is shown;
 * on more elaborate displays a layering setup may be shown, and so on.  Much of
 * this is generic resource contention handling, within constraints that depend
 * on the hardware platform.
 *
 * To make this even more interesting, we desire to do most of the work at
 * compile-time, in macros that expand to the best possible approach for a
 * given hardware phone.
 */


/* The app_level_t type defines the various priority levels at which an end user
 * can experience a phone.  Note that this is totally unrelated to realtime
 * priorities, even if both are a sort of prioirity level with override.
 */
typedef enum {
	APP_LEVEL_ZERO,		/* Nothing to do; this is a sleep mode */
	APP_LEVEL_BACKGROUNDED,	/* Calls in progress, but the user detached from them */
	APP_LEVEL_REINVITE,	/* The user is transferring a call, or creating a conference */
	APP_LEVEL_DIALING,	/* The user is dialing, is entering the digits of a number */
	APP_LEVEL_PHONEBOOK,	/* The user is looking through a phonebook */
	APP_LEVEL_CONNECTING,	/* The phone is setting up a connection to another phone */
	APP_LEVEL_TALKING,	/* The user is on the phone, interacting with others */
	APP_LEVEL_ZRTPCHECK,	/* The user is verifying a ZRTP key hash */
	APP_LEVEL_SOFTBUTTON,	/* The user is engaged in a softbutton-initiated dialog */
	APP_LEVEL_MENU,		/* The user is looking through a menu */
	APP_LEVEL_RINGING,	/* A new call is coming into the system */
	APP_LEVEL_COUNT		/* The number of distinguished resource levels */
} app_level_t;



/* Applications exist at each resource level.  Applications are aware
 * when they are being overruled by another application, and they
 * will make a choice whether to stop or wait for later continuation.
 *
 * Applications will lay claims on resources, such as keys and (partial)
 * display area, or the ability to output sound to the user's handset.
 * These claims may cause applications to be put aside, while in other
 * cases the claims can co-exist.  The more I/O facilities one has, the
 * more power one can deploy, basically.
 *
 * It is possible for a number of applications to share the same resource
 * level.  In that case, the one that prevails is the last one started.
 * An application can decide for itself it is will ever resume, or just
 * quit as soon as it is suspended (or decide on it later, when resumed).
 *
 * Resources may be wished or demanded by an application.  If they are
 * wished, then the removal of these keys by higher layers will not cause
 * suspending the application, like with demanded keys.  If a higher layer
 * wishes for resources that are demanded by the lower layer, then the
 * higher layer will not be given these resources.  If higher and lower
 * layer wish a resource, then the higher layer wins.
 */

typedef void (*app_action_t) (void);
typedef void (*app_event_processor_t) (uint16_t);

typedef struct application app_t;
struct application {
	app_t *app_stackdown;
	app_level_t app_resourcelevel;
	struct resource *res_wish;	/* NULL means: No wish for resources */
	struct resource *res_demand;	/* NULL means: No demanded resources */

	app_action_t app_start;
	app_action_t app_stop;
	app_action_t app_suspend;	/* NULL means: use app_stop  */
	app_action_t app_resume;	/* NULL means: use app_start */

	app_event_processor_t app_event;

	app_level_t app_level;

};


/* A routine to register an application structure.  This may only be
 * called once on an application structure.
 */
void app_register (app_t *app);


/* A routine to focus on an application, provided that nothing at a
 * higher resource level wins any demanded resources from this one.
 * This is used to compete with other applications at the same
 * resource level.
 */
void app_focus (app_t *app);


// TODO: API?  Unfocus, unregister?  When to start, how long to claim, and so on?




#endif
