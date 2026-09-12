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
    CKPROFILE_API auto
    Get_ActiveScriptScopeName() -> FString;

#if STATS
    // Resolves the active AngelScript function directly to its cached stat id. The cache stores
    // only numeric function ids and TStatIds; it never retains AngelScript function/type objects.
    CKPROFILE_API auto
    Get_ActiveScriptScopeStatId() -> TStatId;
#endif

    // The named-event counterpart of the cache above, for the non-STATS FCk_ScopedStat ctor:
    // BeginNamedEvent wants a NAME, not a stat id.
    //
    // Returns a pointer into this thread's cache, valid until the next epoch change. Hand it
    // straight to BeginNamedEvent; do not store it.
    CKPROFILE_API auto
    Get_ActiveScriptScopeName_Cached() -> const TCHAR*;

    // Called from the AngelScript pre-compile boundary. It only advances an epoch: each thread
    // clears its own function-id cache lazily on the next lookup.
    CKPROFILE_API auto
    Invalidate_ActiveScriptScopeStatCache() -> void;

#if WITH_DEV_AUTOMATION_TESTS
    // Narrow test seam for proving the same epoch transition without broadcasting the engine-wide
    // PreCompile delegate, which has unrelated production subscribers.
    CKPROFILE_API auto
    Get_ActiveScriptScopeStatCacheEpoch_ForTests() -> uint64;

    CKPROFILE_API auto
    Get_IsScopedStatStatsEnabled_ForTests() -> bool;

    CKPROFILE_API auto
    Get_ActiveScriptScopeStatId_ForTests() -> uint64;

    CKPROFILE_API auto
    Get_LegacyActiveScriptScopeStatId_ForTests() -> uint64;

    CKPROFILE_API auto
    Get_ActiveScriptScopeStatName_ForTests() -> FString;

    CKPROFILE_API auto
    Get_ActiveScriptScopeNameCached_ForTests() -> FString;

    CKPROFILE_API auto
    Get_ActiveScriptScopeNameCacheHitCount_ForTests() -> uint64;

    CKPROFILE_API auto
    Get_ActiveScriptScopeNameCacheMissCount_ForTests() -> uint64;

    CKPROFILE_API auto
    Reset_ActiveScriptScopeNameCacheCounters_ForTests() -> void;

    // Dynamic stats retain an encoded registry name separately from the user-facing description.
    CKPROFILE_API auto
    Get_LegacyActiveScriptScopeStatName_ForTests() -> FString;

    CKPROFILE_API auto
    Get_ActiveScriptScopeStatDescription_ForTests() -> FString;

    CKPROFILE_API auto
    Get_ActiveScriptScopeStatCacheHitCount_ForTests() -> uint64;

    CKPROFILE_API auto
    Get_ActiveScriptScopeStatCacheMissCount_ForTests() -> uint64;

    CKPROFILE_API auto
    Reset_ActiveScriptScopeStatCacheCounters_ForTests() -> void;

    CKPROFILE_API auto
    Run_ActiveScriptScopeStatBenchmark_ForTests() -> FString;
#endif
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
