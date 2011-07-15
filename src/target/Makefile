# Makefile for 0cpm target platform picking
#
# This file is part of 0cpm Firmerware.
#
# 0cpm Firmerware is Copyright (c)2011 Rick van Rein, OpenFortress.
#
# 0cpm Firmerware is free software: you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation, version 3.
#
# 0cpm Firmerware is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with 0cpm Firmerware.  If not, see <http://www.gnu.org/licenses/>.


#
# Sort each target's object files into objs-bottom-y or objs-bottom-n:
#

objs-bottom-$(CONFIG_TARGET_LINUX_TUNTEST) += src/driver/util/linuxtuntest.o src/driver/util/ledsimu.o

metavars-$(CONFIG_TARGET_LINUX_TUNTEST) += HAVE_NET_LEADER=4

includes-$(CONFIG_TARGET_LINUX_TUNTEST) += bottom/linuxtuntest.h

includes-$(CONFIG_DEVEL) += bottom/devel.h

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

ifdef CONFIG_PLATFORM_BLACKFIN
include src/target/Makefile.blackfin
endif
