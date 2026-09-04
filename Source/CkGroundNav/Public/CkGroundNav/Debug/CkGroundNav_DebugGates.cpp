#include "CkGroundNav_DebugGates.h"

#include "CkGroundNav/CkGroundNav_Log.h"

#include <HAL/IConsoleManager.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_groundnav_debuggates
{
    static TAutoConsoleVariable<int32> CVar_MarkupLiveGate(
        TEXT("ck.GroundNav.Debug.MarkupLiveGate"), 1,
        TEXT("1 answers Get_IsMarkupLive from the published field, which is the shipping behaviour. ")
        TEXT("0 forces GroundNav's answer to TRUE, so a fixture that settles on liveness waits for ")
        TEXT("nothing - which is the run a paint-then-repath race pin must FAIL under to be worth ")
        TEXT("anything."));

    static TAutoConsoleVariable<int32> CVar_RepathOnRebuild(
        TEXT("ck.GroundNav.Debug.RepathOnRebuild"), 1,
        TEXT("1 flags an agent for a repath when a published surface rebuild meets its cached corridor, ")
        TEXT("which is the shipping behaviour. 0 flags nobody, so a fixture that settles on a rebuilt ")
        TEXT("route waits forever - which is the run a rebuild-then-repath pin must FAIL under to be ")
        TEXT("worth anything."));

    // Shipping ships with the pass off because nothing in a shipped build reads what it writes. It is a
    // DEFAULT and not a compile-out: the console variable exists in every configuration, so a shipped
    // build that has to answer "what is this agent's planner doing" can still be told to answer it.
#if UE_BUILD_SHIPPING
    constexpr auto kPathDiagnosticsDefault = 0;
#else
    constexpr auto kPathDiagnosticsDefault = 1;
#endif

    static TAutoConsoleVariable<int32> CVar_PathDiagnostics(
        TEXT("ck.GroundNav.PathDiagnostics"), kPathDiagnosticsDefault,
        TEXT("1 stamps FFragment_GroundNavPath_Diagnostics onto every agent holding the path feature ")
        TEXT("once a tick, which is what the per-agent debug views read. 0 runs the pass over nobody; ")
        TEXT("fragments already stamped stay as they were, so a viewer reading one is told the last ")
        TEXT("state the pass saw rather than a blank."));

    static TAutoConsoleVariable<int32> CVar_DrawMarkup(
        TEXT("ck.GroundNav.Debug.DrawMarkup"), 1,
        TEXT("Outline the area markup the world's ground-nav volumes hold in the plate view and in ")
        TEXT("ck.GroundNav.PathAt / FloodAt: impassable in red, cost in amber with its multiplier, ")
        TEXT("and a record the volume still holds but has disabled in dashed grey."));
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav::debug
{
    auto
        Get_IsMarkupLiveGateBypassed()
        -> bool
    {
        const auto Bypassed = ck_groundnav_debuggates::CVar_MarkupLiveGate.GetValueOnGameThread() == 0;

        // A run under the bypass must say so in its log, or a green it produced could be mistaken for
        // one the live wait earned.
        static auto Announced = false;
        if (Bypassed && NOT Announced)
        {
            Announced = true;
            ck::groundnav::Display(TEXT("ck.GroundNav.Debug.MarkupLiveGate is 0: Get_IsMarkupLive answers true without asking the field"));
        }

        return Bypassed;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_IsRepathOnRebuildBypassed()
        -> bool
    {
        const auto Bypassed = ck_groundnav_debuggates::CVar_RepathOnRebuild.GetValueOnGameThread() == 0;

        // A run under the bypass must say so in its log, or a green it produced could be mistaken for
        // one a rebuilt route earned.
        static auto Announced = false;
        if (Bypassed && NOT Announced)
        {
            Announced = true;
            ck::groundnav::Display(TEXT("ck.GroundNav.Debug.RepathOnRebuild is 0: no cached corridor is flagged when the surface rebuilds"));
        }

        return Bypassed;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_IsPathDiagnosticsEnabled()
        -> bool
    {
        return ck_groundnav_debuggates::CVar_PathDiagnostics.GetValueOnGameThread() != 0;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_IsMarkupDrawEnabled()
        -> bool
    {
        return ck_groundnav_debuggates::CVar_DrawMarkup.GetValueOnGameThread() != 0;
    }
}

// --------------------------------------------------------------------------------------------------------------------
