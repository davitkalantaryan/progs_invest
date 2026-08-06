#
# repo:		progs_invest
# name:		flagsandsys_common.pri
# path:		prj/common/common_qt/flagsandsys_common.pri
# created on:   2026 Aug 06
# created by:   Davit Kalantaryan (davit.kalantaryan@desy.de)
# usage:	Use this qt include file to calculate some platform specific stuff
#


isEmpty(progsInvestFlagsAndSysCommonIncluded){
    progsInvestFlagsAndSysCommonIncluded = 1

    message("!!! $${PWD}/flagsandsys_common.pri")

    progsInvestRepoRoot = $${PWD}/../../..

    isEmpty(artifactRoot) {
        artifactRoot = $$(artifactRoot)
	isEmpty(artifactRoot) {
            artifactRoot = $${progsInvestRepoRoot}
	}
    }

    include("$${progsInvestRepoRoot}/contrib/cinternal/prj/common/common_qt/flagsandsys_common.pri")

    INCLUDEPATH += $${progsInvestRepoRoot}/include

    exists($${progsInvestRepoRoot}/sys/$${CODENAME}/$$CONFIGURATION/lib) {
        LIBS += -L$${progsInvestRepoRoot}/sys/$${CODENAME}/$$CONFIGURATION/lib
    }
    exists($${progsInvestRepoRoot}/sys/$${CODENAME}/$$CONFIGURATION/tlib) {
        LIBS += -L$${progsInvestRepoRoot}/sys/$${CODENAME}/$$CONFIGURATION/tlib
    }
}
