#include "CkAbilityCue_Processor.h"

#include "CkAbility/Cue/CkAbilityCue_Subsystem.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_AbilityCue_Spawn::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AbilityCue_SpawnRequest& InRequest,
            const FGameplayCueParameters& InParams) const
        -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InRequest.Get_WorldContextObject()),
            TEXT("AbilityCue Request Entity [{}] does NOT have a valid WorldContextObject"),
            InHandle)
        { return; }

        const auto SubSystem = InRequest.Get_WorldContextObject()->GetWorld()->GetSubsystem<UCk_AbilityCueReplicator_Subsystem_UE>();
        SubSystem->Request_ExecuteAbilityCue(InParams);
    }
}

// --------------------------------------------------------------------------------------------------------------------