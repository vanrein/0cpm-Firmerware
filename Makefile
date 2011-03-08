
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
include src/phone/Makefile

include src/target/Makefile
include src/driver/Makefile

#
# Dependencies for generated include files
# TODO: Solve cyclic dependencies on non-generated file
#
include/config.h: .config
	( cat $< ; for m in $(metavars-y) ; do echo $$m ; done ) | sed -e '/^#/d' -e 's/^\([A-Z0-9_]*\)=\(.*\)/#define \1 \2/'  > $@
	for i in $(includes-y) ; do echo "#include \"../$$i\"" >> $@ ; done

.PHONY += configincludes
configincludes: include/config.h

#
# Include dependency files
#
-include $(objs-top-kernel-y:.o=.d)
-include $(objs-top-net-y:.o=.d)
-include $(objs-top-phone-y:.o=.d)
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
bin/firmerware.bin: bin/firmerware.elf
	strip -s -o $@ $<

bin/firmerware.elf: bin/top.o bin/bottom.o
	gcc -MD -o $@ bin/top.o bin/bottom.o
	@echo
	@echo "*** Compilation succesful"
	@echo
	@size bin/firmerware.elf

bin/top-kernel.o: $(objs-top-kernel-y)
	ld -r -o $@ $(objs-top-kernel-y)

bin/top-net.o: $(objs-top-net-y)
	ld -r -o $@ $(objs-top-net-y)

bin/top-phone.o: $(objs-top-phone-y)
	ld -r -o $@ $(objs-top-phone-y)

bin/top.o: bin/top-kernel.o bin/top-net.o bin/top-phone.o
	ld -r -o $@ bin/top-kernel.o bin/top-net.o bin/top-phone.o

bin/bottom.o: $(objs-bottom-y)
	ld -r -o $@ $(objs-bottom-y)

#
# General compiler rule
# TODO: Use -Wall and remove silencing-up overrides
# gcc -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -fno-builtin -MD -c -Iinclude -ggdb3 -o $@ $<
#
%.o: %.c
	gcc -fno-builtin -MD -c -Iinclude -ggdb3 -o $@ $<

#
# Create a "tags" file for easy Vim navigation
#
tags:
	ctags $(objs-top-kernel-y:.o=.c) $(objs-top-net-y:.o=.c) $(objs-top-phone-y:.o=.c) $(objs-bottom-y:.o=.c) include/0cpm/*.h include/config.h

.PHONY += clean
clean:
	rm -f $(objs-top-kernel-y)
	rm -f $(objs-top-net-y)
	rm -f $(objs-top-phone-y)
	rm -f $(objs-bottom-y)
	rm -f $(objs-top-kernel-y:.o=.d)
	rm -f $(objs-top-net-y:.o=.d)
	rm -f $(objs-top-phone-y:.o=.d)
	rm -f $(objs-bottom-y:.o=.d)
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
	@echo '*** Bottom half sizes:'
	@echo
	@size $(objs-bottom-y) || true
	@echo
	@echo '*** Major component sizes:'
	@echo
	@size bin/top-kernel.o bin/top-net.o bin/top-phone.o bin/bottom.o || true
	@echo
