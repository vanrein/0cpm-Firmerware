/* Blackfin-specific includes
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


/* Below:
 *  - definitions for this architecture to service the top half
 *  - definitions to bind bottom half files together
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


/* The timer is the top 32-bit half of timer0 */
typedef uint32_t timing_t;
#define TIME_BEFORE(x,y)	(((x)-(y)) >> 31)
#define TIME_BEFOREQ(x,y)	(!(TIME_BEFORE((y),(x))))

#define TIMER_NULL 0

#define TIME_MSEC(x)	(((uint32_t) (x)))
#define TIME_SEC(x)	(((uint32_t) (x))*1000)
#define TIME_MIN(x)	(((uint32_t) (x))*1000*60)
#define TIME_HOUR(x)	(((uint32_t) (x))*1000*60*60)
#define TIME_DAY(x)	(((uint32_t) (x))*1000*60*60*24)



