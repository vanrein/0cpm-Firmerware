/* Extra header for use with 0cpm */

#include <stdbool.h>
#include <config.h>

#define __inline__ inline

#ifndef INT32_MAX
#define INT32_MAX 2147483647
#endif

#ifndef INT32_MIN
#define INT32_MIN -2147483648
#endif

#ifndef UINT32_MAX
#define UINT32_MAX 4294967295
#endif

#ifndef UINT32_MIN
#define UINT32_MIN 0
#endif

#ifndef INT16_MAX
#define INT16_MAX 32767
#endif

#ifndef INT16_MIN
#define INT16_MIN -32768
#endif

#ifndef UINT16_MAX
#define UINT16_MAX 65535
#endif

#ifndef UINT16_MIN
#define UINT16_MIN 0
#endif

#ifndef INT8_MAX
#define INT8_MAX 127
#endif

#ifndef INT8_MIN
#define INT8_MIN -128
#endif

#ifndef UINT8_MAX
#define UINT8_MAX 255
#endif

#ifndef UINT8_MIN
#define UINT8_MIN 0
#endif


