#include "CkNav_Processor.h"

#include "CkNavigation/CkNavigation_Log.h"
#include "CkNavigation/CkNavigation_Stats.h"
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

DECLARE_CYCLE_STAT(TEXT("Nav::DeferredDrain"),   STAT_Nav_DeferredDrain,   STATGROUP_CkNav);
DECLARE_CYCLE_STAT(TEXT("Nav::HandleRequests"),  STAT_Nav_HandleRequests,  STATGROUP_CkNav);

DECLARE_DWORD_COUNTER_STAT(TEXT("Nav Deferred Queue Depth"),    STAT_Nav_DeferredQueueDepth,    STATGROUP_CkNav);
DECLARE_DWORD_COUNTER_STAT(TEXT("Nav Deferred Reprojections"),  STAT_Nav_DeferredReprojections, STATGROUP_CkNav);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_nav_processor
{
    // One process-wide queue, NOT keyed on world — not multi-PIE-instance safe.
    struct FCk_Nav_DeferredRequest
    {
        FCk_Handle Handle;
        FCk_Request_Nav_FindPath Request;
        double DeferredAt = 0.0;   // FPlatformTime::Seconds() at first deferral — drives timeout
    };

    static TArray<FCk_Nav_DeferredRequest> GDeferredNavRequests;

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
        _BudgetRemainingThisTick = UCk_Utils_Nav_Settings_UE::Get_MaxPathQueriesPerFrame();

        auto DrainActions = int32{0};
        if (ck_nav_processor::GDeferredNavRequests.Num() > 0)
        {
            SCOPE_CYCLE_COUNTER(STAT_Nav_DeferredDrain);
            SET_DWORD_STAT(STAT_Nav_DeferredQueueDepth, ck_nav_processor::GDeferredNavRequests.Num());

            const auto Now = FPlatformTime::Seconds();
            const auto MaxDeferralSec = static_cast<double>(ck_nav_processor::CVarMaxDeferralSeconds.GetValueOnGameThread());

            for (auto i = ck_nav_processor::GDeferredNavRequests.Num() - 1; i >= 0; --i)
            {
                auto& Entry = ck_nav_processor::GDeferredNavRequests[i];
                if (NOT ck::IsValid(Entry.Handle))
                { ck_nav_processor::GDeferredNavRequests.RemoveAt(i, EAllowShrinking::No); continue; }

                auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(Entry.Handle);
                auto* NavSys = IsValid(World) ? UNavigationSystemV1::GetCurrent(World) : nullptr;
                auto* NavData = (NavSys != nullptr)
                    ? Cast<ARecastNavMesh>(NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate))
                    : nullptr;

                auto TransformHandle = UCk_Utils_Transform_UE::Cast(Entry.Handle);
                if (NavSys == nullptr || NavData == nullptr || ck::Is_NOT_Valid(TransformHandle))
                {
                    if ((Now - Entry.DeferredAt) >= MaxDeferralSec)
                    { goto ForceFail; }
                    continue;
                }
                {
                    const auto StartLocation = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TransformHandle);
                    const auto Extent = UCk_Utils_Nav_Settings_UE::Get_NavQueryProjectionExtentVec();
                    auto ProbeProj = FNavLocation{};
                    INC_DWORD_STAT(STAT_Nav_DeferredReprojections);
                    const auto bReady = NavSys->ProjectPointToNavigation(StartLocation, ProbeProj, Extent, NavData);
                    if (bReady)
                    {
                        auto Handle = Entry.Handle;
                        auto Request = Entry.Request;
                        ck_nav_processor::GDeferredNavRequests.RemoveAt(i, EAllowShrinking::No);
                        UCk_Utils_Nav_UE::Request_FindPath(Handle, Request);
                        ++DrainActions;
                        continue;
                    }
                }

                if ((Now - Entry.DeferredAt) < MaxDeferralSec)
                { continue; }

            ForceFail:
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
                    ck_nav_processor::GDeferredNavRequests.RemoveAt(i, EAllowShrinking::No);
                    ++DrainActions;
                }
            }
        }

        TProcessor::DoTick(InDeltaT);

        // The drain does work the base DoTick's view count cannot see; a pump that only drained
        // would report zero and the scheduler would skip the follow-up pass (FTickable_Concept::Pump).
        if (DrainActions > 0 and this->_LastVisitedCount >= 0)
        {
            this->_LastVisitedCount += DrainActions;
        }
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
        SCOPE_CYCLE_COUNTER(STAT_Nav_HandleRequests);

        if (NOT UCk_Utils_Net_UE::Get_HasAuthority(InHandle))
        {
            InResult._Status = ECk_Nav_PathStatus::Failed;
            InResult._Diagnostics._LastFailReason = ECk_Nav_PathFailReason::NotAuthority;
            ck::nav::Warning(TEXT("FindPath request on [{}] dropped: client lacks authority"), InHandle);
            // Early-outs skip the CopyAndRemove drain below, so clear the queue here instead.
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
        const auto ProjectionVerticalExtent = UCk_Utils_Nav_Settings_UE::Get_NavQueryVerticalHalfExtent();

        const auto ExtentVec = UCk_Utils_Nav_Settings_UE::Get_NavQueryProjectionExtentVec();
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
                        ck_nav_processor::GDeferredNavRequests.Add({InHandle, InFindPath, NowSec});
                    }, Variant);
                }
            });
            ck::nav::Verbose(TEXT("FindPath on [{}] deferred — start point not yet bakeable (queue depth now {})"),
                InHandle, ck_nav_processor::GDeferredNavRequests.Num());
            return;
        }

        // CopyAndRemove snapshots AND removes the fragment, so Utils' next AddOrGet re-arms the
        // MarkedDirtyBy event — without the removal only the first request ever would drain.
        InHandle.CopyAndRemove(InRequests, [&](const FFragment_Nav_Requests& InSnapshot)
        {
            ck::algo::ForEachRequest(InSnapshot._Requests, ck::Visitor(
                [&](const auto& InFindPath) -> void
                {
                    const auto FilterClass = UCk_Utils_Nav_Settings_UE::Get_QueryFilterClass(InFindPath.Get_QueryFilter());

                    // The readiness gate above probed the ENTITY location; an override start sits
                    // within a band-width of it (same tiles), so that answer carries over.
                    const auto QueryStart = InFindPath.Get_StartOverride() == ECk_EnableDisable::Enable
                        ? InFindPath.Get_StartOverrideLocation()
                        : StartLocation;

                    const auto bSucceeded = FCk_Nav_Algorithm::FindPathSync(
                        *NavSys,
                        *NavData,
                        QueryStart,
                        InFindPath.Get_TargetLocation(),
                        InFindPath.Get_AllowPartialPath(),
                        ProjectionExtent,
                        ProjectionVerticalExtent,
                        /*InAgentRadiusForFirstSkip*/ 0.0f,
                        InResult,
                        FilterClass);

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
