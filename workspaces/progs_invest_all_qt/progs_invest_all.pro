#
# repo:		progs_invest
# name:		progs_invest_all.pro
# path:		workspaces/progs_invest_all_qt/progs_invest_all.pro
# created on:   2026 Aug 06
# created by:   Davit Kalantaryan (davit.kalantaryan@desy.de)
# usage:	Use this qt include file to calculate some platform specific stuff
#

message("!!! $${_PRO_FILE_}")

TEMPLATE = subdirs
#CONFIG += ordered

include ( "$${PWD}/../../prj/common/common_qt/flagsandsys_common.pri" )


#SUBDIRS		+=	"$${progsInvestRepoRoot}/prj/tests/any_quick_test_qt/any_quick_test.pro"
#SUBDIRS		+=	"$${progsInvestRepoRoot}/prj/tests/cpputils_unit_test_mult/cpputils_unit_test.pro"
SUBDIRS	+= "$${cutilsRepoRoot}/workspaces/cutils_all_qt/cutils_all.pro"

OTHER_FILES += $$files($${progsInvestRepoRoot}/docs/*.md,true)
OTHER_FILES += $$files($${progsInvestRepoRoot}/docs/*.txt,true)
OTHER_FILES += $$files($${progsInvestRepoRoot}/scripts/*.sh,true)
OTHER_FILES += $$files($${progsInvestRepoRoot}/scripts/*.bat,true)

OTHER_FILES	+=	\
        "$${progsInvestRepoRoot}/.gitattributes"						\
	"$${progsInvestRepoRoot}/.gitignore"						\
	"$${progsInvestRepoRoot}/.gitmodules"						\
	"$${progsInvestRepoRoot}/LICENSE"							\
	"$${progsInvestRepoRoot}/README.md"
