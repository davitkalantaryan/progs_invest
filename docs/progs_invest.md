# progs_invest

`progs_invest` is a C/C++ debugging/investigation library for finding memory-related problems in native applications.

It provides two main capabilities:

1. Hooks for `malloc`, `calloc`, `realloc`, and `free`.
2. A higher-level memory investigator that tracks live allocations, groups them by allocation stack, and reports stacks that keep accumulating allocations without corresponding deallocations.

The second capability is especially useful for long-running applications whose memory usage grows over time. Instead of only asking *"what was still allocated when the process exited?"*, `progs_invest` helps answer:

> Which allocation path is continuously increasing its number of outstanding allocations?

## Features

- Intercepts `malloc`, `calloc`, `realloc`, and `free`.
- Tracks currently live allocations by pointer.
- Captures the allocation stack for each allocation.
- Groups equal stacks together.
- Counts outstanding allocations per stack.
- Calls a user callback when an allocation stack exceeds a configurable threshold.
- Resolves stack frames to module, function, source file, and line number.
- Exposes current allocation statistics.
- Provides low-level allocation-hook APIs for custom investigation code.
- Supports Linux and Windows implementations.

## Getting the sources

The repository uses Git submodules:

```bash
git clone --recursive https://github.com/davitkalantaryan/progs_invest.git
cd progs_invest
```

For the current development branch:

```bash
git checkout feature/setup_progs_invest
git submodule update --init --recursive
```

The main dependency is `contrib/cutils`, which also contains `cinternal`.

## Quick start

The simplest way to enable the memory investigator is to compile with:

```text
PROGS_INVEST_MEM_LEAK_INVEST_AUTO_INIT_USED
```

Then register a callback and configure the allocation threshold.

```cpp
#include <progs_invest/mem_leak_invest.h>
#include <progs_invest/alloc_free_hook.h>
#include <progs_invest/stack_calcs.h>

#include <cstdio>

static void MemoryInvestCallback(
    const SProgramsInvesigatorStack* stack,
    void* userData,
    void** threadData)
{
    (void)userData;
    (void)threadData;

    std::fprintf(stderr, "Suspicious allocation stack:\n");

    ProgramsInvestigatorStackPrint(
        stack,
        &AllocFreeHookCLibMalloc);
}

int main()
{
    ProgsInvestMemLeakInvestSettingsPtr()->maxAllocs = 1000;

    ProgsInvestMemLeakInvestRegisterClbk(
        &MemoryInvestCallback,
        nullptr);

    // Run the application normally.
    // malloc/calloc/realloc/free are monitored.

    return 0;
}
```

A small working example is available in:

```text
src/tests/main_mem_leak_invest_test.cpp
```

## How leak investigation works

For every monitored allocation the investigator:

1. Calls the original C-runtime allocator.
2. Captures the current call stack.
3. Finds or creates an entry for that allocation stack.
4. Stores the allocated pointer and size.
5. Increments the outstanding-allocation count for that stack.

When the pointer is later passed to `free`, the allocation is removed and the count for its allocation stack is decremented.

Conceptually:

```text
allocation stack A ->    3 live allocations
allocation stack B -> 1250 live allocations   <-- interesting
allocation stack C ->    8 live allocations
```

This makes stack **B** much more useful to investigate than simply knowing that the process currently owns 1261 allocations.

### Threshold

The threshold is controlled by:

```cpp
SProgsInvestMemLeakInvestSettings* settings =
    ProgsInvestMemLeakInvestSettingsPtr();

settings->maxAllocs = 1000;
```

The current default is:

```text
10000
```

The callback is generated when the outstanding allocation count of a stack exceeds `maxAllocs` and reaches a new high-water mark observed by the investigator.

This means `progs_invest` is useful as a **runtime growth detector**, not only as an exit-time leak checker.

## Inspecting the reported stack

The easiest option is:

```cpp
ProgramsInvestigatorStackPrint(
    stack,
    &AllocFreeHookCLibMalloc);
```

Individual frames can also be resolved:

```cpp
const size_t frameCount = ProgramsInvestigatorStackSize(stack);

for (size_t i = 0; i < frameCount; ++i) {
    const SProgramsInvesigatorStackItemResolved* frame =
        ProgramsInvestigatorStackItemResolved(
            stack,
            &AllocFreeHookCLibMalloc,
            i);

    if (!frame)
        continue;

    std::printf(
        "module   : %s\n"
        "function : %s\n"
        "source   : %s\n"
        "line     : %d\n\n",
        frame->moduleName,
        frame->functionName,
        frame->sourceFile,
        frame->lineNumber);

    ProgramsInvestigatorStackItemResolvedClean(frame);
}
```

A resolved frame contains:

```cpp
struct SProgramsInvesigatorStackItemResolved {
    const char* moduleName;
    const char* functionName;
    const char* sourceFile;
    int         lineNumber;
};
```

Always call:

```cpp
ProgramsInvestigatorStackItemResolvedClean(frame);
```

after using a resolved frame.

## Linux stack resolution

On Linux, stacks are captured with `backtrace()`. The corresponding module is found using `dladdr()`, and source information is resolved using `addr2line` with options equivalent to:

```bash
addr2line -e <module> -f -C -p -i <address>
```

For useful source information:

- build the application and relevant libraries with debug symbols;
- keep the binaries unstripped when possible;
- make sure `addr2line` is installed and available in `PATH` (normally part of `binutils`).

Resolving frames is relatively expensive, so a good strategy is to capture allocations continuously but resolve/print stacks only when the callback detects suspicious growth.

## Windows stack resolution

On Windows, stacks are captured with `CaptureStackBackTrace()` and resolved through `DbgHelp` using APIs such as `SymFromAddr()` and `SymGetLineFromAddr64()`.

Keep the matching PDB files available if you want function names, source files, and line numbers.

The allocation-hook implementation patches imported allocation functions in loaded modules and also handles modules subsequently loaded through `LoadLibraryA/W` and `LoadLibraryExA/W`.

## Runtime statistics

Current statistics are available through:

```cpp
const SProgsInvestMemLeakInvestStatus* status =
    ProgsInvestMemLeakInvestStatusPtr();
```

The structure is:

```cpp
struct SProgsInvestMemLeakInvestStatus {
    int     numberOfEvents;
    int     allocsMaxUpToNow;
    int64_t memoryAllocatedInBytes;
    int64_t allocatedItemsCount;
};
```

Meaning:

- `numberOfEvents` - number of threshold events delivered to the callback.
- `allocsMaxUpToNow` - largest outstanding allocation count observed for a single allocation stack.
- `memoryAllocatedInBytes` - total number of bytes currently tracked as allocated.
- `allocatedItemsCount` - total number of currently tracked allocations.

Example:

```cpp
const auto* status = ProgsInvestMemLeakInvestStatusPtr();

std::printf(
    "live allocations=%lld, live bytes=%lld, max per stack=%d, events=%d\n",
    static_cast<long long>(status->allocatedItemsCount),
    static_cast<long long>(status->memoryAllocatedInBytes),
    status->allocsMaxUpToNow,
    status->numberOfEvents);
```

## Temporarily disabling tracking

The following pair disables tracking for allocations performed by the current thread:

```cpp
ProgsInvestMemLeakInvestSkipThisStack();

// Allocations here are ignored by the investigator.
do_some_diagnostic_work();

ProgsInvestMemLeakInvestUnskipThisStack();
```

The mechanism is thread-local and nestable. Every `SkipThisStack()` call should have a matching `UnskipThisStack()` call.

This is useful when diagnostic/investigation code itself performs allocations that should not be included in the results.

## Callback data

The callback type is:

```cpp
typedef void (*TypeProgsInvestMemLeakInvestClbk)(
    const SProgramsInvesigatorStack* currentStack,
    void* userData,
    void** dataForCurrentThread);
```

`userData` is the application pointer passed to:

```cpp
ProgsInvestMemLeakInvestRegisterClbk(callback, userData);
```

`dataForCurrentThread` points to a thread-local user pointer maintained by the investigator. The callback may use it to associate its own state with the current thread.

Ownership of application objects stored through these pointers remains with the application.

## Low-level allocation hooks

The lower-level hook API is declared in:

```cpp
#include <progs_invest/alloc_free_hook.h>
```

Custom handlers can be installed independently:

```cpp
AllocFreeHookSetMallocFnc(myMalloc);
AllocFreeHookSetCallocFnc(myCalloc);
AllocFreeHookSetReallocFnc(myRealloc);
AllocFreeHookSetFreeFnc(myFree);
```

The original C-runtime allocation functions are available through:

```cpp
AllocFreeHookCLibMalloc(size);
AllocFreeHookCLibCalloc(count, size);
AllocFreeHookCLibRealloc(ptr, size);
AllocFreeHookCLibFree(ptr);
```

A custom allocation hook should use these functions instead of recursively calling the hooked `malloc`/`free` functions.

Example:

```cpp
static void* MyMalloc(size_t size)
{
    void* ptr = AllocFreeHookCLibMalloc(size);

    // Record the allocation using hook-safe code.

    return ptr;
}
```

Be careful with logging, C++ containers, strings, streams, and similar code inside low-level hooks because those operations may allocate memory and recursively enter the hook. The higher-level memory investigator contains its own thread-local recursion protection.

## Build integration

The current repository contains reference projects for **qmake** and **Visual Studio**:

```text
prj/tests/mem_leak_invest_test_qt/
prj/tests/mem_leak_invest_test_vs/
```

The repository is currently source-oriented rather than providing an installed CMake/package configuration. For the current branch, these test projects are the best reference for integration.

### qmake

A setup based on the included test project looks like:

```qmake
PROGS_INVEST_ROOT = /path/to/progs_invest
include($${PROGS_INVEST_ROOT}/prj/common/common_qt/flagsandsys_common.pri)

CONFIG -= qt
DEFINES += PROGS_INVEST_MEM_LEAK_INVEST_AUTO_INIT_USED

SOURCES += $$files($${progsInvestRepoRoot}/src/core/*.cpp, true)
SOURCES += $$files($${progsInvestRepoRoot}/src/core/*.c, true)

SOURCES += "$${cinternalRepoRoot}/src/core/cinternal_core_hash.c"
SOURCES += "$${cinternalRepoRoot}/src/core/cinternal_core_logger.c"
```

On Windows, the function-replacement implementation from `cutils` is also needed. See:

```text
prj/tests/mem_leak_invest_test_vs/mem_leak_invest_test.vcxproj
```

for the complete source list used by the current Visual Studio test.

## Compile-time switches

### `PROGS_INVEST_MEM_LEAK_INVEST_AUTO_INIT_USED`

Enables the full memory investigator and requests automatic initialization. It also enables the required stack-calculation and allocation-hook functionality.

This is the easiest switch for normal use.

### `PROGS_INVEST_MEM_LEAK_INVEST_USED`

Enables the memory investigator without requesting automatic startup. Calling an investigator API such as `ProgsInvestMemLeakInvestRegisterClbk()` initializes it.

### `PROGS_INVEST_STACK_CALCS_USED`

Enables stack capture/resolution support.

### `PROGS_INVEST_ALLOC_FREE_HOOK_USED`

Enables the low-level allocation-hook API. It is automatically enabled when the memory investigator is enabled.

## Performance and limitations

`progs_invest` is intended for investigation/debugging builds. Allocation bookkeeping and stack capture add overhead, and source-level stack resolution adds significantly more overhead.

A practical workflow is:

1. Build the investigated application with symbols.
2. Enable `progs_invest`.
3. Choose a meaningful `maxAllocs` threshold.
4. Run the real workload.
5. Print/resolve stacks only when the callback is triggered.
6. Inspect whether the reported growth is expected or caused by missing cleanup.

The library tracks memory that passes through:

```text
malloc / calloc / realloc / free
```

Memory obtained through unrelated mechanisms such as custom pools, direct `mmap`, `VirtualAlloc`, GPU allocators, or application-specific allocators is not automatically tracked unless those mechanisms eventually use the hooked functions.

A large number of outstanding allocations is also not automatically a bug. Caches, queues, pools, and intentionally retained application state may legitimately grow. The reported stack is a starting point for investigation.

### Current `realloc` note

When `realloc` moves a block to a different address, the investigator removes the old tracked allocation and records the new one. If `realloc` returns the same address, the current implementation does not update `memoryAllocatedInBytes` for a changed size. Keep this in mind when interpreting byte statistics for realloc-heavy workloads.

## Repository layout

```text
include/progs_invest/
    alloc_free_hook.h       malloc/calloc/realloc/free hook API
    mem_leak_invest.h       live-allocation investigation API
    stack_calcs.h           stack capture and symbol resolution API

src/core/alloc_free_hook/
    platform-specific allocation interception

src/core/mem_leak_invest/
    allocation tracking and stack aggregation

src/core/stack_calcs/
    Linux/Windows stack capture and source resolution

src/tests/
    example/test code

prj/tests/
    qmake and Visual Studio test projects

contrib/cutils/
    cutils/cinternal dependency
```

## Status

The project is under active development, and APIs/integration details may still change. This README describes the implementation currently present on the `feature/setup_progs_invest` branch.

## License

MIT License. See [LICENSE](LICENSE).