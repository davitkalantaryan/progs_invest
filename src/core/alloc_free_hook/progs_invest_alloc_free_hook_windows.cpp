//
// file:			alloc_free_hook_core_alloc_free_hook_windows.cpp
// path:			src/core/alloc_free_hook/windows/alloc_free_hook_core_alloc_free_hook_windows.cpp
// created on:		2023 Mar 08
// created by:		Davit Kalantaryan (davit.kalantaryan@gmail.com)
//

#include <cinternal/internal_header.h>


#ifdef _WIN32


#include <progs_invest/alloc_free_hook.h>
#include <cinternal/replace_function_sys.h>
#include <stdlib.h>


CPPUTILS_BEGIN_C

//#define CInternalReplaceFunctionsMac	CInternalReplaceFunctions
#define CInternalReplaceFunctionsMac	CInternalReplaceFunctionsAllModules

typedef HMODULE (WINAPI *TypeLoadLibraryA)(LPCSTR lpLibFileName);
typedef HMODULE (WINAPI *TypeLoadLibraryW)(LPWSTR lpLibFileName);
typedef HMODULE (WINAPI *TypeLoadLibraryExA)(LPCSTR lpLibFileName,HANDLE hFile,DWORD  dwFlags);
typedef HMODULE (WINAPI *TypeLoadLibraryExW)(LPWSTR lpLibFileName, HANDLE hFile, DWORD  dwFlags);


static int s_nIsInited = 0;

static TypeAllocFreeHookMalloc  s_malloc_c_lib	/*= &malloc		*/;
static TypeAllocFreeHookCalloc  s_calloc_c_lib	/*= &calloc		*/;
static TypeAllocFreeHookRealloc	s_realloc_c_lib /*= &realloc	*/;
static TypeAllocFreeHookFree	s_free_c_lib	/*= &free		*/;

static TypeLoadLibraryA		s_loadlibA_st_lib	/*= &LoadLibraryA		*/;
static TypeLoadLibraryW		s_loadlibW_st_lib	/*= &LoadLibraryW		*/;
static TypeLoadLibraryExA	s_loadlibExA_st_lib /*= &LoadLibraryExA		*/;
static TypeLoadLibraryExW	s_loadlibExW_st_lib	/*= &LoadLibraryExW		*/;

static TypeAllocFreeHookMalloc  g_malloc;
static TypeAllocFreeHookCalloc  g_calloc;
static TypeAllocFreeHookRealloc g_realloc;
static TypeAllocFreeHookFree    g_free;


static int alloc_free_hook_cleanup(void) CPPUTILS_NOEXCEPT;


static void* AllocFreeHookMalloc(size_t a_size)
{
	return (*g_malloc)(a_size);
}


static void* AllocFreeHookCalloc(size_t a_nmemb, size_t a_size)
{
	return (*g_calloc)(a_nmemb,a_size);
}


static void* AllocFreeHookRealloc(void* a_ptr, size_t a_size)
{
	return (*g_realloc)(a_ptr, a_size);
}


static void AllocFreeHookFree(void* a_ptr)
{
	(*g_free)(a_ptr);
}


/*///////////////////////////////////////////////////////////////*/

static inline void ReplaceAllocFreeFunctionsPrepareInline(struct SCInternalReplaceFunctionData* a_vReplaceData) {
	a_vReplaceData[0].funcname = "malloc";
	a_vReplaceData[0].newFuncAddress = CPPUTILS_STATIC_CAST(const void*, &AllocFreeHookMalloc);
	a_vReplaceData[0].replaceIfAddressIs = CPPUTILS_STATIC_CAST(const void*, s_malloc_c_lib);

	a_vReplaceData[1].funcname = "calloc";
	a_vReplaceData[1].newFuncAddress = CPPUTILS_STATIC_CAST(const void*, &AllocFreeHookCalloc);
	a_vReplaceData[1].replaceIfAddressIs = CPPUTILS_STATIC_CAST(const void*, s_calloc_c_lib);

	a_vReplaceData[2].funcname = "realloc";
	a_vReplaceData[2].newFuncAddress = CPPUTILS_STATIC_CAST(const void*, &AllocFreeHookRealloc);
	a_vReplaceData[2].replaceIfAddressIs = CPPUTILS_STATIC_CAST(const void*, s_realloc_c_lib);


	a_vReplaceData[3].funcname = "free";
	a_vReplaceData[3].newFuncAddress = CPPUTILS_STATIC_CAST(const void*, &AllocFreeHookFree);
	a_vReplaceData[3].replaceIfAddressIs = CPPUTILS_STATIC_CAST(const void*, s_free_c_lib);
}


static HMODULE WINAPI CinternalLoadLibraryA(LPCSTR a_lpLibFileName)
{
	const HMODULE retMod = (*s_loadlibA_st_lib)(a_lpLibFileName);
	if (retMod) {
		struct SCInternalReplaceFunctionData vReplaceData[4];
		ReplaceAllocFreeFunctionsPrepareInline(vReplaceData);
		CInternalReplaceFunctionsForModule(retMod, 4, vReplaceData);
	}

	return retMod;
}


static HMODULE WINAPI CinternalLoadLibraryW(LPWSTR a_lpLibFileName)
{
	const HMODULE retMod = (*s_loadlibW_st_lib)(a_lpLibFileName);
	if (retMod) {
		struct SCInternalReplaceFunctionData vReplaceData[4];
		ReplaceAllocFreeFunctionsPrepareInline(vReplaceData);
		CInternalReplaceFunctionsForModule(retMod, 4, vReplaceData);
	}

	return retMod;
}


static HMODULE WINAPI CinternalLoadLibraryExA(LPCSTR a_lpLibFileName, HANDLE a_hFile, DWORD  a_dwFlags)
{
	const HMODULE retMod = (*s_loadlibExA_st_lib)(a_lpLibFileName,a_hFile,a_dwFlags);
	if (retMod) {
		struct SCInternalReplaceFunctionData vReplaceData[4];
		ReplaceAllocFreeFunctionsPrepareInline(vReplaceData);
		CInternalReplaceFunctionsForModule(retMod, 4, vReplaceData);
	}

	return retMod;
}


static HMODULE WINAPI CinternalLoadLibraryExW(LPWSTR a_lpLibFileName, HANDLE a_hFile, DWORD  a_dwFlags)
{
	const HMODULE retMod = (*s_loadlibExW_st_lib)(a_lpLibFileName, a_hFile, a_dwFlags);
	if (retMod) {
		struct SCInternalReplaceFunctionData vReplaceData[4];
		ReplaceAllocFreeFunctionsPrepareInline(vReplaceData);
		CInternalReplaceFunctionsForModule(retMod, 4, vReplaceData);
	}

	return retMod;
}


static inline void alloc_free_hook_initialize_inline(void) {
	if (!s_nIsInited) {
		struct SCInternalReplaceFunctionData vReplaceData[4];

		s_malloc_c_lib	= &malloc;
		s_calloc_c_lib	= &calloc;
		s_realloc_c_lib	= &realloc;
		s_free_c_lib	= &free;

		s_loadlibA_st_lib	= &LoadLibraryA;
		s_loadlibW_st_lib	= &LoadLibraryW;
		s_loadlibExA_st_lib = &LoadLibraryExA;
		s_loadlibExW_st_lib	= &LoadLibraryExW;

		g_malloc	= s_malloc_c_lib;
		g_calloc	= s_calloc_c_lib;
		g_realloc	= s_realloc_c_lib;
		g_free		= s_free_c_lib;

		ReplaceAllocFreeFunctionsPrepareInline(vReplaceData);
		CInternalReplaceFunctionsMac(4, vReplaceData);
		
		// LoadLibrary and friends
		vReplaceData[0].funcname = "LoadLibraryA";
		vReplaceData[0].newFuncAddress = CPPUTILS_STATIC_CAST(const void*, &CinternalLoadLibraryA);
		vReplaceData[0].replaceIfAddressIs = CPPUTILS_STATIC_CAST(const void*, s_loadlibA_st_lib);

		vReplaceData[1].funcname = "LoadLibraryW";
		vReplaceData[1].newFuncAddress = CPPUTILS_STATIC_CAST(const void*, &CinternalLoadLibraryW);
		vReplaceData[1].replaceIfAddressIs = CPPUTILS_STATIC_CAST(const void*, s_loadlibW_st_lib);

		vReplaceData[2].funcname = "LoadLibraryExA";
		vReplaceData[2].newFuncAddress = CPPUTILS_STATIC_CAST(const void*, &CinternalLoadLibraryExA);
		vReplaceData[2].replaceIfAddressIs = CPPUTILS_STATIC_CAST(const void*, s_loadlibExA_st_lib);

		vReplaceData[3].funcname = "LoadLibraryExW";
		vReplaceData[3].newFuncAddress = CPPUTILS_STATIC_CAST(const void*, &CinternalLoadLibraryExW);
		vReplaceData[3].replaceIfAddressIs = CPPUTILS_STATIC_CAST(const void*, s_loadlibExW_st_lib);

		CInternalReplaceFunctionsMac(4, vReplaceData);

		_onexit(&alloc_free_hook_cleanup);

		s_nIsInited = 1;
	}
}


/*///////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

ALLOCFREEHOOK_EXPORT void* AllocFreeHookCLibMalloc(size_t a_size)
{
	return (*s_malloc_c_lib)(a_size);
}


ALLOCFREEHOOK_EXPORT void* AllocFreeHookCLibCalloc(size_t a_nmemb, size_t a_size)
{
	return (*s_calloc_c_lib)(a_nmemb, a_size);
}


ALLOCFREEHOOK_EXPORT void* AllocFreeHookCLibRealloc(void* a_ptr, size_t a_size)
{
	return (*s_realloc_c_lib)(a_ptr, a_size);
}


ALLOCFREEHOOK_EXPORT void AllocFreeHookCLibFree(void* a_ptr)
{
	(*s_free_c_lib)(a_ptr);
}


/*///////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

ALLOCFREEHOOK_EXPORT void AllocFreeHookSetMallocFnc(TypeAllocFreeHookMalloc a_malloc)
{
	alloc_free_hook_initialize_inline();
	g_malloc = a_malloc;
}


ALLOCFREEHOOK_EXPORT void AllocFreeHookSetCallocFnc(TypeAllocFreeHookCalloc a_calloc)
{
	alloc_free_hook_initialize_inline();
	g_calloc = a_calloc;
}


ALLOCFREEHOOK_EXPORT void AllocFreeHookSetReallocFnc(TypeAllocFreeHookRealloc a_realloc)
{
	alloc_free_hook_initialize_inline();
	g_realloc = a_realloc;
}


ALLOCFREEHOOK_EXPORT void AllocFreeHookSetFreeFnc(TypeAllocFreeHookFree a_free)
{
	alloc_free_hook_initialize_inline();
	g_free = a_free;
}


/*///////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

ALLOCFREEHOOK_EXPORT TypeAllocFreeHookMalloc AllocFreeHookGetMallocFnc(void)
{
	return g_malloc;
}


ALLOCFREEHOOK_EXPORT TypeAllocFreeHookCalloc AllocFreeHookGetCallocFnc(void)
{
	return g_calloc;
}


ALLOCFREEHOOK_EXPORT TypeAllocFreeHookRealloc AllocFreeHookGetReallocFnc(void)
{
	return g_realloc;
}


ALLOCFREEHOOK_EXPORT TypeAllocFreeHookFree AllocFreeHookGetFreeFnc(void)
{
	return g_free;
}


/*///////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

static int alloc_free_hook_cleanup(void) CPPUTILS_NOEXCEPT
{
	if (s_nIsInited) {
		struct SCInternalReplaceFunctionData vReplaceData[4];

		// LoadLibrary and friends
		vReplaceData[0].funcname = "LoadLibraryA";
		vReplaceData[0].newFuncAddress = CPPUTILS_STATIC_CAST(const void*, s_loadlibA_st_lib);
		vReplaceData[0].replaceIfAddressIs = CPPUTILS_STATIC_CAST(const void*, &CinternalLoadLibraryA);

		vReplaceData[1].funcname = "LoadLibraryW";
		vReplaceData[1].newFuncAddress = CPPUTILS_STATIC_CAST(const void*, s_loadlibW_st_lib);
		vReplaceData[1].replaceIfAddressIs = CPPUTILS_STATIC_CAST(const void*, &CinternalLoadLibraryW);

		vReplaceData[2].funcname = "LoadLibraryExA";
		vReplaceData[2].newFuncAddress = CPPUTILS_STATIC_CAST(const void*, s_loadlibExA_st_lib);
		vReplaceData[2].replaceIfAddressIs = CPPUTILS_STATIC_CAST(const void*, &CinternalLoadLibraryExA);

		vReplaceData[3].funcname = "LoadLibraryExW";
		vReplaceData[3].newFuncAddress = CPPUTILS_STATIC_CAST(const void*, s_loadlibExW_st_lib);
		vReplaceData[3].replaceIfAddressIs = CPPUTILS_STATIC_CAST(const void*, &CinternalLoadLibraryExW);

		CInternalReplaceFunctionsMac(4, vReplaceData);

		//
		vReplaceData[0].funcname = "malloc";
		vReplaceData[0].newFuncAddress = CPPUTILS_STATIC_CAST(const void*, s_malloc_c_lib);
		vReplaceData[0].replaceIfAddressIs = CPPUTILS_STATIC_CAST(const void*, &AllocFreeHookMalloc);

		vReplaceData[1].funcname = "calloc";
		vReplaceData[1].newFuncAddress = CPPUTILS_STATIC_CAST(const void*, s_calloc_c_lib);
		vReplaceData[1].replaceIfAddressIs = CPPUTILS_STATIC_CAST(const void*, &AllocFreeHookCalloc);

		vReplaceData[2].funcname = "realloc";
		vReplaceData[2].newFuncAddress = CPPUTILS_STATIC_CAST(const void*, s_realloc_c_lib);
		vReplaceData[2].replaceIfAddressIs = CPPUTILS_STATIC_CAST(const void*, &AllocFreeHookRealloc);

		vReplaceData[3].funcname = "free";
		vReplaceData[3].newFuncAddress = CPPUTILS_STATIC_CAST(const void*, s_free_c_lib);
		vReplaceData[3].replaceIfAddressIs = CPPUTILS_STATIC_CAST(const void*, &AllocFreeHookFree);

		CInternalReplaceFunctionsMac(4, vReplaceData);

		s_nIsInited = 0;
	}

	return 0;
}


CPPUTILS_CODE_INITIALIZER(alloc_free_hook_initialize) {
	alloc_free_hook_initialize_inline();
	_onexit(&alloc_free_hook_cleanup);
}


CPPUTILS_END_C



#endif  //  #ifdef _WIN32