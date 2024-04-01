#pragma once

#include "CkAbilityCue_Utils.h"

#include "CkAbility/AbilityCue/CkAbilityCue_Fragment.h"
#include "CkAbility/AbilityCue/CkAbilityCue_Fragment_Data.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkNet/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AbilityCue_UE::
    Request_Spawn_AbilityCue(
        const FCk_Handle& InHandle,
        const FCk_Request_AbilityCue_Spawn& InRequest)
    -> void
{
    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);
    RequestEntity.AddOrGet<ck::FFragment_AbilityCue_SpawnRequest>(
        InRequest.Get_AbilityCueName(), InRequest.Get_WorldContextObject(), InRequest.Get_ReplicatedParams());
}

auto
    UCk_Utils_AbilityCue_UE::
    Get_Params(
        const FCk_Handle& InAbilityCueEntity)
    -> FCk_AbilityCue_Params
{
    return InAbilityCueEntity.Get<FCk_AbilityCue_Params>();
}

auto
    UCk_Utils_AbilityCue_UE::
    Make_AbilityCue_Params(
        FVector InLocation,
        FVector InNormal,
        FCk_Handle InInstigator,
        FCk_Handle InEffectCauser)
    -> FCk_AbilityCue_Params
{
    FCk_AbilityCue_Params AbilityCueParams;

    AbilityCueParams.Set_Location(InLocation);
    AbilityCueParams.Set_Normal(InNormal);

    if (ck::IsValid(InInstigator))
    {
        CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_EntityReplication(InInstigator) == ECk_Replication::Replicates,
            TEXT("Constructing AbilityCue Params with Instigator Entity [{}] which is NOT replicated! AbilityCue using these params will NOT work as expected"),
            InInstigator)
        {}

        AbilityCueParams.Set_Instigator(InInstigator);
    }

    if (ck::IsValid(InEffectCauser))
    {
        CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_EntityReplication(InEffectCauser) == ECk_Replication::Replicates,
            TEXT("Constructing AbilityCue Params with EffectCauser Entity [{}] which is NOT replicated! AbilityCue using these params will NOT work as expected"),
            InEffectCauser)
        {}

        AbilityCueParams.Set_EffectCauser(InEffectCauser);
    }

    return AbilityCueParams;
}

// --------------------------------------------------------------------------------------------------------------------
