/*
 * http://devel.0cpm.org/ -- Open source firmware for SIP phones.
 *
 * Network multiplexing for incoming traffic.
 */


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <0cpm/cpu.h>
#include <0cpm/irq.h>
#include <0cpm/netfun.h>


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

static irq_t recv_irq = {
	.irq_handler = network_recv_handler,
	.irq_next = NULL,
	.irq_priority = CPU_PRIO_UNKNOWN,
};
static irq_t send_irq = {
	.irq_handler = network_send_handler,
	.irq_next = NULL,
	.irq_priority = CPU_PRIO_UNKNOWN,
};



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
 * the reception of network packets.
 */
static void network_recv_handler (irq_t *irq) {
	bool go = can_recv;
	can_recv = false;
	// TODO: rescheduling would be better than looping
	while (go) {
		packet_t rbuf;
		uint16_t rbuflen = sizeof (rbuf.data);
		go = bottom_network_recv (rbuf.data, &rbuflen);
		if (!go) {
			break;
		}
		printf ("Received a network packet\n");
		intptr_t mem [MEM_NETVAR_COUNT];
		bzero (mem, sizeof (mem));
		retfn *rf = (retfn *) netinput (rbuf.data, rbuflen, mem);
		if (rf != NULL) {
			packet_t wbuf; // TODO: Allocate
			uint8_t *stop = (*rf) (wbuf.data, mem);
			if (stop) {
				mem [MEM_ALL_DONE] = (intptr_t) stop;
				netcore_send_buffer (mem, wbuf.data);
			}
		}
	}
	if (go) {
		can_recv = true;  // TODO: Future IRQ trigger?
	}
}


/* As the processor now has time for it, process
 * the sending of network packets.
 */
static void network_send_handler (irq_t *irq) {
	bool go = can_send;
	can_send = false;
	// TODO: Go over priority queues, bottom_network_send()
}
