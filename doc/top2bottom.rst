TODO: top_main() can return to invoke "next boot option" cyclically
TODO: bool bottom_phone_is_offhook (void);
TODO: TIME_MSEC, TIME_SEC, TIME_MIN, TIME_HOUR, TIME_DAY
TODO: TIME_BEFORE(a,b)
TODO: bottom_show_xxx() series of calls
TODO: bottom_xxx_scan() and top_xxx() from develtests
TODO: top_network_can_xxx() will be called before 1500 bytes free but then certainly called again before done
TODO: bottom_network_send/_recv() accept 32-bit transfers
TODO: bottom_flash_read()/_write() to interact with flash memory partitions sequentially
TODO: bottom_flash_partition_table [0:HAVE_FLASH_PARTITION_TABLE>
TODO: bottom_flash_get_mac (uint8_t mac [6]) sets the MAC address at the buffer address
TODO: top_timer_expiration MUST call bottom_time and bottom_timer_set (or return new timer expiration value)

----------------------------------------------
Kernel API between Phone Bottom and Top Halves
----------------------------------------------

This is the porting guide that explains what parts
of 0cpm should be changed in order to make it run on
new kinds of hardware.

The 0cpm open source firmware for SIP phones is split
into a device-dependent part (the bottom half) and a
device-independent part (the top half).  These parts
interact in the manner described below.

	+----------------+----------------+
        |                |  Applications  |
        |  Top           +----------------+
        |                |  Kernel        |
	+----------------+----------------+
        |  Bottom                         |
	+---------------------------------+
        |  Hardware                       |
	+---------------------------------+

The hardware drivers ideally consists of two kinds of
module: chip-drivers that work the same in any device,
and phone-specific functions to handle the interconnect,
that is, the addressing of the generic chips on the
concrete printed circuit board.


Making changes to this phone firmware
=====================================

Here are a few guidelines to help you on your way with this code.


Never mix top and bottom half code
----------------------------------

When porting this phone firmware to new hardware,
you should never have to change code in the top half
of the software.  If you feel a need to do so, please
contact the developers, because you will either have
misunderstood something about the structure, or you
will have run into a situation that needs general
treatment in this interface definition.

If you additionally feel the need to extend the top half
application for your own purposes, please be sure to
keep the patches completely separate.  This helps to
maintain a clear structure at all times.  We actively
maintain this interface definition in a way to support
this separation of concerns.  Note that even though we
try to maintain clarity with separate patches to top
and bottom halves, it still forms one program that
cannot be separated.


Source tree structure
---------------------

The source tree contains a few dedicated directories
into which bottom half code should go.  Each original
manufacturer of hardware has its own directory, so as
to avoid clashing code.  This allows you to write code
that is highly optimised for your use of the hardware.
Even when the same processor architecture is used, it is
still possible to embed knowledge about the rest of the
hardware in your code.

The following directories are designed for bottom half
code, where MANUF should be replaced with your company
name, or if it is not globally known or unique, your
domain name.  Within the company, choose ``DEVICE``
as a shorthand name to name a device or line of devices
that can share the same binary firmware.

* ``src/bottom/MANUF/`` contains the C and/or Assembly
  code for your bottom half.  It is probably a good idea
  to setup a subdirectory per line of device, and one or
  more with common code.

* ``src/bottom/MANUF/DEVICE/`` must contain a ``Makefile``
  for use with GNU Make.
  TODO: Or rules?

* ``src/bottom/MANUF/include/`` contains the C headers
  that describe how the C compiler from your toolchain
  displays the data types exchanged with the bottom half.
  At the very least, the directory contains a header file
  for each product (or line of products) that share the
  same bottom half code.

* ``src/bottom/MANUF/DEVICE/include/`` must contain a
  single file ``config.h`` that defines the configuration
  of that type of device in terms of general compile-time
  parameters for the top half.  This includes bottom-compatible
  data type definitions used on the interface between the
  bottom and top halves.

You can just create and fill these directories when you start
porting.  Your porting patch should only contain changes in
these directories.  Please submit the toolchain as a separate
package; you should not incllude it in the source tree.

In additon, there are a few include files that define the
upcall and downcall interface:

* ``include/0cpm/downcall.h`` contains the functions that must
  be implemented in all bottom halves.

* ``include/0cpm/upcall.h`` contains the only functions that a
  bottom half may invoke on the top half.

The data types exchanged between bottom and top halves should be
declared as part of the configuration file for the bottom half.
This ensures compatibility of these data types with the bottom half
code, also when it is written in Assembly language.

TODO: Configuration information for ``make menuconfig``.


Open source licensing
---------------------

You are welcome to sell your ported or otherwise changed
phone firmware as part of a phone or as a separate piece
of software, but do note that the GNU Public License on
this code imposes a legal obligation to publish all
changes at no cost, along with a free toolchain to build
the code for your platform.  You should also make it
possible for your users to migrate to versions of the
firmware that were not created by you.

The idea behind the use of the GPL is that you
will gain much more from others than you will need to
share with them; that is the central idea that makes
open source such a powerful tool in commerce.  The legal
obligation exists to ensure that nobody can profit from
your work without sharing their improvements with you,
and that works both ways.

Needless to say, you are under
no obligation to support firmware versions that you did
not create as part of your production, sales or support.


Upcalls and Downcalls
=====================

A function invocation from bottom half to top half is
called an upcall; the functions supporting this are
named ``top_XXX`` where ``XXX`` varies with the function
being called.

A function invocation from top half to bottom half is
called a downcall; the functions supporting this are
named ``bottom`` where ``XXX`` varies with the function
being called.

Upcalls are usually the result of some hardware-initiated
event, such as an interrupt.  The idea of an upcall is
to enable the top half to process newly arrived information,
or a timeout, or whatever caused the upcall.

Downcalls are usually made to access drivers, that is
to achieve some generic effect in a device-dependent
way.

Upcall handler code may never invoke downcalls, and
similarly, downcall handler code may never invoke
upcalls.  If there is a need to trigger an effect in
the layer that called you, you should set it up in some
queue for later processing.


Initialisation
--------------

When the phone is started, the bottom half starts up in
a way that is determined by the hardware alone.  It will
do minimal setup of the hardware, but only enough to make
all future downcalls possible.

The top half supports a function that will be started by
the bottom half after initialisation::

	void top_main (void);

Although it follows the same naming convention, this
is not really an upcall, in that it may actually spend time,
block waiting and make downcalls.

The top half ensures that this function never ends, so
the bottom half can jump to it, rather than call it as
a subroutine.  It is assumed that a sufficiently large
stack has been setup by the bottom before this is done.
To ensure that the stack cannot run out, the top will
refrain from recursive calls.


Asynchronous upcalls
--------------------

Upcalls occur due to external events such as when network packets
arrive, when timers expire, or when a user operates a button.  It
is generally good to process any such situations as soon as possible,
and not to have to poll them.

Since the bottom half usually receives asynchronous events
as interrupts, it gains temporary control over the processor
in an asynchronous manner.  It is possible to format the
information to be communicated in a standard format and
make an upcall with it, but the upcall must be setup to
touch as little of the data structures as possible, that is,
to be as supportive as possible towards asynchronous calls.
Also, the upcall service function ``top_XXX`` is supposed
to return very, very quickly and never to block on any
condition.

TODO: Allow parallel upcalls?  (a) same type, (b) diff type?

The top half may block asynchronous upcalls for short
periods.  This implements a so-called critical region,
where the top half manipulates data that is also handled
during an upcall.  The top half should never block in a
critical region, nor should it do complex things.  The
functions supporting asynchronous upcall blocking are::

	void bottom_critical_region_begin (void);
	void bottom_critical_region_end   (void);

The definition of these functions may well be an
assembler inline function to disable and enable interrupts.
Being bottom calls, these functions may not be invoked
in an upcall.

The code structure for a critical region is::

	#include <bottom.h>

	// non-critical code
	bottom_criticital_region_begin ();
	// critical region
	bottom_criticital_region_end ();
	// non-critical code

When the bottom invokes ``top_main()``, it has not yet
enabled asynchronous upcalls, so after some setup this
function must start by releasing the critical region::

	#include <stdbool.h>
	#include <bottom.h>

	void top_main (void) {
		// top-half setup code
		bottom_critical_region_end ();
		while (true) {
			// main loop, normal operation
		}
	}


Synchronous downcalls
---------------------

Downcalls are always synchronous in nature.  The top half is
a single task, and as upcalls may never make downcalls, it
is safe to assume that the downcall code need not be
re-entrant.  This leads to a simplification of complexity in
the bottom half.  As a result, porting the application to
other platforms should be limited in complexity to the
idiosynchracies of the target platform.


Kernel scheduling
=================

The kernel implements a scheduling discipline that fulfills
the following constraingts:

* soft realtime scheduling
* tickless scheduling inasfar as possible on the hardware
* event scheduling instead of process switching
* priority-levels separate expedited events from background work
* applications support their own precedence order for user interaction


Sleeping top-half
-----------------

If the top half has no work to do, it can rest by asking
the bottom half to sleep until the next need for an upcall.
It does this in two stages, to make sure that no race
conditions occur due to upcalls between the check for no
more work in the top half and asking the bottom half to
yield until the next upcall::

	#include <bottom.h>
	
	bottom_sleep_prepare ();
	if ( /* nothing to do in the top half? */ ) {
		bottom_sleep_commit (SLEEP_SNOOZE);
	}

If it turns out that there is work to be done after calling
``bottom_sleep_prepare()``, then there is no need to cancel
anything.  The next invocation will simply prepare once
again.  The most likely implementation is a flag that is set
during ``bottom_sleep_prepare()`` and cleared by any invocation
of an upcall.  The ``bottom_sleep_commit()`` will atomically
check the flag and only sleep while the flag is set.
So, if an upcall occurred between ``bottom_sleep_prepare()`` and
``bottom_sleep_commit()`` then the latter will return immediately.
This way, the top half program acts as if it was just woken up
on account of a newly processed upcall.

There are two levels of sleep that the top half can request
from the bottom half.  Snoozing is requested while calls are in
progress, and a quick wakeup is anticipated.  Hibernation is
requested when there are no active calls, and a long waited is
anticipated until a major event such as a key press or an incoming
network packet is needed before the phone should wakeup again.
The two forms use ``SLEEP_SNOOZE`` and ``SLEEP_HIBERNATE`` as
flags to ``bottom_sleep_commit()``.

The idea is that the bottom half opts for a sleep mode with high
responsiveness to interrupts when ``SLEEP_SNOOZE`` is requested,
whereas the choice for ``SLEEP_HIBERNATE`` may take some recovery
time.  For example, hibernation could involve stopping the clock
for the processor, while hibernation may not do anything but stop
the intake of instructions.


Sleeping bottom-half
--------------------

It is possible to keep a top half sleeping even if there is
a lot of activity in the bottom half.  For example, the
bottom half can continuously scan the keyboard but not
report anything through an upcall if nothing was pressed
or released.

In general, it is a principe of good design to look for
ways to conserve energy; a phone is always switched
on, and scanning buttons continously is basically a sign
of bad design.  If the buttons are laid out in a matrix with
input columns and output rows, it is probably good to only
trigger on changes; when no button is pressed, a design may
support selection of all columns at the same time, and
wait until either row changes state.  Then it may wait a
small time to enable debouncing.  When a key is pressed,
it is usually sufficient to select only its column and wait
until its row changes state back to the unpressed state.
The only thing left then is to await a debouncing interval.

Polling, in general, is a bad idea for a phone that spends
most of its time doing nothing.  In short, it is a good idea
to design the bottom-half for tickless operation.  The
top-half will actively request timing, the network will
raise an interrupt for incoming traffic, and nothing further
is needed to keep the processor running.  A sleeping phone
is a low-power phone, and it is usually straightforward to
embed that desire into a piece of hardware.


Top-half structure
------------------

Most data processed in the top half will be allocated statically.
Instead of dynamic allocation routines for an unknown number of
calls, is it safe to assume that no more calls can be processed
than the number of lines on a phone, or perhaps two if the phone
has a flash button instead of line buttons.  It is just an example,
but it is generally expected that structures can be allocated at
compile-time.

The top-half software is single-tasking.  This is possible by
making it event-driven, and have a scheduler to handle queues
of events, each of which are delivered when it is their time.
Examples of events range from time expiration to incoming
phone calls, and their targets range from LED-flashing routines
to connection-establishing SIP routines.  In all cases, the
communication is through events.

The software recognises a number of applications, each of which
process their own events, and run in parallel on top of a
simple kernel.  The task of the kernel is to provide the core
mechanisms used in support of all applications, and it ranges
over event handling, deliver and scheduling, as well as
resource allocation and application dynamicity.  Outside the
kernel, there are no support routines for upcalls, nor is it
permitted to make downcalls such as interrupt blocking.

The scheduler for the top-half handles events at priority
levels.  This ensures that the most important events are
handled immediately, 


Top/Bottom API primitives
=========================

The bottom half implements generic drivers, and must therefore
deliver preprocessed information to the top half when making
an upcall, as well as processing generic information when it
receives a bottom call.  The adaption should be trivial, but
it is nonetheless good to understand their design motivations.


LEDs
----

Most phone support LEDs in a variaty of shapes and locations.
The phone's configration provides details, and assigns each
phone a unique code in a gap-less range of index numbers.
These indexes should be used to identify a LED on all
communication between top and bottom halves.  More to the
point, since LEDs do not generate upcalls, the numbering
should always be used during downcalls.

LEDs can display a number of colours, ranging from 0 for the
least intrusive colour to a higher number for the most
intrusive one.  The configuration specifies the highest
number available.  Colour 0 is always the off state for a
LED, and examples of other colours could be 1 for green
and 2 for red, or on another LED it could be 1 for green,
2 for amber and 3 for red.

The function ``bottom_led_set (led, col)`` is defined to set
a LED with index number ``led`` to colour number ``col``.

The top half will arrange for LEDs to flash at a regular
pace with a 50% duty cycle.  The top half has functions
to construct a flashing pattern, but the bottom half is not
expected to support flashing LEDs.

If a display with backlight is configured, then the backlight
LED will be defined as any other LED, with its own symbol and
index number.  If the light intensity can be arranged in a
number of steps, then the colouring scheme will show the
number of grades, ranging from off at 0 to the brightest at
the highest colour value specified for the backlight LED.


Buttons
-------

Buttons only make upcalls.  If a LED is attached to a button,
then the phone configuration describes that fact and the
application logic in the top half will work accordingly.

Buttons are grouped for practical purposes, as follows:

* DTMF keys: ``0`` to ``9``, ``*``, ``#`` and ``A`` to ``D``
* Function-bound keys like Hold, Transfer, Flash, Menu or Up/Down.
* Line buttons, positioned to manage lines/accounts/calls.
* Soft function buttons, usually positioned under a display.
* User programmable buttons, usuable for speed dial and so on.

The configuration files specify which are available, and
how many of the various classes.  The bottom half is
expected to setup translation tables from hardware inputs
to the button class, and with the class the instance,
conforming to the configuration.  The upcalls that report
button actions are::

	void top_button_press (buttonclass_t bcl, buttoncode_t cde);
	void top_button_release (void);

Debouncing the hardware is part of the bottom-half code, but
timer-based repeats and even the processing of overlapping
button presses are part of the top-half logic.  The upcall
that reports a button being pressed implies that any other
ones are released, even if this may not reflect what the
hardware detects.  The ability to decode multiple buttons
pressed at the same time is so dependent on hardware that
the top half should refrain from interpreting such situations.
Furthermore, this is not commonly done for phone keyboards.
This is also why the ``top_button_release()`` function has
no arguments -- everything that may still be thought of as
being pressed should be released after this call.


Timers
------

General frameworks for timing tend to facilitate two kinds of
timers; oneshot timers for a single delay, and interval timers
that cause a timeout event at fixed intervals.  We combine
both these kinds of timer in one concept.

Timers are used to enable tickless realtime operation; in other
words, there is no need for a regular timer interrupt in the
top-half code, but if the application needs to wait a specific
time it will simply create a timer that satisfies the
application-desired waiting period.  The top half operates a
queue of timer requests, and will send the shortest wait time
down for implementation in a hardware timer.

The bottom half is also expected to support a clock, from which
a unix timestamp can be read, so the number of seconds since the
epoch.  This is expected from the following downcalls to get and
set the clock time::

	uint32_t unixtime bottom_clock_get (void);
	void bottom_clock_set (uint32_t unixtime);

The top half will usually employ a protocol like SNTP to obtain
the current time, and set the device clock accordingly.  The
reason that the bottom half is involved is that it usually has
the facilities to include realtime timers and thereby avoid
code in the top half that would need to tick away once a second.
This is especially useful for mobile devices that want to track
time with the least possible power expenditure.

Timeouts cannot be defined at a second granularity.  It is
advised to use a millisecond granularity, as that captures the
most detailed time measurements that a phone could handle.  It
will usually be possible for hardware to accommodate such timing,
but just in case this is not true there is a possibility in the
configuration files to specify multiplication factors for timing.

The current unixtime should be taken into account when setting
a timer, even if that means that the range of an ``uint32_t``
will be exhausted.  The overlapping part will be taken to
apply, however.  At millisecond precisions, that means that
time stretches of up to 24 days in the past and future can be
represented.  That easily suffices for a phone.  The bottom half
should select such a precision that times of up to a day in the
past or future can be represented.  The function definition for
setting the next timer interval is::

	timing_t bottom_timer_set (timing_t timeout);

The value returned is the previous value in the timer.  The special
value ``TIMER_NULL`` is used to represent no timer setting.  By
setting the timer to that value, it will stop running.  If the
timer returns that time, then the timer was not using before.  When
the bottom half invokes ``top_main()`` the timer is not running
yet, so the first invocation of ``bottom_timer_set()`` will return
``TIMER_NULL``.  If the time returned is a valid time, the top
half may assume that the timer has not expired on that time.  In
other words, the top half must either have the value as a later
entry in its queue, or it must process the timeout returned.

When the timer expires, it will make an upcall.  One of the tasks
of this upcall is to return the next timer setting, usually taken
from the next element in the timer queue maintained in the top
half.  The bottom half will immediately check if the new timer
expiration has already passed, and if so, it will make another
upcall on that time, asking for yet another timeout.  The upcall
is made as follows::

	uint32_t top_timer_expiration (uint32_t timeout);

The value sent up is the current timer expiration setting, and the
value expected in return is the new expiration setting.  The
function argument will never be ``TIMER_NULL``, but the value
returned may be, to indicate that no more timeouts are currently
requested.

The normal course of action in ``top_timer_expiration`` is to do
two things: First, schedule an event for top-half handling, and
second, return the timer expiration value for the next timeout
in the timer's queue.


Network
-------

Network events relate to a few events:

* Network connectivity going offline and coming online
* The arrival of a network packet
* The ability to send another network packet

When booted, the network connectivity is assumed to be
down.  Upon activation of upcalls, a check is made to
see if network connectivity is live, and if so, the
corresponding upcall is made to inform the top half.

When the network goes online, the upcall made is
``top_network_online (void)``.  Conversely, when it
goes offline, the call made is
``top_network_offline (void)``.  Where a difference
between uplink and downlink can be detected, these
calls apply to the uplink, and the downlink is
ignored.  Switching between downlink and uplink
is part of the bottom half responsibilities.

The bottom half is also the interface that permits or
stops network reads and writes, as a result of available
buffer space and arrival of traffic.

The following function reports arrival of a packet
over the network::

	void top_network_can_recv (void);

This function must be called when a packet arrives
while there were none available before that.  The
function may also be called for any subsequent
arrives.  This optimally reflects the variations in
available hardware, and it should not be a problem
if the implementation of this top function only does
a few simple administrative things.  The subsequent
handling of network packets is (of course) done in a
polling loop, to handle the uncertainty of the number
of available packets.

Mirrorring this function, the following reports the
ability to send a packet over the network::

	void top_network_can_send (void);

The buffer space available should be at least the MTU
for the network, so 1500 plus ethernet headers.  The
optimal position may however be different.  The function
need not be called when the network interface comes up.

Any high-priority traffic is always immediately offered
for sending, and if that fails it is queued along with
the lower-priority traffic until this interrupt function
is called.  If the network card has internal buffer
space, this is good to use.  However, it makes no sense
to simulate it in main memory at the driver level, as
the top half can do that instead.

To receive packets from the network interface, the following
function is called::

	bool bottom_network_recv (uint8_t *pkt, uint16_t *pktlen);

This function only returns ``true`` if a packet was
available.  A good cause for failure is not having any
packets available for reception.  The ``pkt`` will
be filled with at most ``pktlen`` bytes of data,
and the latter variable will be update upon return to
reflect the length used.  When reporting failure, the
value returned in ``pktlen`` is unchanged and the
``pkt`` may have changed over that length; the results
of that however, are not reliable for processing.

In case of an error, for example a false checksum,
the function returns ``true`` but sets ``pktlen``
to 0.  This means that further polling is sensible.
Some code to poll and process network packets would
be::

	uint16_t buflen;
	uint8_t buf [MTU + 16];
	bool more = true;
	while (more) {
		buflen = sizeof (buf);
		more = bottom_network_recv (buf, &buflen);
		if (buflen > 0) {
			process_packet (buf, buflen);
		}
	}

After the ``bottom_network_recv`` has returned ``false``,
the top layer will await invocation of ``top_network_can_recv``
before it tries again.

The function to send data out to the network card is
the mirror image of receiving::

	bool bottom_network_send (uint8_t *pkt, uint16_t pktlen);

This function only returns ``true`` if the packet could
be sent to a point where the network takes over.  The
place where this would be is decided by the bottom layer,
but the more certainty this can give about internal
processing, the more reliable the entire application gets.

It is possible to submit multiple packets, until no more
could be delivered.  After the ``bottom_network_send``
has returned ``false``, the top layer will await
invocation of ``top_network_can_send`` before it tries again.

Some example code, not taking priorities into account, that
would write data out to the network would be::

	qitem = netsendqueue;
	bool more = true;
	while (more) {
		if (!qitem) {
			break;
		}
		more = bottom_network_send (qitem->buf, qitem->buflen);
		if (more) {
			qitem = qitem->next;
		}
	}
	//...free netsendqueue items up to but excluding qitem...
	netsendqueue = cursend;

The mechanisms for dealing with the upcalls are simple
enough; a flag can be used that is reset just before
making the networking downcall, and that is set by the
corresponding upcall.  This way, no signalling is ever
lost.  This is not shown in the examples above.


Display
-------

TODO -- probably a generic format for the capabilities of a device,
so that the kernel can make choices.  It is possible that a display
represents multiple partial display resources; on a LCD-display there
may be a part for number display, another for times, and a few dedicated
symbols to represent state information; on a graphical display, parts of
the screen may be reserved for softbuttons and/or line buttons.


Sound
-----

**Channels.**
A phone can have a number of sound channels, and the configuration
of the platform defines which are available.  Each of the channels
usually has a device attacheed; the possible devices are:

* Handset.  This is the only obliged channel for sound I/O.
* Speaker.  This usually combines with a microphone, although it
  is possible that the handset microphone doubles with this function.
  The bottom half should present this as a bidirectional channel and
  hide any such choices.
* Headset.

Conceptually, a channel has an input and output aspect.  These are
handled in same-sized blocks of samples.  An input and output sample
is imagined to be clocked at the same instant, even if this might in
practice deviate somewhat; but having such a conceptual model is a
great advantage to echo cancellation.

So, the normal procedure of playing sound is to run the decoder for
the playing codec, clock out a block of sound and at the same time
clock in a block of recorded sound.  The recorded sound has echo
cancellation applied, and is subsequently fed into the encoder for
the recording codec.

Echo cancellation and codec algorithms are *principally* run in the
top-half, but it is recommended to override them (or their most-used
parts) with machine code written specifically for the target platform;
overrides exist to support that.  TODO:WHICH?HOW?

The top half can make the following downcall to instruct the bottom
half about the current sound channel to use.  This implies dropping
any channel currently in use::

	void bottom_soundchannel_device (uint8_t chan, sounddev_t dev);
	bottom_soundchannel_device (PHONE_CHANNEL_TELEPHONY, SOUNDDEV_NONE);
	bottom_soundchannel_device (PHONE_CHANNEL_TELEPHONY, SOUNDDEV_HANDSET);
	bottom_soundchannel_device (PHONE_CHANNEL_TELEPHONY, SOUNDDEV_SPEAKER);
	bottom_soundchannel_device (PHONE_CHANNEL_TELEPHONY, SOUNDDEV_HEADSET);

The first argument represents the sound channel index.  It is
currently assumed that one such channel exists, but future versions
of the software may support multiple, if the hardware can handle
it.  This is the case with some codec chips used in phones, and
may well pave the way for additional functions for the hardware.

Naturally, the bottom half will never be asked to support a sound
channel that it has not made available in the phone's configuration.

**Volume.**
Every sound channel has its own volume setting.  This value may
vary depending on the current use of the channel; if it plays a
ringtone it may be set to a higher volume than during a call.
These settings are made by the top half, and incremented by one
at a time.  The setting 0 represents a muted channel, any higher
value can be suggested by the top half to the bottom half.  If
the suggestion is too high, the top half will reduce it to the
maximum setting for the channel.  The top half must not keep
its own idea of the volume, but instead read it from the sound
channel.  Only when switching the nature of the traffic on the
sound channel could it be retrieved and stored literally::

	void bottom_soundchannel_setvolume (uint8_t chan, uint8_t vol);
	uint8_t bottom_soundchannel_getvolume (uint8_t chan);

As before, the channel code is currently always set to 0, and
it may develop to more possible values in some later version.

The function ``bottom_soundchannel_setvolume`` will not only
detect and correct increments beyond the maximum value, it will
also detect and correct wrap-around in an attempt to go below
the zero volume, or mute.

**Sample rate.**
The bottom half is responsible for playback at as accurate a rate
as possible.  Usually, this will mean using DMA to send samples
out at a predetermined rate, which derives from an accurate crystal
clock and may pass through PLLs and dividers before yielding the
desired frequency.

The frequencies to use are usually pretty standard; for example,
8 kHz for many G.7xx codecs, and 16 kHz or 32 kHz for the ones
with higher quality.  These are important to support accurately,
as deviations might be audible and disrupt normal phone operation.

For higher-end uses, such as playback of MP3, Vorbis or AAC, there
may be a need for other sample rates.  It is probable that 48 kHz
works without problems, but 44.1 kHz (the CD sample rate) is
almost always going to be a problem -- the frequency is composed
of numerous prime factors,
44100=2\ :sup:`2`\ .3\ :sup:`2`\ .5\ :sup:`2`\ .7\ :sup:`2`\ --
what a cruel joke, as it usually very hard to get all these
factors into the operating frequency of a general purpose device,
and so there is going to be some delay from time to time.

It is highly recommended to pay attention to the direction of
rounding the divisor(s) for the sample frequency; in case no
exact divisor exists, it may be harmful to round one way or the
other, depending on the hardware architecture.  The bottom half
is the place where such knowledge resides, and should be isolated
from the top half.

For example, the best approach may be to set a slightly lower frequency
and use counters to detect when a single sample should be
tossed out of the mix.  If 44000 Hz is achievable, this would
mean that 1 out of every 441 samples would have to be dropped.
The opposite, namely the duplication of a sample needed as a
result of a sample rate that is too high, may be easier where
the sample-handling hardware supports it.  The choice can be
made in an optimal way for the hardware used, as it is all
concealed in the bottom layer which is aware of the hardware
involved.

The result is that every possible sample rate may be set, but
that choosing common ones is still a good idea.  To generalise
this a bit, the bottom half exports two functions that help to
determine if a samplerate would be merely acceptable, or if it
would actually be preferred.  All preferred sample rates are
always acceptable::

	bool bottom_soundchannel_acceptable_samplerate (uint8_t chan, uint32_t samplerate);
	bool bottom_soundchannel_preferred_samplerate  (uint8_t chan, uint32_t samplerate);

The bottom must be configured so that at least the commonly used
sample rate of 8000 Hz is preferred; and inasfar as they are
acceptable, the additional sample rates of 16000 Hz and 32000 Hz
should also be preferred -- but that is not a must, as additional
parameters such as coding efficiency may influence such choices.

During SDP negotiations, the top half can first try to find an
offer with a preferred sample rate and, failing that, fall back
to the reduced quality of an acceptable sample rate.  An example
of this behaviour could be a phone acting as a web radio; it may
be offered the popular sample rate 44100 Hz that is impossible to
configure accurately on most phones; this may still be played if
it is an acceptable sample rate, but if 48000 Hz is also offered
and setup as a preferred frequency because it can be configured
accurately, then that stream would be preferred.  That rate is
much more likely to be preferred on common phone hardware, which
is usually engineered to support 8000 Hz rates, and a limited
list of multiples.

Since playback and recording of sound occur in lockstep, there
is one bottom call to set it for both playback and recording::

	void bottom_soundchannel_set_samplerate (uint8_t chan,
		uint32_t samplerate, uint8_t blocksize,
		uint8_t upsample_play, uint8_t downsample_record);

The ``samplerate`` is denominated in Hz, so in samples per seconnd.
It should only be set to values that are known to be at least
acceptable to the bottom drivers and hardware.
The ``blocksize`` indicates how many raw samples should be treated
as a block; this is likely to be influenced by the codec(s) used.
The bottom half will usually contain a buffer for playback and
recording, and its size will be divided by the blocksize parameter
to determine how many blocks are available in the buffer.  It is
advised that the ``blocksize`` is a multiple of 40, possibly even
of 80; the latter represents 10 ms of sound at 8 kHz, so a common
standard size of block to handle at once in generally used codecs.

In the future, we may choose to support upsampling the play
and/or downsampling the record side, for instance to support
playback at 16 kHz and recording at 8 kHz; but at
present, the values of ``downsample_play`` and ``downsample_record``
must be set to 1 to avoid using that functionality.  Current
bottom implementations may simply ignore these values, or require
that they are 1.

**Codec play/record.**
For the actual exchange of sound, a mapping from codec format to
the internal format used for playback (usually 16-bit samples)
is required.  This is the work of the codec, which principally is
part of the top half because it is generic application logic;
however, it is quite possible that a more optimal implementation
exists, using knowledge only available in the bottom half.  The
top-half code contains mechanisms to replace the generic code
from the top half with any such more optimal code.

A structure is defined in ``<0cpm/codec.h>`` to contain the inner
data storage of any codec selected, and to point to the functions
that initialise, encode/decode, and finalise the codec.  These
structures are used as a generic API through which to access the
codec's facilities.  The data storage ensures that the top layer
can abstract from details such as pointers midway a bitfield.  As
far as the top level is concerned, encoded bytes are consumed or
produced in portions of whole bytes, and samples are produced or
consumed in portions of an entire signed 16-bit integer.

Bytes setup for playback go through the following phases:

1. They are setup in the playback buffer by a decoder

2. They are surrendered to the playback facility

3. During playback, the same-timed samples are also recorded

4. The playback buffer and recorded buffer are used for echo suppression

6. The recorded samples are processed by an encoder

6. The playback buffer and recording buffer are released

In practice, there will be a playback process and a recording
process; the playback buffer claims access to additional blocks
in the buffer and the recording buffer causes blocks to be freed
after they have been processed.  Internally, a block may either
be free for playback/recording, be actively read/written by DMA,
or may be locked for echo cancellation and recorded-sound
processing.  The allocation of play and recording buffers happen
in lockstep.

The encoder and decoder are aware of the blocksize that has been
claimed for the channel; so they will claim whole blocks, process
them and release them.  To aid efficiency, there are no access
mechsnisms for individual samples.  The corresponding downcalls
to administer claiming and releasing sample blocks are::

	int16_t *bottom_play_claim (uint8_t chan);
	int16_t *bottom_echo_claim (uint8_t chan);
	int16_t *bottom_record_claim (uint8_t chan);
	void bottom_play_release (uint8_t chan);
	void bottom_record_release (uint8_t chan);

The claim routines return a pointer to a block-sized array of
samples, each as a signed 16-bit integer.  For playback, these
samples can be written; for echo and record these can be read.
The difference between the last two is that the echo values
return the input to echo cancellation, while the recording claim
returns the microphone input that synchronises with the echo
data.  Note that the claiming routines return NULL if they have
nothing to offer; the notification functions below resolve this
sitiation if it arises.  The release routines report that work on
the claimed region has ended; note that ``bottom_record_release``
applies to an optional echo claim as well as to the record claim.

For each channel, it is only possible to claim a single block at
a time.  The reason for still having support for multiple blocks
is to permit the hardware drivers to arrange a scheme where some
buffers are being played, while others are being setup by the
firmware.

The reason to have separate calls for the playing side and the
recording side is that they will usually be implemented by
different processes; the reason for permitting access to the
echo-source signal is also to simplify separate processes.  It
also ensures that the echo cancellation software receives
properly timed material, regardless of any variations in timing
between the playback and recording processes.

Note that zero is a valid respons from the ``bottom_playback_claim``
function; it indicates that no more blocks are available.
Furthermore, since the block sizes 

The bottom half makes upcalls to indicate a positive change or,
as the bottom implementors see fit, every time a block becomes
available for claiming.  The upcalls for the two sides of the
sound channel are::

	void top_codec_can_play   (uint8_t chan);
	void top_codec_can_record (uint8_t chan);

These routines will normally kick a (possibly waiting) task, or
perform another action that makes the top-half codec handlers do
their thing.  These top-half handlers will subsequently try to make
a claim and do their thing.  The routines will certainly be
called after a NULL value has been returned from
``bottom_play_claim`` and ``bottom_record_claim``, respectively.

A trivial example of using these routines would be::

	bool play_welcome = false;

	void top_codec_can_play (uint8_t chan) {
		play_welcome = true;
	}

	void top_main (void) {
		bottom_critical_region_end ();
		while (___more_to_write___) {
			int16_t *outbuf;
			do {
				play_welcome = false;
				outbuf = bottom_play_claim (PHONE_CHANNEL_TELEPHONY);
				if (outbuf != NULL) {
					___send_samples_to_outbuf___;
					bottom_play_release (PHONE_CHANNEL_TELEPHONY);
				}
			} while (outbuf != NULL);
			while (!play_welcome) {
				___awful_example_of_polling___;
			}
		}
	}

Of course, this is not a practical example.  It wastes time polling
and has only a single main loop.  But it is noteworthy how there is
no need to block interrupts.  The trick is to reset the flag that is
set by the ``top_codec_can_play()`` before calling ``bottom_play_claim()``
and not after; this avoids ever missing the top-call due to race
conditions between the main program and background sample playback.
The only thing that can happen is that the flag is set just before
the block is claimed that can actually be used for playback; this
is easily detected on the next attempt to claim a block, and should
be tolerated when this technique is used.  The only reason why the
flag is reset before *every* call to ``bottom_play_claim()`` is to
minimise the number of such spurious calls; it is primarily an
optimisation.


Entropy
-------

Some top applications will require pseudo-random bits.  Although
not all hardware has a random generator on-board, it is not hard
to find on a phone:

* A number of low bits from a counter running at the CPU clock,
  sampled at unpredictable times -- such as network interrupts.

* While coding samples, the amount by which the sample is off
  in compressed form.

* While the phone is not active, a microphone can still be sampled
  and its lower bits used.  This would introduce a potential
  privacy problem though, so it is not something to do without
  marking it clearly in the phone's specifications!

* When the top half is done doing a certain task, it may invoke
  a random seeding routine, possible to gather data from the
  sources above.  The bottom half may assume that the top half
  will regularly call a random seeding function if it also wants
  to be able to collect random material.

The bottom half builds an entropy buffer of a prime number of bytes.
The prime number greatly reduces the chances of cycles occurring;
the lowest number of bytes that should be supported is 17.  When
entropy drips in, it is exclusive-ored with the buffer bytes in a
cyclic fashion.  When random material is needed, the next few
bytes are taken out and the pointer for such retrieval moves forward
while doing so.  There is no synchronisation between writing and
reading, as the service is not truely random, but pseudo-random:
best-effort suffices for telephony applications.

::

	void bottom_rndseed (void);
	void bottom_rnd_pseudo (uint8_t *rnd, uint8_t len);
	//TBD// void bottom_rnd_strong (uint8_t *rnd, uint8_t len);

The ``bottom_rndseed()`` function is used to tell the bottom that
now would be a nice time to sample some entropy; this will usually
be called when the top is done with some job, so that it is as far
and unpredictably away from a reliable measurement moment as possible.

The ``bottom_rnd_pseudo()`` function fills the first ``len`` bytes
pointed at by ``rnd`` with random bytes, each consisting of 8 random
bits.  That is, pseudo-random bits.  The top half should never ask
for more than 4 bytes at a time, to avoid emptying the entropy from
the buffer.


Special hardware
================

We all recognise a phone when we see one, but not all devices have
the same structure, and some need a special treatment.  A discussion
on how these are handled follows.


Analog Telephone Adapters
-------------------------

An ATA is not a complete phone, but it talks to a phone.  Even if
there is just a metre of analog wire between these devices, it still
constrains what can be done to/with the user.  If an ATA can also
connect to a phone line, we suggest passing on the signals to the
analog phone and not to pass it on to the top half.

A bottom half implementing access to an ATA should act as if the
attached phone is the phone that is being programmed:

* The bottom half should detect a Flash button or, equivalently, a
  brief press on the hook contact.

* The bottom half should detect DTMF tones and deliver them as if
  local keys had been pressed.  If the ATA supports rotary phones,
  it should deliver rotary-dialed digits upstream as well; if so,
  when the flash button or the hook contact is used within 4 seconds
  of the end of dialing a digit, it should be delivered as ``*``.
  (This is in support of ITAD dialing schemes.)

* If a LED is present on the ATA, it should be reported by the bottom
  half as a voicemail LED.  This will usually be why it was put there,
  and its second function will be to hint the user about ZRTP status.

* Lacking a display and LEDs, the ATA should make an effort to send
  sound signals downstream.  Minimum signals to support are tones to
  indicate insecure calls, to tell the user about the possibility to
  setup a ZRTP secret, and reading out ZRTP digits to the user in
  small chunks when Flash is used while they are available.


Base stations
-------------

A base station is used for wireless calling, usually over DECT.
Unlike a WiFi base station, these units actually handle SIP and
RTP traffic and format it as user interface material.

Base stations are special in that they can represent multiple
handsets which each behave as an independent phone.  What this means
is that the firmware in a base station must not act as a phone, but
as an array of phones.

If this array of phones was treated as completely independent phones
there would be more overhead than strictly needed; it is quite likely
that the handsets want to share numbers, and this is reflected in the
interface by supporting shared lines among handsets.

Additional features are possible, but not yet implemented in this
software.  These features are all specific to the existence of an
array of handsets:

* Calls between handsets, as well as transferring calls to them or
  pulling them into conference calls.  Until this is implemented, a
  normal SIP call will have to be setup to a number recognised on
  the destination handset.

Note that base stations may be sufficiently complex to run Linux.
If this is the case, then this phone application is best setup as
an application on top of Linux.  That way, existing interface drivers
for DECT, networking and so on can be taken from the Linux kernel.


USB and Bluetooth phones
------------------------

These are usually simple sound I/O devices with limited additional
facilities.  A USB phone may have keys, a bluetooth phone will not.
In both cases, the simple phone and its base station must be treated
as one whole, and that whole should implement the bottom half of this
code.


WiFi phones
-----------

WiFi phones are usually pretty clever; they tend to run Linux and
have one process dedicated to telephony on top of that.  The negotiation
of a WiFi connection (finding a base station, setting up encryption
and so on) should all be dealt with in the bottom half, but other
than that this phone application could be built as an application
that runs on top of Linux.

Remember to use a Linux kernel capable of IPv6 for this application.


VoIP Routers
------------

VoIP routers are not as special as they may seem; inasfar as they
connect to analog FXO and FXS lines they are basically the same as
an ATA, and inasfar as they proxy SIP calls they are just routers.

Having said that, a bottom implementation as a process on the
existing infrastructure (like OpenWRT) is a good idea.

