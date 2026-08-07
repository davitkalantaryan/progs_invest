//
// file:			types01.h
// path:			include/progs_invest/types01.h
// created on:		2023 Mar 08
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//

#ifndef PROGS_INVEST_INCLUDE_PROGS_INVEST_TYPES01_H
#define PROGS_INVEST_INCLUDE_PROGS_INVEST_TYPES01_H

#include <progs_invest/internal_header.h>
#include <cinternal/disable_compiler_warnings.h>
#include <stddef.h>
#include <cinternal/undisable_compiler_warnings.h>

CPPUTILS_BEGIN_C


typedef void* (*TypeAllocFreeHookMalloc)(size_t);
typedef void* (*TypeAllocFreeHookCalloc)(size_t, size_t);
typedef void* (*TypeAllocFreeHookRealloc)(void*, size_t);
typedef void  (*TypeAllocFreeHookFree)(void*);


CPPUTILS_END_C


#endif  //  #ifdef PROGS_INVEST_INCLUDE_PROGS_INVEST_TYPES01_H
