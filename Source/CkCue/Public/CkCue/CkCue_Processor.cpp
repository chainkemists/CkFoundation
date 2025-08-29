#include "CkCue_Processor.h"

#include "CkCueSubsystem_Base.h"

#include "CkEcs/ContextOwner/CkContextOwner_Utils.h"
#include "CkEcs/DeferredEntity/CkDeferredEntity_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Cue_Execute::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Cue_ExecuteRequest& InRequest)
            -> void
    {
        const auto ContextEntity = UCk_Utils_ContextOwner_UE::Get_ContextOwner(InHandle);

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(ContextEntity);
        const auto CueReplicatorSubsystem = World->GetSubsystem<UCk_CueExecutor_Subsystem_Base_UE>();

        CK_ENSURE_IF_NOT(ck::IsValid(CueReplicatorSubsystem),
            TEXT("CueReplicator Subsystem was INVALID when trying to execute Cue [{}]"), InRequest.Get_CueName())
        { return; }

        CueReplicatorSubsystem->Request_ExecuteCue(ContextEntity, InRequest.Get_CueName(), InRequest.Get_SpawnParams());

        // Complete the deferred entity setup if this is a deferred entity
        if (UCk_Utils_DeferredEntity_UE::Has(InHandle))
        {
            auto DeferredEntity = UCk_Utils_DeferredEntity_UE::CastChecked(InHandle);
            UCk_Utils_DeferredEntity_UE::Request_CompleteSetup(DeferredEntity);
        }

        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Cue_ExecuteLocal::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Cue_ExecuteRequestLocal& InRequest)
            -> void
    {
        const auto ContextEntity = UCk_Utils_ContextOwner_UE::Get_ContextOwner(InHandle);

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(ContextEntity);
        const auto CueReplicatorSubsystem = World->GetSubsystem<UCk_CueExecutor_Subsystem_Base_UE>();

        CK_ENSURE_IF_NOT(ck::IsValid(CueReplicatorSubsystem),
            TEXT("CueReplicator Subsystem was INVALID when trying to execute local Cue [{}]"), InRequest.Get_CueName())
        { return; }

        CueReplicatorSubsystem->Request_ExecuteCue_Local(ContextEntity, InRequest.Get_CueName(), InRequest.Get_SpawnParams());

        // Complete the deferred entity setup if this is a deferred entity
        if (UCk_Utils_DeferredEntity_UE::Has(InHandle))
        {
            auto DeferredEntity = UCk_Utils_DeferredEntity_UE::CastChecked(InHandle);
            UCk_Utils_DeferredEntity_UE::Request_CompleteSetup(DeferredEntity);
        }

        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------
