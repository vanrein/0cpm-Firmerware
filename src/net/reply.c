/* netreply.c -- Directly responding to parsed packets
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

// #include <netinet/ip.h>
// #include <netinet/udp.h>
// #include <netinet/ip6.h>
// #include <netinet/ip_icmp.h>
// #include <netinet/icmp6.h>
// #include <netinet/if_ether.h>

#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/irq.h>
#include <0cpm/timer.h>
#include <0cpm/netinet.h>
#include <0cpm/netfun.h>
#include <0cpm/netdb.h>
#include <0cpm/cons.h>


/* A few well-known addresses to look for
 */
extern uint8_t ether_broadcast [ETHER_ADDR_LEN];
uint8_t ether_zero [ETHER_ADDR_LEN] = { 0, 0, 0, 0, 0, 0 };
uint8_t prefix_6bed4 [8] = {
			0x20, 0x01, 0xab, 0xba, 0x6b, 0xed, 0x00, 0x04 };
extern uint32_t ip4_6bed4;
uint8_t ip4_mine [4] = { 192, 168, 3, 13 };

extern uint8_t linklocal_mine [];

//TODO:MAC_FROM_BOTTOM// uint8_t ether_mine [ETHER_ADDR_LEN] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 }; 
uint8_t ether_mine [ETHER_ADDR_LEN] = { 0x00, 0x0b, 0x82, 0x19, 0xa0, 0xf4 }; 

uint16_t bootsecs = 0;


/******************* HEADERVEL CREATION UTILITIES *******************/


/* Create an Ethernet header.
 * Some fields are filled later: h_proto
 */
uint8_t *netreply_ether (uint8_t *pout, intptr_t *mem) {
	struct ethhdr *ethin  = (struct ethhdr *) mem [MEM_ETHER_HEAD];
	struct ethhdr *ethout = (struct ethhdr *) pout;
	if (mem [MEM_ETHER_DST] != 0) {
		memcpy (ethout->h_dest, (void *) mem [MEM_ETHER_DST], ETHER_ADDR_LEN);
	} else if (memcmp (ethin->h_source, ether_zero, ETHER_ADDR_LEN) != 0) {
		memcpy (ethout->h_dest, ethin->h_source, ETHER_ADDR_LEN);
	} else {
		memset (ethout->h_dest, 0xff, ETHER_ADDR_LEN);  // Broadcast
	}
	memcpy (ethout->h_source, ether_mine, ETHER_ADDR_LEN);
	mem [MEM_ETHER_HEAD] = (intptr_t) ethout;
	return &pout [sizeof (struct ethhdr)];
}

/* Create an IPv4 header.
 * Some IPv4 fields are filled later: protocol, tot_len, check.
 */
uint8_t *netreply_ip4 (uint8_t *pout, intptr_t *mem) {
	struct iphdr *ip4in;
	struct iphdr *ip4out;
	pout = netreply_ether (pout, mem);
	ip4in  = (struct iphdr *) mem [MEM_IP4_HEAD];
	ip4out = (struct iphdr *) pout;
	bzero (ip4out, sizeof (struct iphdr));
	netset8  (ip4out->version_ihl, 0x45);
	netset8  (ip4out->ttl, 64);
	netset16 (ip4out->frag_off, 0x4000);	// Don't fragment
	memcpy (&ip4out->saddr, &ip4in->daddr, 4);
	memcpy (&ip4out->daddr, &ip4in->saddr, 4);
	mem [MEM_IP4_HEAD] = (intptr_t) ip4out;
	return &pout [sizeof (struct iphdr)];
}

/* Create an UDPv4 header.
 * Some fields are filled later: len, check.
 */
uint8_t *netreply_udp4 (uint8_t *pout, intptr_t *mem) {
	struct udphdr *udp;
	pout = netreply_ip4 (pout, mem);
	udp = (struct udphdr *) pout;
	netset16 (udp->source, mem [MEM_UDP4_DST_PORT]);
	netset16 (udp->dest  , mem [MEM_UDP4_SRC_PORT]);
	mem [MEM_UDP4_HEAD] = (intptr_t) udp;
	return &pout [sizeof (struct udphdr)];
}

/* Create an UDPv4 header for 6bed4.
 * Some fields are filled later: len, check.
 */
uint8_t *netreply_udp4_6bed4 (uint8_t *pout, intptr_t *mem) {
	struct udphdr *udp;
	mem [MEM_IP4_SRC] = htonl (ip4_6bed4);	// TODO -- netset16?
	pout = netreply_ip4 (pout, mem);
	udp = (struct udphdr *) pout;
	netset16 (udp->source, mem [MEM_UDP4_DST_PORT]);
	netset16 (udp->dest  , 3653);
	mem [MEM_6BED4_PLOAD] =
	mem [MEM_UDP4_HEAD] = (intptr_t) udp;
	return &pout [sizeof (struct udphdr)];
}

/* Create an IPv6 header.
 * Some fields are filled later: plen, nxt.
 */
uint8_t *netreply_ip6 (uint8_t *pout, intptr_t *mem) {
	struct ip6_hdr *in6 = (struct ip6_hdr *) mem [MEM_IP6_HEAD];
	struct ip6_hdr *ip6;
	if (mem [MEM_6BED4_PLOAD] != 0) {
		// Use 6bed4 for destination address
		pout = netreply_udp4_6bed4 (pout, mem);
	} else {
		// Use LAN for destination address
		pout = netreply_ether (pout, mem);
	}
	ip6 = (struct ip6_hdr *) pout;
	netset32 (ip6->ip6_flow, 0x60000000);
	netset8  (ip6->ip6_hlim, 64);
	memcpy (&ip6->ip6_src, &in6->ip6_dst, 16);
	memcpy (&ip6->ip6_dst, &in6->ip6_src, 16);
	mem [MEM_IP6_HEAD] = (intptr_t) ip6;
	return &pout [sizeof (struct ip6_hdr)];
}

/* Create an UDPv6 header.
 * Some fields are filled later: len, check.
 */
uint8_t *netreply_udp6 (uint8_t *pout, intptr_t *mem) {
	struct udphdr *udp;
	pout = netreply_ip6 (pout, mem);
	udp = (struct udphdr *) pout;
	netset16 (udp->source, mem [MEM_UDP6_DST_PORT]);
	netset16 (udp->dest  , mem [MEM_UDP6_SRC_PORT]);
	mem [MEM_UDP6_HEAD] = (intptr_t) udp;
	return &pout [sizeof (struct udphdr)];
}


/******************* APPLICATION LEVEL ROUTINES *******************/


/* Create an ARP Reply packet to respond to an ARP Query
 * There are no checksum or length fields in ARP.
 */
uint8_t *netreply_arp_query (uint8_t *pout, intptr_t *mem) {
	struct ether_arp *arpout;
	struct ether_arp *arpin;
	bottom_printf ("Received an ARP Request; replying\n");
	pout = netreply_ether (pout, mem);
	arpout = (struct ether_arp *) pout;
	arpin  = (struct ether_arp *) mem [MEM_ARP_HEAD];
	//TODO:OLD// if (memcmp (arpin->arp_tpa, ip4_mine, 4) == 0) {
	// TODO: Real binding for IPv4
	if (netget32 (*(nint32_t *)arpin->arp_tpa) == ip4binding [0].ip4addr) {
		// arpout->ea_hdr.ar_hrd = htons (ARPHRD_IEEE802);
		netset16 (arpout->ea_hdr.ar_hrd, ARPHRD_ETHER);
		netset16 (arpout->ea_hdr.ar_pro, ETHERTYPE_IP);
		netset8  (arpout->ea_hdr.ar_hln, ETHER_ADDR_LEN);
		netset8  (arpout->ea_hdr.ar_pln, 4);	// IPv4 len
		netset16 (arpout->ea_hdr.ar_op,  2);	// ARP Reply
		memcpy (arpout->arp_sha, ether_mine, ETHER_ADDR_LEN);
		netset32 (*(nint32_t *)arpout->arp_spa, mem [MEM_IP4_DST]);
		memcpy (arpout->arp_tha, arpin->arp_sha, ETHER_ADDR_LEN);
		memcpy (arpout->arp_tpa, arpin->arp_spa, 4);
		return pout + sizeof (struct ether_arp);
	} else {
		return NULL;
	}
}

/* Create an ICMPv4 Echo Reply packet to respond to Echo Request
 * Some fields are filled later: icmp_cksum.
 */
uint8_t *netreply_icmp4_echo_req (uint8_t *pout, intptr_t *mem) {
	struct icmphdr *icmp4out;
	struct icmphdr *icmp4in;
	uint32_t alen;
	bottom_printf ("Received an ICMPv4 Echo Request; replying\n");
	pout = netreply_ip4 (pout, mem);
	icmp4out = (struct icmphdr *) pout;
	icmp4in  = (struct icmphdr *) mem [MEM_ICMP4_HEAD];
	netset8  (icmp4out->type, ICMP_ECHOREPLY);
	netset8  (icmp4out->code, 0);
	memcpy (&icmp4out->un.echo, &icmp4in->un.echo, 4);
	pout = pout + sizeof (struct icmphdr);
	alen = mem [MEM_ALL_DONE] - mem [MEM_ICMP4_HEAD] - sizeof (struct icmphdr);
	if ((alen > 0) && (alen < 128)) {
		memcpy (pout, &icmp4in [1], alen);
		pout += alen;
	}
	mem [MEM_ICMP4_HEAD] = (intptr_t) icmp4out;
	return pout;
}

/* Create an ICMPv6 Echo Reply packet to respond to Echo Request
 * Some fields are filled later: icmp6_cksum.
 */
uint8_t *netreply_icmp6_echo_req (uint8_t *pout, intptr_t *mem) {
	struct icmp6_hdr *icmp6;
	uint16_t len;
	bottom_printf ("Received an ICMPv6 Echo Request; replying\n");
	pout = netreply_ip6 (pout, mem);
	icmp6 = (struct icmp6_hdr *) pout;
	netset8 (icmp6->icmp6_type, ICMP6_ECHO_REPLY);
	netset8 (icmp6->icmp6_code, 0);
	len = mem [MEM_ALL_DONE] - mem [MEM_ICMP6_HEAD];
	memcpy (&icmp6->icmp6_data8,
			(void *) (mem [MEM_ICMP6_HEAD] + 4),
			len - 4);
	mem [MEM_ICMP6_HEAD] = (intptr_t) icmp6;
	return pout + len;
}

/* Create a Neighbour Advertisement packet to respond to Neighbour Discovery.
 * This will also respond to ff02::1:ffxx:xxxx packets that end in the last
 * 3 bytes of the ethernet address.
 *
 * Some fields are filled later: icmp6_cksum.
 */
uint8_t *netreply_icmp6_ngb_disc (uint8_t *pout, intptr_t *mem) {
	int bndidx;
	uint8_t *addr;
	uint16_t flgs;
	struct ip6_hdr *ip6;
	struct icmp6_hdr *icmp6;
	bottom_printf ("Received an ICMPv6 Neighbour Discovery; replying\n");
	// The first comparison is against the link local address fe80::...
	bndidx = IP6BINDING_COUNT;
	addr = linklocal_mine;
	flgs = I6B_DEFEND_ME;
	do {
		if ((flgs & (I6B_EXPIRED | I6B_DEFEND_ME)) == I6B_DEFEND_ME) {
			if (memcmp ((void *) (mem [MEM_ICMP6_HEAD] + 8), addr, 16) == 0) {
				bndidx++;
				break;
			}
		}
		addr = ip6binding [bndidx].ip6addr;
		flgs = ip6binding [bndidx].flags;
	} while (bndidx-- >= 0);
	if (bndidx < 0) {
		return NULL;
	}
	pout = netreply_ip6 (pout, mem);
	ip6 = (struct ip6_hdr *) mem [MEM_IP6_HEAD];
	memcpy (&ip6->ip6_src, addr, 16);
	netset8 (ip6->ip6_hlim, 255);
	icmp6 = (struct icmp6_hdr *) pout;
	netset8 (icmp6->icmp6_type, ND_NEIGHBOR_ADVERT);
	netset8 (icmp6->icmp6_code, 0);
	netset32 (icmp6->icmp6_data32 [0], 0x60000000);
	memcpy (&icmp6->icmp6_data32 [1], addr, 16);
	netset8 (icmp6->icmp6_data8 [20], ND_OPT_TARGET_LINKADDR);
	netset8 (icmp6->icmp6_data8 [21], 1);	// 1x 8 bytes
	memcpy (icmp6->icmp6_data8 + 22, ether_mine, ETHER_ADDR_LEN);
	mem [MEM_ICMP6_HEAD] = (intptr_t) icmp6;
	return pout + 8 + 16 + 8;
}

/* Respond to a DHCPv4 OFFER with a REQUEST.  An address is being
 * offered in the yiaddr field (offset 16) of the DHCP packet, but
 * that will also be repeated in the future DHCP ACK.
 */
uint8_t *netreply_dhcp4_offer (uint8_t *pout, intptr_t *mem) {
	uint8_t *yiaddrptr = (uint8_t *) (mem [MEM_DHCP4_HEAD] + 16);
	uint8_t *popt;
	static const uint8_t dhcp4_options [] = {
		99, 130, 83, 99,	// Magic cookie, RFC 1497
		53, 1, 3,		// DHCP message type REQUEST
		50, 4, 0, 0, 0, 0,	// Requested IP @ 4 + 3 + 2
		// 55, 4, 1, 3, 42, 2,	// Param Request List:
					// mask, router, ntp?, time offset?.
		255			// End Option
	};
	bottom_printf ("DHCPv4 offer for %d.%d.%d.%d received -- requesting its activation\n", (intptr_t) yiaddrptr [0], (intptr_t) yiaddrptr [1], (intptr_t) yiaddrptr [2], (intptr_t) yiaddrptr [3]);
	// TODO: Validate offer to be mine
	mem [MEM_ETHER_DST] = (intptr_t) ether_broadcast;
	pout = netreply_udp4 (pout, mem);
	netset32 (((struct iphdr *) mem [MEM_IP4_HEAD])->daddr, 0xffffffff);
	bzero (pout, 576);	// erase the acceptable package size
	pout [0] = 1;		// bootrequest
	pout [1] = 1;		// ARP hardware address type
	pout [2] = 6;		// ARP hardware address length
	pout [3] = 0;		// hops
	memcpy (pout + 4, ether_mine + 2, 4);		// client-randomiser
	*(uint16_t *) (pout +  8) = htons (bootsecs++);	// 0 or seconds trying -- TODO:netset16
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
	memcpy (popt + 4 + 3 + 2, yiaddrptr, 4);
	// return popt + sizeof (dhcp4_options);
	return popt + sizeof (dhcp4_options);
}


/* Respond to a DHCPv6 advertisement with a request.
 *
 * Note that the phone is supportive of multi-homing, and wants to
 * be reachable over as many IPv6 addresses as are available, so it
 * will actually take hold of all the IPv6 space that it can get.
 */
uint8_t *netreply_dhcp6_advertise (uint8_t *pout, intptr_t *mem) {
	uint32_t adlen;
	struct udphdr *udp;
	nint16_t *options;
	bottom_printf ("DHCPv6 advertisement\n");
	// TODO: Validate offer to be mine
	adlen = mem [MEM_ALL_DONE] - mem [MEM_DHCP6_HEAD];
	if (adlen > 1024) {
		return NULL;	// Suspicuously long options
	}
	pout = netreply_udp6 (pout, mem);
	udp = (struct udphdr *) mem [MEM_UDP6_HEAD];
	netset16 (udp->dest, 547);
	memcpy (pout, (void *) mem [MEM_DHCP6_HEAD], adlen);
	mem [MEM_DHCP6_HEAD] = (intptr_t) pout;
	netdb_dhcp6_recurse_options ((nint16_t *) (pout + 4), adlen - 4);
	pout [0] = 3;
	return pout + adlen;
}

