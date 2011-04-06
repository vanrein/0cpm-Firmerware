
#include <stdint.h>

typedef uint64_t timing_t;

#define TIMER_NULL 0

#define TIME_MSEC(x)	((x))
#define TIME_SEC(x)	((x)*1000)
#define TIME_MIN(x)	((x)*1000*60)
#define TIME_HOUR(x)	((x)*1000*60*60)
#define TIME_DAY(x)	((x)*1000*60*60*24)
