//
// file:			alloc_free_hook_core_alloc_free_hook_unix.c
// path:			src/core/alloc_free_hook_core_alloc_free_hook_unix.c
// created on:		2023 Mar 14
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//


#include <progs_invest/internal_header.h>

#ifdef PROGRAMS_INVEST_ALLOC_FREE_HOOK_USED

#ifndef cinternal_lw_recursive_mutex_create_needed
#define cinternal_lw_recursive_mutex_create_needed
#endif

#include <progs_invest/mem_leak_invest.h>
#include <progs_invest/alloc_free_hook.h>
#include <progs_invest/stack_calcs.h>
#include <cinternal/hash.h>
#include <cinternal/lw_mutex_recursive.h>
#include <cinternal/logger.h>
#include <cinternal/thread_local_sys.h>
#include <cinternal/disable_compiler_warnings.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <cinternal/undisable_compiler_warnings.h>


CPPUTILS_BEGIN_C


#define PROGS_INVEST_MEM_LEAK_AN_HASH_BY_STACK_BASKETS      4096
#define PROGS_INVEST_MEM_LEAK_AN_HASH_BY_MEM_BASKETS        65536
#define PROGS_INVEST_MEM_LEAK_AN_MAX_ALLOC_DEF              10000


struct SProgsInvestMemLeakStackItem{
    struct SProgramsInvesigatorStack*   pStack;
    CinternalHashItem_t                 hashByStackItem;
    int                                 allocsCount;
    int                                 reserved01;
};


struct SProgsInvestMemLeakMemItem{
    struct SProgsInvestMemLeakStackItem*    pStackData;
    size_t                                  m_size;
};


struct SProgsInvestMemLeakInvestTls{
    void*   pUserData;
    int8_t  useHook;
    int8_t  reserved01[(sizeof(void*)-1*sizeof(int8_t))/sizeof(int8_t)];
};


static void programs_investigator_mem_leak_invest_clean(void) CPPUTILS_NOEXCEPT;
static int ProgsInvestMemLeakInvestAddMemory(void* CPPUTILS_ARG_NN a_ptr, int a_goBackInStack, size_t a_size) CPPUTILS_NOEXCEPT;
static void ProgsInvestMemLeakInvestRemMemory(void* CPPUTILS_ARG_NN a_ptr) CPPUTILS_NOEXCEPT;
static int ProgsInvestMemLeakInvestRemoveAndAddMemory(void* CPPUTILS_ARG_NN a_ptrRem, void* CPPUTILS_ARG_NN a_ptrAdd, int a_goBackInStack, size_t a_size) CPPUTILS_NOEXCEPT;
static void* ProgsInvestMemLeakInvestMalloc(size_t a_size) CPPUTILS_NOEXCEPT;
static void* ProgsInvestMemLeakInvestCalloc(size_t, size_t) CPPUTILS_NOEXCEPT;
static void* ProgsInvestMemLeakInvestRealloc(void*, size_t) CPPUTILS_NOEXCEPT;
static void ProgsInvestMemLeakInvestFree(void*) CPPUTILS_NOEXCEPT;
static size_t ProgsInvestMemLeakInvesHasherByStack(const void* a_key, size_t a_keySize) CPPUTILS_NOEXCEPT;
static bool ProgsInvestMemLeakInvesIsEqualByStack(const void* key1, size_t keySize1, const void* key2, size_t keySize2) CPPUTILS_NOEXCEPT;
static bool ProgsInvestMemLeakInvesStoreKeyByStack(TypeCinternalAllocator a_allocator, void** a_pKeyStore, size_t* a_pKeySizeStore, const void* a_key, size_t a_keySize) CPPUTILS_NOEXCEPT;
static void ProgsInvestMemLeakInvesUnstoreKeyByStack(TypeCinternalDeallocator a_deallocator, void* a_key, size_t a_keySize) CPPUTILS_NOEXCEPT;
static void ProgsInvestMemLeakInvestTlsClean(void* a_tls) CPPUTILS_NOEXCEPT;
static void ProgsInvestMemLeakInvestDefaultClbk(const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_curStack,
                                                const struct SProgsInvestMemLeakInvestStat* CPPUTILS_ARG_NN a_memCurStat,
                                                void* a_pUserData,
                                                void** a_pDataForCurThread) CPPUTILS_NOEXCEPT;

static TypeProgsInvestMemLeakInvestClbk s_eventClbk = &ProgsInvestMemLeakInvestDefaultClbk;
static TypeAllocFreeHookMalloc s_mallocInitial = CPPUTILS_NULL;
static TypeAllocFreeHookCalloc s_callocInitial = CPPUTILS_NULL;
static TypeAllocFreeHookRealloc s_reallocInitial = CPPUTILS_NULL;
static TypeAllocFreeHookFree s_freeInitial = CPPUTILS_NULL;
static cinternal_lw_recursive_mutex_t   s_hashMutex;
static CinternalHash_t  s_hashByStack = CPPUTILS_NULL;
static CinternalHash_t  s_hashByMemory = CPPUTILS_NULL;
static CinternalTlsData s_tlsData = (CinternalTlsData)0;
static struct SProgsInvestMemLeakInvestStat s_dt = {};
static bool s_bIsHookActive = false;
static bool s_bModuleExitStarted = false;
static void* s_pUserData = CPPUTILS_NULL;


static inline struct SProgsInvestMemLeakInvestTls* ProgsInvestMemLeakInvestGetTlsPtrInline(void) CPPUTILS_NOEXCEPT{
    struct SProgsInvestMemLeakInvestTls* pTls = (struct SProgsInvestMemLeakInvestTls*)CinternalTlsGetSpecific(s_tlsData);
    if(pTls){
        return pTls;
    }
    pTls = (struct SProgsInvestMemLeakInvestTls*)AllocFreeHookCLibCalloc(1,sizeof(struct SProgsInvestMemLeakInvestTls));
    CinternalTlsSetSpecific(s_tlsData,pTls);
    return pTls;
}


static inline void AnalyzeStackSituationInline(struct SProgsInvestMemLeakStackItem* CPPUTILS_ARG_NN a_pStackData) CPPUTILS_NOEXCEPT {
    if((a_pStackData->allocsCount)>(s_dt.allocsMax)){
        s_dt.allocsMax = a_pStackData->allocsCount;
        if((a_pStackData->allocsCount)>(s_dt.maxAllocs)){
            struct SProgsInvestMemLeakInvestTls* const pTls = ProgsInvestMemLeakInvestGetTlsPtrInline();
            ++(s_dt.numberOfEvents);
            (*s_eventClbk)(a_pStackData->pStack,&s_dt,s_pUserData,&(pTls->pUserData));
        }  //  if((a_pStackData->allocsCount)>s_maxAllocs){
    }  //  if((a_pStackData->allocsCount)>snMax){
}


static inline void ProgsInvestMemLeakIestInitInline(void) CPPUTILS_NOEXCEPT {
    static bool sbMemLeakInvestInited = false;
    struct SProgsInvestMemLeakInvestTls* pTls;
    if(sbMemLeakInvestInited){
        return;
    }
    sbMemLeakInvestInited = true;
    pTls = ProgsInvestMemLeakInvestGetTlsPtrInline();
    if(!pTls){
        exit(1);
    }
    ProgramsInvestigatorAllocFreeHookInitLibraryIfNotInited();

    s_eventClbk = &ProgsInvestMemLeakInvestDefaultClbk;
    s_bModuleExitStarted = false;
    s_bIsHookActive = false;
    s_dt.maxAllocs = PROGS_INVEST_MEM_LEAK_AN_MAX_ALLOC_DEF;
    s_dt.numberOfEvents = 0;
    s_dt.allocsMax = 0;
    s_dt.memoryAllocatedInBytes = 0;
    s_dt.allocatedItemsCount = 0;

    s_mallocInitial = AllocFreeHookGetMallocFnc();
    s_callocInitial = AllocFreeHookGetCallocFnc();
    s_reallocInitial = AllocFreeHookGetReallocFnc();
    s_freeInitial = AllocFreeHookGetFreeFnc();

    if(cinternal_lw_recursive_mutex_create(&s_hashMutex)){
        exit(1);
    }

    s_hashByStack = CInternalHashCreateAnyEx(
        PROGS_INVEST_MEM_LEAK_AN_HASH_BY_STACK_BASKETS,
        &ProgsInvestMemLeakInvesHasherByStack,
        &ProgsInvestMemLeakInvesIsEqualByStack,
        &ProgsInvestMemLeakInvesStoreKeyByStack,
        &ProgsInvestMemLeakInvesUnstoreKeyByStack,
        &AllocFreeHookCLibMalloc,
        &AllocFreeHookCLibFree);
    if(!s_hashByStack){
        cinternal_lw_recursive_mutex_destroy(&s_hashMutex);
        exit(1);
    }

    s_hashByMemory = CInternalHashCreateSmlIntEx(
        PROGS_INVEST_MEM_LEAK_AN_HASH_BY_MEM_BASKETS,
        &AllocFreeHookCLibMalloc,
        &AllocFreeHookCLibFree);
    if(!s_hashByMemory){
        CInternalHashDestroy(s_hashByStack);
        cinternal_lw_recursive_mutex_destroy(&s_hashMutex);
        exit(1);
    }

    if(CinternalTlsAlloc(&s_tlsData,&ProgsInvestMemLeakInvestTlsClean)){
        CInternalHashDestroy(s_hashByMemory);
        CInternalHashDestroy(s_hashByStack);
        cinternal_lw_recursive_mutex_destroy(&s_hashMutex);
        exit(1);
    }

    pTls->useHook = 1;

    atexit(&programs_investigator_mem_leak_invest_clean);

    cinternal_lw_recursive_mutex_lock(&s_hashMutex);
    AllocFreeHookSetFreeFnc(&ProgsInvestMemLeakInvestFree);
    AllocFreeHookSetReallocFnc(&ProgsInvestMemLeakInvestRealloc);
    AllocFreeHookSetMallocFnc(&ProgsInvestMemLeakInvestMalloc);
    AllocFreeHookSetCallocFnc(&ProgsInvestMemLeakInvestCalloc);
    cinternal_lw_recursive_mutex_unlock(&s_hashMutex);

    pTls->useHook = 0;

    s_bIsHookActive = true;
}


ALLOCFREEHOOK_EXPORT void ProgsInvestMemLeakInvestSkipThisStack(void)CPPUTILS_NOEXCEPT
{
    struct SProgsInvestMemLeakInvestTls* pTls;
    ProgsInvestMemLeakIestInitInline();
    pTls = ProgsInvestMemLeakInvestGetTlsPtrInline();
    ++(pTls->useHook);
}


ALLOCFREEHOOK_EXPORT void ProgsInvestMemLeakInvestUnskipThisStack(void)CPPUTILS_NOEXCEPT
{
    struct SProgsInvestMemLeakInvestTls* pTls;
    ProgsInvestMemLeakIestInitInline();
    pTls = ProgsInvestMemLeakInvestGetTlsPtrInline();
    --(pTls->useHook);
}


ALLOCFREEHOOK_EXPORT void ProgsInvestMemLeakInvestSetMaxAllocsForEvent(int a_maxAllocs)CPPUTILS_NOEXCEPT
{
    ProgsInvestMemLeakIestInitInline();
    s_dt.maxAllocs = a_maxAllocs;
}


ALLOCFREEHOOK_EXPORT void ProgsInvestMemLeakInvestRegisterClbk(TypeProgsInvestMemLeakInvestClbk a_clbk, void* a_pUserData)CPPUTILS_NOEXCEPT
{
    ProgsInvestMemLeakIestInitInline();
    s_pUserData = a_pUserData;
    s_eventClbk = a_clbk;
}


ALLOCFREEHOOK_EXPORT const struct SProgsInvestMemLeakInvestStat* ProgsInvestMemLeakInvestMemData(void)CPPUTILS_NOEXCEPT
{
    ProgsInvestMemLeakIestInitInline();
    return &s_dt;
}


static void* ProgsInvestMemLeakInvestMalloc(size_t a_size) CPPUTILS_NOEXCEPT
{
    void* pRet;
    pRet = (*s_mallocInitial)(a_size);
    if(pRet){
        if(ProgsInvestMemLeakInvestAddMemory(pRet,2,a_size)){
            (*s_freeInitial)(pRet);
            return CPPUTILS_NULL;
        }  //  if(ProgsInvestMemLeakInvestAddMemory(pRet,1)){
    }  //  if(pRet){
    return pRet;
}


static void* ProgsInvestMemLeakInvestCalloc(size_t a_number, size_t a_singleItemSize) CPPUTILS_NOEXCEPT
{
    void* pRet;
    pRet = (*s_callocInitial)(a_number,a_singleItemSize);
    if(pRet){
        if(ProgsInvestMemLeakInvestAddMemory(pRet,2,a_number*a_singleItemSize)){
            (*s_freeInitial)(pRet);
            return CPPUTILS_NULL;
        }  //  if(ProgsInvestMemLeakInvestAddMemory(pRet,1)){
    }  //  if(pRet){
    return pRet;
}


static void* ProgsInvestMemLeakInvestRealloc(void* a_ptr, size_t a_size) CPPUTILS_NOEXCEPT
{
    void* pRet;
    pRet = (*s_reallocInitial)(a_ptr,a_size);
    if(pRet!=a_ptr){
        if(a_ptr && pRet){
            if(ProgsInvestMemLeakInvestRemoveAndAddMemory(a_ptr,pRet,2,a_size)){
                (*s_freeInitial)(pRet);
                return CPPUTILS_NULL;
            }  //  if(ProgsInvestMemLeakInvestRemoveAndAddMemory(a_ptr,pRet,1)){
        }  //  if(a_ptr && pRet){
        else if(a_ptr){
            ProgsInvestMemLeakInvestRemMemory(a_ptr);
        }  //  else if(a_ptr){
        else{
            if(ProgsInvestMemLeakInvestAddMemory(pRet,2,a_size)){
                (*s_freeInitial)(pRet);
                return CPPUTILS_NULL;
            }  //  if(ProgsInvestMemLeakInvestAddMemory(pRet,1)){
        }
    }  //  if(pRet!=a_ptr){
    return pRet;
}


static void ProgsInvestMemLeakInvestFree(void* a_ptr) CPPUTILS_NOEXCEPT
{
    if(a_ptr){
        ProgsInvestMemLeakInvestRemMemory(a_ptr);
        (*s_freeInitial)(a_ptr);
    }  //  if(a_ptr){
}


/*//////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

static void ProgsInvestMemLeakInvestDefaultClbk(const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_curStack,
                                                const struct SProgsInvestMemLeakInvestStat* CPPUTILS_ARG_NN a_memCurStat,
                                                void* a_pUserData,
                                                void** a_pDataForCurThread) CPPUTILS_NOEXCEPT
{
    (void)a_curStack;
    (void)a_memCurStat;
    (void)a_pUserData;
    (void)a_pDataForCurThread;
}


static inline int ProgsInvestMemLeakInvestAddMemoryInlineNoLock(struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_pStack, void* CPPUTILS_ARG_NN a_ptr, size_t a_size) CPPUTILS_NOEXCEPT{
    size_t unHashByStack, unHashByMem;
    struct SProgsInvestMemLeakStackItem* pStackData;
    CinternalHashItem_t hashByStackItem, itemyMem;

    hashByStackItem = CInternalHashFindEx(s_hashByStack,a_pStack,0,&unHashByStack);
    if(hashByStackItem){
        pStackData = (struct SProgsInvestMemLeakStackItem*)hashByStackItem->data;
        ++(pStackData->allocsCount);
        ProgramsInvestigatorStackFree(a_pStack);
    }  //  if(hashByStackItem){
    else{
        pStackData = (struct SProgsInvestMemLeakStackItem*)AllocFreeHookCLibMalloc(sizeof(struct SProgsInvestMemLeakStackItem));
        if(!pStackData){
            return 1;
        }
        pStackData->pStack = a_pStack;
        pStackData->allocsCount = 1;
        pStackData->reserved01 = 0;
        hashByStackItem = CInternalHashAddDataWithKnownHash(s_hashByStack,pStackData,a_pStack,0,unHashByStack);
        if(!hashByStackItem){
            AllocFreeHookCLibFree(pStackData);
            ProgramsInvestigatorStackFree(a_pStack);
            return 1;
        }
        pStackData->hashByStackItem = hashByStackItem;
    }  //  else of 'if(hashByStackItem){'

    AnalyzeStackSituationInline(pStackData);

    itemyMem = CInternalHashFindEx(s_hashByMemory,a_ptr,0,&unHashByMem);
    if(itemyMem){
        CInternalLogWarning("+++++++++++++++++++++++++++++++++++++++++++++ Memory is there");
        struct SProgsInvestMemLeakStackItem* const pStackDataOld = ((struct SProgsInvestMemLeakMemItem*)itemyMem->data)->pStackData;
        if((pStackDataOld->pStack)==a_pStack){
            return 0;
        }
        if(ProgramsInvestigatorStackAreSame(pStackDataOld->pStack,a_pStack)){
            return 0;
        }
        //itemyMem->data = pStackData;
    }  //  if(itemyMem){
    else{
        struct SProgsInvestMemLeakMemItem* const pMemData = (struct SProgsInvestMemLeakMemItem*)AllocFreeHookCLibMalloc(sizeof(struct SProgsInvestMemLeakMemItem));
        if(!pMemData){
            return 1;
        }
        pMemData->pStackData = pStackData;
        pMemData->m_size = a_size;
        itemyMem = CInternalHashAddDataWithKnownHash(s_hashByMemory,pMemData,a_ptr,0,unHashByMem);
        if(!itemyMem){
            //AllocFreeHookCLibFree(pStackData);
            // todo:
            return 1;
        }
        s_dt.memoryAllocatedInBytes += ((int64_t)a_size);
        ++(s_dt.allocatedItemsCount);
    }

    return 0;
}


static inline int ProgsInvestMemLeakInvestAddMemoryInline(void* CPPUTILS_ARG_NN a_ptr, int a_goBackInStack, size_t a_size) CPPUTILS_NOEXCEPT{
    int nReturn;
    struct SProgramsInvesigatorStack* const pCurStack = ProgramsInvestigatorStackGetCurrent(a_goBackInStack+1,&AllocFreeHookCLibMalloc,&AllocFreeHookCLibFree);
    if(!pCurStack){
        return 1;
    }
    cinternal_lw_recursive_mutex_lock(&s_hashMutex);
    nReturn = ProgsInvestMemLeakInvestAddMemoryInlineNoLock(pCurStack,a_ptr,a_size);
    cinternal_lw_recursive_mutex_unlock(&s_hashMutex);
    return nReturn;
}


static int ProgsInvestMemLeakInvestAddMemory(void* CPPUTILS_ARG_NN a_ptr, int a_goBackInStack, size_t a_size) CPPUTILS_NOEXCEPT
{
    if(s_bIsHookActive){
        struct SProgsInvestMemLeakInvestTls* const pTls = ProgsInvestMemLeakInvestGetTlsPtrInline();
        if((pTls->useHook)<1){
            int nReturn;
            const int8_t useHook = pTls->useHook;
            pTls->useHook = 1;
            nReturn = ProgsInvestMemLeakInvestAddMemoryInline(a_ptr,a_goBackInStack+1,a_size);
            pTls->useHook = useHook;
            return nReturn;
        }  //  if((pTls->useHook)<1){
    }  //  if(s_bIsHookActive){
    return 0;
}


static inline void ProgsInvestMemLeakInvestRemMemoryInlineNoLock(void* CPPUTILS_ARG_NN a_ptr) CPPUTILS_NOEXCEPT{
    size_t unHashByMem;
    const CinternalHashItem_t itemyMem = CInternalHashFindEx(s_hashByMemory,a_ptr,0,&unHashByMem);
    if(itemyMem){
        size_t unHashByStack=1;
        struct SProgsInvestMemLeakMemItem* const pMemData = (struct SProgsInvestMemLeakMemItem*)itemyMem->data;
        struct SProgsInvestMemLeakStackItem* const pStackData = pMemData->pStackData;
        if((--(pStackData->allocsCount))<1){
            const CinternalHashConstBasic_t hashByStack = CinternalHashGetBasic(s_hashByStack);
            CInternalHashRemoveDataEx(s_hashByStack,pStackData->hashByStackItem);
            ProgramsInvestigatorStackFree(pStackData->pStack);
            AllocFreeHookCLibFree(pStackData);
            unHashByStack = hashByStack->count;
        }  //  if((--(pStackData->allocsCount))<1){
        CInternalHashRemoveDataEx(s_hashByMemory,itemyMem);
        s_dt.memoryAllocatedInBytes -= ((int64_t)(pMemData->m_size));
        --(s_dt.allocatedItemsCount);
        AllocFreeHookCLibFree(pMemData);
        if(s_bModuleExitStarted && (unHashByStack<1)){
            const CinternalHashConstBasic_t hashByMemory = CinternalHashGetBasic(s_hashByMemory);
            const size_t unHashByMem = hashByMemory->count;
            if(unHashByMem<1){
                AllocFreeHookSetReallocFnc(s_reallocInitial);
                AllocFreeHookSetFreeFnc(s_freeInitial);
                CinternalTlsDelete(s_tlsData);
                CInternalHashDestroy(s_hashByMemory);
                CInternalHashDestroy(s_hashByStack);
                cinternal_lw_recursive_mutex_destroy(&s_hashMutex);
            }  //  if(unHashByMem<1){
        }  //  if(s_bModuleExitStarted && (unHashByStack<1)){
    }  //  if(itemyMem){
}


static void ProgsInvestMemLeakInvestRemMemory(void* CPPUTILS_ARG_NN a_ptr) CPPUTILS_NOEXCEPT
{
    cinternal_lw_recursive_mutex_lock(&s_hashMutex);
    ProgsInvestMemLeakInvestRemMemoryInlineNoLock(a_ptr);
    cinternal_lw_recursive_mutex_unlock(&s_hashMutex);
}


static int ProgsInvestMemLeakInvestRemoveAndAddMemory(void* CPPUTILS_ARG_NN a_ptrRem, void* CPPUTILS_ARG_NN a_ptrAdd, int a_goBackInStack, size_t a_size) CPPUTILS_NOEXCEPT
{
    int nReturn = 0;
    int8_t useHook;
    struct SProgramsInvesigatorStack* pCurStack = CPPUTILS_NULL;
    struct SProgsInvestMemLeakInvestTls* pTls = CPPUTILS_NULL;

    if(s_bIsHookActive){
        pCurStack = ProgramsInvestigatorStackGetCurrent(a_goBackInStack+1,&AllocFreeHookCLibMalloc,&AllocFreeHookCLibFree);
        if(!pCurStack){
            return 1;
        }
        pTls = ProgsInvestMemLeakInvestGetTlsPtrInline();
        useHook = pTls->useHook;
        pTls->useHook = 1;
    }  //  if(s_bIsHookActive){

    cinternal_lw_recursive_mutex_lock(&s_hashMutex);

    ProgsInvestMemLeakInvestRemMemoryInlineNoLock(a_ptrRem);

    if(pTls){
        nReturn = ProgsInvestMemLeakInvestAddMemoryInlineNoLock(pCurStack,a_ptrAdd,a_size);
        pTls->useHook = useHook;
    }  //  if(pTls){

    cinternal_lw_recursive_mutex_unlock(&s_hashMutex);

    return nReturn;
}


static size_t ProgsInvestMemLeakInvesHasherByStack(const void* a_key, size_t a_keySize) CPPUTILS_NOEXCEPT
{
    const struct SProgramsInvesigatorStack* const pStack = (const struct SProgramsInvesigatorStack*)a_key;
    const size_t cunHash = ProgramsInvestigatorStackGetHash(pStack);
    (void)a_keySize;
    return cunHash;
}


static bool ProgsInvestMemLeakInvesIsEqualByStack(const void* a_key1, size_t a_keySize1, const void* a_key2, size_t a_keySize2) CPPUTILS_NOEXCEPT
{
    const struct SProgramsInvesigatorStack* const pStack1 = (const struct SProgramsInvesigatorStack*)a_key1;
    const struct SProgramsInvesigatorStack* const pStack2 = (const struct SProgramsInvesigatorStack*)a_key2;
    const bool isSame = ProgramsInvestigatorStackAreSame(pStack1,pStack2);
    (void)a_keySize1;
    (void)a_keySize2;
    return isSame;
}


static bool ProgsInvestMemLeakInvesStoreKeyByStack(TypeCinternalAllocator a_allocator, void** a_pKeyStore, size_t* a_pKeySizeStore, const void* a_key, size_t a_keySize) CPPUTILS_NOEXCEPT
{
    CPPUTILS_STATIC_CAST(void, a_allocator);
    CPPUTILS_STATIC_CAST(void, a_keySize);
    CPPUTILS_STATIC_CAST(void, a_pKeySizeStore);
    *a_pKeyStore = CPPUTILS_CONST_CAST(void*, a_key);
    return true;
}


static void ProgsInvestMemLeakInvesUnstoreKeyByStack(TypeCinternalDeallocator a_deallocator, void* a_key, size_t a_keySize) CPPUTILS_NOEXCEPT
{
    CPPUTILS_STATIC_CAST(void, a_deallocator);
    CPPUTILS_STATIC_CAST(void, a_key);
    CPPUTILS_STATIC_CAST(void, a_keySize);
}


static void ProgsInvestMemLeakInvestTlsClean(void* a_tls) CPPUTILS_NOEXCEPT
{
    struct SProgsInvestMemLeakInvestTls* const pTls = (struct SProgsInvestMemLeakInvestTls*)a_tls;
    AllocFreeHookCLibFree(pTls);
}


/*//////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

CPPUTILS_C_CODE_INITIALIZER(programs_investigator_mem_leak_invest_init){
#ifdef PROGRAMS_INVEST_ALLOC_FREE_HOOK_USED_AUTO_INIT
    ProgsInvestMemLeakIestInitInline();
#endif
}


static void programs_investigator_mem_leak_invest_clean(void) CPPUTILS_NOEXCEPT
{
    struct SProgsInvestMemLeakInvestTls* pTls;

    s_bModuleExitStarted = true;
    s_bIsHookActive = false;

    pTls = ProgsInvestMemLeakInvestGetTlsPtrInline();

    pTls->useHook = 1;
    cinternal_lw_recursive_mutex_lock(&s_hashMutex);

    AllocFreeHookSetMallocFnc(s_mallocInitial);
    AllocFreeHookSetCallocFnc(s_callocInitial);

    cinternal_lw_recursive_mutex_unlock(&s_hashMutex);
    pTls->useHook = 0;
}


CPPUTILS_END_C


#endif  //  #ifdef PROGRAMS_INVEST_ALLOC_FREE_HOOK_USED