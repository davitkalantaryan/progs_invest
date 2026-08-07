//
// repo:            progs_invest
// file:			progs_invest_stack_calcs_unix_helper.cpp
// path:			src/core/stack_calcs/progs_invest_stack_calcs_unix_helper.cpp
// created on:		2023 Mar 14
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//

#include <progs_invest/internal_header.h>

#ifdef PROGS_INVEST_STACK_CALCS_USED
#ifndef _WIN32

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <progs_invest/stack_calcs.h>
#include <cinternal/logger.h>
#include <cinternal/disable_compiler_warnings.h>
#include <dlfcn.h>
#include <elf.h>
#include <execinfo.h>
#include <link.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits.h>
#include <string>
#include <cinternal/undisable_compiler_warnings.h>


static void printBacktraceSourceLinesStatic(void* const* frames, int count);
static bool ParseSourceLocationBasicStatic(void* a_frame, struct SProgramsInvesigatorStackItemResolved* CPPUTILS_ARG_NN a_pItem, TypeAllocFreeHookMalloc a_malloc);

CPPUTILS_BEGIN_C


CPPUTILS_DLL_PRIVATE void PrintBacktraceSourceLines(void* const* frames, int count, TypeAllocFreeHookMalloc a_malloc)
{
    (void)a_malloc;
    printBacktraceSourceLinesStatic(frames,count);
}


CPPUTILS_DLL_PRIVATE bool ParseSourceLocationBasic(void* a_frame, struct SProgramsInvesigatorStackItemResolved* CPPUTILS_ARG_NN a_pItem, TypeAllocFreeHookMalloc a_malloc)
{
    return ParseSourceLocationBasicStatic(a_frame,a_pItem,a_malloc);
}


CPPUTILS_END_C


// Resolve relative executable/library names such as "./my_program".
static std::string absolutePath(const char* path)
{
    if (!path || !*path)
        path = "/proc/self/exe";

    char resolved[PATH_MAX];

    if (::realpath(path, resolved))
        return resolved;

    return path;
}

// addr2line expects:
//
//   ET_DYN executable/library: address relative to load base
//   ET_EXEC executable:        absolute virtual address
static uintptr_t addressForAddr2Line(uintptr_t pc, const Dl_info& info)
{
    if (!info.dli_fbase)
        return pc;

    const auto* elfHeader =
        reinterpret_cast<const ElfW(Ehdr)*>(info.dli_fbase);

    const bool validElf =
        std::memcmp(elfHeader->e_ident, ELFMAG, SELFMAG) == 0;

    if (validElf && elfHeader->e_type == ET_DYN) {
        return pc - reinterpret_cast<uintptr_t>(info.dli_fbase);
    }

    return pc;
}

static std::string runAddr2Line(const std::string& objectFile,uintptr_t address)
{
    char addressText[2 + sizeof(uintptr_t) * 2 + 1];

    std::snprintf(
        addressText,
        sizeof(addressText),
        "0x%lx",
        static_cast<unsigned long>(address));

    int outputPipe[2];

    if (::pipe(outputPipe) != 0)
        return std::string("pipe failed: ") + std::strerror(errno);

    const pid_t pid = ::fork();

    if (pid == -1) {
        ::close(outputPipe[0]);
        ::close(outputPipe[1]);
        return std::string("fork failed: ") + std::strerror(errno);
    }

    if (pid == 0) {
        // Child process
        ::close(outputPipe[0]);

        if (::dup2(outputPipe[1], STDOUT_FILENO) == -1)
            _exit(126);

        ::close(outputPipe[1]);

        ::execlp(
            "addr2line",
            "addr2line",
            "-e",
            objectFile.c_str(),
            "-f",       // function name
            "-C",       // demangle C++
            "-p",       // one-line readable output
            "-i",       // show inlined functions
            addressText,
            static_cast<char*>(nullptr));

        _exit(127);
    }

    // Parent process
    ::close(outputPipe[1]);

    std::string output;
    char buffer[4096];

    for (;;) {
        const ssize_t result = ::read(outputPipe[0], buffer, sizeof(buffer));

        if (result > 0) {
            output.append(buffer, static_cast<size_t>(result));
            continue;
        }

        if (result == -1 && errno == EINTR)
            continue;

        break;
    }

    ::close(outputPipe[0]);

    int status = 0;

    while (::waitpid(pid, &status, 0) == -1 && errno == EINTR) {
    }

    while (!output.empty() && (output.back() == '\n' || output.back() == '\r')) {
        output.pop_back();
    }

    if (output.empty()) {
        if (WIFEXITED(status) && WEXITSTATUS(status) == 127)
            return "addr2line executable not found";

        return "?? at ??:0";
    }

    return output;
}


static void printBacktraceSourceLinesStatic(void* const* frames, int count)
{
    // Optional fallback representation.
    char** fallbackSymbols = ::backtrace_symbols(frames, count);

    for (int i = 0; i < count; ++i) {
        uintptr_t pc = reinterpret_cast<uintptr_t>(frames[i]);

        /*
         * backtrace() normally returns return addresses. Subtracting one byte
         * makes addr2line resolve the calling instruction rather than the
         * instruction immediately after it.
         */
        if (pc != 0)
            --pc;

        Dl_info info{};

        if (::dladdr(reinterpret_cast<void*>(pc), &info) == 0) {
            CinternalLoggerMakeLogOnlyText(0,
                "#%-2d %s\n",
                i,
                fallbackSymbols ? fallbackSymbols[i] : "unknown frame");
            continue;
        }

        const std::string objectFile = absolutePath(info.dli_fname);
        const uintptr_t objectAddress = addressForAddr2Line(pc, info);
        const std::string source = runAddr2Line(objectFile, objectAddress);

        CinternalLoggerMakeLogOnlyText(0,
            "#%-2d %p  %s(+0x%lx)\n"
            "     %s\n",
            i,
            frames[i],
            objectFile.c_str(),
            static_cast<unsigned long>(objectAddress),
            source.c_str());
    }

    CinternalLoggerMakeLog(0, "", "", 0, "", CinternalLogTypeFinalize, CinternalLogCategoryNone, "\n");

    std::free(fallbackSymbols);
}


struct SourceLocation
{
    std::string functionName;
    std::string sourceFile;
    int lineNumber = 0;
};


static inline bool ParseSourceLocationBasicInline(const char* text, SourceLocation& result)
{
    if (!text) {
        return false;
    }

    const std::string str(text);

    // Separate function name from source location.
    const std::size_t atPos = str.rfind(" at ");
    if (atPos == std::string::npos) {
        return false;
    }

    // Last ':' separates filename from line number.
    const std::size_t colonPos = str.rfind(':');
    if (colonPos == std::string::npos || colonPos <= atPos + 4) {
        return false;
    }

    result.functionName = str.substr(0, atPos);
    result.sourceFile =
        str.substr(atPos + 4, colonPos - (atPos + 4));

    const char* lineStr = str.c_str() + colonPos + 1;
    char* end = nullptr;

    const long line = std::strtol(lineStr, &end, 10);

    if (end == lineStr || *end != '\0' || line <= 0) {
        return false;
    }

    result.lineNumber = static_cast<int>(line);

    return true;
}


static inline char* ProgramsInvestigatorStackStrdup(const char* CPPUTILS_ARG_NN a_src, size_t a_strLen, TypeAllocFreeHookMalloc a_malloc) CPPUTILS_NOEXCEPT  {
    const size_t strLenPlus1 = (size_t)(a_strLen + 1);
    char* const pRet = (char*)((*a_malloc)(sizeof(char) * strLenPlus1));
    if (pRet) {
        memcpy(pRet, a_src, strLenPlus1);
    }
    return pRet;
}


static inline char* ProgramsInvestigatorStackStrdupCpp(const ::std::string& a_src, TypeAllocFreeHookMalloc a_malloc) CPPUTILS_NOEXCEPT  {
    return ProgramsInvestigatorStackStrdup(a_src.c_str(),a_src.size(),a_malloc);
}


static bool ParseSourceLocationBasicStatic(void* a_frame, struct SProgramsInvesigatorStackItemResolved* CPPUTILS_ARG_NN a_pItem, TypeAllocFreeHookMalloc a_malloc)
{
    uintptr_t pc = reinterpret_cast<uintptr_t>(a_frame);

    /*
         * backtrace() normally returns return addresses. Subtracting one byte
         * makes addr2line resolve the calling instruction rather than the
         * instruction immediately after it.
         */
    if (pc != 0)
        --pc;

    Dl_info info{};

    if (::dladdr(reinterpret_cast<void*>(pc), &info) == 0) {
        return false;
    }

    const std::string objectFile = absolutePath(info.dli_fname);
    const uintptr_t objectAddress = addressForAddr2Line(pc, info);
    const std::string source = runAddr2Line(objectFile, objectAddress);

    SourceLocation result;
    if(!ParseSourceLocationBasicInline(source.c_str(),result)){
        return false;
    }

    a_pItem->moduleName = ProgramsInvestigatorStackStrdupCpp(objectFile,a_malloc);
    a_pItem->functionName = ProgramsInvestigatorStackStrdupCpp(result.functionName,a_malloc);
    a_pItem->sourceFile = ProgramsInvestigatorStackStrdupCpp(result.sourceFile,a_malloc);
    a_pItem->lineNumber = result.lineNumber;
    a_pItem->reserved01 = 0;

    return true;
}


#endif  //  #ifndef _WIN32
#endif  //  #ifdef PROGS_INVEST_STACK_CALCS_USED
