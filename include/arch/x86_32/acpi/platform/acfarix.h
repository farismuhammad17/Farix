#ifndef __ACFARIX_H__
#define __ACFARIX_H__

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include "string.h"
#include "ctype.h"

typedef uint8_t   UINT8;
typedef uint16_t  UINT16;
typedef uint32_t  UINT32;
typedef uint64_t  UINT64;
typedef int8_t    INT8;
typedef int16_t   INT16;
typedef int32_t   INT32;
typedef int64_t   INT64;

typedef uint8_t   BOOLEAN;
typedef int32_t   ACPI_NATIVE_INT;
typedef uint32_t  ACPI_NATIVE_UINT;

typedef uint32_t  ACPI_STATUS;
typedef uint32_t  ACPI_SIZE;
typedef uint64_t  ACPI_IO_ADDRESS;
typedef uint64_t  ACPI_PHYSICAL_ADDRESS;

typedef uintptr_t ACPI_UINTPTR_T;

typedef void * ACPI_MUTEX;
typedef void * ACPI_SEMAPHORE;
typedef void * ACPI_SPINLOCK;

#undef ACPI_SYSTEM_XFACE
#undef ACPI_EXTERNAL_XFACE
#undef ACPI_INTERNAL_XFACE
#undef ACPI_INTERNAL_VAR_XFACE

#undef ACPI_EXTERNAL_RETURN_STATUS
#undef ACPI_EXTERNAL_RETURN_OK
#undef ACPI_EXTERNAL_RETURN_VOID
#undef ACPI_EXTERNAL_RETURN_UINT32
#undef ACPI_EXTERNAL_RETURN_PTR

#undef ACPI_INIT_FUNCTION
#undef ACPI_UNUSED_VAR
#undef ACPI_EXPORT_SYMBOL

#define ACPI_SYSTEM_XFACE
#define ACPI_EXTERNAL_XFACE
#define ACPI_INTERNAL_XFACE
#define ACPI_INTERNAL_VAR_XFACE

#define ACPI_EXTERNAL_RETURN_STATUS(p) p;
#define ACPI_EXTERNAL_RETURN_OK(p)     p;
#define ACPI_EXTERNAL_RETURN_VOID(p)   p;
#define ACPI_EXTERNAL_RETURN_UINT32(p) p;
#define ACPI_EXTERNAL_RETURN_PTR(p)    p;

#define ACPI_STATUS_DEFINED
#define ACPI_SIZE_DEFINED
#define ACPI_IO_ADDRESS_DEFINED
#define ACPI_PHYSICAL_ADDRESS_DEFINED

#define ACPI_MUTEX_NULL     (ACPI_MUTEX) 0
#define ACPI_SEMAPHORE_NULL (ACPI_SEMAPHORE) 0
#define ACPI_SPINLOCK_NULL  (ACPI_SPINLOCK) 0

#define ACPI_INIT_FUNCTION
#define ACPI_UNUSED_VAR
#define ACPI_EXPORT_SYMBOL(symbol)

#undef ACPI_STRUCT_INIT
#define ACPI_STRUCT_INIT(field, value)  .field = value

#define ACPI_USE_C99_FIELDS

// Debugging stuff we don't need

// #define ACPI_RS_DUMP_CAN_DEBUG

// #define ACPI_DEBUGGER
// #define ACPI_DISASSEMBLER

// #define DEBUGGER_SINGLE_THREADED    0x01
// #define DEBUGGER_MULTI_THREADED     0x02

// #define DEBUGGER_THREADING          DEBUGGER_SINGLE_THREADED

#define DEBUGGER_THREADING 0

#define ACPI_OS_NAME "Farix"
#define ACPI_MACHINE_WIDTH 32

#define ACPI_USE_SYSTEM_INTTYPES
#define ACPI_USE_STANDARD_HEADERS

#define ACPI_INLINE inline

#define ACPI_EXPORT

#define ACPI_USE_STANDARD_HEADERS

#define ACPI_ARCHITECTURE_DEFINED
#define ACPI_COMPILER_DEFINED

#ifndef __cdecl
    #define __cdecl
#endif

#define ACPI_USE_SYSTEM_INTTYPES
#define ACPI_USE_STANDARD_HEADERS

#define ACPI_TO_INTEGER(p)         ((uintptr_t)(p))

#define COMPILER_DEPENDENT_INT64   long long
#define COMPILER_DEPENDENT_UINT64  unsigned long long
#define ACPI_USE_NATIVE_MATH64
#define ACPI_USE_NATIVE_DIVIDE

#define ACPI_USE_SYSTEM_CLIBRARY
#define ACPI_USE_DO_WHILE_0
#define ACPI_IGNORE_PACKAGE_RESOLUTION_ERRORS

#ifdef ACPI_DEBUG_DEFAULT
    #undef ACPI_DEBUG_DEFAULT
#endif
#define ACPI_DEBUG_DEFAULT         (0x00000004 | 0x00000800)

#ifndef ACPI_OFFSET
    #define ACPI_OFFSET(d, f)      __builtin_offsetof(d, f)
#endif

#endif
