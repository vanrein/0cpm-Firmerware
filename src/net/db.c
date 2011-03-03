/* netdb.c -- Network Database -- Storage for network bindings and addresses.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdint.h>
#include <time.h>

#include <0cpm/netfun.h>
#include <0cpm/netdb.h>

#include <netinet/icmp6.h>


/* A few useful global variables
 */

extern uint8_t ether_mine [];

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


/* An IPv6 Router Advertisement was received.  Process it by storing
 * the offered prefix.
 */
// TODO: Process retractions (life set to 0)
// TODO: Generate ND packet for DAD and start timeout
uint8_t *netdb_router_advertised (uint8_t *pout, uint32_t *mem) {
	printf ("Received & Processing: Router Advertisement\n");
	struct icmp6_hdr *icmp6 = (struct icmp6_hdr *) mem [MEM_ICMP6_HEAD];
	uint32_t routerlife = (uint32_t) icmp6->icmp6_data16 [1];
	uint32_t preferlife = (uint32_t) 0xffffffff;
	uint8_t *options = icmp6->icmp6_data8 + 12;
	uint8_t *ip6uplink_mac = NULL;
	uint8_t *ip6prefix = NULL;
	int ip6prefixlen = -1;
	int best_pref = -1, cur_pref = -1;
	int ok = 1;
	int needDHCP = 0;
	while (ok && (mem [MEM_ALL_DONE] > (uint32_t) options)) {
		if (((uint32_t) options) + 8 * options [1] > mem [MEM_ALL_DONE]) {
			ok = 0;
			break;
		}
		switch (options [0] + 256 * options [1]) {
		case ND_OPT_SOURCE_LINKADDR + 256:
			ip6uplink_mac = options + 2;
			break;
		case ND_OPT_MTU + 256:
			if (ntohl (*(uint32_t *)(options + 4)) < 1280) {
				// MTU under 1280 is illegal, higher ignored
				ok = 0;
				break;
			}
			break;
		case ND_OPT_PREFIX_INFORMATION + 4*256:
			// How much do we appreciate the offer?  Only the
			// best one continues.
			// Autoconfiguration: 8 + 2 * prefix_pref_if_any
			// DHCPv6: 0
			if (options [3] & 0x40) {
				cur_pref = 8;
				needDHCP = 0;
			} else {
				cur_pref = 0;
				if (best_pref == -1) {
					needDHCP = 1;
				}
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
			preferlife = ntohl (*(uint32_t *)(options+8));
			ip6prefix = options + 16;
			ip6prefixlen = options [2];
			best_pref = cur_pref;
			break;
		default:
			ok = 0;
			break;
		}
		options = options + 8 * options [1];
	}
	ok = ok && (mem [MEM_ALL_DONE] == (uint32_t) options);
	ok = ok && (best_pref >= 0);
	if (!ok) {
		return NULL;
	}
	// Now setup the prefix according one of three cases:
	// 0. The only option is to use DHCP6
	// 1. /112 prefix on 6bed4 -> interface-id = 0x0001
	// 2. /64  prefix on LAN   -> interface-id = EIU64(MAC)
	// 3. /112 prefix on LAN   -> interface-id = MAC-based, >= 0x0002
	uint8_t ip6address [16];
	uint16_t bindflags = I6B_ROUTE_SOURCE_AUTOCONF | best_pref;
	memcpy (ip6address, ip6prefix, 16);
	if (needDHCP) {
		// Case 0. Must use DHCP6
		// Do not memorise, as DHCP6 is a fallback anyway
		return NULL;
	} else if (mem [MEM_6BED4_PLOAD] != 0) {
		// Case 1. /112 over 6bed4, interface-id 0x0001
		if (ip6prefixlen != 112) {
			return NULL;	// Oops -- funny prefix, ignore it
		}
		ip6address [14] = 0x00;
		ip6address [15] = 0x01;
		bindflags |= I6B_ROUTE_SOURCE_6BED4_FLAG;
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
	// Now find an entry in the routing table for this tentative
	// address and start up duplicate address detection if needed
	int bindidx = 0; // TODO: Actually find a suitable entry to update
	ip6binding [bindidx].next = NULL;
	bzero (&ip6binding [bindidx], sizeof (ip6binding [bindidx]));
	memcpy (ip6binding [bindidx].ip6addr, ip6address, 16);
	if (ip6uplink_mac != NULL) {
		ip6binding [bindidx].ip6uplink [0] = 0xfe;
		ip6binding [bindidx].ip6uplink [1] = 0x80;
		util_mac2ifid (ip6binding [bindidx].ip6uplink, ip6uplink_mac);
	}
	ip6binding [bindidx].ip6timeout = 0xffffffff;		//TODO
	if (bindflags & I6B_ROUTE_SOURCE_6BED4_FLAG) {
		//TODO:OLD// ip6binding [bindidx].ip4timeout = 0xffffffff;	//TODO
		//TODO:OLD// ip6binding [bindidx].ip4uplink = ip4gateway;
		//TODO:OLD// ip6binding [bindidx].ip4addr = htonl (mem [MEM_IP4_DST]);
		//TODO:OLD// ip6binding [bindidx].ip4port = htons (mem [MEM_UDP4_PORTS] & 0xffff);
		ip6binding [bindidx].ip4binding = &ip4binding[0]; //TODO:mk_dyn
	}
	ip6binding [bindidx].flags = bindflags;
	// Finally, if needed, send a packet for duplicate address detection
	if (ip6binding [bindidx].flags & I6B_TENTATIVE) {
		mem [MEM_BINDING6] = (uint32_t) &ip6binding [bindidx];
		return netsend_icmp6_ngb_sol (pout, mem);
	} else {
		return NULL;
	}
}

/* A neighbour advertisement was received.  Store it for future reference.
 */
uint8_t *netdb_neighbour_advertised (uint8_t *pout, uint32_t *mem) {
	printf ("Received: Neighbour Advertisement\n");
	//TODO -- embodyment of this function//
	return NULL;
}

/* DHCPv4 sends an acknowledgement.  Create a ip6binding for that.
 */
uint8_t *netdb_dhcp4_ack (uint8_t *pout, uint32_t *mem) {
	printf ("Received: DHCP4 ACK\n");
	uint8_t *opt = (uint8_t *) (mem [MEM_DHCP4_HEAD] + 240);
	static const uint8_t cookie [4] = { 99, 130, 83, 99 };
	if (memcmp (opt - 4, cookie, 4) != 0) {
		return NULL;	/* No options? */
	}
	//TODO -- select an ip4binding dynamically?
	int bndidx = 0;
	bzero (&ip4binding [bndidx], sizeof (struct ip4binding));
	ip4binding [bndidx].ip4addr = htonl (* (uint32_t *) (mem [MEM_DHCP4_HEAD] + 16));
	// Security note: The following settings assume that the DHCP4 ACK
	// contains proper lengths.  If not, the only problem is that bad
	// network addresses are copied, which is not a severe problem.
	while (*opt != 255) {
		switch (*opt) {
		case 1:		/* subnet mask */
			ip4binding [bndidx].ip4mask = * (uint32_t *) (opt + 2);
			break;
		case 3:		/* router */
			ip4binding [bndidx].ip4gateway = * (uint32_t *) (opt + 2);
			break;
		case 6:		/* DNS server(s) */
			ip4binding [bndidx].ip4dns [0] = * (uint32_t *) (opt + 2);
			ip4binding [bndidx].ip4dns [1] = * (uint32_t *) (opt + 2 + opt [1] - 4);
			break;
		case 2:		/* Local time offset */
			localtime_ofs = ntohl (* (uint32_t *) (opt + 2));
			break;
		case 58:	/* Renewal time */
			ip4binding [bndidx].ip4timeout = time (NULL) + ntohl (* (uint32_t *) (opt + 2));
			break;
		case 0:		/* Padding, single byte */
			opt++;
			continue;	// Do not increment the usual way
		case 15:	/* Domain name */
		case 59:	/* Rebinding time */
		case 54:	/* DHCP server identifier */
		case 51:	/* IP address lease time */
		default:
			break;
		}
	
		opt = opt + 2 + opt [1];
	}
	return NULL;
}

/* DHCPv4 sends a negative acknowledgement.  Whatever :)
 */
uint8_t *netdb_dhcp4_nak (uint8_t *pout, uint32_t *mem) {
	printf ("Received: DHCP4 NAK\n");
	// Drop this, it is non-information to us
	return NULL;
}

/* DHCPv6 sends a reply to confirm allocation of an IPv6 address
 */
uint8_t *netdb_dhcp6_reply (uint8_t *pout, uint32_t *mem) {
	// TODO: Validate offer to be mine + store configuration data
	printf ("Received: DHCP6 REPLY\n");
	return NULL;
}

/* DHCPv6 wants to reconfigure our settings
 */
uint8_t *netdb_dhcp6_reconfigure (uint8_t *pout, uint32_t *mem) {
	// TODO: Validate offer to be mine + update configuration data
	printf ("Received: DHCP6 RECONFIGURE\n");
	return NULL;
}

