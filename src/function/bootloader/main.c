/* bootloader -- TFTP on top of IP4LL to reveal flash contents
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


#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/net.h>


/* Bootloader IPv4 acquisition -- through IP4LL */
void bootloader_net_ip4ll (void) {
	"TODO: code this function";
}


/* Bootloader announcement of TFTP service -- as DNS-SD, over mDNS */
void bootloader_announce_tftp (void) {
	"TODO: code this function";
}


/* Bootloader network handling -- dealing with TFTP, ICMP and ARP */
void bootloader_handle_net (void) {
	"TODO: code this function";
}


/* Bootloader main program.  This usually starts before the real app.
 *
 * This ends as soon as possible, once the phone is placed on-hook.
 * As a result, a normal phone reboot (with the phone on-hook) will
 * immediately skip bootloading and advance to the actual payload.
 * Also, if a phone is rebooted off-hook and behaves "funny", the
 * user will quickly learn to place the horn back on the hook.
 * In development situations, the horn is usually off-hook as a
 * result of the phone being taken apart.  How practical!
 */
void top_main (void) {
	if (bottom_phone_is_offhook ()) {
		bootloader_net_ip4ll ();
		bootloader_announce_tftp ();
		while (bottom_phone_is_offhook ()) {
			bootloader_handle_net ();
		}
	}
}
