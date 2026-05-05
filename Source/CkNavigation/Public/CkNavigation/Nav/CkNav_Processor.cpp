#include "CkNav_Processor.h"

#include "CkNavigation/CkNavigation_Log.h"
#include "CkNavigation/Nav/CkNav_Algorithm.h"
#include "CkNavigation/Settings/CkNav_ProjectSettings.h"
#include "CkNavigation/Utils/CkNav_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include <NavigationSystem.h>
#include <NavMesh/RecastNavMesh.h>

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    // Process-wide queue of FindPath requests that landed while the nav system was rebuilding.
    // Drained from FProcessor_Nav_HandleRequests::DoTick once IsNavigationBuildInProgress flips
    // false again. Stale handles (PIE end, entity destroyed) are discarded on drain via ck::IsValid.
    //
    // Note: single global queue is intentionally simple — not multi-PIE-instance safe. If a future
    // requirement exposes multiple concurrent worlds with separate nav systems, this needs to key
    // on the entity's owning world.
    struct FCk_Nav_DeferredRequest
    {
        FCk_Handle Handle;
        FCk_Request_Nav_FindPath Request;
        double DeferredAt = 0.0;   // FPlatformTime::Seconds() at first deferral — drives timeout
    };

    static TArray<FCk_Nav_DeferredRequest> GDeferredNavRequests;

    // Hard timeout: if a request has been deferred this long and the nav system still isn't
    // ready, force-fail it so the agent transitions out of Pending and the dev sees the red
    // "NO PATH" marker. Without this the agent sits in yellow Pending forever which is also
    // visible (we draw both states) but the failure semantics is more useful for downstream
    // logic that wants to react to OnPathFailed.
    static TAutoConsoleVariable<float> CVarMaxDeferralSeconds(
        TEXT("ck.Nav.MaxDeferralSeconds"),
        5.0f,
        TEXT("Hard timeout for the deferred-FindPath queue. After this many seconds without the nav\n")
        TEXT("system becoming ready, the deferred request is force-failed with NoNavData so the\n")
        TEXT("caller transitions out of Pending. Default 5s."),
        ECVF_Default);
}

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

        // Drain deferred requests. Per-request readiness check: re-issue any request whose
        // start point now projects cleanly to the navmesh; leave the rest queued. Requests
        // older than the timeout get force-failed so the agent transitions out of Pending.
        //
        // The previous "global ready" gate (IsNavigationBuildInProgress + NavBounds check) was
        // wrong for dynamic navmeshes — IsNavigationBuildInProgress can stay true forever as
        // the system processes dirty tiles, so the queue never drained. Per-point projection
        // is what we actually care about anyway: "is THIS agent's start position on a baked
        // tile right now". Same call FindPathSync uses internally.
        if (GDeferredNavRequests.Num() > 0)
        {
            const auto Now = FPlatformTime::Seconds();
            const auto MaxDeferralSec = static_cast<double>(CVarMaxDeferralSeconds.GetValueOnGameThread());

            for (auto i = GDeferredNavRequests.Num() - 1; i >= 0; --i)
            {
                auto& Entry = GDeferredNavRequests[i];
                if (NOT ck::IsValid(Entry.Handle))
                { GDeferredNavRequests.RemoveAt(i, EAllowShrinking::No); continue; }

                auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(Entry.Handle);
                auto* NavSys = IsValid(World) ? UNavigationSystemV1::GetCurrent(World) : nullptr;
                auto* NavData = (NavSys != nullptr)
                    ? Cast<ARecastNavMesh>(NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate))
                    : nullptr;

                // Snap to the agent's current location for the readiness check. Without a
                // Transform we can't probe — try again next tick.
                auto TransformHandle = UCk_Utils_Transform_UE::Cast(Entry.Handle);
                if (NavSys == nullptr || NavData == nullptr || ck::Is_NOT_Valid(TransformHandle))
                {
                    if ((Now - Entry.DeferredAt) >= MaxDeferralSec)
                    { goto ForceFail; }
                    continue;
                }
                {
                    const auto StartLocation = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TransformHandle);
                    const auto Extent = FVector(UCk_Utils_Nav_Settings_UE::Get_NavQuerySearchHalfExtent());
                    auto ProbeProj = FNavLocation{};
                    const auto bReady = NavSys->ProjectPointToNavigation(StartLocation, ProbeProj, Extent, NavData);
                    if (bReady)
                    {
                        // Navmesh is bake-ready under this agent — re-issue and let it go through
                        // the normal ForEachEntity path on the next tick.
                        auto Handle = Entry.Handle;
                        auto Request = Entry.Request;
                        GDeferredNavRequests.RemoveAt(i, EAllowShrinking::No);
                        UCk_Utils_Nav_UE::Request_FindPath(Handle, Request);
                        continue;
                    }
                }

                if ((Now - Entry.DeferredAt) < MaxDeferralSec)
                { continue; }

            ForceFail:
                // Timed out waiting for the bake — force-fail so the agent flips Pending → Idle
                // and the dev sees the red "NO PATH" marker with NoNavData.
                {
                    auto Handle = Entry.Handle;
                    if (Handle.Has<FFragment_Nav_PathResult>())
                    {
                        auto& Result = Handle.Get<FFragment_Nav_PathResult>();
                        Result._Status = ECk_Nav_PathStatus::Failed;
                        Result._Diagnostics._LastFailReason = ECk_Nav_PathFailReason::NoNavData;
                        UUtils_Signal_Nav_OnPathFailed::Broadcast(Handle, ck::MakePayload(Handle));
                        ck::nav::Warning(
                            TEXT("FindPath on [{}] timed out after {}s deferred — failing with NoNavData. ")
                            TEXT("Check that a NavMeshBoundsVolume covers the agent's start position and ")
                            TEXT("that the level has static walkable geometry inside the volume."),
                            Handle, MaxDeferralSec);
                    }
                    GDeferredNavRequests.RemoveAt(i, EAllowShrinking::No);
                }
            }
        }

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

        // Per-point readiness check: defer only if this agent's start location does NOT yet
        // project onto baked navmesh. Same projection call FindPathSync uses internally — same
        // answer, so when projection succeeds we know FindPathSync's [Start] step will too.
        // Earlier global checks (IsNavigationBuildInProgress / NavBounds.IsValid) were unreliable
        // on dynamic navmeshes — that mode keeps the global "in progress" flag set continuously
        // (incremental tile updates), and the queue never drained even when the agent's spot was
        // bakeable. The green "navmesh projection" overlay uses the same call, so when the dev
        // sees a green circle but path requests fail, this gate is what's wrong.
        const auto ExtentVec = FVector(ProjectionExtent);
        auto StartProbe = FNavLocation{};
        const auto bStartReady = (NavData != nullptr)
            && NavSys->ProjectPointToNavigation(StartLocation, StartProbe, ExtentVec, NavData);

        if (NOT bStartReady)
        {
            InResult._Status = ECk_Nav_PathStatus::Pending;
            const auto NowSec = FPlatformTime::Seconds();
            InHandle.CopyAndRemove(InRequests, [&](const FFragment_Nav_Requests& InSnapshot)
            {
                for (const auto& Variant : InSnapshot._Requests)
                {
                    std::visit([&](const auto& InFindPath)
                    {
                        GDeferredNavRequests.Add({InHandle, InFindPath, NowSec});
                    }, Variant);
                }
            });
            ck::nav::Verbose(TEXT("FindPath on [{}] deferred — start point not yet bakeable (queue depth now {})"),
                InHandle, GDeferredNavRequests.Num());
            return;
        }

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
