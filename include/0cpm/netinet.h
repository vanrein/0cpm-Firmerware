/* netinet.h -- network data structures
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


/* 0cpm firmerware
 *
 * Much of what follows was taken from Linux, but trimmed down to
 * convey only uint8_t[] arrays to represent network packets.  The
 * need for this arises from some DSP processors like tic55x, that
 * work under an unconventional
 *	sizeof (uint8_t) == sizeof (uint16_t) == 2
 * because their memory is 16-bit and can be used as 8-bit-only.
 *
 * The only way to generically overlay packets with structs is to
 * define all parts of those structs in terms of uint8_t[] and
 * use generic routines netget16() / netput16() and so on to change
 * the values stored in these fields.  Of course, optimisations are
 * possible for concrete local architectures; this is why the defs
 * of netget16, netput16 and so on are only done if the symbols have
 * not been defined in the bottom-half specific include files.
 */


#ifndef HEADER_NETINET
#define HEADER_NETINET


/* The network-specific definitions of 8, 16 and 32 bit values that
 * are generic enough to work on special DSP processors as well,
 * together with their utility functions.  Use the netget16, netput16
 * and so on instead of manipulating values with htons, ntohs and so on.
 */

typedef struct { uint8_t b0             } nint8_t;
typedef struct { uint8_t b0, b1         } nint16_t;
typedef struct { uint8_t b0, b1, b2, b3 } nint32_t;

#ifndef netget8
#define netget8(a) ((uint8_t) (a).b0)
#endif

#ifndef netget16
#define netget16(a) ((((uint16_t) (a).b0) << 8) | ((uint16_t) ((a).b1)))
#endif

#ifndef netget32
#define netget32(a) ((((uint32_t) (a).b0) << 24) | (((uint32_t) (a).b1) << 16) | (((uint32_t) (a).b2) << 8) | (((uint32_t) (a).b3)))
#endif

#ifndef netset8
#define netset8(a,v) (((a).b0) = (v))
#endif

#ifndef netset16
#define netset16(a,v) (((a).b0 = ((v) >> 8)), ((a).b1 = ((v) & 0xff)), (v))
#endif

#ifndef netset32
#define netset32(a,v) (((a).b0 = ((v) >> 24)), ((a).b1 = (((v) >> 16) & 0xff)), ((a).b2 = (((v) >> 8) & 0xff)), ((a).b3 = ((v) & 0xff)), (v))
#endif

/* Note: In bottom-specific overrides, raise an error if these values
 * are defined before they are redefined for that specific bottom.
 * This will raise awareness of the order of header files, and avoid
 * different definitions (albeit semantically compatible) in various
 * files due to varying include orders.  The right order is always
 * to include the bottom-half include files before the top half files.
 */


/* Start includes, mostly taken from Linux header files */


/* Standard well-defined IP protocols.  */
enum {
  IPPROTO_IP = 0,               /* Dummy protocol for TCP               */
  IPPROTO_ICMP = 1,             /* Internet Control Message Protocol    */
  IPPROTO_IGMP = 2,             /* Internet Group Management Protocol   */
  IPPROTO_IPIP = 4,             /* IPIP tunnels (older KA9Q tunnels use 94) */
  IPPROTO_TCP = 6,              /* Transmission Control Protocol        */
  IPPROTO_EGP = 8,              /* Exterior Gateway Protocol            */
  IPPROTO_PUP = 12,             /* PUP protocol                         */
  IPPROTO_UDP = 17,             /* User Datagram Protocol               */
  IPPROTO_IDP = 22,             /* XNS IDP protocol                     */
  IPPROTO_DCCP = 33,            /* Datagram Congestion Control Protocol */
  IPPROTO_RSVP = 46,            /* RSVP protocol                        */
  IPPROTO_GRE = 47,             /* Cisco GRE tunnels (rfc 1701,1702)    */

  IPPROTO_IPV6   = 41,          /* IPv6-in-IPv4 tunnelling              */

  IPPROTO_ESP = 50,            /* Encapsulation Security Payload protocol */
  IPPROTO_AH = 51,             /* Authentication Header protocol       */
  IPPROTO_BEETPH = 94,         /* IP option pseudo header for BEET */
  IPPROTO_PIM    = 103,         /* Protocol Independent Multicast       */

  IPPROTO_COMP   = 108,                /* Compression Header protocol */
  IPPROTO_SCTP   = 132,         /* Stream Control Transport Protocol    */
  IPPROTO_UDPLITE = 136,        /* UDP-Lite (RFC 3828)                  */

  IPPROTO_RAW    = 255,         /* Raw IP packets                       */
  IPPROTO_MAX
};

/*
 *      IPV6 extension headers
 */
#define IPPROTO_HOPOPTS         0       /* IPv6 hop-by-hop options      */
#define IPPROTO_ROUTING         43      /* IPv6 routing header          */
#define IPPROTO_FRAGMENT        44      /* IPv6 fragmentation header    */
#define IPPROTO_ICMPV6          58      /* ICMPv6                       */
#define IPPROTO_NONE            59      /* IPv6 no next header          */
#define IPPROTO_DSTOPTS         60      /* IPv6 destination options     */
#define IPPROTO_MH              135     /* IPv6 mobility header         */

/*
 *      IEEE 802.3 Ethernet magic constants.  The frame sizes omit the preamble
 *      and FCS/CRC (frame check sequence). 
 */

#define ETH_ALEN        6               /* Octets in one ethernet addr   */
#define ETH_HLEN        14              /* Total octets in header.       */
#define ETH_ZLEN        60              /* Min. octets in frame sans FCS */
#define ETH_DATA_LEN    1500            /* Max. octets in payload        */
#define ETH_FRAME_LEN   1514            /* Max. octets in frame sans FCS */
#define ETH_FCS_LEN     4               /* Octets in the FCS             */

/* Ethernet protocol ID's */
#define ETHERTYPE_PUP           0x0200          /* Xerox PUP */
#define ETHERTYPE_SPRITE        0x0500          /* Sprite */
#define ETHERTYPE_IP            0x0800          /* IP */
#define ETHERTYPE_ARP           0x0806          /* Address resolution */
#define ETHERTYPE_REVARP        0x8035          /* Reverse ARP */
#define ETHERTYPE_AT            0x809B          /* AppleTalk protocol */
#define ETHERTYPE_AARP          0x80F3          /* AppleTalk ARP */
#define ETHERTYPE_VLAN          0x8100          /* IEEE 802.1Q VLAN tagging */
#define ETHERTYPE_IPX           0x8137          /* IPX */
#define ETHERTYPE_IPV6          0x86dd          /* IP protocol version 6 */
#define ETHERTYPE_LOOPBACK      0x9000          /* used to test interfaces */


#define ETHER_ADDR_LEN  ETH_ALEN                 /* size of ethernet addr */
#define ETHER_TYPE_LEN  2                        /* bytes in type field */
#define ETHER_CRC_LEN   4                        /* bytes in CRC field */
#define ETHER_HDR_LEN   ETH_HLEN                 /* total octets in header */
#define ETHER_MIN_LEN   (ETH_ZLEN + ETHER_CRC_LEN) /* min packet length */
#define ETHER_MAX_LEN   (ETH_FRAME_LEN + ETHER_CRC_LEN) /* max packet length */


/*
 *      These are the defined Ethernet Protocol ID's.
 */

#define ETH_P_LOOP      0x0060          /* Ethernet Loopback packet     */
#define ETH_P_PUP       0x0200          /* Xerox PUP packet             */
#define ETH_P_PUPAT     0x0201          /* Xerox PUP Addr Trans packet  */
#define ETH_P_IP        0x0800          /* Internet Protocol packet     */
#define ETH_P_X25       0x0805          /* CCITT X.25                   */
#define ETH_P_ARP       0x0806          /* Address Resolution packet    */
#define ETH_P_BPQ       0x08FF          /* G8BPQ AX.25 Ethernet Packet  [ NOT AN OFFICIALLY REGISTERED ID ] */
#define ETH_P_IEEEPUP   0x0a00          /* Xerox IEEE802.3 PUP packet */
#define ETH_P_IEEEPUPAT 0x0a01          /* Xerox IEEE802.3 PUP Addr Trans packet */
#define ETH_P_DEC       0x6000          /* DEC Assigned proto           */
#define ETH_P_DNA_DL    0x6001          /* DEC DNA Dump/Load            */
#define ETH_P_DNA_RC    0x6002          /* DEC DNA Remote Console       */
#define ETH_P_DNA_RT    0x6003          /* DEC DNA Routing              */
#define ETH_P_LAT       0x6004          /* DEC LAT                      */
#define ETH_P_DIAG      0x6005          /* DEC Diagnostics              */
#define ETH_P_CUST      0x6006          /* DEC Customer use             */
#define ETH_P_SCA       0x6007          /* DEC Systems Comms Arch       */
#define ETH_P_RARP      0x8035          /* Reverse Addr Res packet      */
#define ETH_P_ATALK     0x809B          /* Appletalk DDP                */
#define ETH_P_AARP      0x80F3          /* Appletalk AARP               */
#define ETH_P_8021Q     0x8100          /* 802.1Q VLAN Extended Header  */
#define ETH_P_IPX       0x8137          /* IPX over DIX                 */
#define ETH_P_IPV6      0x86DD          /* IPv6 over bluebook           */
#define ETH_P_PAUSE     0x8808          /* IEEE Pause frames. See 802.3 31B */
#define ETH_P_SLOW      0x8809          /* Slow Protocol. See 802.3ad 43B */
#define ETH_P_WCCP      0x883E          /* Web-cache coordination protocol
                                         * defined in draft-wilson-wrec-wccp-v2-00.txt */
#define ETH_P_PPP_DISC  0x8863          /* PPPoE discovery messages     */
#define ETH_P_PPP_SES   0x8864          /* PPPoE session messages       */
#define ETH_P_MPLS_UC   0x8847          /* MPLS Unicast traffic         */
#define ETH_P_MPLS_MC   0x8848          /* MPLS Multicast traffic       */
#define ETH_P_ATMMPOA   0x884c          /* MultiProtocol Over ATM       */
#define ETH_P_ATMFATE   0x8884          /* Frame-based ATM Transport
                                         * over Ethernet
                                         */
#define ETH_P_AOE       0x88A2          /* ATA over Ethernet            */
#define ETH_P_TIPC      0x88CA          /* TIPC                         */

/* Internet address.  */
typedef nint32_t in_addr_t;
struct in_addr
  {
    in_addr_t s_addr;
  };


/*
 * Structure of an internet header, naked of options.
 */
struct ip
  {
// #if __BYTE_ORDER == __LITTLE_ENDIAN
    // unsigned int ip_hl:4;               /* header length */
    // unsigned int ip_v:4;                /* version */
// #endif
// #if __BYTE_ORDER == __BIG_ENDIAN
    unsigned int ip_v:4;                /* version */
    unsigned int ip_hl:4;               /* header length */
// #endif
    nint8_t ip_tos;                    /* type of service */
    nint16_t ip_len;                     /* total length */
    nint16_t ip_id;                      /* identification */
    nint16_t ip_off;                     /* fragment offset field */
#define IP_RF 0x8000                    /* reserved fragment flag */
#define IP_DF 0x4000                    /* dont fragment flag */
#define IP_MF 0x2000                    /* more fragments flag */
#define IP_OFFMASK 0x1fff               /* mask for fragmenting bits */
    nint8_t ip_ttl;                    /* time to live */
    nint8_t ip_p;                      /* protocol */
    nint16_t ip_sum;                     /* checksum */
    struct in_addr ip_src, ip_dst;      /* source and dest address */
  };


struct iphdr
  {
    nint8_t version_ihl;
    nint8_t tos;
    nint16_t tot_len;
    nint16_t id;
    nint16_t frag_off;
    nint8_t ttl;
    nint8_t protocol;
    nint16_t check;
    nint32_t saddr;
    nint32_t daddr;
    /*The options start here. */
  };



struct icmp6_hdr
  {
    nint8_t     icmp6_type;   /* type field */
    nint8_t     icmp6_code;   /* code field */
    nint16_t    icmp6_cksum;  /* checksum field */
    union
      {
        nint32_t  icmp6_un_data32[1]; /* type-specific field */
        nint16_t  icmp6_un_data16[2]; /* type-specific field */
        nint8_t   icmp6_un_data8[4];  /* type-specific field */
      } icmp6_dataun;
  };

#define icmp6_data32    icmp6_dataun.icmp6_un_data32
#define icmp6_data16    icmp6_dataun.icmp6_un_data16
#define icmp6_data8     icmp6_dataun.icmp6_un_data8
#define icmp6_pptr      icmp6_data32[0]  /* parameter prob */
#define icmp6_mtu       icmp6_data32[0]  /* packet too big */
#define icmp6_id        icmp6_data16[0]  /* echo request/reply */
#define icmp6_seq       icmp6_data16[1]  /* echo request/reply */
#define icmp6_maxdelay  icmp6_data16[0]  /* mcast group membership */


#define ICMP6_DST_UNREACH             1
#define ICMP6_PACKET_TOO_BIG          2
#define ICMP6_TIME_EXCEEDED           3
#define ICMP6_PARAM_PROB              4

#define ICMP6_INFOMSG_MASK  0x80    /* all informational messages */

#define ICMP6_ECHO_REQUEST          128
#define ICMP6_ECHO_REPLY            129
#define MLD_LISTENER_QUERY          130
#define MLD_LISTENER_REPORT         131
#define MLD_LISTENER_REDUCTION      132

#define ICMP6_DST_UNREACH_NOROUTE     0 /* no route to destination */
#define ICMP6_DST_UNREACH_ADMIN       1 /* communication with destination */
                                        /* administratively prohibited */
#define ICMP6_DST_UNREACH_BEYONDSCOPE 2 /* beyond scope of source address */
#define ICMP6_DST_UNREACH_ADDR        3 /* address unreachable */
#define ICMP6_DST_UNREACH_NOPORT      4 /* bad port */

#define ICMP6_TIME_EXCEED_TRANSIT     0 /* Hop Limit == 0 in transit */
#define ICMP6_TIME_EXCEED_REASSEMBLY  1 /* Reassembly time out */

#define ICMP6_PARAMPROB_HEADER        0 /* erroneous header field */
#define ICMP6_PARAMPROB_NEXTHEADER    1 /* unrecognized Next Header */
#define ICMP6_PARAMPROB_OPTION        2 /* unrecognized IPv6 option */


#define ND_ROUTER_SOLICIT           133
#define ND_ROUTER_ADVERT            134
#define ND_NEIGHBOR_SOLICIT         135
#define ND_NEIGHBOR_ADVERT          136
#define ND_REDIRECT                 137

#define ND_OPT_SOURCE_LINKADDR          1
#define ND_OPT_TARGET_LINKADDR          2
#define ND_OPT_PREFIX_INFORMATION       3
#define ND_OPT_REDIRECTED_HEADER        4
#define ND_OPT_MTU                      5
#define ND_OPT_RTR_ADV_INTERVAL         7
#define ND_OPT_HOME_AGENT_INFO          8

#define ND_OPT_PI_FLAG_ONLINK   0x80
#define ND_OPT_PI_FLAG_AUTO     0x40
#define ND_OPT_PI_FLAG_RADDR    0x20



/*
 * Internal of an ICMP Router Advertisement
 */
struct icmp_ra_addr
{
  nint32_t ira_addr;
  nint32_t ira_preference;
};


struct icmp
{
  nint8_t  icmp_type;  /* type of message, see below */
  nint8_t  icmp_code;  /* type sub code */
  nint16_t icmp_cksum; /* ones complement checksum of struct */
  union
  {
    nint8_t ih_pptr;             /* ICMP_PARAMPROB */
    struct in_addr ih_gwaddr;   /* gateway address */
    struct ih_idseq             /* echo datagram */
    {
      nint16_t icd_id;
      nint16_t icd_seq;
    } ih_idseq;
    nint32_t ih_void;

    /* ICMP_UNREACH_NEEDFRAG -- Path MTU Discovery (RFC1191) */
    struct ih_pmtu
    {
      nint16_t ipm_void;
      nint16_t ipm_nextmtu;
    } ih_pmtu;

    struct ih_rtradv
    {
      nint8_t irt_num_addrs;
      nint8_t irt_wpa;
      nint16_t irt_lifetime;
    } ih_rtradv;
  } icmp_hun;
#define icmp_pptr       icmp_hun.ih_pptr
#define icmp_gwaddr     icmp_hun.ih_gwaddr
#define icmp_id         icmp_hun.ih_idseq.icd_id
#define icmp_seq        icmp_hun.ih_idseq.icd_seq
#define icmp_void       icmp_hun.ih_void
#define icmp_pmvoid     icmp_hun.ih_pmtu.ipm_void
#define icmp_nextmtu    icmp_hun.ih_pmtu.ipm_nextmtu
#define icmp_num_addrs  icmp_hun.ih_rtradv.irt_num_addrs
#define icmp_wpa        icmp_hun.ih_rtradv.irt_wpa
#define icmp_lifetime   icmp_hun.ih_rtradv.irt_lifetime
  union
  {
    struct
    {
      nint32_t its_otime;
      nint32_t its_rtime;
      nint32_t its_ttime;
    } id_ts;
    struct
    {
      struct ip idi_ip;
      /* options and then 64 bits of data */
    } id_ip;
    struct icmp_ra_addr id_radv;
    nint32_t   id_mask;
    nint8_t    id_data[1];
  } icmp_dun;
#define icmp_otime      icmp_dun.id_ts.its_otime
#define icmp_rtime      icmp_dun.id_ts.its_rtime
#define icmp_ttime      icmp_dun.id_ts.its_ttime
#define icmp_ip         icmp_dun.id_ip.idi_ip
#define icmp_radv       icmp_dun.id_radv
#define icmp_mask       icmp_dun.id_mask
#define icmp_data       icmp_dun.id_data
};


struct icmphdr
{
  nint8_t type;                /* message type */
  nint8_t code;                /* type sub-code */
  nint16_t checksum;
  union
  {
    struct
    {
      nint16_t id;
      nint16_t sequence;
    } echo;                     /* echo datagram */
    nint32_t   gateway;        /* gateway address */
    struct
    {
      nint16_t __unused;
      nint16_t mtu;
    } frag;                     /* path mtu discovery */
  } un;
};


#define ICMP_ECHOREPLY          0       /* Echo Reply                   */
#define ICMP_DEST_UNREACH       3       /* Destination Unreachable      */
#define ICMP_SOURCE_QUENCH      4       /* Source Quench                */
#define ICMP_REDIRECT           5       /* Redirect (change route)      */
#define ICMP_ECHO               8       /* Echo Request                 */
#define ICMP_TIME_EXCEEDED      11      /* Time Exceeded                */
#define ICMP_PARAMETERPROB      12      /* Parameter Problem            */
#define ICMP_TIMESTAMP          13      /* Timestamp Request            */
#define ICMP_TIMESTAMPREPLY     14      /* Timestamp Reply              */
#define ICMP_INFO_REQUEST       15      /* Information Request          */
#define ICMP_INFO_REPLY         16      /* Information Reply            */
#define ICMP_ADDRESS            17      /* Address Mask Request         */
#define ICMP_ADDRESSREPLY       18      /* Address Mask Reply           */
#define NR_ICMP_TYPES           18


/* Codes for UNREACH. */
#define ICMP_NET_UNREACH        0       /* Network Unreachable          */
#define ICMP_HOST_UNREACH       1       /* Host Unreachable             */
#define ICMP_PROT_UNREACH       2       /* Protocol Unreachable         */
#define ICMP_PORT_UNREACH       3       /* Port Unreachable             */
#define ICMP_FRAG_NEEDED        4       /* Fragmentation Needed/DF set  */
#define ICMP_SR_FAILED          5       /* Source Route failed          */
#define ICMP_NET_UNKNOWN        6
#define ICMP_HOST_UNKNOWN       7
#define ICMP_HOST_ISOLATED      8
#define ICMP_NET_ANO            9
#define ICMP_HOST_ANO           10
#define ICMP_NET_UNR_TOS        11
#define ICMP_HOST_UNR_TOS       12
#define ICMP_PKT_FILTERED       13      /* Packet filtered */
#define ICMP_PREC_VIOLATION     14      /* Precedence violation */
#define ICMP_PREC_CUTOFF        15      /* Precedence cut off */
#define NR_ICMP_UNREACH         15      /* instead of hardcoding immediate value */

/* Codes for REDIRECT. */
#define ICMP_REDIR_NET          0       /* Redirect Net                 */
#define ICMP_REDIR_HOST         1       /* Redirect Host                */
#define ICMP_REDIR_NETTOS       2       /* Redirect Net for TOS         */
#define ICMP_REDIR_HOSTTOS      3       /* Redirect Host for TOS        */

/* Codes for TIME_EXCEEDED. */
#define ICMP_EXC_TTL            0       /* TTL count exceeded           */
#define ICMP_EXC_FRAGTIME       1       /* Fragment Reass time exceeded */



struct udphdr
{
  nint16_t source;
  nint16_t dest;
  nint16_t len;
  nint16_t check;
};


/* IPv6 address */
struct in6_addr
  {
    union
      {
        nint8_t u6_addr8[16];
        nint16_t u6_addr16[8];
        nint32_t u6_addr32[4];
      } in6_u;
#define s6_addr                 in6_u.u6_addr8
#define s6_addr16               in6_u.u6_addr16
#define s6_addr32               in6_u.u6_addr32
  };

#define IN6ADDR_ANY_INIT { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } } }
#define IN6ADDR_LOOPBACK_INIT { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } } }

#define INET_ADDRSTRLEN 16
#define INET6_ADDRSTRLEN 46


struct ip6_hdr
  {
    union
      {
        struct ip6_hdrctl
          {
            nint32_t ip6_un1_flow;   /* 4 bits version, 8 bits TC,
                                        20 bits flow-ID */
            nint16_t ip6_un1_plen;   /* payload length */
            nint8_t  ip6_un1_nxt;    /* next header */
            nint8_t  ip6_un1_hlim;   /* hop limit */
          } ip6_un1;
        nint8_t ip6_un2_vfc;       /* 4 bits version, top 4 bits tclass */
      } ip6_ctlun;
    struct in6_addr ip6_src;      /* source address */
    struct in6_addr ip6_dst;      /* destination address */
  };

#define ip6_vfc   ip6_ctlun.ip6_un2_vfc
#define ip6_flow  ip6_ctlun.ip6_un1.ip6_un1_flow
#define ip6_plen  ip6_ctlun.ip6_un1.ip6_un1_plen
#define ip6_nxt   ip6_ctlun.ip6_un1.ip6_un1_nxt
#define ip6_hlim  ip6_ctlun.ip6_un1.ip6_un1_hlim
#define ip6_hops  ip6_ctlun.ip6_un1.ip6_un1_hlim

/* This structure defines an ethernet arp header.  */

/* ARP protocol opcodes. */
#define ARPOP_REQUEST   1               /* ARP request.  */
#define ARPOP_REPLY     2               /* ARP reply.  */
#define ARPOP_RREQUEST  3               /* RARP request.  */
#define ARPOP_RREPLY    4               /* RARP reply.  */
#define ARPOP_InREQUEST 8               /* InARP request.  */
#define ARPOP_InREPLY   9               /* InARP reply.  */
#define ARPOP_NAK       10              /* (ATM)ARP NAK.  */

/* See RFC 826 for protocol description.  ARP packets are variable
   in size; the arphdr structure defines the fixed-length portion.
   Protocol type values are the same as those for 10 Mb/s Ethernet.
   It is followed by the variable-sized fields ar_sha, arp_spa,
   arp_tha and arp_tpa in that order, according to the lengths
   specified.  Field names used correspond to RFC 826.  */

struct arphdr
  {
    nint16_t ar_hrd;          /* Format of hardware address.  */
    nint16_t ar_pro;          /* Format of protocol address.  */
    nint8_t ar_hln;               /* Length of hardware address.  */
    nint8_t ar_pln;               /* Length of protocol address.  */
    nint16_t ar_op;           /* ARP opcode (command).  */
#if 0
    /* Ethernet looks like this : This bit is variable sized
       however...  */
    nint8_t __ar_sha[ETH_ALEN];   /* Sender hardware address.  */
    nint8_t __ar_sip[4];          /* Sender IP address.  */
    nint8_t __ar_tha[ETH_ALEN];   /* Target hardware address.  */
    nint8_t __ar_tip[4];          /* Target IP address.  */
#endif
  };

/*
 *      This is an Ethernet frame header.
 */
 
struct ethhdr {
        nint8_t h_dest[ETH_ALEN];       /* destination eth addr */
        nint8_t h_source[ETH_ALEN];     /* source ether addr    */
        nint16_t          h_proto;                /* packet type ID field */
};

/*
 * Ethernet Address Resolution Protocol.
 *
 * See RFC 826 for protocol description.  Structure below is adapted
 * to resolving internet addresses.  Field names used correspond to
 * RFC 826.
 */
struct  ether_arp {
        struct  arphdr ea_hdr;          /* fixed-size header */
        nint8_t arp_sha[ETH_ALEN];     /* sender hardware address */
        nint8_t arp_spa[4];            /* sender protocol address */
        nint8_t arp_tha[ETH_ALEN];     /* target hardware address */
        nint8_t arp_tpa[4];            /* target protocol address */
};
#define arp_hrd ea_hdr.ar_hrd
#define arp_pro ea_hdr.ar_pro
#define arp_hln ea_hdr.ar_hln
#define arp_pln ea_hdr.ar_pln
#define arp_op  ea_hdr.ar_op




/* ARP protocol HARDWARE identifiers. */
#define ARPHRD_NETROM   0               /* From KA9Q: NET/ROM pseudo. */
#define ARPHRD_ETHER    1               /* Ethernet 10/100Mbps.  */
#define ARPHRD_EETHER   2               /* Experimental Ethernet.  */
#define ARPHRD_AX25     3               /* AX.25 Level 2.  */
#define ARPHRD_PRONET   4               /* PROnet token ring.  */
#define ARPHRD_CHAOS    5               /* Chaosnet.  */
#define ARPHRD_IEEE802  6               /* IEEE 802.2 Ethernet/TR/TB.  */
#define ARPHRD_ARCNET   7               /* ARCnet.  */
#define ARPHRD_APPLETLK 8               /* APPLEtalk.  */
#define ARPHRD_DLCI     15              /* Frame Relay DLCI.  */
#define ARPHRD_ATM      19              /* ATM.  */
#define ARPHRD_METRICOM 23              /* Metricom STRIP (new IANA id).  */
#define ARPHRD_IEEE1394 24              /* IEEE 1394 IPv4 - RFC 2734.  */
#define ARPHRD_EUI64            27              /* EUI-64.  */
#define ARPHRD_INFINIBAND       32              /* InfiniBand.  */


#endif

