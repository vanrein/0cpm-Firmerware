/* Grandstream BT20x driver as an extension to the tic55x driver */


#include <stdbool.h>
#include <stdint.h>

#define BOTTOM
#include <config.h>

#include <0cpm/cpu.h>
#include <0cpm/timer.h>
#include <0cpm/led.h>
#include <0cpm/kbd.h>
#include <0cpm/app.h>
#include <0cpm/show.h>

#include <bottom/ht162x.h>


/******** HT162x LCD DRIVER LOW-LEVEL FUNCTIONS ********/


/* The program defines an array of LCDCMD_xxx to
 * initialise the HT162x chip.  The array should be
 * terminated with HT162x_LCDCMD_DONE.
 */
extern uint16_t ht162x_setup_cmdseq [] = {
	HT162x_LCDPREFIX_CMD,
	HT162x_LCDCMD_LCDOFF,
	HT162x_LCDCMD_NORMAL,
	HT162x_LCDCMD_RC256K,
	HT162x_LCDCMD_SYSEN,
	HT162x_LCDCMD_LCDON,
	HT162x_LCDCMD_TIMERDIS,
	HT162x_LCDCMD_WDTDIS,
	HT162x_LCDCMD_TONEOFF,
	HT162x_LCDCMD_BIAS3_4,
	HT162x_LCDCMD_TONE2K,
	// Terminate the sequence:
	HT162x_LCDCMD_DONE
};


/* Chip-enable or -disable the LCD driver */
void ht162x_chipselect (bool selected) {
	if (selected) {
		IODATA &= ~ (1 << 1);
	} else {
		IODATA |=   (1 << 1);
	}
}


/* Send out a databit for the LCD driver, but do not clock it in */
static uint16_t kbdisp_outbuf = 0x0000;
void ht162x_databit_prepare (bool databit) {
	if (databit) {
		kbdisp_outbuf &= ~0x4040;
		kbdisp_outbuf |=  0x8080;
	} else {
		kbdisp_outbuf &= ~0xc0c0;
	}
	kbdisp = kbdisp_outbuf;
}

/* Clock in the database that was prepared for the LCD driver */
void ht162x_databit_commit (void) {
	kbdisp_outbuf |= 0x4040;
	kbdisp = kbdisp_outbuf;
}


/******** LCD-SPECIFIC FORM PAINTING ROUTINES ********/


/* Digit notations on the LCD's 7 segment displays */
uint8_t lcd7seg_digits [10] = {
	0xaf, 0xa0, 0xcb, 0xe9, 0xe4,	// 01234
	0x6d, 0x6f, 0xa8, 0xef, 0xed 	// 56789
};

/* Alphabetic notations on the LCD's 7 segment displays */
uint8_t lcd7seg_alpha [26] = {
	0xee, 0x67, 0x0f, 0xe3, 0x4f,	// AbCdE
	0x4e, 0x2f, 0xe6, 0x06, 0xa1,	// FGHIJ
	0x6e, 0x07, 0x6a, 0x62, 0x63,	// kLMno
	0xce, 0xec, 0x42, 0x6d, 0x47,	// PQRST
	0xa7, 0x23, 0x2b,		// Uvw
	0x49, 0xe5, 0xc2
};

/* Put a character into display byte idx, with ASCII code ch */
void ht162x_putchar (uint8_t idx, uint8_t ch, bool notify) {
	uint8_t chup = ch & 0xdf;
	uint8_t lcd7seg;
	if ((ch >= '0') && (ch <= '9')) {
		lcd7seg = lcd7seg_digits [ch - '0'];
	} else if ((chup >= 'A') && (chup <= 'Z')) {
		lcd7seg = lcd7seg_alpha [chup - 'A'];
	} else if (chup == 0x00) {
		lcd7seg = 0x00;  // space for chup one of 0x00 and 0x20
	} else {
		lcd7seg = 0x40;  // dash
	}
	ht162x_dispdata [idx] = (ht162x_dispdata [idx] & 0x10) | lcd7seg;
	if (notify) {
		ht162x_dispdata_notify (idx, idx);
	}
}

uint8_t lcd_bars [8] = {
	0x00, 0x20, 0x60, 0xe0, 0xe8, 0xec, 0xee, 0xef
};

/* Level indication for volume, showing the upper 3 bits of "level" */
void ht162x_audiolevel (uint8_t level) {
	uint8_t bars = lcd_bars [level >> 5];
	ht162x_dispdata [15] = (ht162x_dispdata [15] & 0x10) | bars;
	ht162x_dispdata_notify (15, 15);
}

#define LCDSYM_NETWORK	14
#define LCDSYM_HANDSET	13
#define LCDSYM_SPEAKER	11
#define LCDSYM_FORWARD	10
#define LCDSYM_CLOCK	8
#define LCDSYM_LOCK	7
#define LCDSYM_BINARY	15
#define LCDSYM_CN1	4
#define LCDSYM_CN2	5
#define LCDSYM_HOUR10	0
#define LCDSYM_HMSEP	1
#define LCDSYM_AM	2
#define LCDSYM_PM	3
#define LCDSYM_DOT12	12
#define LCDSYM_DOT9	9
#define LCDSYM_DOT6	6

/* Special flag setting on the LCD, to flag conditions */
void ht162x_led_set (uint8_t lcdidx, led_colour_t col, bool notify) {
	if (col) {
		ht162x_dispdata [lcdidx] |=  0x10;
	} else {
		ht162x_dispdata [lcdidx] &= ~0x10;
	}
	if (notify) {
		ht162x_dispdata_notify (lcdidx, lcdidx);
	}
}

/******** BOTTOM FUNCTIONS FOR LEDS AND BUTTONS ********/


/* Bottom-half operations to manipulate LED states */
void bottom_led_set (led_idx_t ledidx, led_colour_t col) {
	switch (ledidx) {
	case LED_IDX_HANDSET:
		ht162x_led_set (13, col, true);
		break;
	case LED_IDX_SPEAKERPHONE:
		ht162x_led_set (11, col, true);
		break;
	case LED_IDX_MESSAGE:
		//TODO: Figure out which pin -- incorrectly change backlight
	case LED_IDX_BACKLIGHT:
		// Set bit DXSTAT=5 in PCR=0x2812 to 1/0
		if (col != 0) {
			PCR0 |=     1 << REGBIT_PCR_DXSTAT  ;
		} else {
			PCR0 &= ~ ( 1 << REGBIT_PCR_DXSTAT );
		}
		break;
	default:
		break;
	}
}


/* See if the phone (actually, the horn) is offhook */
bool bottom_phone_is_offhook (void) {
	// The hook switch is attached to GPIO pin 5
	// return ! tic55x_input_get_bit (REGADDR_IODATA, 5);
	return (IODATA & (1 << 5)) != 0;
}


/* Scan to see if the top_hook_update() function must be called */
#if !defined NEED_HOOK_SCANNER_WHEN_ONHOOK || !defined NEED_HOOK_SCANNER_WHEN_OFFHOOK
#  error "The BT200 does not generate an interrupt on hook contact changes"
#endif
static bool bt200_offhook = false;
void bottom_hook_scan (void) {
	if (bt200_offhook != bottom_phone_is_offhook ()) {
		bt200_offhook = !bt200_offhook;
		top_hook_update (bt200_offhook);
	}
}

/* Scan to see if the top_button_press() or top_button_release()
 * functions must be called.
 *
 * The following bits are written to activate one or more keyboard rows:
 * D0..D4 are sent to the variable "kbdisp" which has the proper address.
 * The intermediate variable "kbdisp_outbuf" stores a copy of the bits,
 * and is shared with the LCD routines so unused bits must be retained.
 *
 * The following bits of McBSP0 are read from PCR0 as keyboard columns:
 * C0=CLKRP, C1=CLKXP, C2=FSRP, C3=FSXP, C4=DRSTAT
 *
 * The following routine is designed from the assumption that the keyboard
 * is operated a single key at a time; if not, then any response could be
 * valid.  Note that we do not suppress multiple keys by sending an error
 * code, or behaving like with key release.
 */
#if !defined NEED_KBD_SCANNER_BETWEEN_KEYS || !defined NEED_KBD_SCANNER_DURING_KEYPRESS
#  error "The BT200 does not generate an interrupt on key changes"
#endif
#define KBD_COLUMNS_MASK ( REGVAL_PCR_CLKRP | REGVAL_PCR_CLKXP | REGVAL_PCR_FSRP | REGVAL_PCR_FSXP | REGVAL_PCR_DRSTAT )
static bool bt200_kbd_pressed = false;
static const buttoncode_t keynum2code  [25] = {
	'1',		'2',		'3',		HAVE_BUTTON_MESSAGE,	HAVE_BUTTON_HOLD,
	'4',		'5',		'6',		HAVE_BUTTON_TRANSFER,	HAVE_BUTTON_CONFERENCE,
	'7',		'8',		'9',		HAVE_BUTTON_FLASH,	HAVE_BUTTON_MUTE,
	'*',		'0',		'#',		HAVE_BUTTON_SEND,	HAVE_BUTTON_SPEAKER,
	HAVE_BUTTON_DOWN, HAVE_BUTTON_UP, HAVE_BUTTON_CALLERS, HAVE_BUTTON_CALLED, HAVE_BUTTON_MENU
};
void bottom_keyboard_scan (void) {
	uint16_t scan;
	scan = PCR0 & KBD_COLUMNS_MASK;
	if (bt200_kbd_pressed) {
		// Respond if the key is released
		if (scan == KBD_COLUMNS_MASK) {
			bt200_kbd_pressed = false;
			kbdisp_outbuf &= 0xe0e0;		// Make D0..D4 low
			kbdisp = kbdisp_outbuf;
			top_button_release ();
		}
	} else {
		// Respond if a key is being pressed
		if (scan != KBD_COLUMNS_MASK) {
			uint16_t row;
			for (row = 0; row < 5; row++) {
				kbdisp_outbuf |=   0x1f1f;	// Make D0..D4 high
				kbdisp_outbuf &= ~(0x0101 << row); // Set D$row low
				kbdisp = kbdisp_outbuf;
				{ volatile int pause = 5; while (pause > 0) pause--; }
				scan = PCR0 & KBD_COLUMNS_MASK;
				if (scan != KBD_COLUMNS_MASK) {
					uint16_t col;
					for (col = 0; col < 5; col++) {
						if (! (scan & (0x01 << col))) {
							uint16_t keynum = row * 5 + col;
							buttonclass_t bcl = ((row <= 3) && (col <= 2))? BUTCLS_DTMF: BUTCLS_FIXED_FUNCTION;
							buttoncode_t  cde = keynum2code  [keynum];
							bt200_kbd_pressed = true;
							top_button_press (bcl, cde);
							return;
						}
					}
				}
			}
			// Failed, usually due to an I/O glitch
			kbdisp_outbuf &= 0xe0e0;		// Make D0..D4 low
			kbdisp = kbdisp_outbuf;
		}
	}
}


/******** BOTTOM FUNCTIONS FOR FORMATTING / PRINTING INFORMATION ********/


/* Send an elapsed period (not a wallclock time) to the display.
 * For BT200, this is shown in the top digits as MM:SS or as HH:MM.
 * Note that there are 2 positions, and the first digit can only show a 1.
 */
static app_level_t bt200_period_level = APP_LEVEL_ZERO;
void bottom_show_period (app_level_t level, uint8_t h, uint8_t m, uint8_t s) {
	if (bt200_period_level > level) {
		// Ignore those lower values, as this is highly time-dependent
		return;
	}
	bt200_period_level = level;
	if ((h == 0) && (m <= 19)) {
		h = m;
		m = s;
	}
	ht162x_led_set (2, false,   false); // "AM" off
	ht162x_led_set (3, false,   false); // "PM" off
	ht162x_led_set (1, true,    false); // ":" in "xx:xx"
	ht162x_led_set (0, h >= 10, false); // "1" in "1x:xx"
	ht162x_putchar (0, (h %  10) + '0', false);
	ht162x_putchar (1, (m /  10) + '0', false);
	ht162x_putchar (2, (m %  10) + '0', false);
	ht162x_dispdata_notify (0, 3);
}

/* The level of the main portion of the display, along with an array
 * with the contents at each level
 */
static bt200_displine_level = APP_LEVEL_ZERO;
static bool    bt200_level_active [APP_LEVEL_COUNT];
static uint8_t bt200_level_maintext [APP_LEVEL_COUNT] [12];
static uint8_t bt200_level_dotbits  [APP_LEVEL_COUNT];

/* Internal routine to move a display level to the display.
 * Note that no "led" style bits other than the dots in the main
 * line are influenced.
 */
static void bt200_displine_showlevel (app_level_t level) {
	uint8_t i;
	for (i = 0; i < 12; i++) {
		ht162x_putchar (14 - i, bt200_level_maintext [level] [i], false);
	}
	ht162x_led_set (12, (bt200_level_dotbits [level] & 0x01), false);
	ht162x_led_set ( 9, (bt200_level_dotbits [level] & 0x02), false);
	ht162x_led_set ( 6, (bt200_level_dotbits [level] & 0x04), false);
	ht162x_dispdata_notify (3, 14);
}

/* Internal routine to send a string to the display, either with or
 * without setting the various dots.  The dots are coded in bits
 * 0, 1 and 2 of the dotbits parameter, from left to right.
 */
static void bt200_display_showtxt (app_level_t level, char *txt, uint8_t dotbits) {
	// 1. Print text to the proper level
	int idx = 0;
	while ((idx < 12) && *txt) {
		bt200_level_maintext [level] [idx++] = *txt++;
	}
	while (idx < 12) {
		bt200_level_maintext [level] [idx++] = ' ';
	}
	bt200_level_dotbits [level] = dotbits;
	bt200_level_active [level] = true;
	// 2. If the level is not exceeded by the current, reveal it
	if (bt200_displine_level <= level) {
		bt200_displine_showlevel (level);
		bt200_displine_level = level;
	}
}


/* Stop displaying content at the specified level.  In some cases, older
 * content may now pop up, in others the display could get cleared.
 */
void bottom_show_close_level (app_level_t level) {
	if (bt200_period_level == level) {
		bt200_period_level = APP_LEVEL_ZERO;
		ht162x_led_set (0, false, false);
		ht162x_led_set (1, false, false);
		ht162x_dispdata [0] = 0x00;
		ht162x_dispdata [1] = 0x00;
		ht162x_dispdata [2] = 0x00;
		ht162x_dispdata_notify (0, 2);
	}
	if (bt200_displine_level == level) {
		bt200_level_active [level] = false;
		memset (bt200_level_maintext [level], ' ', 12);
		while ((bt200_displine_level > APP_LEVEL_ZERO)
				&& !bt200_level_active [bt200_displine_level]) {
			bt200_displine_level--;
		}
		bt200_displine_showlevel (bt200_displine_level);
	}
}

/* Print an IPv4 address on the display */
void bottom_show_ip4 (app_level_t level, uint8_t bytes [4]) {
	uint8_t idx;
	char ip [12];
	char *ipptr = ip;
	for (idx = 0; idx < 4; idx++) {
		*ipptr++ = '0' +  (bytes [idx] / 100);
		*ipptr++ = '0' + ((bytes [idx] % 100) / 10);
		*ipptr++ = '0' +  (bytes [idx] % 10);
	}
	bt200_display_showtxt (level, ip, 0x07);
}

void bottom_show_ip6 (app_level_t level, uint16_t bytes [8]) {
	;	// Impossible, skip
}

/* A list of fixed messages, matching the fixed_msg_t values */

static char *bt200_fixed_messages [FIXMSG_COUNT] = {
	"offline",
	"registering",
	"ready",
	"bootload",
	"sending call",
	"ringing",
	"call ended",
};

/* Print a fixed message on the main line of the display */
void bottom_show_fixed_msg (app_level_t level, fixed_msg_t msg) {
	if (msg < FIXMSG_COUNT) {
		bt200_display_showtxt (level, bt200_fixed_messages [msg], 0x00);
	}
}

/* Print a notification of the number of new / old voicemails */
void bottom_show_voicemail (app_level_t level, uint16_t new, uint16_t old) {
	char msg [12];
	if (new > 999) {
		new = 999;
	}
	memcpy (msg, "xxx messages", 12);
	msg [0] =  new / 100;
	msg [1] = (new % 100) / 10;
	msg [2] =  new        % 10;
	bt200_display_showtxt (level, msg, 0x00);
}

/******** BOTTOM LEVEL MAIN PROGRAM ********/


/* Setup the connectivity of the TIC55x as used on Grandstream BT20x */
void main (void) {
	uint16_t idx;
	led_colour_t led = LED_STABLE_ON;
	for (idx = 0; idx < APP_LEVEL_COUNT; idx++) {
		bt200_level_active [idx] = false;
	}
	IODIR  |= (1 << 1);
	IODATA |= (1 << 1);
{ uint16_t ctr = 250; while (ctr > 0) { ctr--; } }	
	bottom_critical_region_begin (); // _disable_interrupts ();
	IER0 = IER1 = 0x0000;
	tic55x_setup_timers ();
	tic55x_setup_interrupts ();
	ht162x_setup_lcd ();
	PCR0 = (1 << REGBIT_PCR_XIOEN) | (1 << REGBIT_PCR_RIOEN);

#if 0
{uint8_t idx, dig, bar;
idx=0; dig=0; bar=0;
memset (ht162x_dispdata, 0x00, sizeof (ht162x_dispdata));
while (true) {
uint32_t ctr;
ht162x_putchar (14 - idx, (dig < 10)? (dig + '0'): (dig + 'A' - 11));
ht162x_audiolevel ((bar < 8)? (bar << 5): ((14-bar) << 5));
bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_OFF);
ht162x_dispdata_notify (14 - idx, 14 - idx);
ht162x_dispdata_notify (15, 15);
bottom_led_set (LED_IDX_MESSAGE, LED_STABLE_ON );
idx = (idx + 1) % 12;
dig = (dig + 1) % 38;
bar = (bar + 1) % 14;
ctr = 650000; while (ctr>0) ctr--;
}
}
#endif

	//TODO// IER0 = 0xdefc;
	//TODO// IER1 = 0x00ff;
	top_main ();
}

