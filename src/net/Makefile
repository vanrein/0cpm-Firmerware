src/net/6bed4.c:
	printf > $@ '#include <stdint.h>\nuint32_t ip4_6bed4 = ( (uint32_t) %d) << 24 | ((uint32_t) %d) << 16 | ((uint32_t) %d) << 8 | ((uint32_t) %d );\n' `echo $(CONFIG_NET_6BED4_SERVER) | sed 's/\./ /g'`

objs-top-net-y += src/net/core.o src/net/input.o src/net/reply.o src/net/send.o src/net/db.o src/net/llc.o src/net/6bed4.o
