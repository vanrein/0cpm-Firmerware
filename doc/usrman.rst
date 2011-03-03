======================================
0cpm Open Source Firmware: User Manual
======================================


Thank you for using the 0cpm Open Source Firmware on your device.
The firmware brings you lots of possibilities to not just conduct
normal phone calls, but also to do many, many other things.


-------------------
Operating the Phone
-------------------

This chapter explains how intuitive it is to operate your phone,
in spite of its potential power.


Concepts
========

Your phone is capable of handling multiple calls, forwarding them,
setting them up in conferences, and so on.  Please take the time
to read these instructions to learn how that works.


Lines and Accounts
------------------

You can configure as many as 6 **accounts** or, if you like the
old term, 6 **phone numbers**.  By default the phone will accept
all incoming calls, but you can set it up to be more restrictive.

Somewhat related are the 6 **lines** on your phone.  The idea of
a line is that it connects you to a remote party, regardless of
whether they called you or you called them.

Every line is loosely related to an account.  If a call comes in
for an account that is setup on a line, then usually that line
will be used for the call.  But if the line is in use, an
incoming call can just as easily spill over to another line.

Outgoing calls work likewise.  If you press a line button, you
initiate an outgoing call using the account for that line.  If you
press the same line button again however, you start to cycle through
the other accounts.  That way, you can dial out on an account even
if its line button is in use.


Soft buttons
------------

Your phone is equiped with 4 **soft buttons**, arranged under the
display.  These buttons change their function depending on the
state the phone is in.  The display prints the function assigned
to each button at every moment in the display, just over the
buttons.


Speed-dial and Presence monitoring
----------------------------------

Your phone has no general-purpose buttons of its own, but extension
units with many such buttons are available for it.  These add
powerful facilities that can really help receptionists overview
their company.

The phone uses general-purpose buttons to configure speed-dial
to a preset remote party, and to reveal presence information of
that remote party.


Step by step
============

These are a few actions, or steps, to get comfortable with the
phone's operation.


Outward dialing
---------------

While your phone has no accounts configured, you cannot dial any
number anywhere, but there are still some calls you can make.  It
works very much like you are used to.

If you like to hear a dial tone, you will pickup the horn
before dialing the number; if you prefer to start dialing right
away you can do so and pickup the horn when done.  Count on
your intuition to operate the phone's buttons for
lines, speakerphone, headset and you will probably get the
behaviour that makes sense to you.

Even without any account setup, you can already make digital calls:

**ISN**, Internet Subscriber Numbers

	These are numbers like ``77*880`` with a local number before,
	and a telephony domain number after an asterisk.  This is a
	free telephony approach, much like the way domain names are
	able to freely arrange email.

**ENUM** for backward compatibility

	This is a normal (officially e.164) phone number, which has
	been registered with a digital alternative.  You will only
	be able to contact digital phones over ENUM if they have
	been registered; but inasfar as you can, there is no
	difference from normal phone calls -- except that it is
	done over the internet.

	ENUM is an international structure, and the official version
	will charge a small annual fee for the registration service.
	There are also informal registers at no cost.  You can
	register your normal phone number and have it translate to,
	say, your ISN.

Even if you are engaged in a call, you can setup a new call.
Just press a free line button and dial as usual.


Getting your own ISN assigned
-----------------------------

Your phone has no phone number configured.  If you would like
to have one for free from 0cpm.org you should dial the following
number::

	77*880

You will be assigned a number or, if you prefer, you can request
sharing a previously assigned number (after jumping through a few
hoops for reasons of security).  This number is also your service
number for future settings related to 0cpm.

If you plan to use multiple accounts, usually because you want to
display several identities: you, your husband, your first company
and your second, and so on.  If you want this you can just dial
the same number from each line that you wish to setup with an ISN.

While setting this up, you will also be offered the option of
setting up ENUM.  Note that a few verification steps will make sure
that you are only registering phone numbers that are actually
yours.


Receiving calls
---------------

Incoming calls will make the phone ring, and when you pickup the
horn, or press a speaker button or similar, you will start to
answer the call.  When you are already on the phone it will
respond differently; instead of ringing out loud, it will ask
for your attention by way of flashing lights and/or tones.

If you are in a call and pickup another one, you are automatically
placing the previous call on hold.  The phone will never connect
parties unless you ask it to.  Likewise, it will not disconnect
a call until you tell the phone to do that, by operating a
button or putting down the horn, and so on.


Calls, Conferences, Forwarding
------------------------------

A **call** is basically any number of parties talking to each
other.  As a phone operator, you may or may not be involved, and
the number of remote parties may be any number.

A **conference call** is in that respect nothing special, it is
merely an informal term to cover talking with more multiple parties
at once.

Similarly, **call forwarding** is just an application of the
general idea of a call in which you as the phone operator are
not involved anymore.

The management of these principles is always the same.  

If you are engaged in a call and want to step out, you can press
the hold button.

When you press a line button, you switch to the call, or conference,
that is running under that button.  If you were in a call before you
did, you will step out of that call.

While you are engaged in a call, you can press the conference
button followed by a remote party's line button to pull him in.
If the remote party was engaged in a conference, then he will
be pulled out of that conference.

Call forwarding is nothing more than setting up a conference and
then forgetting about it.  Depending on whether you set it up with
the conference button or the transfer button, you will or will not
see the lines still active.  As with old phone switches, you can
choose between letting your original party hear the ringtone or
not.


Parking Calls
-------------

Another advanced facility of this firmware is call parking.  This means
that a call is put on hold in such a way that it can be picked up
elsewhere.  This is implemented by forwarding the call to the same
number as it originally called; all phones answering to that number
will then start to ring, including the one that parked the call.  You
will then usually walk to another phone and complete the call there.

Call parking can be arranged with a call transfer, but it can also be
done in a simple way.  First you put the line you wish to park on hold,
and then you put down the horn.  Since this terminates all calls on the
phone, it is vital that you handle them first.  If not, you might park
them all at the same time -- and release chaos!


Using Speed-dial and Presence monitoring
----------------------------------------

Any general-purpose buttons on your phone can get a remote party
assigned.  By default, only the last button is programmed with
the 0cpm service number, ``77*880``.

If you press a speed-dial button, it will simply dial its preset
remote party.  Doing so after pressing the transfer button during
a call will forward that call to the remote party.  Doing so after
pressing the conference button in a call will invite the remote
party into a conference call.

The buttons contain a light to show presence information.  Not all
accounts publish such information, but if they do it helps to see
if a remote party is currently willing to answer calls.

A phone can set its own presence with the do-not-disturb function.
The usual setting is to be available, and the most extreme is to
not answer any calls at all, but in between it is also possible
to politely suggest through presentation information that calls
are not preferred right now.  This principle is also widely used
in chat.


Call Security
=============

Your phone goes through lengths to protect you from eavesdropping,
as well as sidepassing you to a wrong party.


Secure information gathering
----------------------------

The phone needs to collect information on the internet about
parties you intend to call; such parties may or may not have
setup their information securely.  The 0cpm firmware will
check it if they have, and reject false information.

The default security setup for this is somewhat mild; it is
possible to tighten security more, as detailed in the
configuration settings chapter.


Protection from eavesdropping
-----------------------------

Old telephony connected dumb phones over a clever network; with
digital telephony, clever phones are connected over a simple,
transport-only network.  This makes it possible to scramble all
calls in protection against eavesdropping.

Interestingly, it
is in the most developed countries of the world that governments
eavesdrop on citizens as though they had anything to do with
their business; these same governments usually have a reputation
of loosing private information on street corners and so on.
Even if you have nothing to hide, you should wonder if you want
to support governments wasting time on your calls instead of
catching crooks (who already scrambled their calls anyway).
We believe that it is useful to reposess the privacy of calls.

The phone actively tries to scramble calls, but it also depends
on the remoty party's support of the ZRTP mechanism used.  The
phone uses the voicemail indicator to show if it scrambles.  If it
burns solidly, then it failed.  If it is off, the call is
secure.  If it flashes, which it will usually do with a new
remote party, you will be presented with a small code to
exchange with the remote party to ensure that no intermediate
listener is following the call.  As soon as you verified the
code and confirmed that it matched, you can have fully
private conversations with that party.  Future calls will
continue to profit from the one-time code check.


---------------------
Configuring the Phone
---------------------

Although several things can already be done with a phone without
configuring it, it will usually be preferred to configure a phone
until it behaves in a more personalised manner.


The Configuration Menu
======================

The phone has a menu button, and pressing it brings up a menu.
The arrow buttons can be used to select an option, and pressing
OK will activate an option, making it confurable.


Setting up Accounts
===================

The most important thing to do is setting up accounts, or as we used
to call them, phone numbers.  As explained before, it suffices to
dial ``77*880`` to receive a free ISN account that can be dialed
from any phone compliant to this standard, anywhere on the IPv6
internet.  And as explained, a phone number can be requested for
every individual line on the phone, if so desired.

By configuring the phone with the account for outgoing calls, the
phone number will be shown on outgoing traffic, thus enabling
others to see it, and call you back if they want to.  The default
setup is to use anonymous calling, which is not very practical in
online calling, especially because they also accept any incoming
call.

Phone numbers (or accounts) are configured under a line key.
Therefore, you can have as many phone numbers as you have line
keys.  To configure a line, press the line key, pull up the
menu and select the account details.

The number provided, which will look a bit like ``77*880``, should
be setup as the username.  The password is used to login to the
account in all sorts of circumstances.  The provider can be
anything, but in case of an ISN it would be ``0cpm.org``.

Further settings are not required, but they may be for other types
of accounts; the outgoing proxy may be a local PBX, for instance,
or a translation service that supports connectivity to the IPv4
network of SIP telephony.


Setting up Security through DNSSEC
==================================

TODO -- default look at AD, prefer TSIG


Setting up Privacy through ZRTP
===============================

TODO -- default drop sessions, prefer storage


Including Additional Services on the LAN
========================================

TODO -- how to add their address/port pairs



----------------------------
Configuring the 0cpm Service
----------------------------

The previous chapter discussed how to setup the phone; this chapter
explains how the 0cpm service can be tweaked.  Of course this only
applies to accounts setup as 0cpm accounts.


Personalised phone "numbers"
============================

To get even more value out of the phone service, you can create
fancy names like ``sip:your.name@0cpm.org`` that your friends
and family can use from their computer.  The software they should
use for this is a standard SIP-client that is IPv6-ready.

TODO -- setup alias


Advertised freedom
==================

The free service at 0cpm is paid for by small advertisements that
are played while the phone is ringing.  For people calling you
from outside 0cpm, an advertisement about 0cpm is played.  If you
make a call through 0cpm, you can hear any kind of advertisement.

You are never under any obligation to purchase anything from 0cpm
or anyone else, but you should accept that the advertisements are
what makes the free service possible.

You should be aware that the advertisements will never make you
wait longer.  As soon as the remote party picks up their phone,
the advertisement will be terminated.

Some people dislike advertisements, and would rather pay for the
service.  This is certainly possible; we will than charge a small
fee of a few cents for call setup, but there never be a charge
per minute.  One payment will buy as many ad-free incoming calls
as outgoing.

Individuals using 0cpm can prepay for this benefit; companies will
present us with more contact information and in return they will
be billed after the calls are made.  Commercial service at 0cpm
always includes ad-free operation, but at the same price level as
private calls: a few cents for call setup, no per-minute charges.

We always welcome new advertisers; their task is to create a
message that quickly comes to the point, in order to pass on a
message before the remote party picks up the call.  Ads must
meet a certain level of professionality to qualify.  The price
of an ad is the same few cents that are otherwise charged to
the customer for call setup.


Webphone
========

If you want your contacts to call you by pressing a button on your
website, you should login to https://cockpit.0cpm.org/ and follow
the instructions for including a webphone button on your site.

The instructions can be used on any straightforward website, and
consists of the following parts:

* A bit of HTML code, to be pasted into your webpage(s)
* A Java applet that must be installed on the same website
* A button image that invites your visitors

You can customise or replace the button if you like, it is just
there as a starting point.  The reason that you need to install
the applet on your own website is that security requirements for
Java dictate it.

When installed, your visitors see a button, they press it and
their browser will open a webphone that calls you directly.
The webphone is clever enough to create a temporary link to
the IPv6 network if your visitor is not setup with IPv6 yet.


----------------
Business Options
----------------

Businesses usually require more advanced options, and understand
that some services cost a bit of money.  Just like we have a
collection of free consumer options under ``0cpm.org``, we also
have a more extended collection of paid options under ``0cpm.ocm``.

Any business can choose between the ``0cpm.org`` and ``0cpm.com``
variety as they please -- it is certainly permitted to use the
consumer-version for businesses, but the service has not really
been designed for it.  Anyway, the things that are free under
``0cpm.org`` are also for free under ``0cpm.com``.


Internal number ranges
======================

It is common for businesses to require their own number range
for internal use.  This is possible by acquiring an Internet
Telephony Adminsitrative Domain, or ITAD for short.  This is
used in an ISN as the part behind the asterisk; 0cpm uses
``*880`` numbers so its ITAD is ``880``, and businesses can
request their own and choose what numbers to place before the
asterisk.

The 0cpm switches are clever enough to understand that ranges
of 3, 4 or 5 digits are aimed at the ITAD of the dialing party.
So dialing such short numbers would automatically become a
local call.

Having an ITAD is only useful if the underlying technical
setups are hosted.  This can be done by 0cpm or any other
party.  Look on ``0cpm.com`` for options and pricing.


