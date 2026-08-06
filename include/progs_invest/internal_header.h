//
// file:			internal_header.h
// path:			include/progs_invest/internal_header.h
// created on:		2023 Mar 08
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//

#ifndef PROGS_INVEST_INCLUDE_PROGS_INVEST_INTERNAL_HEADER_H
#define PROGS_INVEST_INCLUDE_PROGS_INVEST_INTERNAL_HEADER_H

#include <cinternal/internal_header.h>

#if (defined(CPPUTILS_DEBUG) || defined(PROGS_INVEST_ALLOC_FREE_HOOK_FORCE)) && defined(PROGS_INVEST_ALLOC_FREE_HOOK)
#ifndef PROGS_INVEST_ALLOC_FREE_HOOK_USED
#define PROGS_INVEST_ALLOC_FREE_HOOK_USED
#endif
#endif


#endif  // #ifndef PROGS_INVEST_INCLUDE_PROGS_INVEST_INTERNAL_HEADER_H
