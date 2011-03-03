src/net/6bed4.c:
	printf > $@ '#include <stdint.h>\nuint32_t ip4_6bed4 = ( %d << 24 | %d << 16 | %d << 8 | %d );\n' `echo $(CONFIG_NET_6BED4_SERVER) | sed 's/\./ /g'`

objs-y += src/net/core.o src/net/input.o src/net/reply.o src/net/send.o src/net/db.o src/net/6bed4.o
