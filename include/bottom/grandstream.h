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

#endif


#endif

