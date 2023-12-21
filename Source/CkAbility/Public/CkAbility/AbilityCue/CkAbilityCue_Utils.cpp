#pragma once

#include "CkAbilityCue_Utils.h"

#include "CkAbility/AbilityCue/CkAbilityCue_Fragment.h"
#include "CkAbility/AbilityCue/CkAbilityCue_Fragment_Data.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AbilityCue_UE::
    Request_Spawn_AbilityCue(
        FCk_Handle InHandle,
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
        FCk_Handle InAbilityCueEntity)
    -> FCk_AbilityCue_Params
{
    return InAbilityCueEntity.Get<FCk_AbilityCue_Params>();
}

// --------------------------------------------------------------------------------------------------------------------
