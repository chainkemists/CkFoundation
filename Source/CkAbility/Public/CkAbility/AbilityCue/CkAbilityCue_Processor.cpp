#include "CkAbilityCue_Processor.h"

// needed for non-unity builds
#include <Engine/World.h>

#include "CkAbility/AbilityCue/CkAbilityCue_Subsystem.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_AbilityCue_Spawn::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::Tick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_AbilityCue_Spawn::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AbilityCue_SpawnRequest& InRequest) const
        -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InRequest.Get_WorldContextObject()),
            TEXT("AbilityCue Request Entity [{}] does NOT have a valid WorldContextObject"),
            InHandle)
        { return; }

        const auto SubSystem = InRequest.Get_WorldContextObject()->GetWorld()->GetSubsystem<UCk_AbilityCueReplicator_Subsystem_UE>();

        if (InRequest.Get_Replication() == ECk_Replication::DoesNotReplicate)
        { SubSystem->Request_ExecuteAbilityCue_Local(InRequest.Get_CueName(), InRequest.Get_ReplicatedParams()); }
        else
        { SubSystem->Request_ExecuteAbilityCue(InRequest.Get_CueName(), InRequest.Get_ReplicatedParams()); }

        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------