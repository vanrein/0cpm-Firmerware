/* netdb.h -- Network database structures
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


/* IPv4  routing and addressing information.
 *
 * This structure is used to store local IPv4 information.
 * Even if the application in IPv6-only, it may still need IPv4 as a
 * "local" transport to a nearby 6bed4 tunnel server.
 *
 * Some other protocols may use IPv4 for the time being: DNS, NTP.
 */

struct ip4binding {
	struct ip4binding *next;	// Linked-list -- TODO:ARRAY?
	uint32_t ip4addr;		// My IPv4 address -- network order
	uint32_t ip4mask;		// My IPv4 mask -- network order
	uint32_t ip4gateway;		// My IPv4 gateway
	uint32_t ip4timeout;		// The timeout for ip4addr / ip4gateway
	uint32_t ip4dns [2];		// DNS servers available on this route
	//TODO:NONE?// uint16_t flags;			// See I4B_xxx below
};


/* IPv6 routing and addressing information.
 * 
 * This structure is used to find the routing information from a particular
 * IPv6 address.  It also includes timeout values to aid in refreshing or
 * expiring the allocation.
 * 
 * All data is stored in network byte order.
 * 
 * The ip6 variables represent the global IPv6 addressing information,
 * including an ip6uplink if IPv6 routes locally.
 *
 * The ip4 parts represent local communication parameters for IPv4, if
 * 6bed4 is used as a routing layer.
 *
 * Multiple ip6binding structures can be in use at any time.  The first
 * suitable one from the list is selected, but given an assigned address
 * the route is of course set in stone.
 */
struct ip6binding {
	struct ip6binding *next;	// Linked-list -- TODO:ARRAY?
	uint8_t ip6addr [16];		// My IPv6 address
	uint8_t ip6uplink [16];		// The router's link-local address
	uint32_t ip6timeout;		// The timeout for ip6uplink / ip6addr
	struct ip4binding *ip4binding;	// For 6bed4, underlying IPv4 binding
	uint16_t flags;			// See I6B_xxx below
};

/* An autoconfig ip6binding has a preference; tunnels are always lower */
#define I6B_PREFERENCE_MASK		0x000f

/* An ip6binding can be obtained from one of a few sources */
#define I6B_ROUTE_SOURCE_MASK		0x00f0
#define I6B_ROUTE_SOURCE_6BED4_FLAG	0x0080	/* Native if not 6bed4 */
#define I6B_ROUTE_SOURCE_AUTOCONF	0x0000	/* AUTO6 + NATIVE */
#define I6B_ROUTE_SOURCE_DHCP		0x0010	/* DHCP6 + NATIVE */
#define I6B_ROUTE_SOURCE_STATIC		0x0020	/* STAT6 + NATIVE */
#define I6B_ROUTE_SOURCE_6BED4_AUTOCONF	0x0080	/* IP4LL + 6BED4 */
#define I6B_ROUTE_SOURCE_6BED4_DHCP	0x0090	/* DHCP4 + 6BED4 */
#define I6B_ROUTE_SOURCE_6BED4_STATIC	0x00a0	/* STAT4 + 6BED4 */

/* An ip6binding can be tentative, if it is undergoing peer review */
#define I6B_TENTATIVE			0x0100

/* An ip6binding can be in active use; expiry then only sets a flag */
#define I6B_ACTIVE			0x0200
#define I6B_EXPIRED			0x0400


/* The netdb structure with the binding structures */

#define IP4BINDING_COUNT 16
extern struct ip4binding ip4binding [IP4BINDING_COUNT];

#define IP6BINDING_COUNT 16
extern struct ip6binding ip6binding [IP6BINDING_COUNT];

