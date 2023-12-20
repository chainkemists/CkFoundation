#include "CkOwningActor_Processors.h"

#include "CkEcs/CkEcsLog.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_OwningActor_Destroy::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_OwningActor_Current& InOwningActorComp) const
        -> void
    {
        const auto& EntityOwningActor = InOwningActorComp.Get_EntityOwningActor().Get();

        if (ck::Is_NOT_Valid(EntityOwningActor))
        { return; }

        ecs::VeryVerbose(TEXT("Destroying Owning Actor [{}] associated with Entity [{}]"), EntityOwningActor, InHandle);

        EntityOwningActor->Destroy();
    }
}

// --------------------------------------------------------------------------------------------------------------------
