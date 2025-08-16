#include "CkCue_Processor.h"

#include "CkCue_Utils.h"

#include "CkEcs/ContextOwner/CkContextOwner_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Cue_Execute::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Cue_ExecuteRequest& InRequest) const
        -> void
    {
        const auto ContextEntity = UCk_Utils_ContextOwner_UE::Get_ContextOwner(InHandle);
        DoExecuteCue(InRequest.Get_CueName(), InRequest.Get_SpawnParams(), ContextEntity);

        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
    }

    auto
        FProcessor_Cue_Execute::
        DoExecuteCue(
            const FGameplayTag& InCueName,
            const FInstancedStruct& InSpawnParams,
            const FCk_Handle& InContextEntity)
        -> void
    {
        // First try to find cue in entity-local cue set
        const auto LocalCue = UCk_Utils_Cue_UE::TryGet_Cue(InContextEntity, InCueName);

        if (ck::IsValid(LocalCue))
        {
            // Execute local cue
            UCk_Utils_Cue_UE::Request_Execute_Local(LocalCue, InSpawnParams);
            return;
        }

        // Fall back to global cue database
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InContextEntity);
        const auto CueReplicatorSubsystem = World->GetSubsystem<UCk_CueReplicator_Subsystem_Base_UE>();

        CK_ENSURE_IF_NOT(ck::IsValid(CueReplicatorSubsystem),
            TEXT("CueReplicator Subsystem was INVALID when trying to execute Cue [{}]"), InCueName)
        { return; }

        CueReplicatorSubsystem->Request_ExecuteCue(InCueName, InSpawnParams);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Cue_ExecuteLocal::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Cue_ExecuteRequestLocal& InRequest) const
        -> void
    {
        const auto ContextEntity = UCk_Utils_ContextOwner_UE::Get_ContextOwner(InHandle);

        // First try to find cue in entity-local cue set
        const auto LocalCue = UCk_Utils_Cue_UE::TryGet_Cue(ContextEntity, InRequest.Get_CueName());

        if (ck::IsValid(LocalCue))
        {
            // Execute local cue directly via EntityScript
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(ContextEntity);
            auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(
                World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>());

            const auto CueClass = LocalCue.Get<TSubclassOf<UCk_CueBase_EntityScript>>();
            UCk_Utils_EntityScript_UE::Add(NewEntity, CueClass, InRequest.Get_SpawnParams());

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
            UCk_Utils_Handle_UE::Set_DebugName(NewEntity,
                *ck::Format_UE(TEXT("Local Cue [{}]"), InRequest.Get_CueName()));
#endif
        }
        else
        {
            // Fall back to global cue database
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(ContextEntity);
            const auto CueReplicatorSubsystem = World->GetSubsystem<UCk_CueReplicator_Subsystem_Base_UE>();

            CK_ENSURE_IF_NOT(ck::IsValid(CueReplicatorSubsystem),
                TEXT("CueReplicator Subsystem was INVALID when trying to execute local Cue [{}]"), InRequest.Get_CueName())
            { return; }

            CueReplicatorSubsystem->Request_ExecuteCue_Local(InRequest.Get_CueName(), InRequest.Get_SpawnParams());
        }

        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------
