#include "CkNav_Processor.h"

#include "CkNavigation/CkNavigation_Log.h"
#include "CkNavigation/CkNavigation_Stats.h"
#include "CkNavigation/Nav/CkNav_Algorithm.h"
#include "CkNavigation/Settings/CkNav_ProjectSettings.h"
#include "CkNavigation/Utils/CkNav_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
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
    static auto
        IsSupersededByAuthoritativeRevision(
            int32 InRequestRevision,
            int32 InAuthoritativeRevision)
        -> bool
    {
        return InRequestRevision != 0
            && InAuthoritativeRevision != 0
            && InRequestRevision != InAuthoritativeRevision
            && ck::nav::IsNewerRevision(InAuthoritativeRevision, InRequestRevision);
    }

    static TAutoConsoleVariable<float> CVarMaxDeferralSeconds(
        TEXT("ck.Nav.MaxDeferralSeconds"),
        5.0f,
        TEXT("Hard timeout for the deferred-FindPath queue. After this many seconds without the nav\n")
        TEXT("system becoming ready, the deferred request is force-failed with NoNavData so the\n")
        TEXT("caller transitions out of Pending. Default 5s."),
        ECVF_Default);
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::nav
{
    auto
        IsNewerRevision(
            int32 InCandidate,
            int32 InExisting)
        -> bool
    {
        if (InCandidate == InExisting)
        { return false; }

        constexpr auto RevisionRange = static_cast<int64>(MAX_int32);
        auto ForwardDistance = static_cast<int64>(InCandidate) - InExisting;
        if (ForwardDistance <= 0)
        { ForwardDistance += RevisionRange; }
        return ForwardDistance < (RevisionRange / 2);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        AddDeferredLatest(
            TArray<FNav_DeferredRequest>& InOutQueue,
            const FCk_Handle&             InHandle,
            const FCk_Request_Nav_FindPath& InRequest,
            double                        InDeferredAt)
        -> void
    {
        const auto Revision = InRequest.Get_RequestRevision();
        if (Revision != 0)
        {
            for (auto Index = InOutQueue.Num() - 1; Index >= 0; --Index)
            {
                auto& Existing = InOutQueue[Index];
                if (Existing.Handle != InHandle
                    || Existing.Request.Get_RequestRevision() == 0)
                { continue; }

                if (NOT IsNewerRevision(Revision, Existing.Request.Get_RequestRevision()))
                {
                    InRequest.TryFireCompletion(
                        InHandle, ECk_Request_OperationResult::Failed_Cancelled);
                    return;
                }

                Existing.Request.TryFireCompletion(
                    Existing.Handle, ECk_Request_OperationResult::Failed_Cancelled);
                InOutQueue.RemoveAt(Index, EAllowShrinking::No);
            }
        }

        InOutQueue.Add({InHandle, InRequest, InDeferredAt});
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        PurgeDeferredRequestsFor(
            FCk_Handle& InHandle)
        -> void
    {
        if (ck::Is_NOT_Valid(InHandle))
        { return; }

        auto WorldEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InHandle);
        if (ck::Is_NOT_Valid(WorldEntity, ck::IsValid_Policy_IncludePendingKill{})
            || NOT WorldEntity.Has<FFragment_Nav_DeferredRequests>())
        { return; }

        auto& Queue = WorldEntity.Get<FFragment_Nav_DeferredRequests>()._Requests;

        // Move this entity's entries OUT of the shared queue before completing ANY of them. A
        // completion delegate is caller code (AngelScript, in practice) and may re-enter the
        // abandon path, which would nest a second purge over the very array this one is still
        // erasing from.
        auto Cancelled = TArray<FCk_Request_Nav_FindPath>{};
        for (auto Index = Queue.Num() - 1; Index >= 0; --Index)
        {
            auto& Entry = Queue[Index];
            if (Entry.Handle != InHandle)
            { continue; }

            Cancelled.Add(Entry.Request);
            Queue.RemoveAt(Index, EAllowShrinking::No);
        }

        for (auto& CancelledRequest : Cancelled)
        { CancelledRequest.TryFireCompletion(InHandle, ECk_Request_OperationResult::Failed_Cancelled); }
    }

    auto
        PurgeInFlightQueriesFor(
            FCk_Handle& InHandle)
        -> void
    {
        if (ck::Is_NOT_Valid(InHandle))
        { return; }

        // Undrained requests owe their callers a completion, and one left in the fragment would
        // drain next tick and answer an episode that no longer exists.
        if (InHandle.Has<FFragment_Nav_Requests>())
        {
            const auto Queued = InHandle.Get<FFragment_Nav_Requests>();
            InHandle.Try_Remove<FFragment_Nav_Requests>();
            ck::request::FireCancelledForPending(InHandle, Queued.Get_Requests());
        }

        PurgeDeferredRequestsFor(InHandle);
    }
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
        const auto WorldEntityIsUsable = ck::IsValid(this->_TransientEntity, ck::IsValid_Policy_IncludePendingKill{})
            && this->_TransientEntity.Has<FFragment_Nav_DeferredRequests>();
        if (WorldEntityIsUsable && this->_TransientEntity.Get<FFragment_Nav_DeferredRequests>().Get_Requests().Num() > 0)
        {
            auto& DeferredRequests = this->_TransientEntity.Get<FFragment_Nav_DeferredRequests>()._Requests;

            SCOPE_CYCLE_COUNTER(STAT_Nav_DeferredDrain);
            SET_DWORD_STAT(STAT_Nav_DeferredQueueDepth, DeferredRequests.Num());

            const auto Now = FPlatformTime::Seconds();
            const auto MaxDeferralSec = static_cast<double>(ck_nav_processor::CVarMaxDeferralSeconds.GetValueOnGameThread());

            for (auto i = DeferredRequests.Num() - 1; i >= 0; --i)
            {
                auto& Entry = DeferredRequests[i];
                if (ck::Is_NOT_Valid(Entry.Handle))
                {
                    // The owner died while the request sat in this world's deferral queue — it
                    // never reaches FProcessor_Nav_CancelPendingRequests (the request already left
                    // FFragment_Nav_Requests), so this is the only site that can honor the
                    // fire-exactly-once completion guarantee for it.
                    Entry.Request.TryFireCompletion(Entry.Handle, ECk_Request_OperationResult::Failed_Cancelled);
                    DeferredRequests.RemoveAt(i, EAllowShrinking::No);
                    continue;
                }

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
                        DeferredRequests.RemoveAt(i, EAllowShrinking::No);
                        // Request already carries whatever completion delegate the original caller
                        // bound (mutable, copied along with the struct) — {} here does not drop it.
                        UCk_Utils_Nav_UE::Request_FindPath(Handle, Request, {});
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
                        if (ck_nav_processor::IsSupersededByAuthoritativeRevision(
                            Entry.Request.Get_RequestRevision(),
                            Result.Get_RequestRevision()))
                        {
                            Entry.Request.TryFireCompletion(
                                Handle, ECk_Request_OperationResult::Failed_Cancelled);
                            DeferredRequests.RemoveAt(i, EAllowShrinking::No);
                            ++DrainActions;
                            continue;
                        }
                        Result._Status = ECk_Nav_PathStatus::Failed;
                        Result._RequestRevision = Entry.Request.Get_RequestRevision();
                        Result._Diagnostics._LastFailReason = ECk_Nav_PathFailReason::NoNavData;
                        UUtils_Signal_Nav_OnPathFailed::Broadcast(Handle, ck::MakePayload(Handle));
                        ck::nav::Warning(
                            TEXT("FindPath on [{}] timed out after {}s deferred — failing with NoNavData. ")
                            TEXT("Check that a NavMeshBoundsVolume covers the agent's start position and ")
                            TEXT("that the level has static walkable geometry inside the volume."),
                            Handle, MaxDeferralSec);
                    }
                    Entry.Request.TryFireCompletion(Handle, ECk_Request_OperationResult::Failed);
                    DeferredRequests.RemoveAt(i, EAllowShrinking::No);
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

        auto LatestRequestRevision = int32{0};
        for (const auto& Variant : InRequests.Get_Requests())
        {
            std::visit([&](const auto& InFindPath)
            {
                const auto Revision = InFindPath.Get_RequestRevision();
                if (Revision != 0
                    && (LatestRequestRevision == 0
                    || nav::IsNewerRevision(Revision, LatestRequestRevision)))
                { LatestRequestRevision = Revision; }
            }, Variant);
        }

        if (ck_nav_processor::IsSupersededByAuthoritativeRevision(
            LatestRequestRevision, InResult.Get_RequestRevision()))
        {
            InHandle.CopyAndRemove(InRequests, [&](const auto& InSnapshot)
            {
                request::FireCancelledForPending(InHandle, InSnapshot.Get_Requests());
            });
            return;
        }

        if (NOT UCk_Utils_Net_UE::Get_HasAuthority(InHandle))
        {
            InResult._Status = ECk_Nav_PathStatus::Failed;
            InResult._RequestRevision = LatestRequestRevision;
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
            InResult._RequestRevision = LatestRequestRevision;
            InResult._Diagnostics._LastFailReason = ECk_Nav_PathFailReason::NoNavSystem;
            InHandle.Try_Remove<FFragment_Nav_Requests>();
            return;
        }

        auto* NavSys = UNavigationSystemV1::GetCurrent(World);
        if (NavSys == nullptr)
        {
            InResult._Status = ECk_Nav_PathStatus::Failed;
            InResult._RequestRevision = LatestRequestRevision;
            InResult._Diagnostics._LastFailReason = ECk_Nav_PathFailReason::NoNavSystem;
            InHandle.Try_Remove<FFragment_Nav_Requests>();
            return;
        }

        auto* NavData = Cast<ARecastNavMesh>(NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate));

        auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle);
        if (ck::Is_NOT_Valid(TransformHandle))
        {
            InResult._Status = ECk_Nav_PathStatus::Failed;
            InResult._RequestRevision = LatestRequestRevision;
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
            const auto NowSec = FPlatformTime::Seconds();

            // Re-parking Pending MUST restamp the age. A prior terminal (the pending watchdog's
            // timeout) zeroes _PendingSinceSeconds, and a zero age is the watchdog's "not
            // measurable, leave it alone" early-out — so a bare status write here would re-park an
            // episode that can never be timed out again, which is the silent wedge this whole
            // change exists to remove. Every write of Pending carries its clock.
            InResult._Status = ECk_Nav_PathStatus::Pending;
            InResult._PendingSinceSeconds = NowSec;

            auto WorldEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InHandle);
            auto& DeferredRequests = WorldEntity.AddOrGet<FFragment_Nav_DeferredRequests>()._Requests;

            InHandle.CopyAndRemove(InRequests, [&](const FFragment_Nav_Requests& InSnapshot)
            {
                for (const auto& Variant : InSnapshot._Requests)
                {
                    std::visit([&](const auto& InFindPath)
                    {
                        nav::AddDeferredLatest(DeferredRequests, InHandle, InFindPath, NowSec);
                    }, Variant);
                }
            });
            ck::nav::Verbose(TEXT("FindPath on [{}] deferred — start point not yet bakeable (queue depth now {})"),
                InHandle, DeferredRequests.Num());
            return;
        }

        // CopyAndRemove snapshots AND removes the fragment, so Utils' next AddOrGet re-arms the
        // MarkedDirtyBy event — without the removal only the first request ever would drain.
        InHandle.CopyAndRemove(InRequests, [&](const FFragment_Nav_Requests& InSnapshot)
        {
            ck::algo::ForEachRequest(InSnapshot._Requests, ck::Visitor(
                [&](const auto& InFindPath) -> void
                {
                    auto Result = ECk_Request_OperationResult::Failed;
                    const auto Guard = MakeCompletionGuard(InFindPath, InHandle, Result);

                    // A deferred request can be re-enqueued in the same pump
                    // that a newer policy/goal request reaches this fragment.
                    // Both would otherwise write the one shared PathResult;
                    // whichever happened to run last could erase the current
                    // result and leave its consumer Pending forever.
                    const auto RequestRevision = InFindPath.Get_RequestRevision();
                    if (RequestRevision != 0
                        && LatestRequestRevision != 0
                        && RequestRevision != LatestRequestRevision)
                    {
                        Result = ECk_Request_OperationResult::Failed_Cancelled;
                        return;
                    }

                    const auto FilterTag = InFindPath.Get_QueryFilterOverride().IsValid()
                        ? InFindPath.Get_QueryFilterOverride()
                        : InFindPath.Get_QueryFilter();

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
                        FilterTag,
                        /*InCornerOffsetDistance*/ 0.0f,
                        InFindPath.Get_QueryFilterOverlay());
                    InResult._RequestRevision = InFindPath.Get_RequestRevision();

                    const auto& Diagnostics = InResult.Get_Diagnostics();
                    const auto FilterName = FilterTag.IsValid()
                        ? FilterTag.ToString()
                        : FString{TEXT("NavDataDefault")};
                    ck::nav::Verbose(
                        TEXT("[PNDiag] FindPathSync on [{}]: raw [{}] -> [{}], "
                             "startOverride [{}], allowPartial [{}], filterTag [{}], filter [{}], "
                             "projected [{}:{}] -> [{}:{}], status [{}], waypoints [{}], "
                             "reason [{}], queryMs [{}]"),
                        InHandle,
                        QueryStart,
                        InFindPath.Get_TargetLocation(),
                        InFindPath.Get_StartOverride() == ECk_EnableDisable::Enable,
                        InFindPath.Get_AllowPartialPath(),
                        InFindPath.Get_QueryFilter().ToString(),
                        FilterName,
                        Diagnostics.Get_StartProjected(),
                        Diagnostics.Get_LastProjectedStart(),
                        Diagnostics.Get_EndProjected(),
                        Diagnostics.Get_LastProjectedEnd(),
                        InResult._Status,
                        InResult._Waypoints.Num(),
                        Diagnostics.Get_LastFailReason(),
                        Diagnostics.Get_LastQueryDurationMs());

                    if (bSucceeded)
                    {
                        UUtils_Signal_Nav_OnPathReady::Broadcast(
                            InHandle, ck::MakePayload(InHandle, InResult));
                        Result = ECk_Request_OperationResult::Succeeded;
                    }
                    else
                    {
                        UUtils_Signal_Nav_OnPathFailed::Broadcast(
                            InHandle, ck::MakePayload(InHandle));
                        // Result stays Failed — genuine pathfinding failure.
                    }
                }), ck::policy::DontResetContainer{});
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Nav_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Nav_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }
}

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Nav_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Nav_CancelPendingRequests);

// --------------------------------------------------------------------------------------------------------------------
