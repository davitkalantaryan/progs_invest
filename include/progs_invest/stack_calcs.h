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
#include <cinternal/disable_compiler_warnings.h>
#include <stddef.h>
#include <stdbool.h>
#include <cinternal/undisable_compiler_warnings.h>


CPPUTILS_BEGIN_C

typedef void* (*TypeProgramsInvestMalloc)(size_t);
typedef void  (*TypeProgramsInvestFree)(void*);


struct SProgramsInvesigatorStack;
ALLOCFREEHOOK_EXPORT struct SProgramsInvesigatorStack* ProgramsInvestigatorStackGetCurrent(int a_goBackInTheStackCalc, TypeProgramsInvestMalloc a_malloc, TypeProgramsInvestFree a_free) CPPUTILS_NOEXCEPT;
ALLOCFREEHOOK_EXPORT void ProgramsInvestigatorStackFree(struct SProgramsInvesigatorStack* a_stack) CPPUTILS_NOEXCEPT;
ALLOCFREEHOOK_EXPORT size_t ProgramsInvestigatorStackGetHash(const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack) CPPUTILS_NOEXCEPT;
ALLOCFREEHOOK_EXPORT bool ProgramsInvestigatorStackAreSame(const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack1, const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack2) CPPUTILS_NOEXCEPT;
ALLOCFREEHOOK_EXPORT void ProgramsInvestigatorStackPrint(const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack) CPPUTILS_NOEXCEPT;


CPPUTILS_END_C


#endif  //  #ifdef PROGS_INVEST_STACK_CALCS_USED

#endif  // #ifndef PROGS_INVEST_INCLUDE_PROGS_INVEST_STACK_CALCS_H