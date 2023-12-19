#pragma once

#include "CkAbilityCue_Utils.h"

#include "CkAbility/Cue/CkAbilityCue_Fragment.h"
#include "CkAbility/Cue/CkAbilityCue_Fragment_Data.h"

#include "CkActor/ActorModifier/CkActorModifier_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"

#include <GameplayEffectTypes.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AbilityCue_UE::
    Request_Spawn_AbilityCue(
        FCk_Handle InHandle,
        const FCk_Request_AbilityCue_Spawn& InRequest)
    -> FCk_Handle
{
    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);
    RequestEntity.AddOrGet<ck::FFragment_AbilityCue_SpawnRequest>(InRequest.Get_WorldContextObject());

    auto& GameplayCueParams = RequestEntity.AddOrGet<FGameplayCueParameters>();
    GameplayCueParams.OriginalTag = InRequest.Get_AbilityCueName();

    return RequestEntity;
}

auto
    UCk_Utils_AbilityCue_UE::
    Request_ReplicateLocation(
        FCk_Handle InAbilityCueRequestEntity,
        FVector    InLocation)
    -> void
{
    auto& GameplayCueParams = InAbilityCueRequestEntity.AddOrGet<FGameplayCueParameters>();
    GameplayCueParams.Location = InLocation;
}

// --------------------------------------------------------------------------------------------------------------------
