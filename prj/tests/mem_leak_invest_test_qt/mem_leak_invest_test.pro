#
# file:			focust_p01_llbackend02.pro
# path:			prj/core/focust_p01_llbackend02_qt/focust_p01_llbackend02.pro
# created on:		2023 Apr 28
# creatd by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
# purpose:		Qt Project file to handle focust p01 LL backend
#

message("!!! $${_PRO_FILE_}")
include ($${PWD}/../../common/common_qt/flagsandsys_common.pri)

CONFIG -= qt

DEFINES += PROGS_INVEST_MEM_LEAK_INVEST_AUTO_INIT_USED

SOURCES += "$${progsInvestRepoRoot}/src/tests/main_mem_leak_invest_test.cpp"
SOURCES += $$files($${progsInvestRepoRoot}/src/core/*.cpp,true)
SOURCES += $$files($${progsInvestRepoRoot}/src/core/*.c,true)
SOURCES += "$${cinternalRepoRoot}/src/core/cinternal_core_hash.c"
SOURCES += "$${cinternalRepoRoot}/src/core/cinternal_core_logger.c"

HEADERS += $$files($${progsInvestRepoRoot}/include/*.h,true)
