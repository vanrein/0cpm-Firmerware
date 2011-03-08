/*
 * http://devel.0cpm.org/ -- Open source firmware for SIP phones.
 *
 * Event handling
 */


DEPRECATED FILE


/* Events are situations that normally trigger responses from applications.
 *
 * Examples of events are timers expiring, network packets arriving, buttons being
 * pressed, and anything else that triggers an application to perform actions.  An
 * application under this framework is written to respond to events, much like a
 * modern GUI application.
 *
 * As a result of this event-driven application structure, applications have no
 * stack, so as a result, they have no state other than what they store in data
 * structures declared globally, or as static globals inside their bodies.  This
 * does not seem restrictive to a phone design, regardless of the types of media
 * it services.
 *
 * Not all events are offered to all applications.  Firstly, the applications are
 * tried in stack order until one accepts the event.  Secondly, an application
 * will not even be tried if it is not expressing an interest in the event.  To
 * enable filtering events, they are classified, and each application has a mask
 * to indicate which events are interesting to it.
 *
 * In terms of scheduling, events are stored in queues at different CPU priority
 * levels.  When a new event is raised while running another, it will be entered
 * in the queue that matches its priority.  It is possible for any event handler
 * to schedule events at any level, even higher than their own.
 *
 * Events that are scheduled at a higher priority can be assumed to have finished
 * when the next event at a lower priority is run; events at a lower priority will
 * only run when the system finds time for it, and must therefore be tested for
 * having completed.  Given enough pressure on the device, such an event may never
 * actually schedule.  TODO: In support of knowing about the completion of an event,
 * the firing application will be notified of the completion of each event it sends.
 */


/* The class of an event is its group, and determines in part how it can be handled.
 * The keyboard for example, is split into the numeric pad, soft function keys, line
 * keys, a number of dedicated keys, and so on.
 */

typedef enum {
	/*
	 * A number of different key classes
	 */
	EVT_CLASS_KEY_HOOK,	/* Going on-hook or off-hook */
	EVT_CLASS_KEY_DIALING,	/* Digits and * and # but not DTMF-codes A, B, C, D */
	EVT_CLASS_KEY_ALPHA,	/* Alphanumeric entry mode for strings etc. */
	EVT_CLASS_KEY_LINE,	/* Line keys */
	EVT_CLASS_KEY_GENERIC,	/* Programmable keys */
	EVT_CLASS_KEY_MENU,	/* Operate menu, go up/down */
	EVT_CLASS_KEY_FIXEDFUN,	/* Fixed functions, such as speaker on/off, mute, xfer */
	/*
	 * Sound events
	 */
	EVT_CLASS_MIKE,		/* The microphone has picked up an number of bytes */
	EVT_CLASS_SPEAKER,	/* The speaker is done playing the sound sent to it */
	/*
	 * TODO:Network events
	 */
	TODO_EVT_CLASS_NET_INCOMING,	/* Received a network packet */
	TODO_EVT_CLASS_NET_WENTOUT,	/* Outgoing traffic has been sent as requested */
	/*
	 * Timer events
	 */
	EVT_CLASS_TIMEOUT,	/* Timer expired */
} event_class_t;


/* Event details depend on the class.  For a line key, this would hold the actual key,
 * but for a fixed function key it would use the general key code.  For a timeout,
 * a different thing would happen -- the timer object would be in evt_data.  It all
 * depends on the type of event, really.
 */
typedef uint16_t event_detail_t;


/* An event handler is a plain C function that is called when the scheduler gets to it.
 * It returns true when done handling the event, or false if others should get a chance.
 */
typedef event_t;
bool (*event_handler_t) (event_t *);

/* An event_t structure represents an event and, optionally, the application it is
 * destined for.  In no application is hard-wired, it will be sent down the stack.
 */
typedef struct event_type {
	app_t evt_destination;
	app_t evt_origin;
	event_class_t evt_class;
	event_detail_t evt_detail;
	void *evt_data;
	event_type *evt_next;
} event_t;

