/* bootloader -- TFTP on top of IP4LL to reveal flash contents */

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
