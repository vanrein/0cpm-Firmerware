#
# Sort each target's object files into objs-bottom-y or objs-bottom-n:
#

objs-bottom-$(CONFIG_TARGET_LINUX_TUNTEST) += src/driver/util/linuxtuntest.o src/driver/util/ledsimu.o

metavars-$(CONFIG_TARGET_LINUX_TUNTEST) += HAVE_NET_LEADER=4

includes-$(CONFIG_TARGET_LINUX_TUNTEST) += bottom/linuxtuntest.h

#
# Platform-dependent specifics
#
ifdef CONFIG_PLATFORM_LOCAL
include src/target/Makefile.local
endif

ifdef CONFIG_PLATFORM_TIC54x
include src/target/Makefile.tic54x
endif

ifdef CONFIG_PLATFORM_TIC55x
include src/target/Makefile.tic55x
endif

