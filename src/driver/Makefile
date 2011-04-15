
objs-bottom-$(CONFIG_TARGET_GRANDSTREAM_BT20x) += src/driver/tic55x/grandstream-bt20x.o src/driver/ht162x.o

#TODO: replace rts55.lib with bootstrapping in src/driver/tic55x/trampoline.o

objs-bottom-$(CONFIG_PLATFORM_TIC55x) += src/driver/tic55x/int.o src/driver/tic55x/timer.o src/driver/tic55x/isrmap.o #NOPE# src/driver/tic55x/gpio.o #TODO# 

metavars-$(CONFIG_TARGET_GRANDSTREAM_BT20x) += SYSCLK1_TO_MS_DIVIDER=4096

includes-$(CONFIG_TARGET_GRANDSTREAM) += bottom/grandstream.h
