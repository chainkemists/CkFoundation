#include "CkProceduralAnimation/Gait/CkProceduralGait_Processor.h"

#include "CkProceduralAnimation/Gait/CkProceduralGait_Utils.h"
#include "CkProceduralAnimation/Leg/CkProceduralLeg_Fragment.h"
#include "CkProceduralAnimation/SurfaceMotion/CkSurfaceMotion_Fragment.h"
#include "CkProceduralAnimation/SurfaceMotion/CkSurfaceMotion_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/TypeTraits/CkTypeTraits.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkJolt/Query/CkJoltQuery_Utils.h"

#include <Engine/World.h>

CK_REGISTER_PROCESSOR(ck::FProcessor_ProceduralGait_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_ProceduralGait_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_ProceduralGait_Update);
CK_REGISTER_PROCESSOR(ck::FProcessor_ProceduralGait_CancelPendingRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_procedural_gait
{
    auto
        Get_TrustedHit(
            const FCk_Jolt_HitResult& InHit,
            const FVector& InRayDirection)
        -> bool
    {
        // An inside-origin Jolt ray reports fraction zero at its origin, not a surface contact, and a normal facing
        // along the ray is the back of a surface the ray started behind. Reject both so the caller can retry from
        // outside the geometry.
        return InHit.Get_HasHit() && InHit.Get_Fraction() > 0.0f && InHit.Get_Fraction() <= 1.0f
            && NOT InHit.Get_Position().ContainsNaN() && NOT InHit.Get_Normal().ContainsNaN()
            && NOT InHit.Get_Normal().IsNearlyZero()
            && FVector::DotProduct(InHit.Get_Normal(), InRayDirection) < -KINDA_SMALL_NUMBER;
    }

    // How many of the three lengthening attempts a ray under a point may take. The ideal target and the floor in front of an
    // occluder take every one; the held foothold and the ring take the first, and the second only when the first began
    // inside a solid.
    enum class EGroundProbeAttempts : uint8
    {
        Every,
        RetryFromInsideSolid
    };

    // Returns the first trusted hit across the retry spans; the debug probe records the last attempt either way.
    auto
        Get_GroundHit(
            UWorld* InWorld,
            const FVector& InPoint,
            const FVector& InUp,
            const FVector& InRadial,
            const FCk_ProceduralGait_Probe& InProbe,
            EGroundProbeAttempts InAttempts,
            int32& InOutRayCount,
            FCk_ProceduralAnimation_DebugProbe* InDebugProbe)
        -> TOptional<FCk_Jolt_HitResult>
    {
        for (auto Attempt = 0; Attempt < ck::ProceduralGroundProbeAttempts; ++Attempt)
        {
            const auto Span = ck::MakeProceduralGroundProbeSpan(Attempt, InProbe.Get_Up(), InProbe.Get_Down(), InProbe.Get_OutwardLean());
            const auto Axis = ck::FProceduralGaitSolver::ComputeTraceAxis(InUp, InRadial, Span.Get_OutwardLean());
            const auto ProbeStart = InPoint + Axis * Span.Get_UpDistance();
            const auto ProbeEnd = InPoint - Axis * Span.Get_DownDistance();
            const auto Hit = UCk_Utils_JoltQuery_UE::Get_RayCast(InWorld, ProbeStart, ProbeEnd, InProbe.Get_QueryFilter());
            ++InOutRayCount;

            if (ck::IsValid(InDebugProbe, ck::IsValid_Policy_NullptrOnly{}))
            {
                InDebugProbe->Set_Start(ProbeStart)
                    .Set_End(ProbeEnd)
                    .Set_AttemptCount(Attempt + 1)
                    .Set_Hit(Hit.Get_HasHit())
                    .Set_HitFraction(Hit.Get_Fraction())
                    .Set_HitPosition(Hit.Get_Position())
                    .Set_HitNormal(Hit.Get_Normal());
            }

            if (Get_TrustedHit(Hit, (ProbeEnd - ProbeStart).GetSafeNormal()))
            { return Hit; }

            const auto StartedInsideSolid = Hit.Get_HasHit() && Hit.Get_Fraction() <= 0.0f;
            const auto MayRetry = InAttempts == EGroundProbeAttempts::Every || (Attempt == 0 && StartedInsideSolid);
            if (NOT MayRetry)
            { return {}; }
        }

        return {};
    }

    // --------------------------------------------------------------------------------------------------------------------

    struct FFootholdQuery
    {
        UWorld* World = nullptr;
        FQuat Basis = FQuat::Identity;
        FVector BodyLocation = FVector::ZeroVector;
        FVector Hip = FVector::ZeroVector;
        FVector Outboard = FVector::ZeroVector;
        float MaxHipLateral = 0.0f;
        FVector Ideal = FVector::ZeroVector;
        FVector Plant = FVector::ZeroVector;
        bool Planted = false;
        float Reach = 0.0f;
        float RestDrop = 0.0f;
        const FCk_ProceduralGait_Probe* Probe = nullptr;
        const FCk_ProceduralGait_Step* Step = nullptr;
        const FCk_ProceduralGait_Foothold* Foothold = nullptr;
    };

    // Source None means nothing usable was found; IdealVerdict then says why the ideal target was not.
    struct FFoothold
    {
        FVector Position = FVector::ZeroVector;
        FVector Normal = FVector::UpVector;
        ck::EProceduralFootholdSource Source = ck::EProceduralFootholdSource::None;
        ck::EProceduralFootholdVerdict IdealVerdict = ck::EProceduralFootholdVerdict::Miss;
    };

    // One validated candidate in world space; the occluder is the solid its hip trace met when it is Occluded. A down-ray
    // candidate may be clamped toward the hip and probed again, a face hit may not, because a down ray there would find a
    // different surface.
    struct FFootholdCandidate
    {
        FVector Position = FVector::ZeroVector;
        FVector Normal = FVector::UpVector;
        ck::EProceduralFootholdSource Source = ck::EProceduralFootholdSource::None;
        ck::EProceduralFootholdVerdict Verdict = ck::EProceduralFootholdVerdict::Miss;
        ck::EProceduralFootholdCast Cast = ck::EProceduralFootholdCast::Down;
        TOptional<FCk_Jolt_HitResult> Occluder;
    };

    // Whether a candidate is one the search found, and so must also be level enough.
    enum class EFootholdStage : uint8
    {
        IdealOrHeld,
        Search
    };

    // What a ray under a point does with ground beyond the target reach from the hip. The ideal target and the search's
    // down rays clamp the point toward the hip and probe once more there; a hold is judged at its own point, because the
    // re-probe would stand in a different point for it and, on continuous ground, never let a trailing hold read
    // Unreachable.
    enum class EFootholdReprobe : uint8
    {
        ClampTowardHip,
        None
    };

    // What one solve may still spend on searches, and the solve a search that finds nothing is stamped with.
    struct FFootholdSearchBudget
    {
        int32 RemainingLegs = 0;
        uint64 Solve = 0;
    };

    // One search per this many enabled legs and solve, and never fewer than MinSearchLegs: a walker whose every ideal target
    // fails at once (a centipede on a beam) would otherwise run the full fan on every leg in the same frame. A hold serves for
    // as long as the ideal target is unusable, so a failing leg searches once and then holds, and the budget binds only while
    // many legs fail together. The legs left over keep the contact grace this solve and search on a later one, round-robin.
    constexpr auto EnabledLegsPerSearch = 2;
    constexpr auto MinSearchLegs = 4;
    // A search that found nothing usable is not repeated for this many solves: the ground around the leg barely changes in
    // that time, and every repeat is a full fan. The contact grace runs meanwhile.
    constexpr auto SearchBackoffSolves = uint64{3};
    // How far a search candidate may lie inboard of its hip, along the direction to its rest foot, before it would fold the
    // leg under the body: a little slack keeps a foothold straight under a hip that stands over the edge it grips.
    constexpr auto InboardToleranceShareOfReach = 0.1f;
    // How far inside the widest hip a foothold must lie to count as under the body: the hips sit on the body's surface, so
    // a foothold at a hip's own lateral offset is beside the body, not under it.
    constexpr auto UnderBodyMargin = 5.0f;
    // A face hold is checked with one ray this far to either side of it along its normal: the hold lies on the face, so a
    // short ray meets it again, and a short start stays out of solids next to the face.
    constexpr auto FaceHoldProbeHalfSpan = 10.0f;
    constexpr auto DerivedSearchRadiusShareOfReach = 0.3f;
    constexpr auto DerivedKeepRadiusInStepThresholds = 1.5f;
    constexpr auto FrontFloorShareOfReach = 0.1f;
    constexpr auto InwardCloseDepthInRestDrops = 1.5f;
    constexpr auto InwardFarDepthInRestDrops = 4.0f;
    constexpr auto OutwardHeightInRestDrops = 2.0f;
    constexpr auto RingPoints = 8;
    constexpr auto LandingLiftMaxDegrees = 45.0f;

    auto
        Get_RestDrop(
            const FCk_ProceduralLeg_Placement& InPlacement)
        -> float
    {
        return static_cast<float>(FMath::Abs(InPlacement.Get_HipLocal().Z - InPlacement.Get_RestFootLocal().Z));
    }

    auto
        Get_LegFoothold(
            ck::EProceduralFootholdSource InSource)
        -> ECk_ProceduralLeg_Foothold
    {
        switch (InSource)
        {
            case ck::EProceduralFootholdSource::None: return ECk_ProceduralLeg_Foothold::None;
            case ck::EProceduralFootholdSource::Ideal: return ECk_ProceduralLeg_Foothold::Ideal;
            case ck::EProceduralFootholdSource::Held: return ECk_ProceduralLeg_Foothold::Held;
            case ck::EProceduralFootholdSource::Front: return ECk_ProceduralLeg_Foothold::Front;
            case ck::EProceduralFootholdSource::Inward: return ECk_ProceduralLeg_Foothold::Inward;
            case ck::EProceduralFootholdSource::Outward: return ECk_ProceduralLeg_Foothold::Outward;
            case ck::EProceduralFootholdSource::Ring: return ECk_ProceduralLeg_Foothold::Ring;
        }
        return ECk_ProceduralLeg_Foothold::None;
    }

    // The trace from the hip to InPoint: the solid it meets farther than the occlusion tolerance from InPoint. A trace that
    // starts inside a solid (fraction 0, a hip inside the body's own collider) tells nothing, so it finds no occluder.
    auto
        Get_Occluder(
            const FFootholdQuery& InQuery,
            const FVector& InPoint,
            int32& InOutRayCount)
        -> TOptional<FCk_Jolt_HitResult>
    {
        const auto Hit = UCk_Utils_JoltQuery_UE::Get_RayCast(InQuery.World, InQuery.Hip, InPoint, InQuery.Probe->Get_QueryFilter());
        ++InOutRayCount;

        const auto Blocks = Hit.Get_HasHit() && Hit.Get_Fraction() > 0.0f && NOT Hit.Get_Position().ContainsNaN()
            && FVector::Dist(Hit.Get_Position(), InPoint) > InQuery.Foothold->Get_OcclusionTolerance();
        if (NOT Blocks)
        { return {}; }

        return Hit;
    }

    // A face cast (inward, outward): the first surface on the segment that faces it.
    auto
        DoCast_Segment(
            const FFootholdQuery& InQuery,
            const FVector& InStart,
            const FVector& InEnd,
            int32& InOutRayCount)
        -> TOptional<FCk_Jolt_HitResult>
    {
        const auto Direction = (InEnd - InStart).GetSafeNormal();
        if (Direction.IsNearlyZero())
        { return {}; }

        const auto Hit = UCk_Utils_JoltQuery_UE::Get_RayCast(InQuery.World, InStart, InEnd, InQuery.Probe->Get_QueryFilter());
        ++InOutRayCount;

        if (NOT Get_TrustedHit(Hit, Direction))
        { return {}; }

        return Hit;
    }

    // Validation of a hit, in order: Unreachable beyond the force-step reach (beyond the target reach at once for a face
    // hit), TooSteep for a search candidate beyond the max angle, Inboard for a search candidate off the leg's own side of
    // its hip, UnderBody for a search candidate under the body, Occluded when the hip cannot see it, else Usable.
    auto
        DoValidate(
            const FFootholdQuery& InQuery,
            FFootholdCandidate& InOutCandidate,
            EFootholdStage InStage,
            int32& InOutRayCount)
        -> void
    {
        const auto HipDistance = FVector::Dist(InOutCandidate.Position, InQuery.Hip);
        const auto LimitFraction = InOutCandidate.Cast == ck::EProceduralFootholdCast::Face
            ? InQuery.Step->Get_TargetReachFraction()
            : InQuery.Step->Get_ForceStepReachFraction();
        if (HipDistance > LimitFraction * InQuery.Reach)
        {
            InOutCandidate.Verdict = ck::EProceduralFootholdVerdict::Unreachable;
            return;
        }

        const auto SupportNormal = InQuery.Basis.Inverse().RotateVector(InOutCandidate.Normal);
        if (InStage == EFootholdStage::Search && NOT ck::Get_IsFootholdLevelEnough(SupportNormal, InQuery.Foothold->Get_MaxAngle()))
        {
            InOutCandidate.Verdict = ck::EProceduralFootholdVerdict::TooSteep;
            return;
        }

        if (InStage == EFootholdStage::Search && NOT ck::Get_IsFootholdOnOwnSide(InOutCandidate.Position - InQuery.Hip,
                InQuery.Outboard, InboardToleranceShareOfReach * InQuery.Reach))
        {
            InOutCandidate.Verdict = ck::EProceduralFootholdVerdict::Inboard;
            return;
        }

        const auto FromBody = InQuery.Basis.Inverse().RotateVector(InOutCandidate.Position - InQuery.BodyLocation);
        const auto FromHip = InQuery.Basis.Inverse().RotateVector(InOutCandidate.Position - InQuery.Hip);
        if (InStage == EFootholdStage::Search && ck::Get_IsFootholdUnderBody(static_cast<float>(FromBody.Y),
                static_cast<float>(-FromHip.Z), InQuery.MaxHipLateral, InQuery.RestDrop, UnderBodyMargin))
        {
            InOutCandidate.Verdict = ck::EProceduralFootholdVerdict::UnderBody;
            return;
        }

        InOutCandidate.Occluder = Get_Occluder(InQuery, InOutCandidate.Position, InOutRayCount);
        InOutCandidate.Verdict = InOutCandidate.Occluder.IsSet()
            ? ck::EProceduralFootholdVerdict::Occluded
            : ck::EProceduralFootholdVerdict::Usable;
    }

    // A down ray under InPoint. With ClampTowardHip, ground farther than the target reach from the hip is clamped toward the
    // hip like the query point and probed once more there: the solver's own clamp keeps a target's support-frame height,
    // which across a convex bend lies inside the ground. Ground the second probe misses stays Unreachable, as out of reach.
    auto
        DoProbe_Down(
            const FFootholdQuery& InQuery,
            const FVector& InPoint,
            ck::EProceduralFootholdSource InSource,
            EGroundProbeAttempts InAttempts,
            EFootholdReprobe InReprobe,
            EFootholdStage InStage,
            int32& InOutRayCount,
            FCk_ProceduralAnimation_DebugProbe* InDebugProbe)
        -> FFootholdCandidate
    {
        const auto Up = InQuery.Basis.GetAxisZ();
        const auto ProbeAt = [&](const FVector& InProbePoint) -> TOptional<FCk_Jolt_HitResult>
        {
            const auto Radial = FVector::VectorPlaneProject(InProbePoint - InQuery.BodyLocation, Up).GetSafeNormal();
            return Get_GroundHit(InQuery.World, InProbePoint, Up, Radial, *InQuery.Probe, InAttempts, InOutRayCount, InDebugProbe);
        };

        auto Candidate = FFootholdCandidate{InPoint, Up, InSource, ck::EProceduralFootholdVerdict::Miss, ck::EProceduralFootholdCast::Down};
        auto Hit = ProbeAt(InPoint);
        if (NOT Hit.IsSet())
        { return Candidate; }

        Candidate.Position = Hit->Get_Position();
        Candidate.Normal = Hit->Get_Normal().GetSafeNormal();

        const auto TargetLimit = InQuery.Step->Get_TargetReachFraction() * InQuery.Reach;
        if (InReprobe == EFootholdReprobe::ClampTowardHip && FVector::Dist(Candidate.Position, InQuery.Hip) > TargetLimit)
        {
            const auto InverseBasis = InQuery.Basis.Inverse();
            const auto Reprobed = ProbeAt(InQuery.Basis.RotateVector(ck::FProceduralGaitSolver::ClampToReach(
                InverseBasis.RotateVector(InQuery.Hip), InverseBasis.RotateVector(Candidate.Position), TargetLimit)));
            if (NOT Reprobed.IsSet())
            {
                Candidate.Verdict = ck::EProceduralFootholdVerdict::Unreachable;
                return Candidate;
            }

            Candidate.Position = Reprobed->Get_Position();
            Candidate.Normal = Reprobed->Get_Normal().GetSafeNormal();
        }

        DoValidate(InQuery, Candidate, InStage, InOutRayCount);
        return Candidate;
    }

    auto
        DoProbe_Face(
            const FFootholdQuery& InQuery,
            const TOptional<FCk_Jolt_HitResult>& InHit,
            const FVector& InMissPosition,
            ck::EProceduralFootholdSource InSource,
            EFootholdStage InStage,
            int32& InOutRayCount)
        -> FFootholdCandidate
    {
        auto Candidate = FFootholdCandidate{InMissPosition, InQuery.Basis.GetAxisZ(), InSource, ck::EProceduralFootholdVerdict::Miss,
            ck::EProceduralFootholdCast::Face};
        if (NOT InHit.IsSet())
        { return Candidate; }

        Candidate.Position = InHit->Get_Position();
        Candidate.Normal = InHit->Get_Normal().GetSafeNormal();
        DoValidate(InQuery, Candidate, InStage, InOutRayCount);
        return Candidate;
    }

    auto
        DoProbe_AlongNormal(
            const FFootholdQuery& InQuery,
            const ck::FProceduralFootholdState& InHold,
            int32& InOutRayCount,
            FCk_ProceduralAnimation_DebugProbe* InDebugProbe)
        -> FFootholdCandidate
    {
        const auto Normal = InHold.Get_Normal().GetSafeNormal();
        const auto Start = InHold.Get_Position() + Normal * FaceHoldProbeHalfSpan;
        const auto End = InHold.Get_Position() - Normal * FaceHoldProbeHalfSpan;
        const auto Hit = DoCast_Segment(InQuery, Start, End, InOutRayCount);

        if (ck::IsValid(InDebugProbe, ck::IsValid_Policy_NullptrOnly{}))
        {
            constexpr auto SingleAttempt = 1;
            InDebugProbe->Set_Start(Start)
                .Set_End(End)
                .Set_AttemptCount(SingleAttempt)
                .Set_Hit(Hit.IsSet())
                .Set_HitFraction(Hit.IsSet() ? Hit->Get_Fraction() : 1.0f)
                .Set_HitPosition(Hit.IsSet() ? Hit->Get_Position() : End)
                .Set_HitNormal(Hit.IsSet() ? Hit->Get_Normal() : FVector::ZeroVector);
        }

        return DoProbe_Face(InQuery, Hit, InHold.Get_Position(), ck::EProceduralFootholdSource::Held, EFootholdStage::IdealOrHeld,
            InOutRayCount);
    }

    auto
        DoRecord_Candidate(
            const FFootholdCandidate& InCandidate,
            TArray<FFootholdCandidate, TInlineAllocator<16>>& OutCandidates,
            FCk_ProceduralAnimation_DebugLeg* InOutDebugLeg)
        -> void
    {
        OutCandidates.Add(InCandidate);
        if (ck::Is_NOT_Valid(InOutDebugLeg, ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        InOutDebugLeg->Get_Footholds().Add(FCk_ProceduralAnimation_DebugFoothold{}
            .Set_Position(InCandidate.Position)
            .Set_Normal(InCandidate.Normal)
            .Set_Source(InCandidate.Source)
            .Set_Verdict(InCandidate.Verdict));
    }

    auto
        DoFinish(
            const FFootholdCandidate& InCandidate,
            int32 InIndex,
            ck::EProceduralFootholdVerdict InIdealVerdict,
            FCk_ProceduralAnimation_DebugLeg* InOutDebugLeg)
        -> FFoothold
    {
        if (ck::IsValid(InOutDebugLeg, ck::IsValid_Policy_NullptrOnly{}))
        {
            InOutDebugLeg->Set_ChosenFoothold(InIndex)
                .Set_FootholdSource(InCandidate.Source);
        }
        return FFoothold{InCandidate.Position, InCandidate.Normal, InCandidate.Source, InIdealVerdict};
    }

    // The held foothold is re-validated at its own point and the ideal target probed as the gait always probed it. A
    // usable ideal is the target, unless a usable hold lies within the keep radius of it: then the hold stays, so the
    // foot does not hop between two spots that agree. An unusable ideal leaves a usable hold as the target while the
    // two lie within the leg's force-step reach of each other, because an ideal hanging beside a beam or over a gap
    // would otherwise drop the hold its own search found; farther apart, the body has walked away from the hold (up a
    // log, over a crest) and a leg kept on it re-plants behind its rest point, so the hold is dropped. Only when
    // neither is usable, the budget has a search left and the leg is not backing off a search that found nothing, does
    // the search run; the least-cost usable candidate of its own becomes the held foothold. Every ray increments
    // InOutRayCount; the debug leg, when given, receives every candidate.
    auto
        Get_Foothold(
            const FFootholdQuery& InQuery,
            ck::FProceduralFootholdState& InOutState,
            FFootholdSearchBudget& InOutBudget,
            int32& InOutRayCount,
            FCk_ProceduralAnimation_DebugLeg* InOutDebugLeg)
        -> FFoothold
    {
        auto Candidates = TArray<FFootholdCandidate, TInlineAllocator<16>>{};
        if (ck::IsValid(InOutDebugLeg, ck::IsValid_Policy_NullptrOnly{}))
        {
            InOutDebugLeg->Get_Footholds().Reset();
            InOutDebugLeg->Set_ChosenFoothold(INDEX_NONE)
                .Set_FootholdSource(ck::EProceduralFootholdSource::None);
        }

        const auto& FootholdTuning = *InQuery.Foothold;
        auto* DebugProbe = ck::IsValid(InOutDebugLeg, ck::IsValid_Policy_NullptrOnly{}) ? &InOutDebugLeg->Get_Probe() : nullptr;

        auto Held = FFootholdCandidate{};
        auto HeldIndex = int32{INDEX_NONE};
        if (InOutState.Get_Hold() == ck::EProceduralFootholdHold::Held)
        {
            Held = InOutState.Get_Cast() == ck::EProceduralFootholdCast::Face
                ? DoProbe_AlongNormal(InQuery, InOutState, InOutRayCount, DebugProbe)
                : DoProbe_Down(InQuery, InOutState.Get_Position(), ck::EProceduralFootholdSource::Held,
                    EGroundProbeAttempts::RetryFromInsideSolid, EFootholdReprobe::None, EFootholdStage::IdealOrHeld, InOutRayCount,
                    DebugProbe);
            DoRecord_Candidate(Held, Candidates, InOutDebugLeg);
            HeldIndex = Candidates.Num() - 1;
        }
        const auto HoldIsUsable = HeldIndex != INDEX_NONE && Held.Verdict == ck::EProceduralFootholdVerdict::Usable;
        if (HeldIndex != INDEX_NONE && NOT HoldIsUsable)
        { InOutState = ck::FProceduralFootholdState{}; }

        const auto Ideal = DoProbe_Down(InQuery, InQuery.Ideal, ck::EProceduralFootholdSource::Ideal,
            EGroundProbeAttempts::Every, EFootholdReprobe::ClampTowardHip, EFootholdStage::IdealOrHeld, InOutRayCount, DebugProbe);
        DoRecord_Candidate(Ideal, Candidates, InOutDebugLeg);
        const auto IdealIndex = Candidates.Num() - 1;

        if (Ideal.Verdict == ck::EProceduralFootholdVerdict::Usable)
        {
            const auto KeepRadius = FootholdTuning.Get_KeepRadius() > 0.0f
                ? FootholdTuning.Get_KeepRadius()
                : DerivedKeepRadiusInStepThresholds * InQuery.Step->Get_Threshold();
            if (HoldIsUsable && FVector::Dist(InOutState.Get_Position(), InQuery.Ideal) <= KeepRadius)
            { return DoFinish(Held, HeldIndex, Ideal.Verdict, InOutDebugLeg); }

            InOutState = ck::FProceduralFootholdState{};
            return DoFinish(Ideal, IdealIndex, Ideal.Verdict, InOutDebugLeg);
        }

        const auto TrailLimit = InQuery.Step->Get_ForceStepReachFraction() * InQuery.Reach;
        if (HoldIsUsable && FVector::Dist(InOutState.Get_Position(), InQuery.Ideal) <= TrailLimit)
        { return DoFinish(Held, HeldIndex, Ideal.Verdict, InOutDebugLeg); }

        if (HoldIsUsable)
        { InOutState = ck::FProceduralFootholdState{}; }

        const auto Up = InQuery.Basis.GetAxisZ();
        const auto NothingUsable = FFoothold{InQuery.Ideal, Up, ck::EProceduralFootholdSource::None, Ideal.Verdict};
        const auto& SearchedSolve = InOutState.Get_SearchedSolve();
        const auto BackingOff = SearchedSolve.IsSet() && InOutBudget.Solve - SearchedSolve.GetValue() <= SearchBackoffSolves;
        if (BackingOff || InOutBudget.RemainingLegs <= 0)
        { return NothingUsable; }

        --InOutBudget.RemainingLegs;
        const auto SearchRadius = FootholdTuning.Get_SearchRadius() > 0.0f
            ? FootholdTuning.Get_SearchRadius()
            : DerivedSearchRadiusShareOfReach * InQuery.Reach;

        if (Ideal.Verdict == ck::EProceduralFootholdVerdict::Occluded && Ideal.Occluder.IsSet())
        {
            const auto& Occluder = Ideal.Occluder.GetValue();
            DoRecord_Candidate(DoProbe_Face(InQuery, TOptional<FCk_Jolt_HitResult>{Occluder}, Occluder.Get_Position(),
                ck::EProceduralFootholdSource::Front, EFootholdStage::Search, InOutRayCount), Candidates, InOutDebugLeg);

            const auto InFront = Occluder.Get_Position()
                + (InQuery.Hip - Occluder.Get_Position()).GetSafeNormal() * (FrontFloorShareOfReach * InQuery.Reach);
            DoRecord_Candidate(DoProbe_Down(InQuery, InFront, ck::EProceduralFootholdSource::Front, EGroundProbeAttempts::Every,
                EFootholdReprobe::ClampTowardHip, EFootholdStage::Search, InOutRayCount, nullptr), Candidates, InOutDebugLeg);
        }

        for (const auto Depth : {InwardCloseDepthInRestDrops, InwardFarDepthInRestDrops})
        {
            const auto Focal = InQuery.Hip - Up * (Depth * InQuery.RestDrop);
            const auto End = Focal + (Focal - InQuery.Ideal).GetSafeNormal() * InQuery.Probe->Get_Down();
            DoRecord_Candidate(DoProbe_Face(InQuery, DoCast_Segment(InQuery, InQuery.Ideal, End, InOutRayCount), InQuery.Ideal,
                ck::EProceduralFootholdSource::Inward, EFootholdStage::Search, InOutRayCount), Candidates, InOutDebugLeg);
        }

        {
            const auto Origin = InQuery.Hip + Up * (OutwardHeightInRestDrops * InQuery.RestDrop);
            const auto End = InQuery.Ideal + (InQuery.Ideal - Origin).GetSafeNormal() * InQuery.Probe->Get_Down();
            DoRecord_Candidate(DoProbe_Face(InQuery, DoCast_Segment(InQuery, Origin, End, InOutRayCount), InQuery.Ideal,
                ck::EProceduralFootholdSource::Outward, EFootholdStage::Search, InOutRayCount), Candidates, InOutDebugLeg);
        }

        const auto BasisX = InQuery.Basis.GetAxisX();
        const auto BasisY = InQuery.Basis.GetAxisY();
        for (auto Point = 0; Point < RingPoints; ++Point)
        {
            const auto Angle = UE_DOUBLE_TWO_PI * Point / RingPoints;
            const auto RingPoint = InQuery.Ideal + (BasisX * FMath::Cos(Angle) + BasisY * FMath::Sin(Angle)) * SearchRadius;
            DoRecord_Candidate(DoProbe_Down(InQuery, RingPoint, ck::EProceduralFootholdSource::Ring,
                EGroundProbeAttempts::RetryFromInsideSolid, EFootholdReprobe::ClampTowardHip, EFootholdStage::Search, InOutRayCount,
                nullptr), Candidates, InOutDebugLeg);
        }

        // The search picks among its own candidates only: a usable hold that reaches it is one the force-step bound just
        // dropped, and picking it again would hold it forever.
        const auto FirstSearchIndex = IdealIndex + 1;
        const auto SearchCandidates = TArrayView<const FFootholdCandidate>{Candidates}.RightChop(FirstSearchIndex);
        const auto InverseBasis = InQuery.Basis.Inverse();
        const auto SupportCandidates = ck::algo::Transform<TArray<ck::FProceduralFootholdCandidate, TInlineAllocator<16>>>(SearchCandidates,
        [&](const FFootholdCandidate& InCandidate)
        {
            return ck::FProceduralFootholdCandidate{InverseBasis.RotateVector(InCandidate.Position),
                InverseBasis.RotateVector(InCandidate.Normal), InCandidate.Source, InCandidate.Verdict};
        });
        const auto Settings = ck::FProceduralFootholdSettings{}
            .Set_SlopeWeight(FootholdTuning.Get_SlopeWeight())
            .Set_ContinuityWeight(FootholdTuning.Get_ContinuityWeight())
            .Set_MaxAngleDegrees(FootholdTuning.Get_MaxAngle());
        const auto SearchPick = ck::SelectProceduralFoothold(SupportCandidates, InverseBasis.RotateVector(InQuery.Ideal),
            InverseBasis.RotateVector(InQuery.Plant), InQuery.Planted, InQuery.Reach, Settings);

        if (SearchPick == INDEX_NONE)
        {
            InOutState.Set_SearchedSolve(InOutBudget.Solve);
            return NothingUsable;
        }

        const auto Chosen = FirstSearchIndex + SearchPick;
        const auto& Pick = Candidates[Chosen];
        InOutState = ck::FProceduralFootholdState{}
            .Set_Position(Pick.Position)
            .Set_Normal(Pick.Normal)
            .Set_Source(Pick.Source)
            .Set_Hold(ck::EProceduralFootholdHold::Held)
            .Set_Cast(Pick.Cast);
        return DoFinish(Pick, Chosen, Ideal.Verdict, InOutDebugLeg);
    }

    auto
        Get_IsPlantOccluded(
            const FFootholdQuery& InQuery,
            int32& InOutRayCount)
        -> bool
    {
        return Get_Occluder(InQuery, InQuery.Plant, InOutRayCount).IsSet();
    }

    // The ground under the point a swing will land on, as of the last solve. The stroke overshoot and the freeze push can
    // carry a target probed on a lower tread past the next riser; the swing lifts onto this ground when it is reachable.
    // A target on a face has no tread to lift onto, so its swing is not probed. The debug leg records the point and the ray.
    auto
        Get_LandingGroundHit(
            UWorld* InWorld,
            const FQuat& InBasis,
            const FVector& InHip,
            float InReach,
            const ck::FProceduralGaitLegSwing& InSwing,
            const FVector& InTargetNormal,
            const FCk_ProceduralGait_Probe& InProbe,
            const FCk_ProceduralGait_Step& InStep,
            int32& InOutRayCount,
            FCk_ProceduralAnimation_DebugLeg& InOutDebugLeg)
        -> TOptional<FCk_Jolt_HitResult>
    {
        if (NOT InSwing.Get_Active() || InSwing.Get_CatchStep())
        { return {}; }

        const auto Up = InBasis.GetAxisZ();
        if (FVector::DotProduct(InTargetNormal.GetSafeNormal(), Up) < FMath::Cos(FMath::DegreesToRadians(LandingLiftMaxDegrees)))
        { return {}; }

        const auto Landing = InBasis.RotateVector(InSwing.Get_LandingPoint());
        const auto ProbeStart = Landing + Up * InProbe.Get_Up();
        const auto ProbeEnd = Landing - Up * InProbe.Get_Down();
        const auto Hit = UCk_Utils_JoltQuery_UE::Get_RayCast(InWorld, ProbeStart, ProbeEnd, InProbe.Get_QueryFilter());
        ++InOutRayCount;

        constexpr auto SingleAttempt = 1;
        InOutDebugLeg.Set_LandingPointWorld(Landing);
        InOutDebugLeg.Get_LandingProbe().Set_Start(ProbeStart)
            .Set_End(ProbeEnd)
            .Set_AttemptCount(SingleAttempt)
            .Set_Hit(Hit.Get_HasHit())
            .Set_HitFraction(Hit.Get_Fraction())
            .Set_HitPosition(Hit.Get_Position())
            .Set_HitNormal(Hit.Get_Normal());

        if (NOT Get_TrustedHit(Hit, -Up) || FVector::Dist(Hit.Get_Position(), InHip) > InStep.Get_ForceStepReachFraction() * InReach)
        { return {}; }

        return Hit;
    }

    auto
        Get_LegBit(
            int32 InLegIndex)
        -> uint64
    {
        return uint64{1} << InLegIndex;
    }

    // The direction from the hip to its rest foot in the support plane: the leg's own side. Zero for a rest foot straight
    // under the hip.
    auto
        Get_Outboard(
            const FVector& InHip,
            const FVector& InRestFoot,
            const FVector& InUp)
        -> FVector
    {
        return FVector::VectorPlaneProject(InRestFoot - InHip, InUp).GetSafeNormal();
    }

    // The largest lateral offset of the given legs' hips from the body's centreline, in the body's frame: the body's
    // half-width when the hips sit on its surface.
    template <typename T_Predicate>
    auto
        Get_MaxHipLateral(
            const TArray<FCk_Handle_ProceduralLeg>& InLegs,
            T_Predicate InCounts)
        -> float
    {
        auto MaxHipLateral = 0.0f;
        for (const auto& Leg : InLegs)
        {
            if (NOT InCounts(Leg))
            { continue; }

            const auto& HipLocal = Leg.Get<ck::FFragment_ProceduralLeg_Params>().Get_Placement().Get_HipLocal();
            MaxHipLateral = FMath::Max(MaxHipLateral, static_cast<float>(FMath::Abs(HipLocal.Y)));
        }
        return MaxHipLateral;
    }

    auto
        Get_IsLegEnabled(
            const FCk_Handle_ProceduralLeg& InLeg)
        -> bool
    {
        return ck::IsValid(InLeg)
            && NOT InLeg.Has<ck::FTag_DestroyEntity_Initiate>()
            && NOT InLeg.Has<ck::FTag_ProceduralLeg_Disabled>();
    }

    auto
        Get_SearchBudgetLegs(
            int32 InEnabledLegs)
        -> int32
    {
        return FMath::Max(MinSearchLegs, InEnabledLegs / EnabledLegsPerSearch);
    }

    auto
        DoPublish_FootPhaseChange(
            FCk_Handle_ProceduralLeg& InLeg,
            const ck::FFragment_ProceduralLeg& InLegComp,
            ECk_ProceduralLeg_FootPhase InPreviousPhase,
            const FVector& InPreviousPosition,
            double InDeltaSeconds)
        -> void
    {
        const auto& Foot = InLegComp.Get_Foot();
        if (Foot.Get_Phase() == InPreviousPhase)
        { return; }

        if (Foot.Get_Phase() == ECk_ProceduralLeg_FootPhase::Planted)
        {
            const auto LandingSpeed = InDeltaSeconds > 0.0 ? (Foot.Get_Position() - InPreviousPosition).Size() / InDeltaSeconds : 0.0;
            const auto Footfall = FCk_ProceduralLeg_Footfall{Foot.Get_Position(), Foot.Get_Normal(), Foot.Get_Contact(),
                static_cast<float>(LandingSpeed)};
            ck::UUtils_Signal_OnProceduralLeg_Planted::Broadcast(InLeg, ck::MakePayload(InLeg, Footfall));
            return;
        }

        constexpr auto LiftSpeed = 0.0f;
        const auto Footfall = FCk_ProceduralLeg_Footfall{Foot.Get_Position(), Foot.Get_Normal(), Foot.Get_Contact(), LiftSpeed};
        ck::UUtils_Signal_OnProceduralLeg_Lifted::Broadcast(InLeg, ck::MakePayload(InLeg, Footfall));
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_ProceduralGait_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProceduralGait_Tunables& InTunables,
            FFragment_ProceduralGait& InGaitComp,
            FFragment_ProceduralGait_Debug& InDebugComp,
            const FFragment_Transform& InTransform)
        -> void
    {
        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (ck::Is_NOT_Valid(World))
        { return; }

        const auto& Body = InTransform.Get_Transform();
        const auto BodyFinite = NOT Body.ContainsNaN();
        CK_ENSURE_IF_NOT(BodyFinite,
            TEXT("Procedural gait [{}] body transform contains NaN; feature is failed."), InHandle)
        {
            InHandle.Add<FFragment_ProceduralGait_Failure>(ECk_ProceduralGait_Failure::NanBody);
            InHandle.Remove<MarkedDirtyBy>();
            return;
        }

        const auto Basis = Body.GetRotation();
        const auto InverseBasis = Basis.Inverse();
        const auto Up = Basis.GetAxisZ();

        auto RayCount = int32{0};
        // Setup runs once, so every leg searches: a pick made here is held from the first update on.
        auto SearchBudget = ck_procedural_gait::FFootholdSearchBudget{
            .RemainingLegs = TNumericLimits<int32>::Max(),
            .Solve = InGaitComp._SolveSequence};
        const auto MaxHipLateral = ck_procedural_gait::Get_MaxHipLateral(InGaitComp._Legs,
            [](const FCk_Handle_ProceduralLeg& InLeg) { return ck::IsValid(InLeg); });
        auto InitialFeet = TArray<FVector, TInlineAllocator<8>>{};
        InitialFeet.Reserve(InGaitComp._Legs.Num());
        for (auto Index = 0; Index < InGaitComp._Legs.Num(); ++Index)
        {
            auto& Leg = InGaitComp._Legs[Index];
            if (ck::Is_NOT_Valid(Leg))
            {
                InitialFeet.Add(InverseBasis.RotateVector(Body.GetLocation()));
                continue;
            }

            const auto& Params = Leg.Get<FFragment_ProceduralLeg_Params>();
            const auto& Placement = Params.Get_Placement();
            const auto Neutral = Body.TransformPosition(Placement.Get_RestFootLocal());
            const auto Hip = Body.TransformPosition(Placement.Get_HipLocal());
            constexpr auto NotYetPlanted = false;
            const auto Query = ck_procedural_gait::FFootholdQuery{
                .World = World,
                .Basis = Basis,
                .BodyLocation = Body.GetLocation(),
                .Hip = Hip,
                .Outboard = ck_procedural_gait::Get_Outboard(Hip, Neutral, Up),
                .MaxHipLateral = MaxHipLateral,
                .Ideal = Neutral,
                .Plant = Neutral,
                .Planted = NotYetPlanted,
                .Reach = ck_procedural_gait_utils::Get_Reach(Params.Get_Chain()),
                .RestDrop = ck_procedural_gait::Get_RestDrop(Placement),
                .Probe = &InTunables.Get_Probe(),
                .Step = &InTunables.Get_Step(),
                .Foothold = &InTunables.Get_Foothold()};
            const auto Foothold = ck_procedural_gait::Get_Foothold(Query, InGaitComp._Footholds[Index], SearchBudget, RayCount,
                &InDebugComp._ScratchLegs[Index]);
            const auto Trusted = Foothold.Source != EProceduralFootholdSource::None;
            const auto Position = Trusted ? Foothold.Position : Neutral;
            const auto Normal = Trusted ? Foothold.Normal : Up;

            // Update freezes a leg disabled before Add from this published foot, so it must already hold the probed pose.
            Leg.Get<FFragment_ProceduralLeg>()._Foot.Set_Position(Position)
                .Set_Normal(Normal)
                .Set_Rotation(Basis)
                .Set_SwingAlpha(0.0f)
                .Set_Phase(ECk_ProceduralLeg_FootPhase::Planted)
                .Set_Contact(Trusted ? ECk_ProceduralLeg_FootContact::Trusted : ECk_ProceduralLeg_FootContact::Guessed)
                .Set_Foothold(ck_procedural_gait::Get_LegFoothold(Foothold.Source));

            InitialFeet.Add(InverseBasis.RotateVector(Position));
        }
        InDebugComp._RaysLastSolve = RayCount;

        const auto SolverReset = InGaitComp._Solver.Reset(InitialFeet);
        CK_ENSURE_IF_NOT(SolverReset,
            TEXT("Procedural gait [{}] solver rejected the initial foot positions; feature is failed."), InHandle)
        {
            InHandle.Add<FFragment_ProceduralGait_Failure>(ECk_ProceduralGait_Failure::SolverReset);
            InHandle.Remove<MarkedDirtyBy>();
            return;
        }

        InGaitComp._Basis = Basis;
        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ProceduralGait_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_ProceduralGait_Tunables& InTunables,
            FFragment_ProceduralGait& InGaitComp,
            FFragment_ProceduralGait_Debug& InDebugComp,
            FFragment_ProceduralGait_Requests& InRequestsComp)
        -> void
    {
        auto Requests = MoveTemp(InRequestsComp._Requests);
        InRequestsComp._Requests.Reset();

        algo::ForEachRequest(Requests, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            auto Result = ECk_Request_OperationResult::Failed_Cancelled;
            const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

            if (InHandle.Has<FTag_DestroyEntity_Initiate>())
            { return; }

            DoHandleRequest(InHandle, InTunables, InGaitComp, InDebugComp, InRequest);
            Result = ECk_Request_OperationResult::Succeeded;
        }), policy::DontResetContainer{});

        if (InRequestsComp._Requests.IsEmpty())
        { InHandle.Remove<MarkedDirtyBy>(); }
    }

    auto
        FProcessor_ProceduralGait_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_ProceduralGait_Tunables& InTunables,
            FFragment_ProceduralGait& InGaitComp,
            FFragment_ProceduralGait_Debug& InDebugComp,
            const FCk_Request_ProceduralGait_ApplyPreset& InRequest)
        -> void
    {
        InTunables = FFragment_ProceduralGait_Tunables{InRequest.Get_Timing(), InRequest.Get_Step(), InRequest.Get_Probe(),
            InRequest.Get_Foothold()};

        const auto Built = UCk_Utils_ProceduralGait_UE::DoBuild_SolverSettings(InTunables, InGaitComp._Legs, InGaitComp._EnabledMask);
        InGaitComp._Solver.Set_Settings(Built.Get_Settings());
        InDebugComp._ReachCadenceFloor = Built.Get_ReachCadenceFloor();
        InDebugComp._ReachSkippedLegs = Built.Get_ReachSkippedLegs();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ProceduralGait_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProceduralGait_Tunables& InTunables,
            FFragment_ProceduralGait& InGaitComp,
            FFragment_ProceduralGait_Debug& InDebugComp,
            const FFragment_Transform& InTransform)
        -> void
    {
        const auto Dt = InDeltaT.Get_Seconds();
        if (NOT FMath::IsFinite(Dt) || Dt < 0.0)
        { return; }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (ck::Is_NOT_Valid(World))
        { return; }

        const auto& Body = InTransform.Get_Transform();
        const auto BodyFinite = NOT Body.ContainsNaN();
        CK_ENSURE_IF_NOT(BodyFinite,
            TEXT("Procedural gait [{}] body transform contains NaN; feature is failed."), InHandle)
        {
            InHandle.Add<FFragment_ProceduralGait_Failure>(ECk_ProceduralGait_Failure::NanBody);
            return;
        }

        const auto& Timing = InTunables.Get_Timing();
        const auto& Step = InTunables.Get_Step();
        const auto& Probe = InTunables.Get_Probe();
        const auto Basis = Body.GetRotation();
        const auto InverseBasis = Basis.Inverse();
        const auto Up = Basis.GetAxisZ();
        const auto Velocity = InGaitComp._VelocityTracker.Update(Body.GetLocation(), InDeltaT);
        const auto PlanarVelocity = InverseBasis.RotateVector(FVector::VectorPlaneProject(Velocity, Up));
        const auto YawRate = InGaitComp._YawRateTracker.Update(InGaitComp._Basis, Basis, InDeltaT);
        // ComputeLeadQuery caps the whole lead, travel and turn together, at the max velocity lead.
        const auto Lead = FVector::VectorPlaneProject(Velocity, Up) * Timing.Get_StepDuration().Get_Seconds();

        InGaitComp._Solver.TransformState(InverseBasis * InGaitComp._Basis);

        const auto LegCount = InGaitComp._Legs.Num();

        auto Inputs = TArray<FProceduralGaitLegInput, TInlineAllocator<8>>{};
        Inputs.SetNum(LegCount);
        auto Outputs = TArray<FProceduralGaitLegOutput, TInlineAllocator<8>>{};
        Outputs.SetNum(LegCount);

        auto AllLost = true;
        auto MeanFootRadius = 0.0;
        auto EnabledCount = 0;
        auto EnabledMask = ~uint64{0};
        auto RayCount = int32{0};
        auto SearchBudget = ck_procedural_gait::FFootholdSearchBudget{
            .RemainingLegs = ck_procedural_gait::Get_SearchBudgetLegs(algo::CountIf(InGaitComp._Legs, &ck_procedural_gait::Get_IsLegEnabled)),
            .Solve = InGaitComp._SolveSequence};
        auto LastSearchedLeg = int32{INDEX_NONE};
        const auto MaxHipLateral = ck_procedural_gait::Get_MaxHipLateral(InGaitComp._Legs, &ck_procedural_gait::Get_IsLegEnabled);
        // The legs are visited from the one after the last leg that searched, so the search budget goes round the legs.
        for (auto Visit = 0; Visit < LegCount; ++Visit)
        {
            const auto Index = (InGaitComp._NextSearchLeg + Visit) % LegCount;
            auto& Leg = InGaitComp._Legs[Index];
            auto& Input = Inputs[Index];
            auto& DebugLeg = InDebugComp._ScratchLegs[Index];
            const auto WasEnabled = (InGaitComp._EnabledMask & ck_procedural_gait::Get_LegBit(Index)) != 0;
            const auto LegValid = ck::IsValid(Leg);
            const auto Enabled = ck_procedural_gait::Get_IsLegEnabled(Leg);

            Input.Set_Enabled(Enabled);
            DebugLeg.Set_Enabled(Enabled)
                .Set_LandingPointWorld(FVector::ZeroVector)
                .Set_LandingProbe(FCk_ProceduralAnimation_DebugProbe{})
                .Set_ChosenFoothold(INDEX_NONE)
                .Set_FootholdSource(EProceduralFootholdSource::None)
                .Set_PlantOccluded(false);
            DebugLeg.Get_Footholds().Reset();

            if (NOT LegValid)
            {
                EnabledMask &= ~ck_procedural_gait::Get_LegBit(Index);
                continue;
            }

            const auto& Params = Leg.Get<FFragment_ProceduralLeg_Params>();
            const auto& Placement = Params.Get_Placement();
            auto& LegComp = Leg.Get<FFragment_ProceduralLeg>();
            const auto Neutral = Body.TransformPosition(Placement.Get_RestFootLocal());
            const auto Hip = Body.TransformPosition(Placement.Get_HipLocal());
            Input.Set_PhaseOffset(Placement.Get_PhaseOffset())
                .Set_StepThresholdScale(Placement.Get_StepThresholdScale());
            DebugLeg.Set_Id(Params.Get_Id());
            DebugLeg.Get_Targeting().Set_HipWorld(Hip)
                .Set_NeutralWorld(Neutral);

            if (NOT Enabled)
            {
                EnabledMask &= ~ck_procedural_gait::Get_LegBit(Index);

                if (WasEnabled)
                {
                    Leg.AddOrGet<FFragment_ProceduralLeg_FrozenPose>()
                        .Set_FootLocal(Body.InverseTransformPosition(LegComp._Foot.Get_Position()))
                        .Set_RotationLocal((InverseBasis * LegComp._Foot.Get_Rotation()).GetNormalized());
                }

                const auto& Frozen = Leg.Get<FFragment_ProceduralLeg_FrozenPose>();
                const auto PreviousPhase = LegComp._Foot.Get_Phase();
                const auto PreviousPosition = LegComp._Foot.Get_Position();
                LegComp._Foot.Set_Position(Body.TransformPosition(Frozen.Get_FootLocal()))
                    .Set_Rotation((Basis * Frozen.Get_RotationLocal()).GetNormalized())
                    .Set_Phase(ECk_ProceduralLeg_FootPhase::Planted)
                    .Set_SwingAlpha(0.0f)
                    .Set_Contact(ECk_ProceduralLeg_FootContact::Guessed)
                    .Set_Foothold(ECk_ProceduralLeg_Foothold::None);

                // A detached leg stays valid until its destruction completes, and its last pose is frozen here too;
                // freezing a swinging foot as it goes is not a plant.
                if (NOT Leg.Has<FTag_DestroyEntity_Initiate>())
                { ck_procedural_gait::DoPublish_FootPhaseChange(Leg, LegComp, PreviousPhase, PreviousPosition, Dt); }

                // The frozen pose is the source of truth so a re-enabled leg swings from where it is drawn.
                // On the transition frame the solver's own reconcile freezes this same pose.
                const auto SolverHoldsDisabledLeg = NOT InGaitComp._Solver.IsLegEnabled(Index);
                if (SolverHoldsDisabledLeg)
                {
                    const auto PoseSynced = InGaitComp._Solver.SetDisabledPose(Index,
                        InverseBasis.RotateVector(LegComp._Foot.Get_Position()),
                        (InverseBasis * LegComp._Foot.Get_Rotation()).GetNormalized(),
                        InverseBasis.RotateVector(Up));

                    CK_ENSURE_IF_NOT(PoseSynced,
                        TEXT("Procedural gait [{}] could not sync the frozen pose of disabled leg [{}]; feature is failed."),
                        InHandle, Index)
                    {
                        InHandle.Add<FFragment_ProceduralGait_Failure>(ECk_ProceduralGait_Failure::DisabledPoseSync);
                        return;
                    }
                }
                continue;
            }

            if (NOT WasEnabled)
            { Leg.Try_Remove<FFragment_ProceduralLeg_FrozenPose>(); }

            ++EnabledCount;
            const auto Reach = ck_procedural_gait_utils::Get_Reach(Params.Get_Chain());
            const auto HipSupport = InverseBasis.RotateVector(Hip);
            const auto Query = FProceduralGaitSolver::ComputeLeadQuery(Neutral, Lead, Body.GetLocation(), Up, YawRate,
                Timing.Get_StepDuration(), Step.Get_MaxVelocityLead());
            const auto Ideal = Basis.RotateVector(FProceduralGaitSolver::ClampToReach(HipSupport,
                InverseBasis.RotateVector(Query), Step.Get_TargetReachFraction() * Reach));
            DebugLeg.Get_Targeting().Set_QueryTarget(Ideal);
            MeanFootRadius += FVector::VectorPlaneProject(Placement.Get_RestFootLocal(), FVector::UpVector).Size();

            const auto Planted = LegComp._Foot.Get_Phase() == ECk_ProceduralLeg_FootPhase::Planted;
            const auto FootholdQuery = ck_procedural_gait::FFootholdQuery{
                .World = World,
                .Basis = Basis,
                .BodyLocation = Body.GetLocation(),
                .Hip = Hip,
                .Outboard = ck_procedural_gait::Get_Outboard(Hip, Neutral, Up),
                .MaxHipLateral = MaxHipLateral,
                .Ideal = Ideal,
                .Plant = LegComp._Foot.Get_Position(),
                .Planted = Planted,
                .Reach = Reach,
                .RestDrop = ck_procedural_gait::Get_RestDrop(Placement),
                .Probe = &Probe,
                .Step = &Step,
                .Foothold = &InTunables.Get_Foothold()};
            const auto SearchesLeft = SearchBudget.RemainingLegs;
            const auto Foothold = ck_procedural_gait::Get_Foothold(FootholdQuery, InGaitComp._Footholds[Index], SearchBudget, RayCount,
                &DebugLeg);
            if (SearchBudget.RemainingLegs < SearchesLeft)
            { LastSearchedLeg = Index; }
            const auto Trusted = Foothold.Source != EProceduralFootholdSource::None;
            const auto TargetIsFoothold = Trusted && Foothold.Source != EProceduralFootholdSource::Ideal;
            const auto ProbeAdvanced = InGaitComp._Probes[Index].Advance(Trusted, InDeltaT, Probe.Get_ContactGrace());
            CK_ENSURE_IF_NOT(ProbeAdvanced,
                TEXT("Procedural gait [{}] foot probe of leg [{}] rejected its elapsed time or contact grace; feature is failed."),
                InHandle, Index)
            {
                InHandle.Add<FFragment_ProceduralGait_Failure>(ECk_ProceduralGait_Failure::ProbeAdvance);
                return;
            }

            const auto ProbeState = InGaitComp._Probes[Index].Get_State();
            // A brief miss holds the plant by withholding a target. A prolonged loss lets the leg gather toward its CURRENT
            // rest pose, unless its hip cannot see that pose: then the plant holds until a solve finds a foothold, because a
            // foot never swings through a solid. The fallback never becomes trusted ground.
            const auto GatherWouldCrossASolid = ProbeState == EProceduralFootProbeState::Lost
                && Foothold.IdealVerdict == EProceduralFootholdVerdict::Occluded;
            const auto TargetValid = ProbeState != EProceduralFootProbeState::Guessing && NOT GatherWouldCrossASolid;
            AllLost &= ProbeState == EProceduralFootProbeState::Lost;
            const auto Position = Trusted ? Foothold.Position : Ideal;
            const auto Normal = Trusted ? Foothold.Normal : Up;
            const auto PlantOccluded = Planted && ck_procedural_gait::Get_IsPlantOccluded(FootholdQuery, RayCount);

            Input.Set_IdealTarget(InverseBasis.RotateVector(Position))
                .Set_GroundNormal(InverseBasis.RotateVector(Normal))
                .Set_FacingDirection(FVector::ForwardVector)
                .Set_TargetValid(TargetValid)
                .Set_ClearanceGroundZ(-FLT_MAX)
                .Set_Hip(HipSupport)
                .Set_Reach(Reach)
                .Set_PlantOccluded(PlantOccluded)
                .Set_TargetIsFoothold(TargetIsFoothold);

            if (LegComp._Foot.Get_Phase() == ECk_ProceduralLeg_FootPhase::Swinging)
            {
                const auto Foot = LegComp._Foot.Get_Position();
                const auto ClearanceHit = UCk_Utils_JoltQuery_UE::Get_RayCast(World,
                    Foot + Up * Probe.Get_Up(), Foot - Up * Probe.Get_Down(), Probe.Get_QueryFilter());
                ++RayCount;

                if (ck_procedural_gait::Get_TrustedHit(ClearanceHit, -Up))
                { Input.Set_ClearanceGroundZ(InverseBasis.RotateVector(ClearanceHit.Get_Position()).Z); }
            }

            const auto LandingGround = ck_procedural_gait::Get_LandingGroundHit(World, Basis, Hip, Reach,
                InGaitComp._Solver.GetLegState(Index).Get_Swing(), Normal, Probe, Step, RayCount, DebugLeg);
            if (LandingGround.IsSet())
            { Input.Set_LandingGroundZ(InverseBasis.RotateVector(LandingGround->Get_Position()).Z); }

            LegComp._Foot.Set_Contact(Trusted ? ECk_ProceduralLeg_FootContact::Trusted : ECk_ProceduralLeg_FootContact::Guessed)
                .Set_Foothold(ck_procedural_gait::Get_LegFoothold(Foothold.Source));
            DebugLeg.Set_PlantOccluded(PlantOccluded);
            DebugLeg.Get_Targeting().Set_IdealTarget(Position)
                .Set_TargetValid(TargetValid);
            DebugLeg.Get_Foot().Set_ContactTrusted(Trusted);
            DebugLeg.Get_Probe().Set_State(ProbeState)
                .Set_MissingContact(InGaitComp._Probes[Index].Get_MissingDuration());
        }
        InDebugComp._RaysLastSolve = RayCount;
        if (LastSearchedLeg != INDEX_NONE)
        { InGaitComp._NextSearchLeg = (LastSearchedLeg + 1) % LegCount; }

        if (EnabledMask != InGaitComp._EnabledMask)
        {
            InGaitComp._EnabledMask = EnabledMask;
            const auto Built = UCk_Utils_ProceduralGait_UE::DoBuild_SolverSettings(InTunables, InGaitComp._Legs, InGaitComp._EnabledMask);
            InGaitComp._Solver.Set_Settings(Built.Get_Settings());
            InDebugComp._ReachCadenceFloor = Built.Get_ReachCadenceFloor();
            InDebugComp._ReachSkippedLegs = Built.Get_ReachSkippedLegs();
            UUtils_Signal_OnProceduralGait_LegSetChanged::Broadcast(InHandle, MakePayload(InHandle, EnabledCount, LegCount));
        }

        auto Airborne = EnabledCount > 0 && AllLost;
        if (UCk_Utils_SurfaceMotion_UE::Has(InHandle))
        {
            Airborne = UCk_Utils_SurfaceMotion_UE::Get_Support(UCk_Utils_SurfaceMotion_UE::CastChecked(InHandle))
                == ECk_SurfaceMotion_Support::Airborne;
        }

        const auto CadenceSpeed = PlanarVelocity.Size() + FMath::Abs(YawRate) * MeanFootRadius / FMath::Max(EnabledCount, 1);
        const auto Solved = InGaitComp._Solver.Step(InDeltaT, CadenceSpeed, PlanarVelocity, Inputs, Outputs, Airborne);
        CK_ENSURE_IF_NOT(Solved, TEXT("Procedural gait [{}] solver rejected runtime inputs; feature is failed, planted state retained."), InHandle)
        {
            InHandle.Add<FFragment_ProceduralGait_Failure>(ECk_ProceduralGait_Failure::SolverStep);
            return;
        }

        for (auto Index = 0; Index < LegCount; ++Index)
        {
            if (NOT Inputs[Index].Get_Enabled())
            { continue; }

            const auto& Output = Outputs[Index];
            auto& Leg = InGaitComp._Legs[Index];
            auto& LegComp = Leg.Get<FFragment_ProceduralLeg>();
            const auto PreviousPhase = LegComp._Foot.Get_Phase();
            const auto PreviousPosition = LegComp._Foot.Get_Position();
            LegComp._Foot
                .Set_Position(Basis.RotateVector(Output.Get_Position()))
                .Set_Normal(Basis.RotateVector(Output.Get_Normal()))
                .Set_Rotation((Basis * Output.Get_Rotation()).GetNormalized())
                .Set_SwingAlpha(Output.Get_SwingAlpha())
                .Set_Phase(Output.Get_Planted() ? ECk_ProceduralLeg_FootPhase::Planted : ECk_ProceduralLeg_FootPhase::Swinging);

            ck_procedural_gait::DoPublish_FootPhaseChange(Leg, LegComp, PreviousPhase, PreviousPosition, Dt);
        }

        if (InDeltaT > FCk_Time{})
        {
            ++InGaitComp._SolveSequence;

            for (auto Index = 0; Index < LegCount; ++Index)
            {
                const auto& Output = Outputs[Index];
                const auto& State = InGaitComp._Solver.GetLegState(Index);
                InDebugComp._ScratchLegs[Index].Get_Foot().Set_PlantedPosition(Basis.RotateVector(State.Get_Plant().Get_Position()))
                    .Set_SwingTarget(Basis.RotateVector(State.Get_Swing().Get_Target()))
                    .Set_Position(Basis.RotateVector(Output.Get_Position()))
                    .Set_Rotation((Basis * Output.Get_Rotation()).GetNormalized())
                    .Set_Normal(Basis.RotateVector(Output.Get_Normal()))
                    .Set_Planted(Output.Get_Planted())
                    .Set_SwingAlpha(Output.Get_SwingAlpha())
                    .Set_PhaseOffset(InGaitComp._Solver.GetEffectivePhaseOffset(Index));
                InDebugComp._ScratchLegs[Index].Get_Targeting().Set_StepThreshold(InGaitComp._Solver.Get_Settings().Get_Step().Get_Threshold()
                    * Inputs[Index].Get_StepThresholdScale());
            }

            auto& Snapshot = InDebugComp._Snapshot;
            Snapshot.Get_Status().Set_HasAcceptedSample(true);
            Snapshot.Get_Sample().Set_FrameNumber(GFrameCounter)
                .Set_Sequence(InGaitComp._SolveSequence)
                .Set_Time(FCk_Time{World->GetTimeSeconds()});
            Snapshot.Get_Gait().Set_BodyTransform(Body)
                .Set_Velocity(Velocity)
                .Set_CadenceSpeed(CadenceSpeed)
                .Set_CadenceScale(InGaitComp._Solver.Get_LastCadenceScale())
                .Set_Clock(InGaitComp._Solver.GetGaitClock())
                .Set_Airborne(InGaitComp._Solver.IsAirborne())
                .Set_RestTime(InGaitComp._Solver.GetRestTime())
                .Set_SupportNormal(Up)
                .Set_CadenceSpeedRef(InGaitComp._Solver.Get_Settings().Get_Cadence().Get_CadenceSpeedRef())
                .Set_ReachCadenceFloor(InDebugComp._ReachCadenceFloor)
                .Set_ReachSkippedLegs(InDebugComp._ReachSkippedLegs)
                .Set_MissedLandingLifts(InGaitComp._Solver.Get_MissedLandingLifts())
                .Set_RaysLastSolve(InDebugComp._RaysLastSolve);
            Snapshot.Set_Legs(InDebugComp._ScratchLegs);

            if (InHandle.Has<FFragment_SurfaceMotion>() && InHandle.Has<FFragment_SurfaceMotion_Support>())
            {
                const auto& Motion = InHandle.Get<FFragment_SurfaceMotion>();
                const auto& Support = InHandle.Get<FFragment_SurfaceMotion_Support>()._State;
                Snapshot.Get_Motion().Set_Velocity(Support.Get_Velocity())
                    .Set_RequestedDirection(Motion._Direction)
                    .Set_RequestedSpeed(Motion._Speed)
                    .Set_Grounded(Support.Get_Grounded())
                    .Set_TrustedContact(Support.Get_ContactTrusted())
                    .Set_MissingContact(Support.Get_MissingContact())
                    .Set_ContactSource(UCk_Utils_SurfaceMotion_UE::Get_ContactSource(UCk_Utils_SurfaceMotion_UE::CastChecked(InHandle)))
                    .Set_CandidateNormal(Support.Get_CandidateNormal())
                    .Set_CandidateSeen(Support.Get_CandidateSeen());
                Snapshot.Get_Gait().Set_SupportNormal(Support.Get_SupportNormal());
            }
        }

        InGaitComp._Basis = Basis;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ProceduralGait_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProceduralGait_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }
}

// --------------------------------------------------------------------------------------------------------------------
