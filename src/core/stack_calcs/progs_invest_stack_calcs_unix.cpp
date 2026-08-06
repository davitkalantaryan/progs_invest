//
// file:			alloc_free_hook_core_alloc_free_hook_unix.c
// path:			src/core/alloc_free_hook_core_alloc_free_hook_unix.c
// created on:		2023 Mar 14
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//


#include <progs_invest/internal_header.h>

#ifdef PROGS_INVEST_STACK_CALCS_USED
#ifndef _WIN32

#include <progs_invest/stack_calcs.h>
#include <cinternal/logger.h>
#include <cinternal/disable_compiler_warnings.h>
#include <execinfo.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <cinternal/undisable_compiler_warnings.h>

void printBacktraceSourceLines(void* const* frames, int count);

CPPUTILS_BEGIN_C

#define PROGRAMS_INVEST_MAX_STACK   8192


struct SProgramsInvesigatorStack{
    int                     numberOfFrames;
    int                     reserved01;
    void**                  ppFrames;
    TypeAllocFreeHookFree   m_free;
};


static void* ProgramsInvestStackCalcDefaultMalloc(size_t a_size) CPPUTILS_NOEXCEPT
{
    return malloc(a_size);
}


static void ProgramsInvestStackCalcDefaultFree(void* a_ptr) CPPUTILS_NOEXCEPT
{
    free(a_ptr);
}


PROGSINVEST_STACKCALCS_EXPORT struct SProgramsInvesigatorStack* ProgramsInvestigatorStackGetCurrent(int a_goBackInTheStackCalc, TypeAllocFreeHookMalloc a_malloc, TypeAllocFreeHookFree a_free) CPPUTILS_NOEXCEPT
{
    int numberOfFrames;
    const TypeAllocFreeHookMalloc aMalloc = a_malloc ? a_malloc : (&ProgramsInvestStackCalcDefaultMalloc);
    const TypeAllocFreeHookFree aFree = a_free ? a_free : (&ProgramsInvestStackCalcDefaultFree);
    struct SProgramsInvesigatorStack* pRet = CPPUTILS_NULL;
    void* vpStackFramesTmp[PROGRAMS_INVEST_MAX_STACK];
    const int cnNumberOfFramesPerThisStack = backtrace(vpStackFramesTmp,PROGRAMS_INVEST_MAX_STACK);
    //CInternalLogDebugLogLvl(3,"Current stack dept is %d",cnNumberOfFramesPerThisStack);
    if(cnNumberOfFramesPerThisStack>=PROGRAMS_INVEST_MAX_STACK){
        CInternalLogWarning("Current stack dept is %d, maybe truncated",cnNumberOfFramesPerThisStack);
    }
    if(cnNumberOfFramesPerThisStack<=a_goBackInTheStackCalc){
        return CPPUTILS_NULL;
    }
    numberOfFrames = cnNumberOfFramesPerThisStack-a_goBackInTheStackCalc;
    pRet = (struct SProgramsInvesigatorStack*)((*aMalloc)((size_t)numberOfFrames));
    if(!pRet){
        return CPPUTILS_NULL;
    }
    pRet->ppFrames = (void**)((*aMalloc)(sizeof(void*)*((size_t)numberOfFrames)));
    if(!(pRet->ppFrames)){
        (*aFree)(pRet);
        return CPPUTILS_NULL;
    }
    pRet->numberOfFrames = numberOfFrames;
    pRet->reserved01 = 0;
    pRet->m_free = aFree;
    memcpy(pRet->ppFrames,&(vpStackFramesTmp[a_goBackInTheStackCalc]), CPPUTILS_STATIC_CAST(size_t,numberOfFrames)*sizeof(void*));
    return pRet;
}


PROGSINVEST_STACKCALCS_EXPORT void ProgramsInvestigatorStackFree(struct SProgramsInvesigatorStack* a_stack) CPPUTILS_NOEXCEPT
{
    if(a_stack){
        const TypeAllocFreeHookFree aFree = a_stack->m_free;
        (*aFree)(a_stack->ppFrames);
        (*aFree)(a_stack);
    }  //  if(a_stack){
}


PROGSINVEST_STACKCALCS_EXPORT size_t ProgramsInvestigatorStackGetHash(const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack) CPPUTILS_NOEXCEPT
{
    size_t unReturn = 0;
    size_t koef = 1;
    int ind;
    for(ind = 0; ind<(a_stack->numberOfFrames); ++ind){
        unReturn += koef * ((size_t)a_stack->ppFrames[ind]);
        koef *= 2;
    }  //  for(ind = 0; ind<a_stack->numberOfFrames; ++ind){
    return unReturn;
}


PROGSINVEST_STACKCALCS_EXPORT bool ProgramsInvestigatorStackAreSame(const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack1, const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack2) CPPUTILS_NOEXCEPT
{
    int ind;
    if((a_stack1->numberOfFrames) != (a_stack2->numberOfFrames)){
        return false;
    }
    for(ind = 0; ind<(a_stack1->numberOfFrames); ++ind){
        if(((char*)a_stack1->ppFrames[ind])!=((char*)a_stack2->ppFrames[ind])){
            return false;
        }  //  if(((char*)a_stack1->ppFrames[ind])!=((char*)a_stack2->ppFrames[ind])){
    }  //  for(ind = 0; ind<a_stack->numberOfFrames; ++ind){
    return true;
}


PROGSINVEST_STACKCALCS_EXPORT void ProgramsInvestigatorStackPrint(const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack) CPPUTILS_NOEXCEPT
{
    //char** strings = backtrace_symbols(a_stack->ppFrames, a_stack->numberOfFrames);
    //if(strings){
    //    int j;
    //    for (j = 0; j < (a_stack->numberOfFrames); j++)
    //        printf("%s\n", strings[j]);
    //}
    printBacktraceSourceLines(a_stack->ppFrames,a_stack->numberOfFrames);
}


CPPUTILS_END_C


#endif  //  #ifndef _WIN32
#endif  //  #ifdef PROGS_INVEST_STACK_CALCS_USED