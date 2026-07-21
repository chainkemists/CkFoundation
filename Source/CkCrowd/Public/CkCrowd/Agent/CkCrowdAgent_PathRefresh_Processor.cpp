#include "CkCrowdAgent_PathRefresh_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkNavigation/Nav/CkNav_Algorithm.h"
#include "CkNavigation/Utils/CkNav_Utils.h"

#include "CkPathNetwork/Network/CkPathNetwork_Utils.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Agent/CkCrowdAgent_NavArea.h"
#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

#include <NavigationSystem.h>
#include <NavMesh/RecastNavMesh.h>

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_PathRefresh);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::PathRefresh"), STAT_CkCrowd_PathRefreshProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_PathRefresh::
        DoTick(FCk_Time InDeltaT)
        -> void
    {
        _SettledDiscs.Reset();
        _MaxEligibleSerial = 0;

        const auto* Settings = UCk_Utils_Crowd_Settings_UE::Get();
        if (IsValid(Settings) &&
            Settings->Get_PathRefreshMode() == ECk_CrowdPathRefreshMode::Enabled &&
            Settings->Get_StationaryMarkupMode() == ECk_CrowdStationaryMarkupMode::Enabled)
        {
            auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(this->_TransientEntity);
            auto* NavSys = IsValid(World) ? UNavigationSystemV1::GetCurrent(World) : nullptr;
            auto* NavMesh = (NavSys != nullptr)
                ? Cast<ARecastNavMesh>(NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate))
                : nullptr;
            const auto CrowdAreaID = (NavMesh != nullptr)
                ? NavMesh->GetAreaID(UCk_NavArea_CrowdAgent::StaticClass())
                : INDEX_NONE;

            if (NavMesh != nullptr && CrowdAreaID != INDEX_NONE)
            {
                const auto SettleSeconds = Settings->Get_PathRefreshMarkupSettleSeconds();

                this->_TransientEntity.View<FFragment_CrowdAgent_NavMarkup>().ForEach(
                    [&](FCk_Entity InEntity, FFragment_CrowdAgent_NavMarkup& InMarkup)
                {
                    if (NOT InMarkup.Get_Markup().IsValid())
                    { return; }

                    // Cheap pre-filter before the poly query below.
                    if (InMarkup.Get_SecondsSincePaint() < SettleSeconds)
                    { return; }

                    // A disc's navmesh tiles rebuild asynchronously and the latency is unbounded
                    // under churn — a re-path issued before the rebake returns the same straight
                    // path and burns the one-shot serial. Eligibility is therefore GROUND TRUTH,
                    // not a timer: the disc counts only once the mesh actually reports the crowd
                    // cost area at its centre. Confirmed once per paint, then cached.
                    if (NOT InMarkup.Get_ConfirmedOnMesh())
                    {
                        // Vertical extent = the painted box's — the disc centre rides at capsule
                        // height, and a shorter probe misses the floor polys the box marked.
                        const auto Extent = FVector{InMarkup.Get_MarkupRadiusUu(), InMarkup.Get_MarkupRadiusUu(), InMarkup.Get_MarkupVerticalHalfExtentUu()};
                        const auto PolyRef = NavMesh->FindNearestPoly(InMarkup.Get_MarkupLocation(), Extent);
                        if (PolyRef == INVALID_NAVNODEREF ||
                            static_cast<int32>(NavMesh->GetPolyAreaID(PolyRef)) != CrowdAreaID)
                        { return; }

                        InMarkup._ConfirmedOnMesh = true;
                    }

                    _SettledDiscs.Add(FSettledDisc{
                        InEntity,
                        InMarkup.Get_MarkupLocation(),
                        InMarkup.Get_MarkupRadiusUu(),
                        InMarkup.Get_PaintSerial()});
                    _MaxEligibleSerial = FMath::Max(_MaxEligibleSerial, InMarkup.Get_PaintSerial());
                });
            }
        }

        TProcessor::DoTick(InDeltaT);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_PathRefresh::
        Get_EscapedQueryStart(
            FCk_Handle InAnyWorldHandle,
            FCk_Entity InSelfEntity,
            const FVector& InSelfLocation,
            const FVector& InGoal,
            float InAgentRadius)
        -> TOptional<FVector>
    {
        const auto* Settings = UCk_Utils_Crowd_Settings_UE::Get();
        if (NOT IsValid(Settings) ||
            Settings->Get_PathRefreshMode() != ECk_CrowdPathRefreshMode::Enabled ||
            Settings->Get_StationaryMarkupMode() != ECk_CrowdStationaryMarkupMode::Enabled)
        {
            ck::crowd::Verbose(TEXT("EscapedQueryStart: disabled by settings (self at {})"), InSelfLocation);
            return {};
        }

        auto Centers = TArray<FVector, TInlineAllocator<32>>{};
        auto Radii = TArray<float, TInlineAllocator<32>>{};
        InAnyWorldHandle.View<FFragment_CrowdAgent_NavMarkup>().ForEach(
            [&](FCk_Entity InEntity, const FFragment_CrowdAgent_NavMarkup& InMarkup)
        {
            if (InEntity == InSelfEntity)
            { return; }
            // PAINTED is enough here — unlike the re-path trigger (which must wait for the mesh
            // to actually price a disc before spending its one-shot serial), the escape is pure
            // geometry: planning from a pushed-out start is valid the moment the disc exists and
            // merely arrives early when the rebake hasn't landed yet. Gating on ConfirmedOnMesh
            // opened a paint→confirm window where a fresh MoveTo from inside the band planned
            // "through" — and a repaint (e.g. push-apart drift) reopened that window by resetting
            // the flag.
            if (NOT InMarkup.Get_Markup().IsValid())
            { return; }

            Centers.Add(InMarkup.Get_MarkupLocation());
            Radii.Add(InMarkup.Get_MarkupRadiusUu());
        });

        if (Centers.IsEmpty())
        {
            ck::crowd::Verbose(TEXT("EscapedQueryStart: no painted discs in view (self at {})"), InSelfLocation);
            return {};
        }

        const auto IsInsideAny = [&](const FVector& InPoint) -> bool
        {
            for (auto Idx = 0; Idx < Centers.Num(); ++Idx)
            {
                if (FVector::Dist2D(InPoint, Centers[Idx]) <= Radii[Idx])
                { return true; }
            }
            return false;
        };

        if (NOT IsInsideAny(InSelfLocation))
        {
            ck::crowd::Verbose(TEXT("EscapedQueryStart: self at {} not inside any of {} discs (first disc centre {}, r={})"),
                InSelfLocation, Centers.Num(), Centers[0], Radii[0]);
            return {};
        }
        if (IsInsideAny(InGoal))
        {
            ck::crowd::Verbose(TEXT("EscapedQueryStart: goal {} is itself inside a disc — no escape"), InGoal);
            return {};
        }

        // Ray-march out of the disc union along the direction the agent already leans (away from
        // the nearest disc centre) — the shortest way out. NOT a fixed-point pairwise push: for a
        // painted LINE the discs' push-out zones overlap (spacing < 2x required radius), so each
        // pairwise push lands inside the neighbouring disc's zone and the iteration ping-pongs
        // between neighbours until the cap — the escape then gave up on exactly the scenario the
        // tier exists for. Along a fixed ray each disc (convex) is exited at most once, so the
        // march provably terminates within one step per disc.
        constexpr auto MarginUu = 10.0f;

        auto NearestIdx = 0;
        auto NearestDistSq = TNumericLimits<double>::Max();
        for (auto Idx = 0; Idx < Centers.Num(); ++Idx)
        {
            const auto DistSq = FVector::DistSquared2D(InSelfLocation, Centers[Idx]);
            if (DistSq < NearestDistSq)
            {
                NearestDistSq = DistSq;
                NearestIdx = Idx;
            }
        }

        const auto LeanDir = FVector2D{InSelfLocation} - FVector2D{Centers[NearestIdx]};
        const auto RayDir = LeanDir.SizeSquared() > UE_KINDA_SMALL_NUMBER
            ? LeanDir.GetSafeNormal()
            : FVector2D{1.0f, 0.0f};

        auto Point = FVector2D{InSelfLocation};
        for (auto Step = 0; Step <= Centers.Num(); ++Step)
        {
            auto Moved = false;
            for (auto Idx = 0; Idx < Centers.Num(); ++Idx)
            {
                const auto Required = Radii[Idx] + InAgentRadius + MarginUu;
                const auto Centre2D = FVector2D{Centers[Idx]};
                const auto ToCentre = Centre2D - Point;
                if (ToCentre.SizeSquared() >= Required * Required)
                { continue; }

                // Inside this disc's push-out zone: advance to the ray's FAR intersection with
                // the zone circle. |Point + t*Dir - C| = Required, larger root — guaranteed real
                // because the point is inside.
                const auto B = FVector2D::DotProduct(ToCentre, RayDir);
                const auto Discriminant = B * B + (Required * Required - ToCentre.SizeSquared());
                const auto ExitT = B + FMath::Sqrt(Discriminant);
                Point += RayDir * (ExitT + UE_KINDA_SMALL_NUMBER);
                Moved = true;
            }
            if (NOT Moved)
            { break; }
        }

        const auto Escape = FVector{Point.X, Point.Y, InSelfLocation.Z};

        // Unreachable by construction (forward marching exits each convex disc at most once),
        // kept as a safety net: planning from the real location is the status quo, not a failure.
        if (IsInsideAny(Escape))
        {
            ck::crowd::Verbose(TEXT("EscapedQueryStart: ray-march from {} did not exit the union (last try {})"),
                InSelfLocation, Escape);
            return {};
        }

        ck::crowd::Verbose(TEXT("EscapedQueryStart: self {} escapes to {} ({} discs)"),
            InSelfLocation, Escape, Centers.Num());
        return Escape;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_PathRefresh::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_Nav_PathResult& InPathResult,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_BlockDetect& InBlockDetect) const
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_PathRefreshProc);

        // The common frame: this path already covers every settled disc (or there are none).
        if (InPathFollow.Get_PathSerial() >= _MaxEligibleSerial)
        { return; }

        if (NOT UCk_Utils_Net_UE::Get_HasAuthority(InHandle))
        { return; }

        const auto& Waypoints = InPathResult.Get_Waypoints();
        if (Waypoints.Num() == 0)
        { return; }

        const auto SelfLoc = InTransform.Get_Transform().GetLocation();
        const auto Goal = InPathFollow.Get_ActiveGoal();
        const auto GoalExemptionPad = InPathFollow.Get_ActiveArrivalRadius() + InParams.Get_Radius();
        const auto PathSerial = InPathFollow.Get_PathSerial();
        const auto SelfEntity = InHandle.Get_Entity();
        const auto FirstIdx = FMath::Clamp(InPathFollow.Get_WaypointIndex(), 0, Waypoints.Num() - 1);

        auto Hit = false;
        for (const auto& Disc : _SettledDiscs)
        {
            // Only a disc painted AFTER this path can invalidate it — the planner already saw
            // (and priced) every older disc.
            if (Disc._PaintSerial <= PathSerial)
            { continue; }

            if (Disc._Owner == SelfEntity)
            { continue; }

            // Joining a queue legitimately ENDS beside standing agents — a disc adjacent to the
            // agent's own goal must not bounce it into a re-path whose result clips the same disc.
            if (FVector::Dist2D(Disc._Center, Goal) <= Disc._Radius + GoalExemptionPad)
            { continue; }

            // Only the REMAINING path matters: current position, then the un-retired waypoints.
            const auto DiscCenter2D = FVector2D{Disc._Center};
            auto SegStart = FVector2D{SelfLoc};
            for (auto Idx = FirstIdx; Idx < Waypoints.Num(); ++Idx)
            {
                const auto SegEnd = FVector2D{Waypoints[Idx]};
                const auto Closest = FMath::ClosestPointOnSegment2D(DiscCenter2D, SegStart, SegEnd);
                if (FVector2D::Distance(Closest, DiscCenter2D) <= Disc._Radius)
                {
                    Hit = true;
                    break;
                }
                SegStart = SegEnd;
            }

            if (Hit)
            { break; }
        }

        // Path and discs are both static, so a clean scan against the current set stays clean —
        // fast-forward the serial either way; a re-path's install re-stamps it anyway.
        InPathFollow._PathSerial = _MaxEligibleSerial;

        if (NOT Hit)
        { return; }

        // Re-path at the same goal — BlockedRecheck's resume dance, minus the blocked bookkeeping
        // (this agent is not blocked; its plan is just stale).
        auto NonConstHandle = InHandle;
        NonConstHandle.Try_Remove<FTag_CrowdAgent_Walking>();
        NonConstHandle.AddOrGet<FTag_CrowdAgent_PathPending>();

        InPathFollow._WaypointIndex = 0;

        // Fresh corridor, fresh block-detection window — a feet-sample ring straddling the
        // re-path would smear two corridors into one centroid test.
        InBlockDetect._FeetSamples.Reset();
        InBlockDetect._NextSampleIdx = 0;
        InBlockDetect._SampleAccumulatorSec = 0.0f;

        if (UCk_Utils_PathNetworkFollower_UE::Has(NonConstHandle))
        {
            FCk_Nav_Algorithm::MarkPathPending(NonConstHandle);
            NonConstHandle.Try_Remove<FFragment_CrowdAgent_InstalledRoute>();

            auto Follower = UCk_Utils_PathNetworkFollower_UE::CastChecked(NonConstHandle);
            UCk_Utils_PathNetworkFollower_UE::Request_FindRoute(Follower,
                FCk_Request_PathNetworkFollower_FindRoute{Goal});
        }
        else
        {
            // Park the slot at Pending so OnPathResolved can't consume the stale Ready result —
            // which is the exact path that crosses the fresh markup, defeating this re-path.
            FCk_Nav_Algorithm::MarkPathPending(NonConstHandle);

            auto Request = FCk_Request_Nav_FindPath{Goal};
            Request.Set_QueryFilter(InParams.Get_NavQueryFilter());

            const auto Escaped = Get_EscapedQueryStart(NonConstHandle, InHandle.Get_Entity(), SelfLoc, Goal, InParams.Get_Radius());
            if (Escaped.IsSet())
            {
                Request.Set_StartOverride(ECk_EnableDisable::Enable)
                       .Set_StartOverrideLocation(*Escaped);
            }

            UCk_Utils_Nav_UE::Request_FindPath(NonConstHandle, Request);
        }

        ck::crowd::Verbose(TEXT("CrowdAgent [{}] remaining path crosses fresh stationary markup — re-pathing to {}"),
            InHandle, Goal);
    }
}

// --------------------------------------------------------------------------------------------------------------------
