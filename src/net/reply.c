/* netreply.c -- Directly responding to parsed packets
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdint.h>
#include <stdbool.h>

#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ip6.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>
#include <netinet/if_ether.h>

#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/netfun.h>
#include <0cpm/netdb.h>


/* A few well-known addresses to look for
 */
extern uint8_t ether_broadcast [ETHER_ADDR_LEN];
uint8_t ether_zero [ETHER_ADDR_LEN] = { 0, 0, 0, 0, 0, 0 };
uint8_t prefix_6bed4 [8] = {
			0x20, 0x01, 0xab, 0xba, 0x6b, 0xed, 0x00, 0x04 };
extern uint32_t ip4_6bed4;
uint8_t ip4_mine [4] = { 192, 168, 3, 13 };

extern uint8_t linklocal_mine [];

uint8_t ether_mine [ETHER_ADDR_LEN] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 }; 

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
	pout = netreply_ether (pout, mem);
	struct iphdr *ip4in  = (struct iphdr *) mem [MEM_IP4_HEAD];
	struct iphdr *ip4out = (struct iphdr *) pout;
	bzero (ip4out, sizeof (struct iphdr));
	ip4out->version = 4;
	ip4out->ihl = 5;
	ip4out->ttl = 64;
	ip4out->frag_off = htons (0x4000);	// Don't fragment
	ip4out->saddr = ip4in->daddr;
	ip4out->daddr = ip4in->saddr;
	mem [MEM_IP4_HEAD] = (uint32_t) ip4out;
	return &pout [sizeof (struct iphdr)];
}

/* Create an UDPv4 header.
 * Some fields are filled later: len, check.
 */
uint8_t *netreply_udp4 (uint8_t *pout, intptr_t *mem) {
	pout = netreply_ip4 (pout, mem);
	struct udphdr *udp = (struct udphdr *) pout;
	udp->source = htons (mem [MEM_UDP4_PORTS] & 0xffff);
	udp->dest   = htons (mem [MEM_UDP4_PORTS] >> 16);
	mem [MEM_UDP4_HEAD] = (intptr_t) udp;
	return &pout [sizeof (struct udphdr)];
}

/* Create an UDPv4 header for 6bed4.
 * Some fields are filled later: len, check.
 */
uint8_t *netreply_udp4_6bed4 (uint8_t *pout, intptr_t *mem) {
	mem [MEM_IP4_SRC] = htonl (ip4_6bed4);
	pout = netreply_ip4 (pout, mem);
	struct udphdr *udp = (struct udphdr *) pout;
	udp->source = htons (mem [MEM_UDP4_PORTS] & 0xffff);
	udp->dest   = htons (3653);
	mem [MEM_6BED4_PLOAD] =
	mem [MEM_UDP4_HEAD] = (intptr_t) udp;
	return &pout [sizeof (struct udphdr)];
}

/* Create an IPv6 header.
 * Some fields are filled later: plen, nxt.
 */
uint8_t *netreply_ip6 (uint8_t *pout, intptr_t *mem) {
	struct ip6_hdr *in6 = (struct ip6_hdr *) mem [MEM_IP6_HEAD];
	if (mem [MEM_6BED4_PLOAD] != 0) {
		// Use 6bed4 for destination address
		pout = netreply_udp4_6bed4 (pout, mem);
	} else {
		// Use LAN for destination address
		pout = netreply_ether (pout, mem);
	}
	struct ip6_hdr *ip6 = (struct ip6_hdr *) pout;
	ip6->ip6_vfc = htonl (0x60000000);
	ip6->ip6_hlim = 64;
	memcpy (&ip6->ip6_src, &in6->ip6_dst, 16);
	memcpy (&ip6->ip6_dst, &in6->ip6_src, 16);
	mem [MEM_IP6_HEAD] = (intptr_t) ip6;
	return &pout [sizeof (struct ip6_hdr)];
}

/* Create an UDPv6 header.
 * Some fields are filled later: len, check.
 */
uint8_t *netreply_udp6 (uint8_t *pout, intptr_t *mem) {
	pout = netreply_ip6 (pout, mem);
	struct udphdr *udp = (struct udphdr *) pout;
	udp->source = htons (mem [MEM_UDP6_PORTS] & 0xffff);
	udp->dest   = htons (mem [MEM_UDP6_PORTS] >> 16);
	mem [MEM_UDP6_HEAD] = (intptr_t) udp;
	return &pout [sizeof (struct udphdr)];
}


/******************* APPLICATION LEVEL ROUTINES *******************/


/* Create an ARP Reply packet to respond to an ARP Query
 * There are no checksum or length fields in ARP.
 */
uint8_t *netreply_arp_query (uint8_t *pout, intptr_t *mem) {
	printf ("Received an ARP Request; replying\n");
	pout = netreply_ether (pout, mem);
	struct ether_arp *arpout  = (struct ether_arp *) pout;
	struct ether_arp *arpin  = (struct ether_arp *) mem [MEM_ARP_HEAD];
	if (memcmp (arpin->arp_tpa, ip4_mine, 4) == 0) {
		// arpout->ea_hdr.ar_hrd = htons (ARPHRD_IEEE802);
		arpout->ea_hdr.ar_hrd = htons (ARPHRD_ETHER);
		arpout->ea_hdr.ar_pro = htons (ETHERTYPE_IP);
		arpout->ea_hdr.ar_hln = ETHER_ADDR_LEN;
		arpout->ea_hdr.ar_pln = 4;		// IPv4 len
		arpout->ea_hdr.ar_op  = htons (2);	// ARP Reply
		memcpy (arpout->arp_sha, ether_mine, ETHER_ADDR_LEN);
		memcpy (arpout->arp_spa, ip4_mine, 4);
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
	printf ("Received an ICMPv4 Echo Request; replying\n");
	pout = netreply_ip4 (pout, mem);
	struct icmphdr *icmp4out = (struct icmphdr *) pout;
	struct icmphdr *icmp4in  = (struct icmphdr *) mem [MEM_ICMP4_HEAD];
	icmp4out->type = ICMP_ECHOREPLY;
	icmp4out->code = 0;
	icmp4out->un.echo.id       = icmp4in->un.echo.id ;
	icmp4out->un.echo.sequence = icmp4in->un.echo.sequence;
	pout = pout + sizeof (struct icmphdr);
	uint32_t alen = mem [MEM_ALL_DONE] - mem [MEM_ICMP4_HEAD] - sizeof (struct icmphdr);
	if ((alen > 0) && (alen < 128)) {
		memcpy (&icmp4out [1], &icmp4in [1], alen);
		pout += alen;
	}
	mem [MEM_ICMP4_HEAD] = (intptr_t) icmp4out;
	return pout;
}

/* Create an ICMPv6 Echo Reply packet to respond to Echo Request
 * Some fields are filled later: icmp6_cksum.
 */
uint8_t *netreply_icmp6_echo_req (uint8_t *pout, intptr_t *mem) {
	printf ("Received an ICMPv6 Echo Request; replying\n");
	pout = netreply_ip6 (pout, mem);
	struct icmp6_hdr *icmp6 = (struct icmp6_hdr *) pout;
	icmp6->icmp6_type = ICMP6_ECHO_REPLY;
	icmp6->icmp6_code = 0;
	uint16_t len = mem [MEM_ALL_DONE] - mem [MEM_ICMP6_HEAD];
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
	printf ("Received an ICMPv6 Neighbour Discovery; replying\n");
	int bndidx = IP6BINDING_COUNT;
	uint8_t *addr = linklocal_mine;
	uint16_t flgs = 0;
	do {
		if ((flgs & (I6B_EXPIRED | I6B_TENTATIVE)) == 0) {
			if (memcmp ((void *) (mem [MEM_ICMP6_HEAD] + 8), addr, 16) == 0) {
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
	struct ip6_hdr *ip6 = (struct ip6_hdr *) mem [MEM_IP6_HEAD];
	memcpy (&ip6->ip6_src, addr, 16);
	ip6->ip6_hlim = 255;
	struct icmp6_hdr *icmp6 = (struct icmp6_hdr *) pout;
	icmp6->icmp6_type = ND_NEIGHBOR_ADVERT;
	icmp6->icmp6_code = 0;
	icmp6->icmp6_data32 [0] = htonl (0x60000000);
	memcpy (&icmp6->icmp6_data32 [1], addr, 16);
	icmp6->icmp6_data8 [20] = ND_OPT_TARGET_LINKADDR;
	icmp6->icmp6_data8 [21] = 1;	// 1x 8 bytes
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
	printf ("DHCPv4 offer for %d.%d.%d.%d received -- requesting its activation\n", (int) yiaddrptr [0], (int) yiaddrptr [1], (int) yiaddrptr [2], (int) yiaddrptr [3]);
	// TODO: Validate offer to be mine
	mem [MEM_ETHER_DST] = (intptr_t) ether_broadcast;
	pout = netreply_udp4 (pout, mem);
	((struct iphdr *) mem [MEM_IP4_HEAD])->daddr = 0xffffffff;
	bzero (pout, 576);	// erase the acceptable package size
	pout [0] = 1;		// bootrequest
	pout [1] = 1;		// ARP hardware address type
	pout [2] = 6;		// ARP hardware address length
	pout [3] = 0;		// hops
	memcpy (pout + 4, ether_mine + 2, 4);		// client-randomiser
	*(uint16_t *) (pout +  8) = htons (bootsecs++);	// 0 or seconds trying
	// flags=0, no broadcast reply needed
	// ciaddr [4] is 0.0.0.0, the initial client address
	// yiaddr [4] is 0.0.0.0, the "your" address
	// siaddr [4] is 0.0.0.0, the server address is returned
	// giaddr [4] is 0.0.0.0, gateway address is not set by client
	memcpy (pout + 28, ether_mine, 6);	// client hw addr
	// sname [64], the server hostname is empty
	// file [128], the boot filename, is empty
	// options
	uint8_t *popt = pout + 236;
	static const uint8_t dhcp4_options [] = {
		99, 130, 83, 99,	// Magic cookie, RFC 1497
		53, 1, 3,		// DHCP message type REQUEST
		50, 4, 0, 0, 0, 0,	// Requested IP @ 4 + 3 + 2
		// 55, 4, 1, 3, 42, 2,	// Param Request List:
					// mask, router, ntp?, time offset?.
		255			// End Option
	};
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
	printf ("DHCPv6 advertisement\n");
	// TODO: Validate offer to be mine
	uint32_t adlen = mem [MEM_ALL_DONE] - mem [MEM_DHCP6_HEAD];
	if (adlen > 1024) {
		return NULL;	// Suspicuously long options
	}
	pout = netreply_udp6 (pout, mem);
	struct udphdr *udp = (struct udphdr *) mem [MEM_UDP6_HEAD];
	udp->dest = htons (547);
	memcpy (pout, (void *) mem [MEM_DHCP6_HEAD], adlen);
	pout [0] = 3;
	mem [MEM_DHCP6_HEAD] = (intptr_t) pout;
	return pout + adlen;
}

