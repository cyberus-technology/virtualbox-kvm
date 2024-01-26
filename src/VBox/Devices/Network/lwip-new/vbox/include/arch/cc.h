#ifndef VBOX_ARCH_CC_H_
#define VBOX_ARCH_CC_H_

#ifndef LOG_GROUP
#define LOG_GROUP LOG_GROUP_DRV_LWIP
#endif
#include <VBox/log.h>
#include <iprt/stdint.h>
#include <iprt/cdefs.h>
#include <iprt/assert.h>
#include <iprt/time.h>

#ifndef RT_OS_WINDOWS
# define LWIP_TIMEVAL_PRIVATE 0
#endif

typedef uint8_t u8_t;
#ifndef RT_OS_SOLARIS
typedef int8_t s8_t;
#else
/* Solaris /usr/include/sys/int_types.h incorrectly defines int8_t as "char" */
typedef signed char s8_t;
#endif
typedef uint16_t u16_t;
typedef int16_t s16_t;
typedef uint32_t u32_t;
typedef int32_t s32_t;

typedef uintptr_t mem_ptr_t;

#ifdef _MSC_VER
# define PACK_STRUCT_FIELD(x) x
# define PACK_STRUCT_STRUCT
# define PACK_STRUCT_USE_INCLUDES
# if _MSC_VER < 1600
#  define LWIP_PROVIDE_ERRNO
# else
#  include <errno.h>
# endif
# pragma warning (disable: 4103)
#elif defined(__GNUC__)
# define PACK_STRUCT_FIELD(x) x
# define PACK_STRUCT_STRUCT __attribute__((__packed__))
# define PACK_STRUCT_BEGIN
# define PACK_STRUCT_END
# include <errno.h>
#else
# error This header file has not been ported yet for this compiler.
#endif

/* Provide byte order hint. */
#undef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN

#ifdef DEBUG
#define LWIP_PLATFORM_DIAG(x) Log(x)
#else /* !DEBUG */
#define LWIP_PLATFORM_DIAG(x) LogRel(x)
#endif /* !DEBUG */
#define LWIP_PLATFORM_ASSERT(x) AssertReleaseMsgFailed((x))

#endif /* !VBOX_ARCH_CC_H_ */
