# Debug Callstack System

This document describes the Debug Callstack feature for ECS entities, which enables tracing and debugging by recording callstack entries on entities.

## Overview

The Debug Callstack system allows you to record timestamped entries on entities to help trace what code touched them and when. Each entry can capture:
- Frame number
- C++ function name and line number (compile-time `__FUNCTION__` and `__LINE__`)
- Optional message
- **C++ callstack addresses** (fast ~1-5μs address-only capture, lazy symbol resolution)
- Blueprint callstack (symbol-resolved, expensive)
- Angelscript callstack (symbol-resolved, expensive)

All callstack recording compiles out in shipping builds.

### Performance Characteristics

- **C++ Address Capture**: ~1-5μs per callstack (fast, no symbol resolution)
- **C++ Symbol Resolution**: ~50-200μs per callstack (expensive, done lazily when viewing)
- **Blueprint/Angelscript Capture**: Expensive (symbol-resolved immediately)

The C++ callstack uses a two-phase approach:
1. **Capture Phase** (fast): Store raw program counter addresses
2. **Resolution Phase** (lazy): Resolve addresses to symbols only when viewing in debugger/editor

## Files

| File | Purpose |
|------|---------|
| `CkDebugCallstack_Fragment.h` | Template fragment `TFragment_Debug_Callstack<T>` and trait |
| `CkDebugCallstack_Utils.h` | Template utils class `TCk_Utils_Debug_Callstack<T>` |
| `CkDebugCallstack_Utils.inl.h` | Implementation of the template utils |
| `CkDebugCallstack_Macros.h` | All macros for fragment definition, C++ recording, and Angelscript bindings |

## Setup for a New Feature

To add callstack tracking to a new feature (e.g., Timer):

### 1. Define the Callstack Fragment (Header)

In your fragment header (e.g., `CkTimer_Fragment.h`), inside the `ck` namespace:

```cpp
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

namespace ck
{
    // ... your fragment definitions ...

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Timer_Current);
}
```

This creates:
- `FFragment_Timer_Current_Callstack` struct
- Trait specialization mapping `FFragment_Timer_Current` → `FFragment_Timer_Current_Callstack`

### 2. Define Angelscript Bindings (CPP)

In a `.cpp` file in your module (e.g., `CkTimer_Fragment.cpp`):

```cpp
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

CK_ECS_DEFINE_CALLSTACK_ANGELSCRIPT_UTILS(CKTIMER_API, timer, ck::FFragment_Timer_Current);
```

Parameters:
- `_API_` - Module API macro (e.g., `CKTIMER_API`)
- `_feature_` - Feature name in **lowercase** (e.g., `timer`) - becomes part of namespace name
- `_FragmentType_` - Fully qualified fragment type (e.g., `ck::FFragment_Timer_Current`)

## Usage

### C++ Usage

```cpp
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

// Record entry without message (captures __FUNCTION__ and __LINE__)
CK_CALLSTACK_RECORD(ck::FFragment_Timer_Current, TimerHandle);

// Record entry with formatted message
CK_CALLSTACK_RECORD_MSG(ck::FFragment_Timer_Current, TimerHandle,
    TEXT("Timer started with duration [{}]"), Duration);

// Clear all entries
CK_CALLSTACK_CLEAR(ck::FFragment_Timer_Current, TimerHandle);
```

### Angelscript Usage

```angelscript
// Record entry with optional message (auto-captures AS callstack)
utils_timer_debug_callstack::Record(TimerHandle, "Timer started");
utils_timer_debug_callstack::Record(TimerHandle);  // No message

// Clear all entries
utils_timer_debug_callstack::Clear(TimerHandle);
```

The namespace follows the pattern: `utils_{feature}_debug_callstack`

### Blueprint Usage

Blueprint callstacks are automatically captured when the CVar is enabled. No special macros needed - just call from Blueprint into C++ code that uses `CK_CALLSTACK_RECORD`.

## Reading Callstack Entries

To read the recorded entries:

```cpp
auto& CallstackFragment = Entity.Get<FFragment_Timer_Current_Callstack>();
for (const auto& Entry : CallstackFragment.Get_Entries())
{
    // Compile-time capture (zero cost)
    // Entry.FrameNumber (uint64)
    // Entry.FunctionName (const char*) - may be nullptr for AS entries
    // Entry.LineNumber (int32) - may be 0 for AS entries
    // Entry.Message (FString)

    // C++ callstack - pointers to global symbol cache
    // Background thread resolves symbols asynchronously
    // Entry.CppCallstackAddresses (TArray<const FString*>)

    // Blueprint/Angelscript callstacks - symbol-resolved (expensive to capture)
    // Entry.BlueprintCallstack (TArray<FString>)
    // Entry.AngelscriptCallstack (TArray<FString>)
}
```

### Viewing C++ Callstacks in VS Debugger

**Just expand the array in watch window - no function calls needed!**

When stopped at a breakpoint, simply navigate to the callstack:

```cpp
// In VS Watch window, expand the tree:
Entity
  └─ (Find your callstack fragment - navigation varies by entity type)
      └─ _Entries
          └─ [0]
              └─ CppCallstackAddresses  ← Just expand this!
                  ├─ [0] -> "UCk_Utils_Timer_UE::Request_Start [CkTimer_Utils.cpp:45]"
                  ├─ [1] -> "FProcessor_Timer_HandleRequests::ForEachEntity [...]"
                  └─ ...
```

**How It Works**:
- **Capture**: Addresses converted to pointers into global cache (~1-5μs)
- **Background Thread**: Continuously resolves unresolved symbols asynchronously
- **Viewing**: Pointers show current symbol state (may start as "<resolving...>", updates automatically)
- **Cache**: Global thread-safe cache with `FRWLock` and `std::atomic`
- **Pointer Stability**: `TUniquePtr` guarantees pointers never move

**No manual resolution needed** - background thread handles it automatically!

## Settings

Callstack capture is controlled by user settings in `UCk_Ecs_UserSettings_UE`:

### Enable/Disable Capture
- `_CaptureCallstack_Cpp` - Enable C++ address-only callstack capture (default: false)
- `_CaptureCallstack_Blueprint` - Enable Blueprint callstack capture (default: false)
- `_CaptureCallstack_Angelscript` - Enable Angelscript callstack capture (default: false)

### Max Frames to Capture
- `_MaxCallstackFrames_Cpp` - Max frames for C++ callstacks (default: 8)
  - Lower values = faster capture (each frame is just an address)
  - Range: 1-128
- `_MaxCallstackFrames_Blueprint_Override` - Override for Blueprint max frames (default: 0 = use Core settings)
  - When 0, uses `UCk_Utils_Core_UserSettings_UE::Get_MaxNumberOfBlueprintStackFrames()`
  - Only applies when `_CaptureCallstack_Blueprint` is enabled
- `_MaxCallstackFrames_Angelscript_Override` - Override for Angelscript max frames (default: 0 = use Core settings)
  - When 0, uses `UCk_Utils_Core_UserSettings_UE::Get_MaxNumberOfAngelscriptStackFrames()`
  - Only applies when `_CaptureCallstack_Angelscript` is enabled

### Max Entries Per Entity
- `_MaxCallstackEntries` - Maximum number of callstack entries to keep per entity (default: 8)
  - When limit is reached, oldest entries are removed (FIFO)
  - Prevents unbounded memory growth
  - Range: 1-1024

### Accessing Settings via Utils

```cpp
// Enable/disable capture
UCk_Utils_Ecs_Settings_UE::Get_CaptureCallstack_Cpp();
UCk_Utils_Ecs_Settings_UE::Set_CaptureCallstack_Cpp(true);

// Get max frames
const auto MaxFrames = UCk_Utils_Ecs_Settings_UE::Get_MaxCallstackFrames_Cpp();
UCk_Utils_Ecs_Settings_UE::Set_MaxCallstackFrames_Cpp(16);

// Get/set max entries per entity
const auto MaxEntries = UCk_Utils_Ecs_Settings_UE::Get_MaxCallstackEntries();
UCk_Utils_Ecs_Settings_UE::Set_MaxCallstackEntries(16);
```

## Existing Implementations

| Feature | Fragment | Namespace |
|---------|----------|-----------|
| EntityScript | `ck::FFragment_EntityScript_Current` | `utils_entity_script_debug_callstack` |

## Implementation Details

### C++ Callstack Capture Pipeline

1. **Capture** (in `TCk_Utils_Debug_Callstack::Add()`):
   - Uses `UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_AddressesOnly()`
   - Calls `FPlatformStackWalk::CaptureStackBackTrace()` to get raw addresses
   - Skips 2 frames (the utility function + macro)
   - Converts addresses to pointers into global cache
   - Stores pointers in `Entry.CppCallstackAddresses`
   - Fast: ~1-5μs for 8 frames

2. **Background Resolution** (in `FCallstackResolverThread`):
   - Runs continuously on background thread (sleeps 100ms between passes)
   - Finds unresolved addresses in global cache
   - Calls `FPlatformStackWalk::ProgramCounterToHumanReadableString()` for each
   - Formats as: `FunctionName [Filename.cpp:LineNumber]`
   - Updates cache entries atomically
   - VS debugger shows updated symbols automatically

### Blueprint/Angelscript Callstack Capture

Blueprint and Angelscript callstacks are symbol-resolved immediately during capture because:
- These languages don't provide address-only capture APIs
- Symbol information must be retrieved from the active script context
- Lazy resolution is not possible (context is lost after capture)

## Notes

- All macros compile to no-ops in shipping builds (`UE_BUILD_SHIPPING`)
- Angelscript bindings only exist when `WITH_ANGELSCRIPT_CK` is defined
- The Angelscript `Record` function passes `nullptr`/`0` for function/line since the AS callstack is captured automatically
- The `_feature_` parameter must be lowercase to produce correct namespace names
- C++ address capture is ~40x faster than symbol-resolved capture (1-5μs vs 50-200μs)
