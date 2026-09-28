#include "CkProceduralAnimation/Gait/CkProceduralGait_Processor.h"

#include "CkProceduralAnimation/BodyPose/CkProceduralBodyPose_Fragment.h"
#include "CkProceduralAnimation/BodyPose/CkProceduralBodyPose_Utils.h"
#include "CkProceduralAnimation/Core/CkProceduralFeetPlane.h"
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
        // The body's velocity in the support plane; zero at rest and at setup.
        FVector Travel = FVector::ZeroVector;
        float Reach = 0.0f;
        float RestDrop = 0.0f;
        const FCk_ProceduralGait_Probe* Probe = nullptr;
        const FCk_ProceduralGait_Step* Step = nullptr;
        const FCk_ProceduralGait_Foothold* Foothold = nullptr;
        float FootContactRadius = 0.0f;
        int32 LegIndex = INDEX_NONE;
        TArrayView<const ck::FProceduralFootReservation> Reservations;
        bool* RejectedReservation = nullptr;
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
    // fails at once (a long body over a beam narrower than its hips) would otherwise run the full fan on every leg in the same frame. A hold serves for
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
    // A face-guided down ray samples near the edge before the search ring, at a small share of that ring's radius.
    // Prefer at least a centimetre inside the face; a selected search face may cap this to a narrower usable strip.
    constexpr auto FaceTopInsetShareOfSearchRadius = 0.15f;
    constexpr auto FaceTopPreferredMinInset = 1.0f;
    // A searched face may leave a narrower usable strip than the preferred inset. Skip only an inset too small to
    // distinguish its point from the collider edge; the ordinary candidate validation still judges the hit.
    constexpr auto FaceTopGeometryTolerance = 1.0e-3;
    // A search ring at this share of the reach reaches the next top across a gap as wide as a top while every ring point
    // stays inside the leg's target reach from an ideal at its rest distance.
    constexpr auto DerivedSearchRadiusShareOfReach = 0.3f;
    // Two spots closer than this many step thresholds agree: a foot moved between them would travel less than a step.
    constexpr auto DerivedKeepRadiusInStepThresholds = 1.5f;
    // The floor in front of an occluder is probed this share of the reach back from the face toward the hip: far enough
    // from the face for the foot to stand, close enough to be the spot the occluded target was aiming at.
    constexpr auto FrontFloorShareOfReach = 0.1f;
    // The inward casts aim from the ideal target under the hip at these depths in rest drops: the near one meets a pillar
    // side or a cylinder curving away at the leg's own height, the far one the floor under a hip raised on a top or a post.
    constexpr auto InwardCloseDepthInRestDrops = 1.5f;
    constexpr auto InwardFarDepthInRestDrops = 4.0f;
    // The outward cast starts this many rest drops above the hip so it looks down over a top or a face beyond the ideal.
    constexpr auto OutwardHeightInRestDrops = 2.0f;
    // Eight ring points, 45 degrees apart, put at least two on a top across any gap narrower than the ring's radius while
    // keeping a search under the fan the budget allows.
    constexpr auto RingPoints = 8;
    // While too few feet support the body the last fitted plane is held this many step durations: long enough to bridge the
    // swing of a phase group between two stances, short enough that a body that leaves its feet behind soon rides its rays.
    constexpr auto FeetPlaneHoldStepDurations = 1.5;

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

    auto
        Get_LegFootholdVerdict(
            ck::EProceduralFootholdVerdict InVerdict)
        -> ECk_ProceduralLeg_FootholdVerdict
    {
        switch (InVerdict)
        {
            case ck::EProceduralFootholdVerdict::Usable: return ECk_ProceduralLeg_FootholdVerdict::Usable;
            case ck::EProceduralFootholdVerdict::Miss: return ECk_ProceduralLeg_FootholdVerdict::Miss;
            case ck::EProceduralFootholdVerdict::Unreachable: return ECk_ProceduralLeg_FootholdVerdict::Unreachable;
            case ck::EProceduralFootholdVerdict::TooSteep: return ECk_ProceduralLeg_FootholdVerdict::TooSteep;
            case ck::EProceduralFootholdVerdict::Occluded: return ECk_ProceduralLeg_FootholdVerdict::Occluded;
            case ck::EProceduralFootholdVerdict::Inboard: return ECk_ProceduralLeg_FootholdVerdict::Inboard;
            case ck::EProceduralFootholdVerdict::UnderBody: return ECk_ProceduralLeg_FootholdVerdict::UnderBody;
            case ck::EProceduralFootholdVerdict::Reserved: return ECk_ProceduralLeg_FootholdVerdict::Reserved;
        }
        return ECk_ProceduralLeg_FootholdVerdict::Miss;
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
    // its hip, UnderBody below the body, Occluded when the hip cannot see it, Reserved by another foot, else Usable.
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
        if (InOutCandidate.Occluder.IsSet())
        {
            InOutCandidate.Verdict = ck::EProceduralFootholdVerdict::Occluded;
            return;
        }

        if (NOT ck::Get_IsProceduralFootContactAvailable(InOutCandidate.Position, InQuery.FootContactRadius,
                InQuery.LegIndex, InQuery.Reservations))
        {
            InOutCandidate.Verdict = ck::EProceduralFootholdVerdict::Reserved;
            if (InQuery.RejectedReservation != nullptr)
            { *InQuery.RejectedReservation = true; }
            return;
        }

        InOutCandidate.Verdict = ck::EProceduralFootholdVerdict::Usable;
    }

    // A down ray under InPoint. When its first, leaned attempt meets a face, one straight ray is cast under the same point,
    // and level ground it meets within the force-step reach of the hip is the candidate instead: the lean reaches out past the
    // point and can meet the face of a pillar or a riser beside the floor or the tread under it. Over a gap the straight ray
    // finds nothing in reach and the face stands. With ClampTowardHip, ground farther than the target reach from the hip is
    // clamped toward the hip like the query point and probed once more there, straight down: the solver's own clamp keeps a
    // target's support-frame height, which across a convex bend lies inside the ground, and a second probe leaning out like
    // the first could clip the same edge again. Ground the second probe misses stays Unreachable, as out of reach.
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
        const auto ProbeAt = [&](const FVector& InProbePoint, const FCk_ProceduralGait_Probe& InProbe) -> TOptional<FCk_Jolt_HitResult>
        {
            const auto Radial = FVector::VectorPlaneProject(InProbePoint - InQuery.BodyLocation, Up).GetSafeNormal();
            return Get_GroundHit(InQuery.World, InProbePoint, Up, Radial, InProbe, InAttempts, InOutRayCount, InDebugProbe);
        };

        auto Candidate = FFootholdCandidate{InPoint, Up, InSource, ck::EProceduralFootholdVerdict::Miss, ck::EProceduralFootholdCast::Down};
        auto Hit = ProbeAt(InPoint, *InQuery.Probe);
        if (NOT Hit.IsSet())
        { return Candidate; }

        if (InQuery.Probe->Get_OutwardLean() > 0.0f && ck::FProceduralGaitSolver::Get_IsFaceNormal(Hit->Get_Normal(), Up))
        {
            const auto StraightHit = UCk_Utils_JoltQuery_UE::Get_RayCast(InQuery.World, InPoint + Up * InQuery.Probe->Get_Up(),
                InPoint - Up * InQuery.Probe->Get_Down(), InQuery.Probe->Get_QueryFilter());
            ++InOutRayCount;
            const auto LevelInReach = Get_TrustedHit(StraightHit, -Up)
                && NOT ck::FProceduralGaitSolver::Get_IsFaceNormal(StraightHit.Get_Normal(), Up)
                && FVector::Dist(StraightHit.Get_Position(), InQuery.Hip) <= InQuery.Step->Get_ForceStepReachFraction() * InQuery.Reach;
            if (LevelInReach)
            { Hit = StraightHit; }
        }

        Candidate.Position = Hit->Get_Position();
        Candidate.Normal = Hit->Get_Normal().GetSafeNormal();

        const auto TargetLimit = InQuery.Step->Get_TargetReachFraction() * InQuery.Reach;
        if (InReprobe == EFootholdReprobe::ClampTowardHip && FVector::Dist(Candidate.Position, InQuery.Hip) > TargetLimit)
        {
            const auto InverseBasis = InQuery.Basis.Inverse();
            auto StraightDown = *InQuery.Probe;
            StraightDown.Set_OutwardLean(0.0f);
            const auto Reprobed = ProbeAt(InQuery.Basis.RotateVector(ck::FProceduralGaitSolver::ClampToReach(
                InverseBasis.RotateVector(InQuery.Hip), InverseBasis.RotateVector(Candidate.Position), TargetLimit)), StraightDown);
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

    // A straight ray just inside a face may find the top its leaned or diagonal ray did not sample. The caller supplies
    // the inset: the ideal keeps its existing query, while a selected search face can cap the inset to its usable strip.
    auto
        DoProbe_FaceTop(
            const FFootholdQuery& InQuery,
            const FFootholdCandidate& InFace,
            double InInset,
            int32& InOutRayCount)
        -> TOptional<FFootholdCandidate>
    {
        const auto FaceNormalInPlane = FVector::VectorPlaneProject(InFace.Normal, InQuery.Basis.GetAxisZ()).GetSafeNormal();
        if (FaceNormalInPlane.IsNearlyZero())
        { return {}; }

        auto StraightProbe = *InQuery.Probe;
        StraightProbe.Set_OutwardLean(0.0f);
        auto TopQuery = InQuery;
        TopQuery.Probe = &StraightProbe;
        const auto Source = InFace.Source == ck::EProceduralFootholdSource::Ideal
            || InFace.Source == ck::EProceduralFootholdSource::Held
            ? ck::EProceduralFootholdSource::Inward : InFace.Source;
        return DoProbe_Down(TopQuery, InFace.Position - FaceNormalInPlane * InInset, Source,
            EGroundProbeAttempts::RetryFromInsideSolid, EFootholdReprobe::ClampTowardHip, EFootholdStage::Search, InOutRayCount,
            nullptr);
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
            TArray<FFootholdCandidate, TInlineAllocator<24>>& OutCandidates,
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
    // swinging leg keeps a usable hold. Otherwise a usable hold within the keep radius of the ideal target stays the
    // target unless the ideal exposes an admitted new face or an obstruction, so agreeing spots do not cause hopping. A
    // usable non-face ideal beyond the keep radius clears the hold. An unusable ideal, or one on a face (its normal farther than
    // ProceduralGaitFaceAngleDegrees from the support up), runs the search when the budget has one left and the leg is not
    // backing off a search that found nothing. A usable hold within the leg's force-step reach of the ideal serves while it
    // lies ahead of the hip along the travel, or the body is at rest: an ideal hanging beside a beam or over a gap would
    // otherwise drop the hold its own search found, and a pick beside a moving ideal would be searched again every frame.
    // A hold beyond the keep radius and behind the hip along the travel, one the leg has walked on past, competes in the
    // search as one more candidate, at its own distance from the ideal: a spot beside the ideal beats it, and a hold near
    // the ideal keeps winning. Farther than the force-step reach, the body has walked away from the hold (up a log, over a
    // crest) and a leg kept on it re-plants behind its rest point, so the hold is cleared. A face ideal competes too, with
    // the costs measured from it: a top within half the reach beats a vertical face. The least-cost candidate is the target
    // and, when the search found it, the new hold; when the search finds nothing better, or the leg is past its budget or
    // backing off, the hold stays the target, so the planted foot keeps standing on it, else the face ideal. Every ray
    // increments InOutRayCount; the debug leg, when given, receives every candidate.
    auto
        Get_Foothold(
            const FFootholdQuery& InQuery,
            ck::FProceduralFootholdState& InOutState,
            FFootholdSearchBudget& InOutBudget,
            int32& InOutRayCount,
            FCk_ProceduralAnimation_DebugLeg* InOutDebugLeg)
        -> FFoothold
    {
        auto Candidates = TArray<FFootholdCandidate, TInlineAllocator<24>>{};
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

        // A swinging leg keeps the hold it swings to while the hold stays usable: switching to the ideal in flight would move
        // a foot already on its way, and at the freeze the swing would take the new target for the ground it validated.
        if (HoldIsUsable && NOT InQuery.Planted)
        { return DoFinish(Held, HeldIndex, Ideal.Verdict, InOutDebugLeg); }

        const auto KeepRadius = FootholdTuning.Get_KeepRadius() > 0.0f
            ? FootholdTuning.Get_KeepRadius()
            : DerivedKeepRadiusInStepThresholds * InQuery.Step->Get_Threshold();
        const auto HoldToIdeal = HoldIsUsable ? FVector::Dist(InOutState.Get_Position(), InQuery.Ideal) : 0.0;
        const auto HoldAgrees = HoldIsUsable && HoldToIdeal <= KeepRadius;
        // A valid plant beside a blocked ideal is support, but not evidence that the next step may stay there. Search the
        // obstruction before keeping an ahead-of-hip hold; retain that support as fallback when no alternative is found.
        const auto IdealOccluded = Ideal.Verdict == ck::EProceduralFootholdVerdict::Occluded && Ideal.Occluder.IsSet();

        const auto IdealIsUsable = Ideal.Verdict == ck::EProceduralFootholdVerdict::Usable;
        const auto Up = InQuery.Basis.GetAxisZ();
        const auto IdealOnAFace = IdealIsUsable && ck::FProceduralGaitSolver::Get_IsFaceNormal(Ideal.Normal, Up);
        const auto AdmittedIdealFace = IdealOnAFace && ck::Get_IsFootholdLevelEnough(
            InQuery.Basis.Inverse().RotateVector(Ideal.Normal), FootholdTuning.Get_MaxAngle());
        const auto FaceChangesSurface = AdmittedIdealFace
            && (NOT HoldIsUsable || ck::FProceduralGaitSolver::Get_IsFaceNormal(Ideal.Normal, Held.Normal));
        const auto HoldNeedsSearch = IdealOccluded || FaceChangesSurface;
        if (IdealIsUsable && NOT IdealOnAFace)
        {
            if (HoldAgrees)
            { return DoFinish(Held, HeldIndex, Ideal.Verdict, InOutDebugLeg); }

            InOutState = ck::FProceduralFootholdState{};
            return DoFinish(Ideal, IdealIndex, Ideal.Verdict, InOutDebugLeg);
        }

        if (HoldAgrees && NOT HoldNeedsSearch)
        { return DoFinish(Held, HeldIndex, Ideal.Verdict, InOutDebugLeg); }

        const auto TrailLimit = InQuery.Step->Get_ForceStepReachFraction() * InQuery.Reach;
        const auto HoldWithinReach = HoldIsUsable && HoldToIdeal <= TrailLimit;
        const auto HoldCompetes = HoldWithinReach && NOT HoldNeedsSearch
            && FVector::DotProduct(InOutState.Get_Position() - InQuery.Hip, InQuery.Travel.GetSafeNormal()) < 0.0;
        if (HoldWithinReach && NOT HoldCompetes && NOT HoldNeedsSearch)
        { return DoFinish(Held, HeldIndex, Ideal.Verdict, InOutDebugLeg); }

        if (HoldIsUsable && NOT HoldWithinReach)
        { InOutState = ck::FProceduralFootholdState{}; }

        const auto NothingUsable = FFoothold{InQuery.Ideal, Up, ck::EProceduralFootholdSource::None, Ideal.Verdict};
        const auto& SearchedSolve = InOutState.Get_SearchedSolve();
        const auto BackingOff = SearchedSolve.IsSet() && InOutBudget.Solve - SearchedSolve.GetValue() <= SearchBackoffSolves;
        if (BackingOff || InOutBudget.RemainingLegs <= 0)
        {
            if (HoldCompetes || (HoldNeedsSearch && HoldWithinReach))
            { return DoFinish(Held, HeldIndex, Ideal.Verdict, InOutDebugLeg); }
            return IdealOnAFace ? DoFinish(Ideal, IdealIndex, Ideal.Verdict, InOutDebugLeg) : NothingUsable;
        }

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
            const auto Inward = DoProbe_Face(InQuery, DoCast_Segment(InQuery, InQuery.Ideal, End, InOutRayCount), InQuery.Ideal,
                ck::EProceduralFootholdSource::Inward, EFootholdStage::Search, InOutRayCount);
            DoRecord_Candidate(Inward, Candidates, InOutDebugLeg);

            if (Inward.Verdict == ck::EProceduralFootholdVerdict::UnderBody)
            {
                // Converging on the hip can erase lateral foot spacing on convex support. Retain that spacing in a
                // second cast; the ordinary admission gates still decide whether the resulting surface is usable.
                const auto LateralAxis = InQuery.Basis.GetAxisY();
                const auto SpreadFocal = Focal + LateralAxis * FVector::DotProduct(InQuery.Ideal - InQuery.Hip, LateralAxis);
                const auto SpreadEnd = SpreadFocal + (SpreadFocal - InQuery.Ideal).GetSafeNormal() * InQuery.Probe->Get_Down();
                DoRecord_Candidate(DoProbe_Face(InQuery, DoCast_Segment(InQuery, InQuery.Ideal, SpreadEnd, InOutRayCount),
                    InQuery.Ideal, ck::EProceduralFootholdSource::Inward, EFootholdStage::Search, InOutRayCount),
                    Candidates, InOutDebugLeg);
            }
        }

        {
            const auto Origin = InQuery.Hip + Up * (OutwardHeightInRestDrops * InQuery.RestDrop);
            const auto End = InQuery.Ideal + (InQuery.Ideal - Origin).GetSafeNormal() * InQuery.Probe->Get_Down();
            DoRecord_Candidate(DoProbe_Face(InQuery, DoCast_Segment(InQuery, Origin, End, InOutRayCount), InQuery.Ideal,
                ck::EProceduralFootholdSource::Outward, EFootholdStage::Search, InOutRayCount), Candidates, InOutDebugLeg);
        }

        // A face at the ideal can have a shallow top just behind its edge. The search ring may jump over that top; a
        // straight down ray a short way inside the face samples it without leaning back into the same face. Scale the
        // inset with the search radius, and run the ordinary search validation on whatever surface the ray finds.
        auto FaceTopProbed = false;
        if (IdealOnAFace)
        {
            const auto Inset = FMath::Max(FaceTopPreferredMinInset, SearchRadius * FaceTopInsetShareOfSearchRadius);
            const auto Top = DoProbe_FaceTop(InQuery, Ideal, Inset, InOutRayCount);
            if (Top.IsSet())
            {
                DoRecord_Candidate(Top.GetValue(), Candidates, InOutDebugLeg);
                FaceTopProbed = true;
            }
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

        // A hold the force-step bound dropped is not among the picks, and picking it again would hold it forever. A hold
        // behind the hip and beyond the keep radius competes first, so it wins a tie, then a face ideal, and the costs are
        // measured from the ideal, so a face ideal stands at distance 0 and the hold at its own distance from it.
        const auto FirstPickIndex = HoldCompetes ? HeldIndex : (IdealOnAFace ? IdealIndex : IdealIndex + 1);
        const auto InverseBasis = InQuery.Basis.Inverse();
        auto SlopeUp = Up;
        const auto TransitionNormal = IdealOccluded ? TOptional<FVector>{Ideal.Occluder->Get_Normal()} : TOptional<FVector>{};
        if (TransitionNormal.IsSet() && ck::FProceduralGaitSolver::Get_IsFaceNormal(TransitionNormal.GetValue(), Up)
            && ck::Get_IsFootholdLevelEnough(InverseBasis.RotateVector(TransitionNormal.GetValue()), FootholdTuning.Get_MaxAngle()))
        {
            // A blocked ideal establishes an approach obstruction: rank its face and current support evenly. A usable
            // face ideal retains the ordinary slope reference so a nearby top still beats the face. Admission continues
            // to use the current support frame; this neither admits an over-steep hit nor rotates the body or its probes.
            const auto Midpoint = (Up + TransitionNormal->GetSafeNormal()).GetSafeNormal();
            if (NOT Midpoint.IsNearlyZero())
            { SlopeUp = Midpoint; }
        }
        const auto Settings = ck::FProceduralFootholdSettings{}
            .Set_SlopeWeight(FootholdTuning.Get_SlopeWeight())
            .Set_ContinuityWeight(FootholdTuning.Get_ContinuityWeight())
            .Set_MaxAngleDegrees(FootholdTuning.Get_MaxAngle())
            .Set_SlopeUp(InverseBasis.RotateVector(SlopeUp));
        const auto CostOrigin = IdealOnAFace || IdealOccluded ? Ideal.Position : InQuery.Ideal;
        const auto Get_SearchPick = [&]() -> int32
        {
            const auto PickCandidates = TArrayView<const FFootholdCandidate>{Candidates}.RightChop(FirstPickIndex);
            const auto SupportCandidates = ck::algo::Transform<TArray<ck::FProceduralFootholdCandidate, TInlineAllocator<24>>>(PickCandidates,
            [&](const FFootholdCandidate& InCandidate)
            {
                return ck::FProceduralFootholdCandidate{InverseBasis.RotateVector(InCandidate.Position),
                    InverseBasis.RotateVector(InCandidate.Normal), InCandidate.Source, InCandidate.Verdict};
            });
            return ck::SelectProceduralFoothold(SupportCandidates, InverseBasis.RotateVector(CostOrigin),
                InverseBasis.RotateVector(InQuery.Plant), InQuery.Planted, InQuery.Reach, Settings);
        };
        auto SearchPick = Get_SearchPick();

        // The ideal probe owns the first opportunity to refine a face, in its original candidate order. If it did not
        // run, refine only the usable face the completed search actually chose. Append the top so a strict-cost tie keeps
        // the old winner, and copy the face before the candidate array can grow.
        if (NOT FaceTopProbed && SearchPick != INDEX_NONE)
        {
            const auto Face = Candidates[FirstPickIndex + SearchPick];
            if (Face.Verdict == ck::EProceduralFootholdVerdict::Usable
                && ck::FProceduralGaitSolver::Get_IsFaceNormal(Face.Normal, Up))
            {
                const auto FaceNormalInPlane = FVector::VectorPlaneProject(Face.Normal, Up).GetSafeNormal();
                auto Inset = static_cast<double>(FMath::Max(FaceTopPreferredMinInset, SearchRadius * FaceTopInsetShareOfSearchRadius));
                const auto Lateral = InverseBasis.RotateVector(Face.Position - InQuery.BodyLocation).Y;
                const auto TowardBody = FMath::Sign(Lateral) * InverseBasis.RotateVector(FaceNormalInPlane).Y;
                const auto LateralGap = FMath::Abs(Lateral) - (InQuery.MaxHipLateral - UnderBodyMargin);
                if (InQuery.MaxHipLateral > UnderBodyMargin && LateralGap > 0.0 && TowardBody > KINDA_SMALL_NUMBER)
                {
                    // Keep half the lateral strip outside the existing UnderBody boundary. A nominal inset would
                    // jump over a narrow valid top into the body band; this changes the sample, never its validation.
                    Inset = FMath::Min(Inset, 0.5 * LateralGap / TowardBody);
                }
                if (Inset > FaceTopGeometryTolerance)
                {
                    const auto Top = DoProbe_FaceTop(InQuery, Face, Inset, InOutRayCount);
                    if (Top.IsSet())
                    {
                        DoRecord_Candidate(Top.GetValue(), Candidates, InOutDebugLeg);
                        SearchPick = Get_SearchPick();
                    }
                }
            }
        }

        if (SearchPick == INDEX_NONE)
        {
            InOutState.Set_SearchedSolve(InOutBudget.Solve);
            return HoldCompetes || (HoldNeedsSearch && HoldWithinReach)
                ? DoFinish(Held, HeldIndex, Ideal.Verdict, InOutDebugLeg) : NothingUsable;
        }

        const auto Chosen = FirstPickIndex + SearchPick;
        // The hold stays the target; a search that found nothing better backs off like one that found nothing.
        if (Chosen == HeldIndex)
        {
            InOutState.Set_SearchedSolve(InOutBudget.Solve);
            return DoFinish(Held, HeldIndex, Ideal.Verdict, InOutDebugLeg);
        }

        // The face ideal stays the target; a search that found nothing better backs off like one that found nothing.
        if (Chosen == IdealIndex)
        {
            InOutState = ck::FProceduralFootholdState{}.Set_SearchedSolve(InOutBudget.Solve);
            return DoFinish(Ideal, IdealIndex, Ideal.Verdict, InOutDebugLeg);
        }

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

    struct FLandingGround
    {
        ck::EProceduralGaitLandingGround Report = ck::EProceduralGaitLandingGround::Unknown;
        FVector Position = FVector::ZeroVector;
    };

    // The ground under the point a swing will land on, as of the last solve. The stroke overshoot and the freeze push can
    // carry a target probed on a lower tread past the next riser, where the swing lifts onto the ground found, or past a
    // top's edge into the air, where nothing is found and the swing lands on the target the gait validated instead. A
    // swing whose target was on a face when it accepted it has no tread to lift onto, so it is not probed. The debug leg
    // records the point and the ray.
    auto
        Get_LandingGround(
            UWorld* InWorld,
            const FQuat& InBasis,
            const FVector& InHip,
            float InReach,
            const ck::FProceduralGaitLegSwing& InSwing,
            const FCk_ProceduralGait_Probe& InProbe,
            const FCk_ProceduralGait_Step& InStep,
            int32& InOutRayCount,
            FCk_ProceduralAnimation_DebugLeg& InOutDebugLeg)
        -> FLandingGround
    {
        if (NOT InSwing.Get_Active() || InSwing.Get_CatchStep() || InSwing.Get_TargetOnAFace())
        { return {}; }

        const auto Up = InBasis.GetAxisZ();

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
        { return FLandingGround{ck::EProceduralGaitLandingGround::None}; }

        return FLandingGround{ck::EProceduralGaitLandingGround::Found, Hit.Get_Position()};
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

    // Rebuilt from committed solver state; no second reservation lifetime or stale release path.
    auto
        Get_FootReservations(
            TArrayView<const FCk_Handle_ProceduralLeg> InLegs,
            const ck::FProceduralGaitSolver& InSolver,
            const FQuat& InBasis)
        -> TArray<ck::FProceduralFootReservation, TInlineAllocator<8>>
    {
        auto Reservations = TArray<ck::FProceduralFootReservation, TInlineAllocator<8>>{};
        for (auto Index = 0; Index < InLegs.Num(); ++Index)
        {
            const auto& Leg = InLegs[Index];
            if (NOT Get_IsLegEnabled(Leg))
            { continue; }
            const auto Radius = Leg.Get<ck::FFragment_ProceduralLeg_Params>().Get_FootContactRadius();
            if (Radius <= 0.0f)
            { continue; }
            const auto& State = InSolver.GetLegState(Index);
            const auto& Swing = State.Get_Swing();
            if (Swing.Get_Active())
            {
                if (Swing.Get_TargetTrusted())
                {
                    const auto Point = Swing.Get_CommittedLandingPoint();
                    Reservations.Emplace(InBasis.RotateVector(Point), Radius, Index);
                }
            }
            else if (State.Get_Plant().Get_Trusted())
            { Reservations.Emplace(InBasis.RotateVector(State.Get_Plant().Get_Position()), Radius, Index); }
        }
        return Reservations;
    }

    auto
        Get_SearchBudgetLegs(
            int32 InEnabledLegs)
        -> int32
    {
        return FMath::Max(MinSearchLegs, InEnabledLegs / EnabledLegsPerSearch);
    }

    struct FFeetPlaneFit
    {
        ck::EProceduralFeetPlaneResult Result = ck::EProceduralFeetPlaneResult::Underdetermined;
        ck::FProceduralSurfaceFeetSupport Support;
    };

    // How much a point of ground supports the body from below: nothing unless trusted, and in proportion to how level it
    // lies, so a foot gripping a face (it holds the body sideways) weighs nothing and one on a 45-degree slope 0.71.
    auto
        Get_GroundWeight(
            const FVector& InNormal,
            bool InTrusted)
        -> float
    {
        if (NOT InTrusted)
        { return 0.0f; }

        return static_cast<float>(FMath::Max(0.0, InNormal.GetSafeNormal().Z));
    }

    // One point of ground under the feet, in the solver's support frame: Weight is how much it holds the body up (its trust
    // and how level it lies) times the swing's crossfade. The footprint spans every trusted point whatever its weight, so the
    // plant a swing left and the ground it will land on both bound it whatever the swing alpha, and a leg going to a face
    // foothold ahead keeps the body inside it though the face holds nothing up.
    struct FFeetPlaneGround
    {
        FVector Position = FVector::ZeroVector;
        float Weight = 0.0f;
        bool Trusted = false;
    };

    // The ground the enabled legs stand on or are about to, as the solve left it: a planted leg's plant; a swinging leg's
    // plant it left, weighing 1 - alpha, and the accepted landing point, weighing alpha, including only a lift the solver
    // admitted, with the trust and normal the swing recorded for its target. A foot in flight is never ground.
    // Nothing supports an airborne body.
    auto
        Get_FeetPlaneGround(
            const ck::FProceduralGaitSolver& InSolver,
            TArrayView<const ck::FProceduralGaitLegInput> InInputs)
        -> TArray<FFeetPlaneGround, TInlineAllocator<32>>
    {
        auto Ground = TArray<FFeetPlaneGround, TInlineAllocator<32>>{};
        if (InSolver.IsAirborne())
        { return Ground; }

        for (auto Index = 0; Index < InInputs.Num(); ++Index)
        {
            const auto& Input = InInputs[Index];
            if (NOT Input.Get_Enabled())
            { continue; }

            const auto& State = InSolver.GetLegState(Index);
            const auto& Plant = State.Get_Plant();
            const auto& Swing = State.Get_Swing();
            const auto PlantSupport = Get_GroundWeight(Plant.Get_Normal(), Plant.Get_Trusted());
            if (NOT Swing.Get_Active())
            {
                Ground.Add(FFeetPlaneGround{Plant.Get_Position(), PlantSupport, Plant.Get_Trusted()});
                continue;
            }

            const auto Alpha = FMath::Clamp(Swing.Get_Phase(), 0.0f, 1.0f);
            const auto Landing = Swing.Get_CommittedLandingPoint();
            const auto LandingSupport = Get_GroundWeight(Swing.Get_TargetNormal(), Swing.Get_TargetTrusted());
            Ground.Add(FFeetPlaneGround{Swing.Get_StartPosition(), (1.0f - Alpha) * PlantSupport, Plant.Get_Trusted()});
            Ground.Add(FFeetPlaneGround{Landing, Alpha * LandingSupport, Swing.Get_TargetTrusted()});
        }
        return Ground;
    }

    // The plane through the ground points (support frame), fitted about the body and published in world space with that frame
    // and the footprint of the trusted points.
    auto
        Get_FeetPlane(
            TArrayView<const FFeetPlaneGround> InGround,
            const FTransform& InBody)
        -> FFeetPlaneFit
    {
        const auto Basis = InBody.GetRotation();
        const auto BodySupport = Basis.UnrotateVector(InBody.GetLocation());
        auto Feet = TArray<ck::FProceduralFeetPlaneFoot, TInlineAllocator<32>>{};
        auto FootprintMin = FVector2D{TNumericLimits<double>::Max()};
        auto FootprintMax = FVector2D{TNumericLimits<double>::Lowest()};
        for (const auto& Point : InGround)
        {
            const auto Local = Point.Position - BodySupport;
            Feet.Emplace(Local, Point.Weight);
            if (NOT Point.Trusted)
            { continue; }

            FootprintMin = FVector2D{FMath::Min(FootprintMin.X, Local.X), FMath::Min(FootprintMin.Y, Local.Y)};
            FootprintMax = FVector2D{FMath::Max(FootprintMax.X, Local.X), FMath::Max(FootprintMax.Y, Local.Y)};
        }

        auto Plane = ck::FProceduralFeetPlane{};
        const auto Result = ck::FitProceduralFeetPlane(Feet, ck::ProceduralFeetPlaneMaxAngleDegrees, Plane);
        if (Result != ck::EProceduralFeetPlaneResult::Fitted)
        { return FFeetPlaneFit{Result}; }

        return FFeetPlaneFit{Result, ck::FProceduralSurfaceFeetSupport{}
            .Set_Point(InBody.GetLocation() + Basis.RotateVector(Plane.Get_Point()))
            .Set_Normal(Basis.RotateVector(Plane.Get_Normal()))
            .Set_Basis(Basis)
            .Set_Origin(InBody.GetLocation())
            .Set_FootprintMin(FootprintMin)
            .Set_FootprintMax(FootprintMax)};
    }

    // The ground under a touchdown (ck::ResolveProceduralTouchdown) in world space, confirmed with the ray a face hold is
    // re-validated with: a top along its up, a face along its own normal.
    auto
        Get_Touchdown(
            UWorld* InWorld,
            const FVector& InPlant,
            const FVector& InValidatedTarget,
            const FVector& InTargetNormal,
            const FVector& InUp,
            const FCk_ProceduralGait_Probe& InProbe,
            const FVector& InSimulationHip,
            const TOptional<FVector>& InPresentationHip,
            float InReach,
            float InFootContactRadius,
            int32 InLegIndex,
            TArrayView<const ck::FProceduralFootReservation> InReservations)
        -> ck::FProceduralTouchdown
    {
        const auto RayCast = [&](const FVector& InStart, const FVector& InEnd) -> ck::FProceduralSurfaceHit
        {
            const auto Hit = UCk_Utils_JoltQuery_UE::Get_RayCast(InWorld, InStart, InEnd, InProbe.Get_QueryFilter());
            return ck::FProceduralSurfaceHit{}
                .Set_Hit(Hit.Get_HasHit())
                .Set_Position(Hit.Get_Position())
                .Set_Normal(Hit.Get_Normal())
                .Set_Fraction(Hit.Get_Fraction());
        };
        const auto Available = [&](const FVector& InPosition) -> bool
        {
            return ck::Get_IsProceduralFootContactAvailable(InPosition, InFootContactRadius, InLegIndex, InReservations);
        };
        return ck::ResolveProceduralTouchdown(InPlant, InValidatedTarget, InTargetNormal, InUp, FaceHoldProbeHalfSpan,
            RayCast, InSimulationHip, InPresentationHip, InReach, Available);
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
        auto InitialTrust = TArray<bool, TInlineAllocator<8>>{};
        auto InitialReservations = TArray<FProceduralFootReservation, TInlineAllocator<8>>{};
        InitialFeet.Reserve(InGaitComp._Legs.Num());
        InitialTrust.Reserve(InGaitComp._Legs.Num());
        for (auto Index = 0; Index < InGaitComp._Legs.Num(); ++Index)
        {
            auto& Leg = InGaitComp._Legs[Index];
            if (ck::Is_NOT_Valid(Leg))
            {
                constexpr auto Untrusted = false;
                InitialFeet.Add(InverseBasis.RotateVector(Body.GetLocation()));
                InitialTrust.Add(Untrusted);
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
                .Foothold = &InTunables.Get_Foothold(),
                .FootContactRadius = Params.Get_FootContactRadius(),
                .LegIndex = Index,
                .Reservations = InitialReservations};
            const auto Foothold = ck_procedural_gait::Get_Foothold(Query, InGaitComp._Footholds[Index], SearchBudget, RayCount,
                &InDebugComp._ScratchLegs[Index]);
            const auto Trusted = Foothold.Source != EProceduralFootholdSource::None;
            const auto Position = Trusted ? Foothold.Position : Neutral;
            const auto Normal = Trusted ? Foothold.Normal : Up;

            auto& LegComp = Leg.Get<FFragment_ProceduralLeg>();
            // Update freezes a leg disabled before Add from this published foot, so it must already hold the probed pose.
            LegComp._Foot.Set_Position(Position)
                .Set_Normal(Normal)
                .Set_Rotation(Basis)
                .Set_SwingAlpha(0.0f)
                .Set_Phase(ECk_ProceduralLeg_FootPhase::Planted)
                .Set_Contact(Trusted ? ECk_ProceduralLeg_FootContact::Trusted : ECk_ProceduralLeg_FootContact::Guessed)
                .Set_Foothold(ck_procedural_gait::Get_LegFoothold(Foothold.Source));
            LegComp._IdealVerdict = ck_procedural_gait::Get_LegFootholdVerdict(Foothold.IdealVerdict);

            InitialFeet.Add(InverseBasis.RotateVector(Position));
            InitialTrust.Add(Trusted);
            if (Trusted && Params.Get_FootContactRadius() > 0.0f && ck_procedural_gait::Get_IsLegEnabled(Leg))
            { InitialReservations.Emplace(Position, Params.Get_FootContactRadius(), Index); }
        }
        InDebugComp._RaysLastSolve = RayCount;

        const auto SolverReset = InGaitComp._Solver.Reset(InitialFeet, InitialTrust);
        CK_ENSURE_IF_NOT(SolverReset,
            TEXT("Procedural gait [{}] solver rejected the initial foot positions; feature is failed."), InHandle)
        {
            InHandle.Add<FFragment_ProceduralGait_Failure>(ECk_ProceduralGait_Failure::SolverReset);
            InHandle.Remove<MarkedDirtyBy>();
            return;
        }

        InGaitComp._Basis = Basis;
        InGaitComp._ReachStance = FProceduralGaitReachStance{};
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
            FFragment_ProceduralGait_FeetPlane& InFeetPlaneComp,
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

        const auto HasPose = InHandle.Has<FFragment_ProceduralBodyPose>()
            && UCk_Utils_ProceduralBodyPose_UE::Get_Status(UCk_Utils_ProceduralBodyPose_UE::Cast(InHandle))
                == ECk_ProceduralAnimation_Status::Ready;
        const auto PresentationBody = HasPose
            ? UCk_Utils_ProceduralBodyPose_UE::Get_Offset(UCk_Utils_ProceduralBodyPose_UE::CastChecked(InHandle)) * Body
            : FTransform::Identity;

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

        const FFragment_SurfaceMotion_Support* PaceSupport = nullptr;
        if (Dt > 0.0 && InHandle.Has<FFragment_SurfaceMotion>() && InHandle.Has<FFragment_SurfaceMotion_Support>()
            && UCk_Utils_SurfaceMotion_UE::Get_Status(UCk_Utils_SurfaceMotion_UE::Cast(InHandle)) == ECk_ProceduralAnimation_Status::Ready)
        {
            const auto& Support = InHandle.Get<FFragment_SurfaceMotion_Support>();
            if (Support._EvaluatedFrame == GFrameCounter && Support._EvaluatedBody.Equals(Body, 1.0e-3)
                && Support._State.Get_Grounded() && Support._AttemptedStanceSpeed > 0.0f
                && (Support._ReachPaceState == ECk_SurfaceMotion_ReachPaceState::Pacing
                    || Support._ReachPaceState == ECk_SurfaceMotion_ReachPaceState::Blocked))
            { PaceSupport = &Support; }
        }

        const auto LegCount = InGaitComp._Legs.Num();
        const auto Reservations = ck_procedural_gait::Get_FootReservations(InGaitComp._Legs, InGaitComp._Solver, Basis);

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
                .Set_LandingGround(EProceduralGaitLandingGround::Unknown)
                .Set_ChosenFoothold(INDEX_NONE)
                .Set_FootholdSource(EProceduralFootholdSource::None)
                .Set_PlantOccluded(false);
            DebugLeg.Get_Footholds().Reset();
            DebugLeg.Get_Probe() = FCk_ProceduralAnimation_DebugProbe{};
            DebugLeg.Get_Foot().Set_ContactTrusted(false);

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
                .Set_StepThresholdScale(Placement.Get_StepThresholdScale())
                .Set_FootContactRadius(Params.Get_FootContactRadius());
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
            if (HasPose && Planted && LegComp._Foot.Get_Contact() == ECk_ProceduralLeg_FootContact::Trusted
                && UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(Leg) == InHandle.ConvertToHandle())
            {
                Input.Set_PosedHip(TOptional<FVector>{InverseBasis.RotateVector(
                    PresentationBody.TransformPosition(Placement.Get_HipLocal()))});
            }
            if (PaceSupport && Planted && LegComp._Foot.Get_Contact() == ECk_ProceduralLeg_FootContact::Trusted
                && UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(Leg) == InHandle.ConvertToHandle())
            {
                for (const auto& Feedback : PaceSupport->_ReachPaceFeedback)
                {
                    if (Feedback.Get_Leg() == Leg && Feedback.Get_FootWorld().Equals(LegComp._Foot.Get_Position(), 1.0e-3))
                    {
                        auto Trial = FProceduralGaitReachPaceTrial{}.Set_Hip(InverseBasis.RotateVector(Feedback.Get_TrialHipWorld()));
                        if (HasPose && Feedback.Get_TrialPosedHipWorld().IsSet())
                        { Trial.Set_PosedHip(TOptional<FVector>{InverseBasis.RotateVector(Feedback.Get_TrialPosedHipWorld().GetValue())}); }
                        Input.Set_ReachPaceTrial(TOptional<FProceduralGaitReachPaceTrial>{Trial});
                        break;
                    }
                }
            }
            auto RejectedReservation = false;
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
                .Travel = FVector::VectorPlaneProject(Velocity, Up),
                .Reach = Reach,
                .RestDrop = ck_procedural_gait::Get_RestDrop(Placement),
                .Probe = &Probe,
                .Step = &Step,
                .Foothold = &InTunables.Get_Foothold(),
                .FootContactRadius = Params.Get_FootContactRadius(),
                .LegIndex = Index,
                .Reservations = Reservations,
                .RejectedReservation = &RejectedReservation};
            const auto SearchesLeft = SearchBudget.RemainingLegs;
            const auto Foothold = ck_procedural_gait::Get_Foothold(FootholdQuery, InGaitComp._Footholds[Index], SearchBudget, RayCount,
                &DebugLeg);
            if (SearchBudget.RemainingLegs < SearchesLeft)
            { LastSearchedLeg = Index; }
            const auto Trusted = Foothold.Source != EProceduralFootholdSource::None;
            const auto TargetIsFoothold = Trusted && Foothold.Source != EProceduralFootholdSource::Ideal;
            // Occupied but otherwise admissible ground is observed terrain, not a missing-ground/airborne signal.
            // Its target remains withheld below until a separate free contact is found.
            const auto ProbeAdvanced = InGaitComp._Probes[Index].Advance(Trusted || RejectedReservation, InDeltaT, Probe.Get_ContactGrace());
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
            const auto GatherWouldOverlap = NOT Trusted && (RejectedReservation
                || NOT Get_IsProceduralFootContactAvailable(Ideal, Params.Get_FootContactRadius(), Index, Reservations));
            const auto TargetValid = ProbeState != EProceduralFootProbeState::Guessing
                && NOT GatherWouldCrossASolid && NOT GatherWouldOverlap;
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
                .Set_PlantCrowded(Planted && NOT Get_IsProceduralFootContactAvailable(
                    LegComp._Foot.Get_Position(), Params.Get_FootContactRadius(), Index, Reservations))
                .Set_TargetIsFoothold(TargetIsFoothold)
                .Set_TargetTrusted(Trusted);

            if (LegComp._Foot.Get_Phase() == ECk_ProceduralLeg_FootPhase::Swinging)
            {
                const auto Foot = LegComp._Foot.Get_Position();
                const auto ClearanceHit = UCk_Utils_JoltQuery_UE::Get_RayCast(World,
                    Foot + Up * Probe.Get_Up(), Foot - Up * Probe.Get_Down(), Probe.Get_QueryFilter());
                ++RayCount;

                if (ck_procedural_gait::Get_TrustedHit(ClearanceHit, -Up))
                { Input.Set_ClearanceGroundZ(InverseBasis.RotateVector(ClearanceHit.Get_Position()).Z); }
            }

            const auto LandingGround = ck_procedural_gait::Get_LandingGround(World, Basis, Hip, Reach,
                InGaitComp._Solver.GetLegState(Index).Get_Swing(), Probe, Step, RayCount, DebugLeg);
            Input.Set_LandingGround(LandingGround.Report)
                .Set_LandingGroundZ(static_cast<float>(InverseBasis.RotateVector(LandingGround.Position).Z));
            DebugLeg.Set_LandingGround(LandingGround.Report);

            LegComp._Foot.Set_Foothold(ck_procedural_gait::Get_LegFoothold(Foothold.Source));
            LegComp._IdealVerdict = ck_procedural_gait::Get_LegFootholdVerdict(Foothold.IdealVerdict);
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
        auto CadenceDriveSpeed = TOptional<float>{};
        if (NOT Airborne && InHandle.Has<FFragment_SurfaceMotion>() && InHandle.Has<FFragment_SurfaceMotion_Support>())
        {
            const auto MotionHandle = UCk_Utils_SurfaceMotion_UE::Cast(InHandle);
            const auto& Support = InHandle.Get<FFragment_SurfaceMotion_Support>();
            if (UCk_Utils_SurfaceMotion_UE::Get_Status(MotionHandle) == ECk_ProceduralAnimation_Status::Ready
                && Support._EvaluatedFrame == GFrameCounter && Support._EvaluatedBody.Equals(Body, 1.0e-3)
                && Support._State.Get_Grounded() && Support._ReachPaceScale < 1.0f
                && Support._AttemptedStanceSpeed > 0.0f
                && (Support._ReachPaceState == ECk_SurfaceMotion_ReachPaceState::Pacing
                    || Support._ReachPaceState == ECk_SurfaceMotion_ReachPaceState::Blocked))
            {
                CadenceDriveSpeed = static_cast<float>(FMath::Max(CadenceSpeed,
                    static_cast<double>(Support._AttemptedStanceSpeed)));
            }
        }
        const auto Solved = InGaitComp._Solver.Step(InDeltaT, CadenceSpeed, PlanarVelocity, Inputs, Outputs,
            Airborne, CadenceDriveSpeed);
        CK_ENSURE_IF_NOT(Solved, TEXT("Procedural gait [{}] solver rejected runtime inputs; feature is failed, planted state retained."), InHandle)
        {
            InHandle.Add<FFragment_ProceduralGait_Failure>(ECk_ProceduralGait_Failure::SolverStep);
            return;
        }

        auto TouchdownRays = int32{0};
        for (auto Index = 0; Index < LegCount; ++Index)
        {
            if (NOT Inputs[Index].Get_Enabled())
            { continue; }

            const auto& Output = Outputs[Index];
            auto& Leg = InGaitComp._Legs[Index];
            auto& LegComp = Leg.Get<FFragment_ProceduralLeg>();
            const auto PreviousPhase = LegComp._Foot.Get_Phase();
            const auto PreviousPosition = LegComp._Foot.Get_Position();

            // The ground under a touchdown decides the plant's trust and normal; the landing report describes the landing
            // point of the solve before.
            if (Output.Get_Planted() && PreviousPhase == ECk_ProceduralLeg_FootPhase::Swinging)
            {
                const auto& Landed = InGaitComp._Solver.GetLegState(Index);
                const auto HipLocal = Leg.Get<FFragment_ProceduralLeg_Params>().Get_Placement().Get_HipLocal();
                const auto PresentedHip = HasPose
                    ? TOptional<FVector>{PresentationBody.TransformPosition(HipLocal)} : TOptional<FVector>{};
                // Include earlier native touchdown corrections; test each hit before the resolver chooses a fallback.
                const auto TouchdownReservations = ck_procedural_gait::Get_FootReservations(
                    InGaitComp._Legs, InGaitComp._Solver, Basis);
                const auto Touchdown = ck_procedural_gait::Get_Touchdown(World, Basis.RotateVector(Landed.Get_Plant().Get_Position()),
                    Basis.RotateVector(Landed.Get_Swing().Get_ValidatedTarget()), Basis.RotateVector(Landed.Get_Swing().Get_TargetNormal()),
                    Up, Probe, Body.TransformPosition(HipLocal), PresentedHip, Inputs[Index].Get_Reach(),
                    Inputs[Index].Get_FootContactRadius(), Index, TouchdownReservations);
                TouchdownRays += Touchdown.Get_Rays();
                InGaitComp._Solver.SetPlantedPose(Index, InverseBasis.RotateVector(Touchdown.Get_Position()),
                    InverseBasis.RotateVector(Touchdown.Get_Normal()).GetSafeNormal(), Touchdown.Get_Trusted());
            }

            // A planted foot reports the ground it touched down on; a swinging one, the target this solve found.
            const auto& Plant = InGaitComp._Solver.GetLegState(Index).Get_Plant();
            const auto ContactTrusted = Output.Get_Planted() ? Plant.Get_Trusted() : Inputs[Index].Get_TargetTrusted();
            LegComp._Foot
                .Set_Position(Basis.RotateVector(Output.Get_Planted() ? Plant.Get_Position() : Output.Get_Position()))
                .Set_Normal(Basis.RotateVector(Output.Get_Planted() ? Plant.Get_Normal() : Output.Get_Normal()))
                .Set_Rotation((Basis * (Output.Get_Planted() ? Plant.Get_Rotation() : Output.Get_Rotation())).GetNormalized())
                .Set_SwingAlpha(Output.Get_SwingAlpha())
                .Set_Phase(Output.Get_Planted() ? ECk_ProceduralLeg_FootPhase::Planted : ECk_ProceduralLeg_FootPhase::Swinging)
                .Set_Contact(ContactTrusted ? ECk_ProceduralLeg_FootContact::Trusted : ECk_ProceduralLeg_FootContact::Guessed);
            InDebugComp._ScratchLegs[Index].Get_Foot().Set_ContactTrusted(ContactTrusted);

            ck_procedural_gait::DoPublish_FootPhaseChange(Leg, LegComp, PreviousPhase, PreviousPosition, Dt);
        }
        InDebugComp._RaysLastSolve += TouchdownRays;

        DoUpdate_FeetPlane(InHandle, InDeltaT, InTunables, InGaitComp, Inputs, Body, InFeetPlaneComp);

        if (InDeltaT > FCk_Time{})
        {
            ++InGaitComp._SolveSequence;
            auto& Stance = InGaitComp._ReachStance;
            Stance._Anchors.Reset();
            Stance._BodyAtSolve = Body;
            Stance._SolveSequence = InGaitComp._SolveSequence;
            Stance._HasSample = true;
            for (auto Index = 0; Index < LegCount; ++Index)
            {
                const auto& Leg = InGaitComp._Legs[Index];
                if (NOT Inputs[Index].Get_Enabled() || NOT Outputs[Index].Get_Planted() || ck::Is_NOT_Valid(Leg)
                    || Leg.Has<FTag_DestroyEntity_Initiate>())
                { continue; }

                const auto& Foot = Leg.Get<FFragment_ProceduralLeg>()._Foot;
                if (Foot.Get_Contact() != ECk_ProceduralLeg_FootContact::Trusted)
                { continue; }

                Stance._Anchors.Add(FProceduralGaitReachAnchor{Leg, Foot.Get_Position(),
                    Leg.Get<FFragment_ProceduralLeg_Params>().Get_Placement().Get_HipLocal(), Inputs[Index].Get_Reach()});
            }

            for (auto Index = 0; Index < LegCount; ++Index)
            {
                const auto& Output = Outputs[Index];
                const auto& State = InGaitComp._Solver.GetLegState(Index);
                const auto& Leg = InGaitComp._Legs[Index];
                const auto PublishedFoot = ck::IsValid(Leg) && Leg.Has<FFragment_ProceduralLeg>()
                    ? TOptional<FCk_ProceduralLeg_Foot>{Leg.Get<FFragment_ProceduralLeg>().Get_Foot()}
                    : TOptional<FCk_ProceduralLeg_Foot>{};
                InDebugComp._ScratchLegs[Index].Get_Foot().Set_PlantedPosition(Basis.RotateVector(State.Get_Plant().Get_Position()))
                    .Set_SwingTarget(Basis.RotateVector(State.Get_Swing().Get_Target()))
                    .Set_Position(PublishedFoot.IsSet() ? PublishedFoot->Get_Position() : Basis.RotateVector(Output.Get_Position()))
                    .Set_Rotation(PublishedFoot.IsSet() ? PublishedFoot->Get_Rotation()
                        : (Basis * Output.Get_Rotation()).GetNormalized())
                    .Set_Normal(PublishedFoot.IsSet() ? PublishedFoot->Get_Normal() : Basis.RotateVector(Output.Get_Normal()))
                    .Set_Planted(PublishedFoot.IsSet() ? PublishedFoot->Get_Phase() == ECk_ProceduralLeg_FootPhase::Planted
                        : Output.Get_Planted())
                    .Set_SwingAlpha(PublishedFoot.IsSet() ? PublishedFoot->Get_SwingAlpha() : Output.Get_SwingAlpha())
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
                .Set_RaysLastSolve(InDebugComp._RaysLastSolve)
                .Set_FeetPlane(InFeetPlaneComp.Get_State())
                .Set_FeetPlanePoint(InFeetPlaneComp.Get_Support().Get_Point())
                .Set_FeetPlaneNormal(InFeetPlaneComp.Get_Support().Get_Normal())
                .Set_FeetSupport(InFeetPlaneComp.Get_Support());
            Snapshot.Set_Legs(InDebugComp._ScratchLegs);

            if (InHandle.Has<FFragment_SurfaceMotion>() && InHandle.Has<FFragment_SurfaceMotion_Support>())
            {
                const auto& Motion = InHandle.Get<FFragment_SurfaceMotion>();
                const auto& MotionSupport = InHandle.Get<FFragment_SurfaceMotion_Support>();
                const auto& Support = MotionSupport._State;
                const auto MotionHandle = UCk_Utils_SurfaceMotion_UE::CastChecked(InHandle);
                Snapshot.Get_Motion().Set_Velocity(Support.Get_Velocity())
                    .Set_RequestedDirection(Motion._Direction)
                    .Set_RequestedSpeed(Motion._Speed)
                    .Set_Grounded(Support.Get_Grounded())
                    .Set_TrustedContact(Support.Get_ContactTrusted())
                    .Set_MissingContact(Support.Get_MissingContact())
                    .Set_ContactSource(UCk_Utils_SurfaceMotion_UE::Get_ContactSource(MotionHandle))
                    .Set_HeightSource(UCk_Utils_SurfaceMotion_UE::Get_HeightSource(MotionHandle))
                    .Set_CandidateNormal(Support.Get_CandidateNormal())
                    .Set_CandidateSeen(Support.Get_CandidateSeen())
                    .Set_WallPolicy(UCk_Utils_SurfaceMotion_UE::Get_WallPolicy(MotionHandle))
                    .Set_MaxStepHeight(UCk_Utils_SurfaceMotion_UE::Get_MaxStepHeight(MotionHandle))
                    .Set_Obstruction(UCk_Utils_SurfaceMotion_UE::Get_Obstruction(MotionHandle))
                    .Set_ObstructionNormal(UCk_Utils_SurfaceMotion_UE::Get_ObstructionNormal(MotionHandle))
                    .Set_ReachPaceScale(MotionSupport.Get_ReachPaceScale())
                    .Set_ReachPaceState(MotionSupport.Get_ReachPaceState())
                    .Set_ReachPaceTrials(MotionSupport.Get_ReachPaceTrials())
                    .Set_ReachPaceRays(MotionSupport.Get_ReachPaceRays());
                Snapshot.Get_Gait().Set_SupportNormal(Support.Get_SupportNormal());
            }
        }

        InGaitComp._Basis = Basis;
    }

    auto
        FProcessor_ProceduralGait_Update::
        DoUpdate_FeetPlane(
            HandleType InHandle,
            TimeType InDeltaT,
            const FFragment_ProceduralGait_Tunables& InTunables,
            const FFragment_ProceduralGait& InGaitComp,
            TArrayView<const FProceduralGaitLegInput> InInputs,
            const FTransform& InBody,
            FFragment_ProceduralGait_FeetPlane& InOutFeetPlane)
        -> void
    {
        const auto Ground = ck_procedural_gait::Get_FeetPlaneGround(InGaitComp._Solver, InInputs);
        const auto Fit = ck_procedural_gait::Get_FeetPlane(Ground, InBody);
        const auto GroundFinite = Fit.Result != EProceduralFeetPlaneResult::Malformed;
        CK_ENSURE_IF_NOT(GroundFinite, TEXT("Procedural gait [{}] solved a non-finite plant or landing point; it publishes no feet plane."), InHandle)
        {
            InOutFeetPlane = FFragment_ProceduralGait_FeetPlane{};
            return;
        }

        if (Fit.Result == EProceduralFeetPlaneResult::Fitted)
        {
            InOutFeetPlane._Support = Fit.Support;
            InOutFeetPlane._State = EProceduralGaitFeetPlane::Fitted;
            InOutFeetPlane._SinceFit = FCk_Time{};
            return;
        }

        const auto SinceFit = InOutFeetPlane._SinceFit + InDeltaT;
        const auto Holds = Fit.Result == EProceduralFeetPlaneResult::Underdetermined
            && InOutFeetPlane._State != EProceduralGaitFeetPlane::None
            && SinceFit <= InTunables.Get_Timing().Get_StepDuration() * ck_procedural_gait::FeetPlaneHoldStepDurations;
        if (NOT Holds)
        {
            InOutFeetPlane = FFragment_ProceduralGait_FeetPlane{};
            return;
        }

        InOutFeetPlane._State = EProceduralGaitFeetPlane::Held;
        InOutFeetPlane._SinceFit = SinceFit;
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
