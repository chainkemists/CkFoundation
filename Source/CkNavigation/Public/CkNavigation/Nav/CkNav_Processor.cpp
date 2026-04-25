#include "CkNav_Processor.h"

#include "CkNav_Algorithm.h"

#include "CkNavigation/CkNavigation_Log.h"
#include "CkNavigation/Settings/CkNav_ProjectSettings.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include <Engine/World.h>
#include <NavigationSystem.h>
#include <NavMesh/RecastNavMesh.h>

#include "DetourCrowd/DetourCrowd.h"

// --------------------------------------------------------------------------------------------------------------------

// The factory runs at processor-graph build time, which includes the editor world. Our
// Navigation subsystem only initializes for Game/PIE worlds (per UCk_Game_WorldSubsystem_Base_UE)
// so the registry context entries are MISSING at editor-graph build. GetContext on a missing
// entry returns a bogus reference that crashes on deref — use TryGetContext + empty fallback.
#define CK_NAV_FACTORY(ProcessorType) \
    CK_REGISTER_PROCESSOR_WITH_FACTORY(ProcessorType, \
        [](const FCk_Registry& InRegistry) -> ck::concepts::FTickableType \
        { \
            const auto* CrowdWeakPtr   = InRegistry.TryGetContext<TWeakPtr<dtCrowd>>(); \
            const auto* NavMeshWeakPtr = InRegistry.TryGetContext<TWeakObjectPtr<ARecastNavMesh>>(); \
            const auto CrowdWeak   = CrowdWeakPtr   != nullptr ? *CrowdWeakPtr   : TWeakPtr<dtCrowd>{}; \
            const auto NavMeshWeak = NavMeshWeakPtr != nullptr ? *NavMeshWeakPtr : TWeakObjectPtr<ARecastNavMesh>{}; \
            return ProcessorType{InRegistry, CrowdWeak, NavMeshWeak}; \
        })

CK_NAV_FACTORY(ck::FProcessor_Nav_HandleRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FProcessor_Nav_HandleRequests::
        FProcessor_Nav_HandleRequests(
            const RegistryType& InRegistry,
            const TWeakPtr<dtCrowd>& InCrowdWeak,
            const TWeakObjectPtr<ARecastNavMesh>& InNavMeshWeak)
        : TProcessor(InRegistry)
        , _CrowdWeak(InCrowdWeak)
        , _NavMeshWeak(InNavMeshWeak)
    {}

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Nav_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        // P3-2: reset per-frame budget at the top of each tick. Mirrors CkProbe_Processor.cpp:818-827.
        _RemainingQueriesThisFrame = UCk_Utils_Nav_ProjectSettings::Get_MaxPathQueriesPerFrame();

        TProcessor::DoTick(InDeltaT);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Nav_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Nav_AgentParams& InParams,
            const FFragment_Transform& InTransform,
            FFragment_Nav_Requests& InRequests,
            FFragment_Nav_PathResult& InPathResult)
        -> void
    {
        // P3-2: budget == 0 is DISABLED (subsystem warns once at init). Drop all queries silently —
        // they'll stay queued via FTag_Nav_PathPending and re-fire next frame (where they'll
        // also be dropped, until the setting is fixed). NOT "unbounded"; that's a footgun.
        const auto BudgetCap = UCk_Utils_Nav_ProjectSettings::Get_MaxPathQueriesPerFrame();
        if (BudgetCap == 0)
        { return; }

        // No work to do?
        if (InRequests.Get_Requests().IsEmpty())
        { return; }

        // Resolve nav data once per agent per frame.
        auto* World   = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        auto* NavSys  = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
        auto* NavData = ck::IsValid(NavSys, ck::IsValid_Policy_NullptrOnly{})
                      ? Cast<ARecastNavMesh>(NavSys->GetDefaultNavDataInstance())
                      : nullptr;

        if (ck::Is_NOT_Valid(NavData, ck::IsValid_Policy_NullptrOnly{}))
        {
            // Nav stack not ready (subsystem will republish context once nav bake fires).
            // Mark all queued requests as failed and drop them so we don't accumulate forever.
            ck::nav::Warning(TEXT("Nav agent [{}]: no ARecastNavMesh; failing [{}] queued path request(s)."),
                InHandle, InRequests.Get_Requests().Num());

            InPathResult._Status = ECk_Nav_PathStatus::Failed;
            InHandle.AddOrGet<FTag_Nav_PathFailed>();   // idempotent across repeat-fail frames
            InHandle.Try_Remove<FTag_Nav_PathPending>();
            UUtils_Signal_OnNavPathFailed::Broadcast(InHandle, MakePayload(InHandle));

            InRequests.Get_Requests().Reset();
            return;
        }

        const auto AgentLocation = InTransform.Get_Transform().GetLocation();

        // Drain the queue in-order, consuming budget per request.
        // Process up to (BudgetCap remaining) requests, leave the rest queued.
        auto& Queue = InRequests.Get_Requests();
        auto ProcessedCount = 0;

        for (auto i = 0; i < Queue.Num(); ++i)
        {
            if (_RemainingQueriesThisFrame <= 0)
            { break; }

            const auto& Request = Queue[i];

            const auto bOk = FCk_Nav_Algorithm::FindPathSync(
                *NavSys, *NavData, AgentLocation, Request.Get_TargetLocation(),
                InParams, Request.Get_AllowPartialPath(), InPathResult);

            --_RemainingQueriesThisFrame;
            ++ProcessedCount;

            if (bOk)
            {
                // Ready or Partial — broadcast Ready (consumer reads _Status to distinguish).
                // AddOrGet is idempotent — Add ensures when the tag already exists from a prior
                // success, which happens on every repeat success frame.
                InHandle.AddOrGet<FTag_Nav_PathReady>();
                InHandle.Try_Remove<FTag_Nav_PathFailed>();
                InHandle.AddOrGet<FTag_Nav_MoveTargetDirty>();

                UUtils_Signal_OnNavPathReady::Broadcast(InHandle, MakePayload(InHandle, InPathResult));
            }
            else
            {
                // Note: ExtractWaypoints PRESERVES previous _Waypoints on failure. Only _Status
                // and _DestinationLocation are updated. Consumer can keep walking the old path.
                InHandle.AddOrGet<FTag_Nav_PathFailed>();
                InHandle.Try_Remove<FTag_Nav_PathReady>();

                UUtils_Signal_OnNavPathFailed::Broadcast(InHandle, MakePayload(InHandle));
            }
        }

        // Remove the requests we processed; leave any that didn't fit the budget for next frame.
        if (ProcessedCount == Queue.Num())
        {
            Queue.Reset();
            InHandle.Try_Remove<FTag_Nav_PathPending>();   // Try_ — Request_FindPath uses AddOrGet, but defensive
        }
        else
        {
            Queue.RemoveAt(0, ProcessedCount, EAllowShrinking::No);
            // FTag_Nav_PathPending stays set so the queue is re-evaluated next frame.
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
