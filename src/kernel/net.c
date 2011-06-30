/* Network multiplexing for incoming traffic.
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



/* This is the kernel
 * interface to the networking code; it mainly involves scheduling.
 */


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/irq.h>
#include <0cpm/netinet.h>
#include <0cpm/netfun.h>
#include <0cpm/cons.h>


/* TODO: Current return function from netinput
 */
typedef void *retfn (uint8_t *pout, intptr_t *mem);


/* The can_recv and can_send flags store whether
 * the network is in a state to permit either of
 * these operations on the network interface.
 */
static bool can_send = false;
static bool can_recv = false;


/* IRQ handling structures dealing with reads and
 * writes on the network card.
 */
static void network_recv_handler (irq_t *irq);
static void network_send_handler (irq_t *irq);

/* The network receive handler is fired as soon as network
 * packets arrive.  The network send handler is fired as soon
 * as network packets have completed sending.
 */
static irq_t recv_irq = { network_recv_handler, NULL, CPU_PRIO_UNKNOWN };
static irq_t send_irq = { network_send_handler, NULL, CPU_PRIO_UNKNOWN };



/* This upcall is used to report that the network
 * has new data available.  The bottom half will
 * call this for the first packet to arrive after
 * having failed an attempt to read input.
 * More calls are possible for later arrivals of
 * even more packets, and will be silently ignored
 * below.
 */
void top_network_can_recv (void) {
	if (!can_recv) {
		can_recv = true;
		irq_fire (&recv_irq);
//ht162x_led_set (15, 1, true);	// shown as (101)
	}
}

/* This upcall is used to report that the network
 * is certain to accept a full-size network packet.
 * It is called by the bottom half after having
 * failed an attempt to send output.
 * Additional calls on the same matter are silently
 * ignored.
 */
void top_network_can_send (void) {
	if (!can_send) {
		can_send = true;
		irq_fire (&send_irq);  // Polling, so safe
	}
}


/* This upcall is made to report that the network
 * has gone offline.  This may be detected at the
 * hardware level, as a result of unplugging the
 * cable and/or loosing sync.
 * The state resulting from this is implied at the
 * time of booting the device.
 */
void top_network_offline (void) {
	can_send = false;
	netcore_bootstrap_shutdown (); // TODO: wait 5 min, then call the user
}


/* This upcall is made to report that the network
 * has come online.  This is also called when the
 * network card has been initialised properly.
 */
void top_network_online (void) {
	top_network_can_send (); // Polling, so safe
	netcore_bootstrap_initiate ();
}

/* As the processor now has time for it, process
 * the reception of network packets.  Where simple
 * stimulus-response action is possible, do that.
 */
packet_t myrbuf;//TODO:ELSEWHERE//
packet_t mywbuf; // TODO: Allocate:ELSEWHERE
intptr_t mymem [MEM_NETVAR_COUNT];//TODO:ELSEWHERE//
static void network_recv_handler (irq_t *irq) {
intptr_t netinput (uint8_t *pkt, uint16_t pktlen, intptr_t *mem);
void ht162x_putchar (uint8_t idx, uint8_t ch, bool notify);
	bool go;
//TODO:TEST// ht162x_putchar (1, '0', true);
	go = can_recv;
	can_recv = false;
	// TODO: rescheduling would be better than looping
	while (go) {
		uint16_t rbuflen = sizeof (myrbuf.data);
		retfn *rf;
//TODO:TEST// ht162x_putchar (1, '1', true);
		go = bottom_network_recv (myrbuf.data, &rbuflen);
		if (!go) {
			goto done;//TODO//break;
		}
//TODO:TEST// ht162x_putchar (1, '2', true);
		//TODO:OVERZEALOUS// bottom_printf ("Received a network packet\n");
		bzero (mymem, sizeof (mymem));
		rf = (retfn *) netinput (myrbuf.data, rbuflen, mymem);
//TODO:TEST// ht162x_putchar (1, '3', true);
		if (rf != NULL) {
			uint8_t *stop = (*rf) (mywbuf.data, mymem);
//TODO:TEST// ht162x_putchar (1, '4', true);
			if (stop) {
//TODO:TEST// ht162x_putchar (1, '5', true);
				mymem [MEM_ALL_DONE] = (intptr_t) stop;
				netcore_send_buffer (mymem, mywbuf.data);
//TODO:TEST// ht162x_putchar (1, '6', true);
			}
//TODO:TEST// else ht162x_putchar (1, '7', true);
		}
	}
done://TODO
	if (go) {
//TODO:TEST// ht162x_putchar (1, '8', true);
		bottom_critical_region_begin ();
		if (!can_recv) {
			can_recv = true;
if (irq)
			irq_fire (irq);
		}
		bottom_critical_region_end ();
	}
//TODO:TEST// ht162x_putchar (1, '9', true);
}


/* As the processor now has time for it, process
 * the sending of network packets.
 */
static void network_send_handler (irq_t *irq) {
	bool go = can_send;
	can_send = false;
	// TODO: Go over priority queues, bottom_network_send()
}

