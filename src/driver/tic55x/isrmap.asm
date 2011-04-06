		; Create a subsection with interrupt service routine vectors
		.sect	".isrmap"

		.align	256
		.vli_off

		.def	isrmap0
		.def	isrmap1

		.ref	_c_int00
		.ref	_tic55x_no_isr
		.ref	_tic55x_tint0_isr

_tic55x_bootld	.set	0xff8000

isrmap0:
resetvect	.ivec	_c_int00, NO_RETA
nmi		.ivec	_tic55x_no_isr
int0		.ivec	_tic55x_no_isr
int2		.ivec	_tic55x_no_isr
tint0		.ivec	_tic55x_tint0_isr
rint0		.ivec	_tic55x_no_isr
rint1		.ivec	_tic55x_no_isr
xint1		.ivec	_tic55x_no_isr
lckint		.ivec	_tic55x_no_isr
dmac1		.ivec	_tic55x_no_isr
dspint		.ivec	_tic55x_no_isr
int3		.ivec	_tic55x_no_isr
uart		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
dmac4		.ivec	_tic55x_no_isr
dmac5		.ivec	_tic55x_no_isr
int1		.ivec	_tic55x_no_isr
xint0		.ivec	_tic55x_no_isr
dmac0		.ivec	_tic55x_no_isr
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

isrmap1:
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr
		.ivec	_tic55x_no_isr

		.sect	".text"
		.vli_on

