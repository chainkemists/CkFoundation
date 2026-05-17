#include "CkAggro_Utils.h"

#include "CkAggro/CkAggroOwner_Fragment.h"
#include "CkAggro/CkAggroOwner_Fragment_Data.h"
#include "CkAggro/CkAggro_Fragment.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Aggro_UE::
    Add(
        FCk_Handle_AggroOwner& InHandle,
        const FCk_Handle& InTarget,
        const FCk_Fragment_Aggro_Params& InParams)
    -> FCk_Handle_Aggro
{
    const auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InTarget, [&](FCk_Handle InNewEntity)
    {
        InNewEntity.Add<ck::FTag_Aggro>();
        ck::UAggroedEntity_Utils::AddOrReplace(InNewEntity, InTarget);
        InNewEntity.Add<ck::FFragment_Aggro_Current>();

        // ConstructionScript is optional. Most callers want the default
        // Aggro entity shape (Tag + Aggroed source ref + Current score
        // fragment, all added above) and skip the script entirely.
        // Request_Construct ensure-fails on a null class — guard explicitly.
        if (const auto& ConstructionScript = InParams.Get_ConstructionScript();
            ck::IsValid(ConstructionScript))
        {
            UCk_Entity_ConstructionScript_PDA::Request_Construct(InNewEntity, ConstructionScript);
        }
    });

    auto AggroHandle = Cast(NewEntity);

    // The Aggro entity is identified by its target (Get_AggroTarget), not by
    // a GameplayLabel — labels are only added if the user-supplied
    // ConstructionScript chooses to add them. Tell the record-connect path
    // explicitly that labels are optional here so a script-less Aggro doesn't
    // ensure-fail the connect.
    RecordOfAggro_Utils::Request_Connect(InHandle, AggroHandle, ECk_Record_LabelRequirementPolicy::Optional);

    // TODO: move this to a processor
    ck::UUtils_Signal_OnNewAggroAdded::Broadcast(InHandle, ck::MakePayload(InHandle, AggroHandle));

    return AggroHandle;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Aggro_UE, FCk_Handle_Aggro, ck::FTag_Aggro)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Aggro_UE::
    Get_AggroTarget(
        const FCk_Handle_Aggro& InAggro)
    -> FCk_Handle
{
    return ck::UAggroedEntity_Utils::Get_StoredEntity(InAggro);
}

auto
    UCk_Utils_Aggro_UE::
    Get_AggroScore(
        const FCk_Handle_Aggro& InAggro)
    -> float
{
    return InAggro.Get<ck::FFragment_Aggro_Current>().Get_Score();
}

auto
    UCk_Utils_Aggro_UE::
    Request_Exclude(
        FCk_Handle_Aggro& InAggro)
    -> FCk_Handle_Aggro
{
    InAggro.AddOrGet<ck::FTag_Aggro_Excluded>();
    InAggro.Get<ck::FFragment_Aggro_Current>() = ck::FFragment_Aggro_Current{std::numeric_limits<float>::max()};
    return InAggro;
}

auto
    UCk_Utils_Aggro_UE::
    Request_Include(
        FCk_Handle_Aggro& InAggro)
    -> FCk_Handle_Aggro
{
    InAggro.Try_Remove<ck::FTag_Aggro_Excluded>();
    return InAggro;
}

// --------------------------------------------------------------------------------------------------------------------
