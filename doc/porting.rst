======================================
Porting 0cpm Firmerware to a new Phone
======================================

.. contents::

This manual describes the steps to port the 0cpm Firmerware to
a new phone.


Notes for hardware manufacturers
================================

**Welcome!**
We are delighted about any hardware manufacturer who chooses
to use this firmware on their own phones.  We believe that the
best of worlds comes from dedicated hardware manufacturers who
use firmware from dedicated software manufacturers.  We also
believe that open source gives a great economic boost, as it
enables sharing between parties that would not usually work
together.  But as things go, this is a two-sided story; this
software is not only grab-and-run, but it also obliges you to
share back with the community.  In the end, your skills at
hardware design and marketing are what will set you apart.

**License:**
Please read the ``LICENSE`` file carefully.  Using this software,
or even parts of it, makes it apply to you and your work.
The obligations imposed by the license are all related to keeping
your firmware open and modifiable to its users:

* you must supply source code in all versions sold
* you must include a toolchain for building the code
* you must inform your users about all this

We are quite willing to include your source code changes into the
main repository for this project, as long as you are mindful about
the structure of the project; specifically, the separation between
top and bottom halves is vital.  The top half is for generic logic
and the bottom half is for driving the actual hardware; these should
never be mixed and a generic (but possibly evolving) API should be
used between the two layers.

**Functions:**
Note that this firmware can do more than just support SIP
telephony.  It can be built to fulfil functions that are very
useful to various applications:

* SIP phone, simulating analog phones plus *much* more
* SIP doorbell, a hotline phone that dials out as soon as it is switched on
* SIP alarm clock, answering wakeup calls and playing their sounds
* bootloader, a small tool to access the flash over TFTP
* development/test functions to test drivers one by one

The responsibilities from the license only apply to the portions
of the firmware that you are using; for instance, you could use
the bootloader from this project with your own non-phone application
and only have to release source code and access to the bootloader.


Notes for organisations using phones
====================================

Phones usually represent quite a bit of value to companies using
them, so it is not optimal to discard them solely on account of
their firmware.  This project defines new firmware, and aims it
to run on as many platforms as possible.

If you are interested in reusing your phone with modern, free
software, you could consider finding a hacker or nerd to reverse
engineer the phone, and put in this newer firmware.  If successful,
the phones can be tweaked to do exactly what they are supposed to
do, by simply changing the firmware and uploading it to all
phones in use.

The process of reverse engineering is difficult, and not all the
required information is available for all phones, so it may not
be possible in general.  If you decide to walk this path, you
probably should invest a little in analysing whether it is
possible at all before taking the plunge and handing out the
assignment to actually reverse-engineer your phones.

As such changes and modifications would be sold to you, all
results from the project would be donated back to this project.
This means that you will be doing many people a favour, and that
need not be an anonymous act either!


Materials needed
================

Make sure you have a reasonable kit of electronics equipment
on hand:

* screwdrivers, pliers
* multimeter that beeps if it finds a contact
* glass scraper with a handle
* soldering iron and related tools
* wires, jumpers, access to all sorts of electronics parts
* a thin, sharp and stiff sewing needle
* Linux PC (Windows might work as a sub-optimal substitute)
* useful if available: oscilloscope, logic analyser

The **multimeter** should be sensitive enough to measure through
a device without damaging it.  Its beep is very helpful because
it allows you to drag one pin over the many pins of a large chip
to find the connection to another point in the circuit.

As an electronics engineer, I usually have a gut feeling how things
are wired.  Using the multimeter, I could establish these things.
I would switch off the device, set it in the ohmic beeping mode, place
one probe of the multimeter on the pin I wanted to trace and then
poke around.  Where I suspected connectivity to a chip, I would pull
the second probe along the many pins of such a chip.  I would make a
complete round before zooming in on a single beep -- sometimes you
expect to find a signal but you are actually measuring GND, and that
intuition is delivered audibly as several beeps on the same chip.  If
you hit a single pin, always double check the pins around the suspect,
as you will sometimes be probing two pins instead of just one.

It's not much fun counting pins, but there are usually markings on
the PCB's silk screen for every fifth pin.  Always do this work in
broad daylight; nothing can beat the Sun for proper illumination.
Closing one eye may help, and using a magnifying glass may help too.

No matter how well you do it, you will short-circuit parts of the
circuitry.  When you pull a probe along pins you are inevitably going
to connect neighbouring pins.  You should realise that this is a risk,
albeit a modest one -- most electronic circuitry will survive such
a beating without much more trouble than some spurious behaviour, such
as a spontaneous reset.  More surprisingly perhaps, you may also create
a lasting connection between neighbouring pins.  This can happen if you
use very sharp (and thus useful) probes to measure a pin for some time.
In fixating on the pin, you can easily apply too much force, causing
the pin to split apart and cover a bit more area.  With the small sizes
of surface mount technology, this may lead to a short-circuit with
the neighbouring pins!  Most circuitry will even survive such a harsh
treatment, and you can resolve it by taking a **sewing needle** (not a
pin, as those will bend) and scratching carefully between the pins that
got connected.  A strong sign of such unwanted connectivity is if the
original firmware starts to behave strangely and it appears as though
you destroyed a piece of its hardware.  Still, it will take quite a bit
of your intuition and ingenuity to determine the error spot, but on the
other hand it is simple to check by measuring, once you have a
suspicion.

And yes... I would love to have probes with one conductive side and
one isolated side, so I could poke it in between two pins instead
of balancing it on top of one while moving something else around.
Or, better even, it would be good to have a probe shaped like a
cogwheel, that would rotate over the pins and show which one is the
connected pin.  But those are just silly dreams.

An **oscilloscope** is useful for testing analog signals; in a digital
phone, these are mostly limited to clock signals and sound I/O; in
an ATA there would be a lot more use for an oscilloscope.  Since
the signals are not always repeating, a digital scope is the best
option.  These often come with a logic analyser as an optional extra.

A **logic analyser** can save a lot of work (an excellent reason to
finally get one!) because it makes it possible to observe signals
that are being sent to unknown or uncertain components.
For reverse engineering the BT200, a logic analyser was useful for
confirming that the LCD driver chip was indeed (acting like) a
HT1621D, without having to program a driver and be left with the
uncertainty if the guessed chip was wrong, or the frequencies.
Also, it helped to read out the configuration codes sent to the
LCD driver.  Later on, it showed that the LCD driver that I wrote
did not behaving well on this bus, even if it was not visible;
there were small glitches in the signal as a result of writing
16-bit values to an 8-bit bus; the high part of the word was filled
with zeroes and would make observed signals low for a very short
time where they were supposed to remain high.  Finally,
communication with the codec (sound I/O chip) can easily be viewed
with a logic analyser.  In an ATA, a few more components apply.

.. figure:: pix/ht162x.png
	:scale: 200
	:alt: Logic Analyser at work

	The logic analyser at work.  Shown is the LCD driver of
	the BT200 phone, for which we guessed it could be a
	HT162x, based on searching the connector's pin markings
	on the PCB.  D4 is the DATA signal, D3 is CS, D2 is
	the RD clock (unconnected in the phone), D1 is the WR
	clock.  At every rising edge of D1, the value of D4 is
	checked into the LCD.  Note that D1 is always stable at
	that time, which does not undermine this behaviour from
	the assumed datasheet. The period of a command is marked
	by D3.  12 bits makes it probable that this is a HT162x,
	and the command confirms that: 100.1110.0011.0 is the
	LCD command to set NORMAL mode (not TEST mode) and this
	makes sense, also in the combination with other commands
	observed.  This confirmed the HT162x family's behaviour.
	Based on the empty holes that once held a crystal and
	the number of segments on the display this was further
	refined, and the LCD driver had to be a HT1621D chip.


Understanding your hardware
===========================

This section is mainly of interest to reverse engineers.


Identifying devices
-------------------

Some manufacturers "protect" the identity of a chip with a layer
of paint.  That is a good sign; it probably means that the chip
is generic, and good documentation and toolchains may well be
available for it.

Paint on chips can be scraped off with a glass scraper.  Look for
one with a handle to give you more control.  Use the flat surface
of the chip and drive the scraper through, almost as if you are
removing snow from a sidewalk.  It may take you some practice to
really get handy doing this; just remember to err on the safe side
as your could also cut into the chip with the scraper.

The elements usually found in a phone are:

* A major chip, usually a system-on-chip (SoC) which embeds a
  processor with timers, I/O pin drivers, serial ports and so on.

  Although a family of devices usually sticks to a particular SoC,
  it seems that every manufacturer has their own.  Silly but true.

* An ethernet chip.  If the phone has two external LAN connectors,
  it will also include a switch.  Depending on the phone, it may
  hold two ethernet interfaces (which is rare and silly), one
  ethernet interface and a switch, or a combination of those.

  By far the most common chip used is an RTL8019AS.  Since this is
  a 10 Mb/s device, it is usually combined with a 100 Mb/s switch
  so the outside connectors are faster.

* RAM chip(s).  These used to be static RAM but even here the DRAM
  chips are taking over.  These are silly chips, because they are
  the only type of memory that suffers from amnesia; still, they
  are the most compact and usually offer more bits on an area.

* Flash chip(s).  These come in NAND and NOR flavours, referring to
  block- and byte-addressed varieties, respectively.  The NOR flash
  is common, because it enables running programs straight from the
  flash memory.  For that reason, they commonly use 70 ns chips.

* A codec chip.  These are a bit like embedded sound cards; they can
  be accessed over a protocol like SPI to interact with microphones,
  speakers and so on.  They usually include analog electronics such
  as amplifiers and anti-aliasing filters.

Don't forget to check the PCB's bottom; there may be components on
both sides!


Fetch documentation
-------------------

Given that all devices on the board are identified, lookup their
documentation.  A web search with the serial numbers on the chips
usually does wonders, although these search terms are also being
used to attract people wanting to purchase them to trading sites.
Adding a term like GND or Vcc may help.

If you cannot find all documentation for forward engineering, it
may become difficult to port to this platform.  You could try
contacting the vendor of the chips for information, but only if
they haven't published it online.  Most hardware vendors are
keen on seeing open source projects develop around their chips.

You may be able to locate drivers for peripheral chips in kernels
like Linux' -- this may actually help you to drivers for such
chips that were obtained by reverse engineering such chips when
they were used in PCs.  The hardware used in phones is not nearly
as modern as that used in PCs so the odds to this are good.

Save everything found in a ``datasheet`` directory for your porting
project; you will want to refer to it very often, and you don't want
to have to repeat your search.  Also, it will be easier to share
what you found if you store it like that.


Find a toolchain
----------------

Based on the SoC chip used, look for a toolchain.  The documentation
will probably tell you what kind of processor is embedded, and you
may find an open source toolchain (like gcc_, llvm_, gas) or one
provided by the hardware manufacturer.

You really could confront the hardware vendor with it if you wanted
to use their platform compiler for an open source project.  Most
like open source, and have used it for long as consumers; they may
also see the advantage of their hardware being supported by an open
source phone application, especially if they realise that competitors
do have support for it.  VoIP is currently a cut-throat market.

.. _gcc : http://gcc.gnu.org/install/specific.html

.. _gas : http://sourceware.org/binutils/docs/as/index.html

.. _llvm : http://llvm.org/



Gaining control
===============

Most chips provide a number of ways through which you can gain control.
Most circuit boards will actually have jumper positions for soldering-on
a connector that can hold such controls.

Study the chip documentation for the SoC at hand for ways of getting in,
and see if those are wired to the jumper positions on your board.
Various forms of access exist in practice.


Serial
------

Serial interfaces usually have 3 or 4 pins; GND, RxD, TxD and
sometimes an extra Vcc pin.  The level of these interfaces is
usually 3V3, so you will need a converter for this; you could
use a cable intended for a suitable phone (I use one suitable
for Siemens MC60 phones).

If you do this a lot, you will like to have an
`autobaud interface`_ to the RS-232 port.

.. _`autobaud interface` : http://spritesmods.com/?art=autobaud&page=1

Once attached, you want to try using an application like
``minicom`` to figure out the baudrate and see if a proper
console pops up.  It may give you good information about the
kind of device that is running on the phone.  It may even
give you a root prompt.

Look for bootloaders; if you are lucky, you will be using an
openly documented bootloader that explains how to install new
firmware after something like a TFTP download.

A question to always ask yourself is how users could do such things
without soldering.  Is there a webbed interface that lets you do
the same as the serial bootloader?  It may be worth to try to save
the current firmware first, so you can go back and test such things.


JTAG
----

JTAG access is ideal if you can get it; it will give you direct
control over the bus, so you can probe keys and see on your PC
what it does to your bus, and you could steer selected pins to
see if and how they influence LEDs and the display.  Most
importantly though, it could let you upload and download flash
contents without *any* support from the chip.  In other words,
once you have JTAG working, you have full control over your phone.

The Joint Testing Action Group defined an interface named JTAG_
which clocks bits in and out of a chip; these bits can represent
internal state, but the only standardised part reflects on the
external pins of a chip.  A so-called `boundary scan`_ meanst that
these new external pin values are clocked in while the old ones
are being clocked out, one by one in a sequence.

.. _JTAG : http://en.wikipedia.org/wiki/Jtag

.. _`boundary scan` : http://en.wikipedia.org/wiki/Boundary_scan

The boundary scan interface is specified in a special file that
is usually available from SoC manufacturers; look for BSDL or
BSD downloads.

Tools like UrJTAG_ and OpenOCD_ can help with these boundary
scans, although JTAG often involves tweaking and takes some
keen attention.  But once you got it working, you should be
ready to go.

Of the two tools, UrJTAG is older and not as actively developed
as OpenOCD is.  OpenOCD has a very clear structure, making it
very easy to work on; but it cannot currently process BSDL
yet (I am working on that, as a matter of fact) which is possible
with UrJTAG.  One problem with BSDL is that the syntax is not
public (sigh!) so parsers, as is the case with UrJTAG, may not
successfully process all BSDL files around.

.. _UrJTAG : http://urjtag.org/

.. _OpenOCD : http://openocd.berlios.de/web/


Bootloader
----------

Some chips have built-in bootloaders that can download code over
a more-or-less standard interface, like serial, I2C or SPI.  This
may take a bit of soldering, but it is actually a very good way
to control your device because it does not need to replace the
contents of your phone's flash memory.

Depending on the bootloader, you would need to setup a suitable
access for code from your PC to the booting device.  For instance,
Texas Instruments' TMS320VC55xx devices can boot from I2C EEPROMs
attached to the right pins; you could setup an interface to match
Linux' ``i2c-parport`` interface as documented in the
``Documentation/i2c/busses/i2c-parport`` file in the kernel sources.


Vendor-specific
---------------

Chip vendors sometimes develop their own connectors for development and
debugging.  Although these certaintly give a lot of control, some
are very expensive.  Usually, cheaper alternatives are available
if you have enough determination.

Below is how I gained access to the initial phone demonstrating
this project, the Grandstream BT200.  The chips driving these
phones can be booted over I2C, and an interface to do just that
is available externally.  All you need to do is hang EEPROMs on
the I2C bus, and fill them with the necessary program from the
computer (after passing it through a specially designed ``hex55``
command).

.. figure:: pix/bootfloppy-i2c-busside.jpg
	:scale: 100
	:alt: I2C bootfloppy, bus side

	Since I2C is a bus, and multiple EEPROMs can be mounted
	on it, the majority of the connections is the same on all
	chips.  The ideal configuration therefore is a stack of
	such chips.  Also nice and solid in handling when it
	sits around on your desk.

.. figure:: pix/bootfloppy-i2c-graycodedside.jpg
	:scale: 100
	:alt: I2C bootfloppy, gray-coded address selector side

	The other side has 3 connectors A0/A1/A2 on the left;
	these should each get a different address selector in
	the form of different patterns of 1 and 0 bits.  Had
	we used plain binary counting to enumerate the values,
	then there would have been a lot of crossings.  Thanks
	to `Gray coding`_ however, there are none.  It is actually
	possible to draw a curly line between the pins that are
	"1" and the ones that are "0" valued.  And to top it off
	with even more nerdiness, all connections have been made
	using pins that were either bent or clipped off at
	another place.

.. figure:: pix/bootfloppy-i2c-parport_pc.jpg
	:scale: 100
	:alt: I2C interface to development PC

	The nice thing about I2C is that it is trivial to
	connect to a Linux PC running the compiler and the
	``hex55`` utility that prepares code for the
	bootloader built into the DSP chip.  In the kernel
	documentation of ``i2c_parport`` there is a schema,
	of which we picked the type 3 variant.  This involves
	a few open-collector inverter chips taken from a
	sinlge chip.  By bending the pins of such a chip
	sideways, clipping them off and mounting the necessary
	cross-connections and resistors on top, we managed to
	fit the entire device in a standard DB-25 connector case.
	(If something is so well hidden, you do want to
	document the insides on a sticker, of course.)

.. _`Gray coding` : http://en.wikipedia.org/wiki/Gray_code


Making a flash backup
=====================

Assuming you can now boot the hardware with your own software, a
good thing to aim for is to make a backup of the only vital bit
of information in there -- the contents of the flash memory.  This
can be done as soon as you manage to get the bootloader running,
as one of the targets defined below.

If the busses are a confusing mixture of 8-, 16- and 32-bit technology,
you may want to be really sure you got it all.  You should be able
to download the flash contents as a file the size of the contents of
the flash memory chip; it should not contain any duplications.

A nice way to test for duplications is to copy pages or other ranges
from a TFTP-over-LLC1 download with ``dd`` and to compare them.  A
good way of finding suggestions about such ranges can come from a
bit of commandline handywork, like this::

  hd ALLFLASH.BIN | sed 's/^[^ ]* //' | sort | uniq -c

The lines shown will be prefixed by how often the shown combination
of 16 bytes occurs in the firmware, at a 16-byte rounded address.
If these are multiples of 2, or a power of 2, you can be fairly
certain your downloading algorithm fails in a way that clones parts
of the memory.  Also be sure to inspect the hexdump visually; signs
of error are the occurrence of zeroes or ``ff`` values in all rows
for a certain column.

To give an indication, we initially found 32 copies of most of the
data; after fixing a big this still was 2; after re-arranging the
addressing scheme we ended up with only 310 copied lines in the
hexdump passed through sort and uniq; out of a total of 37249.
Clearly, the duplications left can be attributed to accidental
collisions such as repeated strings or procedures.
Once such a decently-looking copy of the entire flash has been
downloaded, save it in a safe place, to ensure a way of future
recovery.

The safest approach to all this is to first write the bottom routine
``bottom_flash_read()`` that reads the flash memory; once that shows
no sign of misfunctioning, continue to write ``bottom_flash_write()``
and work on it until a written image shows up correctly in the output
of ``bottom_flash_read()``.  Send in arbitrary data, and check its
arrival, (also) accross a power cycle.  Upload the original content
and check that it is there.

Find out the partitioning scheme of the flash memory, if any.  It
is quite common to have parts that can be downloaded into the phone
separately, for instance a bootloader, SIP firmware and ringtones.
You want to know where each is located, and enter the table
``bottom_flash_partition_table`` with the block ranges for each of
the partitions.  It is common to end the table with an entry named
``ALLFLASH.BIN`` covering the entire flash memory range.

With full control over bootstrapping code (bypassing flash) and with
a backup of the flash memory, you will never be caught with your
trousers down -- you can always recover the flash to restore the
original way of functioning for the device.



Mapping your hardware
=====================

You should figure out how all the buttons, LEDs and LCD connections
are wired in your phone.  It is not important to know each resistor
and capacitor in the path, but be aware that your multimeter may not
beep if it finds a connection through a buffering resistor.  Also,
you may have to figure out how to measure through driving transistors.

Even if you need to be mindful of such analog helper components,
what you are looking for is a map of the logic connections between
your SoC and the I/O facilities of your phone.  This may involve
flipflops selected by certain addresses, a scanning matrix for the
keyboard, and so on.

You will also want to find out how the chip-select lines of the
various chips on your board are triggered.  This will help to
establish where Flash, RAM and so on are located in the memory
map of your SoC.

Finally, find out how the codec and/or the microphones and speakers
are driven.  This will determine how you should drive sound.


Actually porting the 0cpm firmerware
====================================

Creating a port of the firmware should take minimal effort; that is,
all that was possible to guide you in a generic sense has been done
in the firmerware.

Save all the binary intermediate results if they work, as well as any
intermediate forms such as ELF or COFF files and source; if everything
breaks down it is good to be able to reconstruct earlier results and
decide whether the problem is related to hardware or your firmware.
This could have stopped me from going insane, if only I had realised
it in time ``;-)``


Extend the configuration
------------------------

TODO


Build 1: Basic I/O
------------------

In ``make menuconfig``, select the firmware function ``Test switch / light``
that will toggle the message light that is usually present on phones
in response to the hook contact.

To build this, you would normally have to write simple I/O facilities.
You would need to read the hook contact to implement
``bottom_phone_is_offhook()`` and you would need to output a bit for
``bottom_led_set()``.  If you care to play with it, update the file
``src/function/develtest/switch2led.c`` but be sure to recover the original
before you submit your owrk.

If this works, you know that you have full control over the device,
and that you have a working toolchain going all the way into the
phone. *Congratulations!*


Build 2: Timers and interrupts
------------------------------

In ``make menuconfig``, select the firmware function ``Test timer interrupts``
that will setup a timer and respond to interrupts every 0,5 second by
togging the message LED.

To build this, you would normally have to write a timer setup and
interrupt service routine to handle ``bottom_time()`` and
``bottom_set_timer_set()`` --do not forget to return the old setting for
the latter-- in addition to the previously written ``bottom_led_set()``
function.  If you care to play with it, edit
``src/function/develtest/timer2led.c`` but be sure to recover the original
before you submit your work.

If this works, you are handling interrupts and you can do time calculations
as well as setup timers.  The complexities of timer queues and interrupt
handling is further arranged in the top half.


Build 3: Keys and display
-------------------------

In ``make menuconfig``, select the firmware function ``Test keyboard / display``
that will scan the keyboard and write its findings to the display.

To build this, you would normally have to write ``bottom_keyboard_scan()``,
``bottom_hook_scan()``, ``bottom_show_fixed_msg()``, ``bottom_show_period``,
``bottom_show_ip4()``, ``bottom_show_ip6()``, ``bottom_show_close_level()``.
Note that the ``bottom_show_`` routines need not all be implemented fully;
they exist to permit the top layer to offer information to be laid out in
a format that is optimal for the target phone.  Do take note that there are
levels of information, to ensure that shared space on the display is made
available for higher priority data if need be.

If this works, you are able to scan keys and write texts on the display.


Build 4: Networked console
--------------------------

In ``make menuconfig``, select the firmware function ``Test network``
that will provide an LLC-based console over ethernet.

To build this, you would normally have to write a driver for the network
chip.  You would need to handle interrupts from the network interface to
permit a smoothe operation of the phone.  As part of the driver, you may
have to locate where the MAC address is in flash -- as that address is
sometimes loaded from an EEPROM, but on an embedded device it is usually
cheaper to have the firmware handle that.

If this works, you are able to send and receive information between the
CPU and the network.


Build 5: Echo unit
------------------

In ``make menuconfig``, select the firmware function ``Test sound``
that will echo anything it hears back after a two-second delay.

This is the simplest test for the complete sound interface  It reads
input signals and sends them back after a delay.  This loop involves
the microphone for sound reception, DMA for receiving the microphone
input, the CPU and RAM for the delay, DMA for sending the speaker output,
and finally the speaker output for playing the sound back at you.

The echo test can toggle between handset mode and speakerphone mode,
if the phone's hardware supports it.  It will see if the phone is
off-hook to determine the mode.  Other interfaces (headset and line)
are not tested, as they are not as vital, and will be much simpler
to debug after one or two sound paths have shown to work.

Please be careful during these tests -- the sound may suddenly jump
up in volume, and you should not have the headset anywhere near your
ears if that happens.  Also, avoid elongated high-volume and/or
high-pitch sounds while working to get sound to behave.  Your
hearing is a precious instrument!

To build this, you would normally have to write a driver for the
sound chip or codec.  This is best done through DMA, as that is the
best way of assuring a constant playing rate for samples.  An
approach based on interrupts would suffer from other interrupts and
critical regions that temporarily disable interrupts.

If this works, the sound parts of your phone appears to be working
all the way from the CPU to the human operating the phone; your next
build would be the bootloader, which is the first real application
that goes beyond developer toys.


Build 6: Bootloader
-------------------

This is the first firmware function that actually reflects a useful
application.  The bootloader sets up an IP4 local address using IP4LL,
and at that address runs a TFTP server that reveals the contents of
flash memory.  While the bootloader is active, it will support LAN
access to the flash memory, even for writing.  Your best bet would be
to first download the entire contents of flash, of course.

The bootloader is the first target that could actually be useful to
burn into the phone's flash.  Ideally, this would be done in such a
place that the original firmware can be re-inserted if so required;
end-users will be more likely to try your firmerware if it is also
possible to move back to the original situation that they had.  It
could be useful if the 0cpm Bootloader could continue to be used
even under the original firmware, as it gives more control to the
end-user.  Please document clearly if and how this works on the
target platform you are porting to.

Note that the bootloader will only run as long as the phone is off-hook;
if it is on-hook during boot, it will skip to the actual application.
Given that development machines are usually open, the horn is usually
not on the hook; whereas in an office situation, a reboot would usually
be performed with the phone on-hook and so the bootloader would be
skipped.  This also rules out various abuse patterns.


Build 7: SIP phone / doorbell / alarm clock / ...
-------------------------------------------------

You can now select the firmware functions that you are really after.

