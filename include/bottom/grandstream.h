/* Grandstream-specific definitions, split up per model where needed */

#ifndef HEADER_GRANDSTREAM
#define HEADER_GRANDSTREAM

/* Settings for Budgetone devices */
#if defined CONFIG_TARGET_GRANDSTREAM_BT20x || defined CONFIG_TARGET_GRANDSTREAM_BT10x

#define HAVE_LED_MESSAGE 1
#define HAVE_LED_BACKLIGHT 1
#define HAVE_LED_HANDSET 1
#define HAVE_LED_SPEAKERPHONE 1

#define HAVE_BUTTON_MESSAGE	'i'
#define HAVE_BUTTON_HOLD	'h'
#define HAVE_BUTTON_TRANSFER	'x'
#define HAVE_BUTTON_CONFERENCE	'c'
#define HAVE_BUTTON_FLASH	'f'
#define HAVE_BUTTON_MUTE	'-'
#define HAVE_BUTTON_SEND	'g'
#define HAVE_BUTTON_SPEAKER	's'
#define HAVE_BUTTON_DOWN	'_'
#define HAVE_BUTTON_UP		'^'
#define HAVE_BUTTON_CALLERS	'?'
#define HAVE_BUTTON_CALLED	'!'
#define HAVE_BUTTON_MENU	'm'

#define NEED_HOOK_SCANNER_WHEN_ONHOOK 1
#define NEED_HOOK_SCANNER_WHEN_OFFHOOK 1
#define NEED_KBD_SCANNER_BETWEEN_KEYS 1
#define NEED_KBD_SCANNER_DURING_KEYPRESS 1

#pragma FAR(kbdisp)
extern volatile uint8_t kbdisp;
asm ("_kbdisp .set 0x666666");

/* The network interface KSZ8842-16 is booted without EEPROM, so it
 * has no knowledge of anything -- specifically, no MAC address.
 * It has been hard-wired to a 16-bit interface, but it is addressed
 * from the DSP chip as a 32-bit interface.  The top half of the
 * values read are therefore jibberish.  Yes, silly is the right word.
 */
#pragma FAR(ksz8842_16)
extern volatile uint32_t ksz8842_16 [];
asm ("_ksz8842_16 .set 0x012340");

/* The following are the low-level routines for accessing this
 * particular use of the KSZ8842-16.  They are used in other
 * routines, and the definitions in <bottom/ksz8842.h>
 */
#define kszreg16(N) ksz8842_16[(N)>>1]

inline uint16_t kszmap16(uint16_t x) {
	return (x << 8) | (x >> 8);
}

inline uint32_t kszmap32(uint32_t x) {
	return (x << 24) | (x >> 24) | ((x << 8) & 0x00ff000) | ((x >> 8) & 0x0000ff00);
}

inline uint16_t ksz_memget16 (uint8_t *mem) {
	return (mem [0] << 8) | (mem [1] & 0xff);
}

inline void ksz_memset16 (uint16_t ksz, uint8_t *mem) {
	mem [0] = ksz >> 8;
	mem [1] = ksz & 0xff;
}


#pragma FAR(flash_16)
extern volatile uint16_t flash_16 [];
asm ("_flash_16 .set 0x200000");

#endif


#endif

