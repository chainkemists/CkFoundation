#pragma once

#include "CoreMinimal.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkCrowd/Agent/CkCrowdAgent_AccelClamp_Algorithm.h"
#include "CkCrowd/Agent/CkCrowdAgent_Avoidance_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Fragment.h"
#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::ck_crowd_agent_avoidance_sample_algorithm
{
    struct FCandidateScore final
    {
        float _DesiredVelocityPenalty = 0.0f;
        float _CurrentVelocityPenalty = 0.0f;
        float _TimeToImpactPenalty = 0.0f;
        float _SidePenalty = 0.0f;
        float _TotalPenalty = TNumericLimits<float>::Max();
    };

    struct FMinimumTimeToCollision final
    {
        int32 _NeighborIndex = INDEX_NONE;
        float _Time = FLT_MAX;
    };

    struct FReachabilityParameters final
    {
        bool _Enabled = false;
        FVector _LastVelocity = FVector::ZeroVector;
        float _MaxAcceleration = 0.0f;
        float _MaxTurnRate = 0.0f;
        float _DeltaSeconds = 0.0f;
    };

    inline auto MakeReachabilityParameters(
        const ECk_AccelClampMode InMode,
        const FVector& InLastVelocity,
        const float InMaxAcceleration,
        const float InMaxTurnRate,
        const float InDeltaSeconds) -> FReachabilityParameters
    {
        return {
            InMode == ECk_AccelClampMode::Enabled,
            InLastVelocity,
            InMaxAcceleration,
            InMaxTurnRate,
            InDeltaSeconds};
    }

    // A cached navmesh wall in the shape the scorer wants. _Touching is dtCrowd's seg->touch, which
    // dtObstacleAvoidanceQuery computes ONCE per agent in prepare() and not per candidate
    // (DetourObstacleAvoidance.cpp:315-320).
    struct FWallSegment final
    {
        FVector _Start = FVector::ZeroVector;
        FVector _End = FVector::ZeroVector;
        bool _Touching = false;
    };

    using FWallSegments = TArray<FWallSegment, TInlineAllocator<FFragment_CrowdAgent_LocalBoundary::MaxSegments>>;

    struct FWallParameters final
    {
        FVector _AgentPosition = FVector::ZeroVector;
        TConstArrayView<FWallSegment> _Segments;
    };

    inline auto Dot2D(const FVector& InLhs, const FVector& InRhs) -> double
    {
        return (InLhs.X * InRhs.X) + (InLhs.Y * InRhs.Y);
    }

    inline auto Cross2D(const FVector& InLhs, const FVector& InRhs) -> double
    {
        return (InLhs.X * InRhs.Y) - (InLhs.Y * InRhs.X);
    }

    // dtDistancePtSegSqr2D (DetourCommon.h). OutSegmentTime is Detour's clamped `t`, which the
    // boundary refresh needs to interpolate the wall's height at the closest point.
    inline auto DistancePointSegmentSquared2D(
        const FVector& InPoint,
        const FVector& InStart,
        const FVector& InEnd,
        double* OutSegmentTime = nullptr) -> double
    {
        const auto Segment = InEnd - InStart;
        const auto ToPoint = InPoint - InStart;
        const auto LengthSquared = (Segment.X * Segment.X) + (Segment.Y * Segment.Y);
        auto SegmentTime = (Segment.X * ToPoint.X) + (Segment.Y * ToPoint.Y);
        if (LengthSquared > 0.0)
        { SegmentTime /= LengthSquared; }
        SegmentTime = FMath::Clamp(SegmentTime, 0.0, 1.0);

        if (OutSegmentTime != nullptr)
        { *OutSegmentTime = SegmentTime; }

        const auto DeltaX = InStart.X + (SegmentTime * Segment.X) - InPoint.X;
        const auto DeltaY = InStart.Y + (SegmentTime * Segment.Y) - InPoint.Y;
        return (DeltaX * DeltaX) + (DeltaY * DeltaY);
    }

    // The OUTWARD normal of a wall - it points off the walkable surface. Two callers depend on that
    // orientation, so neither the transform nor the sign is taken on faith:
    //
    // TRANSFORM. Detour computes it as `snorm[0] = -sdir[2]; snorm[2] = sdir[0]` in its XZ plane
    // (DetourObstacleAvoidance.cpp:376-378). Unreal2RecastPoint is (x, y, z) -> (-x, z, -y)
    // (NavMesh/RecastHelpers.h), so a wall direction D in Unreal has sdir = (-D.X, ., -D.Y);
    // feeding that through gives snorm = (D.Y, ., -D.X), and mapping back to Unreal
    // ((rx, ry, rz) -> (-rx, -rz, ry)) gives exactly (-D.Y, D.X). Because BOTH horizontal axes are
    // negated, the 2D dot and the 2D cross are sign-identical in the two spaces too - so this is
    // Detour's own comparison, not an analogue of it.
    //
    // SIGN. getPolyWallSegments and storeWallSegment both emit a wall in its source polygon's vertex
    // winding (Start = verts[i], End = verts[i+1]), and Recast winds a polygon so its INTERIOR is
    // the area2 < 0 side: RecastMesh.cpp defines left(a,b,c) as area2 < 0, and that is the
    // ear-clipping predicate the poly mesh is triangulated with. area2(Start, End, X) is
    // Cross2D(End - Start, X - Start), which is Dot2D of this vector with (X - Start). An interior
    // point therefore projects NEGATIVE and the normal points away from the mesh. Consequences: a
    // candidate velocity that projects negative is heading back inland, which is the branch Detour
    // lets through free; and an agent that projects negative is standing on the walkable side of
    // the wall, which is dtCrowd's back-face filter (DetourCrowd.cpp:1561) below.
    inline auto MakeWallOutwardNormal(const FWallSegment& InSegment) -> FVector
    {
        const auto Direction = InSegment._End - InSegment._Start;
        return FVector{-Direction.Y, Direction.X, 0.0};
    }

    // isectRaySeg (DetourObstacleAvoidance.cpp:51-68). OutTime is the RAY parameter, so with a
    // candidate VELOCITY as the direction it is seconds; Detour bounds it to [0,1] and so does this.
    inline auto IntersectRaySegment2D(
        const FVector& InRayOrigin,
        const FVector& InRayDirection,
        const FVector& InSegmentStart,
        const FVector& InSegmentEnd,
        float& OutTime) -> bool
    {
        const auto SegmentDelta = InSegmentEnd - InSegmentStart;
        const auto OriginDelta = InRayOrigin - InSegmentStart;

        const auto Denominator = Cross2D(InRayDirection, SegmentDelta);
        if (FMath::Abs(Denominator) < 1e-6)
        { return false; }

        const auto InverseDenominator = 1.0 / Denominator;

        const auto RayTime = Cross2D(SegmentDelta, OriginDelta) * InverseDenominator;
        if (RayTime < 0.0 || RayTime > 1.0)
        { return false; }

        const auto SegmentTime = Cross2D(InRayDirection, OriginDelta) * InverseDenominator;
        if (SegmentTime < 0.0 || SegmentTime > 1.0)
        { return false; }

        OutTime = static_cast<float>(RayTime);
        return true;
    }

    // dtCrowd hands the obstacle query only the walls whose walkable side faces the agent
    // (DetourCrowd.cpp:1561) - a wall bounding some other part of the same connected neighbourhood
    // is not one this agent can walk into. Both that filter and `touch` read the agent's CURRENT
    // position, so they are rebuilt every sampled frame while the segments themselves are cached.
    inline auto BuildWallSegments(
        const FVector& InAgentPosition,
        const FFragment_CrowdAgent_LocalBoundary& InBoundary) -> FWallSegments
    {
        // dtObstacleAvoidanceQuery::prepare's "really close to the segment" radius.
        constexpr auto TouchRadius = 0.01;

        auto Walls = FWallSegments{};
        for (const auto& Segment : InBoundary.Get_Segments())
        {
            auto Wall = FWallSegment{Segment._Start, Segment._End, false};
            if (Dot2D(MakeWallOutwardNormal(Wall), InAgentPosition - Wall._Start) > 0.0)
            { continue; }

            Wall._Touching =
                DistancePointSegmentSquared2D(InAgentPosition, Wall._Start, Wall._End) < (TouchRadius * TouchRadius);
            Walls.Emplace(MoveTemp(Wall));
        }
        return Walls;
    }

    struct FScoringParameters final
    {
        float _AgentRadius = 0.0f;
        float _MaxSpeed = 0.0f;
        float _Horizon = 0.0f;
        float _WeightDesiredVelocity = 0.0f;
        float _WeightCurrentVelocity = 0.0f;
        float _WeightSide = 0.0f;
        float _WeightTimeToImpact = 0.0f;
        ECk_AvoidanceSidePreference _SidePreference = ECk_AvoidanceSidePreference::Disabled;
        FReachabilityParameters _Reachability;
        FWallParameters _Walls;
    };

    struct FSampleCloud final
    {
        FVector _BiasCentre = FVector::ZeroVector;
        TArray<FVector, TInlineAllocator<32>> _Candidates;
        FVector _DesiredVelocity = FVector::ZeroVector;
        float _MaxSpeed = 0.0f;
        float _VelBias = 0.0f;
        float _CloudRadius = 0.0f;
        int32 _AngularDivs = 0;
        int32 _Rings = 0;
        int32 _Depth = 1;
    };

    struct FSampleWinner final
    {
        int32 _CandidateIndex = INDEX_NONE;
        FVector _ScoredVelocity = FVector::ZeroVector;
        FCandidateScore _Score;
    };

    struct FSampleDepthResult final
    {
        int32 _Depth = 0;
        FVector _Centre = FVector::ZeroVector;
        float _Radius = 0.0f;
        FSampleWinner _Winner;
        FVector _WinnerVelocity = FVector::ZeroVector;
        FVector _WinnerScoredVelocity = FVector::ZeroVector;
        bool _MirrorPresent = false;
        int32 _MirrorCandidateIndex = INDEX_NONE;
        FVector _MirrorVelocity = FVector::ZeroVector;
        FVector _MirrorScoredVelocity = FVector::ZeroVector;
        FCandidateScore _MirrorScore;
        float _MirrorMinusWinnerPenalty = 0.0f;
        float _TieEpsilon = 0.0f;
        bool _TieWithinEpsilon = false;
    };

    constexpr auto DiagnosticMirrorEpsilon = KINDA_SMALL_NUMBER;
    constexpr auto DiagnosticTieEpsilon = KINDA_SMALL_NUMBER;
    constexpr auto MaximumSampleAngularDivs = 16;
    constexpr auto MaximumSampleRings = 4;
    constexpr auto MaximumSampleDepth = 8;

    using FSampleDepthResults = TArray<FSampleDepthResult, TInlineAllocator<MaximumSampleDepth>>;

    inline auto HasAvoidanceTag(const FCk_Handle& InAgent, const FGameplayTag& InTag) -> bool
    {
        if (InAgent.Has<FFragment_CrowdAgent_Params>() &&
            InAgent.Get<FFragment_CrowdAgent_Params>().Get_Tags().HasTagExact(InTag))
        { return true; }

        const auto Owner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAgent);
        if (ck::Is_NOT_Valid(Owner) || NOT Owner.Has<FFragment_CrowdAgent_Params>())
        { return false; }
        return Owner.Get<FFragment_CrowdAgent_Params>().Get_Tags().HasTagExact(InTag);
    }

    inline auto ShouldSample(const FCk_Handle_CrowdAgent& InAgent, const FFragment_CrowdAgent_NeighborCache& InCache) -> bool
    {
        if (InAgent.Has<FFragment_CrowdAgent_AvoidancePolicy>())
        {
            switch (InAgent.Get<FFragment_CrowdAgent_AvoidancePolicy>().Get_Policy())
            {
                case ECk_AvoidancePolicy::ForceOnly: return false;
                case ECk_AvoidancePolicy::SamplingAlways: return true;
                case ECk_AvoidancePolicy::UseProjectDefault: break;
            }
        }

        if (HasAvoidanceTag(InAgent, TAG_CrowdAvoidance_NeverSample))
        { return false; }

        const auto Trigger = UCk_Utils_Crowd_Settings_UE::Get_AvoidanceSampleTrigger();
        if (Trigger == ECk_AvoidanceSampleTrigger::Disabled)
        { return false; }

        const auto AlwaysTagSet = HasAvoidanceTag(InAgent, TAG_CrowdAvoidance_AlwaysSample);
        const auto Threshold = UCk_Utils_Crowd_Settings_UE::Get_AvoidanceSampleNeighborThreshold();
        const auto NeighborGate = InCache.Get_Neighbors().Num() >= Threshold;
        switch (Trigger)
        {
            case ECk_AvoidanceSampleTrigger::NeighborCountOnly: return NeighborGate;
            case ECk_AvoidanceSampleTrigger::ZoneTagOnly: return AlwaysTagSet;
            case ECk_AvoidanceSampleTrigger::NeighborCountAndZoneTag: return NeighborGate || AlwaysTagSet;
            default: return false;
        }
    }

    inline auto IsSamplingFrame(const FCk_Handle_CrowdAgent& InAgent) -> bool
    {
        const auto Stride = FMath::Max(1, UCk_Utils_Crowd_Settings_UE::Get_AvoidanceSampleStride());
        return (static_cast<int32>(GFrameCounter) + static_cast<int32>(GetTypeHash(InAgent))) % Stride == 0;
    }

    inline auto IsExpectedPreviousSamplingFrame(
        const int32 InCurrentFrame,
        const int32 InPreviousFrame,
        const int32 InStride) -> bool
    {
        const auto ExpectedStride = FMath::Max(1, InStride);
        return InPreviousFrame != INDEX_NONE &&
            InCurrentFrame > InPreviousFrame &&
            InCurrentFrame - InPreviousFrame == ExpectedStride;
    }

    // Keep the production order: round-robin first, then trigger/override/tag evaluation.
    inline auto ShouldRunSampling(const FCk_Handle_CrowdAgent& InAgent, const FFragment_CrowdAgent_NeighborCache& InCache) -> bool
    {
        return IsSamplingFrame(InAgent) && ShouldSample(InAgent, InCache);
    }

    inline auto BuildCandidatePattern(
        const FVector& InDesiredVelocity,
        const FVector& InCentre,
        const float InCloudRadius,
        const float InMaxSpeed,
        const int32 InAngularDivs,
        const int32 InRings) -> TArray<FVector, TInlineAllocator<32>>
    {
        auto Candidates = TArray<FVector, TInlineAllocator<32>>{};
        const auto HasValidAngularDivs = InAngularDivs >= 1 && InAngularDivs <= MaximumSampleAngularDivs;
        CK_ENSURE_IF_NOT(HasValidAngularDivs,
            TEXT("Invalid crowd avoidance sample angular divisions [{}]"), InAngularDivs) {}
        if (NOT HasValidAngularDivs)
        { return Candidates; }

        const auto HasValidRings = InRings >= 1 && InRings <= MaximumSampleRings;
        CK_ENSURE_IF_NOT(HasValidRings, TEXT("Invalid crowd avoidance sample rings [{}]"), InRings) {}
        if (NOT HasValidRings)
        { return Candidates; }

        Candidates.Reserve(InAngularDivs * InRings + 1);
        const auto MaxSpeedSquared = FMath::Square(InMaxSpeed + KINDA_SMALL_NUMBER);
        if (InCentre.SizeSquared2D() <= MaxSpeedSquared)
        { Candidates.Add(InCentre); }

        const auto DesiredDirection = InDesiredVelocity.IsNearlyZero() ? FVector::ForwardVector : InDesiredVelocity.GetSafeNormal();
        const auto BaseAngle = FMath::Atan2(DesiredDirection.Y, DesiredDirection.X);
        const auto AngularStep = (2.0f * PI) / static_cast<float>(InAngularDivs);
        for (int32 Ring = 1; Ring <= InRings; ++Ring)
        {
            const auto Radius = (static_cast<float>(Ring) / static_cast<float>(InRings)) * InCloudRadius;
            const auto AngleOffset = Ring % 2 == 0 ? 0.5f * AngularStep : 0.0f;
            for (int32 Division = 0; Division < InAngularDivs; ++Division)
            {
                const auto Angle = BaseAngle + AngleOffset + AngularStep * static_cast<float>(Division);
                const auto Candidate = InCentre + FVector{FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f};
                if (Candidate.SizeSquared2D() <= MaxSpeedSquared)
                { Candidates.Add(Candidate); }
            }
        }
        return Candidates;
    }

    inline auto BuildSampleCloud(
        const FVector& InDesiredVelocity,
        const float InMaxSpeed,
        const float InVelBias,
        const int32 InAngularDivs,
        const int32 InRings,
        const int32 InDepth) -> FSampleCloud
    {
        auto Cloud = FSampleCloud{};
        Cloud._DesiredVelocity = InDesiredVelocity;
        Cloud._MaxSpeed = InMaxSpeed;
        Cloud._VelBias = InVelBias;
        Cloud._CloudRadius = InMaxSpeed * (1.0f - InVelBias);
        Cloud._AngularDivs = InAngularDivs;
        Cloud._Rings = InRings;
        Cloud._Depth = FMath::Clamp(InDepth, 1, MaximumSampleDepth);
        Cloud._BiasCentre = InDesiredVelocity * InVelBias;
        Cloud._Candidates = BuildCandidatePattern(
            InDesiredVelocity,
            Cloud._BiasCentre,
            Cloud._CloudRadius,
            InMaxSpeed,
            InAngularDivs,
            InRings);
        return Cloud;
    }

    inline auto CalculateTimeToCollision(const FVector& InCandidateVelocity, const FVector& InCurrentVelocity, const FVector& InRelativePosition, const FVector& InRelativeVelocity, const float InCombinedRadius, const float InHorizon) -> float
    {
        const auto DeltaVelocity = FVector2D(2.0f * (InCandidateVelocity - InCurrentVelocity) - InRelativeVelocity);
        const auto Position = FVector2D(-InRelativePosition.X, -InRelativePosition.Y);
        const auto A = static_cast<float>(FVector2D::DotProduct(DeltaVelocity, DeltaVelocity));
        const auto B = static_cast<float>(FVector2D::DotProduct(Position, DeltaVelocity));
        const auto C = static_cast<float>(FVector2D::DotProduct(Position, Position)) - InCombinedRadius * InCombinedRadius;
        const auto Discriminant = B * B - A * C;
        if (C < 0.0f)
        {
            if (A < KINDA_SMALL_NUMBER)
            { return 0.0f; }
            return -((-B - FMath::Sqrt(Discriminant)) / A) * 0.5f;
        }
        if (A < KINDA_SMALL_NUMBER || B >= 0.0f || Discriminant < 0.0f)
        { return FLT_MAX; }
        const auto Time = (-B - FMath::Sqrt(Discriminant)) / A;
        return Time > InHorizon ? FLT_MAX : Time;
    }

    inline auto CalculateNeighborSidePenalty(const FVector& InCandidateVelocity, const FVector& InNeighborRelativeOffset, const float InMaxSpeed, const ECk_AvoidanceSidePreference InPreference) -> float
    {
        const auto IsValidPreference = InPreference == ECk_AvoidanceSidePreference::Disabled || InPreference == ECk_AvoidanceSidePreference::PassLeft || InPreference == ECk_AvoidanceSidePreference::PassRight;
        CK_ENSURE_IF_NOT(IsValidPreference, TEXT("Invalid crowd avoidance side preference [{}]"), static_cast<uint8>(InPreference)) {}
        if (NOT IsValidPreference || InMaxSpeed <= KINDA_SMALL_NUMBER || InPreference == ECk_AvoidanceSidePreference::Disabled)
        { return 0.0f; }

        const auto PreferenceSign = InPreference == ECk_AvoidanceSidePreference::PassLeft ? -1.0f : 1.0f;
        const auto Direction = FVector2D(InNeighborRelativeOffset.X, InNeighborRelativeOffset.Y).GetSafeNormal();
        if (Direction.IsNearlyZero())
        { return 0.0f; }
        return FMath::Clamp(((Direction.X * InCandidateVelocity.Y - Direction.Y * InCandidateVelocity.X) / InMaxSpeed) * PreferenceSign, 0.0f, 1.0f);
    }

    inline auto ResolveScoredCandidateVelocity(
        const FVector& InRawCandidate,
        const FScoringParameters& InParameters) -> FVector
    {
        if (NOT InParameters._Reachability._Enabled)
        { return InRawCandidate; }

        return ck_crowd_agent_accel_clamp_algorithm::ProjectVelocity(
            InParameters._Reachability._LastVelocity,
            InRawCandidate,
            InParameters._MaxSpeed,
            InParameters._Reachability._MaxAcceleration,
            InParameters._Reachability._MaxTurnRate,
            InParameters._Reachability._DeltaSeconds);
    }

    inline auto CalculateCandidateScoreForNeighbors(
        const FVector& InCandidate,
        const FVector& InDesired,
        const FVector& InCurrent,
        const TConstArrayView<FCk_CrowdAgent_Neighbor> InNeighbors,
        const FScoringParameters& InParameters,
        FMinimumTimeToCollision* OutMinimumTimeToCollision = nullptr) -> FCandidateScore
    {
        if (OutMinimumTimeToCollision != nullptr)
        { *OutMinimumTimeToCollision = FMinimumTimeToCollision{}; }

        const auto ScoredCandidate = ResolveScoredCandidateVelocity(InCandidate, InParameters);
        const auto InverseMaxSpeed = InParameters._MaxSpeed > KINDA_SMALL_NUMBER ? 1.0f / InParameters._MaxSpeed : 0.0f;
        const auto InverseHorizon = InParameters._Horizon > KINDA_SMALL_NUMBER ? 1.0f / InParameters._Horizon : 0.0f;
        const auto DesiredPenalty = InParameters._WeightDesiredVelocity * static_cast<float>(FVector::Dist2D(ScoredCandidate, InDesired)) * InverseMaxSpeed;
        const auto CurrentPenalty = InParameters._WeightCurrentVelocity * static_cast<float>(FVector::Dist2D(ScoredCandidate, InCurrent)) * InverseMaxSpeed;
        auto MinimumTime = InParameters._Horizon;
        auto SidePenaltySum = 0.0f;
        const auto SideEnabled = InParameters._SidePreference != ECk_AvoidanceSidePreference::Disabled;
        for (int32 NeighborIndex = 0; NeighborIndex < InNeighbors.Num(); ++NeighborIndex)
        {
            const auto& Neighbor = InNeighbors[NeighborIndex];
            const auto TimeToCollision = CalculateTimeToCollision(
                ScoredCandidate,
                InCurrent,
                Neighbor.Get_RelativeOffset(),
                Neighbor.Get_RelativeVelocity(),
                InParameters._AgentRadius + InParameters._AgentRadius,
                InParameters._Horizon);
            if (TimeToCollision < MinimumTime)
            {
                MinimumTime = TimeToCollision;
                if (OutMinimumTimeToCollision != nullptr)
                {
                    OutMinimumTimeToCollision->_NeighborIndex = NeighborIndex;
                    OutMinimumTimeToCollision->_Time = TimeToCollision;
                }
            }
            if (SideEnabled)
            { SidePenaltySum += CalculateNeighborSidePenalty(ScoredCandidate, Neighbor.Get_RelativeOffset(), InParameters._MaxSpeed, InParameters._SidePreference); }
        }

        // The segment branch of dtObstacleAvoidanceQuery::processSample
        // (DetourObstacleAvoidance.cpp:369-405). It lowers the SAME minimum time the neighbour
        // sweeps lower, so a wall and a neighbour compete for the one time-to-impact term exactly
        // as they do in Detour. OutMinimumTimeToCollision stays neighbour-only: it names a
        // colliding NEIGHBOUR, and a wall has no index to name.
        for (const auto& Wall : InParameters._Walls._Segments)
        {
            auto TimeToWall = 0.0f;

            if (Wall._Touching)
            {
                // Standing on the wall: a candidate that projects negatively onto the outward
                // normal is heading back into the walkable interior and cannot collide with it.
                // Anything else is aimed INTO the wall and takes the immediate-collision time.
                if (Dot2D(MakeWallOutwardNormal(Wall), ScoredCandidate) < 0.0)
                { continue; }
            }
            else if (NOT IntersectRaySegment2D(
                InParameters._Walls._AgentPosition, ScoredCandidate, Wall._Start, Wall._End, TimeToWall))
            { continue; }

            // "Avoid less when facing walls" (DetourObstacleAvoidance.cpp:402-403).
            TimeToWall *= 2.0f;

            if (TimeToWall < MinimumTime)
            { MinimumTime = TimeToWall; }
        }

        const auto TimeToImpactPenalty = InParameters._WeightTimeToImpact * (1.0f / (0.1f + MinimumTime * InverseHorizon));
        const auto SidePenalty = SideEnabled && NOT InNeighbors.IsEmpty()
            ? InParameters._WeightSide * (SidePenaltySum / static_cast<float>(InNeighbors.Num()))
            : 0.0f;
        return {
            DesiredPenalty,
            CurrentPenalty,
            TimeToImpactPenalty,
            SidePenalty,
            DesiredPenalty + CurrentPenalty + TimeToImpactPenalty + SidePenalty};
    }

    inline auto CalculateCandidateScore(
        const FVector& InCandidate,
        const FVector& InDesired,
        const FVector& InCurrent,
        const FFragment_CrowdAgent_NeighborCache& InCache,
        const FScoringParameters& InParameters,
        FMinimumTimeToCollision* OutMinimumTimeToCollision = nullptr) -> FCandidateScore
    {
        return CalculateCandidateScoreForNeighbors(
            InCandidate,
            InDesired,
            InCurrent,
            InCache.Get_Neighbors(),
            InParameters,
            OutMinimumTimeToCollision);
    }

    inline auto SelectWinnerForCloud(
        const FSampleCloud& InCloud,
        const FVector& InDesired,
        const FVector& InCurrent,
        const FFragment_CrowdAgent_NeighborCache& InCache,
        const FScoringParameters& InParameters,
        TArray<FCandidateScore, TInlineAllocator<32>>* OutScores = nullptr) -> FSampleWinner
    {
        auto Winner = FSampleWinner{};
        if (OutScores != nullptr)
        { OutScores->Reset(); OutScores->Reserve(InCloud._Candidates.Num()); }
        for (int32 CandidateIndex = 0; CandidateIndex < InCloud._Candidates.Num(); ++CandidateIndex)
        {
            const auto Score = CalculateCandidateScore(InCloud._Candidates[CandidateIndex], InDesired, InCurrent, InCache, InParameters);
            if (OutScores != nullptr)
            { OutScores->Add(Score); }
            if (Score._TotalPenalty < Winner._Score._TotalPenalty)
            {
                Winner._CandidateIndex = CandidateIndex;
                Winner._ScoredVelocity = ResolveScoredCandidateVelocity(
                    InCloud._Candidates[CandidateIndex],
                    InParameters);
                Winner._Score = Score;
            }
        }
        return Winner;
    }

    // Depth one deliberately evaluates the exact original cloud. Later depths halve the search
    // radius and retain the same desired-aligned ring order. When downstream reachability is
    // enabled, refine around the velocity the actuator can actually emit; refining around an
    // unreachable raw command makes distinct clouds collapse onto the same scored-output plateau.
    // Disabled reachability preserves the original raw-winner refinement path.
    inline auto FindMirrorCandidateIndex(const FSampleCloud& InCloud, int32 InSelectedCandidateIndex) -> int32;

    inline auto SelectWinner(
        FSampleCloud& InOutCloud,
        const FVector& InDesired,
        const FVector& InCurrent,
        const FFragment_CrowdAgent_NeighborCache& InCache,
        const FScoringParameters& InParameters,
        TArray<FCandidateScore, TInlineAllocator<32>>* OutScores = nullptr,
        FSampleDepthResults* OutDepthResults = nullptr) -> FSampleWinner
    {
        auto Winner = FSampleWinner{};
        auto CloudRadius = InOutCloud._CloudRadius;
        auto DepthScores = TArray<FCandidateScore, TInlineAllocator<32>>{};
        if (OutDepthResults != nullptr)
        {
            OutDepthResults->Reset();
            OutDepthResults->Reserve(InOutCloud._Depth);
        }

        for (int32 DepthIndex = 0; DepthIndex < InOutCloud._Depth; ++DepthIndex)
        {
            auto* ScoresForDepth = OutDepthResults != nullptr ? &DepthScores : OutScores;
            Winner = SelectWinnerForCloud(
                InOutCloud,
                InDesired,
                InCurrent,
                InCache,
                InParameters,
                ScoresForDepth);
            if (Winner._CandidateIndex == INDEX_NONE)
            { return FSampleWinner{}; }

            if (OutScores != nullptr && OutDepthResults != nullptr)
            { *OutScores = DepthScores; }

            if (OutDepthResults != nullptr)
            {
                auto DepthResult = FSampleDepthResult{};
                DepthResult._Depth = DepthIndex + 1;
                DepthResult._Centre = InOutCloud._BiasCentre;
                DepthResult._Radius = InOutCloud._CloudRadius;
                DepthResult._Winner = Winner;
                DepthResult._WinnerVelocity = InOutCloud._Candidates[Winner._CandidateIndex];
                DepthResult._WinnerScoredVelocity = Winner._ScoredVelocity;
                DepthResult._TieEpsilon = DiagnosticTieEpsilon;

                const auto MirrorIndex = FindMirrorCandidateIndex(InOutCloud, Winner._CandidateIndex);
                if (InOutCloud._Candidates.IsValidIndex(MirrorIndex) && DepthScores.IsValidIndex(MirrorIndex))
                {
                    DepthResult._MirrorPresent = true;
                    DepthResult._MirrorCandidateIndex = MirrorIndex;
                    DepthResult._MirrorVelocity = InOutCloud._Candidates[MirrorIndex];
                    DepthResult._MirrorScoredVelocity = ResolveScoredCandidateVelocity(
                        DepthResult._MirrorVelocity,
                        InParameters);
                    DepthResult._MirrorScore = DepthScores[MirrorIndex];
                    DepthResult._MirrorMinusWinnerPenalty =
                        DepthResult._MirrorScore._TotalPenalty - Winner._Score._TotalPenalty;
                    DepthResult._TieWithinEpsilon =
                        FMath::Abs(DepthResult._MirrorMinusWinnerPenalty) <= DepthResult._TieEpsilon;
                }

                OutDepthResults->Add(MoveTemp(DepthResult));
            }

            if (DepthIndex + 1 == InOutCloud._Depth)
            { return Winner; }

            const auto RefinementCentre = InParameters._Reachability._Enabled
                ? Winner._ScoredVelocity
                : InOutCloud._Candidates[Winner._CandidateIndex];
            CloudRadius *= 0.5f;
            InOutCloud._BiasCentre = RefinementCentre;
            InOutCloud._CloudRadius = CloudRadius;
            InOutCloud._Candidates = BuildCandidatePattern(
                InOutCloud._DesiredVelocity,
                InOutCloud._BiasCentre,
                CloudRadius,
                InOutCloud._MaxSpeed,
                InOutCloud._AngularDivs,
                InOutCloud._Rings);
        }
        return FSampleWinner{};
    }

    inline auto FindMirrorCandidateIndex(const FSampleCloud& InCloud, const int32 InSelectedCandidateIndex) -> int32
    {
        if (NOT InCloud._Candidates.IsValidIndex(InSelectedCandidateIndex))
        { return INDEX_NONE; }
        const auto Mirror = 2.0f * InCloud._BiasCentre - InCloud._Candidates[InSelectedCandidateIndex];
        for (int32 CandidateIndex = 0; CandidateIndex < InCloud._Candidates.Num(); ++CandidateIndex)
        {
            if (InCloud._Candidates[CandidateIndex].Equals(Mirror, DiagnosticMirrorEpsilon))
            { return CandidateIndex; }
        }
        return INDEX_NONE;
    }
}

// --------------------------------------------------------------------------------------------------------------------
