//
// repo:            progs_invest
// file:			progs_invest_stack_calcs_unix.cpp
// path:			src/core/stack_calcs/progs_invest_stack_calcs_unix.cpp
// created on:		2023 Mar 14
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//


#include <progs_invest/internal_header.h>

#ifdef PROGS_INVEST_STACK_CALCS_USED
#ifdef _WIN32

#include <progs_invest/stack_calcs.h>
#include <cinternal/disable_compiler_warnings.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <cinternal/undisable_compiler_warnings.h>


CPPUTILS_BEGIN_C


#define PROGRAMS_INVEST_MAX_STACK   8192


struct SProgramsInvesigatorStack {
    TypeAllocFreeHookFree   m_free;
    void**                  ppFrames;
    size_t                  hash;     
    int                     numberOfFrames;
    int                     reserved01;
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
    struct SProgramsInvesigatorStack* pRetData;
    DWORD stackHash;
    void* vpBuffer[PROGRAMS_INVEST_MAX_STACK];
    const TypeAllocFreeHookMalloc aMalloc = a_malloc ? a_malloc : (&ProgramsInvestStackCalcDefaultMalloc);
    const TypeAllocFreeHookFree aFree = a_free ? a_free : (&ProgramsInvestStackCalcDefaultFree);
    const int cnNumberOfFramesPerThisStack = (int)CaptureStackBackTrace((ULONG)a_goBackInTheStackCalc, PROGRAMS_INVEST_MAX_STACK, vpBuffer, &stackHash);
    if (cnNumberOfFramesPerThisStack < 1) {
        return CPPUTILS_NULL;
    }

    pRetData = (struct SProgramsInvesigatorStack*)((*aMalloc)(sizeof(struct SProgramsInvesigatorStack)));
    if (!pRetData) {
        return CPPUTILS_NULL;
    }

    pRetData->ppFrames = (void**)((*aMalloc)(sizeof(void*) * ((size_t)cnNumberOfFramesPerThisStack)));
    if (!(pRetData->ppFrames)) {
        (*aFree)(pRetData);
        return CPPUTILS_NULL;
    }

    pRetData->m_free = aFree;
    pRetData->hash = (size_t)stackHash;
    pRetData->numberOfFrames = cnNumberOfFramesPerThisStack;
    pRetData->reserved01 = 0;
    memcpy(pRetData->ppFrames, vpBuffer, CPPUTILS_STATIC_CAST(size_t, pRetData->numberOfFrames) * sizeof(void*));
    return pRetData;
}


PROGSINVEST_STACKCALCS_EXPORT void ProgramsInvestigatorStackFree(struct SProgramsInvesigatorStack* a_stack) CPPUTILS_NOEXCEPT
{
    if (a_stack) {
        const TypeAllocFreeHookFree aFree = a_stack->m_free;
        (*aFree)(a_stack->ppFrames);
        (*aFree)(a_stack);
    }
}


PROGSINVEST_STACKCALCS_EXPORT size_t ProgramsInvestigatorStackGetHash(const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack) CPPUTILS_NOEXCEPT
{
    return a_stack->hash;
}


PROGSINVEST_STACKCALCS_EXPORT bool ProgramsInvestigatorStackAreSame(const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack1, const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack2) CPPUTILS_NOEXCEPT
{
    int ind;
    if ((a_stack1->numberOfFrames) != (a_stack2->numberOfFrames)) {
        return false;
    }
    for (ind = 0; ind < (a_stack1->numberOfFrames); ++ind) {
        if (((char*)a_stack1->ppFrames[ind]) != ((char*)a_stack2->ppFrames[ind])) {
            return false;
        }  //  if(((char*)a_stack1->ppFrames[ind])!=((char*)a_stack2->ppFrames[ind])){
    }  //  for(ind = 0; ind<a_stack->numberOfFrames; ++ind){
    return true;
}


PROGSINVEST_STACKCALCS_EXPORT void ProgramsInvestigatorStackPrint(const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack) CPPUTILS_NOEXCEPT
{
    (void)a_stack;
}


CPPUTILS_END_C


#endif  //  #ifdef _WIN32
#endif  //  #ifdef PROGS_INVEST_STACK_CALCS_USED