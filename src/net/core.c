/* netcore.c -- Networking core routines.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
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

#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/netfun.h>
#include <0cpm/netdb.h>
#include <0cpm/irq.h>
#include <0cpm/timer.h>


/* Network configuration states */

enum netcfgstate {
	NCS_OFFLINE,	// Await top_network_online()
	NCS_AUTOCONF,	// A few LAN autoconfiguration attempts
	NCS_DHCPV6,	// A few LAN DHCPv6 attempts
	NCS_DHCPV4,	// A few LAN DHCPv4 attempts
	NCS_6BED4,	// A few 6BED4 attempts over IPv4
	NCS_ONLINE,	// Stable online performance
	NCS_RENEW,	// Stable online, but renewing DHCPv6/v4
	NCS_PANIC,	// Not able to bootstrap anything
	NCS_HAVE_ALT = NCS_6BED4,
};


/* Global variables
 */

static enum netcfgstate boot_state   = NCS_OFFLINE;
static uint8_t          boot_retries = 1;

static irqtimer_t netboottimer;

extern packet_t rbuf, wbuf;	// TODO: Testing setup

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
void netcore_send_buffer (intptr_t *mem, uint8_t *wbuf) {
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
		eth->h_proto = htons (ETH_P_IP);
	} else if (ip6) {
		eth->h_proto = htons (ETH_P_IPV6);
	} else if (arp) {
		eth->h_proto = htons (ETH_P_ARP);
	} else {
		return;
	}
	//
	// Actually have the packet sent
	// TODO: Defer if priority lower than current CPU priority level
	uint16_t wlen = mem [MEM_ALL_DONE] - mem [MEM_ETHER_HEAD];
	if (!bottom_network_send (wbuf, wlen)) {
		// TODO: Queue packet for future delivery
	}
}


/* Send a router solicitation.
 */
static void solicit_router (void) {
	uint8_t *start = wbuf.data;
	intptr_t mem [MEM_NETVAR_COUNT];
	bzero (mem, sizeof (mem));
	mem [MEM_BINDING6] = (intptr_t) &ip6binding [0];
	uint8_t *stop = netsend_icmp6_router_solicit (start, mem);
	mem [MEM_ALL_DONE] = (intptr_t) stop;
	uint32_t len = mem [MEM_ALL_DONE] - mem [MEM_ETHER_HEAD];
	if ((len > 0) && (len < 1500 + 18)) {
		netcore_send_buffer (mem, wbuf.data);
	}
}


/* Start to obtain an IPv4 address.
 */
static void get_dhcp4_lease (void) {
	uint8_t *start = wbuf.data;
	intptr_t mem [MEM_NETVAR_COUNT];
	bzero (mem, sizeof (mem));
	uint8_t *stop = netsend_dhcp4_discover (start, mem);
	mem [MEM_ALL_DONE] = (intptr_t) stop;
	uint32_t len = mem [MEM_ALL_DONE] - mem [MEM_ETHER_HEAD];
	if ((len > 0) && (len < 1500 + 18)) {
		netcore_send_buffer (mem, wbuf.data);
	}
}


/* Start to obtain an IPv6 address.
 */
static void get_dhcp6_lease (void) {
	uint8_t *start = wbuf.data;
	intptr_t mem [MEM_NETVAR_COUNT];
	bzero (mem, sizeof (mem));
	uint8_t *stop = netsend_dhcp6_solicit (start, mem);
	mem [MEM_ALL_DONE] = (intptr_t) stop;
	uint32_t len = mem [MEM_ALL_DONE] - mem [MEM_ETHER_HEAD];
	if ((len > 0) && (len < 1500 + 18)) {
		netcore_send_buffer (mem, wbuf.data);
	}
}


/* Is an IPv6 address available?
 */
static bool have_ipv6 (void) {
	int i;
	for (i=0; i < IP6BINDING_COUNT; i++) {
		if (ip6binding [i].flags & (I6B_TENTATIVE | I6B_EXPIRED) == 0) {
			return true;
		}
	}
	return false;
}

/* Is an IPv4 address available?
 */
static bool have_ipv4 (void) {
	int i;
	for (i=0; i < IP4BINDING_COUNT; i++) {
		if (ip4binding [i].ip4addr != 0) {
			return true;
		}
	}
	return false;
}


/* Boot the network: Obtain an IPv6 address, possibly based on an IPv4
 * and a 6bed4 tunnel.  This is implemented as a state diagram made
 * with closures.
 *
 * The booting procedure works through the following 1-second steps:
 *  - ensure that the interface is connected to a network
 *  - try IPv6 autoconfiguration a few times
 *     - first try to reuse an older IPv6 address, even if it was DHCPv6 (TODO)
 *  - try DHCPv6 a few times
 *     - first try to reuse an older IPv6 address, even if it was autoconfig (TODO)
 *  - try DHCPv4 a few times
 *     - first try to reuse an older IPv4 address (TODO)
 *     - when an IPv4 address was acquired, autoconfigure 6bed4 over it
 */
bool netcore_bootstrap_step (irq_t *tmrirq) {
	printf ("Taking a step in network bootstrapping\n");
	timing_t delay = 0;
	if (boot_retries > 0) {
		boot_retries--;
	} else {
		boot_retries = 3;
		if (boot_state <= NCS_HAVE_ALT) {
			boot_state++;
		} else {
			boot_state = NCS_PANIC;
		}
	}
	switch (boot_state) {
	case NCS_DHCPV4:	// A few LAN DHCPv4 attempts
		if (!have_ipv4 ()) {
			get_dhcp4_lease ();
			delay = TIME_MSEC(5000);
			break;
		}
		boot_state = NCS_6BED4;
		boot_retries = 2;
		// Continue into NCS_6BED4 for 6bed4 autoconf
	case NCS_6BED4:		// A few 6BED4 attempts over IPv4
		if (!have_ipv6 ()) {
			ip6binding [0].ip4binding = &ip4binding [0];
			ip6binding [0].flags |= I6B_ROUTE_SOURCE_6BED4_FLAG;
		}
		// Continue into NCS_AUTOCONF for Router Solicitation
	case NCS_AUTOCONF:	// A few LAN autoconfiguration attempts
		if (have_ipv6 ()) {
			return true;
		}
		solicit_router ();
		delay = TIME_MSEC(500);
		break;
	case NCS_DHCPV6:	// A few LAN DHCPv6 attempts
		if (have_ipv6 ()) {
			return true;
		}
		get_dhcp6_lease ();
		delay = TIME_MSEC(1000);
		break;
	case NCS_ONLINE:	// Stable online performance
		return true;
	case NCS_RENEW:		// Stable online, but renewing DHCPv6/v4
		// TODO: Inspect timers or resend renewal request
		return true;
	case NCS_OFFLINE:	// Await top_network_online()
		break;
	case NCS_PANIC:		// Not able to bootstrap anything
		// delay = TIME_MIN(15);
		// break;
		return true;
	default:
		break;
	}
	irqtimer_restart ((irqtimer_t *) tmrirq, delay);
	return true;
}

/* Initiate the bootstrapping process, returning its closure
 */
void netcore_bootstrap_initiate (void) {
	bzero (&netboottimer, sizeof (netboottimer));
	boot_retries = 3 + (ether_mine [5] & 0x03);	// 3..7
	boot_state = NCS_AUTOCONF;
	printf ("Initiating network bootstrapping procedures\n");
	irqtimer_start (&netboottimer, 0, netcore_bootstrap_step, CPU_PRIO_LOW);
}

/* Shutdown the bootstrapping process, if any
 */
void netcore_bootstrap_shutdown (void) {
	boot_retries = 1;
	boot_state   = NCS_OFFLINE;
	irqtimer_stop (&netboottimer);
}

