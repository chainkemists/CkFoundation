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
        Get_IsMarkupDrawEnabled()
        -> bool
    {
        return ck_groundnav_debuggates::CVar_DrawMarkup.GetValueOnGameThread() != 0;
    }
}

// --------------------------------------------------------------------------------------------------------------------
