# Makefile for 0cpm drivers
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

objs-bottom-$(CONFIG_TARGET_GRANDSTREAM_BT20x) += src/driver/tic55x/grandstream-bt20x.o src/driver/ht162x.o src/driver/ksz8842.o src/driver/tlv320aic2x.o

#TODO: replace rts55.lib with bootstrapping in src/driver/tic55x/trampoline.o

objs-bottom-$(CONFIG_PLATFORM_TIC55x) += src/driver/tic55x/int.o src/driver/tic55x/timer.o src/driver/tic55x/isrmap.o #NOPE# src/driver/tic55x/gpio.o #TODO# 

objs-bottom-$(CONFIG_PLATFORM_BLACKFIN) += src/driver/blackfin/timer.o src/driver/blackfin/imaginary.o

metavars-$(CONFIG_TARGET_GRANDSTREAM_BT20x) += SYSCLK1_TO_MS_DIVIDER=122880
# metavars-$(CONFIG_TARGET_GRANDSTREAM_BT20x) += SYSCLK1_TO_MS_DIVIDER=61440

includes-$(CONFIG_TARGET_GRANDSTREAM) += bottom/grandstream.h
