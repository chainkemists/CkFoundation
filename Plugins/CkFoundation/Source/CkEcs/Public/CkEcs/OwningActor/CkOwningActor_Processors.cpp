#include "CkOwningActor_Processors.h"

#include "CkEcs/CkEcsLog.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FCk_Processor_OwningActor_Destroy::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FCk_Fragment_OwningActor_Current& InOwningActorComp) const
        -> void
    {
        const auto& entityOwningActor = InOwningActorComp.Get_EntityOwningActor().Get();

        if (ck::Is_NOT_Valid(entityOwningActor))
        { return; }

        ecs::VeryVerbose(TEXT("Destroying Owning Actor [{}] associated with Entity [{}]"), entityOwningActor, InHandle);

        entityOwningActor->Destroy();
    }
}

// --------------------------------------------------------------------------------------------------------------------
