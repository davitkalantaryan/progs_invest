//
// file:			crash_investigator_linux_simple_malloc_free.cpp
// path:			src/core/crash_investigator_linux_simple_malloc_free.cpp
// created on:		2023 Mar 06
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//


#include <progs_invest/internal_header.h>

#ifdef PROGRAMS_INVEST_ALLOC_FREE_HOOK_USED
#ifndef _WIN32

#include <progs_invest/alloc_free_hook.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <dlfcn.h>


#define MEMORY_HANDLER_INIT_MEM_SIZE    16384


CPPUTILS_BEGIN_C


struct SMemoryHandlerInitMemData{
    size_t      totalSize;
    char        reserved[16-sizeof(size_t)];
};

static void* MemoryHandlerMallocInitialStatic(size_t a_size) CPPUTILS_NOEXCEPT;
static void* MemoryHandlerCallocInitialStatic(size_t a_nmemb, size_t a_size) CPPUTILS_NOEXCEPT;
static void* MemoryHandlerReallocInitialStatic(void* a_ptr, size_t a_size) CPPUTILS_NOEXCEPT;
static void  MemoryHandlerFreeInitialStatic(void* a_ptr) CPPUTILS_NOEXCEPT;
static void* MemoryHandlerReallocFinalStatic(void* a_ptr, size_t a_size) CPPUTILS_NOEXCEPT;
static void MemoryHandlerFreeFinalStatic(void* a_ptr) CPPUTILS_NOEXCEPT;

static void* AllocFreeHookCLibMallocInitialStatic(size_t a_size) CPPUTILS_NOEXCEPT;
static void* AllocFreeHookCLibCallocInitialStatic(size_t a_nmemb, size_t a_size) CPPUTILS_NOEXCEPT;
static void* AllocFreeHookCLibReallocInitialStatic(void* a_ptr, size_t a_size) CPPUTILS_NOEXCEPT;
static void  AllocFreeHookCLibFreeInitialStatic(void* a_ptr) CPPUTILS_NOEXCEPT;

static int s_nLibraryInited = 0;
CPPUTILS_DLL_PRIVATE TypeAllocFreeHookMalloc  g_malloc  = &MemoryHandlerMallocInitialStatic;
CPPUTILS_DLL_PRIVATE TypeAllocFreeHookCalloc  g_calloc  = &MemoryHandlerCallocInitialStatic;
CPPUTILS_DLL_PRIVATE TypeAllocFreeHookRealloc g_realloc = &MemoryHandlerReallocInitialStatic;
CPPUTILS_DLL_PRIVATE TypeAllocFreeHookFree    g_free    = &MemoryHandlerFreeInitialStatic;
static TypeAllocFreeHookRealloc s_realloc_user = CPPUTILS_NULL;
static TypeAllocFreeHookFree    s_free_user    = CPPUTILS_NULL;

static TypeAllocFreeHookMalloc  s_malloc_c_lib  = &AllocFreeHookCLibMallocInitialStatic;
static TypeAllocFreeHookCalloc  s_calloc_c_lib  = &AllocFreeHookCLibCallocInitialStatic;
static TypeAllocFreeHookRealloc s_realloc_c_lib = &AllocFreeHookCLibReallocInitialStatic;
static TypeAllocFreeHookFree    s_free_c_lib    = &AllocFreeHookCLibFreeInitialStatic;

static size_t   s_unInitialMemoryOffset = 0;
static char     s_vcInitialBuffer[MEMORY_HANDLER_INIT_MEM_SIZE];


/*///////////////////////////////////////////////////////////////////////////////////////////////////////////////*/


static void crash_investigator_linux_simple_alloc_free_inc_clean(void) CPPUTILS_NOEXCEPT{

    g_malloc  = s_malloc_c_lib;
    g_calloc  = s_calloc_c_lib;
    s_realloc_user = s_realloc_c_lib;
    s_free_user    = s_free_c_lib;

}


static inline void InitLibraryIfNotInitedInline(void) CPPUTILS_NOEXCEPT{
    static int snStartedInitLibrary = 0;

    if(snStartedInitLibrary){return;}
    snStartedInitLibrary = 1;
    if(s_nLibraryInited){return;}

    s_malloc_c_lib  = (TypeAllocFreeHookMalloc)dlsym(RTLD_NEXT, "malloc");
    s_calloc_c_lib  = (TypeAllocFreeHookCalloc)dlsym(RTLD_NEXT, "calloc");
    s_realloc_c_lib = (TypeAllocFreeHookRealloc)dlsym(RTLD_NEXT, "realloc");
    s_free_c_lib    = (TypeAllocFreeHookFree)dlsym(RTLD_NEXT, "free");
    if((!s_malloc_c_lib)||(!s_calloc_c_lib)||(!s_realloc_c_lib)||(!s_free_c_lib)){
        //fprintf(stderr, "Unable to get addresses of original functions (malloc/realloc/free)\n. Application will exit");
        //fflush(stderr);
        exit(1);
    }

    g_malloc  = s_malloc_c_lib;
    g_calloc  = s_calloc_c_lib;
    s_realloc_user = s_realloc_c_lib;
    s_free_user = s_free_c_lib;
    g_realloc = &MemoryHandlerReallocFinalStatic;
    g_free    = &MemoryHandlerFreeFinalStatic;

    s_nLibraryInited = 1;

    atexit(&crash_investigator_linux_simple_alloc_free_inc_clean);

}


/*///////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

ALLOCFREEHOOK_EXPORT void ProgramsInvestigatorAllocFreeHookInitLibraryIfNotInited(void) CPPUTILS_NOEXCEPT
{
    InitLibraryIfNotInitedInline();
}


ALLOCFREEHOOK_EXPORT void AllocFreeHookSetMallocFnc(TypeAllocFreeHookMalloc a_malloc) CPPUTILS_NOEXCEPT
{
    InitLibraryIfNotInitedInline();
    g_malloc = a_malloc;
}


ALLOCFREEHOOK_EXPORT void AllocFreeHookSetCallocFnc(TypeAllocFreeHookCalloc a_calloc) CPPUTILS_NOEXCEPT
{
    InitLibraryIfNotInitedInline();
    g_calloc = a_calloc;
}


ALLOCFREEHOOK_EXPORT void AllocFreeHookSetReallocFnc(TypeAllocFreeHookRealloc a_realloc) CPPUTILS_NOEXCEPT
{
    InitLibraryIfNotInitedInline();
    s_realloc_user = a_realloc;
}


ALLOCFREEHOOK_EXPORT void AllocFreeHookSetFreeFnc(TypeAllocFreeHookFree a_free) CPPUTILS_NOEXCEPT
{
    InitLibraryIfNotInitedInline();
    s_free_user = a_free;
}


/*///////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

ALLOCFREEHOOK_EXPORT void* AllocFreeHookCLibMalloc(size_t a_size) CPPUTILS_NOEXCEPT
{
    return (*s_malloc_c_lib)(a_size);
}


ALLOCFREEHOOK_EXPORT void* AllocFreeHookCLibCalloc(size_t a_nmemb, size_t a_size) CPPUTILS_NOEXCEPT
{
    return (*s_calloc_c_lib)(a_nmemb,a_size);
}


ALLOCFREEHOOK_EXPORT void* AllocFreeHookCLibRealloc(void* a_ptr, size_t a_size) CPPUTILS_NOEXCEPT
{
    return (*s_realloc_c_lib)(a_ptr,a_size);
}


ALLOCFREEHOOK_EXPORT void AllocFreeHookCLibFree(void* a_ptr) CPPUTILS_NOEXCEPT
{
    (*s_free_c_lib)(a_ptr);
}


/*///////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

ALLOCFREEHOOK_EXPORT TypeAllocFreeHookMalloc AllocFreeHookGetMallocFnc(void) CPPUTILS_NOEXCEPT
{
    return g_malloc;
}


ALLOCFREEHOOK_EXPORT TypeAllocFreeHookCalloc AllocFreeHookGetCallocFnc(void) CPPUTILS_NOEXCEPT
{
    return g_calloc;
}


ALLOCFREEHOOK_EXPORT TypeAllocFreeHookRealloc AllocFreeHookGetReallocFnc(void) CPPUTILS_NOEXCEPT
{
    return s_realloc_user;
}


ALLOCFREEHOOK_EXPORT TypeAllocFreeHookFree AllocFreeHookGetFreeFnc(void) CPPUTILS_NOEXCEPT
{
    return s_free_user;
}


/*///////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

static inline size_t AllocFreeHookCalculateRoundedMemorySizeInline(size_t a_initialSize) CPPUTILS_NOEXCEPT {
    a_initialSize += sizeof(struct SMemoryHandlerInitMemData);
    if(0x8 & a_initialSize){
        return ((a_initialSize>>3)+1)<<3;
    }
    return a_initialSize;
}


static void* MemoryHandlerMallocInitialStatic(size_t a_size) CPPUTILS_NOEXCEPT
{
    InitLibraryIfNotInitedInline();
    if(a_size){
        const size_t cunTotalSize = AllocFreeHookCalculateRoundedMemorySizeInline(a_size);
        const size_t cunNewOffset = s_unInitialMemoryOffset + cunTotalSize;
        if(cunNewOffset<MEMORY_HANDLER_INIT_MEM_SIZE){
            char*const pcCurrentMemPointer = s_vcInitialBuffer + s_unInitialMemoryOffset;
            struct SMemoryHandlerInitMemData*const pItem = (struct SMemoryHandlerInitMemData*)pcCurrentMemPointer;
            pItem->totalSize = cunTotalSize;
            return pcCurrentMemPointer + sizeof(struct SMemoryHandlerInitMemData);
        }
    }

    return CPPUTILS_NULL;
}



static void* AllocFreeHookCLibMallocInitialStatic(size_t a_size) CPPUTILS_NOEXCEPT
{
    InitLibraryIfNotInitedInline();
    return (*s_malloc_c_lib)(a_size);
}


static void* AllocFreeHookCLibCallocInitialStatic(size_t a_nmemb, size_t a_size) CPPUTILS_NOEXCEPT
{
    InitLibraryIfNotInitedInline();
    return (*s_calloc_c_lib)(a_nmemb,a_size);
}


static void* AllocFreeHookCLibReallocInitialStatic(void* a_ptr, size_t a_size) CPPUTILS_NOEXCEPT
{
    InitLibraryIfNotInitedInline();
    return (*s_realloc_c_lib)(a_ptr,a_size);
}


static void AllocFreeHookCLibFreeInitialStatic(void* a_ptr) CPPUTILS_NOEXCEPT
{
    InitLibraryIfNotInitedInline();
    (*s_free_c_lib)(a_ptr);
}



static void* MemoryHandlerCallocInitialStatic(size_t a_nmemb, size_t a_size2) CPPUTILS_NOEXCEPT
{
    const size_t cunTotalWantedByUser = a_nmemb * a_size2;
    InitLibraryIfNotInitedInline();
    if(cunTotalWantedByUser){
        const size_t cunTotalSize = AllocFreeHookCalculateRoundedMemorySizeInline(cunTotalWantedByUser);
        const size_t cunNewOffset = s_unInitialMemoryOffset + cunTotalSize;
        if(cunNewOffset<MEMORY_HANDLER_INIT_MEM_SIZE){
            char* pReturnPointer;
            char*const pcCurrentMemPointer = s_vcInitialBuffer + s_unInitialMemoryOffset;
            struct SMemoryHandlerInitMemData*const pItem = (struct SMemoryHandlerInitMemData*)pcCurrentMemPointer;
            pItem->totalSize = cunTotalSize;
            pReturnPointer = pcCurrentMemPointer + sizeof(struct SMemoryHandlerInitMemData);
            memset(pReturnPointer,0,cunTotalWantedByUser);
            return pReturnPointer;
        }

    }
    return CPPUTILS_NULL;
}


static inline size_t MemoryHandlerCalculateExistingMemOffsetInline(void* a_ptr) CPPUTILS_NOEXCEPT {
    const size_t cunOffsetInit = (size_t)(((char*)a_ptr)-s_vcInitialBuffer);
    return cunOffsetInit - sizeof(struct SMemoryHandlerInitMemData);
}


static void* MemoryHandlerReallocInitialStatic(void* a_ptr, size_t a_size) CPPUTILS_NOEXCEPT
{
    InitLibraryIfNotInitedInline();
    if(a_ptr){
        const size_t cunOffset = MemoryHandlerCalculateExistingMemOffsetInline(a_ptr);
        char*const pcCurrentMemPointer = s_vcInitialBuffer + cunOffset;
        struct SMemoryHandlerInitMemData*const pItem = (struct SMemoryHandlerInitMemData*)pcCurrentMemPointer;
        if(((pItem->totalSize)+cunOffset)==s_unInitialMemoryOffset){
            // this is a last element, simply increase it, or decrease
            const size_t cunTotalSize = a_size?AllocFreeHookCalculateRoundedMemorySizeInline(a_size):0;
            const size_t cunNewOffset = cunOffset+cunTotalSize;
            if(cunNewOffset<MEMORY_HANDLER_INIT_MEM_SIZE){
                pItem->totalSize = cunTotalSize;
                s_unInitialMemoryOffset = cunNewOffset;
                return a_ptr;
            }

            return CPPUTILS_NULL;
        }

        // in case if (((pItem->totalSize)+cunOffset)!=s_unInitialMemoryOffset) we should forgot about existing memory and allocate new one
    }

    return MemoryHandlerMallocInitialStatic(a_size);
}


static void MemoryHandlerFreeInitialStatic(void* a_ptr) CPPUTILS_NOEXCEPT
{
    InitLibraryIfNotInitedInline();
    if(a_ptr){
        const size_t cunOffset = MemoryHandlerCalculateExistingMemOffsetInline(a_ptr);
        const char*const pcCurrentMemPointer = s_vcInitialBuffer + cunOffset;
        const struct SMemoryHandlerInitMemData* pItem = (const struct SMemoryHandlerInitMemData*)pcCurrentMemPointer;
        if(((pItem->totalSize)+cunOffset)==s_unInitialMemoryOffset){
            // this is a last element, simply increase it, or decrease
            s_unInitialMemoryOffset = cunOffset;
        }

        // in case if (((pItem->totalSize)+cunOffset)!=s_unInitialMemoryOffset) we should forgot about existing memory
    }
}



static void* MemoryHandlerReallocFinalStatic(void* a_ptr, size_t a_size) CPPUTILS_NOEXCEPT
{
    if(a_ptr){
        const size_t cunMemPosition = (size_t)((char*)a_ptr);
        const size_t cunInitMemPosition = (size_t)s_vcInitialBuffer;
        if(cunMemPosition>cunInitMemPosition){
            size_t cunOffset = cunMemPosition-cunInitMemPosition;
            if(cunOffset<MEMORY_HANDLER_INIT_MEM_SIZE){
                // the speech is about freeing iniytial memory. The initial memory is not in use, so simply forgot it
                if(a_size>0){
                    void* pRet;
                    size_t memcpySize;
                    char* pcCurrentMemPointer;
                    struct SMemoryHandlerInitMemData* pItem;
                    cunOffset -= sizeof(struct SMemoryHandlerInitMemData);
                    pcCurrentMemPointer = s_vcInitialBuffer + cunOffset;
                    pItem = (struct SMemoryHandlerInitMemData*)pcCurrentMemPointer;
                    pRet = (*s_realloc_user)(CPPUTILS_NULL,a_size);
                    if(pRet){
                        memcpySize = (a_size<pItem->totalSize)?a_size:(pItem->totalSize);
                        memcpy(pRet,pcCurrentMemPointer+sizeof(struct SMemoryHandlerInitMemData),memcpySize);
                        return pRet;
                    }
                    //  realloc failed, return null
                    return CPPUTILS_NULL;
                }
                else{
                    return CPPUTILS_NULL;
                }
            }  //  if(cunOffset<MEMORY_HANDLER_INIT_MEM_SIZE){
        }  //  if(cunMemPosition>cunInitMemPosition){
    }  //  if(a_ptr){

    return (*s_realloc_user)(a_ptr,a_size);  // in all other cases call the users callback
}


static void MemoryHandlerFreeFinalStatic(void* a_ptr) CPPUTILS_NOEXCEPT
{
    if(a_ptr){
        const size_t cunMemPosition = (size_t)((char*)a_ptr);
        const size_t cunInitMemPosition = (size_t)s_vcInitialBuffer;
        if(cunMemPosition>cunInitMemPosition){
            const size_t cunOffset = cunMemPosition-cunInitMemPosition;
            if(cunOffset<MEMORY_HANDLER_INIT_MEM_SIZE){
                // the speech is about freeing initial memory. The initial memory is not in use, so simply forgot it
                return;
            }  //  if(cunOffset<MEMORY_HANDLER_INIT_MEM_SIZE){
        }  //  if(cunMemPosition>cunInitMemPosition){
    }  //  if(a_ptr){

    (*s_free_user)(a_ptr);  // in all other cases call the users callback
}



CPPUTILS_END_C


#endif  //  #ifndef _WIN32
#endif  //  #ifdef PROGRAMS_INVEST_ALLOC_FREE_HOOK_USED