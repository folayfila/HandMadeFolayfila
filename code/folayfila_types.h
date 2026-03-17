// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#if !defined(FOLAYFILA_TYPES_H)
#define FOLAYFILA_TYPES_H

#include <stdint.h>
/************** typedef ***************/
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int32 bool32;
/**************************************/


/*
* Build Options:
** FOLAYFILA_INTERNAL
*   0 - Build for public release.
*   1 - Build for developer only.

* ** FOLAYFILA_SLOW
*   0 - No slow code allowed.
*   1 - Slow code welcome.
*/

#if FOLAYFILA_SLOW
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif  // FOLAYFILA_SLOW

#define Pi32 3.141459265359f

#define Kilobytes(Value) ((Value) * 1024)
#define Megabytes(Value) (Kilobytes(Value) * 1024)
#define Gigabytes(Value) (Megabytes(Value) * 1024)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#endif // FOLAYFILA_TYPES_H