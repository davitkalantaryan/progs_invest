//
// file:			alloc_free_hook.h
// path:			include/allocfreehook/alloc_free_hook.h
// created on:		2023 Mar 08
// created by:		Davit Kalantaryan (davit.kalantaryan@gmail.com)
//

#ifndef ALLOCFREEHOOK_INCLUDE_ALLOCFREEHOOK_MEM_LEAK_INVEST_H
#define ALLOCFREEHOOK_INCLUDE_ALLOCFREEHOOK_MEM_LEAK_INVEST_H

#include <progs_invest/internal_header.h>

#ifdef PROGRAMS_INVEST_ALLOC_FREE_HOOK_USED

#include <progs_invest/export_symbols.h>
#include <progs_invest/stack_calcs.h>
#include <cinternal/disable_compiler_warnings.h>
#include <stdint.h>
#include <cinternal/undisable_compiler_warnings.h>


CPPUTILS_BEGIN_C


struct SProgsInvestMemLeakInvestStat{
    int     maxAllocs;
    int     numberOfEvents;
    int     allocsMax;
    int     reserved01;
    int64_t memoryAllocatedInBytes;
    int64_t allocatedItemsCount;
};


typedef void (*TypeProgsInvestMemLeakInvestClbk)(const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_curStack,
                                                 const struct SProgsInvestMemLeakInvestStat* CPPUTILS_ARG_NN a_memCurStat,
                                                 void* a_pUserData,
                                                 void** a_pDataForCurThread);


ALLOCFREEHOOK_EXPORT void ProgsInvestMemLeakInvestSkipThisStack(void)CPPUTILS_NOEXCEPT;
ALLOCFREEHOOK_EXPORT void ProgsInvestMemLeakInvestUnskipThisStack(void)CPPUTILS_NOEXCEPT;
ALLOCFREEHOOK_EXPORT void ProgsInvestMemLeakInvestSetMaxAllocsForEvent(int a_maxAllocs)CPPUTILS_NOEXCEPT;
ALLOCFREEHOOK_EXPORT void ProgsInvestMemLeakInvestRegisterClbk(TypeProgsInvestMemLeakInvestClbk a_clbk, void* a_pUserData)CPPUTILS_NOEXCEPT;
ALLOCFREEHOOK_EXPORT const struct SProgsInvestMemLeakInvestStat* ProgsInvestMemLeakInvestMemData(void)CPPUTILS_NOEXCEPT;


CPPUTILS_END_C


#endif  //  #ifdef PROGRAMS_INVEST_ALLOC_FREE_HOOK_USED

#endif  // #ifndef ALLOCFREEHOOK_INCLUDE_ALLOCFREEHOOK_MEM_LEAK_INVEST_H