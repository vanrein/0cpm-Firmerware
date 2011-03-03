/* tuntest.c -- Tunnel-based testing of the network code.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>

#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/if_ether.h>

#include <0cpm/netfun.h>
#include <0cpm/netdb.h>



int tunsox = -1;

struct tunbuf rbuf, wbuf;



/*
 * TEST OUTPUT FUNCTIONS
 */

uint8_t *net_arp_reply (uint8_t *pkt, uint32_t pktlen, uint32_t *mem) {
	printf ("Received: ARP Reply\n");
	return NULL;
}

uint8_t *net_rtp (uint8_t *pkt, uint32_t pktlen, uint32_t *mem) {
	printf ("Received: RTP\n");
	return NULL;
}

uint8_t *net_rtcp (uint8_t *pkt, uint32_t pktlen, uint32_t *mem) {
	printf ("Received: RTCP\n");
	return NULL;
}

uint8_t *net_sip (uint8_t *pkt, uint32_t pktlen, uint32_t *mem) {
	printf ("Received: SIP\n");
	return NULL;
}

uint8_t *net_mdns_resp_error (uint8_t *pkt, uint32_t pktlen, uint32_t *mem) {
	//TODO// printf ("Received: mDNS error response\n");
	return NULL;
}

uint8_t *net_mdns_resp_dyn (uint8_t *pkt, uint32_t pktlen, uint32_t *mem) {
	//TODO// printf ("Received: mDNS dynamic response\n");
	return NULL;
}

uint8_t *net_mdns_resp_std (uint8_t *pkt, uint32_t pktlen, uint32_t *mem) {
	//TODO// printf ("Received: mDNS standard response\n");
	return NULL;
}

uint8_t *net_mdns_query_error (uint8_t *pkt, uint32_t pktlen, uint32_t *mem) {
	//TODO// printf ("Received: mDNS error query\n");
	return NULL;
}

uint8_t *net_mdns_query_ok (uint8_t *pkt, uint32_t pktlen, uint32_t *mem) {
	//TODO// printf ("Received: mDNS ok query\n");
	return NULL;
}



/* A collection of initialisation steps.
 */
void init (void) {
	int i;
	for (i=0; i<IP6BINDING_COUNT; i++) {
		ip6binding [i].flags = I6B_EXPIRED;
	}
}

/*
 * TEST SETUP FUNCTIONS
 */

typedef void *retfn (uint8_t *pout, uint32_t *mem);

int process_packets (int sox, int secs) {
	int ok = 1;
	time_t maxtm = secs + time (NULL);
	while (ok) {
		fd_set seln;
		FD_ZERO (&seln);
		FD_SET (sox, &seln);
		struct timeval tout;
		tout.tv_sec = maxtm - time (NULL);
		tout.tv_usec = 500000;
		if ((tout.tv_sec < 0) || (select (sox+1, &seln, NULL, NULL, &tout) <= 0)) {
			return 1;
		}
		size_t len = read (sox, &rbuf, sizeof (struct tun_pi) + sizeof (rbuf.data));
		uint8_t *pkt = rbuf.data;
		uint32_t pktlen = len - sizeof (struct tun_pi);
		if (pktlen > 18) {
			uint32_t mem [28];
			bzero (mem, sizeof (mem));
			retfn *rf = (retfn *) netinput (rbuf.data, len - sizeof (struct tun_pi), mem);
#if 0
			int i;
			for (i=0; i<28; i++) {
				printf ("  M[%d]=0x%08x%s", i, mem [i],
						((i+1)%4 == 0)? "\n": "");
			}
#endif
			if (rf != NULL) {
				uint8_t *start = wbuf.data;
				uint8_t *stop = (*rf) (start, mem);
				if (stop) {
					mem [MEM_ALL_DONE] = (uint32_t) stop;
					uint32_t len = mem [MEM_ALL_DONE] - mem [MEM_ETHER_HEAD];
					if (netcore_send_buffer (tunsox, mem, &wbuf) < 0) {
						printf ("Failed while sending reply data\n");
					}
				}
			}
		}
	}
	return 0;
}


int setup_tunnel (void) {
	tunsox = -1;
	int ok = 1;
	//
	// Open the tunnel
	tunsox = open ("/dev/net/tun", O_RDWR);
	if (tunsox < 0) {
		ok = 0;
	}
	//
	// Set the tunnel name to "test0cpm"
	struct ifreq ifreq;
	bzero (&ifreq, sizeof (ifreq));
	strncpy (ifreq.ifr_name, "test0cpm", IFNAMSIZ);
	ifreq.ifr_flags = IFF_TAP;
	if (ok)
	if (ioctl (tunsox, TUNSETIFF, &ifreq) == -1) {
		ok = 0;
	}
	//
	// Set to promiscuous mode (meaningless for a tunnel)
#if 0
	if (ok)
	if (ioctl(tunsox, SIOCGIFFLAGS, &ifreq) == -1) {
		ok = 0;
	}
	ifreq.ifr_flags |= IFF_PROMISC | IFF_UP;
	if (ok)
	if (ioctl(tunsox, SIOCSIFFLAGS, &ifreq) == -1) {
		ok = 0;
	}
#endif
	//
	// Cleanup and report success or failure
	if (!ok) {
		if (tunsox >= 0) {
			close (tunsox);
			tunsox = -1;
		}
		return -1;
	}
	return tunsox;
}



int main (int argc, char *argv []) {
	init ();
	int tunsox = setup_tunnel ();
	if (tunsox == -1) {
		fprintf (stderr, "Failed to setup tunnel\n");
		exit (1);
	}
	system ("/sbin/ifconfig test0cpm up");
	system ("/sbin/ifconfig test0cpm");
	sleep (10);
	if (!netcore_bootstrap ()) {
		fprintf (stderr, "Failed to bootstrap the network\n");
		exit (1);
	}
	process_packets (tunsox, 60);
	printf ("Done.\n");
	close (tunsox);
	return 0;
}

