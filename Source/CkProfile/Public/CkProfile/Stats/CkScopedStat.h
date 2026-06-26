#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Stats/Stats.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Resolve (and cache, per-thread) the dynamic TStatId for a script-supplied
    // scope name under STATGROUP_CkScript. Caching matters: the same handful of
    // scope names are constructed every frame in hot script loops, and naming a
    // dynamic stat from scratch (CreateStatId) re-registers an FName each call.
    // Returns an empty TStatId in builds without the stats system.
    CKPROFILE_API auto
    Get_ScopedStat_StatId(
        const FString& InName) -> TStatId;

    // Derive "<Class>::<Method>" from the AngelScript call stack — i.e. the
    // script function that is *currently executing*. This is the script analogue
    // of how C++ CK_STAT names a stat from the type at compile time. Returns
    // "Script::Unknown" when no active script context is available (e.g. called
    // from C++, or in a build without AngelScript). Powers FCk_ScopedStat's
    // no-arg constructor.
    CKPROFILE_API auto
    Get_ActiveScriptScopeName() -> FString;
}

// --------------------------------------------------------------------------------------------------------------------

// RAII scope-cycle guard, intended for AngelScript. Mirrors the C++ `CK_STAT`
// idiom (`FScopeCycleCounter` under STATS, a named CPU event otherwise) for
// script code — the difference is that the stat is named at runtime from the
// script string instead of by C++ type at compile time.
//
//   { FCk_ScopedStat _Stat; ...work... }                     // auto-named "<Class>::<Method>"
//   { FCk_ScopedStat _Stat("AI::EvaluateGoals"); ...work... } // explicit name
//
// The no-arg form derives its name from the calling script function (see
// ck::Get_ActiveScriptScopeName) — no string to type. The explicit form is for
// sub-method scopes, or hot loops where skipping the per-call context lookup is
// worth it.
//
// As an AngelScript value type it is constructed as a local and recorded when
// it leaves the enclosing scope. Non-copyable on purpose: a copy would record a
// second (bogus) sample when the duplicate is destroyed.
struct CKPROFILE_API FCk_ScopedStat
{
public:
    // Auto-named from the calling script function's "<Class>::<Method>".
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
