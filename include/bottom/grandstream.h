/* Grandstream-specific definitions, split up per model where needed */

#ifndef HEADER_GRANDSTREAM
#define HEADER_GRANDSTREAM

/* Settings for Budgetone devices */
#if defined CONFIG_TARGET_GRANDSTREAM_BT20x || defined CONFIG_TARGET_GRANDSTREAM_BT10x
#define HAVE_LED_MESSAGE 1
#define HAVE_LED_BACKLIGHT 1
#endif


#endif

