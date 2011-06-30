/* netcore.c -- Networking core routines.
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
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
// #include <fcntl.h>

//TODO// #include <sys/types.h>
//TODO// #include <sys/ioctl.h>
//TODO// #include <sys/socket.h>

//TODO// #include <netinet/ip.h>
//TODO// #include <netinet/ip6.h>
//TODO// #include <netinet/udp.h>
//TODO// #include <netinet/ip_icmp.h>
//TODO// #include <netinet/icmp6.h>
//TODO// #include <net/ethernet.h>

#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/irq.h>
#include <0cpm/timer.h>
#include <0cpm/netinet.h>
#include <0cpm/netfun.h>
#include <0cpm/netdb.h>
#include <0cpm/cons.h>
#include <0cpm/led.h>	// TODO: only for test purposes, LED_IDX_MESSAGE below

/* Network configuration states */

enum netcfgstate {
	NCS_OFFLINE,	// Await top_network_online()
	NCS_AUTOCONF,	// A few LAN autoconfiguration attempts
	NCS_DHCPV6,	// A few LAN DHCPv6 attempts
	NCS_DHCPV4,	// A few LAN DHCPv4 attempts
	NCS_ARP,	// A few gateway-MAC address fishing attempts
	NCS_6BED4,	// A few 6BED4 attempts over IPv4
	NCS_ONLINE,	// Stable online performance
	NCS_RENEW,	// Stable online, but renewing DHCPv6/v4
	NCS_PANIC,	// Not able to bootstrap anything
	NCS_HAVE_ALT = NCS_6BED4,
};


/* Global variables
 */

/*TODO static*/ enum netcfgstate boot_state   = NCS_OFFLINE;
/*TODO static*/ uint8_t          boot_retries = 1;

/* The timer that waits until a next step is to be taken for
 * bootstrapping the network.  This timer waits half a second
 * between sending router sollicitations, and so on.
 */
static irqtimer_t netboottimer;

/* The timer that waits until the next keepalive interval has
 * occurred, so until a new package has to be sent out far
 * enough to keep the local firewall or NAT open; NAT in case
 * of 6bed4 and firewall in the case of normal IPv6.
 */
static irqtimer_t keepalivetimer;

/*extern*/ packet_t rbuf, wbuf;	// TODO: Testing setup

extern uint8_t ether_mine [ETHER_ADDR_LEN];

const uint8_t ether_unknown [6] = { 0x00, 0x00, 0x0, 0x00, 0x00, 0x00 };


/* Calculate the Internet checksum over the given areas.
 * This is done iteratively, by calling this function for a number
 * of different ranges of words.  The length is in bytes, not in
 * words.  Initialise the checksum to CHECKSUM_ZERO before starting.
 */
#define CHECKSUM_ZERO 0xffff
void netcore_checksum_area (uint16_t *checksum, nint16_t *area, uint16_t len) {
	uint32_t csum = ~ *checksum;
	while (len > 1) {
		csum = csum + netget16 (*area);
		area++;
		len -= 2;
	}
	if (len > 0) {
		csum = csum + netget8 (*(nint8_t *)area);
	}
	csum = (csum & 0xffff) + (csum >> 16);
	csum = (csum & 0xffff) + (csum >> 16);
	*checksum = (uint16_t) ~csum;
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
	bool is_llc = (mem [MEM_LLC_DSAP] != 0) || (mem [MEM_LLC_SSAP] != 0);
	uint16_t wlen;
	//
	// Checksum UDPv6 on IPv6
	if (ip6 && udp6) {
		nint16_t pload6;
		uint16_t plen = mem [MEM_ALL_DONE] - mem [MEM_UDP6_HEAD];
		uint16_t csum = CHECKSUM_ZERO;
		netset16 (pload6, IPPROTO_UDP);
		netset16 (udp6->len,     plen);
		netset16 (ip6->ip6_plen, plen);
		netset8  (ip6->ip6_nxt, IPPROTO_UDP);
		netcore_checksum_area (&csum, (nint16_t *) &ip6->ip6_src, 32);
		netcore_checksum_area (&csum, (nint16_t *) &ip6->ip6_plen, 2);
		netcore_checksum_area (&csum, (nint16_t *) &pload6, 2);
		netcore_checksum_area (&csum, (nint16_t *) udp6, 6);
		netcore_checksum_area (&csum, (nint16_t *) (udp6 + 1), (int) (plen - 8));
		netset16 (udp6->check, csum);
	}
	//
	// Checksum ICMPv6 on IPv6
	if (ip6 && icmp6) {
		nint16_t pload6;
		uint16_t csum = CHECKSUM_ZERO;
		netset8 (ip6->ip6_nxt, IPPROTO_ICMPV6);
		netset16 (pload6, IPPROTO_ICMPV6);
		netset16 (ip6->ip6_plen, mem [MEM_ALL_DONE] - mem [MEM_ICMP6_HEAD]);
		netset16 (icmp6->icmp6_cksum, 0);
		netcore_checksum_area (&csum, (nint16_t *) &ip6->ip6_src, 32);
		netcore_checksum_area (&csum, (nint16_t *) &ip6->ip6_plen, 2);
		netcore_checksum_area (&csum, (nint16_t *) &pload6, 2);
		netcore_checksum_area (&csum, (nint16_t *) icmp6, (int) mem [MEM_ALL_DONE] - (int) mem [MEM_ICMP6_HEAD]);
		netset16 (icmp6->icmp6_cksum, csum);
	}
	//
	// Checksum UDPv4 under IPv6 (6bed4 tunnel)
	if (udp4) {
		netset16 (udp4->len, mem [MEM_ALL_DONE] - mem [MEM_UDP4_HEAD]);
	}
	//
	// Checksum UDPv4 on IPv4
	if (ip4 && udp4) {
		nint16_t pload4;
		uint16_t csum = CHECKSUM_ZERO;
		netset8 (ip4->protocol, IPPROTO_UDP);
		netset16 (pload4, IPPROTO_UDP);
		netset16 (ip4->tot_len, 20 + netget16 (udp4->len));
		netcore_checksum_area (&csum, (nint16_t *) &ip4->saddr, 8);
		netcore_checksum_area (&csum, (nint16_t *) &pload4, 2);
		netcore_checksum_area (&csum, (nint16_t *) &udp4->len, 2);
		netcore_checksum_area (&csum, (nint16_t *) udp4, 6);
		netcore_checksum_area (&csum, (nint16_t *) (udp4 + 1), (int) netget16 (udp4->len) - 8);
		netset16 (udp4->check, csum);
	}
	//
	// Checksum ICMPv4
	if (ip4 && icmp4) {
		uint16_t csum = CHECKSUM_ZERO;
		netset16 (ip4->tot_len, mem [MEM_ALL_DONE] - mem [MEM_IP4_HEAD]);
		netset8 (ip4->protocol, IPPROTO_ICMP);
		netset16 (icmp4->icmp_cksum, 0);
		netcore_checksum_area (&csum, (nint16_t *) icmp4, (int) mem [MEM_ALL_DONE] - (int) mem [MEM_ICMP4_HEAD]);
		netset16 (icmp4->icmp_cksum, csum);
	}
	//
	// Checksum IPv4
	if (ip4) {
		uint16_t csum = CHECKSUM_ZERO;
		netcore_checksum_area (&csum, (nint16_t *) ip4, 10);
		netcore_checksum_area (&csum, (nint16_t *) &ip4->saddr, 8);
		netset16 (ip4->check, csum);
	}
	//
	// Determine the tunnel prefix info and ethernet protocol
	if (ip4) {
		netset16 (eth->h_proto, ETH_P_IP);
	} else if (ip6) {
		netset16 (eth->h_proto, ETH_P_IPV6);
	} else if (arp) {
		netset16 (eth->h_proto, ETH_P_ARP);
	} else if (is_llc) {
		; //TODO// netset16 (eth->h_proto, mem [MEM_ALL_DONE] - mem [MEM_ETHER_HEAD]);
	} else {
		return;
	}
	//
	// Actually have the packet sent
	// TODO: Defer if priority lower than current CPU priority level
	wlen = mem [MEM_ALL_DONE] - mem [MEM_ETHER_HEAD];
	if (!bottom_network_send (wbuf, wlen)) {
		// TODO: Queue packet for future delivery
	}
}


/* Send a router solicitation.  If the binding to use relies
 * on an IPv4 binding, it will run over a 6BED4 tunnel, and
 * be sent to the router instead of broadcasted on the LAN.
 */
static void solicit_router (void) {
	uint8_t *start = wbuf.data;
	intptr_t mem [MEM_NETVAR_COUNT];
	uint8_t *stop;
	uint32_t len;
	bzero (mem, sizeof (mem));
	mem [MEM_BINDING6] = (intptr_t) &ip6binding [0];
	stop = netsend_icmp6_router_solicit (start, mem);
	mem [MEM_ALL_DONE] = (intptr_t) stop;
	len = mem [MEM_ALL_DONE] - mem [MEM_ETHER_HEAD];
	if ((len > 0) && (len < 1500 + 18)) {
		netcore_send_buffer (mem, wbuf.data);
	}
}


/* Start to obtain an IPv4 address.
 */
static void get_dhcp4_lease (void) {
	uint8_t *start = wbuf.data;
	intptr_t mem [MEM_NETVAR_COUNT];
	uint8_t *stop;
	uint32_t len;
	bzero (mem, sizeof (mem));
	stop = netsend_dhcp4_discover (start, mem);
	mem [MEM_ALL_DONE] = (intptr_t) stop;
	len = mem [MEM_ALL_DONE] - mem [MEM_ETHER_HEAD];
	if ((len > 0) && (len < 1500 + 18)) {
		netcore_send_buffer (mem, wbuf.data);
	}
}


/* Start to obtain an IPv6 address.
 */
static void get_dhcp6_lease (void) {
	uint8_t *start = wbuf.data;
	intptr_t mem [MEM_NETVAR_COUNT];
	uint8_t *stop;
	uint32_t len;
	bzero (mem, sizeof (mem));
	stop = netsend_dhcp6_solicit (start, mem);
	mem [MEM_ALL_DONE] = (intptr_t) stop;
	len = mem [MEM_ALL_DONE] - mem [MEM_ETHER_HEAD];
	if ((len > 0) && (len < 1500 + 18)) {
		netcore_send_buffer (mem, wbuf.data);
	}
}


/* Figure out the MAC address of any routers or nameservers
 * that are currently undefined for IPv4.  This may send out
 * more than one query, or none at all.
 */
static void get_local_mac_addresses (void) {
	uint8_t *start;
	intptr_t mem [MEM_NETVAR_COUNT];
	uint8_t *stop;
	uint32_t len;
	int bndidx;
	ip4peer_t peernr;
	for (bndidx = 0; bndidx < IP4BINDING_COUNT; bndidx++) {
		for (peernr = 0; peernr < IP4_PEER_COUNT; peernr++) {
ht162x_putchar (peernr, '0', true);
			if (!ip4binding [bndidx].ip4addr) {
				continue;
			}
			if (!ip4binding [bndidx].ip4mask) {
				continue;
			}
			if ((ip4binding [bndidx].ip4addr ^ ip4binding [bndidx].ip4peer [peernr]) & ip4binding [bndidx].ip4mask) {
				continue;
			}
			if (memcmp (ip4binding [bndidx].ip4peermac [IP4_PEER_GATEWAY], ether_unknown, ETHER_ADDR_LEN) == 0) {
ht162x_putchar (peernr, '1', true);
				bzero (mem, sizeof (mem));
				mem [MEM_IP4_SRC] = ip4binding [bndidx].ip4addr;
				mem [MEM_IP4_DST] = ip4binding [bndidx].ip4peer [peernr];
				start = wbuf.data;
				stop = netsend_arp_query (start, mem);
				mem [MEM_ALL_DONE] = (intptr_t) stop;
				len = mem [MEM_ALL_DONE] - mem [MEM_ETHER_HEAD];
				if ((len > 0) && (len < 1500 + 18)) {
ht162x_putchar (peernr, '2', true);
					netcore_send_buffer (mem, wbuf.data);
ht162x_putchar (peernr, '3', true);
				}
			}
		}
	}
}


/* Is an IPv6 address available?  >= 0 is an index, -1 means no address.
 */
static int have_ipv6 (void) {
	int i;
	for (i=0; i < IP6BINDING_COUNT; i++) {
		if ((ip6binding [i].flags & (I6B_DEFEND_ME | I6B_EXPIRED)) == I6B_DEFEND_ME) {
ht162x_led_set (3, 1, true);	// Shows up as PM
			return i;
		}
	}
ht162x_led_set (3, 0, true);	// Shows up as PM
	return -1;
}

/* Is an IPv4 address available?  >= 0 is an index, -1 means no address.
 */
static int have_ipv4 (void) {
	int i;
	for (i=0; i < IP4BINDING_COUNT; i++) {
		if ((ip4binding [i].ip4addr & ip4binding [i].ip4mask) != 0) {
ht162x_led_set (4, 1, true);
			return i;
		}
	}
ht162x_led_set (4, 0, true);
	return -1;
}

/* Is there an IPv4 gateway available, with a MAC address?  >= 0 is an index, -1 means no address.
 */
static int have_ipv4_gateway_mac (void) {
	int i;
	for (i=0; i < IP4BINDING_COUNT; i++) {
		if (ip4binding [i].ip4addr != 0) {
			if (memcmp (ip4binding [i].ip4peermac [IP4_PEER_GATEWAY], ether_unknown, ETHER_ADDR_LEN) != 0) {
ht162x_led_set (10, 1, true);	// Shows up as an arrow
				return i;
			}
		}
	}
ht162x_led_set (10, 0, true);	// Shows up as an arrow (off)
	return -1;
}

/* Keep a 6bed4 link alive, by sending an empty UDP package each 30 seconds.
 */
static void netcore_keepalive (irq_t *tmr) {
	uint8_t *start;
	intptr_t mem [MEM_NETVAR_COUNT];
	uint8_t *stop;
	uint32_t len;
	int bndidx;
	for (bndidx=0; bndidx < IP6BINDING_COUNT; bndidx++) {
		if (!(ip6binding [bndidx].flags & I6B_DEFEND_ME)) {
			continue;
		}
		if ((ip6binding [bndidx].flags & I6B_ROUTE_SOURCE_6BED4_MASK) && (ip6binding [bndidx].ip4binding)) {
			bzero (mem, sizeof (mem));
			//OK// mem [MEM_UDP6_SRC_PORT] = 0;
			//OK// mem [MEM_UDP6_DST_PORT] = 0;
			mem [MEM_BINDING6] = (intptr_t) &ip6binding [bndidx];
			mem [MEM_IP6_DST] = (intptr_t) ip6binding [bndidx].ip6addr; // Brutal: me!
			mem [MEM_ETHER_DST] = (intptr_t) ip6binding [bndidx].ip4binding->ip4peermac [0];
			stop = netsend_udp6 (start, mem);	// No UDP payload!
			mem [MEM_ALL_DONE] = (intptr_t) stop;
			len = mem [MEM_ALL_DONE] - mem [MEM_ETHER_HEAD];
			if ((len > 0) && (len < 1500 + 18)) {
				netcore_send_buffer (mem, wbuf.data);
			}

		}
	}
	irqtimer_restart ((irqtimer_t *) tmr, TIME_SEC (30));
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
void netcore_bootstrap_step (irq_t *tmrirq) {
	timing_t delay;
	bottom_printf ("Taking a step in network bootstrapping\n");
	delay = 0;
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
		if (have_ipv4 () == -1) {
			get_dhcp4_lease ();
			delay = TIME_MSEC(5000);
			boot_retries = 2;
			break;
		}
		boot_state = NCS_ARP;
		boot_retries = 3;
		// Continue into NCS_ARP for gateway location
	case NCS_ARP:		// A few gateway-MAC address fishing attempts
		if (have_ipv4_gateway_mac () == -1) {
			get_local_mac_addresses ();
			delay = TIME_MSEC(250);
			break;
		}
		boot_state = NCS_6BED4;
		boot_retries = 2;
		// Continue into NCS_6BED4 for 6bed4 autoconf
	case NCS_6BED4:		// A few 6BED4 attempts over IPv4
		if (have_ipv6 () == -1) {
			uint8_t v4bnd = have_ipv4_gateway_mac ();
			if (v4bnd == -1) {
				// Error, danger, panic.  Bail out, try again.
				boot_state = NCS_DHCPV4;
				boot_retries = 2;
				delay = TIME_MSEC(5000);
				break;
			}
			ip6binding [0].ip4binding = &ip4binding [v4bnd];
			ip6binding [0].flags |= I6B_ROUTE_SOURCE_6BED4_MASK;
		}
		// Continue into NCS_AUTOCONF for Router Solicitation
	case NCS_AUTOCONF:	// A few LAN autoconfiguration attempts
		if (have_ipv6 () >= 0) {
			return;
		}
		solicit_router ();
		delay = TIME_MSEC(1000);
		break;
	case NCS_DHCPV6:	// A few LAN DHCPv6 attempts
		if (have_ipv6 () >= 0) {
			return;
		}
		get_dhcp6_lease ();
		delay = TIME_MSEC(1000);
		break;
	case NCS_ONLINE:	// Stable online performance
		return;
	case NCS_RENEW:		// Stable online, but renewing DHCPv6/v4
		// TODO: Inspect timers or resend renewal request
		return;
	case NCS_OFFLINE:	// Await top_network_online()
		break;
	case NCS_PANIC:		// Not able to bootstrap anything
		// delay = TIME_MIN(15);
		// break;
		return;
	default:
		break;
	}
	irqtimer_restart ((irqtimer_t *) tmrirq, delay);
}

/* Initiate the bootstrapping process, returning its closure
 */
void netcore_bootstrap_initiate (void) {
	netdb_initialise ();
	irqtimer_stop (&netboottimer);	// Initially ignored, very useful on restart
	bzero (&netboottimer, sizeof (netboottimer));
	boot_retries = 3 + (ether_mine [5] & 0x03);	// 3..7
	boot_state = NCS_AUTOCONF;
	bottom_printf ("Initiating network bootstrapping procedures\n");
	irqtimer_start (&netboottimer, 0, netcore_bootstrap_step, CPU_PRIO_LOW);
	//TODO// irqtimer_start (&keepalivetimer, TIME_SEC(30), netcore_keepalive, CPU_PRIO_LOW);
}

/* Signal that bootstrapping has reached a successful state
 */
void netcore_bootstrap_success (void) {
	irqtimer_stop (&netboottimer);
	boot_state = NCS_ONLINE;
}

/* Signal that bootstrapping must be restarted (expiration due).
 */
void netcore_bootstrap_restart (void) {
	boot_retries = 3 + (ether_mine [5] & 0x03);	// 3..7
	boot_state = NCS_AUTOCONF;
	irqtimer_start (&netboottimer, 0, netcore_bootstrap_step, CPU_PRIO_LOW);
}

/* Shutdown the bootstrapping process, if any
 */
void netcore_bootstrap_shutdown (void) {
	irqtimer_stop (&netboottimer);
	//TODO// irqtimer_stop (&keepalivetimer);
	boot_retries = 1;
	boot_state   = NCS_OFFLINE;
	netdb_initialise ();
}

