
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

#include <linux/if_tun.h>
struct tunbuf {
	struct tun_pi prefix;
	uint8_t data [1500 + 18];
};



uint16_t netcore_checksum_areas (void *area0, ...);
int netcore_send_buffer (int tunsox, uint32_t *mem, struct tunbuf *wbuf);

uint8_t *net_arp_reply (uint8_t *pkt, uint32_t pktlen, uint32_t *mem);
uint8_t *net_dhcp4 (uint8_t *pkt, uint32_t pktlen, uint32_t *mem);
uint8_t *net_rtp (uint8_t *pkt, uint32_t pktlen, uint32_t *mem);
uint8_t *net_rtcp (uint8_t *pkt, uint32_t pktlen, uint32_t *mem);
uint8_t *net_sip (uint8_t *pkt, uint32_t pktlen, uint32_t *mem);
uint8_t *net_mdns_resp_error (uint8_t *pkt, uint32_t pktlen, uint32_t *mem);
uint8_t *net_mdns_resp_dyn (uint8_t *pkt, uint32_t pktlen, uint32_t *mem);
uint8_t *net_mdns_resp_std (uint8_t *pkt, uint32_t pktlen, uint32_t *mem);
uint8_t *net_mdns_query_error (uint8_t *pkt, uint32_t pktlen, uint32_t *mem);
uint8_t *net_mdns_query_ok (uint8_t *pkt, uint32_t pktlen, uint32_t *mem);

uint8_t *netreply_arp_query (uint8_t *pkt, uint32_t *mem);
uint8_t *netreply_icmp4_echo_req (uint8_t *pkt, uint32_t *mem);
uint8_t *netreply_icmp6_ngb_disc (uint8_t *pkt, uint32_t *mem);
uint8_t *netreply_icmp6_echo_req (uint8_t *pkt, uint32_t *mem);
uint8_t *netreply_dhcp4_offer (uint8_t *pkt, uint32_t *mem);
uint8_t *netreply_dhcp6_advertise (uint8_t *pout, uint32_t *mem);

uint8_t *netsend_icmp6_router_solicit (uint8_t *pout, uint32_t *mem);
uint8_t *netsend_icmp6_ngb_sol (uint8_t *pout, uint32_t *mem);
uint8_t *netsend_dhcp4_discover (uint8_t *pout, uint32_t *mem);
uint8_t *netsend_dhcp6_solicit (uint8_t *pout, uint32_t *mem);

uint8_t *netdb_router_advertised (uint8_t *pout, uint32_t *mem);
uint8_t *netdb_neighbour_advertised (uint8_t *pout, uint32_t *mem);
uint8_t *netdb_dhcp4_ack (uint8_t *pout, uint32_t *mem);
uint8_t *netdb_dhcp4_nak (uint8_t *pout, uint32_t *mem);
uint8_t *netdb_dhcp6_reply (uint8_t *pout, uint32_t *mem);
uint8_t *netdb_dhcp6_reconfigure (uint8_t *pout, uint32_t *mem);

uint8_t *netout (uint8_t *pkt, uint32_t pktlen, uint32_t *mem);
