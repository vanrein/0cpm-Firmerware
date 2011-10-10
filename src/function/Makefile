# Makefile for 0cpm device functions
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


# Function-dependent construction of top-objs list

objs-top-$(CONFIG_FUNCTION_NETCONSOLE) += src/net/llconly.o src/function/netconsole.o

#
# Actual targets and their constituent object files
#
objs-top-$(CONFIG_MAINFUNCTION_SIP_PHONE) += src/function/sip6phone/topmain.o bin/top-kernel.o bin/top-net.o bin/top-phone.o

# objs-top-$(CONFIG_MAINFUNCTION_BOOTLOADER) += src/function/bootload.o bin/top-kernel.o bin/top-simplenet.o function

objs-top-$(CONFIG_MAINFUNCTION_BOOTLOADER) += src/function/bootloader.o src/net/llconly.o src/function/netconsole.o

objs-top-$(CONFIG_FUNCTION_NETCONSOLE) += src/function/netconsole.o

objs-top-$(CONFIG_FUNCTION_FIRMWARE_UPGRADES) += src/function/bootloader.o


#
# Test targets, useful during reverse engineering and forward development
#
objs-top-$(CONFIG_MAINFUNCTION_DEVEL_GPIO) += src/function/develtest/switch2led.o

objs-top-$(CONFIG_MAINFUNCTION_DEVEL_TIMER) += src/function/develtest/timer2led.o

objs-top-$(CONFIG_MAINFUNCTION_DEVEL_KEYBOARD) += src/function/develtest/keys2display.o

objs-top-$(CONFIG_MAINFUNCTION_DEVEL_NETWORK) += src/function/develtest/netconsole.o

objs-top-$(CONFIG_MAINFUNCTION_DEVEL_SOUND) += src/function/develtest/echo.o

