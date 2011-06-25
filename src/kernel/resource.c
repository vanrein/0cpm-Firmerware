/* Resource management.
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


#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include <0cpm/resource.h>


/* Refresh a resource, meaning if it has any output aspects, ask it to
 * refresh that output.  This is used if re-ordering resources may have
 * caused another party to be assigned control over the resource.
 *
 * The refreshed state ought to be idempotent, meaning that it should
 * not change anything if this routine is called more than once.
 *
 * TODO: Flashing LEDs might show clumsy timing if not done properly.
 *       Is idempotence ever really used?
 *
 * Among others, this is done when a resource is claimed, so it is best
 * to initialise state before claiming a resource.  When a resource is
 * released, it (TODO:doc)
 */
static void res_refresh (resource_t *res) {
	claim_t *clm = res->res_claims;
	while (clm && !clm->clm_outputrefresh) {
		clm = clm->clm_next;
	}
	if (clm) {
		(*clm->clm_outputrefresh) (clm);
	}
}

/* Notify a resource about an event.  This is used for input resources,
 * for example upon a key being pressed or a packet arriving over the
 * network link.
 */
void res_notify (resource_t *res, event_t *evt) {
	claim_t *clm = res->res_claims;
	bool todo = (clm != NULL);
	while (todo) {
		if (clm->clm_eventhandler) {
			todo = (*clm->clm_eventhandler) (clm, evt);
		}
	}
}

/* Claim a resource for the application named in it.
 */
void res_claim (resource_t *res, claim_t *clm) {
	app_level_t lvl = clm->clm_app->app_level;
	claim_t **clmptr = &res->res_claims;
	bool refresh = 1;
	while (*clmptr && ((*clmptr)->clm_app->app_level > lvl)) {
		if ((*clmptr)->clm_outputrefresh) {
			refresh = 0;
		}
		clmptr = &(*clmptr)->clm_next;
	}
	clm->clm_next = *clmptr;
	*clmptr = clm;
	if (refresh) {
		res_refresh (res);
	}
}

/* Free a resource.
 */
void res_free (resource_t *res, claim_t *clm) {
	claim_t **clmptr = &res->res_claims;
	bool refresh = 1;
	while (*clmptr && (*clmptr != clm)) {
		if ((*clmptr)->clm_outputrefresh) {
			refresh = 0;
		}
		clmptr = &(*clmptr)->clm_next;
	}
	if (*clmptr == clm) {
		*clmptr = clm->clm_next;
		clm->clm_next = NULL;
	}
	if (refresh) {
		res_refresh (res);
	}
}

