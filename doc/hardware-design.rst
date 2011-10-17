Hints on Phone Hardware Design
==============================

This document provides a number of hints regarding the design of phone hardware.
As phones are always on, a main concern below is saving energy.

The 0cpm Firmerware has been designed specifically to take advantage of the
following hardware facilities.


Display
-------

It is not common for phones to employ a bistable display, but it makes a lot of sense
to use them.  An LCD display usually needs a backlight, causing them to glow rather
loudly in the evening, and be hard to read at daytime, unless the backlight is constantly
active.  Bistable displays usually have excellent contrast, and will reflect ambient
light in much the same way as paper; this is why such displays are sometimes referred
to as e-paper.

Aside from the backlight, an LCD driver also sends constantly changing signals to the
LCD, and that too can be improved upon.


Keys / buttons
--------------

They keyboard of a phone is usually a matrix.  It is simpler than a PC keyboard, in that
no phones seem to rely on pressing more than one key at the same time.

Traditionally, keyboards were scanned at regular intervals.  This however, means that
the processor must be active very often, and may not be able to sleep as deeply as it
could otherwise.  Given that (a) phone keys are rarely used, and (b) the keys should
be responded to quickly, it is better to ensure that the keys raise an interrupt on
the target CPU.  This may take a bit more hardware than usual, but it should save in
the total power budget.

Note that a matrix allows selection of all input columns at the same time; any key
pressed would then immediately show up as an activated row.  Once that signal has
arrived, the columns can be scanned to find the actual key being pressed.  Depending
on the CPU, even the release of the key may be picked up on as an interrupt, or it
may require scanning while the keyboard is active.


Hook contact
------------

The hook contact is usually connected to a GPIO port.  Make sure it can trigger an
interrupt, both when pickup up and putting down the horn.  The reason is the same as
for the keyboard matrix, with which it usually is not integrated.


Sound DMA
---------

The codec that exchanges sound with the user is best operated under DMA, so as to
make the rate at which samples are exchanged highly constant.  Without DMA, the
timing would have to rely on something like interrupts; but interrupts may get
delayed by other interrupts and critical sections.  DMA should cut through
everything in a very resource-friendly manner.


Sleep mode
----------

If a CPU or DSP or SoC has a low-power mode, design for it.  Phones are rarely used,
but must always be on.  While setting up a call, the CPU can speed up its clock and
wake up peripherals for the duration of a call.  If the voltages of the CPU can be
lower in its inactive mode, chances are that even more power is saved.  This is a
common phenomenon in processors, memories and more chips.  Design for it and win
the beauty contest of the lowest-energy phone!


Network switching
-----------------

A phone often has two connectors on its back side; one is the uplinnk the the LAN, the
other is the downlink to a PC.  The 0cpm Firmerware assumes that the ports are marked
accordingly.

Inasfar as PC traffic travels through the phone to the uplink, it should be switched at
the lowest possible level of the network stack.  Usually, this means Ethernet switching.
If it is implemented in a hardware switch, the little CPU in the phone can never become
the bottle neck for networking; also, it does not have to spend time, wake up, and so on.

**Looking ahead:**
Ideally, but not commonly, a switch could connect the PC to the uplink at the physical
layer, by connecting the wires of the two ports.  It would however also observe what
travels over the wires, and be able to take them apart and sit in between as a switch.
It would do the latter to respond to Ethernet traffic directed at its MAC address and
when operating as a phone.  As was said, this is not common practice, but it would make
sense for a wide variety of embedded applications that only send very rarely, and in
determined bursts.  That includes phones, measuring equipment, and probably many more
small devices.


Tickless RTOS
-------------

The heart of the 0cpm Firmerware is essentially a tickless RTOS.  It will not do
anything until an event occurs -- where events could be picking up or putting down
the horn, operating a button, or receiving a network packet.  There should be no
other reason for a phone to wake up.  (Although one can never fully predict what
applications people will want to run on their phones -- they might be used for
surveying a room by sampling the sound level, for instance.)

The RTOS is split in a top half (with generic code) and a bottom half (with drivers).
Needless to say, a tickless RTOS will only work well on drivers that support the
tickless behaviour.  So the aforementioned advantages in hardware must be built
into the drivers, and the proper configuration flags should also be applied.


Differentiate with extravagant hardware
---------------------------------------

SIP phones can do much more than just telephony, and the 0cpm Firmerware exists to
make just that happen.  You could consider an alarm clock, a door bell (with camera)
for one home or a building block for appartment blocks that can be forwarded or
voicemailed like any other phone; but there are other variations that can be made
true with simple extensions -- and you might not always be able to foresee them.
In short, it pays to be different, and add extras.

**PS/2.** A simple externalised UART port could do fun things.  Imagine hooking up
a keyboard to enable realtime text (RTT) between users.  This is an RTP-protocol
that is much loved by people with hearing impairments, because it enables them to
interrupt each other, something they cannot do with plain chat.  Needless to say
that devices in support of disabled users also benefit regular users.  It is an
incredible benefit from what is a trivial extension in terms of hardware.  The
interesting thing of adding a feature like RTT to an open source application also
implies that other devices, from other manufacturers, can be updated with your
application code, so your application can be adopted much faster than with a
product that only your company markets.

**HiFi sound.** Many office workers enjoy music in their work place.  Using a codec
with high quality speaker outputs may not only save you the cost of integrating a
speaker in the phone, it also creates possibilities for webradio support.  But do
keep in mind that most phone calls use a sampling rate in the audible region -- so
your codec must be able to either oversample and filter away anti-aliasing aspects
in the sound -- or your user will be very tense from high tone disturbance.

**Stereo sound.** Whether HiFi or not, stereo sound is useful for telephony, and the
protocols support it.  The utility of stereo sound would be that contacts could be
positioned in a semicircle surrounding the caller, so the voices are much simpler
for a user to separate.  This facility could be implemented in a meeting room service
or in the phone itself -- by combining incoming mono voice channels.  As before, this
code would be general 0cpm Firmerware code, and would thus be made available to other
devices as well.  This means that those devices can pick up on stereo telephony
faster than if it was limited to a closed system.

Stereo sound can also be surprisingly useful for microphones.  If you have never
experienced being blind, you might try dining in absolute darkess (asking a real
blind person to serve you in avoidance of a mess).  It is a striking sensation to
notice that you immediately feel if someone is talking directly to you.  Imagine
the value of having that in a phone conversation!

**Be extravagant.** The big lesson seems to be that support for disabled or specialised
users is not a burden, it can actually be an inspiration.  Even if you don't see a
useful application, you can be fairly certain that the open source community will.
Just ship a few of your phones to active developers and see what will happen... it's
the cheapest marketing possible!


Open, open, open
----------------

The openness of the 0cpm Firmerware is not a danger, it is a feature that will
save you lots of work.  Development cycles can shorten dramatically, and the
World is full of potential programmers that may pickup where you left.  By
pointing to an open source community, you can actually tell people to some
extent to help themselves when it comes to support and repair.

To gain these qualities, all you need to do is open your changes to the code.
This is not just a legal requirement of using the 0cpm Firmerware, but it is
also necessary to allow people to help themselves.  That means that any driver
code that you develop in-house for your target chip must also be open.

There are a few checks that you should make before choosing a hardware platform
for the 0cpm Firmerware:

* You must use a freely distributable toolchain.  A lot of platforms are supported
  by ``gcc`` and ``binutils``, including quite a few embedded environments.  In  more
  and more cases, the platform vendor will have embraced open source support for the
  same reasons you are now considering it for your phone.  Choose another platform
  if this check fails.

* You can only use libraries that are compatible with GPL.  Using libraries that
  may not be redistributed or linked to GPL code and/or that come without source code 
  make it impossible for you to open up your application.  Choose another platform
  if this check fails, or decide to write the basic drivers yourself, and add them
  to the bottom half of the 0cpm Firmerware.

* You should not base your work on limited-access documentation.  The lack of
  documentation severely limits future developers to make any contributions
  that are specifically lucrative on your platform.  You may want to choose
  another platform if this check fails.

* You must make it possible for end users to replace the firmware on your device
  with any version that they may have created themselves, or found somewhere
  online.  Obviously, you are not required to support a product with third-party
  firmware.  But you should probably provide a bootloader (like the generic one
  provided as part of the 0cpm Firmerware) or other mechanism to permit an
  upgrade of the firmware.  As a general rule, you should make it as easy for
  end users as it is for you
  to develop for the device, and upload firmware to it.  This implies that you
  must not use digital signatures to ban uploads of firmware that were not
  authorised by you.  Checksums and hashes to validate the contents of an
  image before burning it into Flash memory are a different story; they are a
  good precaution; be sure to document such platform-specifics though, for
  example by including it in the build chain for the firmware.

  If you incorporate the bootloader of the 0cpm Firmerware then you should also
  enable end-users to replace that part of Flash (as a separate module), but
  if you use a closed bootloader then you need not support its replacement.  The generic
  bootloader in the 0cpm Firmerware can be used to upload to independent flash partitions,
  and the same mechanism can also be used to upload other things, like ringtone
  files.

* Your management should underwrite the opening up of any changes that you
  make to the 0cpm Firmerware; if they question this, you can
  explain that they will save lots of money on development and support
  by incorporating lots of existing code and that only a little bit has to be
  added and opened to the World.  In case your manager argues that this makes
  the code available to competitors, explain that the competitor can already
  choose to download complete code for other platforms, and that the essence
  of the advantage is that adding less value in the form of firmware means
  that less protection can be gained from it -- but that the essence of being
  in electronics is not to create and support firmware, but rather to produce hardware.
  And when it comes to making money from a total solution, it is hardware that
  feels an asset to buyers, not firmware.  Be very clear to your managers -- they
  should understand that pulling out of intended openness after porting the
  0cpm Firmerware would make it illegal to sell the product at all -- and that
  there are volunteers who care enough about these things to prosecute
  companies that invalidate open source licenses.

* As soon as your port is working, write a document in the ``doc/bottom/``
  directory.  You can clone ``SKELETON.rst`` as a starting point.  Please
  create a directory with your manufacturing domainname and place your
  documents in there.  The document should explain the following things to
  developer-type end users:

  - Whether they loose their warrenty if they upload their own firmware;
  - What hardware is used for the product they bought;
  - If you feel so inclined, a schematic circuit of the hardware;
  - Where to find the source code (possibly at the 0cpm project itself);
  - Where to find the toolchain that you used to develop the firmware;
  - How you built the firmware for the device (cmdline instructions);
  - Whether debugging interfaces exists, and how to use them;
  - Possibly how to retrieve the current firmware from your device;
  - How to upload newly built firmware to your device;
  - Possibly how to recover if a firmware version fails.

  The ``SKELETON.rst`` file gives examples for each, and you are welcome to
  reuse it to construct this documentation, with or without modification.
  Please retain the ``.rst`` extension and follow the Docutils_ guidelines
  when documenting, and test with a tool like ``doc2html`` whether it is free
  of errors.  You will find that Docutils is a very useful tool for writing
  documentation efficiently for a variety of output formats.

  .. _Docutils : http://docutils.sourceforge.net/rst.html

Please keep in mind that the requirements of openness exist to keep the
0cpm Firmerware open at all times.  This is beneficial for your end users,
and will reflect upon the popularity of your hardware.  Even if you have to
select a different platform from a closed one with a lower per-unit cost, it
will still save you lots in development and support, and make your hardware
more popular and longer-lasting; so, as a result the open platform is likely
to be a financially better alternative due to more than the per-unit cost.

