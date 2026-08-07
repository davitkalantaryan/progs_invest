//
// repo:            progs_invest
// file:			stack_calcs.h
// path:			include/progs_invest/stack_calcs.h
// created on:		2023 Mar 08
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//

#ifndef PROGS_INVEST_INCLUDE_PROGS_INVEST_STACK_CALCS_H
#define PROGS_INVEST_INCLUDE_PROGS_INVEST_STACK_CALCS_H

#include <progs_invest/internal_header.h>

#ifdef PROGS_INVEST_STACK_CALCS_USED

#include <progs_invest/export_symbols.h>
#include <progs_invest/types01.h>
#include <cinternal/disable_compiler_warnings.h>
#include <stddef.h>
#include <stdbool.h>
#include <cinternal/undisable_compiler_warnings.h>


CPPUTILS_BEGIN_C


struct SProgramsInvesigatorStack;
struct SProgramsInvesigatorStackItemResolved {
    const char*     moduleName;
    const char*     functionName;
    const char*     sourceFile;
    int             lineNumber;
    int             reserved01;
};


PROGSINVEST_STACKCALCS_EXPORT struct SProgramsInvesigatorStack* ProgramsInvestigatorStackGetCurrent(int a_goBackInTheStackCalc, TypeAllocFreeHookMalloc a_malloc, TypeAllocFreeHookFree a_free) CPPUTILS_NOEXCEPT;
PROGSINVEST_STACKCALCS_EXPORT void ProgramsInvestigatorStackFree(struct SProgramsInvesigatorStack* a_stack) CPPUTILS_NOEXCEPT;
PROGSINVEST_STACKCALCS_EXPORT size_t ProgramsInvestigatorStackGetHash(const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack) CPPUTILS_NOEXCEPT;
PROGSINVEST_STACKCALCS_EXPORT bool ProgramsInvestigatorStackAreSame(const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack1, const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack2) CPPUTILS_NOEXCEPT;
PROGSINVEST_STACKCALCS_EXPORT struct SProgramsInvesigatorStack* ProgramsInvestigatorStackCopy(const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack, TypeAllocFreeHookMalloc a_malloc) CPPUTILS_NOEXCEPT;
PROGSINVEST_STACKCALCS_EXPORT void ProgramsInvestigatorStackSwap(struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack1, struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack2) CPPUTILS_NOEXCEPT;
PROGSINVEST_STACKCALCS_EXPORT size_t ProgramsInvestigatorStackSize(const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack) CPPUTILS_NOEXCEPT;
PROGSINVEST_STACKCALCS_EXPORT const struct SProgramsInvesigatorStackItemResolved* ProgramsInvestigatorStackItemResolved(const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack, TypeAllocFreeHookMalloc a_malloc, size_t a_frameNum) CPPUTILS_NOEXCEPT;
PROGSINVEST_STACKCALCS_EXPORT const struct SProgramsInvesigatorStackItemResolved* ProgramsInvestigatorStackItemResolvedNext(const struct SProgramsInvesigatorStackItemResolved* CPPUTILS_ARG_NN a_stackItem) CPPUTILS_NOEXCEPT;
PROGSINVEST_STACKCALCS_EXPORT void ProgramsInvestigatorStackItemResolvedClean(const struct SProgramsInvesigatorStackItemResolved* a_stackItem) CPPUTILS_NOEXCEPT;
PROGSINVEST_STACKCALCS_EXPORT void ProgramsInvestigatorStackPrint(const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack) CPPUTILS_NOEXCEPT;


CPPUTILS_END_C


#endif  //  #ifdef PROGS_INVEST_STACK_CALCS_USED

#endif  // #ifndef PROGS_INVEST_INCLUDE_PROGS_INVEST_STACK_CALCS_H