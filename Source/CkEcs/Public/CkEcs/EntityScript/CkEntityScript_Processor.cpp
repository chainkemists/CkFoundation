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
        const auto& LifetimeOwner = InRequest.Get_Owner();

        const auto& Outer = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(LifetimeOwner);

        const auto& NewEntityScript = UCk_Utils_Object_UE::Request_CreateNewObject<UCk_EntityScript_UE>(
            Outer, InRequest.Get_EntityScriptClass(), InRequest.Get_EntityScriptTemplate(), nullptr);

        CK_ENSURE_IF_NOT(ck::IsValid(NewEntityScript), TEXT("Failed to Spawn New Entity using EntityScript [{}]"), InRequest.Get_EntityScriptClass())
        { return; }

        auto NewEntity = InRequest.Get_NewEntity();

        NewEntityScript->_AssociatedEntity = NewEntity;
        UCk_Utils_EntityScript_UE::DoAdd(NewEntity, NewEntityScript);

        NewEntityScript->Construct(NewEntity);

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

