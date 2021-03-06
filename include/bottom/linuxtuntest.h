/* Includes for initial test developments on Linux (using tunnels)
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

typedef uint64_t timing_t;

#define TIMER_NULL 0

#define TIME_MSEC(x)	((x))
#define TIME_SEC(x)	((x)*1000)
#define TIME_MIN(x)	((x)*1000*60)
#define TIME_HOUR(x)	((x)*1000*60*60)
#define TIME_DAY(x)	((x)*1000*60*60*24)
