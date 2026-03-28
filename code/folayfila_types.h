// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#ifndef FOLAYFILA_TYPES_H
#define FOLAYFILA_TYPES_H

#include <stdint.h>
/************** Types ***************/
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

/************** Compilers ***************/
#ifndef COMPILER_MSVC
#define COMPILER_MSVC 0
#endif	// COMPILER_MSVC

#ifndef COMPILER_LLVM
#define COMPILER_LLVM 0
#endif	// COMPILER_LLVM

#if !COMPILER_MSVC && !COMPILER_LLVM
#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#else
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif	// _MSC_VER
#endif	// COMPILER_MSVC && !COMPILER_LLVM

#if COMPILER_MSVC
#include <intrin.h>
#endif // COMPILER_MSVC
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