# Function-dependent construction of top-objs list

#
# Actual targets and their constituent object files
#
objs-top-$(CONFIG_MAINFUNCTION_SIP_PHONE) += src/function/sip6phone.o bin/top-kernel.o bin/top-net.o bin/top-phone.o

objs-top-$(CONFIG_MAINFUNCTION_BOOTLOADER) += src/function/bootload.o bin/top-kernel.o bin/top-simplenet.o function

#
# Test targets, useful during reverse engineering and forward development
#
objs-top-$(CONFIG_MAINFUNCTION_DEVEL_GPIO) += src/function/develtest/switch2led.o

objs-top-$(CONFIG_MAINFUNCTION_DEVEL_TIMER) += src/function/develtest/timer2led.o

objs-top-$(CONFIG_MAINFUNCTION_DEVEL_KEYBOARD) += src/function/develtest/keys2display.o

objs-top-$(CONFIG_MAINFUNCTION_DEVEL_NETCONSOLE) += src/function/develtest/netconsole.o

