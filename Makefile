# Makefile for the entire 0cpm project
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


# TODO: this should be src/Kconfig
Kconfig := src/target/Kconfig

#
# Default target "all"
#
.PHONY += all
all: configincludes tags bin/firmerware.bin

#
# Builder helping hand
#
.PHONY += help
help:
	@echo
	@echo "*** Try 'make menuconfig' followed by 'make'"
	@echo "    Learn about object sizes at any stage with 'make size'"
	@echo
	@echo Platform-specific notes are part of the target help descriptions.
	@echo


#
# Load configuration file and various Makefiles
#
-include .config

include bin/kconfig/Makefile

include src/kernel/Makefile
include src/net/Makefile
include src/sip/Makefile
include src/phone/Makefile
include src/codec/Makefile

include src/target/Makefile
include src/function/Makefile
include src/driver/Makefile

BINPATH = $(CONFIG_PLATFORM_BIN_PATH):$(PATH)

#
# Dependencies for generated include files
# TODO: Solve cyclic dependencies on non-generated file
#
include/config.h: .config
	( cat $< ; for m in $(metavars-y) ; do echo $$m ; done ) | sed -e '/^#/d' -e 's/^\([A-Za-z0-9_]*\)=\(.*\)/#define \1 \2/'  > $@
	for i in $(includes-y) ; do echo "#include <$$i>" >> $@ ; done

.PHONY += configincludes
configincludes: include/config.h

#
# Include dependency files
#
-include $(objs-top-kernel-y:.o=.d)
-include $(objs-top-net-y:.o=.d)
-include $(objs-top-phone-y:.o=.d)
-include $(objs-top-codec-y:.o=.d)
-include $(objs-top-bottom-y:.o=.d)

#
# The main building targets
#
# Building is done in two phases; first top.o and bottom.o
# each hold their respective halves of the process, then they
# are composed to form the target binary.  This separation
# enables a thoroughness check on the API separation.  The
# rule is that all API calls between the two halves must be
# documented in doc/top2bottom.*
#

bin/top-kernel.o: $(objs-top-kernel-y)
	PATH=$(BINPATH) $(LD) $(LDFLAGS) -r -o $@ $(objs-top-kernel-y)

bin/top-net.o: $(objs-top-net-y)
	PATH=$(BINPATH) $(LD) $(LDFLAGS) -r -o $@ $(objs-top-net-y)

bin/top-phone.o: $(objs-top-phone-y)
	PATH=$(BINPATH) $(LD) $(LDFLAGS) -r -o $@ $(objs-top-phone-y)

bin/top-codec.o: $(objs-top-codec-y)
	PATH=$(BINPATH) $(LD) $(LDFLAGS) -r -o $@ $(objs-top-codec-y)

# bin/top.o: bin/top-kernel.o bin/top-net.o bin/top-phone.o bin/top-codec.o
#	$(LD) $(LDFLAGS) -r -o $@ bin/top-kernel.o bin/top-net.o bin/top-phone.o bin/top-codec.o
bin/top.o: $(objs-top-y)
	PATH=$(BINPATH) $(LD) $(LDFLAGS) -r -o $@ $(objs-top-y)

bin/bottom.o: $(objs-bottom-y)
	PATH=$(BINPATH) $(LD) $(LDFLAGS) -r -o $@ $(objs-bottom-y)

#
# Create a "tags" file for easy Vim navigation
#
tags: src/net/6bed4.c
	ctags $(objs-top-kernel-y:.o=.c) $(objs-top-net-y:.o=.c) $(objs-top-phone-y:.o=.c) $(objs-bottom-y:.o=.c) include/0cpm/*.h include/config.h

#
# Create API documentation with doxygen
#
.PHONY += apidoc
apidoc:
	doxygen doc/doxygen.conf

.PHONY += clean
clean:
	rm -f $(objs-top-y) $(objs-top-n) $(objs-top-)
	rm -f $(objs-top-kernel-y) $(objs-top-kernel-n) $(objs-top-kernel-)
	rm -f $(objs-top-net-y) $(objs-top-net-n) $(objs-top-net-)
	rm -f $(objs-top-phone-y) $(objs-top-phone-n) $(objs-top-phone-)
	rm -f $(objs-top-codec-y) $(objs-top-codec-n) $(objs-top-codec-)
	# rm -f $(objs-top-net-y)
	# rm -f $(objs-top-phone-y)
	rm -f $(objs-bottom-y) $(objs-bottom-n) $(objs-bottom-)
	rm -f $(objs-top-y:.o=.d) $(objs-top-n:.o=.d) $(objs-top-:.o=.d)
	# rm -f $(objs-top-kernel-y:.o=.d)
	# rm -f $(objs-top-net-y:.o=.d)
	# rm -f $(objs-top-phone-y:.o=.d)
	rm -f $(objs-bottom-y:.o=.d) $(objs-bottom-n:.o=.d) $(objs-bottom-:.o=.d)
	rm -f bin/top-kernel.o bin/top-net.o bin/top-phone.o
	rm -f bin/top.o bin/bottom.o
	rm -f include/config.h
	rm -f tags src/net/6bed4.c
	rm -f bin/firmerware.exe bin/firmerware.bin

.PHONY += size
size:
	@echo
	@echo '*** Top half kernel sizes:'
	@echo
	@size $(objs-top-kernel-y) || true
	@echo
	@echo '*** Top half network sizes:'
	@echo
	@size $(objs-top-net-y) || true
	@echo
	@echo '*** Top half phone sizes:'
	@echo
	@size $(objs-top-phone-y) || true
	@echo
	@echo '*** Top half codec sizes:'
	@echo
	@size $(objs-top-codec-y) || true
	@echo
	@echo '*** Bottom half sizes:'
	@echo
	@size $(objs-bottom-y) || true
	@echo
	@echo '*** Major component sizes:'
	@echo
	@size bin/top-kernel.o bin/top-net.o bin/top-phone.o bin/bottom.o || true
	@echo
