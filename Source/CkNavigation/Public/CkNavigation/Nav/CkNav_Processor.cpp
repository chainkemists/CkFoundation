#include "CkNav_Processor.h"

#include "CkNav_Algorithm.h"

#include "CkNavigation/CkNavigation_Log.h"
#include "CkNavigation/Settings/CkNav_ProjectSettings.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

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
CK_NAV_FACTORY(ck::FProcessor_Nav_CrowdReadVelocity);
CK_NAV_FACTORY(ck::FProcessor_Nav_CrowdEndPlay);

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    // Resolve the live dtCrowd weak ptr from the registry's context. Cannot capture the
    // weak ptr at processor construction time because the editor builds the processor graph
    // BEFORE any subsystem publishes its context — a snapshot taken at ctor time is empty
    // forever, even after the subsystem allocates Crowd. Always read fresh per-tick.
    auto Resolve_Crowd(const FCk_Handle& InAnyHandle) -> TSharedPtr<dtCrowd>
    {
        const auto* CtxPtr = InAnyHandle.Get_Registry().TryGetContext<TWeakPtr<dtCrowd>>();
        return CtxPtr != nullptr ? CtxPtr->Pin() : TSharedPtr<dtCrowd>{};
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FProcessor_Nav_HandleRequests::
        FProcessor_Nav_HandleRequests(
            const RegistryType& InRegistry,
            const TWeakPtr<dtCrowd>& /*InCrowdWeak*/,
            const TWeakObjectPtr<ARecastNavMesh>& /*InNavMeshWeak*/)
        : TProcessor(InRegistry)
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
            const TWeakPtr<dtCrowd>& /*InCrowdWeak*/,
            const TWeakObjectPtr<ARecastNavMesh>& /*InNavMeshWeak*/)
        : TProcessor(InRegistry)
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
        // Tolerate null Crowd silently here BY DESIGN: the registry context is briefly empty
        // (a) during the subsystem's pre-bind warm-up — agents added before DoTryBindNavSystem
        // first fires hit this branch, and (b) inside the regen-rebuild timer callback where
        // DoExecuteCrowdRebuild clears + re-marks + re-publishes within a single tick. The
        // dirty-marker re-mark in both paths queues these agents for the next CrowdSetup
        // pass, so a missing context here is recoverable, not a bug.
        const auto Crowd = Resolve_Crowd(InHandle);
        if (ck::Is_NOT_Valid(Crowd))
        { return; }

        // Project the entity location onto the navmesh BEFORE registering with dtCrowd.
        // dtCrowd::addAgent uses the navmesh's DefaultQueryExtent (~agent radius/height) to
        // resolve a poly; if the entity transform sits far above/below the navmesh surface
        // (e.g. spawned at Z=0 with the floor at Z=-300), addAgent silently registers on a
        // bogus poly or fails. Same root cause as the FindPath issue — fix it the same way.
        auto AgentLocation = InTransform.Get_Transform().GetLocation();

        if (const auto* NavQuery = Crowd->getNavMeshQuery())
        {
            const auto SearchExtent = UCk_Utils_Nav_ProjectSettings::Get_NavQuerySearchHalfExtent();
            const auto [PolyRef, Snapped] = FCk_Nav_Algorithm::FindNearestPoly(*NavQuery, AgentLocation, SearchExtent);
            if (PolyRef != 0)
            { AgentLocation = Snapped; }
        }

        const auto CrowdParams = FCk_Nav_Algorithm::BuildCrowdAgentParams(InParams);
        const auto AgentIndex  = FCk_Nav_Algorithm::RegisterAgent(*Crowd, AgentLocation, CrowdParams);

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

        // Snap the entity transform to the projected location so CrowdPushPosition's
        // distance check stays in steady-state no-op territory (otherwise every frame the
        // entity transform vs. dtCrowd npos disagree by 300+ cm and we rebuild the agent
        // continuously). Deferred via Request_SetTransform — applied on the next Transform
        // group pass, which is fine: dtCrowd has the projected position already.
        // Cast to the typed transform handle (guaranteed valid — we matched on FFragment_Transform).
        auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle);
        UCk_Utils_Transform_UE::Request_SetTransform(
            TransformHandle,
            FCk_Request_Transform_SetTransform{FTransform{AgentLocation}});
    }

    // ----------------------------------------------------------------------------------------------------------------
    // FProcessor_Nav_CrowdPushPosition
    // ----------------------------------------------------------------------------------------------------------------

    FProcessor_Nav_CrowdPushPosition::
        FProcessor_Nav_CrowdPushPosition(
            const RegistryType& InRegistry,
            const TWeakPtr<dtCrowd>& /*InCrowdWeak*/,
            const TWeakObjectPtr<ARecastNavMesh>& /*InNavMeshWeak*/)
        : TProcessor(InRegistry)
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
        // Tolerate null Crowd silently here BY DESIGN: a CrowdRegistered agent can briefly
        // see empty context during the regen-rebuild timer callback (DoExecuteCrowdRebuild
        // clears + re-publishes within one tick). Re-marking inside the rebuild ensures
        // the agent comes back through CrowdSetup once the new Crowd is published.
        const auto Crowd = Resolve_Crowd(InHandle);
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

        // Project the new location onto the navmesh — same rationale as CrowdSetup. If the
        // consumer warps the transform far above the navmesh (Z=0 vs navmesh Z=-300),
        // addAgent's small DefaultQueryExtent can't span the delta and registration fails.
        auto NewLocation = EntityLocation;
        if (const auto* NavQuery = Crowd->getNavMeshQuery())
        {
            const auto SearchExtent = UCk_Utils_Nav_ProjectSettings::Get_NavQuerySearchHalfExtent();
            const auto [PolyRef, Snapped] = FCk_Nav_Algorithm::FindNearestPoly(*NavQuery, EntityLocation, SearchExtent);
            if (PolyRef != 0)
            { NewLocation = Snapped; }
        }

        const auto CrowdParams = FCk_Nav_Algorithm::BuildCrowdAgentParams(InParams);
        const auto NewIndex    = FCk_Nav_Algorithm::RegisterAgent(*Crowd, NewLocation, CrowdParams);

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

        // If projection moved the new location off the entity's transform, snap the entity
        // transform too. Otherwise the next frame's distance check sees the same delta and
        // we rebuild forever.
        if (NOT NewLocation.Equals(EntityLocation))
        {
            auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle);
            UCk_Utils_Transform_UE::Request_SetTransform(
                TransformHandle,
                FCk_Request_Transform_SetTransform{FTransform{NewLocation}});
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // FProcessor_Nav_CrowdUpdateTarget
    // ----------------------------------------------------------------------------------------------------------------

    FProcessor_Nav_CrowdUpdateTarget::
        FProcessor_Nav_CrowdUpdateTarget(
            const RegistryType& InRegistry,
            const TWeakPtr<dtCrowd>& /*InCrowdWeak*/,
            const TWeakObjectPtr<ARecastNavMesh>& /*InNavMeshWeak*/)
        : TProcessor(InRegistry)
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
        // Tolerate null Crowd silently — same regen-rebuild window rationale as CrowdPushPosition.
        const auto Crowd = Resolve_Crowd(InHandle);
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
            const TWeakPtr<dtCrowd>& /*InCrowdWeak*/,
            const TWeakObjectPtr<ARecastNavMesh>& /*InNavMeshWeak*/)
        : TProcessor(InRegistry)
    {}

    auto
        FProcessor_Nav_CrowdStep::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        // Tolerate null Crowd silently here BY DESIGN: CrowdStep ticks unconditionally
        // (work is in DoTick, not per-entity ForEachEntity), so the framework calls it
        // even in worlds where the Navigation subsystem doesn't run (editor preview, asset
        // preview) and during the brief pre-bind tick before the subsystem's Tick first
        // fires DoTryBindNavSystem. Both are legitimately "no work to do" — no agents can
        // be registered without the subsystem alive, so there's nothing to step.
        const auto* CtxPtr = this->_TransientEntity.Get_Registry().TryGetContext<TWeakPtr<dtCrowd>>();
        const auto Crowd = CtxPtr != nullptr ? CtxPtr->Pin() : TSharedPtr<dtCrowd>{};
        if (ck::Is_NOT_Valid(Crowd))
        { return; }

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

    // ----------------------------------------------------------------------------------------------------------------
    // FProcessor_Nav_CrowdReadVelocity
    // ----------------------------------------------------------------------------------------------------------------

    FProcessor_Nav_CrowdReadVelocity::
        FProcessor_Nav_CrowdReadVelocity(
            const RegistryType& InRegistry,
            const TWeakPtr<dtCrowd>& /*InCrowdWeak*/,
            const TWeakObjectPtr<ARecastNavMesh>& /*InNavMeshWeak*/)
        : TProcessor(InRegistry)
    {}

    auto
        FProcessor_Nav_CrowdReadVelocity::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Nav_CrowdAgent& InCrowdAgent,
            FFragment_Nav_CrowdVelocity& InVelocity) const
        -> void
    {
        // Tolerate null Crowd silently — same regen-rebuild window rationale as CrowdPushPosition.
        const auto Crowd = Resolve_Crowd(InHandle);
        if (ck::Is_NOT_Valid(Crowd))
        { return; }

        const auto* Agent = Crowd->getAgent(InCrowdAgent.Get_CrowdAgentIndex());
        if (Agent == nullptr || NOT Agent->active)
        { return; }

        InVelocity._Velocity        = FCk_Nav_Algorithm::FromRecastFloat3(Agent->vel);
        InVelocity._DesiredVelocity = FCk_Nav_Algorithm::FromRecastFloat3(Agent->dvel);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // FProcessor_Nav_CrowdEndPlay
    // ----------------------------------------------------------------------------------------------------------------

    FProcessor_Nav_CrowdEndPlay::
        FProcessor_Nav_CrowdEndPlay(
            const RegistryType& InRegistry,
            const TWeakPtr<dtCrowd>& /*InCrowdWeak*/,
            const TWeakObjectPtr<ARecastNavMesh>& /*InNavMeshWeak*/)
        : TProcessor(InRegistry)
    {}

    auto
        FProcessor_Nav_CrowdEndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Nav_CrowdAgent& InCrowdAgent) const
        -> void
    {
        // Tolerate null Crowd silently here BY DESIGN: the world subsystem may have torn
        // down before entity teardown (matches CkSpatialQuery's deinit ordering). This is
        // the one place where missing context is genuinely expected, not a bug — every other
        // Crowd-using processor ensures.
        const auto Crowd = Resolve_Crowd(InHandle);
        if (ck::Is_NOT_Valid(Crowd))
        { return; }

        const auto Idx = InCrowdAgent.Get_CrowdAgentIndex();
        if (Idx >= 0)
        { Crowd->removeAgent(Idx); }
    }
}

// --------------------------------------------------------------------------------------------------------------------
