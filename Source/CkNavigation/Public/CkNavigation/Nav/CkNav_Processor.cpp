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
CK_NAV_FACTORY(ck::FProcessor_Nav_CrowdSetup);
CK_NAV_FACTORY(ck::FProcessor_Nav_CrowdPushPosition);
CK_NAV_FACTORY(ck::FProcessor_Nav_CrowdUpdateTarget);
CK_NAV_FACTORY(ck::FProcessor_Nav_CrowdStep);

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
        // No work to do? (Run this guard before BudgetCap so an empty queue on a budget-disabled
        // setup doesn't spuriously stamp BudgetDisabled diagnostics.)
        if (InRequests.Get_Requests().IsEmpty())
        { return; }

        const auto AgentLocation = InTransform.Get_Transform().GetLocation();

        const auto FailAllQueued = [&](ECk_Nav_PathFailReason InReason)
        {
            // Reset diagnostics so the recorded reason is unambiguous, capture target of the
            // FIRST queued request as the "last attempted" target. PRESERVE _Waypoints (consumer
            // may want to keep walking the old path).
            const auto FirstTarget = InRequests.Get_Requests()[0].Get_TargetLocation();
            InPathResult._Diagnostics = FCk_Nav_PathDiagnostics{};
            InPathResult._Diagnostics._LastFailReason     = InReason;
            InPathResult._Diagnostics._LastTargetLocation = FirstTarget;
            InPathResult._Diagnostics._LastAgentLocation  = AgentLocation;
            InPathResult._Diagnostics._LastQueryWallTime  = FPlatformTime::Seconds();
            InPathResult._Status = ECk_Nav_PathStatus::Failed;

            InHandle.AddOrGet<FTag_Nav_PathFailed>();
            InHandle.Try_Remove<FTag_Nav_PathPending>();
            InHandle.Try_Remove<FTag_Nav_PathReady>();
            UUtils_Signal_OnNavPathFailed::Broadcast(InHandle, MakePayload(InHandle));

            InRequests.Get_Requests().Reset();
        };

        // P3-2: budget == 0 is DISABLED (subsystem warns once at init). Surface as a fail-reason
        // so the debugger shows "BudgetDisabled" instead of silent queue accumulation.
        const auto BudgetCap = UCk_Utils_Nav_ProjectSettings::Get_MaxPathQueriesPerFrame();
        if (BudgetCap == 0)
        {
            FailAllQueued(ECk_Nav_PathFailReason::BudgetDisabled);
            return;
        }

        // Resolve nav data once per agent per frame.
        auto* World   = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        auto* NavSys  = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);

        if (ck::Is_NOT_Valid(NavSys, ck::IsValid_Policy_NullptrOnly{}))
        {
            FailAllQueued(ECk_Nav_PathFailReason::NoNavSystem);
            return;
        }

        auto* NavData = Cast<ARecastNavMesh>(NavSys->GetDefaultNavDataInstance());

        if (ck::Is_NOT_Valid(NavData, ck::IsValid_Policy_NullptrOnly{}))
        {
            // Nav stack not ready (subsystem will republish context once nav bake fires).
            ck::nav::Warning(TEXT("Nav agent [{}]: no ARecastNavMesh; failing [{}] queued path request(s)."),
                InHandle, InRequests.Get_Requests().Num());
            FailAllQueued(ECk_Nav_PathFailReason::NoNavData);
            return;
        }

        if (NOT NavData->GetDefaultQueryFilter().IsValid())
        {
            FailAllQueued(ECk_Nav_PathFailReason::NoDefaultFilter);
            return;
        }

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

    // ----------------------------------------------------------------------------------------------------------------
    // FProcessor_Nav_CrowdSetup
    // ----------------------------------------------------------------------------------------------------------------

    FProcessor_Nav_CrowdSetup::
        FProcessor_Nav_CrowdSetup(
            const RegistryType& InRegistry,
            const TWeakPtr<dtCrowd>& InCrowdWeak,
            const TWeakObjectPtr<ARecastNavMesh>& InNavMeshWeak)
        : TProcessor(InRegistry)
        , _CrowdWeak(InCrowdWeak)
        , _NavMeshWeak(InNavMeshWeak)
    {}

    auto
        FProcessor_Nav_CrowdSetup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Nav_AgentParams& InParams,
            const FFragment_Transform& InTransform) const
        -> void
    {
        const auto Crowd = _CrowdWeak.Pin();
        if (ck::Is_NOT_Valid(Crowd))
        {
            // Debounce window during nav-regen — context is intentionally empty until
            // DoReallocateCrowdAndPublishContext republishes. NeedsSetup stays set; we'll
            // pick this agent up next frame.
            return;
        }

        const auto AgentLocation = InTransform.Get_Transform().GetLocation();
        const auto CrowdParams   = FCk_Nav_Algorithm::BuildCrowdAgentParams(InParams);
        const auto AgentIndex    = FCk_Nav_Algorithm::RegisterAgent(*Crowd, AgentLocation, CrowdParams);

        if (AgentIndex == -1)
        {
            ck::nav::Error(TEXT("Nav agent [{}]: dtCrowd full (MaxCrowdAgents=[{}]); registration FAILED — agent will not steer."),
                InHandle, UCk_Utils_Nav_ProjectSettings::Get_MaxCrowdAgents());
            InHandle.Try_Remove<FTag_Nav_NeedsSetup>();
            return;
        }

        InHandle.Add<FFragment_Nav_CrowdAgent>(FFragment_Nav_CrowdAgent{AgentIndex});
        InHandle.AddOrGet<FTag_Nav_CrowdRegistered>();
        InHandle.Try_Remove<FTag_Nav_NeedsSetup>();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // FProcessor_Nav_CrowdPushPosition
    // ----------------------------------------------------------------------------------------------------------------

    FProcessor_Nav_CrowdPushPosition::
        FProcessor_Nav_CrowdPushPosition(
            const RegistryType& InRegistry,
            const TWeakPtr<dtCrowd>& InCrowdWeak,
            const TWeakObjectPtr<ARecastNavMesh>& InNavMeshWeak)
        : TProcessor(InRegistry)
        , _CrowdWeak(InCrowdWeak)
        , _NavMeshWeak(InNavMeshWeak)
    {}

    auto
        FProcessor_Nav_CrowdPushPosition::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Nav_CrowdAgent& InCrowdAgent,
            const FFragment_Transform& InTransform,
            const FFragment_Nav_AgentParams& InParams) const
        -> void
    {
        const auto Crowd = _CrowdWeak.Pin();
        if (ck::Is_NOT_Valid(Crowd))
        { return; }

        const auto* Agent = Crowd->getAgent(InCrowdAgent.Get_CrowdAgentIndex());
        if (Agent == nullptr || NOT Agent->active)
        { return; }

        const auto EntityLocation = InTransform.Get_Transform().GetLocation();
        const auto NPosLocation   = FCk_Nav_Algorithm::FromRecastFloat3(Agent->npos);
        const auto DeltaUu        = FVector::Distance(EntityLocation, NPosLocation);
        const auto ThresholdUu    = UCk_Utils_Nav_ProjectSettings::Get_TeleportThresholdUu();

        if (DeltaUu <= ThresholdUu)
        { return; }   // steady state: zero work, dtCrowd's own integration keeps npos in sync

        // Above threshold — teleport. Full removeAgent + addAgent. A direct npos write would
        // desync the path corridor's poly cache (silent avoidance bugs near walls).
        const auto OldIndex = InCrowdAgent.Get_CrowdAgentIndex();
        Crowd->removeAgent(OldIndex);

        const auto CrowdParams = FCk_Nav_Algorithm::BuildCrowdAgentParams(InParams);
        const auto NewIndex    = FCk_Nav_Algorithm::RegisterAgent(*Crowd, EntityLocation, CrowdParams);

        if (NewIndex == -1)
        {
            ck::nav::Warning(TEXT("Nav agent [{}]: teleport rebuild failed (crowd full); demoting to NeedsSetup."), InHandle);
            InHandle.Try_Remove<FTag_Nav_CrowdRegistered>();
            InHandle.Remove<FFragment_Nav_CrowdAgent>();
            InHandle.AddOrGet<FTag_Nav_NeedsSetup>();
            return;
        }

        InCrowdAgent._CrowdAgentIndex = NewIndex;
        ck::nav::Verbose(TEXT("Nav agent [{}]: teleport rebuild (delta=[{}] uu); crowd index [{}] -> [{}]"),
            InHandle, DeltaUu, OldIndex, NewIndex);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // FProcessor_Nav_CrowdUpdateTarget
    // ----------------------------------------------------------------------------------------------------------------

    FProcessor_Nav_CrowdUpdateTarget::
        FProcessor_Nav_CrowdUpdateTarget(
            const RegistryType& InRegistry,
            const TWeakPtr<dtCrowd>& InCrowdWeak,
            const TWeakObjectPtr<ARecastNavMesh>& InNavMeshWeak)
        : TProcessor(InRegistry)
        , _CrowdWeak(InCrowdWeak)
        , _NavMeshWeak(InNavMeshWeak)
    {}

    auto
        FProcessor_Nav_CrowdUpdateTarget::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Nav_CrowdAgent& InCrowdAgent,
            const FFragment_Nav_PathResult& InPathResult) const
        -> void
    {
        const auto Crowd = _CrowdWeak.Pin();
        if (ck::Is_NOT_Valid(Crowd))
        { return; }

        const auto& Waypoints  = InPathResult.Get_Waypoints();
        const auto Destination = Waypoints.IsEmpty()
                               ? InPathResult.Get_DestinationLocation()
                               : Waypoints.Last();

        const auto* NavQuery = Crowd->getNavMeshQuery();
        if (NavQuery == nullptr)
        {
            ck::nav::Warning(TEXT("Nav agent [{}]: dtCrowd has no nav-mesh-query; skipping move-target push."), InHandle);
            InHandle.Try_Remove<FTag_Nav_MoveTargetDirty>();
            return;
        }

        const auto SearchExtent     = UCk_Utils_Nav_ProjectSettings::Get_NavQuerySearchHalfExtent();
        const auto [PolyRef, Snapped] = FCk_Nav_Algorithm::FindNearestPoly(*NavQuery, Destination, SearchExtent);

        if (PolyRef == 0)
        {
            ck::nav::Warning(TEXT("Nav agent [{}]: destination [{}] is not on the navmesh; skipping move-target push."),
                InHandle, Destination);
            InHandle.Try_Remove<FTag_Nav_MoveTargetDirty>();
            return;
        }

        dtReal RecastDest[3];
        FCk_Nav_Algorithm::ToRecastFloat3(Snapped, RecastDest);

        const auto Ok = Crowd->requestMoveTarget(InCrowdAgent.Get_CrowdAgentIndex(), PolyRef, RecastDest);
        if (NOT Ok)
        {
            // P3-6: false from requestMoveTarget means start poly and target poly are in
            // disconnected nav components, or the agent index is invalid post-rebuild.
            ck::nav::Warning(TEXT("Nav agent [{}]: dtCrowd::requestMoveTarget returned false (target=[{}], poly=[{}])"),
                InHandle, Destination, PolyRef);
            InHandle.AddOrGet<FTag_Nav_PathFailed>();
            InHandle.Try_Remove<FTag_Nav_PathReady>();
        }

        InHandle.Try_Remove<FTag_Nav_MoveTargetDirty>();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // FProcessor_Nav_CrowdStep
    // ----------------------------------------------------------------------------------------------------------------

    FProcessor_Nav_CrowdStep::
        FProcessor_Nav_CrowdStep(
            const RegistryType& InRegistry,
            const TWeakPtr<dtCrowd>& InCrowdWeak,
            const TWeakObjectPtr<ARecastNavMesh>& /*InNavMeshWeak*/)
        : TProcessor(InRegistry)
        , _CrowdWeak(InCrowdWeak)
    {}

    auto
        FProcessor_Nav_CrowdStep::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        const auto Crowd = _CrowdWeak.Pin();
        if (ck::Is_NOT_Valid(Crowd))
        { return; }   // silent — debounce window during nav-regen (P3-3)

        Crowd->update(static_cast<float>(InDeltaT.Get_Seconds()), nullptr);

        // Deliberately do NOT call TProcessor::DoTick(InDeltaT). dtCrowd::update advanced
        // every agent in one pass; per-agent ForEachEntity iteration would be pure overhead.
    }

    auto
        FProcessor_Nav_CrowdStep::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType /*InHandle*/,
            const FFragment_Nav_CrowdAgent& /*InCrowdAgent*/) const
        -> void
    {
        // No-op — exists only to satisfy the CRTP framework. Work happens in DoTick.
    }
}

// --------------------------------------------------------------------------------------------------------------------
