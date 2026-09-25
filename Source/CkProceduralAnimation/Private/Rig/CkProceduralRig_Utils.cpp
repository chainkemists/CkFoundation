#include "CkProceduralAnimation/Rig/CkProceduralRig_Utils.h"
#include "CkProceduralAnimation/Rig/CkProceduralRig_Fragment.h"
#include "CkProceduralAnimation/Gait/CkProceduralGait_Utils.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_ProceduralRig_UE, FCk_Handle_ProceduralRig,
    ck::FFragment_ProceduralRig_Params, ck::FFragment_ProceduralRig_Current);

auto
    UCk_Utils_ProceduralRig_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_ProceduralRig_ParamsData& InParams)
    -> FCk_Handle_ProceduralRig
{
    const auto CompositionValid = ck::IsValid(InHandle) && NOT InHandle.Has<ck::FTag_DestroyEntity_Initiate>()
        && UCk_Utils_EntityLifetime_UE::Get_CanCreateEntity(InHandle)
        && UCk_Utils_Transform_UE::Has(InHandle) && UCk_Utils_ProceduralGait_UE::Has(InHandle) && NOT Has(InHandle);
    CK_ENSURE_IF_NOT(CompositionValid, TEXT("Procedural rig needs a live gait/transform entity with no existing rig feature."))
    { return {}; }
    if (NOT CompositionValid)
    { return {}; }
    const auto GaitLegs = UCk_Utils_ProceduralGait_UE::Get_Legs(UCk_Utils_ProceduralGait_UE::CastChecked(InHandle));
    auto Current = ck::FFragment_ProceduralRig_Current{};
    auto Ids = TSet<FName>{};
    auto Parts = TSet<FCk_Handle>{};
    const auto Body = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(UCk_Utils_Transform_UE::CastChecked(InHandle));
    auto Valid = NOT InParams.Get_Legs().IsEmpty() && Body.GetScale3D().Equals(FVector::OneVector);
    for (const auto& Leg : InParams.Get_Legs())
    {
        const auto Index = GaitLegs.IndexOfByPredicate([&](const auto& InLeg) { return InLeg.Get_Id() == Leg.Get_Id(); });
        Valid &= Index != INDEX_NONE && NOT Ids.Contains(Leg.Get_Id())
            && FMath::IsFinite(Leg.Get_UpperLength()) && Leg.Get_UpperLength() > 0.0f
            && FMath::IsFinite(Leg.Get_LowerLength()) && Leg.Get_LowerLength() > 0.0f
            && NOT Leg.Get_PoleLocal().ContainsNaN();
        Ids.Add(Leg.Get_Id());
        Current._GaitLegIndices.Add(Index);
        Current._HasFoot.Add(Leg.Get_Foot() != FCk_Handle_Transform{});
        const FCk_Handle_Transform LimbParts[] = {Leg.Get_Upper(), Leg.Get_Lower(), Leg.Get_Foot()};
        for (auto PartIndex = 0; PartIndex < 3; ++PartIndex)
        {
            const auto& Part = LimbParts[PartIndex];
            if (PartIndex == 2 && Part == FCk_Handle_Transform{})
            { continue; }
            const auto PartValid = ck::IsValid(Part) && NOT Part.Has<ck::FTag_DestroyEntity_Initiate>()
                && UCk_Utils_Transform_UE::Has(Part)
                && UCk_Utils_EntityLifetime_UE::Get_CanCreateEntity(Part)
                && Part != InHandle && NOT Parts.Contains(Part);
            Valid &= PartValid;
            if (PartValid)
            { Valid &= UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(Part) == InHandle; }
            Parts.Add(Part);
        }
    }
    CK_ENSURE_IF_NOT(Valid, TEXT("Procedural rig admission requires unit root scale, unique owned transform parts and matching stable gait leg IDs."))
    { return {}; }
    if (NOT Valid)
    { return {}; }
    InHandle.Add<ck::FFragment_ProceduralRig_Params>(InParams);
    InHandle.Add<ck::FFragment_ProceduralRig_Current>(MoveTemp(Current));
    return CastChecked(InHandle);
}

auto
    UCk_Utils_ProceduralRig_UE::
    Get_Failure(
        const FCk_Handle_ProceduralRig& InHandle)
    -> ECk_ProceduralRig_Failure
{
    return ck::IsValid(InHandle) && Has(InHandle)
        ? InHandle.Get<ck::FFragment_ProceduralRig_Current>()._Failure : ECk_ProceduralRig_Failure::MissingPart;
}

auto
    UCk_Utils_ProceduralRig_UE::
    Get_IsReady(
        const FCk_Handle_ProceduralRig& InHandle)
    -> bool
{
    return ck::IsValid(InHandle) && Has(InHandle) && InHandle.Get<ck::FFragment_ProceduralRig_Current>()._Ready;
}
