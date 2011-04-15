/* Show output on displays.
 *
 * Display interaction depends heavily on available hardware.
 * For this reason, the messages are formatted in the bottom half
 * of the 0cpm firmerware.  By defining different routines for
 * different kinds of display action, and assigning priority
 * levels to what is being displayed, the resulting show ought
 * to be optimal on any concrete phone device.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */



#ifndef HEADER_SHOW
#define HEADER_SHOW


/* A number of fixed messages is defined in a numeric way, so their
 * actual representation can be optimised for the display (notably,
 * for its size).
 */
typedef enum {
	FIXMSG_OFFLINE,		// The phone is offline
	FIXMSG_REGISTERING,	// The phone is registering
	FIXMSG_READY,		// The phone is ready (shown actively!)
	FIXMSG_BOOTLOAD,	// The bootloader is active
	FIXMSG_SENDING,		// A call request is being / has been sent
	FIXMSG_RINGING,		// The remote phone is ringing
	FIXMSG_CALL_ENDED,	// The call has ended
	FIXMSG_COUNT		// The number of fixed messages
} fixed_msg_t;


/* Stop displaying content at the specified level.  In some cases, older
 * content may now pop up, in others the display could get cleared.
 */
void bottom_show_close_level (app_level_t level);

/* Show a period (not a wallclock time) on the display.
 */
void bottom_show_period (app_level_t level, uint8_t h, uint8_t m, uint8_t s);

/* Print an IP address on the display */
void bottom_show_ip4 (app_level_t level, uint8_t  bytes [4]);
void bottom_show_ip6 (app_level_t level, uint16_t bytes [8]);

/* Print a fixed message prominently on the display */
void bottom_show_fixed_msg (app_level_t level, fixed_msg_t msg);

/* Print a notification of the number of new / old voicemails */
void bottom_show_voicemail (app_level_t level, uint16_t new, uint16_t old);
#endif
