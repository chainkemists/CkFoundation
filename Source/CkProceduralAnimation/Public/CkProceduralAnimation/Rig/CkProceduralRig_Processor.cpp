#include "CkProceduralAnimation/Rig/CkProceduralRig_Processor.h"

#include "CkProceduralAnimation/Gait/CkProceduralGait_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include <FABRIK.h>
#include <TwoBoneIK.h>

CK_REGISTER_PROCESSOR(ck::FProcessor_ProceduralRig_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_ProceduralRig_Update);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_procedural_rig
{
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
            TArray<FVector>& InOutJoints,
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
        const auto Gait = UCk_Utils_ProceduralGait_UE::Cast(Body);
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
        const auto Hip = BodyTransform.TransformPosition(InLegParams.Get_Placement().Get_HipLocal());
        const auto Pole = BodyTransform.TransformPosition(Chain.Get_PoleLocal());
        auto PoleDirection = (Pole - Hip).GetSafeNormal();
        if (PoleDirection.IsNearlyZero())
        { PoleDirection = BodyTransform.GetRotation().GetAxisZ(); }

        auto& Joints = InRigComp._Joints;
        Joints[0] = Hip;

        switch (Segments.Num())
        {
            case 1:
            {
                Joints[1] = Target;
                break;
            }
            case 2:
            {
                constexpr auto AllowStretching = false;
                const auto SeedJoint = Hip + PoleDirection * Lengths[0];
                auto Joint = SeedJoint;
                auto End = Target;
                AnimationCore::SolveTwoBoneIK(Hip, SeedJoint, Target, Pole, Target, Joint, End,
                    Lengths[0], Lengths[1], AllowStretching, 1.0, 1.0);
                Joints[1] = Joint;
                Joints[2] = End;
                break;
            }
            default:
            {
                ck_procedural_rig::DoSolve_Fabrik(Joints, Lengths, PoleDirection, Target);
                break;
            }
        }

        for (auto Index = 0; Index < Segments.Num(); ++Index)
        {
            const auto& From = Joints[Index];
            const auto& To = Joints[Index + 1];
            ck_procedural_rig::Request_Pose(Segments[Index], (From + To) * 0.5,
                ck_procedural_rig::Get_SegmentRotation(From, To, Pole));
        }

        if (HasFoot)
        { ck_procedural_rig::Request_Pose(InParams.Get_Foot(), Joints.Last(), Foot.Get_Rotation()); }

        InRigComp._PosedSolveSequence = Gait.Get<FFragment_ProceduralGait>()._SolveSequence;
    }
}

// --------------------------------------------------------------------------------------------------------------------
