#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Stats/Stats.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Per-thread cache of dynamic TStatIds under STATGROUP_CkScript; CreateStatId re-registers an
    // FName per call, which hot script loops cannot afford. Empty TStatId without the stats system.
    CKPROFILE_API auto
    Get_ScopedStat_StatId(
        const FString& InName) -> TStatId;

    // "<Class>::<Method>" of the currently executing AngelScript function, or "Script::Unknown"
    // when there is no active script context (called from C++, or built without AngelScript).
    // Costs three FString allocations per call - prefer the cached accessor below on hot paths.
    CKPROFILE_API auto
    Get_ActiveScriptScopeName() -> FString;

    // The TStatId for the currently executing script scope, keyed by the script FUNCTION rather
    // than by its name. Reaching the function is the same work either way, but the key skips the
    // three FString allocations + FName construction that naming costs, so the name is built ONCE
    // PER FUNCTION instead of once per CALL. This is the hot path - a populated store opens well
    // over a thousand script scopes per frame, and that cost lands inside whatever scope encloses
    // it, which silently taxes every script measurement taken with this system.
    CKPROFILE_API auto
    Get_ScopedStat_StatId_ForActiveScope() -> TStatId;

    // "<Class>::<Method>" of the active script function, cached per function pointer. The pointer
    // is the cached string's buffer and is valid until this thread's next cache miss - hand it
    // straight to whatever consumes it.
    CKPROFILE_API auto
    Get_ActiveScriptScopeName_Cached() -> const TCHAR*;

    // Drops the per-function scope caches. MUST run before an AngelScript recompile: script function
    // pointers do not survive a reload and the allocator recycles the addresses, so a surviving
    // entry would attribute one function's time to a different function's name. Wired to
    // FAngelscriptCodeModule::GetPreCompile() wherever AngelScript is present - NOT editor-only,
    // because hot reload also runs in non-editor builds launched with -as-development-mode.
    CKPROFILE_API auto
    Invalidate_ScopedStat_ScopeCache() -> void;
}

// --------------------------------------------------------------------------------------------------------------------

// RAII scope-cycle guard, the AngelScript peer of the C++ `CK_STAT` idiom — see
// CkProfile/CLAUDE.md for the script-side usage. Non-copyable on purpose: a copy would record a
// second, bogus sample when the duplicate is destroyed.
struct CKPROFILE_API FCk_ScopedStat
{
public:
    FCk_ScopedStat();

    explicit FCk_ScopedStat(
        const FString& InName);

    ~FCk_ScopedStat();

    FCk_ScopedStat(const FCk_ScopedStat&) = delete;
    auto operator=(const FCk_ScopedStat&) -> FCk_ScopedStat& = delete;

private:
#if STATS
    FScopeCycleCounter _Cycle;
#endif
};

// --------------------------------------------------------------------------------------------------------------------
