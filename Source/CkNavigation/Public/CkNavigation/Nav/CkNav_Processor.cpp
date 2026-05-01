#include "CkNav_Processor.h"

#include "CkNavigation/CkNavigation_Log.h"
#include "CkNavigation/Nav/CkNav_Algorithm.h"
#include "CkNavigation/Settings/CkNav_ProjectSettings.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include <NavigationSystem.h>
#include <NavMesh/RecastNavMesh.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Nav_HandleRequests::
        DoTick(FCk_Time InDeltaT)
        -> void
    {
        // Gate 1 drains every queued request; per-tick budget cap is wired in Gate 6
        // alongside the stress-test scenario. Default reads kept here for future use.
        _BudgetRemainingThisTick = UCk_Utils_Nav_Settings_UE::Get_MaxPathQueriesPerFrame();
        TProcessor::DoTick(InDeltaT);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Nav_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Nav_Requests& InRequests,
            FFragment_Nav_PathResult& InResult) const
        -> void
    {
        // Server-authoritative: drop client-side requests on the floor.
        if (NOT UCk_Utils_Net_UE::Get_HasAuthority(InHandle))
        {
            InResult._Status = ECk_Nav_PathStatus::Failed;
            InResult._Diagnostics._LastFailReason = ECk_Nav_PathFailReason::NotAuthority;
            ck::nav::Warning(TEXT("FindPath request on [{}] dropped: client lacks authority"), InHandle);
            // CopyAndRemove handles fragment removal below; for the early-out branches we
            // still need to clear the queue so it doesn't accumulate stale work.
            InHandle.Try_Remove<FFragment_Nav_Requests>();
            return;
        }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (NOT IsValid(World))
        {
            InResult._Status = ECk_Nav_PathStatus::Failed;
            InResult._Diagnostics._LastFailReason = ECk_Nav_PathFailReason::NoNavSystem;
            InHandle.Try_Remove<FFragment_Nav_Requests>();
            return;
        }

        auto* NavSys = UNavigationSystemV1::GetCurrent(World);
        if (NavSys == nullptr)
        {
            InResult._Status = ECk_Nav_PathStatus::Failed;
            InResult._Diagnostics._LastFailReason = ECk_Nav_PathFailReason::NoNavSystem;
            InHandle.Try_Remove<FFragment_Nav_Requests>();
            return;
        }

        auto* NavData = Cast<ARecastNavMesh>(NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate));
        if (NavData == nullptr)
        {
            InResult._Status = ECk_Nav_PathStatus::Failed;
            InResult._Diagnostics._LastFailReason = ECk_Nav_PathFailReason::NoNavData;
            InHandle.Try_Remove<FFragment_Nav_Requests>();
            return;
        }

        auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle);
        if (NOT ck::IsValid(TransformHandle))
        {
            InResult._Status = ECk_Nav_PathStatus::Failed;
            InResult._Diagnostics._LastFailReason = ECk_Nav_PathFailReason::StartProjectFailed;
            ck::nav::Warning(TEXT("FindPath request on [{}] dropped: entity has no Transform feature"), InHandle);
            InHandle.Try_Remove<FFragment_Nav_Requests>();
            return;
        }
        const auto StartLocation = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TransformHandle);

        const auto ProjectionExtent = UCk_Utils_Nav_Settings_UE::Get_NavQuerySearchHalfExtent();

        // Drain via the standard CopyAndRemove + ForEachRequest pattern (matches CkInventory,
        // CkCue, CkAttribute). CopyAndRemove takes a snapshot of the fragment AND removes it
        // from the entity — so the next AddOrGet<Requests>() in Utils::Request_FindPath
        // re-adds it (re-firing the dirty event so the processor sees the next batch).
        // Without this, MarkedDirtyBy = FFragment_Nav_Requests would only fire on the first
        // request ever; subsequent requests would be silently queued and never drained.
        InHandle.CopyAndRemove(InRequests, [&](const FFragment_Nav_Requests& InSnapshot)
        {
            ck::algo::ForEachRequest(InSnapshot._Requests, ck::Visitor(
                [&](const auto& InFindPath) -> void
                {
                    const auto bSucceeded = FCk_Nav_Algorithm::FindPathSync(
                        *NavSys,
                        *NavData,
                        StartLocation,
                        InFindPath.Get_TargetLocation(),
                        InFindPath.Get_AllowPartialPath(),
                        ProjectionExtent,
                        /*InAgentRadiusForFirstSkip*/ 0.0f,
                        InResult);

                    ck::nav::Verbose(TEXT("FindPathSync on [{}] -> target [{}]: {} (waypoints={} reason={})"),
                        InHandle,
                        InFindPath.Get_TargetLocation(),
                        InResult._Status,
                        InResult._Waypoints.Num(),
                        InResult._Diagnostics._LastFailReason);

                    if (bSucceeded)
                    {
                        UUtils_Signal_Nav_OnPathReady::Broadcast(
                            InHandle, ck::MakePayload(InHandle, InResult));
                    }
                    else
                    {
                        UUtils_Signal_Nav_OnPathFailed::Broadcast(
                            InHandle, ck::MakePayload(InHandle));
                    }
                }), ck::policy::DontResetContainer{});
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Nav_HandleRequests);

// --------------------------------------------------------------------------------------------------------------------
