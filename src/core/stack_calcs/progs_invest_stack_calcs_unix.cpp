//
// repo:            progs_invest
// file:			progs_invest_stack_calcs_unix.cpp
// path:			src/core/stack_calcs/progs_invest_stack_calcs_unix.cpp
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


PROGSINVEST_STACKCALCS_EXPORT struct SProgramsInvesigatorStack* ProgramsInvestigatorStackCopy(const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack, TypeAllocFreeHookMalloc a_malloc) CPPUTILS_NOEXCEPT
{
    const TypeAllocFreeHookMalloc aMalloc = a_malloc ? a_malloc : (&ProgramsInvestStackCalcDefaultMalloc);
    struct SProgramsInvesigatorStack* const pRetData = (struct SProgramsInvesigatorStack*)((*aMalloc)(sizeof(struct SProgramsInvesigatorStack)));
    if (!pRetData) {
        return CPPUTILS_NULL;
    }

    pRetData->ppFrames = (void**)((*aMalloc)(sizeof(void*) * ((size_t)(a_stack->numberOfFrames))));
    if (!(pRetData->ppFrames)) {
        (*(a_stack->m_free))(pRetData);
        return CPPUTILS_NULL;
    }

    pRetData->m_free = a_stack->m_free;
    pRetData->hash = a_stack->hash;
    pRetData->numberOfFrames = a_stack->numberOfFrames;
    pRetData->reserved01 = a_stack->reserved01;
    memcpy(pRetData->ppFrames, a_stack->ppFrames, CPPUTILS_STATIC_CAST(size_t, pRetData->numberOfFrames) * sizeof(void*));
    return pRetData;
}


PROGSINVEST_STACKCALCS_EXPORT void ProgramsInvestigatorStackSwap(struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack1, struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack2) CPPUTILS_NOEXCEPT
{
    struct SProgramsInvesigatorStack tmpStack;
    memcpy(&tmpStack, a_stack1, sizeof(struct SProgramsInvesigatorStack));
    memcpy(a_stack1, a_stack2, sizeof(struct SProgramsInvesigatorStack));
    memcpy(a_stack2, &tmpStack, sizeof(struct SProgramsInvesigatorStack));
}


PROGSINVEST_STACKCALCS_EXPORT size_t ProgramsInvestigatorStackSize(const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack) CPPUTILS_NOEXCEPT
{
    return CPPUTILS_STATIC_CAST(size_t, a_stack->numberOfFrames);
}


PROGSINVEST_STACKCALCS_EXPORT const struct SProgramsInvesigatorStackItemResolved* ProgramsInvestigatorStackItemResolved(const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack, TypeAllocFreeHookMalloc a_malloc, size_t a_frameNum) CPPUTILS_NOEXCEPT
{
    void* pFrame;
    DWORD_ci  dwAddress;
    struct SProgramsInvesigatorStackItemResolvedPrivate* pRetData;
    const TypeAllocFreeHookMalloc aMalloc = a_malloc ? a_malloc : (&ProgramsInvestStackCalcDefaultMalloc);
    const int frameNum = CPPUTILS_STATIC_CAST(int, a_frameNum);

    if ((frameNum < 0) || (frameNum > (a_stack->numberOfFrames))) {
        return CPPUTILS_NULL;
    }

    pRetData = (struct SProgramsInvesigatorStackItemResolvedPrivate*)((*aMalloc)(sizeof(struct SProgramsInvesigatorStackItemResolvedPrivate) * ((size_t)(a_stack->numberOfFrames))));
    if (!pRetData) {
        return CPPUTILS_NULL;
    }

    pRetData->publ.moduleName = CPPUTILS_NULL;
    pRetData->publ.functionName = CPPUTILS_NULL;
    pRetData->publ.sourceFile = CPPUTILS_NULL;
    pRetData->publ.lineNumber = -1;
    pRetData->publ.reserved01 = 0;
    pRetData->m_malloc = aMalloc;
    pRetData->stack = a_stack;
    pRetData->indInStack = frameNum;
    pRetData->reserved02 = 0;

    pFrame = a_stack->ppFrames[frameNum];
    dwAddress = CPPUTILS_STATIC_CAST(DWORD_ci, CPPUTILS_REINTERPRET_CAST(size_t, pFrame));

    cinternal_lw_recursive_mutex_lock(&s_mutex_for_dbg_functions);
    ProgramsInvestigatorStackGetFunctionNameInlineNoLock(pRetData, dwAddress);
    ProgramsInvestigatorStackGetSourceInfoInlineNoLock(pRetData, dwAddress);
    ProgramsInvestigatorStackGetModuleInfoInlineNoLock(pRetData, dwAddress);
    cinternal_lw_recursive_mutex_unlock(&s_mutex_for_dbg_functions);

    return &(pRetData->publ);

}


static inline struct SProgramsInvesigatorStackItemResolvedPrivate* ProgramsInvestigatorStackGetPrivItemFromPubl(const struct SProgramsInvesigatorStackItemResolved* CPPUTILS_ARG_NN a_stackItem) CPPUTILS_NOEXCEPT {
    struct SProgramsInvesigatorStackItemResolvedPrivate* const pRet = (struct SProgramsInvesigatorStackItemResolvedPrivate*)a_stackItem;
    return pRet;
}


PROGSINVEST_STACKCALCS_EXPORT const struct SProgramsInvesigatorStackItemResolved* ProgramsInvestigatorStackItemResolvedNext(const struct SProgramsInvesigatorStackItemResolved* CPPUTILS_ARG_NN a_stackItem) CPPUTILS_NOEXCEPT
{
    struct SProgramsInvesigatorStackItemResolvedPrivate* const pInpData = ProgramsInvestigatorStackGetPrivItemFromPubl(a_stackItem);
    return ProgramsInvestigatorStackItemResolved(pInpData->stack, pInpData->m_malloc, CPPUTILS_STATIC_CAST(size_t, pInpData->indInStack + 1));
}


PROGSINVEST_STACKCALCS_EXPORT void ProgramsInvestigatorStackItemResolvedClean(const struct SProgramsInvesigatorStackItemResolved* CPPUTILS_ARG_NN a_stackItem) CPPUTILS_NOEXCEPT
{
    if (a_stackItem) {
        struct SProgramsInvesigatorStackItemResolvedPrivate* const pInpData = ProgramsInvestigatorStackGetPrivItemFromPubl(a_stackItem);
        const TypeAllocFreeHookFree aFree = pInpData->stack->m_free;
        (*aFree)((void*)pInpData->publ.moduleName);
        (*aFree)((void*)pInpData->publ.functionName);
        (*aFree)((void*)pInpData->publ.sourceFile);
        (*aFree)(pInpData);
    }  //  if (a_stackItem) {
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