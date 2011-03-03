/* netcore.c -- Networking core routines.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>
#include <net/ethernet.h>

#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/if_ether.h>

#include <0cpm/netfun.h>
#include <0cpm/netdb.h>


/* Global variables
 */

extern struct tunbuf rbuf, wbuf;	// TODO: Testing setup
extern int tunsox;			// TODO: Testing setup

extern uint8_t ether_mine [ETHER_ADDR_LEN];

/* netcore_checksum_areas (void *area0, uint16_t len0, ..., NULL)
 * Calculate the Internet checksum over the given areas
 */
uint16_t netcore_checksum_areas (void *area0, ...) {
	uint32_t csum = 0;
	uint16_t *area;
	va_list pairs;
	va_start (pairs, area0);
	area = (uint16_t *) area0;
	while (area) {
		uint16_t len = (int16_t) va_arg (pairs, int);
		while (len > 1) {
			csum = csum + ntohs (*area++);
			len -= 2;
		}
		if (len > 0) {
			csum = csum + * (uint8_t *) area;
		}
		area = va_arg (pairs, uint16_t *);
	}
	va_end (pairs);
	csum = (csum & 0xffff) + (csum >> 16);
	csum = (csum & 0xffff) + (csum >> 16);
	return htons ((uint16_t) ~csum);
}


/* Send a buffer with a given MEM[] array of internal pointers.
 * This calculates length and checksum fields, then sends the
 * message into the world.
 */
int netcore_send_buffer (int tunsox, uint32_t *mem, struct tunbuf *wbuf) {
	struct icmp6_hdr *icmp6 = (void *) mem [MEM_ICMP6_HEAD];
	struct icmp      *icmp4 = (void *) mem [MEM_ICMP4_HEAD];
	struct udphdr *udp4 = (void *) mem [MEM_UDP4_HEAD];
	struct udphdr *udp6 = (void *) mem [MEM_UDP6_HEAD];
	struct iphdr   *ip4 = (void *) mem [MEM_IP4_HEAD];
	struct ip6_hdr *ip6 = (void *) mem [MEM_IP6_HEAD];
	struct arphdr  *arp = (void *) mem [MEM_ARP_HEAD];
	struct ethhdr  *eth = (void *) mem [MEM_ETHER_HEAD];
	//
	// Checksum UDPv6 on IPv6
	if (ip6 && udp6) {
		uint16_t pload6 = htons (IPPROTO_UDP);
		udp6->len = htons (mem [MEM_ALL_DONE] - mem [MEM_UDP6_HEAD]);
		ip6->ip6_plen = udp6->len;
		ip6->ip6_nxt = IPPROTO_UDP;
		udp6->check = netcore_checksum_areas (
				&ip6->ip6_src, 32,
				&ip6->ip6_plen, 2,
				&pload6, 2,
				udp6, 6,
				udp6 + 1, (int) htons (udp6->len) - 8,
				NULL);
	}
	//
	// Checksum ICMPv6 on IPv6
	if (ip6 && icmp6) {
		ip6->ip6_nxt = IPPROTO_ICMPV6;
		uint16_t pload6 = htons (IPPROTO_ICMPV6);
		ip6->ip6_plen = htons (mem [MEM_ALL_DONE] - mem [MEM_ICMP6_HEAD]);
		icmp6->icmp6_cksum = 0;
		icmp6->icmp6_cksum = netcore_checksum_areas (
				&ip6->ip6_src, 32,
				&ip6->ip6_plen, 2,
				&pload6, 2,
				icmp6, (int) mem [MEM_ALL_DONE] - (int) mem [MEM_ICMP6_HEAD],
				NULL);
	}
	//
	// Checksum UDPv4 under IPv6 (6bed4 tunnel)
	if (udp4) {
		udp4->len = htons (mem [MEM_ALL_DONE] - mem [MEM_UDP4_HEAD]);
	}
	//
	// Checksum UDPv4 on IPv4
	if (ip4 && udp4) {
		ip4->protocol = IPPROTO_UDP;
		uint16_t pload4 = htons (IPPROTO_UDP);
		ip4->tot_len = htons (20 + ntohs (udp4->len));
		udp4->check = netcore_checksum_areas (
				&ip4->saddr, 8,
				&pload4, 2,
				&udp4->len, 2,
				udp4, 6,
				udp4 + 1, (int) ntohs (udp4->len) - 8,
				NULL);
	}
	//
	// Checksum ICMPv4
	if (icmp4) {
		ip4->tot_len = htons (mem [MEM_ALL_DONE] - mem [MEM_IP4_HEAD]);
		ip4->protocol = IPPROTO_ICMP;
		icmp4->icmp_cksum = 0;
		icmp4->icmp_cksum = netcore_checksum_areas (
				icmp4, (int) mem [MEM_ALL_DONE] - (int) mem [MEM_ICMP4_HEAD],
				NULL);
	}
	//
	// Checksum IPv4
	if (ip4) {
		ip4->check = netcore_checksum_areas (
				ip4, 10,
				&ip4->saddr, 8,
				NULL);
	}
	//
	// Determine the tunnel prefix info and ethernet protocol
	if (ip4) {
		wbuf->prefix.proto = htons (ETH_P_IP);
	} else if (ip6) {
		wbuf->prefix.proto = htons (ETH_P_IPV6);
	} else if (arp) {
		wbuf->prefix.proto = htons (ETH_P_ARP);
	} else {
		return -1;
	}
	eth->h_proto = wbuf->prefix.proto;
	wbuf->prefix.flags = 0;
	//
	// Actually send the packet
	int alen = mem [MEM_ALL_DONE] - mem [MEM_ETHER_HEAD] + sizeof (struct tun_pi);
	if ((alen > 0) && (alen < 1500+12)) {
		int wlen = write (tunsox, wbuf, alen);
printf ("Written %d out of %d:\n", wlen, alen); int i; for (i=0; i<alen; i++) { printf ("%02x%s", ((uint8_t *) wbuf) [i], (i%16 == 0? "\n": " ")); }; printf ("\n");
		if (wlen != alen) {
			if (wlen < 0) {
				return -1;
			} else {
				// No guaranteed delivery -- swallow problem
				return 0;
			}
		}
	}
	return 0;
}


/* Send a router solicitation.
 */
static void solicit_router (void) {
	uint8_t *start = wbuf.data;
	uint32_t mem [MEM_NETVAR_COUNT];
	bzero (mem, sizeof (mem));
	mem [MEM_BINDING6] = (uint32_t) &ip6binding [0];
	uint8_t *stop = netsend_icmp6_router_solicit (start, mem);
	mem [MEM_ALL_DONE] = (uint32_t) stop;
	uint32_t len = mem [MEM_ALL_DONE] - mem [MEM_ETHER_HEAD];
	if ((len > 0) && (len < 1500 + 18)) {
		netcore_send_buffer (tunsox, mem, &wbuf);
	}
}


/* Start to obtain an IPv4 address.
 */
static void get_dhcp4_lease (void) {
	uint8_t *start = wbuf.data;
	uint32_t mem [MEM_NETVAR_COUNT];
	bzero (mem, sizeof (mem));
	uint8_t *stop = netsend_dhcp4_discover (start, mem);
	mem [MEM_ALL_DONE] = (uint32_t) stop;
	uint32_t len = mem [MEM_ALL_DONE] - mem [MEM_ETHER_HEAD];
	if ((len > 0) && (len < 1500 + 18)) {
		netcore_send_buffer (tunsox, mem, &wbuf);
	}
}

/* Start to obtain an IPv6 address.
 */
static void get_dhcp6_lease (void) {
	uint8_t *start = wbuf.data;
	uint32_t mem [MEM_NETVAR_COUNT];
	bzero (mem, sizeof (mem));
	uint8_t *stop = netsend_dhcp6_solicit (start, mem);
	mem [MEM_ALL_DONE] = (uint32_t) stop;
	uint32_t len = mem [MEM_ALL_DONE] - mem [MEM_ETHER_HEAD];
	if ((len > 0) && (len < 1500 + 18)) {
		netcore_send_buffer (tunsox, mem, &wbuf);
	}
}


/* Boot the network: Obtain an IPv6 address, possibly based on an IPv4
 * and a 6bed4 tunnel.  The routine returns success as nonzero.
 *
 * The booting procedure works through the following 1-second steps:
 *  - ensure that the interface is connected to a network
 *  - try IPv6 autoconfiguration a few times (random number)
 *     - first try to reuse an older IPv6 address, even if it was DHCPv6
 *  - try DHCPv6 a few times
 *     - first try to reuse an older IPv6 address, even if it was autoconfig
 *  - try DHCPv4 a few times
 *     - first try to reuse an older IPv4 address
 *     - when an IPv4 address is acquired, autoconfigure 6bed4 over it
 */
int netcore_bootstrap (void) {
	int process_packets (int sox, int secs); //TODO:NOTHERE
	int i;
	int rnd1 = 2 + ((ether_mine [5] & 0x1c) >> 2);
	int rnd2 =       ether_mine [5] & 0x03;
	//
	// TODO: better wipe bindings at physical boot
	bzero (ip4binding, sizeof (ip4binding));
	bzero (ip6binding, sizeof (ip6binding));
	//
	// TODO: ensure physical network connectivity
	//
	// Obtain an IPv6 address through stateless autoconfiguration
	i = rnd1;
	while (i-- > 0) {
		solicit_router ();
		process_packets (tunsox, 2);
		// TODO: return 1 if bound
	}
	//
	// Obtain an IPv6 address through DHCPv6
	i = 3;
	while (i-- > 0) {
		get_dhcp6_lease ();
		process_packets (tunsox, 3);
		// TODO: return 1 if bound
	}
	//
	// Obtain an IPv4 address through DHCPv4
	i = 3;
	while (i-- > 0) {
		get_dhcp4_lease ();
		process_packets (tunsox, rnd2);
		// TODO: break if bound
	}
	// TODO: fail (return 0) if no IPv4 connection
	//
	// Obtain an IPv6 address through 6bed4 over IPv4
	i = rnd1;
	while (i-- > 0) {
		ip6binding [0].ip4binding = &ip4binding [0];
		ip6binding [0].flags |= I6B_ROUTE_SOURCE_6BED4_FLAG;
		solicit_router ();	//TODO: Over 6bed4
		sleep (1);
		// TODO:return 1 if bound
	}
	return 0;
}

