//
// repo:            progs_invest
// file:			main_mem_leak_invest_test.cpp
// path:			src/tests/main_mem_leak_invest_test.cpp
// created on:		2026 Aug 06
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//

#include <cinternal/internal_header.h>
#include <progs_invest/mem_leak_invest.h>
#include <cinternal/disable_compiler_warnings.h>
#include <stdlib.h>
#include <cinternal/undisable_compiler_warnings.h>


static void ProgsInvestMemLeakInvestClbk(const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_curStack,void* a_pUserData,void** a_pDataForCurThread);


int main()
{
    ProgsInvestMemLeakInvestRegisterClbk(&ProgsInvestMemLeakInvestClbk, CPPUTILS_NULL);

    for (int i(0); i < 100000; ++i) {
        void* pMem = malloc(100);
        (void)pMem;
    }

    return 0;
}


static void ProgsInvestMemLeakInvestClbk(const struct SProgramsInvesigatorStack* CPPUTILS_ARG_NN a_curStack, void* a_pUserData, void** a_pDataForCurThread)
{   
    (void)a_pUserData;
    (void)a_pDataForCurThread;
    ProgramsInvestigatorStackPrint(a_curStack);
    exit(1);
}
