
objs-bottom-$(CONFIG_TARGET_GRANDSTREAM_BT20x) += src/driver/tic55x/grandstream-bt20x.o src/driver/ht162x.o src/driver/ksz8842.o src/driver/tlv320aic2x.o

#TODO: replace rts55.lib with bootstrapping in src/driver/tic55x/trampoline.o

objs-bottom-$(CONFIG_PLATFORM_TIC55x) += src/driver/tic55x/int.o src/driver/tic55x/timer.o src/driver/tic55x/isrmap.o #NOPE# src/driver/tic55x/gpio.o #TODO# 


metavars-$(CONFIG_TARGET_GRANDSTREAM_BT20x) += SYSCLK1_TO_MS_DIVIDER=122880
# metavars-$(CONFIG_TARGET_GRANDSTREAM_BT20x) += SYSCLK1_TO_MS_DIVIDER=61440

includes-$(CONFIG_TARGET_GRANDSTREAM) += bottom/grandstream.h
