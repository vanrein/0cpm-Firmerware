/* netinput.c -- Incoming network packet parser and switch.
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

/*
 * This code analyses frames, sets aside the juicy details and
 * returns a value which in this case is either the address of a
 * function to invoke, or NULL if the frame should be dropped.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>		//TODO:TESTONLY//

// #include <netinet/ip.h>
// #include <netinet/ip6.h>
// #include <netinet/udp.h>
// #include <netinet/tcp.h>
// #include <netinet/icmp6.h>
// #include <netinet/ether.h>
// #include <net/if_arp.h>

#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/netinet.h>
#include <0cpm/netcmd.h>
#include <0cpm/netfun.h>
#include <0cpm/cons.h>		//TODO:TESTONLY//

/* Utility functions for packet analyses */

#define forward(t) here += sizeof (t); fromhere -= sizeof (t);
#define store(i, t) if (fromhere < sizeof (t)) return (intptr_t) NULL; mem [i] = (intptr_t) (t *) here;
#define store_forward(i, t) store(i,t) forward(t)
#ifndef get08
#  define get08(o) ((uint8_t) here [o])
#endif
#ifndef get16
#  define get16(o) ((((uint16_t) here [o]) << 8) | ((uint16_t) here [o+1]))
#endif
#ifndef get32
#  define get32(o) ((((uint32_t) here [o]) << 24) | (((uint32_t) here [o+1]) << 16) | (((uint32_t) here [o+2]) << 8) | ((uint32_t) here [o+3]))
#endif


/* Analyse incoming network packets.
 *
 * This is one big switch statement that returns how to
 * process a packet, or NULL if it is to be dropped.
 * Instead of proper nesting, which would be a bit
 * overzealously indented and therefore confusing, a
 * less structured approach is used, with portions of
 * the function analysing particular headers and jumping
 * at such parts of the function with a goto statement.
 * Surely goto is bad structure, but some situations
 * really call for it -- like this one.
 *
 * The utility functions below _can_ be overridden in
 * config.h to benefit from the more specific knowledge
 * for specifically known targets.
 */

intptr_t netinput (uint8_t *pkt, uint16_t pktlen, intptr_t *mem) {

	register uint8_t *here = pkt;
	register uint16_t fromhere = pktlen;
	uint16_t ethertp;
	uint16_t arpproto;
	uint32_t arpdetails;
	uint8_t ip4_ptype;
	uint16_t icmp4_type_code;
	uint16_t udp4_src;
	uint16_t udp4_dst;
	uint32_t dhcp4_magic_cookie;
	uint8_t ip6_nxthdr;
	uint16_t icmp6_type_code;
	uint16_t udp6_src;
	uint16_t udp6_dst;
	uint8_t dhcp6_tag;
	uint16_t dnssd_flags;
	uint8_t llc_ssap, llc_dsap;
	uint16_t llc_cmd;

	//
	// Store the end of the entire packet
	mem [MEM_ALL_DONE] = (intptr_t) &pkt [pktlen];

	//
	// Store the ethernet header
	ethertp = get16 (12);
	if (ethertp == 0x8100) {
		struct ethhdrplus { struct ethhdr std; uint8_t ext8021q [4]; };
		store_forward (MEM_ETHER_HEAD, struct ethhdrplus);
		ethertp = get16 (16);
	} else {
		store_forward (MEM_ETHER_HEAD, struct ethhdr);
	}
	//
	// Jump, depending on the ethernet type
	switch (ethertp) {
	case 0x86dd: goto netin_IP6_sel;
	case 0x0800: goto netin_IP4_sel;
	case 0x0806: goto netin_ARP_sel;
	default:
#if defined(CONFIG_FUNCTION_NETCONSOLE) || defined(CONFIG_FUNCTION_FIRMWARE_UPGRADES)
		if (ethertp <= 1500) {
			mem [MEM_LLC_PKTLEN] = (intptr_t) ethertp;
			goto netin_LLC_sel;
		}
#endif
		return (intptr_t) NULL;
	}

	//
	// Accept ARP reply and query messages translating IPv4 to MAC
netin_ARP_sel:
	arpproto = get16 (2);
ht162x_led_set (4, 1, true); // Chinese top symbol
	if (arpproto != 0x0800) return (intptr_t) NULL;
ht162x_led_set (5, 1, true); // Chinese bottom symbol
	store (MEM_ARP_HEAD, struct ether_arp);
	mem [MEM_IP4_SRC] = get32 (14);
	mem [MEM_IP4_DST] = get32 (24);
	arpdetails = get32 (4);
	//
	// Decide what to return based on the ARP header contents
	switch (arpdetails) {
	case 0x06040001:
ht162x_led_set (4, 0, true); // Chinese top symbol (off)
			 return (intptr_t) netreply_arp_query;
	case 0x06040002:
ht162x_led_set (5, 0, true); // Chinese bottom symbol (off)
			 return (intptr_t) netdb_arp_reply;
	default: return (intptr_t) NULL;
	}

	//
	// Decide what IPv4 protocol has come in
netin_IP4_sel:
	store (MEM_IP4_HEAD, struct iphdr);
	if (get08 (0) != 0x45) return (intptr_t) NULL;
	mem [MEM_IP4_SRC] = get32 (12);
	mem [MEM_IP4_DST] = get32 (15);
	ip4_ptype = get08 (9);
	forward (struct iphdr);
	mem [MEM_IP4_PLOAD] = (intptr_t) here;
	//
	// Jump to a handler for the payload type
	switch (ip4_ptype) {
	case 17: goto netin_UDP4_sel;
	case 1:  goto netin_ICMP4_sel;
	default: return (intptr_t) NULL;
	}

	//
	// Decide how to handle ICMP4
netin_ICMP4_sel:
	icmp4_type_code = get16 (0);
	switch (icmp4_type_code) {
	case 8 << 8: return (intptr_t) netreply_icmp4_echo_req;
	default: return (intptr_t) NULL;
	}

	//
	// Decide how to handle UDP4
netin_UDP4_sel:
	store (MEM_UDP4_HEAD, struct udphdr);
	udp4_src = mem [MEM_UDP4_SRC_PORT] = get16 (0);
	udp4_dst = mem [MEM_UDP4_DST_PORT] = get16 (2);
	forward (struct udphdr);
	mem [MEM_UDP4_PLOAD] = (intptr_t) here;
	//
	// Choose a handler based on port
	if (udp4_src == 3653) goto netin_6BED4_sel;
	if ((udp4_src == 67) && (udp4_dst == 68)) goto netin_DHCP4_sel;
	if (udp4_dst == 53) goto netin_DNS_sel;
	return (intptr_t) NULL;

	//
	// Handle incoming DHCP4 traffic
netin_DHCP4_sel:
	store (MEM_DHCP4_HEAD, uint8_t);
	if (fromhere < 236 + 4) return (intptr_t) NULL;
	dhcp4_magic_cookie = get32 (236);
	if (dhcp4_magic_cookie != 0x63825363) return (intptr_t) NULL;	// TODO: BOOTP?
	here     += 236 + 4;
	fromhere -= 236 + 4;
	//
	// Parse DHCP4 options
	while (true) {
		uint8_t dhcp4_option = get08 (0);
		if (dhcp4_option != 0) {
			if (2 + get08 (1) > fromhere) {
				return (intptr_t) NULL;
			}
		}
		switch (dhcp4_option) {
		case 255: // Termination, not run into an offer
			return (intptr_t) NULL;
		case 0: // Padding
			here++;
			continue;
		case 53: // DHCP4_offer
			switch (get08 (2)) {
			case 2: return (intptr_t) netreply_dhcp4_offer;
			case 5: return (intptr_t) netdb_dhcp4_ack;
			case 6: return (intptr_t) netdb_dhcp4_nak;
			default: return (intptr_t) NULL;
			}
		default:
			here += 2 + here [1];
			break;
		}
	}

	//
	// Handling 6BED4 tunnelled traffic
netin_6BED4_sel:
	mem [MEM_6BED4_PLOAD] = mem [MEM_UDP4_PLOAD];
	// Code follows immediately below: goto netin_IP6_sel;

	//
	// Handle incoming IP6 traffic
netin_IP6_sel:
	if ((get08 (0) & 0xf0) != 0x60) return (intptr_t) NULL;
	store (MEM_IP6_HEAD, struct ip6_hdr);
	ip6_nxthdr = get08 (6);
	forward (struct ip6_hdr);
	store (MEM_IP6_PLOAD, uint8_t);
	//
	// Choose a handler based on the next header
	switch (ip6_nxthdr) {
	case 17: goto netin_UDP6_sel;
	case 58: goto netin_ICMP6_sel;
	default: return (intptr_t) NULL;
	}

	//
	// Hande incoming ICMP6 traffic
netin_ICMP6_sel:
	store (MEM_ICMP6_HEAD, struct icmp6_hdr);
	icmp6_type_code = get16 (0);
	switch (icmp6_type_code) {
	case ND_ROUTER_ADVERT << 8:	return (fromhere < 16)? (intptr_t) NULL: (intptr_t) netdb_router_advertised;
	case ND_NEIGHBOR_SOLICIT << 8:	return (fromhere < 24)? (intptr_t) NULL: (intptr_t) netreply_icmp6_ngb_disc;
	case ND_NEIGHBOR_ADVERT << 8:	return (fromhere < 24)? (intptr_t) NULL: (intptr_t) netdb_neighbour_advertised;
	case ND_REDIRECT << 8:		return (fromhere < 40)? (intptr_t) NULL: (intptr_t) NULL; /* TODO */
	case ICMP6_ECHO_REQUEST << 8:	return (fromhere <  8)? (intptr_t) NULL: (intptr_t) netreply_icmp6_echo_req;
	case ICMP6_ECHO_REPLY << 8:	return (fromhere <  8)? (intptr_t) NULL: (intptr_t) NULL; /* Future option */
	default: return (intptr_t) NULL;
	}

	//
	// Decide how to handle incoming UDP6 traffic
netin_UDP6_sel:
	store (MEM_UDP6_HEAD, struct udphdr);
	mem [MEM_UDP6_PLOAD] = (intptr_t) (here + sizeof (struct udphdr));
	udp6_src = mem [MEM_UDP6_SRC_PORT] = get16 (0);
	udp6_dst = mem [MEM_UDP6_DST_PORT] = get16 (2);
	here += sizeof (struct udphdr);
	//
	// Port mapping:
	// udp6_dst >= 0x4000 are RTP (even ports) or RTCP (odd ports)
	// udp6_dst == 546    is DHCP6
	// udp6_dst == 5060   is SIP
	// udp6_dst == 5353   is DNS-SD
	// udp6_dst == 53     is DNS
	if (udp6_dst >= 0x4000) {
		if (udp6_dst & 0x0001) {
			mem [MEM_RTP_HEAD] = (intptr_t) here;
			return (intptr_t) net_rtp;
		} else {
			return (intptr_t) net_rtcp;
		}
	} else if (udp6_dst == 5060) {
		mem [MEM_SIP_HEAD] = (intptr_t) here;
		return (intptr_t) net_sip;
	} else if (udp6_dst == 5353) {
		goto netin_DNSSD_sel;
	} else if (udp6_dst == 53) {
		goto netin_DNS_sel;
	} else if (udp6_dst == 546) {
		goto netin_DHCP6_sel;
	} else {
		return (intptr_t) NULL;
	}
	

netin_DHCP6_sel:
	store (MEM_DHCP6_HEAD, uint8_t);
	dhcp6_tag = *here;
	switch (dhcp6_tag) {
	case 2: return (intptr_t) netreply_dhcp6_advertise;
	case 7: return (intptr_t) netdb_dhcp6_reply;
	case 10: return (intptr_t) netdb_dhcp6_reconfigure;
	default: return (intptr_t) NULL;
	}

netin_DNS_sel:
	store (MEM_DNS_HEAD, uint8_t);
	return (intptr_t) NULL;	// TODO: Actually, split/process DNS

netin_DNSSD_sel:
	store (MEM_DNSSD_HEAD, uint8_t);
	dnssd_flags = get16 (2);
	if (dnssd_flags & 0x8000) {
		// Response
		if (dnssd_flags & 0x0203) {
			return (intptr_t) net_mdns_resp_error;
		} else if (dnssd_flags & 0x0400) {
			return (intptr_t) net_mdns_resp_dyn;
		} else {
			return (intptr_t) net_mdns_query_ok;
		}
	} else {
		// Query
		if ((dnssd_flags ^ 0x2000) & 0x3c00) {
			return (intptr_t) net_mdns_query_error;
		} else {
			return (intptr_t) net_mdns_query_ok;
		}
	}

	// Finally, a catchall that rejects anything that makes it to here
	return (intptr_t) NULL;


/********** OPTIONAL CODE FOR LLC: NETCONSOLE, FIRMWARE UPGRADES **********/


#if defined (CONFIG_FUNCTION_NETCONSOLE) || defined (CONFIG_FUNCTION_FIRMWARE_UPGRADES)

netin_LLC_sel:
	mem [MEM_LLC_DSAP] = (intptr_t) get08 (0);
	mem [MEM_LLC_SSAP] = (intptr_t) get08 (1);;
	mem [MEM_LLC_CMD] =  get08 (2);
	if ((mem [MEM_LLC_CMD] & 0x03) != 0x03) {
		mem [MEM_LLC_CMD] |= get08 (3) << 8;
		goto netin_LLC2_sel;
	}
	// else, fallthrough to netin_LLC1_sel

netin_LLC1_sel:
	mem [MEM_LLC_PAYLOAD] = (intptr_t) (here + 3);
	switch (mem [MEM_LLC_CMD]) {
#ifdef CONFIG_FUNCTION_FIRMWARE_UPGRADES
	case 0x03:
		if (mem [MEM_LLC_DSAP] == 68) {
			return (intptr_t) netllc_tftp;
		}
		break;
#endif // FIRMWARE_UPGRADES
#ifdef CONFIG_FUNCTION_NETCONSOLE
	case 0x7f:
		if (mem [MEM_LLC_DSAP] == 20) {
			return (intptr_t) netllc_console_sabme;
		}
		break;
	case 0x53:
		if (mem [MEM_LLC_DSAP] == 20) {
			return (intptr_t) netllc_console_disc;
		}
		break;
	case 0x87:
		if (mem [MEM_LLC_DSAP] == 20) {
			return (intptr_t) netllc_console_frmr;
		}
		break;
#endif // NETCONSOLE
	default:
		break;
	}
	return (intptr_t) NULL;

#ifdef CONFIG_FUNCTION_NETCONSOLE
netin_LLC2_sel:
	mem [MEM_LLC_PAYLOAD] = (intptr_t) (here + 4);
	if ((mem [MEM_LLC_CMD] & 0x0001) == 0x0000) {
		return (intptr_t) netllc_console_datasentback;
	} else if ((mem [MEM_LLC_CMD] & 0x0007) == 0x0001) {
		return (intptr_t) netllc_console_receiverfeedback;
	}
	return (intptr_t) NULL;
#endif // NETCONSOLE

#endif // NETCONSOLE || FIRMWARE_UPGRADES

}
