#include "CkCrowdAgent_PathRefresh_Processor.h"

#include "CkCrowd/Agent/CkCrowdAgent_HandleRequests_Processor.h"
#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_Fragment.h"
#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkNavigation/Nav/CkNav_Algorithm.h"
#include "CkNavigation/NavSurface/CkNavSurface_Utils.h"
#include "CkNavigation/NavSurface/Recast/CkNavSurface_RecastAdapter.h"
#include "CkNavigation/Utils/CkNav_Utils.h"
#include "CkNavigation/Settings/CkNav_ProjectSettings.h"

#include "CkPathNetwork/Network/CkPathNetwork_Utils.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_NavGameplayTags.h"
#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

#include <NavigationSystem.h>
#include <NavMesh/RecastNavMesh.h>

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_PathRefresh);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::PathRefresh"), STAT_CkCrowd_PathRefreshProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_agent_path_refresh
{
    // Process-wide (not per-world): only monotonicity matters. A path and a disc are compared only
    // within one world, while sharing the counter prevents serial reuse across world transitions.
    static auto GConfirmationSerial = uint64{0};

    // A body held at contact by push-apart rests at the disc edge minus the push-apart slop, so a
    // boundary-exact test reads it as standing in the band. Touching a band from outside is being
    // blocked by it, not standing in it.
    constexpr auto kBandContactToleranceUu = 1.0f;

    auto Is_InsidePaintedBand(float InDistance2D, float InRadius) -> bool
    {
        return InDistance2D < InRadius - kBandContactToleranceUu;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_PathRefresh::
        DoTick(FCk_Time InDeltaT)
        -> void
    {
        _SettledDiscs.Reset();
        _SettledVolumes.Reset();
        _SettledRetirements.Reset();
        _MaxConfirmationSerial = 0;

        const auto* Settings = UCk_Utils_Crowd_Settings_UE::Get();
        if (IsValid(Settings) &&
            Settings->Get_PathRefreshMode() == ECk_CrowdPathRefreshMode::Enabled &&
            Settings->Get_StationaryMarkupMode() == ECk_CrowdStationaryMarkupMode::Enabled)
        {
            auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(this->_TransientEntity);
            const auto ProviderHealth = UCk_Utils_NavSurface_UE::Get_ProviderHealth(World);
            const auto SurfaceHasData =
                ProviderHealth != ECk_NavSurface_ProviderHealth::NoData &&
                ProviderHealth != ECk_NavSurface_ProviderHealth::Error;

            if (SurfaceHasData)
            {
                const auto SettleSeconds = Settings->Get_PathRefreshMarkupSettleSeconds();

                this->_TransientEntity.View<FFragment_CrowdAgent_NavMarkup>().ForEach(
                    [&](FCk_Entity InEntity, FFragment_CrowdAgent_NavMarkup& InMarkup)
                {
                    if (NOT InMarkup.Get_Markup().IsValid())
                    { return; }

                    if (InMarkup.Get_SecondsSincePaint() < SettleSeconds)
                    { return; }

                    // Tile rebake latency is unbounded under churn, so eligibility is ground truth
                    // and not a timer: the mesh must actually report the cost area at the centre.
                    if (NOT InMarkup.Get_ConfirmedOnMesh())
                    {
                        // Vertical extent is the painted box's — the disc centre rides at capsule
                        // height, and a shorter probe misses the floor polys the box marked.
                        const auto Extent = FVector{InMarkup.Get_MarkupRadiusUu(), InMarkup.Get_MarkupRadiusUu(), InMarkup.Get_MarkupVerticalHalfExtentUu()};
                        if (NOT ck::nav_surface_recast::Get_IsAreaLiveAt(
                            World, TAG_Nav_Area_Crowd_Agent, InMarkup.Get_MarkupLocation(), Extent))
                        { return; }

                        InMarkup._ConfirmationSerial = IssueConfirmationSerial();
                        InMarkup._ConfirmedOnMesh = true;

                        auto ConfirmedAgent = ck::MakeHandle(InEntity, _TransientEntity);
                        ConfirmedAgent.AddOrGet<FTag_CrowdAgent_StationaryMarkupConfirmed>();
                    }

                    _SettledDiscs.Add(FSettledDisc{
                        InEntity,
                        InMarkup.Get_MarkupLocation(),
                        InMarkup.Get_MarkupRadiusUu(),
                        InMarkup.Get_ConfirmationSerial()});
                    _MaxConfirmationSerial =
                        FMath::Max(
                            _MaxConfirmationSerial,
                            InMarkup.Get_ConfirmationSerial());
                });
            }
        }

        if (IsValid(Settings) &&
            Settings->Get_PathRefreshMode() == ECk_CrowdPathRefreshMode::Enabled)
        {
            this->_TransientEntity.View<FFragment_CrowdAvoidanceVolume_ProbeRef>().ForEach(
                [&](FCk_Entity InEntity, const FFragment_CrowdAvoidanceVolume_ProbeRef& InRuntime)
            {
                if (NOT InRuntime.Get_Markup().IsValid() ||
                    NOT InRuntime.Get_ConfirmedOnMesh() ||
                    NOT InRuntime.Get_AuthoredObb().IsFiniteAndPositive() ||
                    InRuntime.Get_ConfirmationSerial() == 0)
                { return; }

                _SettledVolumes.Add(FSettledVolume{
                    InEntity,
                    InRuntime.Get_AuthoredObb(),
                    InRuntime.Get_PaintedObb(),
                    InRuntime.Get_ConfirmationSerial()});
                _MaxConfirmationSerial = FMath::Max(
                    _MaxConfirmationSerial,
                    InRuntime.Get_ConfirmationSerial());
            });
        }

        auto Transient = this->_TransientEntity;
        if (ck::IsValid(Transient) &&
            Transient.Has<FFragment_CrowdAvoidanceVolume_Retirements>())
        {
            auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(Transient);

            // Zero is the surface's "no observer bound" answer — the first successful bind is what
            // advances the revision off zero, so a nonzero read IS the observability proof.
            const auto NavigationRevision = UCk_Utils_NavSurface_UE::Get_SurfaceRevision(World);
            const auto CanObserveRevision = NavigationRevision != 0;

            if (CanObserveRevision && NOT UCk_Utils_NavSurface_UE::Get_IsBuildInProgress(World))
            {
                auto& Records = Transient.Get<FFragment_CrowdAvoidanceVolume_Retirements>()._Records;
                for (auto RecordIndex = Records.Num() - 1; RecordIndex >= 0; --RecordIndex)
                {
                    const auto& Record = Records[RecordIndex];
                    const auto RecordIsValid = Record._PhysicalObb.IsFiniteAndPositive() &&
                        Record._PaintedObb.IsFiniteAndPositive();
                    CK_ENSURE_IF_NOT(RecordIsValid,
                        TEXT("CrowdAvoidanceVolume retirement record contains invalid OBB geometry"))
                    {
                        Records.RemoveAtSwap(RecordIndex);
                        continue;
                    }

                    if (NavigationRevision == static_cast<int64>(Record._NavigationRevisionAtUnregister))
                    { continue; }

                    const auto ConfirmationSerial = IssueConfirmationSerial();
                    _SettledRetirements.Add(FSettledRetirement{
                        Record._PhysicalObb,
                        Record._PaintedObb,
                        ConfirmationSerial});
                    _MaxConfirmationSerial = FMath::Max(
                        _MaxConfirmationSerial, ConfirmationSerial);
                    Records.RemoveAtSwap(RecordIndex);
                }
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
        const auto InputsAreValid =
            ck::IsValid(InAnyWorldHandle) &&
            NOT InSelfLocation.ContainsNaN() &&
            NOT InGoal.ContainsNaN() &&
            FMath::IsFinite(InAgentRadius) &&
            InAgentRadius >= 0.0f;
        CK_ENSURE_IF_NOT(
            InputsAreValid,
            TEXT("Invalid escaped-query-start inputs "
                 "(handle [{}], self [{}], location [{}], goal [{}], radius [{}])"),
            InAnyWorldHandle,
            InSelfEntity,
            InSelfLocation,
            InGoal,
            InAgentRadius)
        { return {}; }

        const auto* Settings = UCk_Utils_Crowd_Settings_UE::Get();
        const auto UseStationaryMarkup = IsValid(Settings) &&
            Settings->Get_PathRefreshMode() == ECk_CrowdPathRefreshMode::Enabled &&
            Settings->Get_StationaryMarkupMode() == ECk_CrowdStationaryMarkupMode::Enabled;

        auto Centers = TArray<FVector, TInlineAllocator<32>>{};
        auto Radii = TArray<float, TInlineAllocator<32>>{};
        if (UseStationaryMarkup)
        {
            InAnyWorldHandle.View<FFragment_CrowdAgent_NavMarkup>().ForEach(
                [&](FCk_Entity InEntity, const FFragment_CrowdAgent_NavMarkup& InMarkup)
            {
                if (InEntity == InSelfEntity)
                { return; }
                // PAINTED is enough here, deliberately NOT ConfirmedOnMesh: the escape is pure
                // geometry and is valid the moment the disc exists.
                const auto& MarkupLocation = InMarkup.Get_MarkupLocation();
                const auto MarkupRadius = InMarkup.Get_MarkupRadiusUu();
                if (NOT InMarkup.Get_Markup().IsValid() ||
                    MarkupLocation.ContainsNaN() ||
                    NOT FMath::IsFinite(MarkupRadius) ||
                    MarkupRadius <= 0.0f)
                { return; }

                Centers.Add(MarkupLocation);
                Radii.Add(MarkupRadius);
            });
        }

        auto Volumes = TArray<crowd_avoidance_volume::FCk_Obb, TInlineAllocator<16>>{};
        InAnyWorldHandle.View<FFragment_CrowdAvoidanceVolume_ProbeRef>().ForEach(
            [&](FCk_Entity InEntity, const FFragment_CrowdAvoidanceVolume_ProbeRef& InRuntime)
        {
            const auto EffectiveObb = crowd_avoidance_volume::MakeEffectiveAgentObb(
                InRuntime.Get_AuthoredObb(), InRuntime.Get_PaintedObb(), InAgentRadius);
            if (InRuntime.Get_Markup().IsValid() && EffectiveObb.IsFiniteAndPositive())
            { Volumes.Add(EffectiveObb); }
        });

        if (Centers.IsEmpty() && Volumes.IsEmpty())
        {
            ck::crowd::Verbose(TEXT("EscapedQueryStart: no painted markup in view (self at {})"), InSelfLocation);
            return {};
        }

        const auto IsInsideAny = [&](const FVector& InPoint) -> bool
        {
            for (auto Idx = 0; Idx < Centers.Num(); ++Idx)
            {
                if (ck_crowd_agent_path_refresh::Is_InsidePaintedBand(
                        static_cast<float>(FVector::Dist2D(InPoint, Centers[Idx])), Radii[Idx]))
                { return true; }
            }
            for (const auto& Volume : Volumes)
            {
                if (crowd_avoidance_volume::ContainsPoint(
                    Volume, InPoint))
                { return true; }
            }
            return false;
        };

        if (NOT IsInsideAny(InSelfLocation))
        {
            ck::crowd::Verbose(TEXT("EscapedQueryStart: self at {} not inside painted markup"), InSelfLocation);
            return {};
        }
        if (IsInsideAny(InGoal))
        {
            ck::crowd::Verbose(TEXT("EscapedQueryStart: goal {} is itself inside a disc — no escape"), InGoal);
            return {};
        }

        // Ray-march, NOT a pairwise push: along a fixed ray each convex region is exited at most
        // once, so the march provably terminates within one step per region.
        constexpr auto MarginUu = 10.0f;

        auto CandidateDirections = TArray<FVector2D, TInlineAllocator<24>>{};
        const auto AddCandidateDirection = [&](const FVector2D& InDirection)
        {
            const auto Direction = InDirection.GetSafeNormal();
            if (Direction.IsNearlyZero())
            { return; }

            constexpr auto DirectionMergeDistanceSq = 0.0001f;
            for (const auto& Existing : CandidateDirections)
            {
                if ((Existing - Direction).SizeSquared() <= DirectionMergeDistanceSq)
                { return; }
            }
            CandidateDirections.Add(Direction);
        };

        // A nearest-centre radial can point through the rest of an overlapping line. Use a fixed
        // angular fan so work remains bounded as crowds grow, then keep the shortest candidate ray
        // that exits the whole expanded union. Goal-relative rays reduce quantization in the most
        // useful directions without making candidate count depend on disc count.
        const auto Self2D = FVector2D{InSelfLocation};
        constexpr auto RadialSampleCount = 16;
        for (auto SampleIndex = 0; SampleIndex < RadialSampleCount; ++SampleIndex)
        {
            const auto Angle =
                2.0f * UE_PI * static_cast<float>(SampleIndex) /
                static_cast<float>(RadialSampleCount);
            AddCandidateDirection(FVector2D{FMath::Cos(Angle), FMath::Sin(Angle)});
        }

        const auto GoalDirection = FVector2D{InGoal} - Self2D;
        AddCandidateDirection(GoalDirection);
        AddCandidateDirection(-GoalDirection);
        AddCandidateDirection(FVector2D{GoalDirection.Y, -GoalDirection.X});
        AddCandidateDirection(FVector2D{-GoalDirection.Y, GoalDirection.X});
        for (const auto& Volume : Volumes)
        {
            const auto Escape = crowd_avoidance_volume::FindNearestFaceEscapePoint(
                Volume, InSelfLocation, MarginUu);
            if (Escape.IsSet())
            { AddCandidateDirection(FVector2D{Escape.GetValue() - InSelfLocation}); }
        }

        const auto IsInsideExpandedUnion = [&](const FVector2D& InPoint) -> bool
        {
            for (auto Idx = 0; Idx < Centers.Num(); ++Idx)
            {
                const auto Required = Radii[Idx] + InAgentRadius + MarginUu;
                if ((InPoint - FVector2D{Centers[Idx]}).SizeSquared() < Required * Required)
                { return true; }
            }
            const auto WorldPoint = FVector{InPoint.X, InPoint.Y, InSelfLocation.Z};
            for (const auto& Volume : Volumes)
            {
                if (crowd_avoidance_volume::ContainsPoint(
                    Volume.ExpandedXY(MarginUu), WorldPoint))
                { return true; }
            }
            return false;
        };

        auto BestPoint = TOptional<FVector2D>{};
        auto BestDistanceSq = TNumericLimits<double>::Max();
        for (const auto& RayDir : CandidateDirections)
        {
            auto Point = FVector{InSelfLocation};
            for (auto Step = 0; Step <= Centers.Num() + Volumes.Num(); ++Step)
            {
                auto Moved = false;
                for (auto Idx = 0; Idx < Centers.Num(); ++Idx)
                {
                    const auto Required = Radii[Idx] + InAgentRadius + MarginUu;
                    const auto Centre2D = FVector2D{Centers[Idx]};
                    const auto ToCentre = Centre2D - FVector2D{Point};
                    if (ToCentre.SizeSquared() >= Required * Required)
                    { continue; }

                    // Larger root of |Point + t*Dir - Centre| = Required, guaranteed real because
                    // the point is inside that expanded zone.
                    const auto ProjectionOntoRay = FVector2D::DotProduct(ToCentre, RayDir);
                    const auto Discriminant =
                        ProjectionOntoRay * ProjectionOntoRay +
                        (Required * Required - ToCentre.SizeSquared());
                    const auto ExitT =
                        ProjectionOntoRay + FMath::Sqrt(FMath::Max(0.0f, Discriminant));
                    const auto MovedPoint = FVector2D{Point} + RayDir * (ExitT + UE_KINDA_SMALL_NUMBER);
                    Point.X = MovedPoint.X;
                    Point.Y = MovedPoint.Y;
                    Moved = true;
                }
                for (const auto& Volume : Volumes)
                {
                    if (NOT crowd_avoidance_volume::ContainsPoint(Volume, Point))
                    { continue; }

                    const auto Exit = crowd_avoidance_volume::FindRayExitPoint(
                        Volume, Point, RayDir, MarginUu + UE_KINDA_SMALL_NUMBER);
                    if (NOT Exit.IsSet())
                    { continue; }
                    Point = Exit.GetValue();
                    Moved = true;
                }
                if (NOT Moved)
                { break; }
            }

            if (IsInsideExpandedUnion(FVector2D{Point}))
            { continue; }

            const auto DistanceSq = (FVector2D{Point} - Self2D).SizeSquared();
            if (DistanceSq < BestDistanceSq)
            {
                BestPoint = FVector2D{Point};
                BestDistanceSq = DistanceSq;
            }
        }

        // The fallback (plan from the real location) is the status quo, not a failure.
        if (NOT BestPoint.IsSet())
        {
            ck::crowd::Verbose(
                TEXT("EscapedQueryStart: no candidate ray from {} exited the expanded union"),
                InSelfLocation);
            return {};
        }

        const auto& ChosenPoint = BestPoint.GetValue();
        const auto Escape = FVector{ChosenPoint.X, ChosenPoint.Y, InSelfLocation.Z};

        ck::crowd::Verbose(
            TEXT("EscapedQueryStart: self {} escapes to {} via shortest of {} rays ({} discs)"),
            InSelfLocation,
            Escape,
            CandidateDirections.Num(),
            Centers.Num() + Volumes.Num());
        return Escape;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_PathRefresh::
        Try_BuildStationaryMarkupEscapePath(
            FCk_Handle InAnyWorldHandle,
            FCk_Entity InSelfEntity,
            const FVector& InSelfLocation,
            const FVector& InEscapedLocation,
            const FFragment_CrowdAgent_Params& InParams,
            ECk_CrowdAvoidanceVolume_QueryPhase InVolumeQueryPhase,
            TArray<FVector>& OutWaypoints)
        -> bool
    {
        const auto InputsAreValid =
            ck::IsValid(InAnyWorldHandle) &&
            NOT InSelfLocation.ContainsNaN() &&
            NOT InEscapedLocation.ContainsNaN() &&
            FMath::IsFinite(InParams.Get_Radius()) &&
            InParams.Get_Radius() > 0.0f;
        CK_ENSURE_IF_NOT(
            InputsAreValid,
            TEXT("Invalid stationary-markup escape-path inputs "
                 "(handle [{}], self [{}], start [{}], escaped [{}], radius [{}])"),
            InAnyWorldHandle,
            InSelfEntity,
            InSelfLocation,
            InEscapedLocation,
            InParams.Get_Radius())
        { return false; }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InAnyWorldHandle);
        auto* NavSys = IsValid(World) ? UNavigationSystemV1::GetCurrent(World) : nullptr;
        auto* NavData = NavSys != nullptr
            ? Cast<ARecastNavMesh>(
                NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate))
            : nullptr;
        if (NavSys == nullptr || NavData == nullptr)
        { return false; }

        auto EscapeResult = FCk_Nav_PathResult{};
        const auto FoundEscape = FCk_Nav_Algorithm::FindPathSync(
            *NavSys,
            *NavData,
            InSelfLocation,
            InEscapedLocation,
            /*InAllowPartial*/ false,
            UCk_Utils_Nav_Settings_UE::Get_NavQuerySearchHalfExtent(),
            UCk_Utils_Nav_Settings_UE::Get_NavQueryVerticalHalfExtent(),
            InParams.Get_Radius(),
            EscapeResult,
            InParams.Get_NavQueryFilter(),
            0.0f,
            UCk_Utils_CrowdAvoidanceVolume_UE::Get_NavQueryFilterOverlay(InVolumeQueryPhase));
        if (NOT FoundEscape || EscapeResult.Get_Waypoints().IsEmpty())
        { return false; }

        const auto& ProjectedEscape = EscapeResult.Get_Waypoints().Last();
        auto ProjectedEscapeIsClear = NOT ProjectedEscape.ContainsNaN();
        auto PaintedCenters = TArray<FVector2D, TInlineAllocator<32>>{};
        auto PaintedExpandedRadii = TArray<float, TInlineAllocator<32>>{};
        auto PaintedVolumes = TArray<crowd_avoidance_volume::FCk_Obb, TInlineAllocator<16>>{};
        InAnyWorldHandle.View<FFragment_CrowdAgent_NavMarkup>().ForEach(
            [&](FCk_Entity InEntity, const FFragment_CrowdAgent_NavMarkup& InMarkup)
        {
            if (NOT ProjectedEscapeIsClear ||
                InEntity == InSelfEntity ||
                NOT InMarkup.Get_Markup().IsValid())
            { return; }

            const auto& MarkupLocation = InMarkup.Get_MarkupLocation();
            const auto MarkupRadius = InMarkup.Get_MarkupRadiusUu();
            const auto MarkupGeometryIsValid =
                NOT MarkupLocation.ContainsNaN() &&
                FMath::IsFinite(MarkupRadius) &&
                MarkupRadius > 0.0f;
            if (NOT MarkupGeometryIsValid)
            {
                // A malformed painted obstacle cannot establish that the projected endpoint is
                // safe. Reject the physical prefix and retain the ordinary PathNetwork route.
                ProjectedEscapeIsClear = false;
                return;
            }

            constexpr auto EndpointMarginUu = 1.0f;
            const auto RequiredClearance =
                MarkupRadius +
                InParams.Get_Radius() +
                EndpointMarginUu;
            PaintedCenters.Add(FVector2D{MarkupLocation});
            PaintedExpandedRadii.Add(RequiredClearance);
            if (FVector::DistSquared2D(
                    ProjectedEscape,
                    MarkupLocation) <
                FMath::Square(RequiredClearance))
            {
                ProjectedEscapeIsClear = false;
            }
        });
        InAnyWorldHandle.View<FFragment_CrowdAvoidanceVolume_ProbeRef>().ForEach(
            [&](FCk_Entity InEntity, const FFragment_CrowdAvoidanceVolume_ProbeRef& InRuntime)
        {
            if (NOT ProjectedEscapeIsClear || NOT InRuntime.Get_Markup().IsValid())
            { return; }

            constexpr auto EndpointMarginUu = 1.0f;
            const auto Effective = crowd_avoidance_volume::MakeEffectiveAgentObb(
                InRuntime.Get_AuthoredObb(),
                InRuntime.Get_PaintedObb(),
                InParams.Get_Radius());
            const auto Expanded = Effective.ExpandedXY(EndpointMarginUu);
            if (NOT Expanded.IsFiniteAndPositive())
            {
                ProjectedEscapeIsClear = false;
                return;
            }
            PaintedVolumes.Add(Expanded);
            if (crowd_avoidance_volume::ContainsPoint(Expanded, ProjectedEscape))
            { ProjectedEscapeIsClear = false; }
        });
        if (NOT ProjectedEscapeIsClear)
        {
            ck::crowd::Verbose(
                TEXT("Stationary-markup escape path [{} -> {}] projected back inside the expanded union"),
                InSelfLocation,
                ProjectedEscape);
            return false;
        }

        // The Recast filter treats painted agents as expensive rather than impassable. Starting
        // inside the painted union necessarily traverses its initial component, but once the
        // egress reaches clear ground it must never enter any painted component again. Validate
        // exact disc/segment intervals rather than waypoint positions: a long straight segment
        // can cross a disc while both of its corners remain clear.
        auto HasExitedPaintedUnion = false;
        auto SegmentStart = FVector2D{InSelfLocation};
        auto WorldSegmentStart = InSelfLocation;
        for (const auto& Waypoint : EscapeResult.Get_Waypoints())
        {
            if (Waypoint.ContainsNaN())
            { return false; }

            const auto SegmentEnd = FVector2D{Waypoint};
            const auto Segment = SegmentEnd - SegmentStart;
            const auto SegmentLengthSq = Segment.SizeSquared();
            auto InsideIntervals = TArray<TPair<float, float>, TInlineAllocator<32>>{};
            for (auto DiscIndex = 0; DiscIndex < PaintedCenters.Num(); ++DiscIndex)
            {
                const auto FromCenter = SegmentStart - PaintedCenters[DiscIndex];
                const auto Radius = PaintedExpandedRadii[DiscIndex];
                if (SegmentLengthSq <= UE_KINDA_SMALL_NUMBER)
                {
                    if (FromCenter.SizeSquared() < Radius * Radius)
                    { InsideIntervals.Emplace(0.0f, 1.0f); }
                    continue;
                }

                const auto B = 2.0f * FVector2D::DotProduct(FromCenter, Segment);
                const auto C = FromCenter.SizeSquared() - Radius * Radius;
                const auto Discriminant = B * B - 4.0f * SegmentLengthSq * C;
                if (Discriminant < 0.0f)
                { continue; }

                const auto Root = FMath::Sqrt(FMath::Max(0.0f, Discriminant));
                const auto Denominator = 2.0f * SegmentLengthSq;
                const auto EnterT = FMath::Max(0.0f, (-B - Root) / Denominator);
                const auto ExitT = FMath::Min(1.0f, (-B + Root) / Denominator);
                if (EnterT <= ExitT)
                { InsideIntervals.Emplace(EnterT, ExitT); }
            }
            for (const auto& Volume : PaintedVolumes)
            {
                const auto Interval = crowd_avoidance_volume::GetSegmentInsideInterval(
                    Volume, WorldSegmentStart, Waypoint);
                if (Interval.IsSet())
                { InsideIntervals.Add(Interval.GetValue()); }
            }

            InsideIntervals.Sort(
                [](const TPair<float, float>& InA, const TPair<float, float>& InB)
                { return InA.Key < InB.Key; });

            auto MergedIntervals = TArray<TPair<float, float>, TInlineAllocator<32>>{};
            for (const auto& Interval : InsideIntervals)
            {
                if (MergedIntervals.IsEmpty() ||
                    Interval.Key > MergedIntervals.Last().Value + UE_KINDA_SMALL_NUMBER)
                {
                    MergedIntervals.Add(Interval);
                    continue;
                }
                MergedIntervals.Last().Value =
                    FMath::Max(MergedIntervals.Last().Value, Interval.Value);
            }

            for (const auto& Interval : MergedIntervals)
            {
                if (Interval.Value <= UE_KINDA_SMALL_NUMBER)
                { continue; }

                if (HasExitedPaintedUnion || Interval.Key > UE_KINDA_SMALL_NUMBER)
                {
                    ck::crowd::Verbose(
                        TEXT("Stationary-markup escape path [{} -> {}] re-enters painted markup"),
                        InSelfLocation,
                        ProjectedEscape);
                    return false;
                }

                if (Interval.Value < 1.0f - UE_KINDA_SMALL_NUMBER)
                { HasExitedPaintedUnion = true; }
            }

            if (MergedIntervals.IsEmpty())
            { HasExitedPaintedUnion = true; }
            SegmentStart = SegmentEnd;
            WorldSegmentStart = Waypoint;
        }
        if (NOT HasExitedPaintedUnion)
        { return false; }

        OutWaypoints = EscapeResult.Get_Waypoints();
        return true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
    FProcessor_CrowdAgent_PathRefresh::
        Get_CurrentConfirmationSerial()
        -> uint64
    {
        return ck_crowd_agent_path_refresh::GConfirmationSerial;
    }

    auto
        FProcessor_CrowdAgent_PathRefresh::
        IssueConfirmationSerial()
        -> uint64
    {
        return ++ck_crowd_agent_path_refresh::GConfirmationSerial;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_PathRefresh::
        Try_BuildStationaryMarkupDetour(
            FCk_Handle InAnyWorldHandle,
            FCk_Entity InSelfEntity,
            const FVector& InStartLocation,
            const FVector& InGoal,
            const FFragment_CrowdAgent_Params& InParams,
            float InArrivalRadius,
            const TArray<FVector>& InCorridorWaypoints,
            ECk_CrowdAvoidanceVolume_QueryPhase InVolumeQueryPhase,
            TArray<FVector>& OutWaypoints)
        -> bool
    {
        if (InCorridorWaypoints.IsEmpty())
        { return false; }

        const auto* Settings = UCk_Utils_Crowd_Settings_UE::Get();
        if (NOT IsValid(Settings) ||
            Settings->Get_PathRefreshMode() != ECk_CrowdPathRefreshMode::Enabled)
        { return false; }

        auto Discs = TArray<FSettledDisc, TInlineAllocator<32>>{};
        const auto GoalExemptionPad = InArrivalRadius + InParams.Get_Radius();
        if (Settings->Get_StationaryMarkupMode() == ECk_CrowdStationaryMarkupMode::Enabled)
        { InAnyWorldHandle.View<FFragment_CrowdAgent_NavMarkup>().ForEach(
            [&](FCk_Entity InEntity, const FFragment_CrowdAgent_NavMarkup& InMarkup)
        {
            if (InEntity == InSelfEntity ||
                NOT InMarkup.Get_Markup().IsValid() ||
                NOT InMarkup.Get_ConfirmedOnMesh())
            { return; }

            // Joining a queue legitimately ends beside standing agents. Match PathRefresh's
            // exemption so a corridor install does not manufacture a loop around its own goal.
            if (FVector::Dist2D(InMarkup.Get_MarkupLocation(), InGoal) <=
                InMarkup.Get_MarkupRadiusUu() + GoalExemptionPad)
            { return; }

            Discs.Add(FSettledDisc{
                InEntity,
                InMarkup.Get_MarkupLocation(),
                InMarkup.Get_MarkupRadiusUu(),
                InMarkup.Get_ConfirmationSerial()});
        }); }

        auto Volumes = TArray<crowd_avoidance_volume::FCk_Obb, TInlineAllocator<16>>{};
        InAnyWorldHandle.View<FFragment_CrowdAvoidanceVolume_ProbeRef>().ForEach(
            [&](FCk_Entity InEntity, const FFragment_CrowdAvoidanceVolume_ProbeRef& InRuntime)
        {
            if (NOT InRuntime.Get_Markup().IsValid() || NOT InRuntime.Get_ConfirmedOnMesh())
            { return; }
            const auto Expanded = crowd_avoidance_volume::MakeEffectiveAgentObb(
                InRuntime.Get_AuthoredObb(),
                InRuntime.Get_PaintedObb(),
                InParams.Get_Radius());
            if (Expanded.IsFiniteAndPositive())
            { Volumes.Add(Expanded); }
        });

        if (Discs.IsEmpty() && Volumes.IsEmpty())
        { return false; }

        const auto GetPoint = [&](int32 InPointIndex) -> const FVector&
        {
            return InPointIndex == 0
                ? InStartLocation
                : InCorridorWaypoints[InPointIndex - 1];
        };

        auto FirstHitSegment = int32{INDEX_NONE};
        auto LastHitSegment = int32{INDEX_NONE};
        for (auto SegmentIndex = 0; SegmentIndex < InCorridorWaypoints.Num(); ++SegmentIndex)
        {
            const auto& WorldSegmentStart = GetPoint(SegmentIndex);
            const auto& WorldSegmentEnd = GetPoint(SegmentIndex + 1);
            const auto SegmentStart = FVector2D{WorldSegmentStart};
            const auto SegmentEnd = FVector2D{WorldSegmentEnd};

            auto SegmentHit = false;
            for (const auto& Disc : Discs)
            {
                const auto DiscCenter = FVector2D{Disc._Center};
                const auto Closest =
                    FMath::ClosestPointOnSegment2D(DiscCenter, SegmentStart, SegmentEnd);
                if (FVector2D::Distance(Closest, DiscCenter) > Disc._Radius)
                { continue; }

                if (FirstHitSegment == INDEX_NONE)
                { FirstHitSegment = SegmentIndex; }
                LastHitSegment = SegmentIndex;
                SegmentHit = true;
                break;
            }
            if (NOT SegmentHit)
            {
                for (const auto& Volume : Volumes)
                {
                    if (NOT crowd_avoidance_volume::IntersectsSegment(
                        Volume, WorldSegmentStart, WorldSegmentEnd))
                    { continue; }
                    if (FirstHitSegment == INDEX_NONE)
                    { FirstHitSegment = SegmentIndex; }
                    LastHitSegment = SegmentIndex;
                    break;
                }
            }
        }

        if (FirstHitSegment == INDEX_NONE)
        { return false; }

        // Give Recast one clean corridor point on each side of the affected span. With the
        // normal corridor spacing this prevents either query endpoint from sitting on the cost
        // area boundary, while still rejoining locally rather than replacing the whole route.
        const auto EntryPointIndex = FMath::Max(0, FirstHitSegment - 1);
        const auto ExitPointIndex =
            FMath::Min(InCorridorWaypoints.Num(), LastHitSegment + 2);
        const auto& EntryPoint = GetPoint(EntryPointIndex);
        const auto& ExitPoint = GetPoint(ExitPointIndex);

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InAnyWorldHandle);
        auto* NavSys = IsValid(World) ? UNavigationSystemV1::GetCurrent(World) : nullptr;
        auto* NavData = (NavSys != nullptr)
            ? Cast<ARecastNavMesh>(
                NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate))
            : nullptr;
        if (NavSys == nullptr || NavData == nullptr)
        { return false; }

        auto DetourResult = FCk_Nav_PathResult{};
        const auto FoundDetour = FCk_Nav_Algorithm::FindPathSync(
            *NavSys,
            *NavData,
            EntryPoint,
            ExitPoint,
            /*InAllowPartial*/ false,
            UCk_Utils_Nav_Settings_UE::Get_NavQuerySearchHalfExtent(),
            UCk_Utils_Nav_Settings_UE::Get_NavQueryVerticalHalfExtent(),
            InParams.Get_Radius(),
            DetourResult,
            InParams.Get_NavQueryFilter(),
            0.0f,
            UCk_Utils_CrowdAvoidanceVolume_UE::Get_NavQueryFilterOverlay(InVolumeQueryPhase));
        if (NOT FoundDetour || DetourResult.Get_Waypoints().IsEmpty())
        {
            ck::crowd::Verbose(
                TEXT("Stationary-markup corridor splice [{} -> {}] found no complete nav detour"),
                EntryPoint,
                ExitPoint);
            return false;
        }

        auto Candidate = TArray<FVector>{};
        Candidate.Reserve(
            InCorridorWaypoints.Num() + DetourResult.Get_Waypoints().Num());
        const auto AppendDistinct = [&](const FVector& InPoint)
        {
            constexpr auto MergeDistanceUu = 1.0f;
            if (Candidate.IsEmpty() ||
                FVector::DistSquared(Candidate.Last(), InPoint) >
                    FMath::Square(MergeDistanceUu))
            {
                Candidate.Add(InPoint);
            }
        };

        // Point zero is the agent's current location and is not stored in a nav path. Point N
        // maps to corridor waypoint N-1.
        for (auto WaypointIndex = 0;
             WaypointIndex < EntryPointIndex;
             ++WaypointIndex)
        {
            AppendDistinct(InCorridorWaypoints[WaypointIndex]);
        }
        for (const auto& DetourWaypoint : DetourResult.Get_Waypoints())
        { AppendDistinct(DetourWaypoint); }
        for (auto WaypointIndex = ExitPointIndex;
             WaypointIndex < InCorridorWaypoints.Num();
             ++WaypointIndex)
        {
            AppendDistinct(InCorridorWaypoints[WaypointIndex]);
        }

        if (Candidate.IsEmpty())
        { return false; }

        // Confirmation proves the cost reached Recast, but retain a total failure path: a custom
        // filter may deliberately make the crowd area cheap enough to cross. In that case the
        // corridor remains valid preferred geometry, so do not claim or install a fake detour.
        auto CandidateCrossesMarkup = false;
        auto SegmentStart = FVector2D{InStartLocation};
        auto WorldSegmentStart = InStartLocation;
        for (const auto& Waypoint : Candidate)
        {
            const auto SegmentEnd = FVector2D{Waypoint};
            for (const auto& Disc : Discs)
            {
                const auto DiscCenter = FVector2D{Disc._Center};
                const auto Closest =
                    FMath::ClosestPointOnSegment2D(DiscCenter, SegmentStart, SegmentEnd);
                if (FVector2D::Distance(Closest, DiscCenter) <= Disc._Radius)
                {
                    CandidateCrossesMarkup = true;
                    break;
                }
            }
            if (NOT CandidateCrossesMarkup)
            {
                for (const auto& Volume : Volumes)
                {
                    if (crowd_avoidance_volume::IntersectsSegment(
                        Volume, WorldSegmentStart, Waypoint))
                    {
                        CandidateCrossesMarkup = true;
                        break;
                    }
                }
            }
            if (CandidateCrossesMarkup)
            { break; }
            SegmentStart = SegmentEnd;
            WorldSegmentStart = Waypoint;
        }

        if (CandidateCrossesMarkup)
        {
            ck::crowd::Verbose(
                TEXT("Stationary-markup corridor splice [{} -> {}] still crosses confirmed markup"),
                EntryPoint,
                ExitPoint);
            return false;
        }

        ck::crowd::Verbose(
            TEXT("Stationary-markup corridor splice replaced segments [{}..{}] with [{}] nav waypoints"),
            FirstHitSegment,
            LastHitSegment,
            DetourResult.Get_Waypoints().Num());
        OutWaypoints = MoveTemp(Candidate);
        return true;
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
        if (InPathFollow.Get_PathSerial() >= _MaxConfirmationSerial)
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

        auto CrossesFreshMarkup = false;
        for (const auto& Disc : _SettledDiscs)
        {
            // Only a disc confirmed AFTER this path can invalidate it — the planner already saw
            // (and priced) every older confirmed disc.
            if (Disc._ConfirmationSerial <= PathSerial)
            { continue; }

            if (Disc._Owner == SelfEntity)
            { continue; }

            // Joining a queue legitimately ENDS beside standing agents — a disc adjacent to the
            // agent's own goal must not bounce it into a re-path that clips the same disc.
            if (FVector::Dist2D(Disc._Center, Goal) <= Disc._Radius + GoalExemptionPad)
            { continue; }

            const auto DiscCenter2D = FVector2D{Disc._Center};
            auto SegStart = FVector2D{SelfLoc};
            for (auto Idx = FirstIdx; Idx < Waypoints.Num(); ++Idx)
            {
                const auto SegEnd = FVector2D{Waypoints[Idx]};
                const auto Closest = FMath::ClosestPointOnSegment2D(DiscCenter2D, SegStart, SegEnd);
                if (ck_crowd_agent_path_refresh::Is_InsidePaintedBand(
                        static_cast<float>(FVector2D::Distance(Closest, DiscCenter2D)), Disc._Radius))
                {
                    CrossesFreshMarkup = true;
                    break;
                }
                SegStart = SegEnd;
            }

            if (CrossesFreshMarkup)
            { break; }
        }

        if (NOT CrossesFreshMarkup)
        {
            for (const auto& Volume : _SettledVolumes)
            {
                if (Volume._ConfirmationSerial <= PathSerial)
                { continue; }

                const auto ExpandedObb = crowd_avoidance_volume::MakeEffectiveAgentObb(
                    Volume._PhysicalObb, Volume._PaintedObb, InParams.Get_Radius());
                if (NOT ExpandedObb.IsFiniteAndPositive())
                { continue; }

                auto SegmentStart = SelfLoc;
                for (auto Idx = FirstIdx; Idx < Waypoints.Num(); ++Idx)
                {
                    if (crowd_avoidance_volume::IntersectsSegment(
                        ExpandedObb, SegmentStart, Waypoints[Idx]))
                    {
                        CrossesFreshMarkup = true;
                        break;
                    }
                    SegmentStart = Waypoints[Idx];
                }

                if (CrossesFreshMarkup)
                { break; }
            }
        }

        if (NOT CrossesFreshMarkup)
        {
            for (const auto& Retirement : _SettledRetirements)
            {
                if (Retirement._ConfirmationSerial <= PathSerial)
                { continue; }

                const auto EffectiveObb = crowd_avoidance_volume::MakeEffectiveAgentObb(
                    Retirement._PhysicalObb,
                    Retirement._PaintedObb,
                    InParams.Get_Radius());
                if (EffectiveObb.IsFiniteAndPositive() &&
                    crowd_avoidance_volume::IntersectsSegment(
                        EffectiveObb, SelfLoc, Goal))
                {
                    CrossesFreshMarkup = true;
                    break;
                }
            }
        }

        // Path and discs are both static, so a clean scan against the current set stays clean —
        // fast-forward the serial either way; a re-path's install re-stamps it anyway.
        InPathFollow._PathSerial = _MaxConfirmationSerial;

        if (NOT CrossesFreshMarkup)
        { return; }

        auto NonConstHandle = InHandle;
        NonConstHandle.Try_Remove<FTag_CrowdAgent_Walking>();
        NonConstHandle.AddOrGet<FTag_CrowdAgent_PathPending>();

        InPathFollow._WaypointIndex = 0;
        FProcessor_CrowdAgent_HandleRequests::AdvanceNavigationRequestRevision(InPathFollow);
        InPathFollow._StrictPlanFailed = false;
        InPathFollow._StrictStandingCrowdPlanFailed = false;

        // A detour is longer than the path it replaces, so carrying the old remaining-distance
        // baseline across would read the re-route itself as lost progress.
        InBlockDetect.DoResetProgressWindow();

        if (UCk_Utils_PathNetworkFollower_UE::Has(NonConstHandle))
        {
            FCk_Nav_Algorithm::MarkPathPending(
                NonConstHandle, InPathFollow.Get_ActiveNavigationRequestRevision());
            NonConstHandle.Try_Remove<FFragment_CrowdAgent_InstalledRoute>();

            auto Follower = UCk_Utils_PathNetworkFollower_UE::CastChecked(NonConstHandle);
            auto Request = FCk_Request_PathNetworkFollower_FindRoute{Goal};
            InPathFollow._PlanPhase =
                FProcessor_CrowdAgent_HandleRequests::Get_ShouldPlanStrict(NonConstHandle, InPathFollow)
                ? ECk_CrowdAgent_PlanPhase::Strict
                : ECk_CrowdAgent_PlanPhase::Permissive;
            InPathFollow._PlanUsesStrictStandingCrowdFilter = false;
            Request.Set_NavQueryFilter(InParams.Get_NavQueryFilter());
            Request.Set_QueryFilterOverlay(
                UCk_Utils_CrowdAvoidanceVolume_UE::Get_NavQueryFilterOverlay(
                    InPathFollow.Get_PlanPhase() == ECk_CrowdAgent_PlanPhase::Strict
                        ? ECk_CrowdAvoidanceVolume_QueryPhase::Strict
                        : ECk_CrowdAvoidanceVolume_QueryPhase::Permissive));
            FProcessor_CrowdAgent_HandleRequests::ApplyMarkupEscapeStart(
                NonConstHandle, InParams, Goal, Request);
            Request.Set_RequestRevision(InPathFollow.Get_ActiveNavigationRequestRevision());
            UCk_Utils_PathNetworkFollower_UE::Request_FindRoute(Follower, Request, {});
        }
        else
        {
            // A newly confirmed disc IS new markup evidence — the strict phase gets a fresh attempt.
            FProcessor_CrowdAgent_HandleRequests::Request_NavigationPath(
                NonConstHandle, InParams, InPathFollow, Goal);
        }

        ck::crowd::Verbose(TEXT("CrowdAgent [{}] remaining path crosses fresh nav-area markup — re-pathing to {}"),
            InHandle, Goal);
    }
}

// --------------------------------------------------------------------------------------------------------------------
