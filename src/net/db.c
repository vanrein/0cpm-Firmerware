/* netdb.c -- Network Database -- Storage for network bindings and addresses.
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
#include <stdarg.h>

// #include <time.h>

// #include <netinet/icmp6.h>

#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/irq.h>
#include <0cpm/timer.h>
#include <0cpm/netinet.h>
#include <0cpm/netfun.h>
#include <0cpm/netdb.h>
#include <0cpm/cons.h>


/* A few useful global variables
 */

extern uint8_t ether_mine [];
extern const uint8_t ether_unknown [];

uint32_t localtime_ofs = 0;

/* The binding table shows all available bindings and their routing */
// TODO: Initialise all binding entries as expired
struct ip4binding ip4binding [IP4BINDING_COUNT];
struct ip6binding ip6binding [IP6BINDING_COUNT];


/* Distortion value, stored in flash, distorting local addresses
 * from the very first attempt.  This is used for /112 allocations
 * over the LAN, in order to avoid clashes.  Once resolved, such
 * clash avoidance should stick until a new clash occurs.  As long
 * as new phones are only plugged in while others are operational,
 * new phones should automatically adapt their /112 accordingly.
 */
uint8_t ifid_distort [8] = { 0, 0, 0, 0, 0, 0, 0, 0 };


/* Use a MAC address build an EUI64 interface identifier in a IPv6 address.
 */
void util_mac2ifid (uint8_t *ip6address, const uint8_t *mac) {
	ip6address [ 8] = mac [0] ^ 0x02;
	ip6address [ 9] = mac [1];
	ip6address [10] = mac [2];
	ip6address [11] = 0xff;
	ip6address [12] = 0xfe;
	ip6address [13] = mac [3];
	ip6address [14] = mac [4];
	ip6address [15] = mac [5];
}


/* When creating a new address, the tentative address must be probed
 * through neighbour discovery, followed by a small-period wait.  This
 * routine is invoked at the end of this waiting period.
 */
void tentative_wait_passed (irq_t *irq) {
	struct ip6binding *bnd = (struct ip6binding *) irq;
	bnd->flags |=  I6B_DEFEND_ME;
	bnd->flags &= ~I6B_TENTATIVE;
	netcore_bootstrap_success ();
	// TODO: Set expiration timer
}


/* An IPv6 Router Advertisement was received.  Process it by storing
 * the offered prefix.
 */
// TODO: Process retractions (life set to 0)
// TODO: Generate ND packet for DAD and start timeout
uint8_t *netdb_router_advertised (uint8_t *pout, intptr_t *mem) {
	struct icmp6_hdr *icmp6;
	uint32_t routerlife;
	uint32_t preferlife;
	uint8_t *options;
	uint8_t *ip6uplink_mac = NULL;
	uint8_t *ip6prefix = NULL;
	int ip6prefixlen = -1;
	int best_pref = -1, cur_pref = -1;
	bool ok = true;
extern uint8_t bad;
	bool needDHCP = true;
	uint8_t ip6address [16];
	uint16_t bindflags;
	int bindidx = 0; // TODO: Actually find a suitable entry to update
bad = 0;
	bottom_printf ("Received & Processing: Router Advertisement\n");
	icmp6 = (struct icmp6_hdr *) mem [MEM_ICMP6_HEAD];
	needDHCP = netget8 (icmp6->icmp6_data8 [1]) >> 7;
	routerlife = (uint32_t) netget16 (icmp6->icmp6_data16 [1]);
	preferlife = (uint32_t) 0xffffffff;
	options = (uint8_t *) (icmp6->icmp6_data8 + 12);
	while (ok && (mem [MEM_ALL_DONE] > (intptr_t) options)) {
		if (((intptr_t) options) + 8 * options [1] > mem [MEM_ALL_DONE]) {
			ok = false;
bad = 101;
			break;
		}
		switch (options [0] + 256 * options [1]) {
		case ND_OPT_SOURCE_LINKADDR + 256:
			ip6uplink_mac = options + 2;
			break;
		case ND_OPT_MTU + 256:
			//TODO// if (ntohl (*(uint32_t *)(options + 4)) < 1280) { ...}
			if (netget32 (*(nint32_t *) (options + 4)) < 1280) {
				// MTU under 1280 is illegal, higher ignored
				ok = false;
bad = 102;
				break;
			}
			break;
		case ND_OPT_PREFIX_INFORMATION + 4*256:
			// How much do we appreciate the offer?  Only the
			// best one continues.
			// Autoconfiguration: 8 + 2 * prefix_pref_if_any
			// DHCPv6: 0
			if (needDHCP) {
				// we prefer autoconfiguration, so DHCPv6 ranks low
				cur_pref = 0;
			} else {
				// preferences according to RFC 4191
				static int preferencemap [4] = { 8, 10, /*reserved:*/ 8, 6 };
				cur_pref = preferencemap [(options [3] >> 3) & 0x03];
			}
			// Accept only /64 or /112 prefixes, prefer /64
			if (options [2] == 64) {
				cur_pref++;
			} else if (options [2] != 112) {
				break;
			}
			// Choose the best available prefix
			if (cur_pref < best_pref) {
				break;
			}
			// Copy settings for this prefix, as it is the best
			//TODO// preferlife = ntohl (*(uint32_t *)(options+8));
			preferlife = netget32 (*(nint32_t *) (options+8));
			ip6prefix = options + 16;
			ip6prefixlen = options [2];
			best_pref = cur_pref;
			break;
		//TODO// ND_OPT_RDNSS
		default:
			//TODO:DONT_PANIC_BUT_BE_GENTLE// ok = false;
//TODO:DONT_PANIC_BUT_BE_GENTLE// bad = 103;
			break;
		}
		options = options + 8 * options [1];
	}
	ok = ok && (mem [MEM_ALL_DONE] == (intptr_t) options);
if ((bad==0) && !ok) bad = 111;
	ok = ok && (best_pref >= 0);
if ((bad==0) && !ok) bad = 112;
	ok = ok && (routerlife > 300);
if ((bad==0) && !ok) bad = 113;
	if (!ok) {
//TODO:TEST// ht162x_led_set (15, 0, true);	// shown as (101)
		return NULL;
	}
bad = best_pref;
	// Now setup the prefix according one of three cases:
	// 0. The only option is to use DHCP6
	// 1. /112 prefix on 6bed4 -> interface-id = 0x0001
	// 2. /64  prefix on LAN   -> interface-id = EIU64(MAC)
	// 3. /112 prefix on LAN   -> interface-id = MAC-based, >= 0x0002
	bindflags = I6B_ROUTE_SOURCE_AUTOCONF | best_pref;
	memcpy (ip6address, ip6prefix, 16);
	if (needDHCP) {
		// Case 0. Must use DHCP6
		// Do not memorise, as DHCP6 is a fallback anyway
bad = 121;
		return NULL;
	} else if (mem [MEM_6BED4_PLOAD] != 0) {
		// Case 1. /112 over 6bed4, interface-id 0x0001
		if (ip6prefixlen != 112) {
			return NULL;	// Oops -- funny prefix, ignore it
		}
		ip6address [14] = 0x00;
		ip6address [15] = 0x01;
		bindflags |= I6B_ROUTE_SOURCE_6BED4_MASK;
	} else if (ip6prefixlen == 112) {
		// Case 2. /112 over LAN, interface-id constructed:
		//         Addresses are random-ish, but replayable
		// Note: Addresses 0x0000 and 0x0001 are usually reserved
		//       for the tunnel server and proxy/client; duplicate
		//       address detection will then handle & randomise
		//       by changing the lan_distort values until okay.
		ip6address [14] = ether_mine [0] ^ ether_mine [1]
		                ^ ether_mine [2] ^ ifid_distort [6];
		ip6address [15] = ether_mine [3] ^ ether_mine [4]
		                ^ ether_mine [5] ^ ifid_distort [7];
		bindflags |= I6B_TENTATIVE;
	} else {
		// Case 3. /64 over LAN, interface-id is EUI64
		util_mac2ifid (ip6address, ether_mine);
		bindflags |= I6B_TENTATIVE;
	}
//TODO:TEST// ht162x_led_set (15, 1, true);	// shown as (101)
	// Now find an entry in the routing table for this tentative
	// address and start up duplicate address detection if needed
	irqtimer_stop (&ip6binding [bindidx].timer); // Just in case
	ip6binding [bindidx].next = NULL;
	bzero (&ip6binding [bindidx], sizeof (ip6binding [bindidx]));
	memcpy (ip6binding [bindidx].ip6addr, ip6address, 16);
	if (ip6uplink_mac != NULL) {
		ip6binding [bindidx].ip6uplink [0] = 0xfe;
		ip6binding [bindidx].ip6uplink [1] = 0x80;
		util_mac2ifid (ip6binding [bindidx].ip6uplink, ip6uplink_mac);
	}
	ip6binding [bindidx].ip6timeout = 0xffffffff;		//TODO
	if (bindflags & I6B_ROUTE_SOURCE_6BED4_MASK) {
		ip6binding [bindidx].ip4binding = &ip4binding[0]; //TODO:mk_dyn
	}
	ip6binding [bindidx].flags = bindflags;
	// Finally, send a packet for duplicate address detection OR report success
	if (bindflags & I6B_TENTATIVE) {
		mem [MEM_BINDING6] = (intptr_t) &ip6binding [bindidx];
		pout = netsend_icmp6_ngb_sol (pout, mem);
		if (pout) {
			irqtimer_start (&ip6binding [bindidx].timer, TIME_MSEC (500), tentative_wait_passed, CPU_PRIO_LOW);
		}
		//TODO: else irqtimer_start (...expiration...)
		return pout;
	} else {
		ip6binding [bindidx].flags |= I6B_DEFEND_ME;
		netcore_bootstrap_success ();
		return NULL;
	}
}

/* A neighbour advertisement was received.  Store it for future reference.
 */
uint8_t *netdb_neighbour_advertised (uint8_t *pout, intptr_t *mem) {
	bottom_printf ("Received: Neighbour Advertisement\n");
	//TODO -- embodyment of this function -- remove tentative, possibly defend territory //
	return NULL;
}

/* DHCPv4 sends an acknowledgement.  Create a ip6binding for that.
 */
uint8_t *netdb_dhcp4_ack (uint8_t *pout, intptr_t *mem) {
	uint8_t *opt;
	static const uint8_t cookie [4] = { 99, 130, 83, 99 };
	int bndidx = 0; //TODO -- select an ip4binding dynamically?
	bottom_printf ("Received: DHCP4 ACK\n");
	opt = (uint8_t *) (mem [MEM_DHCP4_HEAD] + 240);
	if (memcmp (opt - 4, cookie, 4) != 0) {
		return NULL;	/* No options? */
	}
	bzero (&ip4binding [bndidx], sizeof (struct ip4binding));
	ip4binding [bndidx].ip4addr = netget32 (* (nint32_t *) (mem [MEM_DHCP4_HEAD] + 16));
	// Security note: The following settings assume that the DHCP4 ACK
	// contains proper lengths.  If not, the only problem is that bad
	// network addresses are copied, which is not a severe problem.
	while (*opt != 255) {
		switch (*opt) {
		case 1:		/* subnet mask */
			ip4binding [bndidx].ip4mask = netget32 (*(nint32_t *) (opt + 2));
			break;
		case 3:		/* router */
			ip4binding [bndidx].ip4peer [IP4_PEER_GATEWAY] = netget32 (*(nint32_t *) (opt + 2));
			break;
		case 6:		/* DNS server(s) */
			ip4binding [bndidx].ip4peer [IP4_PEER_DNS0] = netget32 (*(nint32_t *) (opt + 2));
			ip4binding [bndidx].ip4peer [IP4_PEER_DNS1] = netget32 (*(nint32_t *) (opt + 2 + opt [1] - 4));
			break;
		case 2:		/* Local time offset */
			localtime_ofs = netget32 (*(nint32_t *) (opt + 2));
			break;
		case 58:	/* Renewal time */
			ip4binding [bndidx].ip4timeout = time (NULL) + netget32 (*(nint32_t *) (opt + 2));
			break;
		case 0:		/* Padding, single byte */
			opt++;
			continue;	// Do not increment the usual way
		case 15:	/* Domain name */
		case 59:	/* Rebinding time */
		case 54:	/* DHCP server identifier */
		case 51:	/* IP address lease time */
		case 35:	/* DHCP message type */
		default:
			break;	// Ignore, and continue with the next option
		}
	
		opt = opt + 2 + opt [1];
	}
	return NULL;
}

/* DHCPv4 sends a negative acknowledgement.  Whatever :)
 */
uint8_t *netdb_dhcp4_nak (uint8_t *pout, intptr_t *mem) {
	bottom_printf ("Received: DHCP4 NAK\n");
	// Drop this, it is non-information to us
	return NULL;
}

/* ARP sends a mapping from IPv4 address to MAC address.
 */
uint8_t *netdb_arp_reply (uint8_t *pkt, intptr_t *mem) {
	struct ether_arp *arp = (struct ether_arp *) mem [MEM_ARP_HEAD];
	int bndidx;
	ip4peer_t peernr;
	uint32_t ip4offer = (uint32_t) mem [MEM_IP4_SRC];
	nint8_t *ip4mac = arp->arp_sha;
	if (memcmp (ip4mac, ether_unknown, ETHER_ADDR_LEN) == 0) {
		return NULL;
	}
	bottom_printf ("Received: ARP\n");
	for (bndidx = 0; bndidx < IP4BINDING_COUNT; bndidx++) {
		for (peernr = 0; peernr < IP4_PEER_COUNT; peernr++) {
			if (ip4binding [bndidx].ip4peer [peernr] == ip4offer) {
				memcpy (ip4binding [bndidx].ip4peermac [peernr], ip4mac, ETHER_ADDR_LEN);
			}
		}
	}
	return NULL;
}

/* DHCPv6 sends an offer; recurse over the options offered
 */
void netdb_dhcp6_recurse_options (nint16_t *dhcp6opts, uint16_t optlen) {
	while (optlen > 4) {
		uint16_t opttag = netget16 (dhcp6opts [0]);
		uint16_t nextlen = 4 + netget16 (dhcp6opts [1]);
		if (nextlen > optlen) {
			break;
		}
		switch (opttag) {
		case 3:		/* OPTION_IA_NA, contains OPTION_IAADDR */
			if (nextlen > 16) {
				netdb_dhcp6_recurse_options (dhcp6opts +  8, optlen - 16);
			}
			break;
		case 4:		/* OPTION_IA_TA, contains OPTION_IAADDR */
			if (nextlen > 8) {
				netdb_dhcp6_recurse_options (dhcp6opts +  4, optlen -  8);
			}
			break;
		case 5:		/* OPTION_IAADDR */
			if (nextlen >= 28) {
				uint8_t bindidx = 0; /*TODO*/
				//TODO: Properly process offer, setup as temporary until Reply
				irqtimer_stop (&ip6binding [bindidx].timer); // Just in case
				bzero (&ip6binding [bindidx], sizeof (ip6binding [bindidx]));
				memcpy (ip6binding [bindidx].ip6addr, dhcp6opts + 2, 16);
				ip6binding [bindidx].ip6timeout = bottom_time () + netget32 (*(nint32_t *) (dhcp6opts+12));
				ip6binding [bindidx].flags = I6B_ROUTE_SOURCE_DHCP | I6B_TENTATIVE;
				netdb_dhcp6_recurse_options (dhcp6opts + 14, optlen - 28);
			}
			break;
		/* TODO: Nameservers */
		/* TODO: Router */
		default:
			break;
		}
		dhcp6opts += nextlen >> 1;
		optlen -= nextlen;
	}
}

/* DHCPv6 sends a reply to confirm allocation of an IPv6 address
 */
uint8_t *netdb_dhcp6_reply (uint8_t *pout, intptr_t *mem) {
	// TODO: Validate offer to be mine
	uint8_t bindidx = 0;	// TODO: Proper binding
	bottom_printf ("Received: DHCP6 REPLY\n");
	ip6binding [bindidx].flags |=  I6B_DEFEND_ME;
	ip6binding [bindidx].flags &= ~I6B_TENTATIVE;
	return NULL;
}

/* DHCPv6 wants to reconfigure our settings
 */
uint8_t *netdb_dhcp6_reconfigure (uint8_t *pout, intptr_t *mem) {
	// TODO: Validate offer to be mine + update configuration data
	bottom_printf ("Received: DHCP6 RECONFIGURE\n");
	return NULL;
}


/* Initialise the networking database after a restart
 */
void netdb_initialise (void) {
	bzero (ip6binding, sizeof (ip6binding));
	bzero (ip4binding, sizeof (ip4binding));
}

