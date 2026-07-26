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
