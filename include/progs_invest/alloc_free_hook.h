//
// repo:            progs_invest
// file:			alloc_free_hook.h
// path:			include/progs_invest/alloc_free_hook.h
// created on:		2023 Mar 08
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//

#ifndef PROGS_INVEST_INCLUDE_PROGS_INVEST_ALLOC_FREE_HOOK_H
#define PROGS_INVEST_INCLUDE_PROGS_INVEST_ALLOC_FREE_HOOK_H

#include <progs_invest/internal_header.h>

#ifdef PROGS_INVEST_ALLOC_FREE_HOOK_USED

#include <progs_invest/export_symbols.h>
#include <progs_invest/types01.h>
#include <cinternal/disable_compiler_warnings.h>
#include <stddef.h>
#include <cinternal/undisable_compiler_warnings.h>


CPPUTILS_BEGIN_C


PROGSINVEST_ALLOCFREEHOOK_EXPORT void ProgramsInvestigatorAllocFreeHookInitLibraryIfNotInited(void) CPPUTILS_NOEXCEPT;

PROGSINVEST_ALLOCFREEHOOK_EXPORT void AllocFreeHookSetMallocFnc(TypeAllocFreeHookMalloc a_malloc) CPPUTILS_NOEXCEPT;
PROGSINVEST_ALLOCFREEHOOK_EXPORT void AllocFreeHookSetCallocFnc(TypeAllocFreeHookCalloc a_calloc) CPPUTILS_NOEXCEPT;
PROGSINVEST_ALLOCFREEHOOK_EXPORT void AllocFreeHookSetReallocFnc(TypeAllocFreeHookRealloc a_realloc) CPPUTILS_NOEXCEPT;
PROGSINVEST_ALLOCFREEHOOK_EXPORT void AllocFreeHookSetFreeFnc(TypeAllocFreeHookFree a_free) CPPUTILS_NOEXCEPT;


PROGSINVEST_ALLOCFREEHOOK_EXPORT void* AllocFreeHookCLibMalloc(size_t a_size) CPPUTILS_NOEXCEPT;
PROGSINVEST_ALLOCFREEHOOK_EXPORT void* AllocFreeHookCLibCalloc(size_t a_nmemb, size_t a_size) CPPUTILS_NOEXCEPT;
PROGSINVEST_ALLOCFREEHOOK_EXPORT void* AllocFreeHookCLibRealloc(void* a_ptr, size_t a_size) CPPUTILS_NOEXCEPT;
PROGSINVEST_ALLOCFREEHOOK_EXPORT void  AllocFreeHookCLibFree(void* a_ptr) CPPUTILS_NOEXCEPT;

PROGSINVEST_ALLOCFREEHOOK_EXPORT TypeAllocFreeHookMalloc AllocFreeHookGetMallocFnc(void) CPPUTILS_NOEXCEPT;
PROGSINVEST_ALLOCFREEHOOK_EXPORT TypeAllocFreeHookCalloc AllocFreeHookGetCallocFnc(void) CPPUTILS_NOEXCEPT;
PROGSINVEST_ALLOCFREEHOOK_EXPORT TypeAllocFreeHookRealloc AllocFreeHookGetReallocFnc(void) CPPUTILS_NOEXCEPT;
PROGSINVEST_ALLOCFREEHOOK_EXPORT TypeAllocFreeHookFree AllocFreeHookGetFreeFnc(void) CPPUTILS_NOEXCEPT;


CPPUTILS_END_C


#endif  //  #ifdef PROGS_INVEST_ALLOC_FREE_HOOK_USED

#endif  // #ifndef PROGS_INVEST_INCLUDE_PROGS_INVEST_ALLOC_FREE_HOOK_H