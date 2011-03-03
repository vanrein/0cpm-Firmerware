
# TODO: this should be src/Kconfig
Kconfig := src/target/Kconfig

.PHONY += all
all: src/target/tuntest/firmly0cpm.bin

%.o: %.c
	gcc -c -Iinclude -ggdb3 -o $@ $^

include bin/kconfig/Makefile

include src/target/tuntest/Makefile
include src/net/Makefile

.PHONY += clean
clean:
	find . -name \*\.o -exec rm {} \;
	rm -f src/target/tuntest/firmly0cpm.bin
