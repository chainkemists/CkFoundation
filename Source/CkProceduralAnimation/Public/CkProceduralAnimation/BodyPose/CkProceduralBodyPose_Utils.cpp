#include "CkProceduralAnimation/BodyPose/CkProceduralBodyPose_Utils.h"

#include "CkProceduralAnimation/BodyPose/CkProceduralBodyPose_Fragment.h"
#include "CkProceduralAnimation/Gait/CkProceduralGait_Fragment.h"
#include "CkProceduralAnimation/Leg/CkProceduralLeg_Fragment.h"
#include "CkProceduralAnimation/Leg/CkProceduralLeg_Utils.h"
#include "CkProceduralAnimation/Rig/CkProceduralRig_Fragment.h"
#include "CkProceduralAnimation/Rig/CkProceduralRig_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_ProceduralBodyPose_UE, FCk_Handle_ProceduralBodyPose, ck::FFragment_ProceduralBodyPose);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_procedural_body_pose_utils
{
    auto
        Get_IsBoundByAnyRig(
            const FCk_Handle_Transform& InPart,
            const FCk_Handle& InBody)
        -> bool
    {
        return ck::algo::AnyOf(UCk_Utils_ProceduralLeg_UE::Get_Legs(InBody), [&](const FCk_Handle_ProceduralLeg& InLeg) -> bool
        {
            if (NOT UCk_Utils_ProceduralRig_UE::Has(InLeg))
            { return false; }

            const auto& Chain = InLeg.Get<ck::FFragment_ProceduralRig_Params>();
            return Chain.Get_Segments().Contains(InPart) || Chain.Get_Foot() == InPart;
        });
    }

    auto
        Get_IsPresentationAdmissible(
            const FCk_Handle_Transform& InPresentation,
            const FCk_Handle& InBody)
        -> bool
    {
        return ck::IsValid(InPresentation)
            && NOT InPresentation.Has<ck::FTag_DestroyEntity_Initiate>()
            && UCk_Utils_Transform_UE::Has(InPresentation)
            && InPresentation != InBody
            && UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InPresentation) == InBody
            && NOT Get_IsBoundByAnyRig(InPresentation, InBody);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProceduralBodyPose_UE::
    Add(
        FCk_Handle_ProceduralGait& InGait,
        const FCk_ProceduralBodyPose_Spec& InParams)
    -> FCk_Handle_ProceduralBodyPose
{
    const auto GaitValid = ck::IsValid(InGait)
        && NOT InGait.Has<ck::FTag_DestroyEntity_Initiate>()
        && NOT Has(InGait);
    const auto Valid = GaitValid
        && ck::IsValid(InParams)
        && ck_procedural_body_pose_utils::Get_IsPresentationAdmissible(InParams.Get_Presentation(), InGait.ConvertToHandle())
        && ck::algo::AllOf(InGait.Get<ck::FFragment_ProceduralGait>()._Legs, [](const FCk_Handle_ProceduralLeg& InLeg) -> bool
        {
            return ck::IsValid(InLeg) && NOT InLeg.Has<ck::FTag_DestroyEntity_Initiate>();
        });
    CK_ENSURE_IF_NOT(Valid,
        TEXT("Procedural body pose Add rejected gait [{}]. The gait must be live with no body pose; the presentation must be a "
             "live transform entity that is a direct lifetime child of the body, not the body itself and not a rig part; spring "
             "stiffness and mass must be positive, damping and max attitude lag non-negative, collapse drop non-negative and max tilt "
             "within 0..89 degrees; the conform's max tilt must be within 0..89 degrees, its height weight within 0..1 and its max "
             "height non-negative. "
             "Every leg the gait captured must still be live."),
        InGait)
    { return {}; }

    auto HipLocals = ck::algo::Transform<TArray<FVector>>(InGait.Get<ck::FFragment_ProceduralGait>()._Legs,
        [](const FCk_Handle_ProceduralLeg& InLeg) -> FVector
        {
            return InLeg.Get<ck::FFragment_ProceduralLeg_Params>().Get_Placement().Get_HipLocal();
        });

    const auto BodyRotation = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(UCk_Utils_Transform_UE::CastChecked(InGait)).GetRotation();

    InGait.Add<ck::FFragment_ProceduralBodyPose_Params>(InParams.Get_Presentation(), InParams.Get_Spring(), InParams.Get_Support());
    InGait.Add<ck::FFragment_ProceduralBodyPose_SupportLayout>(MoveTemp(HipLocals));
    InGait.Add<ck::FFragment_ProceduralBodyPose>(BodyRotation);

    const auto& Conform = InParams.Get_Conform();
    if (Conform.Get_Mode() == ECk_ProceduralBodyPose_ConformMode::PlantedFeet)
    {
        InGait.Add<ck::FFragment_ProceduralBodyPose_Conform>(Conform.Get_MaxTilt(), Conform.Get_HeightWeight(), Conform.Get_MaxHeight());
        InGait.Add<ck::FFragment_ProceduralBodyPose_ConformState>();
    }

    return CastChecked(InGait);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProceduralBodyPose_UE::
    Get_Status(
        const FCk_Handle_ProceduralBodyPose& InBodyPose)
    -> ECk_ProceduralAnimation_Status
{
    if (ck::Is_NOT_Valid(InBodyPose) || NOT Has(InBodyPose) || InBodyPose.Has<ck::FFragment_ProceduralBodyPose_Failure>())
    { return ECk_ProceduralAnimation_Status::Failed; }

    return ECk_ProceduralAnimation_Status::Ready;
}

auto
    UCk_Utils_ProceduralBodyPose_UE::
    Get_Failure(
        const FCk_Handle_ProceduralBodyPose& InBodyPose)
    -> ECk_ProceduralBodyPose_Failure
{
    return ck::IsValid(InBodyPose) && Has(InBodyPose) && InBodyPose.Has<ck::FFragment_ProceduralBodyPose_Failure>()
        ? InBodyPose.Get<ck::FFragment_ProceduralBodyPose_Failure>().Get_Reason()
        : ECk_ProceduralBodyPose_Failure::None;
}

auto
    UCk_Utils_ProceduralBodyPose_UE::
    Get_Offset(
        const FCk_Handle_ProceduralBodyPose& InBodyPose)
    -> FTransform
{
    return Get_Status(InBodyPose) == ECk_ProceduralAnimation_Status::Ready
        ? InBodyPose.Get<ck::FFragment_ProceduralBodyPose>().Get_Offset()
        : FTransform::Identity;
}

auto
    UCk_Utils_ProceduralBodyPose_UE::
    Get_TargetOffset(
        const FCk_Handle_ProceduralBodyPose& InBodyPose)
    -> FTransform
{
    return Get_Status(InBodyPose) == ECk_ProceduralAnimation_Status::Ready
        ? InBodyPose.Get<ck::FFragment_ProceduralBodyPose>().Get_TargetOffset()
        : FTransform::Identity;
}

auto
    UCk_Utils_ProceduralBodyPose_UE::
    Get_Presentation(
        const FCk_Handle_ProceduralBodyPose& InBodyPose)
    -> FCk_Handle_Transform
{
    return ck::IsValid(InBodyPose) && Has(InBodyPose)
        ? InBodyPose.Get<ck::FFragment_ProceduralBodyPose_Params>().Get_Presentation()
        : FCk_Handle_Transform{};
}

// --------------------------------------------------------------------------------------------------------------------
