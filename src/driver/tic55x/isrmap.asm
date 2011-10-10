; Create a subsection with interrupt service routine vectors
;
; This file is part of 0cpm Firmerware.
;
; 0cpm Firmerware is Copyright (c)2011 Rick van Rein, OpenFortress.
;
; 0cpm Firmerware is free software: you can redistribute it and/or
; modify it under the terms of the GNU General Public License as
; published by the Free Software Foundation, version 3.
;
; 0cpm Firmerware is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with 0cpm Firmerware.  If not, see <http://www.gnu.org/licenses/>.


		.sect	".isrmap"

		.align	256
		.vli_off

		.def	isrmap0
		.def	isrmap1

		.ref	_c_int00
		.ref	_tic55x_no_isr
		.ref	_tic55x_tint0_isr
		.ref	_tic55x_int0_isr
		.ref	_tic55x_dmac0_isr
		.ref	_tic55x_dmac1_isr

_tic55x_bootld	.set	0xff8000

;
; NOTE: According to SPRU371, the interrupt vectors are not
;       mapped logically: isrmap0 / IVPD points to interrupts
;	0-15 and 24-31; isrmap1 / IVPH points to interrupts
;	16-23.  Go figure.  Also, it is not documented that
;	the latter series is prefixed by 128 bytes of... what?
;	We'll make them overlap, as we're not using soft irqs.
;

isrmap0:
isrmap1:

resetvect	.ivec	_c_int00, NO_RETA
nmi		.ivec	_tic55x_no_isr
int0		.ivec	_tic55x_int0_isr
int2		.ivec	_tic55x_no_isr
tint0		.ivec	_tic55x_tint0_isr
rint0		.ivec	_tic55x_no_isr
rint1		.ivec	_tic55x_no_isr
xint1		.ivec	_tic55x_no_isr
lckint		.ivec	_tic55x_no_isr
dmac1		.ivec	_tic55x_dmac1_isr
dspint		.ivec	_tic55x_no_isr
int3		.ivec	_tic55x_no_isr
uart		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
dmac4		.ivec	_tic55x_no_isr
dmac5		.ivec	_tic55x_no_isr

int1		.ivec	_tic55x_no_isr
xint0		.ivec	_tic55x_no_isr
dmac0		.ivec	_tic55x_dmac0_isr
		.ivec	_tic55x_no_isr
dmac2		.ivec	_tic55x_no_isr
dmac3		.ivec	_tic55x_no_isr
tint1		.ivec	_tic55x_no_isr
iic		.ivec	_tic55x_no_isr
berr		.ivec	_tic55x_no_isr
dlog		.ivec	_tic55x_no_isr
rtos		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr

		.sect	".text"
		.vli_on

