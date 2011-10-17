The Keepalive mechanism in 0cpm Firmerware
==========================================

Phones need to be open to incoming calls, possibly hours
after they have registered over an UDP port.  In most
networks, the fact that UDP has gone from the inside out
is forgotten long before that call comes in.  So, there
is a need for some form of keepalive.


What should be kept open?
-------------------------

If the LAN is a retro network, offering only IPv4, the
phone uses 6bed4 as a last resort to IPv6 connectivity.
In these cases, the phone usually has to pierce through
(at least) one layer of NAT, but except for NAT and
firewalls at the IPv4 layer, it is not to be expected
that the embedded IPv6 traffic requires any keepalive
functionality.

If the LAN supports IPv6, then it is probable that there
is a firewall (or multiple) in the path to the outside
world.  This means that state is probably kept and in
lieu of that for UDP, that holes opened by outgoing
UDP traffic would be dropped.  So, for IPv6 it is also
needed to keep those holes open.


A few stupid approaches
-----------------------

The brute force approach to keep connections open at all
times is to send a regular packet to the SIP server,
possibly without any data contained after the UDP header.
Given that some NATs are very quick to forget holes due
to outgoing traffic, that would have to be done once per
30 seconds.

This would be end-to-end and extremely often, so the burden
on the network would be maximal, and could actually be
pretty big if the number of phones doing that would be
huge.  Given that this is open source firmware, it has
an enormous potential to grow rapidly, so it is better
to reduce the network load by limiting the number of
hops to just get outside NAT and/or firewalls, and by
making the repeating period longer.  As long as it is
autodetected, this should be no problem to users.


Autodetecting the repeating period
----------------------------------

To find the shortest working repeat period, we will try
a value just under 30 seconds and double for as long as
it works.  Starting at 28 seconds, doubling it several
times gets to a value of 3584 seconds or just under an
hour, which is the most likely value to work.  It does
not seem to waste much time of the 3600 seconds that
are expected to be very common in NAT and firewalls.

The system will maintain a safe setting for the actual
connection for the data, and experiment on the side
with an extra connection that exists just for that
purpose.  While keeping the safe connection alive, the
test connection can be skipped once in every two
keepalive tests.  If a few such tests lead to proper
behaviour, the safe setting can be doubled, and the
process can repeat.  Of course there is no sense in
testing on the keepalive process when its interval
has exceeded the SIP registration interval.


Autodetecting the hop limit
---------------------------

To avoid bothering more routers and servers than
strictly required, a lower hop limit on the keepalive
packets helps.  In the case of native IPv6, this
would be the IPv6 hop limit; in the case of the 6bed4
fallback, the Time To Live in the underlying IPv4 packet 
would be used.

Not all firewalls can be detected by way of their
changing of an IPv4 address, especially not IPv6
firewalls.  So, it will not be sufficient to detect
when a global IP address is assigned.  Instead, the
autodetection of the hop limit must be based on
actual communication through the outside world.

The simplest approach is to start with a zero hop
limit, and increment it until it is possible to
receive external communications.  Once that works,
it is clear that all intermediate firewalls have
had a hole punched in them.

TODO: EXTERNAL-ACCESS-HOW?



Combining the autodetection algorithms
--------------------------------------

The two autodetection mechanisms could be combined
in a number of orders.  The best approach is probably
to start from an impossibly short hop limit, incrementing
it until success, and then to migrate the detection of
the longest possible keepalive repeat period to a
background process.  That way, it is possible to quickly
fire up a working service, while still putting a modest
and local footprint on the network; period autodetection
needs time and can then be done quietly in the background,
without any danger of overloading anything.

Given the modesty after hop limit detection alone, it
could be argued that there need not be any further work
done.  But why not go for the ultimate, especially if
on some networks it would save all the work?


