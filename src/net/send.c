/* netsend.c -- Initiatiating interactions by composing new packets
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


/* This set of routines is used to build network packets from scratch.
 * Only really small actions will be handled by immediate netreply_
 * routines, but many others will need some internal processing first.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <string.h>

// #include <netinet/ip.h>
// #include <netinet/udp.h>
// #include <netinet/ip6.h>
// #include <netinet/icmp6.h>
// #include <netinet/if_ether.h>

#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/irq.h>
#include <0cpm/timer.h>
#include <0cpm/netinet.h>
#include <0cpm/netfun.h>
#include <0cpm/netdb.h>



/* A few well-known addresses to look for
 */

extern uint16_t bootsecs;

extern uint8_t ether_mine [];

extern uint32_t ip4_6bed4;

const uint8_t ether_broadcast [ETHER_ADDR_LEN] = {
				0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

uint8_t ether_multicast_all_dhcp6_servers [ETHER_ADDR_LEN] = {
				0x33, 0x33, 0x00, 0x01, 0x00, 0x02 };

uint8_t linklocal_mine [] = {
	0xfe, 0x80, 0, 0, 0, 0, 0, 0,
	//TODO:MAC_FROM_BOTTOM// 0x13, 0x22, 0x33, 0xff, 0xfe, 0x44, 0x55, 0x66 };
	0x02, 0x0b, 0x82, 0xff, 0xfe, 0x19, 0xa0, 0xf4 };

uint8_t allnodes_linklocal_address [] = {
        0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00, 0x01 };

uint8_t allrouters_linklocal_address [] = {
        0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00, 0x02 };

struct ip6binding binding_linklocal [1] = { {
	{ { NULL, NULL, 0 }, NULL, TIME_MSEC(0) },
	NULL,
	// 2 lines -- copy of linklocal_mine above:
	{ 0xfe, 0x80, 0, 0, 0, 0, 0, 0,
	//TODO:MAC_FROM_BOTTOM//   0x13, 0x22, 0x33, 0xff, 0xfe, 0x44, 0x55, 0x66 },
	  0x02, 0x0b, 0x82, 0xff, 0xfe, 0x19, 0xa0, 0xf4 },
	{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
	0xffffffff,
	NULL,
	I6B_ROUTE_SOURCE_STATIC },
};

uint8_t ip6_multicast_all_hosts [16] = {
	0xff, 0x02, 0,0,0,0,0,0,0,0,0,0, 0x00,0x00, 0x00,0x01 };

uint8_t ip6_multicast_all_dhcp6_servers [16] = {
	0xff, 0x02, 0,0,0,0,0,0,0,0,0,0, 0x00,0x01, 0x00,0x02 };

uint8_t ipv6_router_solicitation [] = {
        // IPv6 header
        0x60, 0x00, 0x00, 0x00,
        16 / 256, 16 % 256, IPPROTO_ICMPV6, 255,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,          // unspecd src
        0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x02, // all-rtr tgt
        // ICMPv6 header: router solicitation
        ND_ROUTER_SOLICIT, 0, 0x7a, 0xae,       // Checksum from WireShark :)
        // ICMPv6 body: reserved
        0, 0, 0, 0,
        // ICMPv6 option: source link layer address 0x0001 (end-aligned)
        0x01, 0x01, 0, 0, 0, 0, 0x00, 0x01,
};




/******************* HEADERVEL CREATION UTILITIES *******************/


/* Create an Ethernet header.
 * Some fields are filled later: h_proto
 * TODO: When to use the broadcast address?
 */
uint8_t *netsend_ether (uint8_t *pout, intptr_t *mem) {
	struct ethhdr *eth = (struct ethhdr *) pout;
	memcpy (eth->h_source, ether_mine, ETHER_ADDR_LEN);
	memcpy (eth->h_dest, (void *) mem [MEM_ETHER_DST], ETHER_ADDR_LEN);
	if (mem [MEM_VLAN_ID] != 0) {
		// TODO: Setup VLAN with mem [MEM_VLAN] number
	}
	mem [MEM_ETHER_HEAD] = (intptr_t) eth;
	return pout + sizeof (struct ethhdr);
}

/* Create an IPv4 header.
 * This may be used for local protocols (DNS, DHCPv4) or as a 6bed4 carrier.
 * Some IPv4 fields are filled later: protocol, tot_len, check.
 */
uint8_t *netsend_ip4 (uint8_t *pout, intptr_t *mem) {
	struct iphdr *ip4;
	pout = netsend_ether (pout, mem);
	ip4 = (struct iphdr *) pout;
	bzero (ip4, sizeof (struct iphdr));
	netset8  (ip4->version_ihl, 0x45);
	netset8  (ip4->ttl, 64);
	netset16 (ip4->frag_off, 0x4000);	// Don't fragment
	netset32 (ip4->saddr, mem [MEM_IP4_SRC]);
	netset32 (ip4->daddr, mem [MEM_IP4_DST]);
	mem [MEM_IP4_HEAD] = (uint32_t) ip4;
	return &pout [sizeof (struct iphdr)];
}

/* Send an UDPv4 header.
 * Some fields are filled later: len, check.
 */
uint8_t *netsend_udp4 (uint8_t *pout, intptr_t *mem) {
	struct udphdr *udp;
	// TODO: Always setup MEM_ETHER_DST with router's address?
	// struct ip6binding *bnd = (struct ip6binding *) mem [MEM_BINDING6];
	// mem [MEM_ETHER_DST] = (intptr_t) bnd->ip4binding->ip4peermac [0];
	pout = netsend_ip4 (pout, mem);
	udp = (struct udphdr *) pout;
	netset16 (udp->source, mem [MEM_UDP4_SRC_PORT]);
	netset16 (udp->dest  , mem [MEM_UDP4_DST_PORT]);
	mem [MEM_UDP4_HEAD] = (intptr_t) udp;
	return &pout [sizeof (struct udphdr)];
}

/* Send an UDPv4 header for 6bed4.
 * Some fields are filled later: len, check.
 */
uint8_t *netsend_udp4_6bed4 (uint8_t *pout, intptr_t *mem) {
	struct ip6binding *bnd = (struct ip6binding *) mem [MEM_BINDING6];
	struct udphdr *udp;
	mem [MEM_IP4_SRC] = bnd->ip4binding->ip4addr;
	mem [MEM_IP4_DST] = ip4_6bed4;
	mem [MEM_ETHER_DST] = (intptr_t) bnd->ip4binding->ip4peermac [0];
	pout = netsend_ip4 (pout, mem);
	udp = (struct udphdr *) pout;
	//TODO:dyn.ports?netset16?// udp->source = htons (bnd->ip4binding->ip4port);
	netset16 (udp->source, 3653);
	netset16 (udp->dest  , 3653);
	mem [MEM_UDP4_HEAD] = (intptr_t) udp;
	mem [MEM_6BED4_PLOAD] = (intptr_t) (pout + sizeof (struct udphdr));
	return (uint8_t *) mem [MEM_6BED4_PLOAD];
}

/* Send an IPv6 header, and wrap it in 6bed4 if needed.
 * Some fields are filled later: plen, nxt.
 */
uint8_t *netsend_ip6 (uint8_t *pout, intptr_t *mem) {
	struct ip6binding *bnd = (struct ip6binding *) mem [MEM_BINDING6];
	struct ip6_hdr *ip6;
	if (bnd->flags & I6B_ROUTE_SOURCE_6BED4_MASK) {
		// Use 6bed4 for destination address
		pout = netsend_udp4_6bed4 (pout, mem);
	} else {
		// Use LAN for destination address
		// TODO: Setup MEM_ETHER_DST with router's address?
		pout = netsend_ether (pout, mem);
	}
	ip6 = (struct ip6_hdr *) pout;
	netset32 (ip6->ip6_flow, 0x60000000);
	netset8   (ip6->ip6_hlim, 64);
	memcpy (&ip6->ip6_src, bnd->ip6addr, 16);
	memcpy (&ip6->ip6_dst, (uint8_t *) mem [MEM_IP6_DST], 16);
	mem [MEM_IP6_HEAD] = (intptr_t) ip6;
	return &pout [sizeof (struct ip6_hdr)];
}

/* Send an UDP6 header over IPv6.
 * Some fields are filled later: len, check.
 */
uint8_t *netsend_udp6 (uint8_t *pout, intptr_t *mem) {
	struct udphdr *udp;
	// TODO: Setup MEM_ETHER_DST with router's address?
	pout = netsend_ip6 (pout, mem);
	udp = (struct udphdr *) pout;
	netset16 (udp->source, mem [MEM_UDP6_SRC_PORT]);
	netset16 (udp->dest  , mem [MEM_UDP6_DST_PORT]);
	mem [MEM_UDP6_HEAD] = (intptr_t) udp;
	return &pout [sizeof (struct udphdr)];
}



/******************* APPLICATION LEVEL ROUTINES *******************/


/* Create an ARP query for a local IPv4 address.
 * The reply address for this host is in MEM_IP4_SRC.
 * The address to send to make the request for is in MEM_IP4_DST.
 * TODO: Consider refreshes sent only to the known MAC address.
 */
uint8_t *netsend_arp_query (uint8_t *pout, intptr_t *mem) {
	struct ether_arp *arpout;
	mem [MEM_ETHER_DST] = (intptr_t) ether_broadcast;
	arpout = (struct ether_arp *) netsend_ether (pout, mem);
	netset16 (arpout->ea_hdr.ar_hrd, ARPHRD_ETHER);
	netset16 (arpout->ea_hdr.ar_pro, ETHERTYPE_IP);
	netset8  (arpout->ea_hdr.ar_hln, ETHER_ADDR_LEN);
	netset8  (arpout->ea_hdr.ar_pln, 4);	// IPv4 len
	netset16 (arpout->ea_hdr.ar_op, 1);	// ARP request
	memcpy   (arpout->arp_sha, ether_mine, ETHER_ADDR_LEN);
	netset32 (*(nint32_t *)arpout->arp_spa, mem [MEM_IP4_SRC]);
	bzero    (arpout->arp_tha, ETHER_ADDR_LEN);
	netset32 (*(nint32_t *)arpout->arp_tpa, mem [MEM_IP4_DST]);
	pout = (uint8_t *) (arpout + 1);
	mem [MEM_ARP_HEAD] = (intptr_t) pout;
	return pout;
}


/* Create an ICMPv6 Router Solicitation.
 */
uint8_t *netsend_icmp6_router_solicit (uint8_t *pout, intptr_t *mem) {
	struct ip6binding *bnd;
	mem [MEM_ETHER_DST] = (intptr_t) ether_broadcast;
	bnd = (struct ip6binding *) mem [MEM_BINDING6];
	if (bnd->flags & I6B_ROUTE_SOURCE_6BED4_MASK) {
		// Use 6bed4 for destination address
		pout = netsend_udp4_6bed4 (pout, mem);
	} else {
		// Use LAN for destination address
		pout = netsend_ether (pout, mem);
	}
	memcpy (pout, ipv6_router_solicitation, sizeof (ipv6_router_solicitation));
	memcpy (pout + 40 + 8 + 2, ether_mine, ETHER_ADDR_LEN);
	memcpy (pout + 8, linklocal_mine, 16);
	//ADDS_NOTHING// memcpy (pout + 8, linklocal_mine, 16);
	mem [MEM_IP6_HEAD] = (intptr_t) pout;
	mem [MEM_ICMP6_HEAD] = (intptr_t) pout + 40;
	return pout + sizeof (ipv6_router_solicitation);
}

/* Send a Neighbour Solicitation, possibly as part of autoconfiguration.
 * Some fields are filled later: icmp6_cksum
 * TODO: Set multicast target address for IPv6
 */
uint8_t *netsend_icmp6_ngb_sol (uint8_t *pout, intptr_t *mem) {
	struct icmp6_hdr *icmp6;
	struct ip6binding *bnd;
	mem [MEM_ETHER_DST] = (intptr_t) ether_broadcast;
	mem [MEM_IP6_DST] = (intptr_t) ip6_multicast_all_hosts;
	pout = netsend_ip6 (pout, mem);
	icmp6 = (struct icmp6_hdr *) pout;
	bnd = (struct ip6binding *) mem [MEM_BINDING6];
	mem [MEM_ICMP6_HEAD] = (intptr_t) pout;
	netset8  (icmp6->icmp6_type, ND_NEIGHBOR_SOLICIT);
	netset8  (icmp6->icmp6_code, 0);
	netset32 (icmp6->icmp6_data32 [0], 0);
	memcpy (icmp6->icmp6_data8 + 4, bnd->ip6addr, 16);
	netset8 (icmp6->icmp6_data8 [4+16+0], 1);
	netset8 (icmp6->icmp6_data8 [4+16+1], 1);
	memcpy (icmp6->icmp6_data8 + 4+16+2, ether_mine, 6);
	return pout + 24 + 8;
}

/* Send a DHCPv4 DISCOVER packet to initiatie IPv4 LAN configuration.
 */
uint8_t *netsend_dhcp4_discover (uint8_t *pout, intptr_t *mem) {
	uint8_t *popt;
	static const uint8_t dhcp4_options [] = {
		99, 130, 83, 99,	// Magic cookie, RFC 1497
		53, 1, 1,		// DHCP message type DISCOVER
		55, 4, 1, 3, 42, 2,	// Param Request List:
					// mask, router, ntp?, time offset?.
		255			// End Option
	};
	mem [MEM_UDP4_SRC_PORT] = 68;
	mem [MEM_UDP4_DST_PORT] = 67;
	mem [MEM_IP4_DST] = 0xffffffff;
	mem [MEM_ETHER_DST] = (intptr_t) ether_broadcast;
	pout = netsend_udp4 (pout, mem);
	bzero (pout, 576);	// erase the acceptable package size
	pout [0] = 1;		// bootrequest
	pout [1] = 1;		// ARP hardware address type
	pout [2] = 6;		// ARP hardware address length
	pout [3] = 0;		// hops
	memcpy (pout + 4, ether_mine + 2, 4);		// client-randomiser
	*(uint16_t *) (pout +  8) = htons (bootsecs++);	// 0 or seconds trying -- TODO: netset16?
	// flags=0, no broadcast reply needed
	// ciaddr [4] is 0.0.0.0, the initial client address
	// yiaddr [4] is 0.0.0.0, the "your" address
	// siaddr [4] is 0.0.0.0, the server address is returned
	// giaddr [4] is 0.0.0.0, gateway address is not set by client
	memcpy (pout + 28, ether_mine, 6);	// client hw addr
	// sname [64], the server hostname is empty
	// file [128], the boot filename, is empty
	// options
	popt = pout + 236;
	memcpy (popt, dhcp4_options, sizeof (dhcp4_options));
	return popt + sizeof (dhcp4_options);
}

/* Send a DHCPv6 SOLICIT packet to initiatie native IPv6 configuration.
 */
uint8_t *netsend_dhcp6_solicit (uint8_t *pout, intptr_t *mem) {
	static const uint8_t dhcp6_options [] = {
		0,1, 0,4+6,	// Client ID, ether @ 4+4 (below), etherlen 6
			0, 3, 0, 1, 0,0,0,0,0,0,
		0,18, 0,6,	// Interface ID, ether @4+4+6+4 below, len 6
			0,0,0,0,0,0,
		0,6, 0,8,	// Option Request
			0,23, 0,22, 0,56, 0,141,
				// DNS resolver, SIP proxy, NTP, SIP UAcfg
		0,3, 0,40,	// Identity Association, non-temporary
			0,0,0,0,		// IAID = 0x00000000
			0x00,0x09,0x3a,0x80,	// Refresh: 7d
			0x00,0x28,0xde,0x80,	// Extend:  31d
			0,5, 0,24,		// IdAssoc Address
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0x00,0x28,0xde,0x80,	// Preferred: 31d
			0x00,0x28,0xde,0x80,	// Valid: 31d
		// 0,14, 0,0,	// Rapid Commit -- IMPOSSIBLE with IP assignmt
	};
	mem [MEM_UDP6_SRC_PORT] = 546;
	mem [MEM_UDP6_DST_PORT] = 547;
	mem [MEM_BINDING6] = (intptr_t) binding_linklocal;
	mem [MEM_IP6_DST] = (intptr_t) ip6_multicast_all_dhcp6_servers;
	mem [MEM_ETHER_DST] = (intptr_t) ether_multicast_all_dhcp6_servers;
	pout = netsend_udp6 (pout, mem);
	pout [0] = 1;
	memcpy (pout + 1, ether_mine + 3, 3);
	memcpy (pout + 4          , dhcp6_options, sizeof (dhcp6_options));
	memcpy (pout + 4 + 4+4    , ether_mine, ETHER_ADDR_LEN);
	memcpy (pout + 4 + 4+4+6+4, ether_mine, ETHER_ADDR_LEN);
	mem [MEM_DHCP6_HEAD] = (intptr_t) pout;
	return pout + 4 + sizeof (dhcp6_options);
}

