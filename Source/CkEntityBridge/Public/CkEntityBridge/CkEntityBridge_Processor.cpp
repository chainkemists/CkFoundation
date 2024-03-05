#include "CkEntityBridge_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkEntityBridge/CkEntityBridge_Log.h"
#include "CkNet/CkNet_Utils.h"

#include "CkVariables/CkUnrealVariables_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_EntityBridge_HandleRequests::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::Tick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_EntityBridge_HandleRequests::
        ForEachEntity(
            TimeType,
            HandleType InHandle,
            FFragment_EntityBridge_Requests& InRequests)
        -> void
    {
        DoHandleRequest(InHandle, InRequests.Get_Request());

        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
    }

    auto
        FProcessor_EntityBridge_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_EntityBridge_SpawnEntity& InRequest)
        -> void
    {
        const auto& EntityConfig = InRequest.Get_EntityConfig();

        CK_ENSURE_IF_NOT(ck::IsValid(EntityConfig),
            TEXT("EntityConfig is INVALID. Unable to handle Request for [{}]"), InHandle)
        { return; }

        const auto& NewEntityLifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);

        if (UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(NewEntityLifetimeOwner))
        {
            entity_bridge::Verbose(TEXT("Aborting Spawn Entity process since the New Entity's Lifetime Owner is PendingKill"));
            return;
        }

        auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(NewEntityLifetimeOwner);

        UCk_Utils_Net_UE::Copy(NewEntityLifetimeOwner, NewEntity);

        if (InRequest.Get_PreBuildFunc())
        { InRequest.Get_PreBuildFunc() (NewEntity); }

        EntityConfig->Build(NewEntity, InRequest.Get_OptionalBuildParams());

        if (InRequest.Get_PostSpawnFunc())
        { InRequest.Get_PostSpawnFunc()(NewEntity); }

        entity_bridge::VeryVerbose(TEXT("[EntityBridge] Built new Entity [{}] from Entity Config PDA [{}] by Request from [{}]"),
            NewEntity, EntityConfig, InHandle);

        UUtils_Signal_OnEntitySpawned::Broadcast(InHandle, MakePayload(NewEntity,
            UCk_Utils_Variables_InstancedStruct_UE::Get(InHandle, FGameplayTag::EmptyTag)));
    }
}

// --------------------------------------------------------------------------------------------------------------------
