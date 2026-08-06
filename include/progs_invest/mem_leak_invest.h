//
// repo:            progs_invest
// file:			mem_leak_invest.h
// path:			include/progs_invest/mem_leak_invest.h
// created on:		2023 Mar 08
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//

#ifndef PROGS_INVEST_INCLUDE_PROGS_INVEST_MEM_LEAK_INVEST_H
#define PROGS_INVEST_INCLUDE_PROGS_INVEST_MEM_LEAK_INVEST_H

#include <progs_invest/internal_header.h>

#ifdef PROGS_INVEST_MEM_LEAK_INVEST_USED

#include <progs_invest/export_symbols.h>
#include <progs_invest/stack_calcs.h>
#include <cinternal/disable_compiler_warnings.h>
#include <stdint.h>
#include <cinternal/undisable_compiler_warnings.h>


CPPUTILS_BEGIN_C


struct SProgsInvestMemLeakInvestSettings{
    int     maxAllocs;
    int     reserved01;
};


struct SProgsInvestMemLeakInvestStatus {
    int     numberOfEvents;
    int     allocsMaxUpToNow;
    int64_t memoryAllocatedInBytes;
    int64_t allocatedItemsCount;
};


typedef void (*TypeProgsInvestMemLeakInvestClbk)(const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_curStack,
                                                 void* a_pUserData,
                                                 void** a_pDataForCurThread);


PROGSINVEST_MEMLEAKINVEST_EXPORT void ProgsInvestMemLeakInvestSkipThisStack(void)CPPUTILS_NOEXCEPT;
PROGSINVEST_MEMLEAKINVEST_EXPORT void ProgsInvestMemLeakInvestUnskipThisStack(void)CPPUTILS_NOEXCEPT;
PROGSINVEST_MEMLEAKINVEST_EXPORT void ProgsInvestMemLeakInvestRegisterClbk(TypeProgsInvestMemLeakInvestClbk a_clbk, void* a_pUserData)CPPUTILS_NOEXCEPT;
PROGSINVEST_MEMLEAKINVEST_EXPORT struct SProgsInvestMemLeakInvestSettings* ProgsInvestMemLeakInvestSettingsPtr(void)CPPUTILS_NOEXCEPT;
PROGSINVEST_MEMLEAKINVEST_EXPORT const struct SProgsInvestMemLeakInvestStatus* ProgsInvestMemLeakInvestStatusPtr(void)CPPUTILS_NOEXCEPT;


CPPUTILS_END_C


#endif  //  #ifdef PROGS_INVEST_MEM_LEAK_INVEST_USED

#endif  // #ifndef PROGS_INVEST_INCLUDE_PROGS_INVEST_MEM_LEAK_INVEST_H