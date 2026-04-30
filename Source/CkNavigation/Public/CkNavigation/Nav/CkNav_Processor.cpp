#include "CkNav_Processor.h"

#include "CkNavigation/CkNavigation_Log.h"
#include "CkNavigation/Nav/CkNav_Algorithm.h"
#include "CkNavigation/Settings/CkNav_ProjectSettings.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
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
            InRequests._Requests.Reset();
            return;
        }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (NOT IsValid(World))
        {
            InResult._Status = ECk_Nav_PathStatus::Failed;
            InResult._Diagnostics._LastFailReason = ECk_Nav_PathFailReason::NoNavSystem;
            InRequests._Requests.Reset();
            return;
        }

        auto* NavSys = UNavigationSystemV1::GetCurrent(World);
        if (NavSys == nullptr)
        {
            InResult._Status = ECk_Nav_PathStatus::Failed;
            InResult._Diagnostics._LastFailReason = ECk_Nav_PathFailReason::NoNavSystem;
            InRequests._Requests.Reset();
            return;
        }

        auto* NavData = Cast<ARecastNavMesh>(NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate));
        if (NavData == nullptr)
        {
            InResult._Status = ECk_Nav_PathStatus::Failed;
            InResult._Diagnostics._LastFailReason = ECk_Nav_PathFailReason::NoNavData;
            InRequests._Requests.Reset();
            return;
        }

        // Read start location from the entity's Transform feature. CkNavigation does not
        // require a typesafe handle of its own; any entity with CkEcsExt's Transform can
        // request a path.
        auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle);
        if (NOT ck::IsValid(TransformHandle))
        {
            InResult._Status = ECk_Nav_PathStatus::Failed;
            InResult._Diagnostics._LastFailReason = ECk_Nav_PathFailReason::StartProjectFailed;
            ck::nav::Warning(TEXT("FindPath request on [{}] dropped: entity has no Transform feature"), InHandle);
            InRequests._Requests.Reset();
            return;
        }
        const auto StartLocation = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TransformHandle);

        const auto ProjectionExtent = UCk_Utils_Nav_Settings_UE::Get_NavQuerySearchHalfExtent();

        // Drain requests up to the per-tick budget. Un-drained requests stay in the
        // fragment for next-tick retry; the dirty marker is the request fragment itself
        // so the entity stays in the view as long as _Requests is non-empty.
        auto Remaining = TArray<FFragment_Nav_Requests::RequestType>{};

        for (const auto& Variant : InRequests._Requests)
        {
            if (_BudgetRemainingThisTick <= 0)
            {
                Remaining.Add(Variant);
                continue;
            }

            std::visit(ck::Visitor([&](const auto& InFindPath) -> void
            {
                --_BudgetRemainingThisTick;

                const auto bSucceeded = FCk_Nav_Algorithm::FindPathSync(
                    *NavSys,
                    *NavData,
                    StartLocation,
                    InFindPath.Get_TargetLocation(),
                    InFindPath.Get_AllowPartialPath(),
                    ProjectionExtent,
                    /*InAgentRadiusForFirstSkip*/ 0.0f,
                    InResult);

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
            }), Variant);
        }

        InRequests._Requests = MoveTemp(Remaining);
    }
}

// --------------------------------------------------------------------------------------------------------------------
