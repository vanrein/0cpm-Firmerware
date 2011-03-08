#ifndef HEADER_NETFUN
#define HEADER_NETFUN

#define MTU 1500

enum mem_netinput {
	//
	// Pointers to recognised structures
	MEM_ETHER_HEAD,
	MEM_ETHER_PLOAD,
	MEM_ARP_HEAD,
	MEM_IP4_HEAD,
	MEM_IP4_PLOAD,
	MEM_ICMP4_HEAD,
	MEM_DHCP4_HEAD,
	MEM_UDP4_HEAD,
	MEM_UDP4_PLOAD,
	MEM_6BED4_PLOAD,
	MEM_IP6_HEAD,
	MEM_IP6_PLOAD,
	MEM_ICMP6_HEAD,
	MEM_UDP6_HEAD,
	MEM_UDP6_PLOAD,
	MEM_RTP_HEAD,
	MEM_RTCP_HEAD,
	MEM_DHCP6_HEAD,
	MEM_DNS_HEAD,
	MEM_DNSSD_HEAD,
	MEM_SIP_HEAD,
	//
	// Pointer to the first bit outside the packet
	MEM_ALL_DONE,
	//
	// Miscellaneous data from the parsed packets
	MEM_IP4_SRC,
	MEM_IP4_DST,
	MEM_ETHER_SRC,
	MEM_ETHER_DST,
	MEM_UDP4_PORTS,
	MEM_UDP6_PORTS,
	MEM_VLAN_ID,
	MEM_BINDING6,
	MEM_IP6_DST,
	//
	// The number of entries in this enum
	MEM_NETVAR_COUNT
};

typedef struct packet packet_t;
struct packet {
#ifdef HAVE_NET_LEADER
	uint8_t preamble [HAVE_NET_LEADER];
#endif
	uint8_t data [1500 + 18];
#ifdef HAVE_NET_TRAILER
	uint8_t postfix [HAVE_NET_TRAILER];
#endif
};



uint16_t netcore_checksum_areas (void *area0, ...);
void netcore_send_buffer (intptr_t *mem, uint8_t *wbuf);
void netcore_bootstrap_initiate (void);
void netcore_bootstrap_shutdown (void);

uint8_t *net_arp_reply (uint8_t *pkt, uint32_t pktlen, intptr_t *mem);
uint8_t *net_dhcp4 (uint8_t *pkt, uint32_t pktlen, intptr_t *mem);
uint8_t *net_rtp (uint8_t *pkt, uint32_t pktlen, intptr_t *mem);
uint8_t *net_rtcp (uint8_t *pkt, uint32_t pktlen, intptr_t *mem);
uint8_t *net_sip (uint8_t *pkt, uint32_t pktlen, intptr_t *mem);
uint8_t *net_mdns_resp_error (uint8_t *pkt, uint32_t pktlen, intptr_t *mem);
uint8_t *net_mdns_resp_dyn (uint8_t *pkt, uint32_t pktlen, intptr_t *mem);
uint8_t *net_mdns_resp_std (uint8_t *pkt, uint32_t pktlen, intptr_t *mem);
uint8_t *net_mdns_query_error (uint8_t *pkt, uint32_t pktlen, intptr_t *mem);
uint8_t *net_mdns_query_ok (uint8_t *pkt, uint32_t pktlen, intptr_t *mem);

uint8_t *netreply_arp_query (uint8_t *pkt, intptr_t *mem);
uint8_t *netreply_icmp4_echo_req (uint8_t *pkt, intptr_t *mem);
uint8_t *netreply_icmp6_ngb_disc (uint8_t *pkt, intptr_t *mem);
uint8_t *netreply_icmp6_echo_req (uint8_t *pkt, intptr_t *mem);
uint8_t *netreply_dhcp4_offer (uint8_t *pkt, intptr_t *mem);
uint8_t *netreply_dhcp6_advertise (uint8_t *pout, intptr_t *mem);

uint8_t *netsend_icmp6_router_solicit (uint8_t *pout, intptr_t *mem);
uint8_t *netsend_icmp6_ngb_sol (uint8_t *pout, intptr_t *mem);
uint8_t *netsend_dhcp4_discover (uint8_t *pout, intptr_t *mem);
uint8_t *netsend_dhcp6_solicit (uint8_t *pout, intptr_t *mem);

void netdb_initialise (void);
uint8_t *netdb_router_advertised (uint8_t *pout, intptr_t *mem);
uint8_t *netdb_neighbour_advertised (uint8_t *pout, intptr_t *mem);
uint8_t *netdb_dhcp4_ack (uint8_t *pout, intptr_t *mem);
uint8_t *netdb_dhcp4_nak (uint8_t *pout, intptr_t *mem);
uint8_t *netdb_dhcp6_reply (uint8_t *pout, intptr_t *mem);
uint8_t *netdb_dhcp6_reconfigure (uint8_t *pout, intptr_t *mem);

uint8_t *netout (uint8_t *pkt, uint32_t pktlen, intptr_t *mem);

/* Bottom layer functions for reading and writing packets */
bool bottom_network_recv (uint8_t *pktbuf, uint16_t *pktbuflen);
bool bottom_network_send (uint8_t *pktbuf, uint16_t  pktbuflen);

/* Top layer functions for processing network interrupts */
void top_network_can_recv (void);
void top_network_can_send (void);

#endif
