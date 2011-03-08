/* netinput.c
 *
 * Process networked input and select actions to take.
 *
 * The "assembly language" used below is the BPF pseudo-language,
 * documented in
 *
 * 			The BSD Packet Filter:
 * 	A New Architecture for User-Level Packet Capture
 * 		by Steven McCanne and Van Jacobson
 *
 *	 ftp://ftp.ee.lbl.gov/papers/bpf-usenix93.ps.Z
 *
 * It serves to analyse frames, set aside the juicy details and
 * return a value which in this case is either the address of a
 * function to invoke, or NULL if the frame should be dropped.
 *
 * It should not be difficult to interpret the consistent assembly
 * instructions (stored in a fixed structure format) from native
 * code, thus leading to extremely efficient network packet handling
 * with a very high level of flexibility for future extensions.
 *
 * This language is designed for speed, and is used in tcpdump et al.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdint.h>
#include <stdbool.h>

#include <netinet/icmp6.h>

#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/netcmd.h>
#include <0cpm/netfun.h>




/*
 * The following is written in BPF assembler, because that is a proper
 * form for handling this kind of problem.  The language lends itself
 * for optimal translation.  In spite of that, it is very flexible.
 *
 * The labels ending in _sel indicate that a definite selection has been
 * made of the kind of frame, but some validation may still be in order.
 *
 * The code below does not calculate checksums yet.  It also does not
 * take packet length into account.
 *
 * The code collects various bits of data in the M[] scratchpad array:
 * TODO:OLD:USING MEM_INPUT ENUM NOW
 * M[0] is the ethernet payload
 * M[1] is the IP4.src address			(or 0 for native IP6)
 * M[2] is the IP4.dst address			(or 0 for native IP6)
 * M[3] is the IP4 payload			(or 0 for native IP6)
 * M[4] is the UDP4 src.PORT|dst.PORT		(or 0 for native IP6)
 * M[5] is the UDP4 payload			(or 0 for native IP6)
 * M[6] is the IP6 frame address (+40 is payload)
 * M[7] is the UDP6 src.PORT|dst.PORT
 * M[8] is the UDP6 payload
 * M[9] is the ethernet header
 *
 * These addresses and values can be used in further analyses, based
 * on the functions returned from the analysis.  Note that the first
 * things these functions do is (1) check lengths and (2) check sums.
 *
 * If all that is okay, it can start processing the frame contents.
 */


/* Declare an efficient network packet analysis function:
 *
 * intptr_t netinput (uint8_t *pkt, uint32_t pktlen, intptr_t *mem) {
 * 	...possibly assembly...
 * }
 *
 * TODO: Check lengths
 * TODO: Verify checksums
 */
BPF_BEGIN(netinput)

	return NULL;		//TODO// Early bail-out

	STX_MEM (MEM_ETHER_HEAD)		// Store the ethernet header
	LDW_LEN ()
	ADD_REX ()
	STW_MEM (MEM_ALL_DONE)

// IN: ETHER frame at X
// OUT: ARP, IP4, IP6 frame at X=M[0]
//
// netin_ETHER		ldw	#14
// 			add	x
// 			tax
// 			ldh	[x-2]
// 			jeq	#8100h, .1, .2	; 802.1Q tag? => step 4 bytes
// .1			ldw	#4
// 			add	x
// 			tax
// 			ldh	[x-2]
// .2			stx	M[0]
// 			jeq	#86DDh, netin_IP6_sel, .3
// .3			jeq	#0800h, netin_IP4_sel, .4
// .4			jeq	#0806h, netin_ARP_sel, netin_DROP

	LDW_LIT (14)
	ADD_REX ()
	LDX_ACC ()
	LDH_IDX (-2)
	JNE_LIT (0x8100, _ether_1)
	LDW_LIT	(4)
	ADD_REX ()
	LDX_ACC ()
	LDH_IDX (-2)
BPF_LABEL (_ether_1)
	STX_MEM (MEM_ETHER_PLOAD)
	JEQ_LIT (0x86dd, netin_IP6_sel)
	JEQ_LIT (0x0800, netin_IP4_sel)
	JEQ_LIT (0x0806, netin_ARP_sel)
	JMP_ALW (netin_DROP)


// IN: ARP frame at X=M[0]
// RET: ARP fn with frame in M[0], IPv4 src addr in M[1], IPv4 dst addr in M[2]
//
// netin_ARP_sel	ldh	[x+2]			; protocol type
// 			jeq	#0800h, .1, netin_DROP	; check IPv4
// .1			ld	[x+4]			; hwalen, protalen, cmd
// 			stx	M[0]
// 			jeq	#06040001, netin_ARPREQ, .2
// .2			jeq	#06040002, netin_ARPREPLY, netin_DROP
// netin_ARPREQ		return	#net_arp_request
// netin_ARPREPLY	return	#net_arp_reply

BPF_LABEL(netin_ARP_sel)
	STX_MEM (MEM_ARP_HEAD)
	LDH_IDX (2)
	JNE_LIT (0x0800, netin_DROP)
	LDW_IDX	(4)
	JEQ_LIT (0x06040001, netin_ARPREQ)
	JEQ_LIT (0x06040002, netin_ARPREPLY)
	JMP_ALW (netin_DROP)
BPF_LABEL(netin_ARPREPLY)
	LDW_IDX (14)
	STW_MEM (MEM_IP4_SRC)
	LDW_IDX (24)
	STW_MEM (MEM_IP4_DST)
	RET_LIT (net_arp_reply)
BPF_LABEL(netin_ARPREQ)
	LDW_IDX (14)
	STW_MEM (MEM_IP4_SRC)
	LDW_IDX (24)
	STW_MEM (MEM_IP4_DST)
	RET_LIT (netreply_arp_query)


// IN: IP4 frame at X=M[0]
// OUT: UDP4, ICMP4 at X+20=M[3]; M[1]=src.IP4, M[2]=dst.IP4
// Assumption: No IPv4 option headers
//
// netin_IP4_sel	ldb	[x]			; version/headerlen
// 			jeq	#45h, .1, netin_DROP
// .1			ld	[x+12]			; src.IP4
// 			st	M[1]
// 			ld	[x+16]			; dst.IP4
// 			st	M[2]
// 			ldx	#20
// 			add	x
// 			st	M[3]			; IP4 payload address
// 			tax
// 			ldb	[x-20+9]		; payload protocol
// 			jeq	#17, netin_UDP4_sel, .2
// .2			jeq	#1, netin_ICMP4_sel, netin_DROP

BPF_LABEL(netin_IP4_sel)
	STX_MEM (MEM_IP4_HEAD)
	LDB_IDX (0)
	JNE_LIT (0x0045, netin_DROP)
	LDW_IDX (12)
	STW_MEM (MEM_IP4_SRC)
	LDW_IDX (16)
	STW_MEM (MEM_IP4_DST)
	LDW_LIT (20)
	ADD_REX ()
	STW_MEM (MEM_IP4_PLOAD)
	LDX_ACC ()
	LDB_IDX (-20+9)
	JEQ_LIT (17, netin_UDP4_sel)
	JEQ_LIT (1, netin_ICMP4_sel)
	JMP_ALW (netin_DROP)


// IN: ICMP4 frame at X=M[3]
// OUT: TODO
//
// netin_ICMP4_sel	TODO

BPF_LABEL(netin_ICMP4_sel)
	STX_MEM (MEM_ICMP4_HEAD)
	LDH_IDX (0)	// type/code
	JEQ_LIT (8 << 8, netin_ICMP4_ping)
	RET_LIT (0)

BPF_LABEL(netin_ICMP4_ping)
	RET_LIT (netreply_icmp4_echo_req)


// IN: UDP4 frame at X=M[3]
// OUT: 6BED4, DHCP4 at X+20+8=M[5]; M[4]=src.PORT|dst.PORT
//
// netin_UDP4_sel	ld	#8
// 			add	x
// 			st	M[5]
// 			ld	[x]		; src.PORT|dst.PORT
// 			st	M[4]
// 			jeq	#0e450e45h, netin_6BED4_sel, .1
// .1			jeq	#4344h, netin_DHCP4_sel, .2
// .2			and	#ffff0000h
// 			jeq	#00350000h, netin_DNS_sel, netin_DROP

BPF_LABEL(netin_UDP4_sel)
	STX_MEM (MEM_UDP4_HEAD)
	LDW_LIT (8)
	ADD_REX ()
	LDX_ACC ()
	STX_MEM (MEM_UDP4_PLOAD)
	LDW_IDX (-8)
	STW_MEM (MEM_UDP4_PORTS)
	JEQ_LIT (0x0e450e45, netin_6BED4_sel)
	JEQ_LIT (0x00430044, netin_DHCP4_sel)
	AND_LIT (0xffff0000)
	JEQ_LIT (0x00350000, netin_DNS_sel)
	JMP_ALW (netin_DROP)


// IN: DHCP4 frame at X=M[5]
// OUT: Returns one of netreply_dhcp4_offer, netdb_dhcp4_ack, netdb_dhcp4_nak

BPF_LABEL(netin_DHCP4_sel)
	STX_MEM (MEM_DHCP4_HEAD)
	LDW_LIT	(236 + 4)
	ADD_REX ()
	LDX_ACC ()
	LDW_IDX (-4)	// Magic cookie check
	JNE_LIT (0x63825363, netin_DROP)
BPF_LABEL(netin_DHCP4_optparse)
	LDB_IDX (0)
	JEQ_LIT (255, netin_DROP)
	JEQ_LIT (0, netin_DHCP4_padding)
	JNE_LIT (53, netin_DHCP4_optskip)
	LDB_IDX (2)
	JEQ_LIT (2, netin_DHCP4_offer)
	JEQ_LIT (5, netin_DHCP4_ack)
	JEQ_LIT (6, netin_DHCP4_nak)
	JMP_ALW (netin_DROP)	// Including for my b'casted DHCP4 discover
BPF_LABEL(netin_DHCP4_padding)
	LDW_LIT (1)
	ADD_REX ()
	LDX_ACC ()
	JMP_ALW (netin_DHCP4_optparse)
BPF_LABEL(netin_DHCP4_optskip)
	AND_LIT (0xff)
	ADD_LIT (2)
	ADD_REX ()
	LDX_ACC ()
	JMP_ALW (netin_DHCP4_optparse)
BPF_LABEL(netin_DHCP4_offer)
	RET_LIT (netreply_dhcp4_offer)
BPF_LABEL(netin_DHCP4_ack)
	RET_LIT (netdb_dhcp4_ack)
BPF_LABEL(netin_DHCP4_nak)
	RET_LIT (netdb_dhcp4_nak)


// IN: 6BED4 frame at X=M[5]
// OUT: IP6 at X=M[6]
// Note: M[1]=src.IP, M[2]=dst.IP, M[3]=IP4, M[4]=src.PORT|dst.PORT all nonzero
//
// netin_6BED4_sel	ldx	M[5]		; decapsulate IP6 package
// 			jmp	netin_IP6_sel_anysrc

BPF_LABEL(netin_6BED4_sel)
	STX_MEM (MEM_UDP4_PLOAD)
	STX_MEM (MEM_6BED4_PLOAD)
	JMP_ALW (netin_IP6_sel_anysrc)


// IN: IP6 frame at X
// OUT: IP6 frame at X=M[6], UDP6 or ICMP6 at X+40
//
// netin_IP6_sel	ld	#0
// 			st	M[1]		; no src.IP4
// 			st	M[2]		; no dst.IP4
// 			st	M[3]		; no IP4 payload
// 			st	M[4]		; no UDP4 src.PORT|dst.PORT
// netin_IP6sel__anysrc	stx	M[6]		; IP6 frame
// 			ldb	[x+0]		; version/trafficclass
// 			and	#f0h		; version/0
// 			jeq	#60h, .1, netin_DROP
// .1			ldb	[x+6]		; next header
// 			jeq	#17, netin_UDP6_sel, .2
// .2			jeq	#58, netin_ICMP6_sel, netin_DROP

BPF_LABEL(netin_IP6_sel)
#if 0
	LDW_LIT (0)
	STW_MEM (MEM_IP4_SRC)
	STW_MEM (MEM_IP4_DST)
	STW_MEM (MEM_IP4_PLOAD)
	STW_MEM (MEM_UDP4_PLOAD)
#endif
BPF_LABEL(netin_IP6_sel_anysrc)
	STX_MEM (MEM_IP6_HEAD)
	LDB_LIT (40)
	ADD_REX ()
	LDX_ACC ()
	STX_MEM (MEM_IP6_PLOAD)
	LDB_IDX (-40)
	AND_LIT (0xf0)
	JNE_LIT (0x60, netin_DROP)
	LDB_IDX (-40+6)
	JEQ_LIT (17, netin_UDP6_sel)
	JEQ_LIT (58, netin_ICMP6_sel)
	JMP_ALW (netin_DROP)


// IN: ICMP6 at X
// OUT: TODO
//
// netin_ICMP6_sel: switch between alternative message types

BPF_LABEL(netin_ICMP6_sel)
	STX_MEM (MEM_ICMP6_HEAD)
	LDH_IDX (0)	// Fetch type, code
	// Ignore ND_ROUTER_SOLICIT  << 8, netin_ICMP6_ROUTER_SOLICIT
	JEQ_LIT (ND_ROUTER_ADVERT    << 8, netin_ICMP6_ROUTER_ADVERT)
	JEQ_LIT (ND_NEIGHBOR_SOLICIT << 8, netin_ICMP6_NEIGHBOUR_SOLICIT)
	JEQ_LIT (ND_NEIGHBOR_ADVERT  << 8, netin_ICMP6_NEIGHBOUR_ADVERT)
	//TODO//JEQ_LIT (ND_REDIRECT << 8, netin_ICMP6_REDIRECT)
	JEQ_LIT (ICMP6_ECHO_REQUEST  << 8, netin_ICMP6_ECHO_REQUEST)
	JEQ_LIT (ICMP6_ECHO_REPLY    << 8, netin_ICMP6_ECHO_REPLY)
	JMP_ALW (netin_DROP)

BPF_LABEL(netin_ICMP6_ROUTER_ADVERT)
	RET_LIT (netdb_router_advertised)

// ND Neighbour Solicitation:
// If aimed at me, respond automatically with Neigbour Advertisement
BPF_LABEL(netin_ICMP6_NEIGHBOUR_SOLICIT)
	RET_LIT (netreply_icmp6_ngb_disc)

BPF_LABEL(netin_ICMP6_NEIGHBOUR_ADVERT)
	RET_LIT (netdb_neighbour_advertised)

BPF_LABEL(netin_ICMP6_ECHO_REQUEST)
	RET_LIT (netreply_icmp6_echo_req)

BPF_LABEL(netin_ICMP6_ECHO_REPLY)
	JMP_ALW (netin_DROP)	// TODO: Process if it is used for tests?


// IN: UDP6 at X
// OUT: RTP, RTCP, SIP, DHCP6, DNS, DNS_SD at X=M[8]; M[7]=src.PORT|dst.PORT
// Note: RTP/RTCP is located at ports 8192..65535
//
// netin_UDP6_sel	ld	#48
// 			add	x
// 			st	M[8]
// 			tax
// 			ld	[x-8+0]			; src.PORT|dst.PORT
// 			jset	#0000e000h, .1, .3	; RTP/RTCP or other?
// .1			jset	#00000001h, netin_RTCP_sel, netin_RTP_sel
// .2			jeq	#02230222h, netin_DHCP6_sel, .3
// .3			and	#0000ffffh		; dst.PORT
// 			jeq	#5060, netin_SIP_sel, .4
// .4			jeq	#5353, netin_DNS_SD_sel, .5
// .5			jeq	#53, netin_DNS_sel, netin_DROP

BPF_LABEL(netin_UDP6_sel)
	STX_MEM (MEM_UDP6_HEAD)
	LDW_LIT (8)
	ADD_REX ()
	STW_MEM (MEM_UDP6_PLOAD)
	LDX_ACC ()
	LDW_IDX (-8+0)
	STW_MEM (MEM_UDP6_PORTS)
	JMZ_LIT (0x0000e000, _UDP6_2)
	JMZ_LIT (0x00000001, netin_RTCP_sel)
	JMP_ALW (netin_RTP_sel)
BPF_LABEL(_UDP6_2)
	AND_LIT (0x0000ffff)
	JEQ_LIT (546, netin_DHCP6_sel)
	JEQ_LIT (5060, netin_SIP_sel)
	JEQ_LIT (5353, netin_DNS_SD_sel)
	JEQ_LIT (53, netin_DNS_sel)
	JMP_ALW (netin_DROP)


// IN: RTP at M[8]=X
// OUT: net_rtp
//
// netin_RTP_sel	return	#net_rtp

BPF_LABEL(netin_RTP_sel)
	STX_MEM (MEM_RTP_HEAD)
	RET_LIT (net_rtp)


// IN: RTCP at M[8]=X
// OUT: net_rtcp
//
// netin_RTCP_sel	return	#net_rtcp

BPF_LABEL(netin_RTCP_sel)
	STX_MEM (MEM_RTCP_HEAD)
	RET_LIT (net_rtcp)


// IN: SIP at M[8]=X
// OUT: net_sip
//
// netin_SIP_sel	return	#net_sip

BPF_LABEL(netin_SIP_sel)
	STX_MEM (MEM_SIP_HEAD)
	RET_LIT (net_sip)


// IN: DHCP6 at M[8]=X
// OUT: TODO

BPF_LABEL(netin_DHCP6_sel)
	STX_MEM (MEM_DHCP6_HEAD)
	LDB_IDX (0)
	JEQ_LIT (2, netin_DHCP6_advertise)
	JEQ_LIT (7, netin_DHCP6_reply)
	JNE_LIT (10, netin_DROP)
BPF_LABEL(netin_DHCP6_reconfigure)
	RET_LIT (netdb_dhcp6_reconfigure)
BPF_LABEL(netin_DHCP6_reply)
	RET_LIT (netdb_dhcp6_reply)
BPF_LABEL(netin_DHCP6_advertise)
	RET_LIT (netreply_dhcp6_advertise)


// IN: DNS at M[8]=X
// OUT: TODO

BPF_LABEL(netin_DNS_sel)
	STX_MEM (MEM_DNS_HEAD)
	RET_LIT (0)


// IN: DNS_SD at M[8]=X
// OUT: TODO: Nogal wat gegokt over hoe MDNS/DNS_SD werkt
//
// netin_DNS_SD_sel	ldh	[x+2]
// 			jset	#8000h, .resp, .query
//
// .resp		jset	#0203h, .resp.ko, .resp.ok   ; TC or RC!=OK
// .resp.ko		return	#net_mdns_resp_error
// .resp.ok		jset	#0400h, .resp.dyn, .resp.std
// .resp.dyn		return	#net_mdns_resp_dyn
// .resp.std		return	#net_mdns_resp_std
//
// .query		xor	#2000h	; std query flag
// 			jset	#3c00h, .query.ko, .query.ok
// .query.ko		return	#net_mdns_query_error
// .query.ok		return	#net_mdns_query_ok

BPF_LABEL(netin_DNS_SD_sel)
	STX_MEM (MEM_DNSSD_HEAD)
	LDH_IDX (2)
	JMZ_LIT (0x8000, _DNS_query)
	JMZ_LIT (0x0203, _DNS_resp_ok)
	RET_LIT (net_mdns_resp_error)
BPF_LABEL(_DNS_resp_ok)
	JMZ_LIT (0x0400, _DNS_resp_std)
	RET_LIT (net_mdns_resp_dyn)
BPF_LABEL(_DNS_resp_std)
	RET_LIT (net_mdns_resp_std)

BPF_LABEL(_DNS_query)
	XOR_LIT (0x2000)
	JMZ_LIT (0x3c00, _DNS_query_ok)
	RET_LIT (net_mdns_query_error)
BPF_LABEL(_DNS_query_ok)
	RET_LIT (net_mdns_query_ok)


// Note: Return a panic result, intending to cause a silent frame drop.
//       No explicitly communicated error message should jump here, only
//       frames that match no known pattern should end up here.
//
// netin_DROP		return	#0			; request frame drop

BPF_LABEL(netin_DROP)
	RET_LIT (0)


// End of the BPF-styled assembly code
//
BPF_END()

