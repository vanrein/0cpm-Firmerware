/* tuntest.c -- Tunnel-based testing of the network code.
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
// #include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
// #include <errno.h>

// #include <sys/types.h>
// #include <sys/ioctl.h>
// #include <sys/socket.h>
// #include <sys/select.h>
// #include <sys/time.h>
// #include <sys/uio.h>

// #include <netinet/ip.h>
// #include <netinet/ip6.h>
// #include <netinet/udp.h>
// #include <netinet/ip_icmp.h>
// #include <netinet/icmp6.h>

// #include <linux/if.h>
// #include <linux/if_tun.h>
// #include <linux/if_ether.h>

#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/netfun.h>
#include <0cpm/netdb.h>
#include <0cpm/timer.h>
#include <0cpm/led.h>
#include <0cpm/netinet.h>
#include <0cpm/cons.h>


int tunsox = -1;

packet_t rbuf, wbuf;

timing_t timeout = TIMER_NULL;

bool sleepy = false;


/*
 * TEST OUTPUT FUNCTIONS
 */

uint8_t *net_arp_reply (uint8_t *pkt, uint32_t pktlen, intptr_t *mem) {
	bottom_printf ("Received: ARP Reply\n");
	return NULL;
}

uint8_t *net_arp_query (uint8_t *pkt, uint32_t pktlen, intptr_t *mem) {
	bottom_printf ("Received: ARP Query\n");
	return NULL;
}

uint8_t *net_rtp (uint8_t *pkt, uint32_t pktlen, intptr_t *mem) {
	bottom_printf ("Received: RTP\n");
	return NULL;
}

uint8_t *net_rtcp (uint8_t *pkt, uint32_t pktlen, intptr_t *mem) {
	bottom_printf ("Received: RTCP\n");
	return NULL;
}

uint8_t *net_sip (uint8_t *pkt, uint32_t pktlen, intptr_t *mem) {
	bottom_printf ("Received: SIP\n");
	return NULL;
}

uint8_t *net_mdns_resp_error (uint8_t *pkt, uint32_t pktlen, intptr_t *mem) {
	//TODO// bottom_printf ("Received: mDNS error response\n");
	return NULL;
}

uint8_t *net_mdns_resp_dyn (uint8_t *pkt, uint32_t pktlen, intptr_t *mem) {
	//TODO// bottom_printf ("Received: mDNS dynamic response\n");
	return NULL;
}

uint8_t *net_mdns_resp_std (uint8_t *pkt, uint32_t pktlen, intptr_t *mem) {
	//TODO// bottom_printf ("Received: mDNS standard response\n");
	return NULL;
}

uint8_t *net_mdns_query_error (uint8_t *pkt, uint32_t pktlen, intptr_t *mem) {
	//TODO// bottom_printf ("Received: mDNS error query\n");
	return NULL;
}

uint8_t *net_mdns_query_ok (uint8_t *pkt, uint32_t pktlen, intptr_t *mem) {
	//TODO// bottom_printf ("Received: mDNS ok query\n");
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
 * Environment simulation functions
 */

timing_t bottom_time (void) {
	struct timeval now;
	timing_t retval;
	if (gettimeofday (&now, NULL) != 0) {
		bottom_printf ("Failed to get clock time: %s\n", strerror (errno));
		return 0;
	}
	retval = now.tv_sec * 1000 + (now.tv_usec / 1000);
	return retval;
}

timing_t bottom_timer_set (timing_t nextirq) {
	timing_t retval = timeout;
	timeout = nextirq;
	return timeout;
}

void bottom_critical_region_begin (void) {
	// No real action, as this is already software
}

void bottom_critical_region_end (void) {
	// No real action, as this is already software
}

void bottom_sleep_prepare (void) {
	sleepy = true;
}

void bottom_sleep_commit (sleep_depth_t depth) {
	if (!sleepy) {
		return;
	}
	fd_set seln;
	FD_ZERO (&seln);
	FD_SET (tunsox, &seln);
	struct timeval seltimeout;
	timing_t tm = 0;
	if (timeout != TIMER_NULL) {
		gettimeofday (&seltimeout, NULL);
		tm = seltimeout.tv_sec * 1000 + seltimeout.tv_usec / 1000;
		if (timeout < tm) {
			timeout = TIMER_NULL;
			top_timer_expiration (tm);
			sleepy = false;
			return;
		}
		tm = timeout - tm;
		seltimeout.tv_sec  =  tm / 1000        ;
		seltimeout.tv_usec = (tm % 1000) * 1000;
	}
	int sel = select (tunsox+1, &seln, NULL, NULL, &seltimeout);
	if (sel >= 1) {
		top_network_can_recv ();
		sleepy = false;
	} else if (sel == 0) {
		gettimeofday (&seltimeout, NULL);
		timing_t exptm = (seltimeout.tv_sec * 1000) * (seltimeout.tv_usec / 1000);
		timeout = TIMER_NULL;
		top_timer_expiration (exptm);
		sleepy = false;
	} else {
		bottom_printf ("select() returned an error: %s\n", strerror (errno));
		sleep (1);
	}
}

bool bottom_network_send (uint8_t *buf, uint16_t buflen) {
	struct tun_pi *tunpi = (struct tun_pi *) &buf [-sizeof (struct tun_pi)];
	tunpi->flags = 0;
	tunpi->proto = ((struct ethhdr *) buf)->h_proto;
	size_t wsz = write (tunsox, buf - sizeof (struct tun_pi), buflen + sizeof (struct tun_pi));
	if (wsz == -1) {
		if (errno == EAGAIN) {
			return false;
		} else {
			bottom_printf ("Ignoring error after write to tunnel: %s\n", strerror (errno));
		}
	}
	return true;
}

bool bottom_network_recv (uint8_t *buf, uint16_t *buflen) {
	uint16_t bl = (*buflen) + sizeof (struct tun_pi);
	size_t rsz = read (tunsox, buf - sizeof (struct tun_pi), bl);
	if (rsz == -1) {
		if (errno == EAGAIN) {
			return false;
		} else {
			bottom_printf ("Ignoring error after read from tunnel: %s\n", strerror (errno));
		}
		*buflen = 0;
	} else if (rsz > sizeof (struct tun_pi)) {
		*buflen = rsz - sizeof (struct tun_pi);
	} else {
		*buflen = 0;
	}
	return true;
}


/*
 * TEST SETUP FUNCTIONS
 */

typedef void *retfn (uint8_t *pout, intptr_t *mem);

int process_packets (int secs) {
	int ok = 1;
	time_t maxtm = secs + time (NULL);
	while (ok) {
		fd_set seln;
		FD_ZERO (&seln);
		FD_SET (tunsox, &seln);
		struct timeval tout;
		tout.tv_sec = maxtm - time (NULL);
		tout.tv_usec = 500000;
		if ((tout.tv_sec < 0) || (select (tunsox+1, &seln, NULL, NULL, &tout) <= 0)) {
			return 1;
		}
		size_t len = read (tunsox, &rbuf.data [-sizeof (struct tun_pi)], sizeof (struct tun_pi) + sizeof (rbuf.data));
		uint8_t *pkt = rbuf.data;
		uint32_t pktlen = len - sizeof (struct tun_pi);
		if (pktlen > 18) {
			intptr_t mem [28];
			bzero (mem, sizeof (mem));
			retfn *rf = (retfn *) netinput (rbuf.data, pktlen, mem);
#if 0
			int i;
			for (i=0; i<28; i++) {
				bottom_printf ("  M[%d]=0x%08x%s", i, mem [i],
						((i+1)%4 == 0)? "\n": "");
			}
#endif
			if (rf != NULL) {
				uint8_t *start = wbuf.data;
				uint8_t *stop = (*rf) (start, mem);
				if (stop) {
					mem [MEM_ALL_DONE] = (intptr_t) stop;
					netcore_send_buffer (mem, wbuf.data);
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
	tunsox = open ("/dev/net/tun", O_RDWR | O_NONBLOCK);
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
		bottom_printf ("Failed to setup tunnel\n");
		exit (1);
	}
	system ("/sbin/ifconfig test0cpm up");
	system ("/sbin/ifconfig test0cpm");
	sleep (10);
	top_main ();
	bottom_printf (stderr, "top_main() returned -- which MUST NOT happen!\n");
	close (tunsox);
	exit (1);
}

