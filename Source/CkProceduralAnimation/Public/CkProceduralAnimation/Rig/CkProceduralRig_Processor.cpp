#include "CkProceduralAnimation/Rig/CkProceduralRig_Processor.h"

#include "CkProceduralAnimation/BodyPose/CkProceduralBodyPose_Utils.h"
#include "CkProceduralAnimation/Core/CkProceduralChainClearance.h"
#include "CkProceduralAnimation/Core/CkProceduralLegCurve.h"
#include "CkProceduralAnimation/Gait/CkProceduralGait_Fragment.h"
#include "CkProceduralAnimation/Gait/CkProceduralGait_Utils.h"

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
            int32& InOutRayCount)
        -> FClearedChain
    {
        float Order[UE_ARRAY_COUNT(ck::ProceduralPoleSwivelFanDegrees)];
        const auto OrderCount = ck::Get_ProceduralPoleSwivelOrder(InLastClearDegrees, MakeArrayView(Order));

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

            Kept = FClearedChain{.Posed = true, .Pole = Pole, .SwivelDegrees = Degrees, .CrossingLinks = Crossings};
            KeptCrossings = Crossings;
            if (IsClear)
            { break; }
        }

        if (Kept.CrossingLinks > 0)
        { Kept.SwivelDegrees = 0.0f; }

        return Kept;
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
        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ProceduralRig_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProceduralRig_Params& InParams,
            FFragment_ProceduralRig& InRigComp,
            const FFragment_ProceduralLeg_Params& InLegParams,
            const FFragment_ProceduralLeg& InLegComp)
        -> void
    {
        auto Body = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        auto Gait = UCk_Utils_ProceduralGait_UE::Cast(Body);
        if (UCk_Utils_ProceduralGait_UE::Get_Status(Gait) != ECk_ProceduralAnimation_Status::Ready)
        { return; }

        const auto BodyTransform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(UCk_Utils_Transform_UE::CastChecked(Body));
        // Bone lengths are authored in world centimetres; a scaled root requires explicitly
        // reauthoring the rig, not a silent mismatch between IK lengths and visible geometry.
        const auto RootScaleValid = BodyTransform.GetScale3D().Equals(FVector::OneVector);
        CK_ENSURE_IF_NOT(RootScaleValid,
            TEXT("Procedural rig [{}] root changed to unsupported non-unit scale; rig has failed."), InHandle)
        {
            InHandle.Add<FFragment_ProceduralRig_Failure>(ECk_ProceduralRig_Failure::InvalidRootScale);
            return;
        }

        // The body pose update ran earlier this frame; the presentation entity's own transform is still a pending request.
        const auto Posed = UCk_Utils_ProceduralBodyPose_UE::Get_Offset(UCk_Utils_ProceduralBodyPose_UE::Cast(Body)) * BodyTransform;

        const auto& Segments = InParams.Get_Segments();
        const auto HasFoot = InParams.Get_Foot() != FCk_Handle_Transform{};
        const auto PartsValid = algo::AllOf(Segments, [](const FCk_Handle_Transform& InPart) -> bool
            {
                return ck_procedural_rig::Get_IsPartLive(InPart);
            })
            && (NOT HasFoot || ck_procedural_rig::Get_IsPartLive(InParams.Get_Foot()));
        CK_ENSURE_IF_NOT(PartsValid,
            TEXT("Procedural rig [{}] lost an authored part; rig has failed without partially posing its chain."), InHandle)
        {
            InHandle.Add<FFragment_ProceduralRig_Failure>(ECk_ProceduralRig_Failure::MissingPart);
            return;
        }

        const auto& Chain = InLegParams.Get_Chain();
        const auto& Lengths = Chain.Get_SegmentLengths();
        const auto& Foot = InLegComp.Get_Foot();
        const auto Target = Foot.Get_Position();
        const auto Hip = Posed.TransformPosition(InLegParams.Get_Placement().Get_HipLocal());
        const auto Pole = Posed.TransformPosition(Chain.Get_PoleLocal());
        const auto BodyRotation = BodyTransform.GetRotation();
        const auto ChainPose = ck_procedural_rig::Get_ChainPose(Segments.Num(), InParams.Get_Solver());

        auto& Joints = InRigComp._Joints;
        auto PosedPole = Pole;
        auto ChainPosed = false;
        if (InParams.Get_Clearance() == ECk_ProceduralRig_Clearance::Swivel)
        {
            auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            if (ck::Is_NOT_Valid(World))
            { return; }

            // The gait update ran earlier this frame and reset the counter; the rig adds its link rays to that solve's.
            auto RayCount = int32{0};
            const auto& Placement = InLegParams.Get_Placement();
            const auto RestDrop = static_cast<float>(FMath::Abs(Placement.Get_HipLocal().Z - Placement.Get_RestFootLocal().Z));
            const auto BodySlab = ck_procedural_rig::Get_BodySlab(Gait.Get<FFragment_ProceduralGait>()._Legs, RestDrop);
            const auto Cleared = ck_procedural_rig::DoPose_ClearChain(ChainPose, Joints, Lengths, Hip, Target, Pole, BodyRotation, Posed, BodySlab,
                InRigComp._SwivelDegrees, World, Gait.Get<FFragment_ProceduralGait_Tunables>().Get_Probe().Get_QueryFilter(), RayCount);
            Gait.Get<FFragment_ProceduralGait_Debug>()._RaysLastSolve += RayCount;

            ChainPosed = Cleared.Posed;
            if (ChainPosed)
            {
                PosedPole = Cleared.Pole;
                InRigComp._SwivelDegrees = Cleared.SwivelDegrees;
                InRigComp._CrossingLinks = Cleared.CrossingLinks;
                InRigComp._ChainState = Cleared.CrossingLinks > 0 ? ECk_ProceduralRig_ChainState::Crossing : ECk_ProceduralRig_ChainState::Clear;
            }
        }
        else
        { ChainPosed = ck_procedural_rig::DoPose_Chain(ChainPose, Joints, Lengths, Hip, Target, Pole, BodyRotation); }

        CK_ENSURE_IF_NOT(ChainPosed,
            TEXT("Procedural rig [{}] could not pose its curve chain from hip [{}] to foot [{}]; the chain keeps its last pose this frame."),
            InHandle, Hip, Target)
        { return; }

        for (auto Index = 0; Index < Segments.Num(); ++Index)
        {
            const auto& From = Joints[Index];
            const auto& To = Joints[Index + 1];
            ck_procedural_rig::Request_Pose(Segments[Index], (From + To) * 0.5,
                ck_procedural_rig::Get_SegmentRotation(From, To, PosedPole));
        }

        if (HasFoot)
        { ck_procedural_rig::Request_Pose(InParams.Get_Foot(), Joints.Last(), Foot.Get_Rotation()); }

        InRigComp._PosedSolveSequence = Gait.Get<FFragment_ProceduralGait>()._SolveSequence;
    }
}

// --------------------------------------------------------------------------------------------------------------------
