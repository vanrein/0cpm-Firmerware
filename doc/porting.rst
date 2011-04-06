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
* Linux PC (Windows might work as a sub-optimal substitute)
* useful if available: oscilloscope, logic analyser

The multimeter should be sensitive enough to measure through
a device without damaging it.  Its beep is very helpful because
it allows you to drag one pin over the many pins of a large chip
to find the connection to another point in the circuit.


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
  as amplifiers.

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

Vendors sometimes develop their own connectors for development and
debugging.  Although these certaintly give a lot of control, some
are very expensive.  Usually, cheaper alternatives are available
if you have enough determination.



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

TODO: To build this, you would normally have to write a timer setup and
interrupt service routine to handle ``bottom_time()`` and
``bottom_set_timer_set()`` --do not forget to return the old setting for
the latter-- in addition to the previously written ``bottom_led_set()``
function.  If you care to play with it, edit
``src/function/develtest/keys2display.c`` but be sure to recover the original
before you submit your work.

If this works, you are able to scan keys and write texts on the display.


Build 4: Networked console
--------------------------

In ``make menuconfig``, select the firmware function ``Test network``
that will provide an LLC-based console over ethernet.

TODO: To build this, you would normally have to write a timer setup and
interrupt service routine to handle ``bottom_time()`` and
``bottom_set_timer_set()`` --do not forget to return the old setting for
the latter-- in addition to the previously written ``bottom_led_set()``
function.  If you care to play with it, edit
``src/function/develtest/keys2display.c`` but be sure to recover the original
before you submit your work.

If this works, you are able to use the network; your next build would
be the bootloader, which is the first real application that goes beyond
developer toys.


Build 5: Bootloader
-------------------

This is the first firmware function that actually reflects a useful
application.  The bootloader sets up an IP4 local address using IP4LL,
and at that address runs a TFTP server that reveals the contents of
flash memory.  While the bootloader is active, it will support LAN
access to the flash memory, even for writing.  Your best bet would be
to first download the entire contents of flash, of course.

Note that the bootloader will only run as long as the phone is off-hook;
if it is on-hook during boot, it will skip to the actual application.
Given that development machines are usually open, the horn is usually
not on the hook; whereas in an office situation, a reboot would usually
be performed with the phone on-hook and so the bootloader would be
skipped.  This also rules out various abuse patterns.


Build 6: SIP phone / doorbell / alarm clock
-------------------------------------------

You can now select the firmware functions that you are really after.

