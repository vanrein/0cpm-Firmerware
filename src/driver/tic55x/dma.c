/* TIC55x direct memory access
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


/* The tic55x architecture has 6 DMA channels, which will be used
 * as follows:
 *
 *  0 - HIPRI - Read  sound from the current microphone input
 *  1 - HIPRI - Write sound to   the current speaker    output
 *  2 - UNUSE - Read  packet data from the network?  Nope.
 *  3 - UNUSE - Write packet data to   the network?  Nope.
 *  4 - UNUSE
 *  5 - UNUSE
 */

