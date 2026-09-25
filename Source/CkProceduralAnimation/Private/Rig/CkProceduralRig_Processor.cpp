#include "CkProceduralAnimation/Rig/CkProceduralRig_Processor.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkCore/Ensure/CkEnsure.h"
#include <TwoBoneIK.h>

CK_REGISTER_PROCESSOR(ck::FProcessor_ProceduralRig_Update);

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
        auto Pose = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(InPart);
        Pose.SetLocation(InLocation);
        Pose.SetRotation(InRotation);
        UCk_Utils_Transform_UE::Request_SetTransform(InPart, FCk_Request_Transform_SetTransform{Pose}, {});
    }
}

namespace ck
{
    auto
        FProcessor_ProceduralRig_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProceduralRig_Params& InParams,
            FFragment_ProceduralRig_Current& InCurrent,
            const FFragment_ProceduralGait_Params& InGaitParams,
            const FFragment_ProceduralGait_Current& InGait,
            const FFragment_Transform& InTransform)
        -> void
    {
        InCurrent._Ready = false;
        if (InCurrent._Failure != ECk_ProceduralRig_Failure::None || NOT InGait._Ready)
        { return; }
        const auto& Body = InTransform.Get_Transform();
        // Bone lengths are authored in world centimetres; a scaled root requires explicitly
        // reauthoring the rig, not a silent mismatch between IK lengths and visible geometry.
        if (NOT Body.GetScale3D().Equals(FVector::OneVector))
        {
            InCurrent._Failure = ECk_ProceduralRig_Failure::InvalidRootScale;
            CK_ENSURE_IF_NOT(false, TEXT("Procedural rig root changed to unsupported nonunit scale; rig has failed."))
            { return; }
            return;
        }
        for (auto Index = 0; Index < InParams.Get_Legs().Num(); ++Index)
        {
            const auto& Leg = InParams.Get_Legs()[Index];
            const auto GaitIndex = InCurrent._GaitLegIndices[Index];
            const auto PartsValid = ck::IsValid(Leg.Get_Upper()) && ck::IsValid(Leg.Get_Lower())
                && NOT Leg.Get_Upper().Has<FTag_DestroyEntity_Initiate>()
                && NOT Leg.Get_Lower().Has<FTag_DestroyEntity_Initiate>()
                && InGait._Feet.IsValidIndex(GaitIndex)
                && (NOT InCurrent._HasFoot[Index] || (ck::IsValid(Leg.Get_Foot())
                    && NOT Leg.Get_Foot().Has<FTag_DestroyEntity_Initiate>()));
            if (NOT PartsValid)
            {
                InCurrent._Failure = ECk_ProceduralRig_Failure::MissingPart;
                CK_ENSURE_IF_NOT(false, TEXT("Procedural rig lost an authored part; rig has failed without partially posing its limbs."))
                { return; }
                return;
            }
        }
        for (auto Index = 0; Index < InParams.Get_Legs().Num(); ++Index)
        {
            const auto& Leg = InParams.Get_Legs()[Index];
            const auto GaitIndex = InCurrent._GaitLegIndices[Index];
            const auto& Foot = InGait._Feet[GaitIndex];
            const auto Hip = Body.TransformPosition(InGaitParams.Get_Legs()[GaitIndex].Get_HipLocal());
            const auto Pole = Body.TransformPosition(Leg.Get_PoleLocal());
            auto PoleDirection = (Pole - Hip).GetSafeNormal();
            if (PoleDirection.IsNearlyZero())
            { PoleDirection = Body.GetRotation().GetAxisZ(); }
            const auto SeedJoint = Hip + PoleDirection * Leg.Get_UpperLength();
            auto Joint = SeedJoint;
            auto End = Foot.Get_Position();
            AnimationCore::SolveTwoBoneIK(Hip, SeedJoint, End, Pole, Foot.Get_Position(), Joint, End,
                Leg.Get_UpperLength(), Leg.Get_LowerLength(), false, 1.0, 1.0);
            ck_procedural_rig::Request_Pose(Leg.Get_Upper(), (Hip + Joint) * 0.5,
                ck_procedural_rig::Get_SegmentRotation(Hip, Joint, Pole));
            ck_procedural_rig::Request_Pose(Leg.Get_Lower(), (Joint + End) * 0.5,
                ck_procedural_rig::Get_SegmentRotation(Joint, End, Pole));
            if (ck::IsValid(Leg.Get_Foot()))
            {
                ck_procedural_rig::Request_Pose(Leg.Get_Foot(), End,
                    Foot.Get_Rotation());
            }
        }
        InCurrent._DebugGaitSequence = InGait._DebugSnapshot.Get_Sequence();
        InCurrent._Ready = true;
    }
}
