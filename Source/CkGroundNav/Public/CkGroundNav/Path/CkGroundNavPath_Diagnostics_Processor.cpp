#include "CkGroundNavPath_Diagnostics_Processor.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkGroundNav/Debug/CkGroundNav_DebugGates.h"

#include "CkNavigation/NavSurface/CkNavSurface_Utils.h"

#include <Engine/World.h>

CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavPath_Diagnostics);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_GroundNavPath_Diagnostics::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        // One console read for the whole pass instead of one per agent. A world nobody is looking at
        // stops paying for a copy nothing consumes; a fragment stamped before the gate closed keeps
        // the state it was stamped with, because this pass composes and writes and never removes.
        if (NOT groundnav::debug::Get_IsPathDiagnosticsEnabled())
        { return; }

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_GroundNavPath_Diagnostics::
        ForEachEntity(
            TimeType                               InDeltaT,
            HandleType                             InPathEntity,
            const FFragment_GroundNavPath_Current& InCurrent,
            const FFragment_GroundNavPath_Result&  InResult)
        -> void
    {
        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InPathEntity);

        auto& Diagnostics = InPathEntity.AddOrGet<FFragment_GroundNavPath_Diagnostics>();

        // Written before anything else, so every column below is read under a flag that is already
        // true: a reader that sees this false is holding a value no pass has ever touched.
        Diagnostics._HasBeenStamped = true;

        // A world-less agent still answers, with the project's default provider: the neutral facade
        // resolves one for every world including none, and a blank column here would read as a
        // provider that does not exist.
        Diagnostics._Provider = UCk_Utils_NavSurface_UE::Get_Provider(World);

        Diagnostics._ProfileTag = InCurrent.Get_ProfileTag();
        Diagnostics._CorridorLinkIds = InCurrent.Get_LastCorridorLinkIds();
        Diagnostics._CorridorEpoch = InCurrent.Get_LastCorridorEpoch()._Value;

        Diagnostics._PublishedWaypointCount = InResult.Get_Result().Get_Waypoints().Num();

        Diagnostics._RepathRequired = InPathEntity.Has<FTag_GroundNavPath_RepathRequired>();

        // The verdict is only readable off a slot that belongs to no search in flight. A slot that
        // does not keeps whatever verdict was last read out of it, so the column always names the
        // last thing this agent's planner actually answered.
        if (NOT InResult.Get_HasFreshResult())
        { return; }

        Diagnostics._PathStatus = InResult.Get_Result().Get_Status();

        // Dated once per PLAN. The published sequence moves on a publish and on nothing else, so a
        // standing plan re-read for the next hundred ticks keeps the date it was planned at, and a
        // plan that supersedes it in the same slot carries the date forward with it.
        const auto PublishSequence = InResult.Get_PublishSequence();

        if (Diagnostics._LastPlanSequence == PublishSequence)
        { return; }

        Diagnostics._LastPlanSequence = PublishSequence;

        Diagnostics._LastPlanWorldTime = ck::IsValid(World)
            ? FCk_Time{static_cast<double>(World->GetTimeSeconds())}
            : FCk_Time{};
    }
}

// --------------------------------------------------------------------------------------------------------------------
