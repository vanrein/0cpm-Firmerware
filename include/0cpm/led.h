/*
 * http://devel.0cpm.org/ -- Open source firmware for SIP phones.
 *
 * LED drivers
 */


#ifndef HEADER_LED
#define HEADER_LED


/* LED drivers are split in a top half and a bottom half.  The top half is generic, the
 * bottom half is device-specific.  The bottom half routines start with bottom_led_
 * while the top half functions start with led_
 *
 * Both systems number LEDs sequentially.  The colour of a LED is numbered from 0 up,
 * where 0 represents the least intrusive colour (which means off) and the highest
 * value represents the most intrusive colour.
 *
 * In addition, the colour can be combined to flash with LED_FLASHING(col1,col2).
 */


/* Define a LED_IDX for all available LED functions.
 * End with LED_IDX_COUNT to define the number of LEDs defined
 */

#define LED_IDX_LINEKEYS 0

#if HAS_LED_LINEKEYS
#   define LED_IDX_GENERIC (LED_IDX_LINEKEYS+NUM_LINEKEYS)
#else
#   define LED_IDX_GENERIC LED_IDX_LINEKEYS
#endif

#if HAS_LED_GENERIC
#   define LED_IDX_MESSAGE (LED_IDX_GENERIC+HAS_KBD_GENERIC)
#else
#   define LED_IDX_MESSAGE LED_IDX_GENERIC
#endif

#if HAS_LED_MESSAGE
#   define LED_IDX_MUTE (LED_IDX_GENERIC+1)
#else
#   define LED_IDX_MUTE LED_IDX_GENERIC
#endif

#if HAS_LED_MUTE
#   define LED_IDX_HANDSET (LED_IDX_MUTE+1)
#else
#   define LED_IDX_HANDSET LED_IDX_MUTE
#endif

#if HAS_LED_HANDSET
#   define LED_IDX_HEADSET (LED_IDX_HANDSET+1)
#else
#   define LED_IDX_HEADSET LED_IDX_HANDSET
#endif

#if HAS_LED_HEADSET
#   define LED_IDX_SPEAKERPHONE (LED_IDX_HEADSET+1)
#else
#   define LED_IDX_SPEAKERPHONE LED_IDX_HEADSET
#endif

#if HAS_LED_SPEAKERPHONE
#   define LED_IDX_BACKLIGHT (LED_IDX_SPEAKERPHONE+1)
#else
#   define LED_IDX_BACKLIGHT LED_IDX_SPEAKERPHONE
#endif

#if HAS_LED_BACKLIGHT
#   define LED_IDX_COUNT (LED_IDX_BACKLIGHT+1)
#else
#   define LED_IDX_COUNT LED_IDX_BACKLIGHT
#endif

/* Define led_idx_t as a value that can hold the index number
 * of an existing LED in the system.
 */
#if LED_IDX_COUNT > 256
	typedef uint16_t led_idx_t;
#else
	typedef uint8_t  led_idx_t;
#endif

/*
 * Define led_colour_t as the colour type for a LED, including flashing.
 * The flashing definition consists of an XOR pattern in the top half.
 * Combinations of colours can be composed to a flashing pattern with
 * the LED_FLASH(col1,col2) macro.
 */
typedef uint8_t led_colour_t;

#define LED_STABLE(col) ((col) & 0x0f)
#define LED_STABLE_OFF 0x00
#define LED_STABLE_ON  0x01

#define LED_FLASH(col1,col2) ((col1) | ((col1)^(col2)) << 4)
#define LED_FLASHING_CURRENT(col) ((col) & 0x0f)
#define LED_FLASHING_OTHER(col) (((col) ^ (col >> 4)) & 0x0f)
#define LED_FLASH_FLIP(col) ((col) ^ ((col) >> 4))

/*
 * Define led_flashtime_t as the time for half a delay of a flashing LED.
 * Define values LED_FLASHTIME_XXX for various flashing speeds, with
 * NONE for no flashing at all.
 */
typedef timing_t led_flashtime_t;
#define LED_FLASHTIME_NONE	((led_flashtime_t) (   0 * TIME_MSEC))
#define LED_FLASHTIME_FAST	((led_flashtime_t) ( 500 * TIME_MSEC))
#define LED_FLASHTIME_MEDIUM	((led_flashtime_t) (1000 * TIME_MSEC))
#define LED_FLASHTIME_SLOW	((led_flashtime_t) (2000 * TIME_MSEC))


/* Top-half operations to manipulate LED states */
void led_set (led_idx_t ledidx, led_colour_t col, led_flashtime_t ft);
led_colour_t led_getcolour (led_idx_t ledidx);
led_flashtime_t led_getflashtime (led_idx_t ledidx);
static inline void led_set_off (led_idx_t ledidx) { led_set (ledidx, LED_STABLE_OFF, LED_FLASHTIME_NONE); }
static inline void led_set_on  (led_idx_t ledidx) { led_set (ledidx, LED_STABLE_ON,  LED_FLASHTIME_NONE); }


/* Bottom-half operations to manipulate LED states */
void bottom_led_set (led_idx_t ledidx, led_colour_t col);

#endif
