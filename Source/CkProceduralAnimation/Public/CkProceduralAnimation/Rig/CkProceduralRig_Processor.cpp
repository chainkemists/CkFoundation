#include "CkProceduralAnimation/Rig/CkProceduralRig_Processor.h"

#include "CkProceduralAnimation/BodyPose/CkProceduralBodyPose_Utils.h"
#include "CkProceduralAnimation/Core/CkProceduralChainClearance.h"
#include "CkProceduralAnimation/Core/CkProceduralLegCurve.h"
#include "CkProceduralAnimation/Gait/CkProceduralGait_Fragment.h"
#include "CkProceduralAnimation/Gait/CkProceduralGait_Utils.h"
#include "CkProceduralAnimation/Leg/CkProceduralLeg_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkJolt/Query/CkJoltQuery_Utils.h"

#include <Engine/World.h>
#include <FABRIK.h>
#include <TwoBoneIK.h>

CK_REGISTER_PROCESSOR(ck::FProcessor_ProceduralRig_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_ProceduralRig_Update);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_procedural_rig
{
    enum class EChainPose : uint8
    {
        Aim,
        TwoBone,
        Fabrik,
        Curve
    };

    // A ray along a link that meets a solid strictly inside the link crosses it. A hit within the last 2 % is the surface
    // the next joint stands on, not a crossing; a hit at the very start is a joint inside a solid, and so a crossing.
    constexpr auto LinkNearEndFraction = 0.02f;
    constexpr auto LinkFarEndFraction = 0.98f;
    // The link rays see only solids, and a drawn body may have no collider, so a knee swivelled far from its pole can fold
    // into it unseen. The slab stands in for the body: the box of its hips in the body frame, a little wider than the hips,
    // which sit on its surface, and a quarter of the leg's rest drop above and below them, the depth the gait's under-body
    // rule uses.
    constexpr auto BodySlabMargin = 5.0;
    constexpr auto BodySlabDepthShareOfRestDrop = 0.25;

    using FChainJoints = TArray<FVector, TInlineAllocator<9>>;

    auto
        Get_ChainPose(
            int32 InSegmentCount,
            ECk_ProceduralRig_ChainSolver InSolver)
        -> EChainPose
    {
        if (InSegmentCount == 1)
        { return EChainPose::Aim; }

        switch (InSolver)
        {
            case ECk_ProceduralRig_ChainSolver::Fabrik:
            { return EChainPose::Fabrik; }
            case ECk_ProceduralRig_ChainSolver::Curve:
            { return EChainPose::Curve; }
            default:
            { return InSegmentCount == 2 ? EChainPose::TwoBone : EChainPose::Curve; }
        }
    }

    auto
        Get_SegmentRotation(
            const FVector& InFrom,
            const FVector& InTo,
            const FVector& InPole)
        -> FQuat
    {
        const auto Along = (InTo - InFrom).GetSafeNormal();
        if (Along.IsNearlyZero())
        { return FQuat::Identity; }

        auto TowardPole = FVector::VectorPlaneProject(InPole - InFrom, Along).GetSafeNormal();
        if (TowardPole.IsNearlyZero())
        { TowardPole = FVector::VectorPlaneProject(FVector::UpVector, Along).GetSafeNormal(); }

        if (TowardPole.IsNearlyZero())
        { TowardPole = FVector::VectorPlaneProject(FVector::RightVector, Along).GetSafeNormal(); }

        return FRotationMatrix::MakeFromXY(Along, TowardPole).ToQuat();
    }

    auto
        Request_Pose(
            FCk_Handle_Transform InPart,
            const FVector& InLocation,
            const FQuat& InRotation)
        -> void
    {
        UCk_Utils_Transform_UE::Request_SetLocationAndRotation(InPart,
            FCk_Request_Transform_SetLocationAndRotation{InLocation, InRotation.Rotator()}, {});
    }

    auto
        Get_IsPartLive(
            const FCk_Handle_Transform& InPart)
        -> bool
    {
        return ck::IsValid(InPart) && NOT InPart.Has<ck::FTag_DestroyEntity_Initiate>();
    }

    auto
        DoSolve_Fabrik(
            TArrayView<FVector> InOutJoints,
            const TArray<float>& InLengths,
            const FVector& InPoleDirection,
            const FVector& InTarget)
        -> void
    {
        constexpr auto Precision = 0.1;
        constexpr auto MaxIterations = int32{10};

        auto Chain = TArray<FFABRIKChainLink>{};
        Chain.Reserve(InLengths.Num() + 1);
        Chain.Emplace(InOutJoints[0], 0.0, 0, 0);

        auto MaximumReach = 0.0;
        for (auto LinkIndex = 1; LinkIndex <= InLengths.Num(); ++LinkIndex)
        {
            const auto Parent = Chain[LinkIndex - 1].Position;
            const auto Length = InLengths[LinkIndex - 1];
            auto Direction = LinkIndex <= InLengths.Num() / 2 ? InPoleDirection : (InTarget - Parent).GetSafeNormal();
            if (Direction.IsNearlyZero())
            { Direction = InPoleDirection; }

            Chain.Emplace(Parent + Direction * Length, Length, LinkIndex, LinkIndex);
            MaximumReach += Length;
        }

        AnimationCore::SolveFabrik(Chain, InTarget, MaximumReach, Precision, MaxIterations);

        for (auto LinkIndex = 0; LinkIndex < Chain.Num(); ++LinkIndex)
        { InOutJoints[LinkIndex] = Chain[LinkIndex].Position; }
    }

    auto
        DoSolve_Aim(
            TArrayView<FVector> InOutJoints,
            float InLength,
            const FVector& InTarget,
            const FVector& InBodyDown)
        -> void
    {
        auto Direction = (InTarget - InOutJoints[0]).GetSafeNormal();
        if (Direction.IsNearlyZero())
        { Direction = InBodyDown; }

        InOutJoints[1] = InOutJoints[0] + Direction * InLength;
    }

    // The core rejects a bend along hip->foot rather than picking a side; the rig supplies the fallbacks, in order.
    auto
        DoSolve_Curve(
            TArrayView<FVector> InOutJoints,
            const TArray<float>& InLengths,
            const FVector& InHip,
            const FVector& InTarget,
            const FVector& InPoleBend,
            const FQuat& InBodyRotation)
        -> bool
    {
        for (const auto& Bend : {InPoleBend, InBodyRotation.GetAxisZ(), InBodyRotation.GetAxisY()})
        {
            if (ck::SolveProceduralLegCurve(InHip, InTarget, Bend, InLengths, InOutJoints))
            { return true; }
        }
        return false;
    }

    // False only when the curve finds no bend it can use.
    auto
        DoPose_Chain(
            EChainPose InPose,
            TArrayView<FVector> OutJoints,
            const TArray<float>& InLengths,
            const FVector& InHip,
            const FVector& InTarget,
            const FVector& InPole,
            const FQuat& InBodyRotation)
        -> bool
    {
        OutJoints[0] = InHip;
        auto PoleDirection = (InPole - InHip).GetSafeNormal();
        if (PoleDirection.IsNearlyZero())
        { PoleDirection = InBodyRotation.GetAxisZ(); }

        switch (InPose)
        {
            case EChainPose::Aim:
            {
                DoSolve_Aim(OutJoints, InLengths[0], InTarget, -InBodyRotation.GetAxisZ());
                return true;
            }
            case EChainPose::TwoBone:
            {
                constexpr auto AllowStretching = false;
                const auto SeedJoint = InHip + PoleDirection * InLengths[0];
                auto Joint = SeedJoint;
                auto End = InTarget;
                AnimationCore::SolveTwoBoneIK(InHip, SeedJoint, InTarget, InPole, InTarget, Joint, End,
                    InLengths[0], InLengths[1], AllowStretching, 1.0, 1.0);
                OutJoints[1] = Joint;
                OutJoints[2] = End;
                return true;
            }
            case EChainPose::Fabrik:
            {
                DoSolve_Fabrik(OutJoints, InLengths, PoleDirection, InTarget);
                return true;
            }
            case EChainPose::Curve:
            {
                return DoSolve_Curve(OutJoints, InLengths, InHip, InTarget, InPole - InHip, InBodyRotation);
            }
        }
        return false;
    }

    auto
        Get_IsLinkCrossing(
            const FCk_Jolt_HitResult& InHit)
        -> bool
    {
        if (NOT InHit.Get_HasHit())
        { return false; }

        const auto Fraction = InHit.Get_Fraction();
        return Fraction <= 0.0f || (Fraction > LinkNearEndFraction && Fraction < LinkFarEndFraction);
    }

    // Empty when no captured leg is live.
    auto
        Get_BodySlab(
            TConstArrayView<FCk_Handle_ProceduralLeg> InCapturedLegs,
            float InRestDrop)
        -> FBox
    {
        auto Hips = FBox{ForceInit};
        for (const auto& Leg : InCapturedLegs)
        {
            if (ck::Is_NOT_Valid(Leg))
            { continue; }

            Hips += Leg.Get<ck::FFragment_ProceduralLeg_Params>().Get_Placement().Get_HipLocal();
        }

        if (NOT Hips.IsValid)
        { return Hips; }

        return Hips.ExpandBy(FVector{BodySlabMargin, BodySlabMargin, BodySlabDepthShareOfRestDrop * InRestDrop});
    }

    // Links counts the links whose ray crosses a solid; LinksOrSlab also counts the links with an interior joint (neither the
    // hip nor the foot) inside the body slab.
    struct FChainCrossings
    {
        int32 Links = 0;
        int32 LinksOrSlab = 0;
    };

    auto
        DoCount_CrossingLinks(
            UWorld* InWorld,
            TArrayView<const FVector> InJoints,
            const FTransform& InDrawnBody,
            const FBox& InBodySlab,
            const FCk_Jolt_QueryFilter& InFilter,
            int32& InOutRayCount)
        -> FChainCrossings
    {
        const auto IsInteriorJointInSlab = [&](int32 InJointIndex) -> bool
        {
            const auto IsInterior = InJointIndex > 0 && InJointIndex + 1 < InJoints.Num();
            return IsInterior && InBodySlab.IsValid
                && InBodySlab.IsInsideOrOn(InDrawnBody.InverseTransformPositionNoScale(InJoints[InJointIndex]));
        };

        auto Crossings = FChainCrossings{};
        for (auto Index = 0; Index + 1 < InJoints.Num(); ++Index)
        {
            const auto Hit = UCk_Utils_JoltQuery_UE::Get_RayCast(InWorld, InJoints[Index], InJoints[Index + 1], InFilter);
            ++InOutRayCount;

            const auto RayCrosses = Get_IsLinkCrossing(Hit);
            if (RayCrosses)
            { ++Crossings.Links; }

            if (RayCrosses || IsInteriorJointInSlab(Index) || IsInteriorJointInSlab(Index + 1))
            { ++Crossings.LinksOrSlab; }
        }
        return Crossings;
    }

    // SwivelDegrees is the angle to try first on the next solve: the angle of a fully clear pose, else 0.
    struct FClearedChain
    {
        bool Posed = false;
        FVector Pole = FVector::ZeroVector;
        float SwivelDegrees = 0.0f;
        int32 CrossingLinks = 0;
        float PoseDegrees = 0.0f;
        FChainCrossings MeasuredCrossings;
    };

    // The fan, from the last clear angle on: the first pose whose every link is clear of solids and of the body slab is kept;
    // when none is, the one with the fewest crossing links, ties to the smaller swivel. The authored pole is judged there by
    // its rays alone, so a rig with the policy never poses worse than one without.
    auto
        DoPose_ClearChain(
            EChainPose InPose,
            TArrayView<FVector> OutJoints,
            const TArray<float>& InLengths,
            const FVector& InHip,
            const FVector& InTarget,
            const FVector& InPole,
            const FQuat& InBodyRotation,
            const FTransform& InDrawnBody,
            const FBox& InBodySlab,
            float InLastClearDegrees,
            UWorld* InWorld,
            const FCk_Jolt_QueryFilter& InFilter,
            int32& InOutRayCount,
            bool InRetainRefinedAngle = false)
        -> FClearedChain
    {
        float Order[UE_ARRAY_COUNT(ck::ProceduralPoleSwivelFanDegrees) + 1];
        auto OrderCount = ck::Get_ProceduralPoleSwivelOrder(InLastClearDegrees, MakeArrayView(Order));
        const auto CanRetain = InRetainRefinedAngle && FMath::IsFinite(InLastClearDegrees)
            && FMath::Abs(InLastClearDegrees) <= 165.0f
            && FMath::IsNearlyEqual(InLastClearDegrees / 15.0f, FMath::RoundToFloat(InLastClearDegrees / 15.0f), 1.0e-3f);
        auto AlreadyOrdered = false;
        for (auto Index = 0; Index < OrderCount; ++Index)
        { AlreadyOrdered |= FMath::IsNearlyEqual(Order[Index], InLastClearDegrees, 1.0e-3f); }
        if (CanRetain && NOT AlreadyOrdered)
        {
            for (auto Index = OrderCount; Index > 0; --Index)
            { Order[Index] = Order[Index - 1]; }
            Order[0] = InLastClearDegrees;
            ++OrderCount;
        }

        auto Candidate = FChainJoints{};
        Candidate.SetNumZeroed(OutJoints.Num());
        auto Kept = FClearedChain{};
        auto KeptCrossings = TNumericLimits<int32>::Max();
        for (auto OrderIndex = 0; OrderIndex < OrderCount; ++OrderIndex)
        {
            const auto Degrees = Order[OrderIndex];
            const auto Pole = ck::ComputeProceduralPoleSwivel(InHip, InTarget, InPole, Degrees);
            if (NOT DoPose_Chain(InPose, Candidate, InLengths, InHip, InTarget, Pole, InBodyRotation))
            { return {}; }

            const auto ChainCrossings = DoCount_CrossingLinks(InWorld, Candidate, InDrawnBody, InBodySlab, InFilter, InOutRayCount);
            const auto IsClear = ChainCrossings.LinksOrSlab == 0;
            const auto IsAuthored = Degrees == 0.0f;
            const auto Crossings = IsAuthored ? ChainCrossings.Links : ChainCrossings.LinksOrSlab;
            const auto Improves = IsClear || Crossings < KeptCrossings
                || (Crossings == KeptCrossings && FMath::Abs(Degrees) < FMath::Abs(Kept.SwivelDegrees));
            if (NOT Improves)
            { continue; }

            for (auto JointIndex = 0; JointIndex < OutJoints.Num(); ++JointIndex)
            { OutJoints[JointIndex] = Candidate[JointIndex]; }

            Kept = FClearedChain{.Posed = true, .Pole = Pole, .SwivelDegrees = Degrees, .CrossingLinks = Crossings,
                .PoseDegrees = Degrees, .MeasuredCrossings = ChainCrossings};
            KeptCrossings = Crossings;
            if (IsClear)
            { break; }
        }

        if (Kept.CrossingLinks > 0)
        { Kept.SwivelDegrees = 0.0f; }

        return Kept;
    }

    struct FBodyCandidate
    {
        FChainJoints Joints;
        FVector Pole = FVector::ZeroVector;
        float Degrees = 0.0f;
        int32 Crossings = 0;
        FChainCrossings MeasuredCrossings;
    };

    struct FBodyChain
    {
        FCk_Handle_ProceduralLeg Leg;
        uint32 StableId = 0;
        FVector Hip = FVector::ZeroVector;
        FVector Target = FVector::ZeroVector;
        FVector AuthoredPole = FVector::ZeroVector;
        EChainPose Pose = EChainPose::Aim;
        FBox BodySlab = FBox{ForceInit};
        TArray<FBodyCandidate, TInlineAllocator<24>> Candidates;
        TArray<ck::FProceduralChainPoseCandidate, TInlineAllocator<24>> CandidateViews;
        int32 Selected = 0;
        int32 SiblingCrossings = 0;
        bool CanSearch = false;
    };

    auto
        DoAdd_BodyCandidate(
            FBodyChain& InOutChain,
            float InDegrees,
            const FQuat& InBodyRotation,
            const FTransform& InDrawnBody,
            UWorld* InWorld,
            const FCk_Jolt_QueryFilter& InFilter,
            int32& InOutRayCount)
        -> void
    {
        if (InOutChain.Candidates.Num() >= 24 || ck::algo::AnyOf(InOutChain.Candidates,
            [&](const FBodyCandidate& InCandidate) -> bool
            { return FMath::IsNearlyEqual(InCandidate.Degrees, InDegrees, 1.0e-3f); }))
        { return; }

        const auto& Lengths = InOutChain.Leg.Get<ck::FFragment_ProceduralLeg_Params>().Get_Chain().Get_SegmentLengths();
        auto Candidate = FBodyCandidate{};
        Candidate.Joints.SetNumZeroed(Lengths.Num() + 1);
        Candidate.Pole = ck::ComputeProceduralPoleSwivel(InOutChain.Hip, InOutChain.Target, InOutChain.AuthoredPole, InDegrees);
        Candidate.Degrees = InDegrees;
        if (NOT DoPose_Chain(InOutChain.Pose, Candidate.Joints, Lengths, InOutChain.Hip, InOutChain.Target,
                Candidate.Pole, InBodyRotation))
        { return; }

        Candidate.MeasuredCrossings = DoCount_CrossingLinks(InWorld, Candidate.Joints, InDrawnBody,
            InOutChain.BodySlab, InFilter, InOutRayCount);
        const auto& Baseline = InOutChain.Candidates[0].MeasuredCrossings;
        if (Candidate.MeasuredCrossings.Links > Baseline.Links
            || Candidate.MeasuredCrossings.LinksOrSlab > Baseline.LinksOrSlab)
        { return; }

        // Retain the legacy authored-pole exemption and diagnostic severity; sibling scoring is a separate measurement.
        Candidate.Crossings = InDegrees == 0.0f ? Candidate.MeasuredCrossings.Links : Candidate.MeasuredCrossings.LinksOrSlab;
        InOutChain.Candidates.Add(MoveTemp(Candidate));
    }
}


// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_ProceduralRig_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProceduralRig_Params& InParams,
            FFragment_ProceduralRig& InRigComp)
        -> void
    {
        // Returning before the tag removal re-arms Setup next tick: a rig stays PendingSetup until its gait is Ready.
        auto Body = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        const auto Gait = UCk_Utils_ProceduralGait_UE::Cast(Body);
        if (UCk_Utils_ProceduralGait_UE::Get_Status(Gait) != ECk_ProceduralAnimation_Status::Ready)
        { return; }

        InRigComp._Joints.SetNum(InParams.Get_Segments().Num() + 1);
        if (NOT InParams.Get_SegmentClearanceRadii().IsEmpty())
        { Body.AddOrGet<FFragment_ProceduralRig_BodyClearance>(); }
        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ProceduralRig_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProceduralGait& InGaitComp,
            FFragment_ProceduralGait_Debug& InDebugComp)
        -> void
    {
        if (UCk_Utils_ProceduralGait_UE::Get_Status(InHandle) != ECk_ProceduralAnimation_Status::Ready)
        { return; }

        const auto Body = InHandle.ConvertToHandle();
        const auto BodyTransform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(UCk_Utils_Transform_UE::CastChecked(Body));
        auto LiveLegs = TArray<FCk_Handle_ProceduralLeg, TInlineAllocator<16>>{};
        for (const auto& Leg : InGaitComp._Legs)
        {
            if (ck::IsValid(Leg) && NOT Leg.Has<FTag_DestroyEntity_Initiate>()
                && Leg.Has<FFragment_ProceduralLeg_Params>() && Leg.Has<FFragment_ProceduralLeg>()
                && UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(Leg) == Body)
            { LiveLegs.Add(Leg); }
        }

        const auto RootScaleValid = BodyTransform.GetScale3D().Equals(FVector::OneVector);
        CK_ENSURE_IF_NOT(RootScaleValid,
            TEXT("Procedural rig body [{}] root changed to unsupported non-unit scale; its rigs have failed."), InHandle)
        {
            for (auto& Leg : LiveLegs)
            {
                if (Leg.Has<FFragment_ProceduralRig>() && NOT Leg.Has<FFragment_ProceduralRig_Failure>())
                { Leg.Add<FFragment_ProceduralRig_Failure>(ECk_ProceduralRig_Failure::InvalidRootScale); }
            }
            return;
        }

        // Compute the whole body's legacy poses before issuing any part transform requests. Setup and BodyPose ran earlier.
        const auto Posed = UCk_Utils_ProceduralBodyPose_UE::Get_Offset(UCk_Utils_ProceduralBodyPose_UE::Cast(Body)) * BodyTransform;
        const auto BodyRotation = BodyTransform.GetRotation();
        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        const auto& Filter = InHandle.Get<FFragment_ProceduralGait_Tunables>().Get_Probe().Get_QueryFilter();
        auto Chains = TArray<ck_procedural_rig::FBodyChain, TInlineAllocator<8>>{};
        auto RayCount = int32{0};
        for (auto StableId = 0; StableId < InGaitComp._Legs.Num(); ++StableId)
        {
            auto Leg = InGaitComp._Legs[StableId];
            if (NOT LiveLegs.Contains(Leg) || NOT Leg.Has<FFragment_ProceduralRig_Params>()
                || NOT Leg.Has<FFragment_ProceduralRig>() || Leg.Has<FTag_ProceduralRig_NeedsSetup>()
                || Leg.Has<FFragment_ProceduralRig_Failure>())
            { continue; }

            const auto& Params = Leg.Get<FFragment_ProceduralRig_Params>();
            const auto& LegParams = Leg.Get<FFragment_ProceduralLeg_Params>();
            const auto& LegComp = Leg.Get<FFragment_ProceduralLeg>();
            const auto& Rig = Leg.Get<FFragment_ProceduralRig>();
            const auto HasFoot = Params.Get_Foot() != FCk_Handle_Transform{};
            const auto PartsValid = algo::AllOf(Params.Get_Segments(), &ck_procedural_rig::Get_IsPartLive)
                && (NOT HasFoot || ck_procedural_rig::Get_IsPartLive(Params.Get_Foot()));
            CK_ENSURE_IF_NOT(PartsValid,
                TEXT("Procedural rig [{}] lost an authored part; rig has failed without partially posing its chain."), Leg)
            {
                Leg.Add<FFragment_ProceduralRig_Failure>(ECk_ProceduralRig_Failure::MissingPart);
                continue;
            }

            auto Chain = ck_procedural_rig::FBodyChain{};
            Chain.Leg = Leg;
            Chain.StableId = static_cast<uint32>(StableId);
            Chain.Hip = Posed.TransformPosition(LegParams.Get_Placement().Get_HipLocal());
            Chain.Target = LegComp.Get_Foot().Get_Position();
            Chain.AuthoredPole = Posed.TransformPosition(LegParams.Get_Chain().Get_PoleLocal());
            Chain.Pose = ck_procedural_rig::Get_ChainPose(Params.Get_Segments().Num(), Params.Get_Solver());
            const auto RestDrop = static_cast<float>(FMath::Abs(LegParams.Get_Placement().Get_HipLocal().Z
                - LegParams.Get_Placement().Get_RestFootLocal().Z));
            Chain.BodySlab = ck_procedural_rig::Get_BodySlab(LiveLegs, RestDrop);
            auto Legacy = ck_procedural_rig::FBodyCandidate{};
            Legacy.Joints.SetNumZeroed(Params.Get_Segments().Num() + 1);
            Legacy.Pole = Chain.AuthoredPole;
            auto ChainPosed = false;
            if (Params.Get_Clearance() == ECk_ProceduralRig_Clearance::Swivel)
            {
                if (ck::Is_NOT_Valid(World))
                { continue; }
                const auto Cleared = ck_procedural_rig::DoPose_ClearChain(Chain.Pose, Legacy.Joints,
                    LegParams.Get_Chain().Get_SegmentLengths(), Chain.Hip, Chain.Target, Chain.AuthoredPole,
                    BodyRotation, Posed, Chain.BodySlab, Rig._SwivelDegrees, World, Filter, RayCount,
                    NOT Params.Get_SegmentClearanceRadii().IsEmpty());
                ChainPosed = Cleared.Posed;
                Legacy.Pole = Cleared.Pole;
                Legacy.Degrees = Cleared.PoseDegrees;
                Legacy.Crossings = Cleared.CrossingLinks;
                Legacy.MeasuredCrossings = Cleared.MeasuredCrossings;
            }
            else
            {
                ChainPosed = ck_procedural_rig::DoPose_Chain(Chain.Pose, Legacy.Joints,
                    LegParams.Get_Chain().Get_SegmentLengths(), Chain.Hip, Chain.Target, Chain.AuthoredPole, BodyRotation);
            }
            CK_ENSURE_IF_NOT(ChainPosed,
                TEXT("Procedural rig [{}] could not pose its curve chain from hip [{}] to foot [{}]; its last pose is retained."),
                Leg, Chain.Hip, Chain.Target)
            { continue; }
            Chain.Candidates.Add(MoveTemp(Legacy));
            Chains.Add(MoveTemp(Chain));
        }
        InDebugComp._RaysLastSolve += RayCount;
        RayCount = 0;
        if (Chains.IsEmpty())
        { return; }

        auto AvoidanceChains = TArray<int32, TInlineAllocator<16>>{};
        for (auto Index = 0; Index < Chains.Num(); ++Index)
        {
            if (NOT Chains[Index].Leg.Get<FFragment_ProceduralRig_Params>().Get_SegmentClearanceRadii().IsEmpty())
            { AvoidanceChains.Add(Index); }
        }

        if (AvoidanceChains.Num() > 1)
        {
            auto& Batch = InHandle.AddOrGet<FFragment_ProceduralRig_BodyClearance>();
            Batch._Choices.Init(0, AvoidanceChains.Num());
            Batch._CrossingLinks.Init(0, AvoidanceChains.Num());
            const auto Select = [&]() -> bool
            {
                auto Legs = TArray<FProceduralChainAvoidanceLeg, TInlineAllocator<16>>{};
                for (auto Slot = 0; Slot < AvoidanceChains.Num(); ++Slot)
                {
                    auto& Chain = Chains[AvoidanceChains[Slot]];
                    Chain.CandidateViews.Reset();
                    for (const auto& Candidate : Chain.Candidates)
                    { Chain.CandidateViews.Emplace(MakeArrayView(Candidate.Joints)); }
                    Legs.Emplace(Chain.StableId, MakeArrayView(Chain.CandidateViews),
                        MakeArrayView(Chain.Leg.Get<FFragment_ProceduralRig_Params>().Get_SegmentClearanceRadii()), Batch._Choices[Slot]);
                }
                auto Outcome = FProceduralChainAvoidanceOutcome{};
                return SelectProceduralChainPoses(Legs, Batch._Scratch, MakeArrayView(Batch._Choices),
                    MakeArrayView(Batch._CrossingLinks), Outcome);
            };

            // First detect overlap on legacy poses. Empty radii never participate; None and disabled chains are fixed obstacles.
            auto ValidSelection = Select();
            CK_ENSURE_IF_NOT(ValidSelection, TEXT("Procedural rig body [{}] rejected its legacy avoidance inputs; poses retained."), InHandle)
            { return; }
            for (auto Slot = 0; Slot < AvoidanceChains.Num(); ++Slot)
            {
                auto& Chain = Chains[AvoidanceChains[Slot]];
                const auto& Params = Chain.Leg.Get<FFragment_ProceduralRig_Params>();
                const auto Enabled = UCk_Utils_ProceduralLeg_UE::Get_EnableDisable(Chain.Leg) == ECk_EnableDisable::Enable;
                Chain.CanSearch = Batch._CrossingLinks[Slot] > 0 && Enabled
                    && Params.Get_Clearance() == ECk_ProceduralRig_Clearance::Swivel;
                if (NOT Chain.CanSearch)
                { continue; }
                for (const auto Degrees : ProceduralPoleSwivelFanDegrees)
                { ck_procedural_rig::DoAdd_BodyCandidate(Chain, Degrees, BodyRotation, Posed, World, Filter, RayCount); }
            }
            InDebugComp._RaysLastSolve += RayCount;
            RayCount = 0;
            ValidSelection = Select();
            CK_ENSURE_IF_NOT(ValidSelection, TEXT("Procedural rig body [{}] rejected its coarse avoidance inputs; poses retained."), InHandle)
            { return; }

            auto Refined = false;
            for (auto Slot = 0; Slot < AvoidanceChains.Num(); ++Slot)
            {
                auto& Chain = Chains[AvoidanceChains[Slot]];
                if (NOT Chain.CanSearch || Batch._CrossingLinks[Slot] == 0)
                { continue; }
                for (const auto Degrees : {15.0f, -15.0f, 45.0f, -45.0f, 75.0f, -75.0f,
                    105.0f, -105.0f, 135.0f, -135.0f, 165.0f, -165.0f})
                { ck_procedural_rig::DoAdd_BodyCandidate(Chain, Degrees, BodyRotation, Posed, World, Filter, RayCount); }
                Refined = true;
            }
            if (Refined)
            {
                InDebugComp._RaysLastSolve += RayCount;
                RayCount = 0;
                ValidSelection = Select();
                CK_ENSURE_IF_NOT(ValidSelection, TEXT("Procedural rig body [{}] rejected its refined avoidance inputs; poses retained."), InHandle)
                { return; }
            }
            InDebugComp._RaysLastSolve += RayCount;
            for (auto Slot = 0; Slot < AvoidanceChains.Num(); ++Slot)
            {
                auto& Chain = Chains[AvoidanceChains[Slot]];
                Chain.Selected = Batch._Choices[Slot];
                Chain.SiblingCrossings = Batch._CrossingLinks[Slot];
            }
        }

        // All choices are final before any requests are queued. Validate every chain's lifetime again before publication.
        for (auto& Chain : Chains)
        {
            if (ck::Is_NOT_Valid(Chain.Leg) || Chain.Leg.Has<FTag_DestroyEntity_Initiate>()
                || UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(Chain.Leg) != Body)
            { return; }
            const auto& Params = Chain.Leg.Get<FFragment_ProceduralRig_Params>();
            const auto HasFoot = Params.Get_Foot() != FCk_Handle_Transform{};
            if (NOT algo::AllOf(Params.Get_Segments(), &ck_procedural_rig::Get_IsPartLive)
                || (HasFoot && NOT ck_procedural_rig::Get_IsPartLive(Params.Get_Foot())))
            { return; }
        }
        for (auto& Chain : Chains)
        {
            const auto& Chosen = Chain.Candidates[Chain.Selected];
            const auto& Params = Chain.Leg.Get<FFragment_ProceduralRig_Params>();
            auto& Rig = Chain.Leg.Get<FFragment_ProceduralRig>();
            Rig._Joints = Chosen.Joints;
            Rig._SwivelDegrees = Chosen.Crossings == 0 ? Chosen.Degrees : 0.0f;
            Rig._CrossingLinks = Chosen.Crossings;
            Rig._SiblingCrossingLinks = Chain.SiblingCrossings;
            Rig._ChainState = Chosen.Crossings > 0 || Chain.SiblingCrossings > 0
                ? ECk_ProceduralRig_ChainState::Crossing : ECk_ProceduralRig_ChainState::Clear;
            for (auto Index = 0; Index < Params.Get_Segments().Num(); ++Index)
            {
                const auto& From = Chosen.Joints[Index];
                const auto& To = Chosen.Joints[Index + 1];
                ck_procedural_rig::Request_Pose(Params.Get_Segments()[Index], (From + To) * 0.5,
                    ck_procedural_rig::Get_SegmentRotation(From, To, Chosen.Pole));
            }
            if (Params.Get_Foot() != FCk_Handle_Transform{})
            { ck_procedural_rig::Request_Pose(Params.Get_Foot(), Chosen.Joints.Last(), Chain.Leg.Get<FFragment_ProceduralLeg>().Get_Foot().Get_Rotation()); }
            Rig._PosedSolveSequence = InGaitComp._SolveSequence;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
