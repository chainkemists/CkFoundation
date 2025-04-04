#include "CkEntityScript_Processor.h"

#include "CkEntityScript_Utils.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_EntityScript_SpawnEntity_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);
        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_EntityScript_SpawnEntity_HandleRequests::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const FFragment_EntityScript_RequestSpawnEntity& InRequestFragment)
        -> void
    {
        DoHandleRequest(InHandle, InRequestFragment);
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
    }

    auto
        FProcessor_EntityScript_SpawnEntity_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_EntityScript_SpawnEntity& InRequest)
        -> void
    {
        const auto EntityScriptClass = InRequest.Get_EntityScriptClass();

        CK_ENSURE_IF_NOT(ck::IsValid(EntityScriptClass),
            TEXT("Invalid EntityScript supplied, cannot Spawn Entity"))
        { return; }

        const auto OptionalEntityScriptTemplate = InRequest.Get_EntityScriptTemplate();

        const auto& LifetimeOwner = InRequest.Get_Owner();

        const auto& Outer = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(LifetimeOwner);

        const auto& NewEntityScript = UCk_Utils_Object_UE::Request_CreateNewObject<UCk_EntityScript_UE>(Outer,
            EntityScriptClass, OptionalEntityScriptTemplate, nullptr);

        CK_ENSURE_IF_NOT(ck::IsValid(NewEntityScript), TEXT("Failed to Spawn New Entity using EntityScript [{}]"), EntityScriptClass)
        { return; }

        auto NewEntity = InRequest.Get_NewEntity();

        NewEntityScript->_AssociatedEntity = NewEntity;
        NewEntity.Add<ck::FFragment_EntityScript_Current>(NewEntityScript);

        NewEntityScript->Construct(NewEntity);

        InRequest.Get_PostConstruction_Func()(NewEntity);

        if (NewEntityScript->Get_Replication() == ECk_Replication::Replicates)
        {
            // this request is to be handled by the ReplicationDrive
            NewEntity.Add<FCk_Request_EntityScript_Replicate>(InRequest.Get_Owner(), InRequest.Get_EntityScriptClass());
        }

        // TODO: Fire signal
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityScript_BeginPlay::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            FFragment_EntityScript_Current& InCurrent)
        -> void
    {
        const auto& EntityScript = InCurrent.Get_Script().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(EntityScript), TEXT("EntityScript is INVALID for [{}] when attempting to invoke BeginPlay on it"), InHandle)
        { return; }

        EntityScript->BeginPlay();

        UCk_Utils_EntityScript_UE::Request_MarkEntityScript_AsHasBegunPlay(InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityScript_EndPlay::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            FFragment_EntityScript_Current& InCurrent)
        -> void
    {
        const auto& EntityScript = InCurrent.Get_Script().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(EntityScript), TEXT("EntityScript is INVALID for [{}] when attempting to invoke EndPlay on it"), InHandle)
        { return; }

        EntityScript->EndPlay();
    }
}

// --------------------------------------------------------------------------------------------------------------------

