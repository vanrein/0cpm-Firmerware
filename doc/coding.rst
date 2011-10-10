-------------------------------------------
Coding Practices for Firmerware Portability
-------------------------------------------

Writing code for embedded systems means having to deal with a variety of compilers,
not all of the quality that we would like.  What follows is a set of coding rules
that should keep you out of trouble.  Please follow them, even if your compiler
does not reveal any problems -- the last thing you want your patch to do is to
upset the code on another platform.


.. contents::


Code aesthetics
===============

Try not to mix styles, it makes code look ugly and inconsistent.

* The coding style is a tabstop of 8 positions per indentation level.

* All code blocks are enclosed in braces; the opening one behind the code
  fragment introducing the block, and the closing one on a line on its own,
  indented by the same amount as the line with the opening brace.

* The bracket enclosing functional parameters is preceded with a space.


Type mappings
=============

Not all architectures can access series of bytes as shorts or longwords.

* Do not use ``htons()`` and ``ntohl()`` and so on.  These functions map a
  packed representation of bits, assuming that these can be read from or
  written to memory in that packed format.  This is not always true.  So
  instead, use ``netset16()`` and ``netget32()`` and so on.  The types of
  network-represented unsigned integers are ``nint8_t``, ``nint16_t`` and
  ``nint32_t``.

* Where possible, refrain from using variable-sized types, such as ``int``.
  Instead, prefer ``uint16_t`` and such.  It is generally unsafe to assume
  that the size of ``int`` is 32-bit; for instance, a DSP processor may
  prefer 16-bit values.  Similarly, ``long long`` is not always 64-bit or
  more.

* Use ``uint16_t`` for internal representation, and ``nint16_t`` for the
  representation of a 16-bit value in network buffers.

* Do not rely on the ``<varargs.h>`` mechanism; sadly, it does not work on
  all machines (probably due to variations in word sizes and memory models)
  and the possibility of variable arguments in ``#define`` macros also is
  not commonly implemented.


Initialisation
==============

Setting values does not seem to work the same everywhere.

* Initialise all global and stack variables before they are being used.
  It is not generally safe to assume that they will be setup with zeroes.

* Static variables may be set to a value at compile time, or assumed to
  initialise at zero.

* Static variables initialised inside a function body may on some compilers
  be initialised for every invocation of the function, or entry of the
  containing statement block.


Declarations
============

Not all compilers are called ``gcc``, and some really show their age.

* Within a statement block, do not write statements before the declarations
  have all been made.  Modern compilers are forgiving in this respect, and
  it is very tempting to use, but not all compilers are that modern.

* The applications in a phone each have their own, fixed amount of storage.
  This is allocated as global variables, taken from main memory.  They should
  be declared static where module-local scope is possible.  There is very
  little need for dynamicity; perhaps the network packet buffer is the
  only exception.

* Do not allocate large structures on stack; embedded environments do not
  have a stack large enough to hold, say, a network packet buffer.


Global names
============

If you declare global names in your modules, you naturally risk name clashes.

* Global names starting with ``bottom_`` or ``top_`` are reserved for the API
  between top and bottom halves of the code.  Do not use any such names
  for other purposes.

* Name clashes between top and bottom halves should be resolved by changing
  the bottom-half name; the top is meant to co-operate with many kinds of
  bottoms, and taking them all into account in global names would be highly
  unpractical.

* Never ever allow global names to be used accross the separation between
  the top and bottom half.  If you need the API changed, contact us about
  it instead of hacking a solution that will only work for you and frustrate
  all the others.  You should be able to checkin any changes as separate
  updates for top and bottom, in any order, assuming that the API is not
  changed.


Checking in code
================

* Not all toolchains are up-to-speed with dependencies.  Before assuming
  code works, clean the entire target for a full rebuild and test.  We have
  seen cases where changes in data structures led to incoherent interpretation
  by modules, because they did not know their dependency on header files.

* If your work changes both top and bottom halves, create separate patches
  for them, and test them separately.  They should not change the API unless
  this was first discussed and agreed; the design of the API should stay as
  clean as possible.


Strings
=======

I never understood why every product defines their own string abstraction.
Then I caught myself building one for this one :)

I'd tried to work with libosip2 in the past, and found
that its API neither documents nor conceals memory allocation, which
clearly is a problem that can lead to memory leaks and freeing memory
that was not used.  This was a reason to build my own SIP parser, which
turned out to be no big deal at all, given my approach to string handling.

While parsing SIP, the message gets cut up in ever smaller pieces, and
doing this with standard C strings would imply either overwriting the
message with '\0' bytes, thus destroying larger wholes while zooming
in on them, or it would imply copying lots of strings, managing memory,
and having copies of the same content in many places.  This seemed like
a waste.

In order to store string references, a structure ``textptr_t`` contains
a pointer to the first character, plus the number of characters from
that point onwards.  These structures can be stored in all sorts of
scopes that matches the lifetime during which that zooming-in on text
is interesting; if the ``textptr_t`` is removed, nobody will miss it
because the actual string is not stored explicitly.  A null string
would contain a NULL pointer reference to the first character.

A few macros/functions exist to make life easier while working with
these simple strings.  They are defined in include file ``<0cpm/text.h>``
to do the following:

* ``textnullify()`` clears a ``textptr_t`` to a null string;
* ``textisnull()`` checks if a ``textptr_t`` is a null string;
* ``texteq()`` checks if two strings (their length and content) are equal;
* ``txtcat()`` concatenates a ``textptr_t`` to a ``(char *)`` pointer;
* ``intcat()`` and ``hexcat()`` append an integer value to a ``(char *)`` pointer.


Development cycles: Small steps at a time
=========================================

Debugging in an embedded environment is difficult, as you will not be able
to easily detect on what line your code gets stuck.  If you forget to
increment a counter (yes, you will!) you will have to resort to iterating
before you know where the problem arises.  The logging facilities don't
help much either; they need to get a turn running on the CPU, and if your
code gets stuck the logged information will not actually be sent.  (That
might be an interesting change though?)

For this reason, make small steps at a time.  The best approach is probably
to make one conceptual improvement at a time (as half-written code is usually
not half bad, but double bad).  At times, this will mean writing a lot of
code and testing it all at once.  Still, its structure will help you to
find the problem spot more easily.

Just don't forget to increment your counters...

