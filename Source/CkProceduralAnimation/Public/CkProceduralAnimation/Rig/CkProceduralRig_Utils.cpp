#include "CkProceduralAnimation/Rig/CkProceduralRig_Utils.h"

#include "CkProceduralAnimation/Leg/CkProceduralLeg_Utils.h"
#include "CkProceduralAnimation/Rig/CkProceduralRig_Fragment.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_ProceduralRig_UE, FCk_Handle_ProceduralRig,
    ck::FFragment_ProceduralRig_Params, ck::FFragment_ProceduralRig_Current);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_procedural_rig_utils
{
    auto
        Get_IsBoundByAnotherRig(
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
        Get_ArePartsAdmissible(
            const FCk_Handle_ProceduralLeg& InLeg,
            const FCk_Fragment_ProceduralRig_ParamsData& InParams)
        -> bool
    {
        const auto Body = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InLeg);
        if (ck::Is_NOT_Valid(Body))
        { return false; }

        const auto IsPartAdmissible = [&](const FCk_Handle_Transform& InPart) -> bool
        {
            return ck::IsValid(InPart)
                && NOT InPart.Has<ck::FTag_DestroyEntity_Initiate>()
                && UCk_Utils_Transform_UE::Has(InPart)
                && InPart != InLeg.ConvertToHandle()
                && InPart != Body
                && UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InPart) == Body
                && NOT Get_IsBoundByAnotherRig(InPart, Body);
        };

        const auto HasFoot = InParams.Get_Foot() != FCk_Handle_Transform{};
        return ck::algo::AllOf(InParams.Get_Segments(), IsPartAdmissible)
            && (NOT HasFoot || IsPartAdmissible(InParams.Get_Foot()));
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProceduralRig_UE::
    Add(
        FCk_Handle_ProceduralLeg& InLeg,
        const FCk_Fragment_ProceduralRig_ParamsData& InParams)
    -> FCk_Handle_ProceduralRig
{
    const auto LegValid = ck::IsValid(InLeg)
        && NOT InLeg.Has<ck::FTag_DestroyEntity_Initiate>()
        && NOT Has(InLeg);
    const auto Valid = LegValid
        && ck::IsValid(InParams)
        && InParams.Get_Segments().Num() == UCk_Utils_ProceduralLeg_UE::Get_ChainGeometry(InLeg).Get_SegmentLengths().Num()
        && ck_procedural_rig_utils::Get_ArePartsAdmissible(InLeg, InParams);
    CK_ENSURE_IF_NOT(Valid,
        TEXT("Procedural rig Add rejected leg [{}]. The leg must be live with no rig; the chain needs 1..8 unique live segments, "
             "as many as the leg's segment lengths; every segment and the optional foot must carry a transform, must not be the "
             "leg or its body, must be a direct lifetime child of the leg's body and must not be bound by another leg's rig."),
        InLeg)
    { return {}; }

    auto Current = ck::FFragment_ProceduralRig_Current{};
    Current._Joints.SetNum(InParams.Get_Segments().Num() + 1);

    InLeg.Add<ck::FFragment_ProceduralRig_Params>(InParams);
    InLeg.Add<ck::FFragment_ProceduralRig_Current>(MoveTemp(Current));

    return CastChecked(InLeg);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProceduralRig_UE::
    Get_IsReady(
        const FCk_Handle_ProceduralRig& InRig)
    -> bool
{
    return ck::IsValid(InRig) && Has(InRig) && InRig.Get<ck::FFragment_ProceduralRig_Current>()._Ready;
}

auto
    UCk_Utils_ProceduralRig_UE::
    Get_Failure(
        const FCk_Handle_ProceduralRig& InRig)
    -> ECk_ProceduralRig_Failure
{
    return ck::IsValid(InRig) && Has(InRig)
        ? InRig.Get<ck::FFragment_ProceduralRig_Current>()._Failure
        : ECk_ProceduralRig_Failure::MissingPart;
}

auto
    UCk_Utils_ProceduralRig_UE::
    Get_Chain(
        const FCk_Handle_ProceduralRig& InRig)
    -> FCk_Fragment_ProceduralRig_ParamsData
{
    return ck::IsValid(InRig) && Has(InRig)
        ? InRig.Get<ck::FFragment_ProceduralRig_Params>()
        : FCk_Fragment_ProceduralRig_ParamsData{};
}

// --------------------------------------------------------------------------------------------------------------------
