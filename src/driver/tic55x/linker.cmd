-m bin/firmerware.map

MEMORY {
	ISRMAP (RWIX)	: origin = 0x000100,	length = 0x000300
	STACK (RWI)	: origin = 0x000400,	length = 0x000c00
	ROM (RIX)	: origin = 0x001000,	length = 0x006000
	DARAM (RWIX)	: origin = 0x007000,	length = 0x001000
	RAM (RWIX)	: origin = 0x800000,	length = 0x080000
	CONSTRAM (RWIX)	: origin = 0x880000,	length = 0x040000
	BSSRAM (RWIX)	: origin = 0x8c0000,	length = 0x040000
	VECTOR (RIX)	: origin = 0xffff00,	length = 0x000100
}

SECTIONS {
	// .text			> ROM
	.text			> RAM
	.text_quick		> DARAM
	// .switch			> ROM
	.switch			> CONSTRAM
	// .const			> ROM
	.const			> CONSTRAM
	// .const			> CONSTRAM
	.const_codec2_codebook	> CONSTRAM
	// .cinit			> ROM
	.cinit			> CONSTRAM
	.interrupts		> VECTOR
	.isrmap			> ISRMAP
	.data			> RAM
	.bss			> BSSRAM
	.sysmem			> RAM
	.stack			> STACK
	.sysstack		> STACK
	.cio			> DARAM
}

