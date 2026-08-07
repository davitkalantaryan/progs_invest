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

#ifndef cinternal_lw_recursive_mutex_create_needed
#define cinternal_lw_recursive_mutex_create_needed
#endif

#include <progs_invest/stack_calcs.h>
#include <cinternal/lw_mutex_recursive.h>
#include <cinternal/logger.h>
#include <cinternal/disable_compiler_warnings.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <DbgHelp.h>
#include <cinternal/undisable_compiler_warnings.h>


CPPUTILS_BEGIN_C


#ifdef _MSC_VER
#pragma comment(lib, "dbghelp.lib")
#endif


#define PROGRAMS_INVEST_MAX_STACK       8192
#define PROGS_INVEST_UNKNOWN_NAME_STR   "__unknown"
#define PROGS_INVEST_UNKNOWN_NAME_LEN   (sizeof(PROGS_INVEST_UNKNOWN_NAME_STR) - 1)


static void StackInvestCleanupRoutine(void) CPPUTILS_NOEXCEPT;

struct SProgramsInvesigatorStack {
    TypeAllocFreeHookFree   m_free;
    void**                  ppFrames;
    size_t                  hash;     
    int                     numberOfFrames;
    int                     reserved01;
};


struct SProgramsInvesigatorStackItemResolvedPrivate {
    struct SProgramsInvesigatorStackItemResolved    publ;
    TypeAllocFreeHookMalloc                         m_malloc;
    const struct SProgramsInvesigatorStack*         stack;
    int                                             indInStack;
    int                                             reserved02;
};


static HANDLE s_currentProcess = CPPUTILS_NULL;
static cinternal_lw_recursive_mutex_t  s_mutex_for_dbg_functions;


static void* ProgramsInvestStackCalcDefaultMalloc(size_t a_size) CPPUTILS_NOEXCEPT
{
    return malloc(a_size);
}


static void ProgramsInvestStackCalcDefaultFree(void* a_ptr) CPPUTILS_NOEXCEPT
{
    free(a_ptr);
}


static inline void StackInvestInitializationRoutineInline(void) CPPUTILS_NOEXCEPT {
    if (s_currentProcess) { return; }
    if (cinternal_lw_recursive_mutex_create(&s_mutex_for_dbg_functions)) {
        CInternalLogError("Unable create mutex");
        exit(1);
    }
    s_currentProcess = GetCurrentProcess();
    cinternal_lw_recursive_mutex_lock(&s_mutex_for_dbg_functions);
    if (SymInitialize(s_currentProcess, CPPUTILS_NULL, TRUE)) {
        cinternal_lw_recursive_mutex_unlock(&s_mutex_for_dbg_functions);
    }  //  if (SymInitialize(s_currentProcess, CPPUTILS_NULL, TRUE)) {
    else {
        // SymInitialize failed
        const DWORD dwError = GetLastError();
        cinternal_lw_recursive_mutex_unlock(&s_mutex_for_dbg_functions);
        s_currentProcess = CPPUTILS_NULL;
        CInternalLogError("SymInitialize returned error : %d", CPPUTILS_STATIC_CAST(int, dwError));
        exit(1);
    }  //  else of 'if (SymInitialize(s_currentProcess, CPPUTILS_NULL, TRUE)) {'
    atexit(&StackInvestCleanupRoutine);
}


/*////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

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


#ifdef _WIN64
typedef DWORD64  DWORD_ci;
#else
typedef DWORD  DWORD_ci;
#endif


static inline char* ProgramsInvestigatorStackStrdup(const char* CPPUTILS_ARG_NN a_src, size_t a_strLen, TypeAllocFreeHookMalloc a_malloc) CPPUTILS_NOEXCEPT  {
    const size_t strLenPlus1 = (size_t)(a_strLen + 1);
    char* const pRet = (char*)((*a_malloc)(sizeof(char) * strLenPlus1));
    if (pRet) {
        memcpy(pRet, a_src, strLenPlus1);
    }
    return pRet;
}


static inline void ProgramsInvestigatorStackGetFunctionNameInlineNoLock(struct SProgramsInvesigatorStackItemResolvedPrivate* CPPUTILS_ARG_NN a_pStackItem, DWORD_ci a_dwAddres) CPPUTILS_NOEXCEPT  {
    DWORD64  dwDisplacement = 0;
    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

    pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    pSymbol->MaxNameLen = MAX_SYM_NAME;

    if (SymFromAddr(s_currentProcess, a_dwAddres, &dwDisplacement, pSymbol)) {
        a_pStackItem->publ.functionName = ProgramsInvestigatorStackStrdup(pSymbol->Name,(size_t)(pSymbol->NameLen), a_pStackItem->m_malloc);
    }
    else {
        a_pStackItem->publ.functionName = ProgramsInvestigatorStackStrdup(PROGS_INVEST_UNKNOWN_NAME_STR, PROGS_INVEST_UNKNOWN_NAME_LEN, a_pStackItem->m_malloc);
    }
}


static inline void ProgramsInvestigatorStackGetSourceInfoInlineNoLock(struct SProgramsInvesigatorStackItemResolvedPrivate* CPPUTILS_ARG_NN a_pStackItem, DWORD_ci a_dwAddres) CPPUTILS_NOEXCEPT {
    DWORD  dwDisplacement;
    IMAGEHLP_LINE64 line;

    SymSetOptions(SYMOPT_LOAD_LINES);

    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    if (SymGetLineFromAddr64(s_currentProcess, a_dwAddres, &dwDisplacement, &line)) {
        if (line.FileName) {
            a_pStackItem->publ.sourceFile = ProgramsInvestigatorStackStrdup(line.FileName,strlen(line.FileName),a_pStackItem->m_malloc);
        }
        else {
            a_pStackItem->publ.sourceFile = ProgramsInvestigatorStackStrdup(PROGS_INVEST_UNKNOWN_NAME_STR, PROGS_INVEST_UNKNOWN_NAME_LEN, a_pStackItem->m_malloc);
        }
        a_pStackItem->publ.lineNumber = CPPUTILS_STATIC_CAST(int, line.LineNumber);
    }
    else {
        // SymGetLineFromAddr64 failed
        a_pStackItem->publ.lineNumber = -1;
        a_pStackItem->publ.sourceFile = ProgramsInvestigatorStackStrdup(PROGS_INVEST_UNKNOWN_NAME_STR, PROGS_INVEST_UNKNOWN_NAME_LEN, a_pStackItem->m_malloc);
    }
}


static inline void ProgramsInvestigatorStackGetModuleInfoInlineNoLock(struct SProgramsInvesigatorStackItemResolvedPrivate* CPPUTILS_ARG_NN a_pStackItem, DWORD_ci a_dwAddres) CPPUTILS_NOEXCEPT {
    IMAGEHLP_MODULE aModuleInfo;
    aModuleInfo.SizeOfStruct = sizeof(IMAGEHLP_MODULE);

    if (SymGetModuleInfo(s_currentProcess, a_dwAddres, &aModuleInfo)) {
        a_pStackItem->publ.moduleName = ProgramsInvestigatorStackStrdup(aModuleInfo.ImageName, strlen(aModuleInfo.ImageName), a_pStackItem->m_malloc);
    }
    else {
        a_pStackItem->publ.moduleName = ProgramsInvestigatorStackStrdup(PROGS_INVEST_UNKNOWN_NAME_STR, PROGS_INVEST_UNKNOWN_NAME_LEN, a_pStackItem->m_malloc);
    }
}


PROGSINVEST_STACKCALCS_EXPORT const struct SProgramsInvesigatorStackItemResolved* ProgramsInvestigatorStackItemResolved(const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack, TypeAllocFreeHookMalloc a_malloc, size_t a_frameNum) CPPUTILS_NOEXCEPT
{
    void* pFrame;
    DWORD_ci  dwAddress;
    struct SProgramsInvesigatorStackItemResolvedPrivate* pRetData;
    const TypeAllocFreeHookMalloc aMalloc = a_malloc ? a_malloc : (&ProgramsInvestStackCalcDefaultMalloc);
    const int frameNum = CPPUTILS_STATIC_CAST(int, a_frameNum);
    
    if((frameNum <0)||(frameNum >(a_stack->numberOfFrames))) {
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


static inline struct SProgramsInvesigatorStackItemResolvedPrivate* ProgramsInvestigatorStackGetPrivItemFromPubl(const struct SProgramsInvesigatorStackItemResolved* CPPUTILS_ARG_NN a_stackItem) CPPUTILS_NOEXCEPT  {
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


PROGSINVEST_STACKCALCS_EXPORT void ProgramsInvestigatorStackPrint(const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_stack, TypeAllocFreeHookMalloc a_malloc) CPPUTILS_NOEXCEPT
{
    int i;
    const struct SProgramsInvesigatorStackItemResolved* pItem;

    for (i = 0; i < (a_stack->numberOfFrames); ++i) {
        pItem = ProgramsInvestigatorStackItemResolved(a_stack, a_malloc, CPPUTILS_STATIC_CAST(size_t, i));
        if (pItem) {
            CinternalLoggerMakeLogOnlyText(0,
                "    fl: \"%s\", ln: %d, fn: %s\n",
                pItem->sourceFile, pItem->lineNumber, pItem->functionName);
            ProgramsInvestigatorStackItemResolvedClean(pItem);
        }  //  if (pItem) {
    }  //  for (i = 0; i < (a_stack->numberOfFrames); ++i) {

    CinternalLoggerMakeLog(0, "", "", 0, "", CinternalLogTypeFinalize, CinternalLogCategoryNone, "\n");
}


/*////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

CPPUTILS_C_CODE_INITIALIZER(StackInvestInitializationRoutine)
{
    StackInvestInitializationRoutineInline();
}


static void StackInvestCleanupRoutine(void) CPPUTILS_NOEXCEPT
{
    if (s_currentProcess) {
        cinternal_lw_recursive_mutex_lock(&s_mutex_for_dbg_functions);
        SymCleanup(s_currentProcess);
        cinternal_lw_recursive_mutex_unlock(&s_mutex_for_dbg_functions);
        s_currentProcess = CPPUTILS_NULL;
    }

    cinternal_lw_recursive_mutex_destroy(&s_mutex_for_dbg_functions);
}


CPPUTILS_END_C


#endif  //  #ifdef _WIN32
#endif  //  #ifdef PROGS_INVEST_STACK_CALCS_USED