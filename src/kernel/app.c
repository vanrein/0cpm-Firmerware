/* Application drivers
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

#include <0cpm/app.h>


/* Applications are the parties that scream for resources.
 * They may wish or demand resources.  If they ask for it at
 * the same strength, then the application with the highest
 * resource level wins; if two applications reside at the
 * same resource level, then the latest one started wins.
 * If however, one application has a wish and another has a
 * demand for a resource, and no other resources compete,
 * then the demand wins, regardless of its higher or lower
 * position in the resource ranking.
 */


/* The stack pointer shows the active application; that is,
 * the one that has the highest resource level and/or was
 * started last.  The stack is ordered strictly on these
 * values, so the question who wins a battle over equally
 * desired resources is resolved by finding the first to
 * occur on the stack.
 */

app_t *stackpointer = NULL;


/* Register an application, placing it as high on the stack
 * as possible, given its resource level.
 */
void app_register (app_t *app) {
	app_level_t lvl = app->app_resourcelevel;
	app_t **spp = &stackpointer;
	while ((*spp) && ((*spp)->app_resourcelevel > lvl)) {
		spp = &(*spp)->app_stackdown;
	}
	// TODO: Is this what we want?  Or is this "app_focus"?
	app->app_stackdown = *spp;
	*spp = app;
}

