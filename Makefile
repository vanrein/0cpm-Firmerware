
# TODO: this should be src/Kconfig
Kconfig := src/target/Kconfig

.PHONY += all
all: bin/firmerware.bin

#
# Load configuration file and various Makefiles
#
-include .config

include bin/kconfig/Makefile

include src/target/Makefile
include src/driver/Makefile
include src/net/Makefile

#
# The main building target
#
bin/firmerware.bin: $(objs-y)
	gcc -o $@ $^

.PHONY += clean
clean:
	find . -name \*\.o -exec rm {} \;
	rm -f src/target/tuntest/firmly0cpm.bin


%.o: %.c
	gcc -c -Iinclude -ggdb3 -o $@ $^

.PHONY += help
help:
	@echo
	@echo "*** Try 'make menuconfig' followed by 'make'"
	@echo
	@echo Platform-specific notes are part of the target help descriptions.
	@echo
